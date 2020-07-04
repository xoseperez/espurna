import os
import sys


def libalgobsec_inject_patcher(env):
    libalgobsec_builder = next(
        (
            builder
            for builder in env.GetLibBuilders()
            if builder.name == "BSEC Software Library"
        ),
        None,
    )

    if libalgobsec_builder is None:
        return

    def process_archive(target, source, env):
        import subprocess

        # Allows `import espurna_utils` for external scripts, where we might not be within scons runtime
        from SCons.Script import Delete, Mkdir

        tmpdir = env.get(
            "ESPURNA_LIBALGOBSEC_PATCHER_TMPDIR",
            os.path.join(str(target[0].get_dir()), "_tmpdir"),
        )

        env.Execute(Mkdir(tmpdir))

        # XXX: $AR does not support output argument for the extraction
        #      always switch into tmpdir when running commands
        def run(cmd):
            sys.stdout.write(" ".join(cmd))
            sys.stdout.write("\n")
            subprocess.check_call(cmd, cwd=tmpdir)

        run([env.subst("$AR"), "x", source[0].abspath])

        names = []
        for infilename in os.listdir(tmpdir):
            newname = infilename
            if not infilename.endswith(".c.o"):
                newname = infilename.replace(".o", ".c.o")
            os.rename(os.path.join(tmpdir, infilename), os.path.join(tmpdir, newname))
            names.append(newname)

        pack_cmd = [env.subst("$AR"), "cr", target[0].abspath]
        pack_cmd.extend(names)
        run(pack_cmd)

        env.Execute(Delete(tmpdir))

    # Instead of replacing the file in-place, link with the patched version
    libalgobsec_dir = os.path.join(libalgobsec_builder.src_dir, "esp8266")

    target = env.File(
        "libalgobsec.a", directory=env.subst("$BUILD_DIR/libalgobsec_patched")
    )
    source = env.File("libalgobsec.a", directory=libalgobsec_dir)

    command = env.Command(target, source, process_archive)
    patcher = env.Alias("patch-libalgobsec", command)

    env.Append(LIBPATH=[target.get_dir()])
    env.Append(LIBS=["algobsec"])

    env.Depends("$BUILD_DIR/${PROGNAME}.elf", patcher)

