import re
import pathlib

from typing import Iterable

PWD = pathlib.Path(__file__).parent.resolve()


def make_config_path(name: str) -> pathlib.Path:
    return (PWD / ".." / ".." / "espurna" / "config" / name).resolve()


def raw_support_defines(path: pathlib.Path) -> list[str]:
    out = []

    with path.open("r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("#ifndef") and "_SUPPORT" in line:
                _, name = line.strip().split(" ", 1)
                out.append(name)

    out = [*set(out)]
    out.sort()

    return out


def raw_hardware_defines(path: pathlib.Path) -> list[str]:
    out = []

    with path.open("r", encoding="utf-8") as f:
        for line in f:
            match = re.search(r"^#elif defined\((\w*)\)", line)
            if match:
                out.append(match.group(1))

    out = [*set(out)]
    out.sort()

    return out


def generate_arduino_h(path: pathlib.Path):
    LINES = "-" * 80

    def format_defines(fmt: str, defines: list[str]) -> str:
        return "\n".join([fmt.format(d=d) for d in defines])

    hardware = raw_hardware_defines(
        make_config_path("hardware.h"))

    features = raw_support_defines(
        make_config_path("general.h"))

    sensors = raw_support_defines(
        make_config_path("sensors.h"))

    with path.open("w", encoding="utf-8") as f:
        f.write(rf"""//{LINES}
// These settings are normally provided by PlatformIO
// Uncomment the appropiate line(s) to build from the Arduino IDE
//{LINES}

#pragma once

//{LINES}
// Hardware
//{LINES}

{format_defines("//#define {d}", hardware)}

//{LINES}
// Enable features (values below may not be our default values!)
//{LINES}

{format_defines("//#define {d} 1", features)}

//{LINES}
// Enable sensors (values below may not be our default values!)
//{LINES}

{format_defines("//#define {d} 1", sensors)}
""")


def app_add_compiledb_defines(env):
    from SCons.Script import COMMAND_LINE_TARGETS

    if not "compiledb" in COMMAND_LINE_TARGETS:
        return

    # XXX platformio middlewares env != projenv, cannot emit
    # the node correctly while also swapping its flags
    # at least, with the suggested `env.Object(node, CPPDEFINES=...)`

    defines = []
    for name in ("general.h", "sensors.h"):
        defines.extend(raw_support_defines(make_config_path(name)))

    print("Implicitly enabling the following modules:")
    for define in defines:
        print(f"* {define}")

    env.Append(CPPDEFINES=defines)
