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

import os
import argparse
import re
import shlex
import configparser
import collections

CI = any([os.environ.get("TRAVIS"), os.environ.get("CI")])
Build = collections.namedtuple("Build", "env extends build_flags src_build_flags")


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
        src_build_flags = None

        try:
            build_flags = cfg.get(section, "build_flags")
            build_flags = RE_NEWLINE.sub(" ", build_flags).strip()
            build_flags = " ".join(
                BASE_BUILD_FLAGS ^ set(shlex.split(expand_variables(cfg, build_flags)))
            )
        except configparser.NoOptionError:
            pass

        try:
            src_build_flags = cfg.get(section, "src_build_flags")
            src_build_flags = RE_NEWLINE.sub(" ", src_build_flags).strip()
            src_build_flags = expand_variables(cfg, src_build_flags)
        except configparser.NoOptionError:
            pass

        yield Build(
            section.replace("env:", ""),
            cfg.get(section, "extends").replace("env:", ""),
            build_flags,
            src_build_flags,
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
        if build.src_build_flags:
            flags.append('ESPURNA_FLAGS="{}"'.format(build.src_build_flags))
        flags.append('ESPURNA_RELEASE_NAME="{env}"'.format(env=build.env))
        flags.append("ESPURNA_BUILD_SINGLE_SOURCE=1")

        cmd = ["env"]
        cmd.extend(flags)
        cmd.extend(["pio", "run", "-e", build.extends, "-s", "-t", "release"])

        line = " ".join(cmd)

        # push core variants to the front as they definetly include global build_flags
        output = generic
        if "ESPURNA_CORE" in build.src_build_flags:
            output = cores

        output.append(line)

    return cores + generic


def every(seq, nth, total):
    index = 0
    for value in seq:
        if index == nth:
            yield value
        index = (index + 1) % total


if __name__ == "__main__":
    if not CI:
        raise ValueError("* Not in CI *")

    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--destination", required=True)
    parser.add_argument("--ignore", action="append")
    args = parser.parse_args()

    Config = configparser.ConfigParser()
    with open("platformio.ini", "r") as f:
        Config.read_file(f)

    builder_total_threads = int(os.environ["BUILDER_TOTAL_THREADS"])
    builder_thread = int(os.environ["BUILDER_THREAD"])
    if builder_thread >= builder_total_threads:
        raise ValueError("* Builder thread index out of range *")

    builds = every(get_builds(Config), builder_thread, builder_total_threads)

    print("#!/bin/bash")
    print("set -e -x")
    print('export ESPURNA_RELEASE_VERSION="{}"'.format(args.version))
    print('export ESPURNA_RELEASE_DESTINATION="{}"'.format(args.destination))
    print('trap "ls -l ${ESPURNA_RELEASE_DESTINATION}" EXIT')
    print(
        'echo "Selected thread #{} out of {}"'.format(
            builder_thread + 1, builder_total_threads
        )
    )
    for line in generate_lines(builds, args.ignore or ()):
        print(line)
