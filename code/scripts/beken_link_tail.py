# Beken: `ltchiptool link2bin` for an image with the Matter code in its own flash region (ref. beken_matter_tail.py)
#
#   python beken_link_tail.py <tail origin> <board json of the Quectel layout> <ltchiptool arguments>
#
# Runs ltchiptool as usual, except that
# - the raw binary (and so the .rbl / .uf2 images built from it) leaves out the .matter_tail section
# - matter-tail.bin is written next to the ELF: the tail, its header filled in, encrypted and CRC'd for the
#   flash offset it lives at, in a small container that ESPurna recognizes in the web OTA upload
# - the link fails when code outside libMatterApp (except the Matter glue of ESPurna) uses anything in the tail,
#   such code would run before the tail is checked, or without one
# - one OTA file for every BK7231N bootloader, espurna-beken-matter.bin (ref. arch/beken/ota_beken.cpp): the RBL of the
#   Tuya layout (the image without the tail, as firmware.uf2 has it), the RBL of the Quectel layout (the whole image,
#   the tail included: it lies inside that layout's application partition) and matter-tail.bin
# - espurna-beken-initial-quectel.uf2: the whole image for the Quectel layout, for the first flash over UART (ltchiptool) and
#   for OTA from firmware built for that layout alone

import json
import re
import struct
import subprocess
import sys
import zlib
from datetime import datetime
from io import BytesIO
from os.path import basename, dirname, join

TAIL_ORIGIN = int(sys.argv.pop(1), 16)
QUECTEL_BOARD = sys.argv.pop(1)

TAIL_MAGIC = 0x314C544D  # "MTL1", header at the start of the tail (logical)
TAIL_HEADER = struct.Struct("<IIII20s")

CONTAINER_MAGIC = b"ESPMTAIL"
CONTAINER_HEADER = struct.Struct("<8sIIII20s")
CONTAINER_SIZE = 64

# objects allowed to call into the tail: they only do it after the tail is checked
ALLOWED = re.compile(r"(libMatterApp\.a|arch[/\\]beken[/\\]matter_beken\.cpp\.o)")

from ltchiptool.soc.bk72xx.binary import BK72XXBinary, to_offset  # noqa: E402
from ltchiptool.soc.bk72xx.util import RBL, BekenBinary, DataType, OTACompression, OTAEncryption  # noqa: E402
from ltchiptool.util.toolchain import Toolchain  # noqa: E402

objcopy_original = Toolchain.objcopy


def objcopy_head(self, input, output, sections=None, fmt="binary"):
    if sections or fmt != "binary":
        return objcopy_original(self, input, output, sections, fmt)
    self.cmd("objcopy", ["-R", ".matter_tail", "-O", "binary", input, output]).read()
    return output


def symbols(toolchain, elf, demangle):
    out = {}
    args = ["-C", elf] if demangle else [elf]
    for line in toolchain.cmd("nm", args).readlines():
        line = line.decode(errors="replace").rstrip("\r\n")
        parts = line.split(" ", 2)
        if len(parts) != 3:
            continue
        try:
            out[parts[2]] = int(parts[0], 16)
        except ValueError:
            continue
    return out


def check_references(mapfile, tail_names):
    """--cref table: symbol + defining file, then the referencing files, indented"""
    with open(mapfile, "r", errors="replace") as f:
        text = f.read()

    start = text.find("Cross Reference Table")
    if start < 0:
        raise ValueError(f"{mapfile}: no cross reference table (-Wl,--cref)")

    # symbol line: name (demangled, may have spaces) and the defining file, the last token (paths have no spaces);
    # indented lines after it: the files that reference it
    bad = []
    symbol = None
    seen = 0
    for line in text[start:].splitlines()[3:]:
        if not line.strip():
            continue
        if not line.startswith(" "):
            parts = line.rsplit(None, 1)
            symbol = parts[0].strip() if len(parts) == 2 else line.strip()
            if symbol in tail_names:
                seen += 1
            continue
        ref = line.strip()
        if symbol in tail_names and not ALLOWED.search(ref):
            bad.append((symbol, ref))
    if not seen:
        raise ValueError(f"{mapfile}: no symbol of the tail found in the cross reference table")
    return bad


def make_tail(self, elf):
    toolchain = self.board.toolchain
    folder = dirname(elf)
    raw_path = join(folder, "matter-tail.raw")
    toolchain.cmd("objcopy", ["-j", ".matter_tail", "-O", "binary", elf, raw_path]).read()

    names = symbols(toolchain, elf, False)
    start = names["__matter_tail_start"]
    end = names["__matter_tail_end"]
    if start != TAIL_ORIGIN:
        raise ValueError(f"tail at 0x{start:X}, expected 0x{TAIL_ORIGIN:X}")

    # nothing outside the Matter code may need the tail
    tail = {name for name, addr in names.items() if start <= addr < end}
    tail |= {name for name, addr in symbols(toolchain, elf, True).items() if start <= addr < end}
    tail -= {"__matter_tail_start", "__matter_tail_end", "__matter_tail_init_start", "__matter_tail_init_end"}
    bad = check_references(elf[: -len(".elf")] + ".map", tail)
    if bad:
        for symbol, ref in bad[:40]:
            print(f"ERROR: {ref} uses {symbol}, which is in the Matter tail")
        raise ValueError(f"{len(bad)} references into the Matter tail from outside the Matter code")

    # build id of the link, the head has it in .rodata
    with open(elf[: -len(".elf")] + ".bin", "rb") as f:
        head = f.read()
    offset = names["__build_id_start"] - names["_vector_start"]
    namesz, descsz, kind = struct.unpack_from("<III", head, offset)
    if namesz != 4 or kind != 3 or head[offset + 12 : offset + 16] != b"GNU\0":
        raise ValueError("no GNU build id in the head (-Wl,--build-id)")
    build_id = head[offset + 16 : offset + 16 + descsz][:20].ljust(20, b"\0")

    with open(raw_path, "rb") as f:
        data = bytearray(f.read())
    if len(data) != end - start or len(data) % 32:
        raise ValueError(f"tail is {len(data)} bytes, expected {end - start}")

    body_crc = zlib.crc32(data[TAIL_HEADER.size :]) & 0xFFFFFFFF
    data[0 : TAIL_HEADER.size] = TAIL_HEADER.pack(TAIL_MAGIC, len(data), body_crc, 0, build_id)

    coeffs = self.board["build.bkcrypt_coeffs"] or ("0" * 32)
    bk = BekenBinary(coeffs)
    encrypted = b"".join(bk.crypt(TAIL_ORIGIN, bytes(data)))
    flash = b"".join(bk.crc(encrypted))

    flash_offset = TAIL_ORIGIN * 34 // 32
    header = CONTAINER_HEADER.pack(
        CONTAINER_MAGIC, 1, flash_offset, len(flash), zlib.crc32(flash) & 0xFFFFFFFF, build_id
    ).ljust(CONTAINER_SIZE, b"\xff")

    out_path = join(folder, "matter-tail.bin")
    with open(out_path, "wb") as f:
        f.write(header)
        f.write(flash)

    print(
        f"|-- {basename(out_path)}: {len(data)} bytes of code at 0x{TAIL_ORIGIN:X}, "
        f"{len(flash)} bytes at flash 0x{flash_offset:06X}..0x{flash_offset + len(flash):06X}, "
        f"build id {build_id.hex()[:16]}"
    )

    return build_id, bytes(data[0 : TAIL_HEADER.size])


IMAGE_MAGIC = b"ESPBEKEN"
IMAGE_HEADER = struct.Struct("<8sII20s28x")
IMAGE_PART = struct.Struct("<IIII16x")
PART_RBL_TUYA = 1
PART_RBL_QUECTEL = 2
PART_MATTER_TAIL = 3


def merged(base, extra):
    out = dict(base)
    for key, value in extra.items():
        if isinstance(value, dict) and isinstance(out.get(key), dict):
            out[key] = merged(out[key], value)
        else:
            out[key] = value
    return out


def ota_rbl(board, raw):
    rbl = RBL(
        name="app",
        version=f"{datetime.now():%y.%m.%d}-{board['build.mcu']}",
        encryption=OTAEncryption.AES256,
        compression=OTACompression.GZIP,
    )
    bk = BekenBinary(board["build.bkcrypt_coeffs"] or ("0" * 32))
    return b"".join(bk.ota_package(BytesIO(raw), rbl, key=board["build.bkota.key"], iv=board["build.bkota.iv"]))


def make_image(self, elf, build_id, tail_header):
    toolchain = self.board.toolchain
    folder = dirname(elf)
    base = elf[: -len(".elf")]

    # the whole image, the gap up to the tail filled with zeroes by objcopy
    full_path = join(folder, "raw_firmware_full.bin")
    objcopy_original(toolchain, elf, full_path)
    with open(full_path, "rb") as f:
        full = bytearray(f.read())
    # the header of the tail is filled in by make_tail(), the ELF has zeroes there
    header_at = TAIL_ORIGIN - 0x10000
    full[header_at : header_at + len(tail_header)] = tail_header
    full = bytes(full)

    with open(join(folder, "image_bk7231n_app.ota.rbl"), "rb") as f:
        rbl_tuya = f.read()
    with open(join(folder, "matter-tail.bin"), "rb") as f:
        tail = f.read()

    # the Quectel layout: its board json over the one of this build
    with open(QUECTEL_BOARD, "r") as f:
        quectel = json.load(f)
    board_path = next(arg for arg in sys.argv if arg.endswith(".json"))
    with open(board_path, "r") as f:
        board = merged(json.load(f), quectel)
    quectel_board_path = join(folder, "board-quectel.json")
    with open(quectel_board_path, "w") as f:
        json.dump(board, f, indent=1)

    from ltchiptool import Board  # noqa: E402

    qboard = Board(quectel_board_path)
    rbl_quectel = ota_rbl(qboard, full)

    app_size = int(qboard["build.bkrbl_size_app"], 16)
    if len(full) > app_size:
        raise ValueError(f"the whole image ({len(full)} bytes) does not fit the Quectel layout ({app_size})")

    parts = (
        (PART_RBL_TUYA, rbl_tuya),
        (PART_RBL_QUECTEL, rbl_quectel),
        (PART_MATTER_TAIL, tail),
    )
    head_size = IMAGE_HEADER.size + IMAGE_PART.size * len(parts)
    table = b""
    offset = head_size
    for kind, data in parts:
        table += IMAGE_PART.pack(kind, offset, len(data), zlib.crc32(data) & 0xFFFFFFFF)
        offset += len(data)

    image_path = join(folder, "espurna-beken-matter.bin")
    with open(image_path, "wb") as f:
        f.write(IMAGE_HEADER.pack(IMAGE_MAGIC, 1, len(parts), build_id))
        f.write(table)
        for _, data in parts:
            f.write(data)

    print(
        f"|-- {basename(image_path)}: Tuya {len(rbl_tuya)} B, Quectel {len(rbl_quectel)} B "
        f"(of {qboard.region('download')[1]}), Matter code {len(tail)} B"
    )

    # first flash over UART: the whole image with the CRC of every 32 bytes and the RBL header at the end of the
    # application partition, as elf2bin makes them for a board of that layout
    coeffs = qboard["build.bkcrypt_coeffs"] or ("0" * 32)
    bk = BekenBinary(coeffs)
    app_addr = 0x10000
    rbl = RBL(name="app", version=f"{datetime.now():%y.%m.%d}-{qboard['build.mcu']}", container_size=app_size)
    crc_path = join(folder, "image_quectel_app.crc")
    rblh_path = join(folder, "image_quectel_app.rblh")
    with open(crc_path, "wb") as crc_out, open(rblh_path, "wb") as rblh_out:
        generator = bk.package(BytesIO(full), app_addr, len(full), rbl, partial=True)
        for data_type, data in generator:
            if data_type == DataType.BINARY:
                crc_out.write(data)
            elif data_type == DataType.RBL:
                rblh_out.write(data)

    app_offs = to_offset(app_addr)
    rbl_offs = to_offset(app_size) - 102
    ota_path = join(folder, "image_quectel_app.ota.rbl")
    with open(ota_path, "wb") as f:
        f.write(rbl_quectel)

    # the whole application partition in one piece (image, padding, RBL header at its end): written as one aligned
    # run over UART. The RBL header alone at its unaligned offset fails to write on the Quectel bootloader
    with open(crc_path, "rb") as f:
        crc_data = f.read()
    with open(rblh_path, "rb") as f:
        rblh_data = f.read()
    # rbl_offs counts from the start of the application partition, as ltchiptool's "file+offset=part" does
    padding = rbl_offs - len(crc_data)
    if padding < 0:
        raise ValueError("the image overlaps the RBL header of the Quectel layout")
    app_path = join(folder, "image_quectel_app.0x011000.rbl")
    with open(app_path, "wb") as f:
        f.write(crc_data)
        f.write(bytes([0xFF]) * padding)
        f.write(rblh_data)

    uf2_path = join(folder, "espurna-beken-initial-quectel.uf2")
    subprocess.run(
        [
            sys.executable, "-m", "ltchiptool", "uf2", "write",
            "--output", uf2_path,
            "--board", quectel_board_path,
            "--fw", f"espurna:{datetime.now():%y.%m.%d}",
            "--date", str(int(datetime.now().timestamp())),
            "--legacy",
            f"{app_path}=flasher:app",
            f"{ota_path}=device:download",
        ],
        check=True,
    )
    print(f"|-- {basename(uf2_path)} (app at 0x{app_offs:06X})")


elf2bin_original = BK72XXBinary.elf2bin


def elf2bin(self, input, ota_idx):
    result = elf2bin_original(self, input, ota_idx)
    build_id, tail_header = make_tail(self, input)
    make_image(self, input, build_id, tail_header)
    return result


Toolchain.objcopy = objcopy_head
BK72XXBinary.elf2bin = elf2bin

from ltchiptool.__main__ import cli  # noqa: E402

cli()
