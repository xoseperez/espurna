import os

from .display import Color, clr, print_filler, print_warning


def check_printsize(target, source, env):
    (binary,) = target
    path = binary.get_abspath()
    size = os.stat(path).st_size
    print(clr(Color.LIGHT_BLUE, "Binary size: {} bytes".format(size)))

    # Warn 1MB variants about exceeding OTA size limit
    flash_size = int(env.BoardConfig().get("upload.maximum_size", 0))
    if (flash_size == 1048576) and (size >= 512000):
        print_filler("*", color=Color.LIGHT_YELLOW, err=True)
        print_warning(
            "File is too large for OTA! Here you can find instructions on how to flash it:"
        )
        print_warning(
            "https://github.com/xoseperez/espurna/wiki/TwoStepUpdates",
            color=Color.LIGHT_CYAN,
        )
        print_filler("*", color=Color.LIGHT_YELLOW, err=True)


def check_cppcheck(target, source, env):
    print_warning("Started cppcheck...\n")
    call(["cppcheck", os.getcwd() + "/espurna", "--force", "--enable=all"])
    print_warning("Finished cppcheck...\n")
