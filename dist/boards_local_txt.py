# adapted boards.txt.py from esp8266/Arduino
# - single board definition, ldscripts
# - portable boards.local.txt for 2.3.0 and up

import os
import argparse
import sys
import collections

if sys.version < (3, 2):
    import string
    _format = string.Formatter().vformat
    def format_map(tmpl, f_map):
        return _format(tmpl, None, f_map)
else:
    def format_map(tmpl, f_map):
        return tmpl.format_map(f_map)


class VersionedSubstitution(collections.MutableMapping):
    def __init__(self, substitutions, targets, *args, **kwargs):
        self._targets = targets
        self._version = None
        self._store = substitutions.copy()
        self.update(dict(*args, **kwargs))

    def set_version(self, version):
        self._version = version

    def __getitem__(self, key):
        if self._version in self._targets:
            return self._store[key]
        return key

    def __setitem__(self, key, value):
        self._store[key] = value

    def __delitem__(self, key):
        del self._store[key]

    def __iter__(self):
        return iter(self._store)

    def __len__(self):
        return len(self._store)


BOARDS_LOCAL = {
    "global": collections.OrderedDict(
        [("menu.float_support", "scanf and printf float support")]
    ),
    "flash_size": collections.OrderedDict(
        [
            (".menu.{eesz}.1M", "1M (1 EEPROM Sector, no SPIFFS)"),
            (".menu.{eesz}.1M.build.flash_size", "1M"),
            (".menu.{eesz}.1M.build.flash_size_bytes", "0x100000"),
            (".menu.{eesz}.1M.build.flash_ld", "eagle.flash.1m0m1s.ld"),
            (".menu.{eesz}.1M.build.spiffs_pagesize", "256"),
            (".menu.{eesz}.1M.upload.maximum_size", "1023984"),
            (".menu.{eesz}.1M.build.rfcal_addr", "0xFC000"),
            (".menu.{eesz}.2M", "2M (4 EEPROM Sectors, 1M SPIFFS)"),
            (".menu.{eesz}.2M.build.flash_size", "2M"),
            (".menu.{eesz}.2M.build.flash_size_bytes", "0x200000"),
            (".menu.{eesz}.2M.build.flash_ld", "eagle.flash.2m1m4s.ld"),
            (".menu.{eesz}.2M.build.spiffs_pagesize", "256"),
            (".menu.{eesz}.2M.upload.maximum_size", "1044464"),
            (".menu.{eesz}.2M.build.rfcal_addr", "0x1FC000"),
            (".menu.{eesz}.4M1M", "4M (4 EEPROM Sectors, 1M SPIFFS)"),
            (".menu.{eesz}.4M1M.build.flash_size", "4M"),
            (".menu.{eesz}.4M1M.build.flash_size_bytes", "0x400000"),
            (".menu.{eesz}.4M1M.build.flash_ld", "eagle.flash.4m1m4s.ld"),
            (".menu.{eesz}.4M1M.build.spiffs_pagesize", "256"),
            (".menu.{eesz}.4M1M.upload.maximum_size", "1044464"),
            (".menu.{eesz}.4M1M.build.rfcal_addr", "0x3FC000"),
            (".menu.{eesz}.4M3M", "4M (4 EEPROM Sectors, 3M SPIFFS)"),
            (".menu.{eesz}.4M3M.build.flash_size", "4M"),
            (".menu.{eesz}.4M3M.build.flash_size_bytes", "0x400000"),
            (".menu.{eesz}.4M3M.build.flash_ld", "eagle.flash.4m3m4s.ld"),
            (".menu.{eesz}.4M3M.build.spiffs_pagesize", "256"),
            (".menu.{eesz}.4M3M.upload.maximum_size", "1044464"),
            (".menu.{eesz}.4M3M.build.rfcal_addr", "0x3FC000"),
        ]
    ),
    "float_support": collections.OrderedDict(
        [
            (".menu.float_support.disabled", "Disabled (Recommended)"),
            (".menu.float_support.disabled.build.float", ""),
            (".menu.float_support.enabled", "Enabled"),
            (
                ".menu.float_support.enabled.build.float",
                "-u _printf_float -u _scanf_float",
            ),
        ]
    ),
}


BOARD = "generic"
MENUS = ["flash_size", "float_support"]


CORE_VERSIONS = ["2.3.0", "2.4.2", "2.5.0", "2.5.1"]


EXTRA_FLAGS = [
    (".compiler.cpp.extra_flags", "-DNO_GLOBAL_EEPROM -DMQTT_MAX_PACKET_SIZE=400")
]


SUBSTITUTIONS = VersionedSubstitution(
    dict(eesz="FlashSize", wipe="FlashErase", baud="UploadSpeed", vt="VTable"),
    ["2.3.0", "2.4.2"],
)


def generate(versions, directory, sub=SUBSTITUTIONS):

    for version in versions:

        sub.set_version(version)

        result = ["#version={}\n\n".format(version)]
        result.extend("{}={}\n".format(k, v) for k, v in BOARDS_LOCAL["global"].items())
        result.append("\n")

        # print("{} unused:".format(version, set(BOARDS_LOCAL.keys()) - set(MENUS[version])))
        # continue

        for menu in MENUS:
            section = []
            for k, v in BOARDS_LOCAL[menu].items():
                k = format_map(k, sub)
                section.append(BOARD + "=".join([k, v]))
            result.append("\n".join(section))
            result.append("\n\n")

        if EXTRA_FLAGS:
            result.extend(("{}{}={}".format(BOARD, k, v)) for k, v in EXTRA_FLAGS)

        f_path = os.path.join(directory, version, "boards.local.txt")
        f_dir, _ = os.path.split(f_path)
        if not os.path.exists(f_dir):
            os.makedirs(f_dir)

        with open(f_path, "w") as f:
            for part in result:
                f.write(part)
            f.write("\n")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--directory", default="arduino_ide")
    parser.add_argument(
        "--versions", action="store_true", default=False, help="supported versions"
    )
    parser.add_argument("--generate", nargs="*", action="append", default=CORE_VERSIONS)

    args = parser.parse_args()

    if args.versions:
        print("Supported versions:")
        for version in CORE_VERSIONS:
            print(version)
        sys.exit(1)

    if args.generate:
        generate(args.generate, args.directory)
