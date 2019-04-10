#!/usr/bin/env python
from __future__ import print_function

import os
import re
import sys
from subprocess import call
from fileinput import FileInput

import click

Import("env", "projenv")
from SCons.Subst import quote_spaces
from SCons.Script import Mkdir

# ------------------------------------------------------------------------------
# Utils
# ------------------------------------------------------------------------------

class Color(object):
    BLACK = '\x1b[1;30m'
    RED = '\x1b[1;31m'
    GREEN = '\x1b[1;32m'
    YELLOW = '\x1b[1;33m'
    BLUE = '\x1b[1;34m'
    MAGENTA = '\x1b[1;35m'
    CYAN = '\x1b[1;36m'
    WHITE = '\x1b[1;37m'
    LIGHT_GREY = '\x1b[0;30m'
    LIGHT_RED = '\x1b[0;31m'
    LIGHT_GREEN = '\x1b[0;32m'
    LIGHT_YELLOW = '\x1b[0;33m'
    LIGHT_BLUE = '\x1b[0;34m'
    LIGHT_MAGENTA = '\x1b[0;35m'
    LIGHT_CYAN = '\x1b[0;36m'
    LIGHT_WHITE = '\x1b[0;37m'

def clr(color, text):
    return color + str(text) + '\x1b[0m'

def print_warning(message, color=Color.LIGHT_YELLOW):
    print(clr(color, message), file=sys.stderr)

def print_filler(fill, color=Color.WHITE, err=False):
    width, _ = click.get_terminal_size()
    if len(fill) > 1:
        fill = fill[0]

    out = sys.stderr if err else sys.stdout
    print(clr(color, fill * width), file=out)

# https://github.com/python/cpython/commit/6cb7b659#diff-78790b53ff259619377058acd4f74672
if sys.version_info[0] < 3:
    class FileInputCtx(FileInput):
        def __enter__(self):
            return self

        def __exit__(self, type, value, traceback):
            self.close()


    FileInput = FileInputCtx

# ------------------------------------------------------------------------------
# Callbacks
# ------------------------------------------------------------------------------

def remove_float_support():

    flags = " ".join(env['LINKFLAGS'])
    flags = flags.replace("-u _printf_float", "")
    flags = flags.replace("-u _scanf_float", "")
    newflags = flags.split()

    env.Replace(
        LINKFLAGS = newflags
    )

def cpp_check(target, source, env):
    print("Started cppcheck...\n")
    call(["cppcheck", os.getcwd()+"/espurna", "--force", "--enable=all"])
    print("Finished cppcheck...\n")

def check_size(target, source, env):
    (binary,) = target
    path = binary.get_abspath()
    size = os.stat(path).st_size
    print(clr(Color.LIGHT_BLUE, "Binary size: {} bytes".format(size)))

    # Warn 1MB variants about exceeding OTA size limit
    flash_size = int(env.BoardConfig().get("upload.maximum_size", 0))
    if (flash_size == 1048576) and (size >= 512000):
        print_filler("*", color=Color.LIGHT_YELLOW, err=True)
        print_warning("File is too large for OTA! Here you can find instructions on how to flash it:")
        print_warning("https://github.com/xoseperez/espurna/wiki/TwoStepUpdates", color=Color.LIGHT_CYAN)
        print_filler("*", color=Color.LIGHT_YELLOW, err=True)

def dummy_ets_printf(target, source, env):
    (postmortem_src_file, ) = source
    (postmortem_obj_file, ) = target

    cmd = ["xtensa-lx106-elf-objcopy"]

    # recent Core switched to cpp+newlib & ets_printf_P
    cmd.extend(["--redefine-sym", "ets_printf=dummy_ets_printf"])
    cmd.extend(["--redefine-sym", "ets_printf_P=dummy_ets_printf"])

    cmd.append(postmortem_obj_file.get_abspath())
    env.Execute(env.VerboseAction(" ".join(cmd), "Removing ets_printf / ets_printf_P"))
    env.Depends(postmortem_obj_file,"$BUILD_DIR/src/dummy_ets_printf.c.o")

# Intermediate solution to the hardware migration.
# Generate hardware headers with dynamic settings
def get_legacy_hardware(env, hardware_h=os.path.join("$PROJECT_DIR", "espurna", "config", "hardware.h"), ignore=("TRAVIS",)):

    def extract(line):
        plain_re = re.compile("defined\s+(?P<plain>\w+)")
        paren_re = re.compile("defined\s*\((?P<paren>\w+)\)")

        res = paren_re.search(line)
        if res:
            return res.group(1)
        res = plain_re.search(line)
        if res:
            return res.group(1)

        return None

    with open(env.subst(hardware_h), "r") as header:
        for line in header:
            if not any([line.startswith("#if"), line.startswith("#elif")]):
                continue

            out = extract(line)
            if out in ignore:
                continue

            yield out

def generate_hardware_header(env, out_dir, hardware):
    template = env.subst(os.path.join("$PROJECT_DIR", "espurna", "libs", "migrate_template.h"))
    cmd_template = "$CC -x c++ -dI -E $CCFLAGS $_CCCOMCOM -D{hardware} $_ESPURNA_ALL_H {source} -o {target}"

    out = env.subst(os.path.join(out_dir, "{}.h".format(hardware)))
    cmd = cmd_template.format(hardware=hardware, source=template, target=out)

    # first, we a generating raw header file
    env.Execute(env.Action(cmd, "Generating {}".format(out)))

    # then, we modify it to skip raw included files and keep only migrate_template contents
    with FileInput(out, inplace=True) as header:
        write = False
        first = True
        for line in header:
            if first:
                first = False
                continue

            if not line.strip():
                continue
            if not write and "migrate_template.h" in line:
                write = True
            if line.startswith("#"):
                continue

            if write:
                sys.stdout.write(line.strip())
                sys.stdout.write("\n")

env.AddMethod(get_legacy_hardware)
env.AddMethod(generate_hardware_header)

def generate_hardware(target, source, env):
    all_hardware = list(env.get_legacy_hardware())

    # ignore current hardware setting from platformio.ini
    defines = list(env["CPPDEFINES"])
    for hardware in all_hardware:
        entry = (hardware, )
        if entry in defines:
            defines.remove(entry)
            env.Replace(CPPDEFINES=defines)
            break

    # add include paths from libraries
    for builder in env.GetLibBuilders():
        env.PrependUnique(CPPPATH=[quote_spaces(x) for x in builder.env.get("CPPPATH")])

    # our local header chain (NOTE: will include custom.h if build flags allow it)
    env.Append(_ESPURNA_ALL_H=["-include", os.path.join("$PROJECT_DIR", "espurna", "config", "all.h")])
    env.Append(CPPPATH=[os.path.join("$PROJECT_DIR", "espurna")])

    out_dir = os.path.join("$PROJECT_DIR", "espurna", "hardware")
    for hardware in all_hardware:
        env.generate_hardware_header(out_dir, hardware)

# ------------------------------------------------------------------------------
# Hooks
# ------------------------------------------------------------------------------

# Always show warnings for project code
projenv.ProcessUnFlags("-w")

# 2.4.0 and up
#remove_float_support()

# two-step update hint when using 1MB boards
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_size)

# disable postmortem printing to the uart. another one is in eboot, but this is what causes the most harm
if "DISABLE_POSTMORTEM_STACKDUMP" in env["CPPFLAGS"]:
    env.AddPostAction("$BUILD_DIR/FrameworkArduino/core_esp8266_postmortem.c.o", dummy_ets_printf)
    env.AddPostAction("$BUILD_DIR/FrameworkArduino/core_esp8266_postmortem.cpp.o", dummy_ets_printf)

env.AlwaysBuild(env.Alias("generate-hardware", None, generate_hardware))
