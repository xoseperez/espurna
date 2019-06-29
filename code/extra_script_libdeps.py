Import("env")

import os

def subprocess_libdeps(storage, lib_deps):
    import subprocess
    args = [env.subst("$PYTHONEXE"), "-mplatformio", "lib", "-d", storage, "install"]
    args.extend(lib_deps)

    subprocess.check_call(args)

def library_manager_libdeps(storage, lib_deps):
    from platformio.managers.lib import LibraryManager
    manager = LibraryManager(storage)
    for lib in lib_deps:
        print(lib)
        print(*manager.parse_pkg_uri(lib))
        if manager.get_package_dir(*manager.parse_pkg_uri(lib)):
            continue
        print("installing", lib)
        manager.install(lib)

if os.environ.get("ESPURNA_PIO_SHARED_LIBRARIES"):
    cfg = env.GetProjectConfig()

    lib_deps = env.GetProjectOption("lib_deps")
    storage = os.path.join(env["PROJECT_DIR"], cfg.get("common", "shared_libdeps_dir"))

    print("using shared library storage: ", storage)
    #library_manager_libdeps(STORAGE, LIB_DEPS)
    subprocess_libdeps(storage, lib_deps)
