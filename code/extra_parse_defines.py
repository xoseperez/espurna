Import("env")

#from SCons.cpp import PreProcessor
#flags = env.ParseFlags(env.get("BUILD_FLAGS"))
#
#pp = PreProcessor(current=env.subst('$PROJECT_DIR/espurna/config'), dict=defines)
#print(pp(env.subst('$PROJECT_DIR/espurna/config/all.h')))
#res = pp.cpp_namespace
#print(res["MANUFACTURER"])
#print(res["DEVICE"])
#
#import sys
#sys.exit()

import os
import re
from io import StringIO
from subprocess import call, Popen, PIPE
from SCons.Scanner.C import dictify_CPPDEFINES


re_define = re.compile(r"^[\s]*?#define[\s]+?(?P<key>\w+)[\s]+?(?P<value>.*$)")


def read_header(header):
    header_data = None

    with open(header) as h_file:
        lines = []
        for line in h_file:
            line = line.decode('utf-8')
            if u"prototypes" in line:
                continue
            lines.append(line)
        header_data = u"".join(lines)

    return header_data


def prepare_cmdline(env):
    cmd = [os.path.sep.join([
        os.path.expanduser("~"), ".platformio", "packages",
        "toolchain-xtensa", "bin", "xtensa-lx106-elf-gcc"])]

    build_flags_defines = dictify_CPPDEFINES(env.ParseFlags(env.get("BUILD_FLAGS")))
    for k, v in build_flags_defines.items():
        if v:
            flag = "-D{}=v".format(k, v)
        else:
            flag = "-D{}".format(k)
        cmd.append(flag)

    # gcc needs somewhere to look for header files
    cmd.extend(["-I", env.subst("$PROJECT_DIR/espurna/config/")])

    # use c++ language, do not expand definitions, stop after preprocessor
    # can debug using -C flag - it will insert comments with origin of #define
    cmd.extend(["-x", "c++", "-dD", "-E", "-"])

    return cmd


def parse_config_header(env, header=env.subst("$PROJECT_DIR/espurna/config/all.h")):
    header_data = read_header(header)
    cmd = prepare_cmdline(env)
    
    print("cat {} | {}".format(header, " ".join(cmd)))

    proc = Popen(cmd, stdout=PIPE, stderr=PIPE, stdin=PIPE) 
    stdout, stderr = proc.communicate(header_data)

    defines = {}
    for line in StringIO(stdout.decode("utf-8")):
        if not line or not u"#define" in line:
            continue
        line = line.strip()

        match = re_define.search(line)
        if not match:
            continue

        key, value = match.groups()
        if key.startswith(u"_"):
            continue

        value = value.strip()

        # often, define is dependent on some other one
        # they are always in order, so it is expected to be in dict
        if value in defines.keys():
            value = defines.get(value)
            defines[key] = value
            continue

        if value in (u"true", u"false"):
            value = True if (value == u"true") else False
        elif value.startswith(u'"') and value.endswith(u'"'):
            value = value.replace(u'"', u'')
        elif value.startswith(u"0x"):
            value = int(value, 16)
        elif value.isdecimal():
            value = int(value, 10)

        if (key.endswith(u"_SUPPORT") or key.endswith(u"_ENABLED")) \
                and not isinstance(value, bool):
            value = bool(value)

        defines[key] = value

    return defines


env["ESPURNA_DEFINES"] = parse_config_header(env)
# for k, v in env["ESPURNA_DEFINES"].items():
#     print(k,v)

