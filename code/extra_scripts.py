#!/usr/bin/env python
from __future__ import print_function

import os
import re
import sys
from subprocess import call, Popen, PIPE

import click
from platformio import util
from SCons.Scanner.Dir import DirScanner
from SCons.Script import COMMAND_LINE_TARGETS

Import("env")

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

def VerboseWhereIs(env, prog, message, path=None):
    result = env.WhereIs(prog=prog, path=path)
    if not result:
        if path:
            prog = os.path.join(path, prog)
        print_warning("'{}' {}".format(prog, message))
    return result
env.AddMethod(VerboseWhereIs)

re_define = re.compile(r"^[\s]*?#define[\s]+?(?P<key>\w+)[\s]+?(?P<value>.*$)")

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

def cpp_check(source, target, env):
    print("Started cppcheck...\n")
    call(["cppcheck", os.getcwd()+"/espurna", "--force", "--enable=all"])
    print("Finished cppcheck...\n")

def check_size(source, target, env):
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

def parse_config_headers(env, headers=("$PROJECT_DIR/espurna/config/all.h",)):
    # config[env:xxx][build_flags] + $PLATFORMIO_BUILD_FLAGS
    # TODO does scons parse this somewhere as list?
    cmd = [env.WhereIs(env.get("CC", "gcc"))]

    (build_flags, ) = env.get("BUILD_FLAGS", [""])
    cmd.extend(build_flags.split())

    # use c++ language, do not expand definitions, stop after preprocessor
    # can debug using -C flag - it will insert comments with origin of #define
    # XXX does break re-difinitions. better turn on warnings for all build!
    cmd.extend(["-x", "c++", "-dD", "-E"])
    cmd.extend(env.subst(header) for header in headers)

    proc = Popen(cmd, stdout=PIPE, stderr=PIPE)

    defines = {}
    for line in proc.stdout:
        line = line.decode('utf-8')
        if not line or not u"#define" in line:
            continue
        line = line.strip()

        match = re_define.search(line)
        if not match:
            continue

        key, value = match.groups()
        if key.startswith(u"_"):
            continue

        value = value.strip()

        # often, define is dependent on some other one
        # they are always in order, so it is expected to be in dict
        if value in defines.keys():
            value = defines.get(value)
            defines[key] = value
            continue

        if value in (u"true", u"false"):
            value = True if (value == u"true") else False
        elif value.startswith(u'"') and value.endswith(u'"'):
            value = value.replace(u'"', u'')
        elif value.startswith(u"0x"):
            value = int(value, 16)
        elif value.isdecimal():
            value = int(value, 10)

        if (key.endswith(u"_SUPPORT") or key.endswith(u"_ENABLED")) \
                and not isinstance(value, bool):
            value = bool(value)

        defines[key] = value

    return defines

def build_webui(**kwargs):
    cmd = "node node_modules/gulp/bin/gulp.js webui_${WEBUI_MODULE}"
    if env.Execute(cmd):
        print_warning("Did not build {}".format(env.subst("$WEBUI_MODULE")))
        env.Exit(1)

def inject_gulp(env):
    if not env["WEBUI_SUPPORT"]:
        return

    have_node = env.VerboseWhereIs(
        "node",
        "is not installed"
    )

    have_gulp = env.VerboseWhereIs(
        "gulp.js",
        "is not installed (run 'npm install --only=dev')",
        path="$PROJECT_DIR/node_modules/gulp/bin/"
    )

    # DirScanner() is essentialy this:
    # def get_all_files(path):
    #    for name, dirs, files in os.walk(env.subst(path)):
    #        for file in files:
    #            yield os.path.join(name, file)

    if have_node and have_gulp:
        env.SetDefault(
            WEBUI_SOURCE="$PROJECT_DIR/html/",
            WEBUI_ARCHIVE="$PROJECT_DIR/espurna/data/index.${WEBUI_MODULE}.html.gz",
            WEBUI_HEADER="$PROJECT_DIR/espurna/static/index.${WEBUI_MODULE}.html.gz.h"
        )

        env.Depends("$BUILD_DIR/src/espurna.ino.cpp.o", "$WEBUI_HEADER")
        env.Depends(env.Alias("buildprog"), "$WEBUI_HEADER")
        cmd_webui = env.Command("$WEBUI_HEADER", "$WEBUI_SOURCE",
                build_webui, source_scanner=DirScanner())
        env.SideEffect("$WEBUI_ARCHIVE", cmd_webui)
    else:
        print_warning("Not building the web UI header")

def inject_webui_module(env):
    defines = parse_config_headers(env)

    env["WEBUI_SUPPORT"] = defines[u"WEB_SUPPORT"]
    if not env["WEBUI_SUPPORT"]:
        return

    modules_to_defines = {
        "rfbridge": defines[u"RELAY_PROVIDER_RFBRIDGE"] == defines[u"RELAY_PROVIDER"],
        "light": defines.has_key(u"LIGHT_PROVIDER") and defines.get(u"LIGHT_PROVIDER") != defines[u"LIGHT_PROVIDER_NONE"],
        "sensor": defines.has_key(u"SENSOR_SUPPORT") and defines.get(u"SENSOR_SUPPORT")
    }

    enabled_modules = [module for module, enabled in modules_to_defines.items() if enabled]
    if len(enabled_modules) > 1:
        env["WEBUI_MODULE"] = "all"
    elif len(enabled_modules) == 0:
        env["WEBUI_MODULE"] = "small"
    else:
        (env["WEBUI_MODULE"], ) = enabled_modules

    print_warning("WebUI module defined by the headers: {}".format(env["WEBUI_MODULE"]))

# ------------------------------------------------------------------------------
# Hooks
# ------------------------------------------------------------------------------

remove_float_support()

if not "nobuild" in COMMAND_LINE_TARGETS:
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_size)
    inject_webui_module(env)
    inject_gulp(env)

#from SCons.Script import COMMAND_LINE_TARGETS, DEFAULT_TARGETS, BUILD_TARGETS
#for target in "COMMAND_LINE_TARGETS", "DEFAULT_TARGETS", "BUILD_TARGETS":
#    print(target, [x.name for x in locals()[target]])
