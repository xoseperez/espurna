import pathlib
import itertools
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

    return out


def make_support_defines(defines: Iterable[str]) -> list[tuple[str, str]]:
    return [(x, "1") for x in defines]


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
