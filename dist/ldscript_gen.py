#!/usr/bin/env python

import os
import argparse
import logging
from collections import defaultdict
from math import trunc

FORMAT = "%(asctime)-15s %(message)s"
log = logging.getLogger("ldscript-get")
logging.basicConfig(format=FORMAT)

# mapping from esp8266/tools/boards.txt.py:
# sketch | reserved | empty |   fs   | eeprom | rf-cal | sdk-wifi-settings
# ...... |    4112B | ..... | ...... | ...... |                      16 KB

IROM0_SPI_FLASH_START = 0x40200000
IROM0_RESERVED_SKETCH_SIZE = 0x1010
IROM0_RESERVED_SDK_SIZE = 0x4000

SIZE = {512: 0x80000, 1024: 0x100000, 2048: 0x200000, 3072: 0x300000, 4096: 0x400000}

# supported sizes
# flash (bytes), fs (bytes), eeprom (sectors)
VARIANTS = [
    [SIZE[512], 0, 1],
    [SIZE[1024], 0, 1],
    [SIZE[1024], 0, 2],
    [SIZE[2048], SIZE[1024], 4],
    [SIZE[4096], SIZE[1024], 4],
    [SIZE[4096], SIZE[3072], 4],
]


def size_suffix(size):
    if size >= SIZE[1024] or not size:
        size = trunc(size / SIZE[1024])
        suffix = "m"
    else:
        size = trunc(size / 1024)
        suffix = "k"

    return size, suffix


def variant_name(variant):
    tmpl = "{flash_size}{flash_suffix}{fs_size}{fs_suffix}{sectors}s"

    flash_size, fs_size, sectors = variant

    flash_size, flash_suffix = size_suffix(flash_size)
    fs_size, fs_suffix = size_suffix(fs_size)

    return tmpl.format(
        flash_size=flash_size,
        flash_suffix=flash_suffix,
        fs_size=fs_size,
        fs_suffix=fs_suffix,
        sectors=sectors,
    )


TEMPLATE = """\
/*
sketch: {size_kb}KB
fs: {fs_size_kb}KB
eeprom: {eeprom_size_kb}KB
*/

MEMORY
{{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = {size:#x}
}}

/*
Provide both _SPIFFS_ and _FS_ to be compatible with 2.3.0...2.6.0+ and
any library that is using old _SPIFFS_...
*/

PROVIDE ( _SPIFFS_start = {fs_start:#x} );
PROVIDE ( _SPIFFS_end = {fs_end:#x} );
PROVIDE ( _SPIFFS_page = {fs_page:#x} );
PROVIDE ( _SPIFFS_block = {fs_block:#x} );

PROVIDE ( _FS_start = _SPIFFS_start );
PROVIDE ( _FS_end = _SPIFFS_end );
PROVIDE ( _FS_page = _SPIFFS_page );
PROVIDE ( _FS_block = _SPIFFS_block );

INCLUDE \"{include}\"
"""


def flash_map(flashsize, fs, sectors):
    reserved = IROM0_RESERVED_SKETCH_SIZE
    sdk_reserved = IROM0_RESERVED_SDK_SIZE
    eeprom_size = 0x1000 * sectors

    fs_end = IROM0_SPI_FLASH_START + (flashsize - sdk_reserved - eeprom_size)
    fs_page = 0x100
    if flashsize <= SIZE[1024]:
        max_upload_size = (flashsize - (fs + eeprom_size + sdk_reserved)) - reserved
        fs_start = IROM0_SPI_FLASH_START + fs_end - fs
        fs_block = 4096
    else:
        max_upload_size = 1024 * 1024 - reserved
        fs_start = IROM0_SPI_FLASH_START + (flashsize - fs)
        if fs < SIZE[512]:
            fs_block = 4096
        else:
            fs_block = 8192

    if not fs:
        fs_block = 0
        fs_page = 0
        fs_start = fs_end

    # Adjust FS_end to be a multiple of the block size
    # ref: https://github.com/esp8266/Arduino/pull/5989
    if fs:
        fs_end = fs_block * ((fs_end - fs_start) // fs_block) + fs_start

    result = {
        "size": max_upload_size,
        "size_kb": int(max_upload_size / 1024),
        "eeprom_size_kb": int(eeprom_size / 1024),
        "fs_size_kb": int((fs_end - fs_start) / 1024),
        "fs_start": fs_start,
        "fs_end": fs_end,
        "fs_page": fs_page,
        "fs_block": fs_block,
    }

    return result


def render(variant, legacy):
    name = variant_name(variant)
    name = "eagle.flash.{}.ld".format(name)

    ld_include = "local.eagle.app.v6.common.ld"
    ld_dir = "ld/latest"
    if legacy:
        ld_include = "eagle.app.v6.common.ld"
        ld_dir = "ld/pre_2.5.0"

    path = os.path.join(ld_dir, name)

    log.info("render %s (INCLUDE %s)", name, ld_include)

    with open(path, "w") as f:
        f.write(TEMPLATE.format(include=ld_include, **flash_map(*variant)))


def render_all():
    for variant in VARIANTS:
        render(variant, True)
        render(variant, False)


if __name__ == "__main__":
    variants = {variant_name(x): x for x in VARIANTS}
    choices = ["all"]
    choices.extend(variants.keys())

    parser = argparse.ArgumentParser()
    parser.add_argument("--legacy", action="store_true", default=False)
    parser.add_argument("--verbose", action="store_true", default=False)
    parser.add_argument("variant", choices=choices)

    args = parser.parse_args()
    if args.verbose:
        log.setLevel(logging.DEBUG)

    if args.variant == "all":
        render_all()
    else:
        render(variants[args.variant], args.legacy)
