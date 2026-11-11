Import("env")
import os


def _quote(p):
    """Wrap a path in double quotes so spaces survive cmd.exe / shell."""
    return '"{}"'.format(p)


def _app_offsets(env):
    """Offsets of the app slot (ota_0, else the first app) and of the factory partition when there is an ota_0 too,
    from the partition table csv."""
    csv = env.BoardConfig().get("build.partitions", "default.csv")
    path = os.path.join(env.subst("$PROJECT_DIR"), csv)
    if not os.path.exists(path):
        framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
        path = os.path.join(framework_dir, "tools", "partitions", csv)

    apps = {}
    with open(path) as f:
        for line in f:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            cols = [c.strip() for c in line.split(",")]
            if len(cols) >= 4 and cols[1] == "app":
                apps.setdefault(cols[2], cols[3])

    if "ota_0" in apps:
        return apps["ota_0"], apps.get("factory")
    if apps:
        return next(iter(apps.values())), None
    raise Exception("[POST-BUILD] No app partition in {}".format(path))


def merge_bin(source, target, env):
    print("\n[POST-BUILD] Creating merged production binary...")

    platform = env.PioPlatform()
    esptool = os.path.join(platform.get_package_dir("tool-esptoolpy") or "", "esptool.py")
    if not os.path.exists(esptool):
        esptool = "esptool.py"  # fall back to PATH

    build_dir = env.subst("$BUILD_DIR")
    board = env.get("BOARD_MCU") or env.BoardConfig().get("build.mcu", "esp32")
    pio_env = env["PIOENV"]

    # Derive flash size and mode from board config so non-default targets
    # aren't silently mis-flashed (bootloader and image must agree on mode).
    flash_size = env.BoardConfig().get("upload.flash_size", "4MB")
    flash_mode = env.BoardConfig().get("build.flash_mode", "dout")
    flash_freq = env.BoardConfig().get("build.f_flash", "40000000L")
    # f_flash is "40000000L" or "80000000L" — normalize to "40m" / "80m"
    # Refuse to silently fall back to 40 MHz: the bootloader and the image must
    # agree on speed or the chip won't boot, and a bad board config should
    # surface here rather than producing a brick.
    try:
        freq_mhz = int(str(flash_freq).rstrip("L")) // 1000000
    except (TypeError, ValueError):
        raise Exception(
            "[POST-BUILD] Could not parse build.f_flash={!r} from board config".format(flash_freq))
    bootloader_suffix = "{}_{}m".format(flash_mode, freq_mhz)

    output = os.path.join(build_dir, "espurna-{}.bin".format(pio_env))

    bootloader = os.path.join(build_dir, "bootloader.bin")
    if not os.path.exists(bootloader):
        framework_dir = platform.get_package_dir("framework-arduinoespressif32")
        # e.g. bootloader_dout_40m.bin, bootloader_dio_80m.bin, …
        bootloader = os.path.join(framework_dir, "tools", "sdk", "bin",
                                  "bootloader_{}.bin".format(bootloader_suffix))

    partitions = os.path.join(build_dir, "partitions.bin")
    boot_app0 = os.path.join(build_dir, "boot_app0.bin")
    if not os.path.exists(boot_app0):
        framework_dir = platform.get_package_dir("framework-arduinoespressif32")
        boot_app0 = os.path.join(framework_dir, "tools", "partitions", "boot_app0.bin")

    firmware = os.path.join(build_dir, "firmware.bin")
    app_offset, safeboot_offset = _app_offsets(env)

    missing = [f for f in (bootloader, partitions, boot_app0, firmware) if not os.path.exists(f)]
    if missing:
        # Fail the PIO build so CI doesn't publish an un-merged binary as success.
        raise Exception("[POST-BUILD] Missing components for merge: {}".format(", ".join(missing)))

    cmd = (
        "$PYTHONEXE {esptool} --chip {chip} merge_bin -o {out} "
        "--flash_mode {fmode} --flash_size {fsize} "
        "0x1000 {bootloader} "
        "0x8000 {partitions} "
        "0xe000 {boot_app0} "
        "{app_offset} {firmware}"
    ).format(
        esptool=_quote(esptool),
        chip=board,
        out=_quote(output),
        fmode=flash_mode,
        fsize=flash_size,
        bootloader=_quote(bootloader),
        partitions=_quote(partitions),
        boot_app0=_quote(boot_app0),
        firmware=_quote(firmware),
        app_offset=app_offset,
    )

    # Safeboot layout: the factory partition holds the ESPurna safeboot image (code/safeboot-app, built before
    # this one: `pio run -d safeboot-app`, or a copy in code/safeboot/). Without it the merged image still boots
    # (boot_app0 selects ota_0), only the `safeboot` command refuses to switch to an empty partition
    project = env.subst("$PROJECT_DIR")
    candidates = (
        os.path.join(project, "safeboot", "espurna-esp32-safeboot.bin"),
        os.path.join(project, "safeboot-app", ".pio", "build", "esp32-safeboot", "firmware.bin"),
    )
    safeboot = next((path for path in candidates if os.path.exists(path)), None)
    if safeboot_offset and safeboot:
        cmd += " {} {}".format(safeboot_offset, _quote(safeboot))

    print("Running: " + cmd)
    rc = env.Execute(cmd)
    if rc:
        raise Exception("[POST-BUILD] merge_bin failed with exit code {}".format(rc))
    print("[DONE] File ready: {}\n".format(output))


env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bin)
