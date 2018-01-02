#!/usr/bin/env python
#-------------------------------------------------------------------------------
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
#-------------------------------------------------------------------------------

from collections import OrderedDict
from sortedcontainers import SortedDict
import shlex
import commands
import subprocess
import sys
import re
import argparse

#-------------------------------------------------------------------------------

TOTAL_IRAM = 32786;
TOTAL_DRAM = 81920;
env="esp8266-4m-ota"
objdump_binary = "xtensa-lx106-elf-objdump"
sections = OrderedDict([
    ("data", "Initialized Data (RAM)"),
    ("rodata", "ReadOnly Data (RAM)"),
    ("bss", "Uninitialized Data (RAM)"),
    ("text", "Cached Code (IRAM)"),
    ("irom0_text", "Uncached Code (SPI)")
])
description = "ESPurna Memory Analyzer v0.1"

#-------------------------------------------------------------------------------

def analyse_memory(elf_file):

    command = "%s -t '%s' " % (objdump_binary, elf_file)
    response = subprocess.check_output(shlex.split(command))
    if isinstance(response, bytes):
        response = response.decode('utf-8')
    lines = response.split('\n')

    # print("{0: >10}|{1: >30}|{2: >12}|{3: >12}|{4: >8}".format("Section", "Description", "Start (hex)", "End (hex)", "Used space"));
    # print("------------------------------------------------------------------------------");
    ret={}
    usedRAM = 0
    usedIRAM = 0

    i = 0
    for (id, descr) in list(sections.items()):
        sectionStartToken = " _%s_start" %  id
        sectionEndToken   = " _%s_end" % id
        sectionStart = -1
        sectionEnd = -1
        for line in lines:
            if sectionStartToken in line:
                data = line.split(' ')
                sectionStart = int(data[0], 16)

            if sectionEndToken in line:
                data = line.split(' ')
                sectionEnd = int(data[0], 16)

            if sectionStart != -1 and sectionEnd != -1:
                break

        sectionLength = sectionEnd - sectionStart
        # if i < 3:
        #     usedRAM += sectionLength
        # if i == 3:
        #     usedIRAM = TOTAL_IRAM - sectionLength;

        ret[id]=sectionLength
        # print("{0: >10}|{1: >30}|{2:12X}|{3:12X}|{4:8}".format(id, descr, sectionStart, sectionEnd, sectionLength))
        # i += 1

    # print("Total Used RAM : %d" % usedRAM)
    # print("Free RAM : %d" % (TOTAL_DRAM - usedRAM))
    # print("Free IRam : %d" % usedIRAM)
    return(ret)

def run(env, modules):
    flags = ""
    for item in modules.items():
        flags += "-D%s_SUPPORT=%d " % item
    command = "export ESPURNA_BOARD=\"WEMOS_D1_MINI_RELAYSHIELD\"; export ESPURNA_FLAGS=\"%s\"; platformio run --silent --environment %s" % (flags, env)
    subprocess.check_call(command, shell=True)

def modules_get():
    modules = SortedDict()
    for line in open("espurna/config/arduino.h"):
        m = re.search(r'(\w*)_SUPPORT', line)
        if m:
            modules[m.group(1)] = 0
    del modules['LLMNR']
    del modules['NETBIOS']
    return modules

try:

    # Parse command line options
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("modules", nargs='*', help="Modules to test")
    parser.add_argument("-c", "--core", help="use core as base configuration instead of default", default=0, action='count')
    parser.add_argument("-l", "--list", help="list available modules", default=0, action='count')
    args = parser.parse_args()

    # Hello
    print
    print description
    print

    # Check xtensa-lx106-elf-objdump is in the path
    status, result = commands.getstatusoutput(objdump_binary)
    if status != 512:
        print "xtensa-lx106-elf-objdump not found, please check it is in your PATH"
        sys.exit(1)

    # Load list of all modules
    available_modules = modules_get()
    if args.list > 0:
        print "List of available modules:\n"
        for key, value in available_modules.items():
            print "* " + key
        print
        sys.exit(0)

    # Which modules to test?
    if len(args.modules) > 0:
        test_modules = args.modules
    else:
        test_modules = available_modules.keys()

    # Check test modules exist
    for module in test_modules:
        if module not in available_modules:
            print "Module %s not found" % module
            sys.exit(2)

    # Define base configuration
    if args.core == 0:
        modules = SortedDict()
        for m in test_modules:
            modules[m] = 0
    else:
        modules = available_modules

    # Show init message
    print "Analyzing module(s) %s on top of %s configuration\n" % (", ".join(test_modules), "CORE" if args.core > 0 else "DEFAULT")
    output_format="{:<20}|{:<11}|{:<11}|{:<11}|{:<11}|{:<11}"
    print(output_format.format(
        "Module",
        "Cache IRAM",
        "Init RAM",
        "R.O. RAM",
        "Uninit RAM",
        "Flash ROM"
    ))

    # Build the core without modules to get base memory usage
    run(env, modules)
    base = analyse_memory(".pioenvs/"+env+"/firmware.elf")
    print(output_format.format(
        "CORE" if args.core == 1 else "DEFAULT",
        base['text'],
        base['data'],
        base['rodata'],
        base['bss'],
        base['irom0_text'],
    ))

    # Test each module
    results = {}
    for module in test_modules:

        modules[module] = 1
        run(env, modules)
        results[module]=analyse_memory(".pioenvs/"+env+"/firmware.elf")
        modules[module] = 0

        print(output_format.format(
            module,
            results[module]['text'] - base['text'],
            results[module]['data'] - base['data'],
            results[module]['rodata'] - base['rodata'],
            results[module]['bss'] - base['bss'],
            results[module]['irom0_text'] - base['irom0_text'],
        ))

    # Test all modules
    for module in test_modules:
        modules[module] = 1
    run(env, modules)
    total = analyse_memory(".pioenvs/"+env+"/firmware.elf")

    if len(test_modules) > 1:
        print(output_format.format(
            "ALL MODULES",
            total['text'] - base['text'],
            total['data'] - base['data'],
            total['rodata'] - base['rodata'],
            total['bss'] - base['bss'],
            total['irom0_text'] - base['irom0_text'],
        ))

    print(output_format.format(
        "TOTAL",
        total['text'],
        total['data'],
        total['rodata'],
        total['bss'],
        total['irom0_text'],
    ))

except:
    raise

subprocess.check_call("export ESPURNA_BOARD=\"\"; export ESPURNA_FLAGS=\"\"", shell=True)

print("\n")
