#!/usr/bin/env python3
# pylint: disable=C0301,C0114,C0116,W0511
# coding=utf-8
# -------------------------------------------------------------------------------
# ESPurna module memory analyser
# xose.perez@gmail.com
#
# Rewritten for python-3 by Maxim Prokhorov
# prokhorov.max@outlook.com
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
from __future__ import print_function

import argparse
import os
import re
import subprocess
import sys
from collections import OrderedDict

__version__ = (0, 2)
from subprocess import getstatusoutput

# -------------------------------------------------------------------------------

TOTAL_IRAM = 32786
TOTAL_DRAM = 81920

DEFAULT_ENV = "nodemcu-lolin"
OBJDUMP_PREFIX = "~/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-"
SECTIONS = OrderedDict(
    [
        ("data", "Initialized Data (RAM)"),
        ("rodata", "ReadOnly Data (RAM)"),
        ("bss", "Uninitialized Data (RAM)"),
        ("text", "Cached Code (IRAM)"),
        ("irom0_text", "Uncached Code (SPI)"),
    ]
)
DESCRIPTION = "ESPurna Memory Analyzer v{}".format(
    ".".join(str(x) for x in __version__)
)


# -------------------------------------------------------------------------------


def objdump_path(prefix):
    return "{}objdump".format(os.path.expanduser(prefix))


def file_size(file):
    try:
        return os.stat(file).st_size
    except OSError:
        return 0


def analyse_memory(elf_file, objdump):
    command = [objdump, "-t", elf_file]
    response = subprocess.check_output(command)
    proc = subprocess.Popen(command, stdout=subprocess.PIPE, universal_newlines=True)
    lines = proc.stdout.readlines()

    # print("{0: >10}|{1: >30}|{2: >12}|{3: >12}|{4: >8}".format("Section", "Description", "Start (hex)", "End (hex)", "Used space"));
    # print("------------------------------------------------------------------------------");
    ret = {}

    for (id_, _) in list(SECTIONS.items()):
        section_start_token = " _{}_start".format(id_)
        section_end_token = " _{}_end".format(id_)
        section_start = -1
        section_end = -1
        for line in lines:
            line = line.strip()
            if section_start_token in line:
                data = line.split(" ")
                section_start = int(data[0], 16)

            if section_end_token in line:
                data = line.split(" ")
                section_end = int(data[0], 16)

            if section_start != -1 and section_end != -1:
                break

        section_length = section_end - section_start
        # if i < 3:
        #     usedRAM += section_length
        # if i == 3:
        #     usedIRAM = TOTAL_IRAM - section_length;

        ret[id_] = section_length
        # print("{0: >10}|{1: >30}|{2:12X}|{3:12X}|{4:8}".format(id_, descr, section_start, section_end, section_length))
        # i += 1

    # print("Total Used RAM : {:d}".format(usedRAM))
    # print("Free RAM : {:d}".format(TOTAL_DRAM - usedRAM))
    # print("Free IRam : {:d}".format(usedIRAM))
    return ret


def run(env_, modules_, debug=False):
    flags = " ".join("-D{}_SUPPORT={:d}".format(k, v) for k, v in modules_.items())

    os_env = os.environ.copy()
    os_env["PLATFORMIO_SRC_BUILD_FLAGS"] = flags
    os_env["PLATFORMIO_BUILD_CACHE_DIR"] = "test/pio_cache"
    os_env["ESPURNA_PIO_SHARED_LIBRARIES"] = "y"

    command = ["platformio", "run"]
    if not debug:
        command.append("--silent")
    command.extend(["--environment", env_])

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
        "-p",
        "--prefix",
        help="where to find the xtensa toolchain binaries",
        default=OBJDUMP_PREFIX,
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


def objdump_check(args):

    status, _ = getstatusoutput(objdump_path(args.prefix))
    if status not in (2, 512):
        print(
            "xtensa-lx106-elf-objdump not found, please check that the --prefix is correct"
        )
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


class Analyser:
    """Run platformio and print info about the resulting binary."""

    OUTPUT_FORMAT = "{:<20}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}"
    ELF_FORMAT = ".pio/build/{env}/firmware.elf"
    BIN_FORMAT = ".pio/build/{env}/firmware.bin"

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
        self._prefix = args.prefix
        self._environment = args.environment
        self._elf_path = self.ELF_FORMAT.format(env=args.environment)
        self._bin_path = self.BIN_FORMAT.format(env=args.environment)
        self.modules = modules
        self.baseline = None

    def enable(self, module=None):
        return self._Enable(self, module)

    def print(self, *args):
        print(self.OUTPUT_FORMAT.format(*args))

    def print_delimiters(self):
        self.print(
            "-" * 20,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
        )

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
            "", ".text", ".data", ".rodata", ".bss", "heap + stack", ".irom0.text", ""
        )
        self.print_delimiters()
        self.baseline = self.run()
        self.print_values(name, self.baseline)

    def print_values(self, header, values):
        self.print(
            header,
            values["text"],
            values["data"],
            values["rodata"],
            values["bss"],
            values["free"],
            values["irom0_text"],
            values["size"],
        )

    # TODO: sensor modules need to be print_compared with SENSOR as baseline
    # TODO: some modules need to be print_compared with WEB as baseline
    def print_compare(self, header, values):
        self.print(
            header,
            values["text"] - self.baseline["text"],
            values["data"] - self.baseline["data"],
            values["rodata"] - self.baseline["rodata"],
            values["bss"] - self.baseline["bss"],
            values["free"] - self.baseline["free"],
            values["irom0_text"] - self.baseline["irom0_text"],
            values["size"] - self.baseline["size"],
        )

    def run(self):
        run(self._environment, self.modules, self._debug)

        values = analyse_memory(self._elf_path, objdump_path(self._prefix))

        free = 80 * 1024 - values["data"] - values["rodata"] - values["bss"]
        free = free + (16 - free % 16)
        values["free"] = free

        values["size"] = file_size(self._bin_path)

        return values

    modules = None
    baseline = None

    _debug = False
    _modules = []
    _environment = None
    _prefix = None
    _bin_path = None
    _elf_path = None


def main(args):

    # Check xtensa-lx106-elf-objdump is in the path
    objdump_check(args)

    # Which modules to test?
    configuration, modules = get_modules(args)

    # print_values init message
    print("Selected environment \"{}\"".format(args.environment), end='')
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
