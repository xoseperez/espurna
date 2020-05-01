import atexit
import os
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


# emulate .ino concatenation to speed up compilation times
def merge_cpp(sources, output):
    with tempfile.TemporaryFile() as tmp:
        tmp.write(b"// !!! Automatically generated file; DO NOT EDIT !!! \n")
        tmp.write(b'#include "espurna.h"\n')
        for source in sources:
            with open(source, "rb") as fobj:
                tmp.write('# 1 "{}"\n'.format(source.replace("\\", "/")).encode('utf-8'));
                shutil.copyfileobj(fobj, tmp)

        tmp.seek(0)

        with open(output, "wb") as fobj:
            shutil.copyfileobj(tmp, fobj)
        atexit.register(try_remove, output)
