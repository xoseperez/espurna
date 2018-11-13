#!/usr/bin/env python
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
import shlex
import sys
from collections import OrderedDict
from sortedcontainers import SortedDict
import subprocess

if (sys.version_info > (3, 0)):
    from subprocess import getstatusoutput as getstatusoutput
else:
    from commands import getstatusoutput as getstatusoutput

# -------------------------------------------------------------------------------

TOTAL_IRAM = 32786
TOTAL_DRAM = 81920
env = "esp8266-4m-ota"
objdump_binary = "xtensa-lx106-elf-objdump"
sections = OrderedDict([
    ("data", "Initialized Data (RAM)"),
    ("rodata", "ReadOnly Data (RAM)"),
    ("bss", "Uninitialized Data (RAM)"),
    ("text", "Cached Code (IRAM)"),
    ("irom0_text", "Uncached Code (SPI)")
])
description = "ESPurna Memory Analyzer v0.1"


# -------------------------------------------------------------------------------

def file_size(file):
    try:
        return os.stat(file).st_size
    except OSError:
        return 0


def analyse_memory(elf_file):
    command = "%s -t '%s'" % (objdump_binary, elf_file)
    response = subprocess.check_output(shlex.split(command))
    if isinstance(response, bytes):
        response = response.decode('utf-8')
    lines = response.split('\n')

    # print("{0: >10}|{1: >30}|{2: >12}|{3: >12}|{4: >8}".format("Section", "Description", "Start (hex)", "End (hex)", "Used space"));
    # print("------------------------------------------------------------------------------");
    ret = {}

    for (id_, _) in list(sections.items()):
        section_start_token = " _%s_start" % id_
        section_end_token = " _%s_end" % id_
        section_start = -1
        section_end = -1
        for line in lines:
            if section_start_token in line:
                data = line.split(' ')
                section_start = int(data[0], 16)

            if section_end_token in line:
                data = line.split(' ')
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

    # print("Total Used RAM : %d" % usedRAM)
    # print("Free RAM : %d" % (TOTAL_DRAM - usedRAM))
    # print("Free IRam : %d" % usedIRAM)
    return ret


def run(env_, modules_):
    flags = ""
    for item in modules_.items():
        flags += "-D%s_SUPPORT=%d " % item
    command = "ESPURNA_BOARD=\"WEMOS_D1_MINI_RELAYSHIELD\" ESPURNA_FLAGS=\"%s\" platformio run --silent --environment %s 2>/dev/null" % (flags, env_)
    subprocess.check_call(command, shell=True)

def calc_free(module):
    free = 80 * 1024 - module['data'] - module['rodata'] - module['bss']
    free = free + (16 - free % 16)
    module['free'] = free

def modules_get():
    modules_ = SortedDict()
    for line in open("espurna/config/arduino.h"):
        m = re.search(r'(\w*)_SUPPORT', line)
        if m:
            modules_[m.group(1)] = 0
    del modules_['LLMNR']
    del modules_['NETBIOS']
    return modules_

try:

    # Parse command line options
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("modules", nargs='*', help="Modules to test (use ALL to test them all)")
    parser.add_argument("-c", "--core", help="use core as base configuration instead of default", default=0, action='count')
    parser.add_argument("-l", "--list", help="list available modules", default=0, action='count')
    args = parser.parse_args()

    # Hello
    print()
    print(description)
    print()

    # Check xtensa-lx106-elf-objdump is in the path
    status, result = getstatusoutput(objdump_binary)
    if status != 2 and status != 512:
        print("xtensa-lx106-elf-objdump not found, please check it is in your PATH")
        sys.exit(1)

    # Load list of all modules
    available_modules = modules_get()
    if args.list > 0:
        print("List of available modules:\n")
        for key, value in available_modules.items():
            print("* " + key)
        print()
        sys.exit(0)

    # Which modules to test?
    test_modules = []
    if len(args.modules) > 0:
        if "ALL" in args.modules:
            test_modules = available_modules.keys()
        else:
            test_modules = args.modules

    # Check test modules exist
    for module in test_modules:
        if module not in available_modules:
            print("Module %s not found" % module)
            sys.exit(2)

    # Define base configuration
    if args.core == 0:
        modules = SortedDict()
        for m in test_modules:
            modules[m] = 0
    else:
        modules = available_modules

    # Show init message
    if len(test_modules) > 0:
        print("Analyzing module(s) %s on top of %s configuration\n" % (", ".join(test_modules), "CORE" if args.core > 0 else "DEFAULT"))
    else:
        print("Analyzing %s configuration\n" % ("CORE" if args.core > 0 else "DEFAULT"))

    output_format = "{:<20}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}|{:<15}"
    print(output_format.format(
            "Module",
            "Cache IRAM",
            "Init RAM",
            "R.O. RAM",
            "Uninit RAM",
            "Available RAM",
            "Flash ROM",
            "Binary size"
    ))
    print(output_format.replace("<", ">").format(
            "",
            ".text",
            ".data",
            ".rodata",
            ".bss",
            "heap + stack",
            ".irom0.text",
            ""
    ))
    print(output_format.format(
            "-" * 20,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15,
            "-" * 15
    ))

    # Build the core without modules to get base memory usage
    run(env, modules)
    base = analyse_memory(".pioenvs/%s/firmware.elf" % env)
    base['size'] = file_size(".pioenvs/%s/firmware.bin" % env)
    calc_free(base)
    print(output_format.format(
            "CORE" if args.core == 1 else "DEFAULT",
            base['text'],
            base['data'],
            base['rodata'],
            base['bss'],
            base['free'],
            base['irom0_text'],
            base['size'],
    ))

    # Test each module
    results = {}
    for module in test_modules:

        modules[module] = 1
        run(env, modules)
        results[module] = analyse_memory(".pioenvs/%s/firmware.elf" % env)
        results[module]['size'] = file_size(".pioenvs/%s/firmware.bin" % env)
        calc_free(results[module])
        modules[module] = 0

        print(output_format.format(
                module,
                results[module]['text'] - base['text'],
                results[module]['data'] - base['data'],
                results[module]['rodata'] - base['rodata'],
                results[module]['bss'] - base['bss'],
                results[module]['free'] - base['free'],
                results[module]['irom0_text'] - base['irom0_text'],
                results[module]['size'] - base['size'],
        ))

    # Test all modules
    if len(test_modules) > 0:

        for module in test_modules:
            modules[module] = 1
        run(env, modules)
        total = analyse_memory(".pioenvs/%s/firmware.elf" % env)
        total['size'] = file_size(".pioenvs/%s/firmware.bin" % env)
        calc_free(total)

        if len(test_modules) > 1:
            print(output_format.format(
                    "ALL MODULES",
                    total['text'] - base['text'],
                    total['data'] - base['data'],
                    total['rodata'] - base['rodata'],
                    total['bss'] - base['bss'],
                    total['free'] - base['free'],
                    total['irom0_text'] - base['irom0_text'],
                    total['size'] - base['size'],
            ))

        print(output_format.format(
                "TOTAL",
                total['text'],
                total['data'],
                total['rodata'],
                total['bss'],
                total['irom0_text'],
                total['size'],
        ))

except:
    raise

print("\n")
