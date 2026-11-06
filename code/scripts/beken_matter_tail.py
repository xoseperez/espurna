# Beken: Matter code in its own flash region ("tail"), so that the Matter image fits 2 MiB boards
#
# The Tuya bootloader of 2 MiB boards installs the application from the 'download' partition (664 KiB, gzip)
# into the 'app' partition (1124 KiB). The Matter image is bigger than both. This build links the code of
# libMatterApp.a at a fixed address in the 'download' partition instead (logical 0x120000 = flash 0x132000):
# - the head (SDK + ESPurna) is the usual OTA image, installed by the bootloader as before
# - the tail is written by ESPurna itself, after the head is installed (ref. arch/beken/matter_beken.cpp)
# Head and tail are halves of one link, the head refuses to start Matter with a tail of another build.
# On 4 MiB boards the same address is inside the 'app' partition, there the whole image is installed at once.
#
# Enable with `custom_matter_tail = yes` in the env. Outputs next to the firmware:
# - raw_firmware.bin / *.rbl / *.uf2: the head only (.matter_tail is left out)
# - matter-tail.bin: the tail container, uploaded through the same web OTA form as the firmware

import os

Import("env")  # noqa: F821

TAIL_ORIGIN = 0x120000  # logical; flash 0x132000, sector aligned (0x120000 * 34 / 32)
TAIL_END_FLASH = 0x1D0000  # 'calibration' of the Tuya 2 MiB layout


def to_logical(offset):
    return offset * 32 // 34


def patch_ldscript(env):
    board = env.BoardConfig()
    path = os.path.join(env.subst("$BUILD_DIR"), board.get("build.ldscript"))
    with open(path, "r") as f:
        text = f.read()

    if ".matter_tail" in text:
        return

    length = (to_logical(TAIL_END_FLASH) - TAIL_ORIGIN) & ~0xFF

    def replace(old, new):
        nonlocal text
        if old not in text:
            raise ValueError(f"{path}: no '{old.strip()}' to patch")
        text = text.replace(old, new, 1)

    replace(
        "\tram   (rw!x)",
        f"\ttail   (rx) : ORIGIN = 0x{TAIL_ORIGIN:06X}, LENGTH = 0x{length:06X}\n\tram   (rw!x)",
    )

    # before every other output section, the first matching input section statement wins
    replace(
        "\t/* instructions go to the text region*/",
        "\t/* Matter code and constants: the tail region, ref. scripts/beken_matter_tail.py */\n"
        "\t.matter_tail :\n"
        "\t{\n"
        "\t\t__matter_tail_start = .;\n"
        "\t\tKEEP(*(.matter_tail_header))\n"
        "\t\t. = ALIGN(4);\n"
        "\t\t__matter_tail_init_start = .;\n"
        "\t\tKEEP(*libMatterApp.a:*(SORT(.init_array.*)))\n"
        "\t\tKEEP(*libMatterApp.a:*(.init_array))\n"
        "\t\t__matter_tail_init_end = .;\n"
        "\t\t*libMatterApp.a:*(.text .text.* .rodata .rodata.* .gnu.linkonce.t* .gnu.linkonce.r*)\n"
        "\t\t. = ALIGN(32);\n"
        "\t\t__matter_tail_end = .;\n"
        "\t} > tail\n\n"
        "\t/* instructions go to the text region*/",
    )

    # build id of the whole link, the head compares it with the one in the tail header
    replace(
        "\t.rodata ALIGN(8) :\n\t{\n",
        "\t.rodata ALIGN(8) :\n\t{\n\t\t__build_id_start = .;\n\t\tKEEP(*(.note.gnu.build-id))\n",
    )

    with open(path, "w") as f:
        f.write(text)


if env.GetProjectOption("custom_matter_tail", "no") in ("yes", "true", "1"):
    patch_ldscript(env)
    env.Append(LINKFLAGS=["-Wl,--build-id=sha1", "-Wl,--cref"])
    # the sources of the project are built with projenv, a copy made before this script ran
    Import("projenv")  # noqa: F821
    for e in (env, projenv):  # noqa: F821
        e.Append(CPPDEFINES=[("MATTER_TAIL_SUPPORT", 1)])
    script = os.path.join(env.subst("$PROJECT_DIR"), "scripts", "beken_link_tail.py")
    # same as ${LTCHIPTOOL} link2bin, through the wrapper that splits the image
    env.Replace(
        LINK=f'"${{LTPYTHONEXE}}" "{script}" 0x{TAIL_ORIGIN:X} -r -i 1 -L "${{LT_DIR}}" link2bin ${{BOARD_JSON}} "" ""'
    )
