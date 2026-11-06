# Beken BK7231N: zero-initialized data of ESPurna and of the Matter library in the TCM, not in the main RAM
#
# The chip has 60 KiB of data TCM next to the 192 KiB main RAM. The linker script of the SDK puts only the .bss of
# the Wi-Fi driver and of the BLE stack there; with BLE off (CFG_SUPPORT_BLE = 0) about 45 KiB of it stay unused,
# while the heap (whatever is left of the main RAM) is what Matter runs out of. Moving .bss here gives the heap that
# much more. Only .bss / COMMON: the boot code zeroes the TCM (_sysboot_tcmbss_init), it copies nothing into it.
#
# Enable with `custom_tcm_bss = yes` in the env. The linker fails with "region `tcm' overflowed" when it does not fit.

import os

Import("env")  # noqa: F821

# input sections already placed by the first matching statement, so these go before the generic .bss rules
PATTERNS = (
    "*libMatterApp.a:*(.bss .bss.* COMMON)",
    "*/src/*.o(.bss .bss.* COMMON)",
)


def patch_ldscript(env):
    board = env.BoardConfig()
    path = os.path.join(env.subst("$BUILD_DIR"), board.get("build.ldscript"))
    with open(path, "r") as f:
        text = f.read()

    marker = "/* bss in the TCM, ref. scripts/beken_tcm.py */"
    if marker in text:
        return

    anchor = "\t.tcm ALIGN(8) :\n\t{\n"
    if anchor not in text:
        raise ValueError(f"{path}: no .tcm section to patch")

    lines = "".join(f"\t\t{pattern}\n" for pattern in PATTERNS)
    text = text.replace(anchor, f"{anchor}\t\t{marker}\n{lines}", 1)

    with open(path, "w") as f:
        f.write(text)


if env.GetProjectOption("custom_tcm_bss", "no") in ("yes", "true", "1"):
    patch_ldscript(env)
