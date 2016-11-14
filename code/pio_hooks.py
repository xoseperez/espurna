#!/bin/python

import subprocess
import socket
from SCons.Script import DefaultEnvironment
env = DefaultEnvironment()

def is_valid_ip(ip):
    try:
        socket.inet_aton(ip)
        return True
    except socket.error:
        return False

def before_build_spiffs(source, target, env):
    env.Execute("gulp buildfs")

def before_upload(source, target, env):
    upload_port = env.get('UPLOAD_PORT', False)
    if upload_port and upload_port[0] == '/':
        cmd = ["mosquitto_sub", "-t", upload_port, "-h", "192.168.1.10", "-N", "-C", "1"]
        ip = subprocess.check_output(cmd)
        if is_valid_ip(ip):
            env['UPLOAD_PORT'] = '"' + ip + '"'

#env.AddPreAction("uploadfs", before_upload)
#env.AddPreAction("upload", before_upload)
env.AddPreAction(".pioenvs/%s/spiffs.bin" % env['PIOENV'], before_build_spiffs)
