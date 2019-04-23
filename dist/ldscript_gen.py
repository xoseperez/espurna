import os
import argparse
import logging
from collections import defaultdict
from string import Formatter  # vformat / no format_map in py2.7
from math import trunc

FORMAT = '%(asctime)-15s %(message)s'
log = logging.getLogger("ldscript-get")
logging.basicConfig(format=FORMAT)

# mapping from esp8266/tools/boards.txt.py:
# sketch | reserved | empty | spiffs | eeprom | rf-cal | sdk-wifi-settings
# ...... |    4112B | ..... | ...... | ...... |                      16 KB

IROM0_SPI_FLASH_START = 0x40200000
IROM0_RESERVED_SKETCH_SIZE = 0x1010
IROM0_RESERVED_SDK_SIZE = 0x4000

SIZE = {
    512: 0x80000,
    1024: 0x100000,
    2048: 0x200000,
    3072: 0x300000,
    4096: 0x400000
}

# supported sizes
# flash (bytes), spiffs (bytes), eeprom (sectors)
VARIANTS = [
    [SIZE[1024], 0, 1],
    [SIZE[1024], 0, 2],
    [SIZE[2048], SIZE[1024], 4],
    [SIZE[4096], SIZE[1024], 4],
    [SIZE[4096], SIZE[3072], 4]
]

def variant_name(variant):
    tmpl = "{}m{}m{}s"

    size, spiffs, sectors = variant

    size = trunc(size / SIZE[1024])
    spiffs = trunc(spiffs/ SIZE[1024])

    return tmpl.format(size, spiffs, sectors)


TEMPLATE = """
/*
sketch: {size_kb}KB
spiffs: {spiffs_size_kb}KB
eeprom: {eeprom_size_kb}KB
/*
MEMORY
{{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = {size:#x}
}}

PROVIDE ( _SPIFFS_start = {spiffs_start:#x} );
PROVIDE ( _SPIFFS_end = {spiffs_end:#x} );
PROVIDE ( _SPIFFS_page = {spiffs_page:#x} );
PROVIDE ( _SPIFFS_block = {spiffs_block:#x} );

INCLUDE \"{include}\"
"""

def flash_map(flashsize, spiffs, sectors):
    reserved = IROM0_RESERVED_SKETCH_SIZE
    sdk_reserved = IROM0_RESERVED_SDK_SIZE
    eeprom_size = (0x1000 * sectors)

    spiffs_end = IROM0_SPI_FLASH_START + (flashsize - sdk_reserved - eeprom_size)
    spiffs_page = 0x100
    if flashsize <= SIZE[1024]:
        max_upload_size = (flashsize - (spiffs + eeprom_size + sdk_reserved)) - reserved
        spiffs_start = IROM0_SPI_FLASH_START + spiffs_end - spiffs
        spiffs_block = 4096
    else:
        max_upload_size = 1024 * 1024 - reserved
        spiffs_start = IROM0_SPI_FLASH_START + (flashsize - spiffs)
        if spiffs < SIZE[512]:
            spiffs_block = 4096
        else:
            spiffs_block = 8192

    if not spiffs:
        spiffs_block = 0
        spiffs_page = 0
        spiffs_start = spiffs_end

    result = {
        'size': max_upload_size,
        'size_kb': int(max_upload_size / 1024),
        'eeprom_size_kb': int(eeprom_size / 1024),
        'spiffs_size_kb': int(spiffs / 1024),
        'spiffs_start': spiffs_start,
        'spiffs_end': spiffs_end,
        'spiffs_page': spiffs_page,
        'spiffs_block': spiffs_block
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
        f.write(TEMPLATE.format(
            include=ld_include,
            **flash_map(*variant)
        ))

def render_all():
    for variant in VARIANTS:
        render(variant, True)
        render(variant, False)


if __name__ == "__main__":
    variants = {variant_name(x):x for x in VARIANTS}
    choices = ["all"]
    choices.extend(variants.keys())

    parser = argparse.ArgumentParser()
    parser.add_argument("--legacy", action='store_true', default=False)
    parser.add_argument("--verbose", action='store_true', default=False)
    parser.add_argument("variant", choices=choices)

    args = parser.parse_args()
    if args.verbose:
        log.setLevel(logging.DEBUG)

    if args.variant == "all":
        render_all()
    else:
        render(variants[args.variant], args.legacy)
