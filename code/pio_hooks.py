#!/bin/python

import subprocess
import socket
from SCons.Script import DefaultEnvironment
env = DefaultEnvironment()

def before_build_spiffs(source, target, env):
    env.Execute("gulp")

env.AddPreAction(".pioenvs/%s/spiffs.bin" % env['PIOENV'], before_build_spiffs)
