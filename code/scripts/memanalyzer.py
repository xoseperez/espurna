#!/usr/bin/env python3
# coding=utf-8
# -------------------------------------------------------------------------------
# ESPurna module memory analyser
# xose.perez@gmail.com
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
    if isinstance(response, bytes):
        response = response.decode("utf-8")
    lines = response.split("\n")

    # print("{0: >10}|{1: >30}|{2: >12}|{3: >12}|{4: >8}".format("Section", "Description", "Start (hex)", "End (hex)", "Used space"));
    # print("------------------------------------------------------------------------------");
    ret = {}

    for (id_, _) in list(SECTIONS.items()):
        section_start_token = " _{}_start".format(id_)
        section_end_token = " _{}_end".format(id_)
        section_start = -1
        section_end = -1
        for line in lines:
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


def calc_free(module):
    free = 80 * 1024 - module["data"] - module["rodata"] - module["bss"]
    free = free + (16 - free % 16)
    module["free"] = free


def modules_get():
    modules_ = OrderedDict()
    for line in open("espurna/config/arduino.h"):
        m = re.search(r"(\w*)_SUPPORT", line)
        if m:
            modules_[m.group(1)] = 0
    modules_ = OrderedDict(sorted(modules_.items(), key=lambda t: t[0]))

    return modules_


if __name__ == "__main__":

    # Parse command line options
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument(
        "-e", "--environment", help="platformio envrionment to use", default=DEFAULT_ENV
    )
    parser.add_argument(
        "-p",
        "--prefix",
        help="where to find xtensa toolchain, default is {}".format(OBJDUMP_PREFIX),
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
    args = parser.parse_args()

    # Check xtensa-lx106-elf-objdump is in the path
    status, result = getstatusoutput(objdump_path(args.prefix))
    if status != 2 and status != 512:
        print(
            "xtensa-lx106-elf-objdump not found, please check that the --prefix is correct"
        )
        sys.exit(1)

    # Load list of all modules
    available_modules = modules_get()
    if args.list:
        print("List of available modules:\n")
        for key, value in available_modules.items():
            print("* " + key)
        print()
        sys.exit(0)

    # Which modules to test?
    test_modules = []
    if len(args.modules):
        if "ALL" in args.modules:
            test_modules.extend(available_modules.keys())
        else:
            test_modules.extend(args.modules)
    test_modules.sort()

    # Check test modules exist
    for module in test_modules:
        if module not in available_modules:
            print("Module {} not found".format(module))
            sys.exit(2)

    configuration = "CORE" if args.core else "DEFAULT"
    # Define base configuration
    if not args.core:
        modules = OrderedDict()
        for m in test_modules:
            modules[m] = 0
    else:
        modules = available_modules

    # Show init message
    if len(test_modules) > 0:
        print(
            "Analyzing module(s) {} on top of {} configuration\n".format(
                " ".join(test_modules), configuration
            )
        )
    else:
        print("Analyzing {} configuration\n".format(configuration))

    output_format = "{:<20}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}"

    def print_format(*args):
        print(output_format.format(*args))

    def print_delimiters():
        print_format(
            "-" * 20,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
        )

    print_format(
        "Module",
        "Cache IRAM",
        "Init RAM",
        "R.O. RAM",
        "Uninit RAM",
        "Available RAM",
        "Flash ROM",
        "Binary size",
    )
    print_format(
        "", ".text", ".data", ".rodata", ".bss", "heap + stack", ".irom0.text", ""
    )
    print_delimiters()

    environment = args.environment
    bin_path = ".pio/build/{}/firmware.bin".format(environment)
    elf_path = ".pio/build/{}/firmware.elf".format(environment)

    def _analyse_memory():
        return analyse_memory(elf_path, objdump_path(args.prefix))

    def _file_size():
        return file_size(bin_path)

    def _run():
        run(environment, modules, args.debug)

    def stats():
        _run()
        values = _analyse_memory()
        values["size"] = _file_size()
        return values

    def show(header, values):
        print(
            output_format.format(
                header,
                values["text"],
                values["data"],
                values["rodata"],
                values["bss"],
                values["free"],
                values["irom0_text"],
                values["size"],
            )
        )

    # TODO: sensor modules need to be compared with SENSOR as base
    # TODO: some modules need to be compared with WEB as base
    def compare(header, values):
        print(
            output_format.format(
                header,
                values["text"] - base["text"],
                values["data"] - base["data"],
                values["rodata"] - base["rodata"],
                values["bss"] - base["bss"],
                values["free"] - base["free"],
                values["irom0_text"] - base["irom0_text"],
                values["size"] - base["size"],
            )
        )

    # Build the core without modules to get base memory usage
    base = stats()

    calc_free(base)
    show(configuration, base)

    # Test each module
    results = {}
    for module in test_modules:

        modules[module] = 1
        results[module] = stats()

        calc_free(results[module])
        modules[module] = 0

        compare(module, results[module])

    # Test all modules
    if len(test_modules):

        for module in test_modules:
            modules[module] = 1

        total = stats()

        calc_free(total)

        print_delimiters()
        if len(test_modules) > 1:
            compare("ALL MODULES", total)

        show("TOTAL", total)
