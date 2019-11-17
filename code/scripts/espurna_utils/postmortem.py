def dummy_ets_printf(target, source, env):
    (postmortem_src_file,) = source
    (postmortem_obj_file,) = target

    cmd = ["xtensa-lx106-elf-objcopy"]

    # recent Core switched to cpp+newlib & ets_printf_P
    cmd.extend(["--redefine-sym", "ets_printf=dummy_ets_printf"])
    cmd.extend(["--redefine-sym", "ets_printf_P=dummy_ets_printf"])

    cmd.append(postmortem_obj_file.get_abspath())
    env.Execute(env.VerboseAction(" ".join(cmd), "Removing ets_printf / ets_printf_P"))
    env.Depends(postmortem_obj_file, "$BUILD_DIR/src/dummy_ets_printf.c.o")
