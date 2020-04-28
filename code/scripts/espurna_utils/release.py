import atexit
import os
import pathlib
import shutil
import tempfile

from .display import print_warning


def try_remove(path):
    try:
        os.remove(path)
    except:  # pylint: disable=bare-except
        print_warning("Please manually remove the file `{}`".format(path))


def copy_release(target, source, env):
    # target filename and subdir for release files
    name = env["ESPURNA_RELEASE_NAME"]
    version = env["ESPURNA_RELEASE_VERSION"]
    destdir = env["ESPURNA_RELEASE_DESTINATION"]

    if not name or not version or not destdir:
        raise ValueError("Cannot set up release without release variables present")

    if not os.path.exists(destdir):
        os.makedirs(destdir)

    dest = os.path.join(
        destdir, "espurna-{version}-{name}.bin".format(version=version, name=name)
    )
    src = env.subst("$BUILD_DIR/${PROGNAME}.bin")

    shutil.copy(src, dest)


def merge_cpp(srcdir, result_file):
    to_restore = []
    result_path = os.path.join(srcdir, result_file)

    with tempfile.TemporaryFile() as tmp:
        tmp.write(b'#include "espurna.h"\n')
        for source in pathlib.Path(srcdir).iterdir():
            # emulate .ino concatenation to speed up compilation times
            if source.suffix == ".cpp":
                to_restore.append(str(source))
                with source.open(mode="rb") as source_file:
                    shutil.copyfileobj(source_file, tmp)

        tmp.seek(0)

        with open(result_path, "wb") as result:
            shutil.copyfileobj(tmp, result)
        atexit.register(try_remove, result_path)
