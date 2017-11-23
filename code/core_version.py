#!/bin/python

import json
import commands
import subprocess
import os
import sys

def core_version(env):

    # Get the core folder
    fwdir = env["FRAMEWORK_ARDUINOESP8266_DIR"]

    # Get the core version
    with open(fwdir + '/package.json') as data_file:
        data = json.load(data_file)
    core_version = data["version"].upper().replace(".", "_").replace("-", "_")
    print "CORE VERSION: %s" % core_version

    # Get git version
    pr = subprocess.Popen(
        "git --git-dir .git rev-parse --short=8 HEAD 2>/dev/null || echo ffffffff",
        cwd = fwdir,
        shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE )
    (out, error) = pr.communicate()
    git_version = str(out).replace('\n', "")
    print "GIT VERSION: %s" % git_version

    #env["BUILD_FLAGS"][0] += str(" -DARDUINO_ESP8266_RELEASE=" + core_version)
    #env["BUILD_FLAGS"][0] += str(" -DARDUINO_ESP8266_RELEASE_" + core_version)
    #env["BUILD_FLAGS"][0] += str(" -DARDUINO_ESP8266_GIT_VER=" + git_version)

    with open('espurna/config/core_version.h', 'w') as the_file:
        the_file.write('#define ARDUINO_ESP8266_RELEASE "%s"\n' % core_version)
        the_file.write('#define ARDUINO_ESP8266_RELEASE_%s\n' % core_version)
        the_file.write('#define ARDUINO_ESP8266_GIT_VER "%s"\n' % git_version)

    #env.Append(
    #    CFLAGS = [
    #        str("-DARDUINO_ESP8266_RELEASE=" + core_version),
    #        str("-DARDUINO_ESP8266_RELEASE_" + core_version),
    #        str("-DARDUINO_ESP8266_GIT_VER=" + git_version)
    #    ]
    #)

    #print " -DARDUINO_ESP8266_RELEASE=" + core_version +
    #    " -DARDUINO_ESP8266_RELEASE_" + core_version +
    #    " -DARDUINO_ESP8266_GIT_VER=" + git_version

Import('env')
core_version(env)
