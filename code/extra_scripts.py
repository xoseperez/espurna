#!/usr/bin/env python
import time
Import("env")

# ------------------------------------------------------------------------------
# Utils
# ------------------------------------------------------------------------------

class Color:
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

def check_size(source, target, env):
    time.sleep(1)
    size = target[0].get_size()
    print clr(Color.LIGHT_BLUE, "Binary size: %s bytes" % size)
    #if size > 512000:
    #    print clr(Color.LIGHT_RED, "File too large for OTA!")
    #    Exit(1)

# ------------------------------------------------------------------------------
# Hooks
# ------------------------------------------------------------------------------

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_size)
