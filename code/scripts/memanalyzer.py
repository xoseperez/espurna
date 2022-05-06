#!/usr/bin/env python3
# pylint: disable=C0301,C0114,C0116,W0511
# coding=utf-8
# -------------------------------------------------------------------------------
# ESPurna module memory analyser
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
# $ py -3 scripts\memanalyzer.py --toolchain-prefix C:\.platformio\packages\toolchain-xtensa\bin\xtensa-lx106-elf- <args>
#
# You could also change the path to platformio binary in a similar fashion:
# $ py -3 scripts\memanalyzer.py --platformio-prefix C:\Users\Max\platformio-penv\Scripts\
#
# -------------------------------------------------------------------------------

import argparse
import os
import re
import subprocess
import sys
from collections import OrderedDict
from subprocess import getstatusoutput

__version__ = (0, 3)

# -------------------------------------------------------------------------------

TOTAL_IRAM = 32786
TOTAL_DRAM = 81920

DEFAULT_ENV = "nodemcu-lolin"
TOOLCHAIN_PREFIX = "~/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-"
PLATFORMIO_PREFIX = ""
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


def size_binary_path(prefix):
    return "{}size".format(os.path.expanduser(prefix))


def file_size(file):
    try:
        return os.stat(file).st_size
    except OSError:
        return 0


def analyse_memory(size, elf_file):
    proc = subprocess.Popen(
        [size, "-A", elf_file], stdout=subprocess.PIPE, universal_newlines=True
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


def run(prefix, env, modules, debug):
    flags = " ".join("-D{}_SUPPORT={:d}".format(k, v) for k, v in modules.items())

    os_env = os.environ.copy()
    os_env["PLATFORMIO_BUILD_SRC_FLAGS"] = flags
    os_env["PLATFORMIO_BUILD_CACHE_DIR"] = "test/pio_cache"

    command = [os.path.join(prefix, "platformio"), "run"]
    if not debug:
        command.append("--silent")
    command.extend(["--environment", env])

    output = None if debug else subprocess.DEVNULL

    try:
        subprocess.check_call(
            command, shell=False, env=os_env, stdout=output, stderr=output
        )
    except subprocess.CalledProcessError:
        print(" - Command failed: {}".format(command))
        print(" - Selected flags: {}".format(flags))
        sys.exit(1)


def get_available_modules():
    modules = []
    for line in open("espurna/config/arduino.h"):
        match = re.search(r"(\w*)_SUPPORT", line)
        if match:
            modules.append((match.group(1), 0))
    modules.sort(key=lambda item: item[0])

    return OrderedDict(modules)


def parse_commandline_args():
    parser = argparse.ArgumentParser(
        description=DESCRIPTION, formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "-e", "--environment", help="platformio envrionment to use", default=DEFAULT_ENV
    )
    parser.add_argument(
        "--toolchain-prefix",
        help="where to find the xtensa toolchain binaries",
        default=TOOLCHAIN_PREFIX,
    )
    parser.add_argument(
        "--platformio-prefix",
        help="where to find the platformio executable",
        default=PLATFORMIO_PREFIX,
    )
    parser.add_argument(
        "-c",
        "--core",
        help="use core as base configuration instead of default",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-l",
        "--list",
        help="list available modules",
        action="store_true",
        default=False,
    )
    parser.add_argument("-d", "--debug", action="store_true", default=False)
    parser.add_argument(
        "modules", nargs="*", help="Modules to test (use ALL to test them all)"
    )

    return parser.parse_args()


def size_binary_exists(args):

    status, _ = getstatusoutput(size_binary_path(args.toolchain_prefix))
    if status != 1:
        print("size not found, please check that the --toolchain-prefix is correct")
        sys.exit(1)


def get_modules(args):

    # Load list of all modules
    available_modules = get_available_modules()
    if args.list:
        print("List of available modules:\n")
        for module in available_modules:
            print("* " + module)
        print()
        sys.exit(0)

    modules = []
    if args.modules:
        if "ALL" in args.modules:
            modules.extend(available_modules.keys())
        else:
            modules.extend(args.modules)
    modules.sort()

    # Check test modules exist
    for module in modules:
        if module not in available_modules:
            print("Module {} not found".format(module))
            sys.exit(2)

    # Either use all of modules or specified subset
    if args.core:
        modules = available_modules
    else:
        modules = OrderedDict((x, 0) for x in modules)

    configuration = "CORE" if args.core else "DEFAULT"

    return configuration, modules


# -------------------------------------------------------------------------------


class Analyser:
    """Run platformio and print info about the resulting binary."""

    OUTPUT_FORMAT = "{:<20}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}"
    DELIMETERS = OUTPUT_FORMAT.format(
        "-" * 20, "-" * 15, "-" * 15, "-" * 15, "-" * 15, "-" * 15, "-" * 15, "-" * 15
    )
    FIRMWARE_FORMAT = ".pio/build/{env}/firmware.{suffix}"

    class _Enable:
        def __init__(self, analyser, module=None):
            self.analyser = analyser
            self.module = module

        def __enter__(self):
            if not self.module:
                for name in self.analyser.modules:
                    self.analyser.modules[name] = 1
            else:
                self.analyser.modules[self.module] = 1
            return self.analyser

        def __exit__(self, *args, **kwargs):
            if not self.module:
                for name in self.analyser.modules:
                    self.analyser.modules[name] = 0
            else:
                self.analyser.modules[self.module] = 0

        analyser = None
        module = None

    def __init__(self, args, modules):
        self._debug = args.debug
        self._platformio_prefix = args.platformio_prefix
        self._toolchain_prefix = args.toolchain_prefix
        self._environment = args.environment
        self.modules = modules
        self.baseline = None

    def enable(self, module=None):
        return self._Enable(self, module)

    def print(self, *args):
        print(self.OUTPUT_FORMAT.format(*args))

    def print_delimiters(self):
        print(self.DELIMETERS)

    def begin(self, name):
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
        self.print_values(name, self.baseline)

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

    # TODO: sensor modules need to be print_compared with SENSOR as baseline
    # TODO: some modules need to be print_compared with WEB as baseline
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

    def run(self):
        run(self._platformio_prefix, self._environment, self.modules, self._debug)

        elf_path = self.FIRMWARE_FORMAT.format(env=self._environment, suffix="elf")
        bin_path = self.FIRMWARE_FORMAT.format(env=self._environment, suffix="bin")

        values = analyse_memory(
            size_binary_path(self._toolchain_prefix), elf_path
        )

        free = 80 * 1024 - values[".data"] - values[".rodata"] - values[".bss"]
        free = free + (16 - free % 16)
        values["free"] = free

        values["size"] = file_size(bin_path)

        return values


def main(args):

    # Check xtensa-lx106-elf-size is in the path
    size_binary_exists(args)

    # Which modules to test?
    configuration, modules = get_modules(args)

    # print_values init message
    print('Selected environment "{}"'.format(args.environment), end="")
    if modules:
        print(" with modules: {}".format(" ".join(modules.keys())))
    else:
        print()

    print()
    print("Analyzing {} configuration".format(configuration))
    print()

    # Build the core without any modules to get base memory usage
    analyser = Analyser(args, modules)
    analyser.begin(configuration)

    # Test each module separately
    results = {}
    for module in analyser.modules:
        with analyser.enable(module):
            results[module] = analyser.run()
        analyser.print_compare(module, results[module])

    # Test all modules
    if analyser.modules:

        with analyser.enable():
            total = analyser.run()

        analyser.print_delimiters()
        if len(analyser.modules) > 1:
            analyser.print_compare("ALL MODULES", total)

        analyser.print_values("TOTAL", total)


if __name__ == "__main__":
    main(parse_commandline_args())
