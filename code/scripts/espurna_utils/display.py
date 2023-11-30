import os
import sys


class Color(object):
    BOLD = "\x1b[1;1m"
    BLACK = "\x1b[1;30m"
    RED = "\x1b[1;31m"
    GREEN = "\x1b[1;32m"
    YELLOW = "\x1b[1;33m"
    BLUE = "\x1b[1;34m"
    MAGENTA = "\x1b[1;35m"
    CYAN = "\x1b[1;36m"
    WHITE = "\x1b[1;37m"
    LIGHT_GREY = "\x1b[0;30m"
    LIGHT_RED = "\x1b[0;31m"
    LIGHT_GREEN = "\x1b[0;32m"
    LIGHT_YELLOW = "\x1b[0;33m"
    LIGHT_BLUE = "\x1b[0;34m"
    LIGHT_MAGENTA = "\x1b[0;35m"
    LIGHT_CYAN = "\x1b[0;36m"
    LIGHT_WHITE = "\x1b[0;37m"


def clr(color, text):
    return color + str(text) + "\x1b[0m"


def print_warning(message, color=Color.LIGHT_YELLOW):
    print(clr(color, message), file=sys.stderr)


def print_filler(fill, color=Color.WHITE, err=False, width_default=80):
    width = width_default
    try:
        width = int(os.environ["COLUMNS"])
    except (KeyError, ValueError):
        pass

    if len(fill) > 1:
        fill = fill[0]

    out = sys.stderr if err else sys.stdout
    print(clr(color, fill * width), file=out)
