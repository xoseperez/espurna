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


PWD = pathlib.Path(__file__).resolve().parent

ROOT_PATH = (PWD / "..").resolve()
TEST_PATH = (ROOT_PATH / "test" / "build").resolve()

CONFIG_PATH = TEST_PATH / "config"
CACHE_PATH = TEST_PATH / "cache"
BUILD_PATH = ROOT_PATH / ".pio" / "build"

CACHE_TIMEDELTA = datetime.timedelta(days=1)
TIMEDELTA_PARAM = {
    "d": "days",
    "h": "hours",
    "m": "minutes",
    "s": "seconds",
}


def bold(string: str):
    return clr(Color.BOLD, string)


def format_configurations(configurations: list[pathlib.Path]):
    return "\n".join(str(cfg) for cfg in configurations)


def pluralize(string: str, length: int):
    if length > 1:
        return f"{string}s"

    return string


def make_timedelta(string: str) -> datetime.timedelta:
    if not string:
        return datetime.timedelta(seconds=0)

    if string.isdigit():
        suffix = "d"
    else:
        suffix = string[-1]
        string = string[:-1]

    param = TIMEDELTA_PARAM[suffix]
    value = int(string, 10)

    return datetime.timedelta(**{param: value})


def cache_cleanup(cache_path: pathlib.Path, offset: datetime.timedelta):
    now = datetime.datetime.now()

    for pair in cache_path.iterdir():
        # {CACHE_DIR} / AA / AA...rest of the hash...
        if not pair.is_dir():
            continue

        for f in pair.iterdir():
            mtime_raw = f.stat().st_mtime
            mtime_dt = datetime.datetime.fromtimestamp(mtime_raw)

            if now - mtime_dt > offset:
                f.unlink()

        if not any(pair.iterdir()):
            pair.rmdir()


def build_configurations(args: argparse.Namespace, configurations: list[pathlib.Path]):
    cache_path = args.cache_path.resolve()

    if cache_path.is_dir():
        cache_cleanup(cache_path, args.expire_cache)

    cmd = ["platformio", "run"]
    if args.silent:
        cmd.extend(["-s"])
    cmd.extend(["-e", args.environment])

    build_time = datetime.timedelta(seconds=0)

    while configurations:
        cfg = configurations.pop()
        log.info("%s contents\n%s", bold(cfg.name), cfg.read_text())

        os_env = os.environ.copy()
        os_env["PLATFORMIO_BUILD_CACHE_DIR"] = cache_path.resolve().as_posix()
        if args.single_source:
            os_env["ESPURNA_BUILD_SINGLE_SOURCE"] = "1"

        os_env["PLATFORMIO_BUILD_SRC_FLAGS"] = " ".join(
            [
                '-DMANUFACTURER=\\"TEST_BUILD\\"',
                '-DDEVICE=\\"{}\\"'.format(cfg.stem.replace(" ", "_").upper()),
                '-include "{}"'.format(cfg.resolve().as_posix()),
            ]
        )

        build_start = time.time()
        try:
            subprocess.check_call(cmd, env=os_env)
        except subprocess.CalledProcessError:
            log.error("%s failed to build", bold(str(cfg)))
            if configurations:
                log.info(
                    "%s %s left\n%s",
                    bold(str(len(configurations))),
                    pluralize("configuration", len(configurations)),
                    format_configurations(configurations),
                )
            raise

        diff = datetime.timedelta(seconds=time.time() - build_start)

        firmware_bin = args.build_path / args.environment / "firmware.bin"

        log.info(
            "%s finished in %s, %s is %s bytes",
            *(
                bold(str(x))
                for x in (cfg, diff, firmware_bin, firmware_bin.stat().st_size)
            ),
        )

        build_time += diff

    if build_time:
        log.info("Done after %s", bold(str(build_time)))


def main(args: argparse.Namespace):
    if not args.environment:
        log.error("No environment selected")
        return

    log.info("Using [env:%s]", bold(args.environment))

    configurations = [pathlib.Path(x) for x in args.configurations]
    if not configurations:
        configurations = [x for x in args.config_path.glob("*.h")]

    configurations = [x.resolve() for x in configurations]

    if args.start_from:
        offset = 0
        for n, p in enumerate(configurations, start=1):
            if args.start_from in p.name:
                offset = n
                break

        if offset:
            for cfg in configurations[offset:]:
                log.info("Skipping %s", cfg)
            configurations = configurations[:offset]

    if args.filter:
        configurations = [p for p in configurations for f in args.filter if f in p.name]

    if not configurations:
        log.error("No configurations selected")
        return

    log.info(
        "Found %s %s\n%s",
        bold(str(len(configurations))),
        pluralize("configuration", len(configurations)),
        format_configurations(configurations),
    )

    if args.build:
        build_configurations(args, configurations)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument("-e", "--environment", help="PIO environment")

    parser.add_argument(
        "--start-from",
        help="Skip configurations until this string is found in configuration filename",
        default="",
    )

    parser.add_argument(
        "--filter",
        action="append",
        help="Only build configurations with filenames containing this string",
    )

    parser.add_argument(
        "--silent",
        default=True,
        action=argparse.BooleanOptionalAction,
        help="Silence PlatformIO output",
    )

    parser.add_argument(
        "--single-source",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Use SCons 'unity' build",
    )

    parser.add_argument(
        "--build",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Build specified configurations",
    )

    parser.add_argument(
        "--config-path",
        default=CONFIG_PATH,
        type=pathlib.Path,
        help="Directory with build test configuration headers",
    )

    parser.add_argument(
        "--cache-path",
        default=CACHE_PATH,
        type=pathlib.Path,
        help="PlatformIO SCons cache path",
    )

    parser.add_argument(
        "--build-path",
        default=BUILD_PATH,
        type=pathlib.Path,
        help="PlatformIO build path",
    )

    parser.add_argument(
        "--expire-cache",
        default=CACHE_TIMEDELTA,
        type=make_timedelta,
        help="PlatformIO cache expiration time (NUMBER or NUMBER{d,h,m,s} for days, hours, minutes or seconds respectively)}",
    )

    parser.add_argument(
        "configurations",
        nargs="*",
        default=[],
    )

    main(parser.parse_args())
