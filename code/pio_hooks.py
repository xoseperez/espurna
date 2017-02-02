#!/bin/python

import subprocess
import socket
from SCons.Script import DefaultEnvironment
env = DefaultEnvironment()

def before_build_spiffs(source, target, env):
    env.Execute("node node_modules/gulp/bin/gulp.js")

env.AddPreAction(".pioenvs/%s/spiffs.bin" % env['PIOENV'], before_build_spiffs)
