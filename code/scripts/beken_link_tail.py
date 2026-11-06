# Beken: `ltchiptool link2bin` for an image with the Matter code in its own flash region (ref. beken_matter_tail.py)
#
#   python beken_link_tail.py <tail origin> <ltchiptool arguments>
#
# Runs ltchiptool as usual, except that
# - the raw binary (and so the .rbl / .uf2 images built from it) leaves out the .matter_tail section
# - matter-tail.bin is written next to the ELF: the tail, its header filled in, encrypted and CRC'd for the
#   flash offset it lives at, in a small container that ESPurna recognizes in the web OTA upload
# - the link fails when code outside libMatterApp (except the Matter glue of ESPurna) uses anything in the tail,
#   such code would run before the tail is checked, or without one

import re
import struct
import sys
import zlib
from os.path import basename, dirname, join

TAIL_ORIGIN = int(sys.argv.pop(1), 16)

TAIL_MAGIC = 0x314C544D  # "MTL1", header at the start of the tail (logical)
TAIL_HEADER = struct.Struct("<IIII20s")

CONTAINER_MAGIC = b"ESPMTAIL"
CONTAINER_HEADER = struct.Struct("<8sIIII20s")
CONTAINER_SIZE = 64

# objects allowed to call into the tail: they only do it after the tail is checked
ALLOWED = re.compile(r"(libMatterApp\.a|arch[/\\]beken[/\\]matter_beken\.cpp\.o)")

from ltchiptool.soc.bk72xx.binary import BK72XXBinary  # noqa: E402
from ltchiptool.soc.bk72xx.util import BekenBinary  # noqa: E402
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


elf2bin_original = BK72XXBinary.elf2bin


def elf2bin(self, input, ota_idx):
    result = elf2bin_original(self, input, ota_idx)
    make_tail(self, input)
    return result


Toolchain.objcopy = objcopy_head
BK72XXBinary.elf2bin = elf2bin

from ltchiptool.__main__ import cli  # noqa: E402

cli()
