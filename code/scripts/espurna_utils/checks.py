import os

from .display import Color, clr, print_filler, print_warning


BYTES = 1
KILOBYTES = 1024
MEGABYTES = 1024 * 1024


def Bytes(size):
    return int(size)


def Kilobytes(size):
    """Represent N kilobytes as bytes
    >>> Kilobytes(1)
    1024
    >>> Kilobytes(1024)
    1048576
    """
    return int(KILOBYTES * size)


def Megabytes(size):
    """Represent N megabytes as bytes
    >>> Megabytes(1)
    1048576
    >>> Megabytes(1024)
    1073741824
    """
    return int(MEGABYTES * size)


def humanize(size, *, decimal=False, convert=None):
    """Print something intelligible instead of just the value as-is.
    To use with .ld and menu, also support custom suffixes for ratios
    as [RATIO, SUFFIX] pairs.
    >>> humanize(Bytes(8))
    '8B'
    >>> humanize(Megabytes(1))
    '1MB'
    >>> humanize(Megabytes(1) - Bytes(10))
    '1023KB'
    >>> humanize(Megabytes(1) + Bytes(10))
    '1MB'
    >>> humanize(Megabytes(1) + Kilobytes(512))
    '1536KB'
    >>> humanize(Bytes(7315456))
    '7144KB'
    >>> humanize(Bytes(6266880))
    '6120KB'
    """

    if not convert:
        convert = [
            [Bytes(1), "B"],
            [Kilobytes(1), "KB"],
            [Megabytes(1), "MB"],
        ]

    for ratio, suffix in reversed(convert):
        if size >= ratio:
            if size % ratio > (size / 8):
                continue

            size = f"{size / ratio:.02f}"
            if not decimal:
                size = size[:-3]

            size = size.replace(".00", "")

            return f"{size}{suffix}"

    return ""


def check_env(name, default):
    return os.environ.get(name, default) in ("1", "y", "yes", "true")


def print_size_warning():
    print_filler("*", color=Color.LIGHT_YELLOW, err=True)
    print_warning(
        "File is too large for OTA! Here you can find instructions on how to flash it:"
    )
    print_warning(
        "https://github.com/xoseperez/espurna/wiki/TwoStepUpdates",
        color=Color.LIGHT_CYAN,
    )
    print_filler("*", color=Color.LIGHT_YELLOW, err=True)


FLASH_BLOCK = Kilobytes(4)

def block_aligned_flash_size(size):
    return size + (FLASH_BLOCK - (size % FLASH_BLOCK))


def check_binsize(target, source, env):
    # common report only handles the .elf sizing, which
    # does not include extra space from the actual .bin
    # - bootloader sector at the start
    # - a couple of bytes of extra space after bootloader
    # - optional signature blob at the end
    file_path = target[0].get_abspath()
    file_size = os.stat(file_path).st_size

    print(f"Total binary size: {humanize(file_size)} ({file_size} bytes)")

    flash_size = int(env.BoardConfig().get("upload.maximum_size", 0))
    if not flash_size:
        return

    # RFCAL, SDK and WiFi driver storage space at the end
    flash_size -= 4 * FLASH_BLOCK

    # updater will round our write target up to the block space
    # TODO: use zlib right here? both platform versions already support it
    aligned_size = block_aligned_flash_size(file_size)
    half_size = int(flash_size / 2)

    if aligned_size >= half_size:
        print_size_warning()
