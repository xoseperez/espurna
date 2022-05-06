#!/usr/bin/env python
#
# Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>
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
import re
import shlex
import configparser
import collections

Build = collections.namedtuple("Build", "env extends build_flags build_src_flags")


def expand_variables(cfg, value):
    RE_VARS = re.compile("\$\{.*?\}")

    for var in RE_VARS.findall(value):
        section, option = var.replace("${", "").replace("}", "").split(".", 1)
        value = value.replace(var, expand_variables(cfg, cfg.get(section, option)))

    return value


def get_builds(cfg):
    RE_NEWLINE = re.compile("\r\n|\n")
    BASE_BUILD_FLAGS = set(
        shlex.split(expand_variables(cfg, cfg.get("env", "build_flags")))
    )

    for section in cfg.sections():
        if (not section.startswith("env:")) or (
            section.startswith("env:esp8266-") and section.endswith("-base")
        ):
            continue

        build_flags = None
        build_src_flags = None

        try:
            build_flags = cfg.get(section, "build_flags")
            build_flags = RE_NEWLINE.sub(" ", build_flags).strip()
            build_flags = " ".join(
                BASE_BUILD_FLAGS ^ set(shlex.split(expand_variables(cfg, build_flags)))
            )
        except configparser.NoOptionError:
            pass

        try:
            build_src_flags = cfg.get(section, "build_src_flags")
            build_src_flags = RE_NEWLINE.sub(" ", build_src_flags).strip()
            build_src_flags = expand_variables(cfg, build_src_flags)
        except configparser.NoOptionError:
            pass

        yield Build(
            section.replace("env:", ""),
            cfg.get(section, "extends").replace("env:", ""),
            build_flags,
            build_src_flags,
        )


def find_any(string, values):
    for value in values:
        if value in string:
            return True

    return False


def generate_lines(builds, ignore):
    cores = []
    generic = []

    for build in builds:
        if find_any(build.env, ignore):
            continue

        flags = []
        if build.build_flags:
            flags.append('PLATFORMIO_BUILD_FLAGS="{}"'.format(build.build_flags))
        if build.build_src_flags:
            flags.append('ESPURNA_FLAGS="{}"'.format(build.build_src_flags))
        flags.append('ESPURNA_BUILD_NAME="{env}"'.format(env=build.env))

        cmd = ["env"]
        cmd.extend(flags)
        cmd.extend(["pio", "run", "-e", build.extends, "-s", "-t", "build-and-copy"])

        line = " ".join(cmd)

        # push core variants to the front as they definetly include global build_flags
        output = generic
        if "ESPURNA_CORE" in build.build_src_flags:
            output = cores

        output.append(line)

    return cores + generic


def every(seq, nth, total):
    index = 0
    for value in seq:
        if index == nth:
            yield value
        index = (index + 1) % total


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--destination", help="Where to place the resulting .bin", required=True
    )
    parser.add_argument(
        "--single-source",
        help="Combine .cpp files into one to speed up compilation",
        action="store_true",
        default=True,
    )
    parser.add_argument(
        "--ignore", help="Do not build envs that contain the string(s)", action="append"
    )

    builder_thread = parser.add_argument_group(
        title="Builder thread control for CI parallel builds"
    )
    builder_thread.add_argument("--builder-thread", type=int, required=True)
    builder_thread.add_argument("--builder-total-threads", type=int, required=True)

    full_version = parser.add_argument_group(
        title="Fully replace the version string for the build system"
    )
    full_version.add_argument("--full-version")

    version_parts = parser.add_argument_group(
        "Replace parts of the version string that would have been detected by the build system"
    )
    version_parts.add_argument("--version")
    version_parts.add_argument("--revision")
    version_parts.add_argument("--suffix")

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    Config = configparser.ConfigParser()
    with open("platformio.ini", "r") as f:
        Config.read_file(f)

    builder_total_threads = args.builder_total_threads
    builder_thread = args.builder_thread
    if builder_thread >= builder_total_threads:
        raise ValueError("* Builder thread index out of range *")

    builds = every(get_builds(Config), builder_thread, builder_total_threads)

    print("#!/bin/bash")
    print("set -e -x")

    variables = [
        ["ESPURNA_BUILD_DESTINATION", args.destination],
        ["ESPURNA_BUILD_SINGLE_SOURCE", int(args.single_source)],
        ["ESPURNA_BUILD_FULL_VERSION", args.full_version],
        ["ESPURNA_BUILD_VERSION", args.version],
        ["ESPURNA_BUILD_REVISION", args.revision],
        ["ESPURNA_BUILD_VERSION_SUFFIX", args.suffix]]

    for var, value in variables:
        if value or not value is None:
            print("export {}=\"{}\"".format(var, value))

    print('trap "ls -R ${ESPURNA_BUILD_DESTINATION}" EXIT')
    print(
        'echo "Selected thread #{} out of {}"'.format(
            builder_thread + 1, builder_total_threads
        )
    )
    for line in generate_lines(builds, args.ignore or ()):
        print(line)
