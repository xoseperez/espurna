#!/usr/bin/env python
#
# Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function

import time
import glob
import argparse
import atexit
import subprocess
import os
import sys
import datetime
from espurna_utils.display import Color, clr, print_warning

CUSTOM_HEADER = "espurna/config/custom.h"
if os.path.exists(CUSTOM_HEADER):
    raise SystemExit(
        clr(
            Color.YELLOW,
            "{} already exists, please run this script in a git-worktree(1) or a separate directory".format(
                CUSTOM_HEADER
            ),
        )
    )


def try_remove(path):
    try:
        os.remove(path)
    except: # pylint: disable=bare-except
        print_warning("Please manually remove the file `{}`".format(path))


atexit.register(try_remove, CUSTOM_HEADER)


def main(args):
    configurations = []
    if not args.no_default:
        configurations = list(glob.glob(args.default_configurations))

    configurations.extend(x for x in (args.add or []))
    if not configurations:
        raise SystemExit(clr(Color.YELLOW, "No configurations selected"))

    print(clr(Color.BOLD, "> Selected configurations:"))
    for cfg in configurations:
        print(cfg)
    if args.list:
        return

    if not args.environment:
        raise SystemExit(clr(Color.YELLOW, "No environment selected"))
    print(clr(Color.BOLD, "> Selected environment: {}".format(args.environment)))

    for cfg in configurations:
        print(clr(Color.BOLD, "> Building {}".format(cfg)))
        with open(CUSTOM_HEADER, "w") as custom_h:

            def write(line):
                sys.stdout.write(line)
                custom_h.write(line)

            name, _ = os.path.splitext(cfg)
            name = os.path.basename(name)
            write('#define MANUFACTURER "TEST_BUILD"\n')
            write('#define DEVICE "{}"\n'.format(name.upper()))
            with open(cfg, "r") as cfg_file:
                for line in cfg_file:
                    write(line)

        os_env = os.environ.copy()
        os_env["PLATFORMIO_SRC_BUILD_FLAGS"] = "-DUSE_CUSTOM_H"
        os_env["PLATFORMIO_BUILD_CACHE_DIR"] = "test/pio_cache"
        cmd = ["platformio", "run", "-s", "-e", args.environment]

        start = time.time()
        subprocess.check_call(cmd, env=os_env)
        end = time.time()
        print(
            clr(
                Color.BOLD,
                "> {}: {} bytes, {}".format(
                    cfg,
                    os.stat(
                        os.path.join(".pio", "build", args.environment, "firmware.bin")
                    ).st_size,
                    datetime.timedelta(seconds=(end - start)),
                ),
            )
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-l", "--list", action="store_true", help="List selected configurations"
    )
    parser.add_argument(
        "-n",
        "--no-default",
        action="store_true",
        help="Do not use default configurations (--default-configurations=...)",
    )
    parser.add_argument(
        "-a",
        "--add",
        action="append",
        help="Add path to selected configurations (can specify multiple times)",
    )
    parser.add_argument("-e", "--environment", help="PIO environment")
    parser.add_argument(
        "--default-configurations",
        default="test/build/*.h",
        help="(glob) default configuration headers",
    )

    main(parser.parse_args())
