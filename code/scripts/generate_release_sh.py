import os
import re
import sys
import configparser
import collections

CI = any([os.environ.get("TRAVIS"), os.environ.get("CI")])
Build = collections.namedtuple("Build", "env extends build_flags src_build_flags")


def get_builds(cfg):
    RE_STRIP_VARS = re.compile("\$\{.*\}")
    RE_STRIP_NEWLINE = re.compile("\r\n|\n")

    for section in cfg.sections():
        if (not section.startswith("env:")) or (
            section.startswith("env:esp8266-") and section.endswith("-base")
        ):
            continue

        build_flags = None
        src_build_flags = None

        try:
            build_flags = cfg.get(section, "build_flags")
            build_flags = RE_STRIP_VARS.sub("", build_flags).strip()
            build_flags = RE_STRIP_NEWLINE.sub(" ", build_flags).strip()
        except configparser.NoOptionError:
            pass

        try:
            src_build_flags = cfg.get(section, "src_build_flags")
            src_build_flags = RE_STRIP_VARS.sub("", src_build_flags).strip()
            src_build_flags = RE_STRIP_NEWLINE.sub(" ", src_build_flags).strip()
        except configparser.NoOptionError:
            pass

        yield Build(
            section.replace("env:", ""),
            cfg.get(section, "extends").replace("env:", ""),
            build_flags,
            src_build_flags,
        )


def generate_lines(builds):
    cores = []
    generic = []

    for build in builds:

        flags = []
        if build.build_flags:
            flags.append('PLATFORMIO_BUILD_FLAGS="{}"'.format(build.build_flags))
        if build.src_build_flags:
            flags.append('ESPURNA_FLAGS="{}"'.format(build.src_build_flags))
        flags.append('ESPURNA_NAME="{env}"'.format(env=build.env))

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
        raise SystemExit("* Not in CI *")
    if len(sys.argv) != 2:
        raise SystemExit("* Invalid arguments *")

    Config = configparser.ConfigParser()
    with open("platformio.ini", "r") as f:
        Config.read_file(f)

    builder_total_threads = int(os.environ["BUILDER_TOTAL_THREADS"])
    builder_thread = int(os.environ["BUILDER_THREAD"])
    version = sys.argv[1]

    builds = every(get_builds(Config), builder_thread, builder_total_threads)

    print("#!/bin/bash")
    print("set -e -x")
    print('export ESPURNA_VERSION="{}"'.format(version))
    print('trap "ls -l ${TRAVIS_BUILD_DIR}/firmware/${ESPURNA_VERSION}" EXIT')
    print(
        'echo "Selected thread #{} out of {}"'.format(
            builder_thread, builder_total_threads
        )
    )
    for line in generate_lines(builds):
        print(line)
