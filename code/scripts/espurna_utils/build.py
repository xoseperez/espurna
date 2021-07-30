import atexit
import os
import shutil
import tempfile
import functools

from .display import print_warning
from .version import app_full_version_for_env


def try_remove(path):
    try:
        os.remove(path)
    except:  # pylint: disable=bare-except
        print_warning("Please manually remove the file `{}`".format(path))


# emulate .ino concatenation to speed up compilation times
def merge_cpp(sources, output):
    with tempfile.TemporaryFile() as tmp:
        tmp.write(b"// !!! Automatically generated file; DO NOT EDIT !!! \n")
        tmp.write(b'#include "espurna.h"\n')
        for source in sources:
            src_include = '#include "{}"\n'.format(source)
            tmp.write(src_include.encode("utf-8"))

        tmp.seek(0)

        with open(output, "wb") as fobj:
            shutil.copyfileobj(tmp, fobj)
        atexit.register(try_remove, output)


def firmware_prefix(env):
    return "espurna-{}".format(app_full_version_for_env(env))


# generate an common name for the current build
def firmware_filename(env):
    suffix = "{}.bin".format(env["ESPURNA_BUILD_NAME"] or env["PIOENV"])
    return "-".join([firmware_prefix(env), suffix])


def firmware_destination(env):
    destdir = env["ESPURNA_BUILD_DESTINATION"] or env["PROJECT_DIR"]
    subdir = os.path.join(destdir, firmware_prefix(env))

    return os.path.join(subdir, firmware_filename(env))


def app_add_target_build_and_copy(env):
    from SCons.Script import Copy

    copy_dest = firmware_destination(env)
    copy = env.Command(
        copy_dest, "${BUILD_DIR}/${PROGNAME}.bin", Copy("$TARGET", "$SOURCE")
    )
    env.AddTarget(
        "build-and-copy",
        copy_dest,
        actions=None,  # command invocation already handles this
        title="Build firmware.bin and store a copy",
        description="Build and store firmware.bin as $ESPURNA_BUILD_DESTINATION/espurna-<version>-$ESPURNA_BUILD_NAME.bin (default destination is $PROJECT_DIR)",
    )
