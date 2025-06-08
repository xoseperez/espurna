#!/usr/bin/env python3
# pylint: disable=C0301,C0114,C0116,W0511
# coding=utf-8
# -------------------------------------------------------------------------------
# ESPurna module memory analyzer
# xose.perez@gmail.com
#
# Rewritten for python-3 and changed to use "size" instead of "objdump"
# Based on https://github.com/esp8266/Arduino/pull/6525
# by Maxim Prokhorov <prokhorov.max@outlook.com>
#
# Based on:
# https://github.com/letscontrolit/ESPEasy/blob/mega/memanalyzer.py
# by psy0rz <edwin@datux.nl>
# https://raw.githubusercontent.com/SmingHub/Sming/develop/tools/memanalyzer.py
# by Slavey Karadzhov <slav@attachix.com>
# https://github.com/Sermus/ESP8266_memory_analyzer
# by Andrey Filimonov
#
# -------------------------------------------------------------------------------
#
# When using Windows with non-default installation at the C:\.platformio,
# you would need to specify toolchain path manually. For example:
#
# $ py -3 ://path/to/memanalyzer.py --toolchain-path C:/.platformio/packages/toolchain-xtensa/bin <args>
#
# You could also change the path to platformio binary in a similar fashion:
# $ py -3 ://path/to/memanalyzer.py --platformio-cmd C:/Users/Max/platformio-penv/Scripts/pio.exe
#
# -------------------------------------------------------------------------------

import argparse
import logging
import os
import pathlib
import re
import subprocess
import sys

from collections import OrderedDict
from typing import Optional

__version__ = (0, 4)

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger(__file__)

# -------------------------------------------------------------------------------

# TODO IRAM split is not always 32/32, sometimes it is 48/16
# TODO codepath using these is removed to the time being
#      and overflows are already critical build errors
TOTAL_IRAM = 32786
TOTAL_DRAM = 81920

PWD = pathlib.Path(__file__).resolve().parent

ROOT_PATH = (PWD / "..").resolve()

CACHE_PATH = ROOT_PATH / "test" / "build" / "cache"
BUILD_PATH = ROOT_PATH / ".pio" / "build"
CONFIG_PATH = ROOT_PATH / "espurna" / "config" / "arduino.h"

SIZE = "xtensa-lx106-elf-size"

PLATFORMIO_CMD = "pio"

DEFAULT_ENV = "esp8266-1m-git-base"

SECTIONS = OrderedDict(
    [
        (".data", "Initialized Data (RAM)"),
        (".rodata", "ReadOnly Data (RAM)"),
        (".bss", "Uninitialized Data (RAM)"),
        (".text", "Cached Code (IRAM)"),
        (".irom0.text", "Uncached Code (SPI)"),
    ]
)
DESCRIPTION = "ESPurna Memory Analyzer v{}".format(
    ".".join(str(x) for x in __version__)
)


# -------------------------------------------------------------------------------


def analyze_memory(elf_file, *, size_cmd=SIZE):
    proc = subprocess.Popen(
        [size_cmd, "-A", elf_file.as_posix()],
        stdout=subprocess.PIPE,
        universal_newlines=True,
    )
    lines = proc.stdout.readlines()

    values = {}

    for line in lines:
        words = line.split()
        for name in SECTIONS.keys():
            if line.startswith(name):
                value = values.setdefault(name, 0)
                value += int(words[1])
                values[name] = value
                break

    return values


def make_firmware_paths(envname: str, *, build_path=BUILD_PATH, firmware="firmware") -> tuple[pathlib.Path, pathlib.Path]:
    elf_path = build_path / envname / f"{firmware}.elf"
    bin_path = build_path / envname / f"{firmware}.bin"

    return (elf_path, bin_path)


def run(envname, modules, *, piocmd=PLATFORMIO_CMD, cache_path=CACHE_PATH, silent=True):
    flags = [
        '-DMANUFACTURER=\\"TEST_BUILD\\"',
        f'-DDEVICE=\\"{envname.upper()}\\"',
    ]

    for module, enabled in modules.items():
        flags.append(f"-D{module}_SUPPORT={enabled}")

    os_env = os.environ.copy()
    os_env["PLATFORMIO_BUILD_SRC_FLAGS"] = " ".join(flags)
    os_env["PLATFORMIO_BUILD_CACHE_DIR"] = cache_path.as_posix()

    command = [piocmd, "run"]
    if silent:
        command.append("--silent")
    command.extend(["--environment", envname])

    output = None if not silent else subprocess.DEVNULL

    try:
        subprocess.check_call(
            command, shell=False, env=os_env, stdout=output, stderr=output
        )
    except subprocess.CalledProcessError as e:
        logging.error("unable to build command %s with flags", command, flags)
        raise


def get_available_modules(args) -> OrderedDict[str, int]:
    modules = []

    with args.config_path.open("r") as f:
        for line in f:
            match = re.search(r"(\w*)_SUPPORT", line)
            if not match:
                continue

            modules.append((match.group(1), 0))
    modules.sort(key=lambda item: item[0])

    return OrderedDict(modules)


def parse_commandline_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=DESCRIPTION, formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "-e", "--environment", help="platformio envrionment to use", default=DEFAULT_ENV
    )
    parser.add_argument(
        "--toolchain-path",
        help="where to find the xtensa toolchain binaries",
        default=None,
    )
    parser.add_argument(
        "--platformio-cmd",
        help='"pio" command line (when outside of $PATH)',
        default=PLATFORMIO_CMD,
    )
    parser.add_argument(
        "--cache-path",
        type=pathlib.Path,
        default=CACHE_PATH,
    )
    parser.add_argument(
        "--build-path",
        type=pathlib.Path,
        default=BUILD_PATH,
    )
    parser.add_argument(
        "--config-path",
        type=pathlib.Path,
        default=CONFIG_PATH,
    )
    parser.add_argument(
        "--build",
        action=argparse.BooleanOptionalAction,
        default=True,
    )
    parser.add_argument(
        "--available",
        help="exclude the list of command line modules instead of keeping it in",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--keep-modules",
        help="don't disable module after enabling it",
        action=argparse.BooleanOptionalAction,
        default=False,
    )
    parser.add_argument(
        "--silent",
        help="Silence PlatformIO output",
        action=argparse.BooleanOptionalAction,
        default=True,
    )
    parser.add_argument(
        "--enabled",
        action="append",
        help='Module that is always enabled',
    )
    parser.add_argument(
        "modules",
        nargs="*",
        default=[],
        help='Use specific modules instead of only the default ones.',
    )

    return parser.parse_args()


# -------------------------------------------------------------------------------


class Analyzer:
    """Run platformio and print info about the resulting binary."""

    OUTPUT_FORMAT = "{:<20}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}"
    DELIMETERS = OUTPUT_FORMAT.format(
        "-" * 20, "-" * 15, "-" * 15, "-" * 15, "-" * 15, "-" * 15, "-" * 15, "-" * 15
    )

    class _Enable:
        def __init__(self, analyzer, names: list[str], keep: bool):
            self.analyzer = analyzer
            self.names = names
            self.keep = keep
            self.modules = None

        def __enter__(self):
            self.modules = self.analyzer.modules.copy()
            for name in self.names or self.analyzer.modules.keys():
                self.analyzer.modules[name] = 1
            return self.analyzer

        def __exit__(self, *args, **kwargs):
            if not self.keep:
                self.analyzer.modules = self.modules
            self.modules = None

    def __init__(self, args, modules):
        self._envname = args.environment

        self._build_path = args.build_path
        self._cache_path = args.cache_path
        self._silent = args.silent
        self._keep_modules = args.keep_modules

        self.modules = modules
        self.baseline = None

        self._platformio_cmd = args.platformio_cmd

        if not args.toolchain_path:
            self._size_cmd = SIZE
        else:
            path = pathlib.Path(args.toolchain_path).resolve()
            self._size_cmd = (path / SIZE).as_posix()

    def enable(self, names=()):
        return self._Enable(self, names, keep=self._keep_modules)

    def print(self, *args):
        print(self.OUTPUT_FORMAT.format(*args))

    def print_delimiters(self):
        print(self.DELIMETERS)

    def begin(self):
        self.print(
            "Module",
            "Cache IRAM",
            "Init RAM",
            "R.O. RAM",
            "Uninit RAM",
            "Available RAM",
            "Flash ROM",
            "Binary size",
        )
        self.print(
            "",
            ".text + .text1",
            ".data",
            ".rodata",
            ".bss",
            "heap + stack",
            ".irom0.text",
            "",
        )
        self.print_delimiters()
        self.baseline = self.run()
        self.print_values("Baseline", self.baseline)

    def print_values(self, header, values):
        self.print(
            header,
            values[".text"],
            values[".data"],
            values[".rodata"],
            values[".bss"],
            values["free"],
            values[".irom0.text"],
            values["size"],
        )

    def print_compare(self, header, values):
        self.print(
            header,
            values[".text"] - self.baseline[".text"],
            values[".data"] - self.baseline[".data"],
            values[".rodata"] - self.baseline[".rodata"],
            values[".bss"] - self.baseline[".bss"],
            values["free"] - self.baseline["free"],
            values[".irom0.text"] - self.baseline[".irom0.text"],
            values["size"] - self.baseline["size"],
        )

    def run(self, modules: Optional[OrderedDict[str, int]] = None):
        run(
            self._envname,
            modules or self.modules,
            piocmd=self._platformio_cmd,
            cache_path=self._cache_path,
            silent=self._silent,
        )

        elf_path, bin_path = make_firmware_paths(
            self._envname, build_path=self._build_path
        )
        values = analyze_memory(elf_path, size_cmd=self._size_cmd)

        free = 80 * 1024 - values[".data"] - values[".rodata"] - values[".bss"]
        free = free + (16 - free % 16)
        values["free"] = free

        try:
            values["size"] = bin_path.stat().st_size
        except:
            values["size"] = 0

        return values


def main(args: argparse.Namespace):
    available_modules = get_available_modules(args)

    modules: OrderedDict[str, int] = OrderedDict()
    if args.available:
        modules = available_modules

    for module in args.enabled:
        if not module in available_modules:
            raise ValueError(f'"{module}" is not a valid module name')

        modules[module] = 1

    for module in args.modules:
        if not module in available_modules:
            raise ValueError(f'"{module}" is not a valid module name')

        if args.available:
            del modules[module]
        else:
            modules[module] = 0

    if not args.build:
        print("Selected modules:")
        for module, enabled in modules.items():
            print(f"* {module} => {enabled}")
        print()
        return

    # Build without any modules to get base memory usage
    analyzer = Analyzer(args, modules)
    analyzer.begin()

    # Then, build & compare with specified modules
    results = {}
    for module, enabled in analyzer.modules.items():
        if enabled:
            continue
        with analyzer.enable((module,)):
            results[module] = analyzer.run()
        analyzer.print_compare(module, results[module])

    if analyzer.modules:
        with analyzer.enable():
            total = analyzer.run()

        analyzer.print_delimiters()
        if len(analyzer.modules) > 1:
            analyzer.print_compare("ALL MODULES", total)

        analyzer.print_values("TOTAL", total)


if __name__ == "__main__":
    main(parse_commandline_args())
