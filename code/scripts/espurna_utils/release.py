import os
import shutil

def copy_release(target, source, env):
    # target filename and subdir for release files
    name = env["ESPURNA_NAME"]
    version = env["ESPURNA_VERSION"]

    if not name or not version:
        raise ValueError("Cannot set up release without release variables present")

    destdir = os.path.join(env.subst("$PROJECT_DIR"), "..", "firmware", version)
    if not os.path.exists(destdir):
        os.makedirs(destdir)

    dest = os.path.join(
        destdir, "espurna-{name}-{version}.bin".format(name=name, version=version)
    )
    src = env.subst("$BUILD_DIR/${PROGNAME}.bin")

    shutil.copy(src, dest)

