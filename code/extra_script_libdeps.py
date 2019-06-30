from __future__ import print_function

Import("env")

import os
import sys

def subprocess_libdeps(storage, lib_deps):
    import subprocess
    args = [env.subst("$PYTHONEXE"), "-mplatformio", "lib", "-d", storage, "install"]
    args.extend(lib_deps)

    subprocess.check_call(args)

def library_manager_libdeps(storage, lib_deps):
    from platformio.managers.lib import LibraryManager
    manager = LibraryManager(storage)
    for lib in lib_deps:
        if manager.get_package_dir(*manager.parse_pkg_uri(lib)):
            continue
        print("installing", lib, file=sys.stderr)
        manager.install(lib)

if os.environ.get("ESPURNA_PIO_SHARED_LIBRARIES"):
    cfg = env.GetProjectConfig()

    lib_deps = env.GetProjectOption("lib_deps")
    storage = os.path.join(env["PROJECT_DIR"], cfg.get("common", "shared_libdeps_dir"))

    print("using shared library storage: ", storage, file=sys.stderr)
    #library_manager_libdeps(STORAGE, LIB_DEPS)
    subprocess_libdeps(storage, lib_deps)
