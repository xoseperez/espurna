import os
import shutil

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

