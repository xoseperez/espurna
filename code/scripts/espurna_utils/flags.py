def app_inject_flags(projenv):
    # in theory, could be something different from a single cppdefine, keeping separate from flags below
    board = projenv.get("ESPURNA_BOARD", "")
    if board:
        projenv.Append(CPPDEFINES=[board])

    # only CPPDEFINES make sense here, since we should not change any others
    flags = projenv.get("ESPURNA_FLAGS", "")
    if flags:
        projenv.MergeFlags({"CPPDEFINES": projenv.ParseFlags(flags)["CPPDEFINES"]})
