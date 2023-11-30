import os
import shutil
import tempfile

from .version import app_full_version_for_env


# to avoid distributing the original .elf, just extract the debug symbols
# which then can be used /w addr2line (since it would still be an .elf format)
def app_add_extract_debug_symbols(env):
    def builder_generator(target, source, env, for_signature):
        return env.VerboseAction(
            "$OBJCOPY --only-keep-debug --compress-debug-sections $SOURCE $TARGET",
            "Extracting debug symbols from $SOURCE",
        )

    env.Append(
        BUILDERS={
            "ExtractDebugSymbols": env.Builder(
                generator=builder_generator, suffix=".debug", src_suffix=".elf"
            )
        }
    )


# extra builder code to compress our output
def app_add_gzip_file(env):
    def gzip_target(target, source, env):
        import gzip
        import shutil

        with open(str(source[0]), "rb") as input:
            with gzip.open(str(target[0]), "wb") as output:
                shutil.copyfileobj(input, output)

    def builder_generator(target, source, env, for_signature):
        return env.VerboseAction(gzip_target, "Compressing $SOURCE")

    env.Append(
        BUILDERS={
            "GzipFile": env.Builder(
                generator=builder_generator, suffix=".gz", src_suffix=".bin"
            )
        }
    )

    env.GzipFile("$BUILD_DIR/${PROGNAME}.bin")


# emulate .ino concatenation to speed up compilation times
def merge_cpp(target, source, env, encoding="utf-8"):
    with tempfile.TemporaryFile() as tmp:
        tmp.write(b"// !!! Automatically generated file; DO NOT EDIT !!! \n")
        tmp.write(
            '#include "{}"\n'.format(
                env.File("${PROJECT_DIR}/espurna/espurna.h").get_abspath()
            ).encode(encoding)
        )
        for src in source:
            src_include = '#include "{}"\n'.format(src.get_abspath())
            tmp.write(src_include.encode(encoding))

        tmp.seek(0)

        with open(target[0].get_abspath(), "wb") as fobj:
            shutil.copyfileobj(tmp, fobj)


def app_add_builder_single_source(env):
    # generate things in the $BUILD_DIR, so there's no need for any extra clean-up code
    source = os.path.join("${BUILD_DIR}", "espurna_single_source", "src", "main.cpp")
    env.SetDefault(ESPURNA_SINGLE_SOURCE_TARGET=source)

    dep = os.path.join("${BUILD_DIR}", "espurna_single_source", "src", "main.cpp.d")
    env.SetDefault(ESPURNA_SINGLE_SOURCE_DEP=dep)

    env.SideEffect(dep, source)

    # also allow to generate .E file from the .cpp, so we can inspect build flags
    env.SetDefault(PREPROCESSCOM=env["CXXCOM"].replace("-c", "-M -MF $ESPURNA_SINGLE_SOURCE_DEP -E"))

    # Create pseudo-builder and add to enviroment
    def builder_generator(target, source, env, for_signature):
        return env.VerboseAction(
            "$PREPROCESSCOM",
            "Preprocessing $SOURCE",
        )

    env.Append(
        BUILDERS={
            "PreProcess": env.Builder(
                generator=builder_generator, suffix=".E", src_suffix=".cpp"
            )
        }
    )

    # substitute a single node instead of building it somewhere else as a lib or extra source dir
    # (...and since we can't seem to modify src_filter specifically for the project dir, only middleware works :/)
    def ignore_node(node):
        if node.name.endswith("main.cpp"):
            return env.File(source)
        return None

    project = env.Dir("${PROJECT_DIR}/espurna")
    env.AddBuildMiddleware(ignore_node, os.path.join(project.get_abspath(), "*.cpp"))
    env.Command(
        source,
        env.Glob("${PROJECT_DIR}/espurna/*.cpp"),
        env.VerboseAction(merge_cpp, "Merging project sources into $TARGET"),
    )


# common name for all our output files (.bin, .elf, .map, etc.)


def firmware_prefix(env):
    return f"espurna-{app_full_version_for_env(env)}"


def firmware_filename(env):
    return "-".join(
        [firmware_prefix(env), env.get("ESPURNA_BUILD_NAME", env["PIOENV"])]
    )


def firmware_destination(env):
    dest = env.get("ESPURNA_BUILD_DESTINATION")

    # implicit default to a local directory
    if not dest:
        dest = "${PROJECT_DIR}/build"
    # its a SCons var
    elif dest.startswith("$"):
        pass
    # due to runtime (?) quirks, we will end up in scripts/
    # without specifying this as relative to the projdir
    elif not dest.startswith("/"):
        dest = f"${{PROJECT_DIR}}/{dest}"

    return env.Dir(dest)


def app_add_target_build_and_copy(env):
    env.Replace(ESPURNA_BUILD_DESTINATION=firmware_destination(env))
    env.Replace(ESPURNA_BUILD_FILENAME=firmware_filename(env))

    app_add_extract_debug_symbols(env)
    env.ExtractDebugSymbols("$BUILD_DIR/${PROGNAME}")

    env.InstallAs(
        "${ESPURNA_BUILD_DESTINATION}/${ESPURNA_BUILD_FILENAME}.bin",
        "$BUILD_DIR/${PROGNAME}.bin",
    )
    for suffix in ("map", "elf.debug"):
        env.InstallAs(
            f"${{ESPURNA_BUILD_DESTINATION}}/debug/${{ESPURNA_BUILD_FILENAME}}.{suffix}",
            f"$BUILD_DIR/${{PROGNAME}}.{suffix}",
        )

    env.Alias("install", "$ESPURNA_BUILD_DESTINATION")
    env.Alias("build-and-copy", ["$BUILD_DIR/${PROGNAME}.bin", "install"])


# NOTICE that .re <-> .re.ipp dependency is tricky, b/c we want these to exist *before* any source is built
# (or, attempted to be built. `projenv` does not exist yet, and so there are no dependecies generated)


def app_add_target_build_re2c(env):
    from SCons.Script import COMMAND_LINE_TARGETS

    targets = []

    for target in COMMAND_LINE_TARGETS:
        if target.endswith(".re.ipp"):
            targets.append(target)

    if targets:
        action = env.VerboseAction(
            "re2c --no-generation-date --case-ranges -W -Werror -o $TARGET $SOURCE",
            "Generating $TARGET",
        )

        for target in targets:
            action(
                [env.File(target)], [env.File(target.replace(".re.ipp", ".re"))], env
            )
        env.Exit(0)


# c/p giant hack from https://github.com/platformio/platformio-core/issues/4574
# force node path used for signature to be relative to `BUILD_DIR` instead of `PROJECT_DIR`
# thus, forcing CacheDir to share results between multiple environments
def app_patch_cachedir(env):
    from SCons.Node.FS import hash_collect, File

    build_dir = env["BUILD_DIR"]

    def patched_get_cachedir_bsig(self):
        try:
            return self.cachesig
        except AttributeError:
            pass

        children = self.children()
        sigs = [n.get_cachedir_csig() for n in children]

        sigs.append(self.get_contents_sig())
        sigs.append(self.get_path(build_dir))

        result = self.cachesig = hash_collect(sigs)

        return result

    File.get_cachedir_bsig = patched_get_cachedir_bsig
