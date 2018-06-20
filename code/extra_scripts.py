#!/usr/bin/env python
from subprocess import call
from platformio import util
import os
import time

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
    time.sleep(2)
    size = target[0].get_size()
    print clr(Color.LIGHT_BLUE, "Binary size: %s bytes" % size)
    #if size > 512000:
    #    print clr(Color.LIGHT_RED, "File too large for OTA!")
    #    Exit(1)

def build_webui(source, target, env):
    config = util.load_project_config()
    kv = dict(config.items("env:" + env.get('PIOENV')))
    if 'modules' in kv:
        os.environ['WEBUI_MODULES'] = kv["modules"]
    call(["gulp"])

# ------------------------------------------------------------------------------
# Hooks
# ------------------------------------------------------------------------------

remove_float_support()

#env.AddPreAction("buildprog", cpp_check)
env.AddPreAction("$BUILD_DIR/src/espurna.ino.cpp.o", build_webui)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_size)
