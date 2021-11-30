#!/usr/bin/env python
#
# Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
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

import argparse
import datetime
import logging
import os
import pathlib
import subprocess
import time

from espurna_utils.display import clr, Color

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("main")


def bold(string):
    return clr(Color.BOLD, string)


def format_configurations(configurations):
    return "\n".join(str(cfg) for cfg in configurations)


def build_configurations(args, configurations):
    cmd = ["platformio", "run"]
    if not args.no_silent:
        cmd.extend(["-s"])
    cmd.extend(["-e", args.environment])

    build_time = datetime.timedelta(seconds=0)

    while len(configurations):
        cfg = configurations.pop()
        with open(cfg, "r") as contents:
            log.info("%s contents\n%s", bold(cfg), contents.read())

        os_env = os.environ.copy()
        os_env["PLATFORMIO_BUILD_CACHE_DIR"] = "test/pio_cache"
        if not args.no_single_source:
            os_env["ESPURNA_BUILD_SINGLE_SOURCE"] = "1"

        os_env["PLATFORMIO_SRC_BUILD_FLAGS"] = " ".join(
            [
                '-DMANUFACTURER=\\"TEST_BUILD\\"',
                '-DDEVICE=\\"{}\\"'.format(cfg.stem.replace(" ", "_").upper()),
                '-include "{}"'.format(cfg.resolve().as_posix()),
            ]
        )

        build_start = time.time()
        try:
            subprocess.check_call(cmd, env=os_env)
        except subprocess.CalledProcessError as e:
            log.error("%s failed to build", bold(cfg))
            log.exception(e)
            if configurations:
                log.info(
                    "%s configurations left\n%s",
                    bold(len(configurations)),
                    format_configurations(configurations),
                )
            raise

        diff = datetime.timedelta(seconds=time.time() - build_start)

        firmware_bin = pathlib.Path(".pio/build").joinpath(
            args.environment, "firmware.bin"
        )

        log.info(
            "%s finished in %s, %s is %s bytes",
            *(bold(x) for x in (cfg, diff, firmware_bin, firmware_bin.stat().st_size))
        )

        build_time += diff

    if build_time:
        log.info("Done after %s", bold(build_time))


def main(args):
    if not args.environment:
        log.error("No environment selected")
        return

    log.info("Using env:%s", bold(args.environment))

    configurations = []
    if not args.no_default:
        configurations = list(pathlib.Path(".").glob(args.default_configurations))

    if args.add:
        configurations.extend(args.add)

    if not configurations:
        log.error("No configurations selected")
        return

    log.info(
        "Found %s configurations\n%s",
        bold(len(configurations)),
        format_configurations(configurations),
    )

    if not args.no_build:
        build_configurations(args, configurations)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-n",
        "--no-default",
        action="store_true",
        help="Do not use default configurations (--default-configurations=...)",
    )

    parser.add_argument(
        "-a",
        "--add",
        type=pathlib.Path,
        action="append",
        help="Add path to selected configurations (can specify multiple times)",
    )

    parser.add_argument("-e", "--environment", help="PIO environment")
    parser.add_argument(
        "--default-configurations",
        default="test/build/*.h",
        help="(glob) default configuration headers",
    )
    parser.add_argument(
        "--no-silent", action="store_true", help="Do not silence pio-run"
    )
    parser.add_argument(
        "--no-single-source", action="store_true", help="Disable 'unity' build"
    )
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Stop after listing the available configurations",
    )

    main(parser.parse_args())
