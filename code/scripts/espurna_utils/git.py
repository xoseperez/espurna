import os
import subprocess

def git(*args):
    cmd = ["git"]
    cmd.extend(args)
    proc = subprocess.Popen(
        cmd, stderr=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True
    )
    return proc.stdout.readlines()[0].strip()

def app_inject_revision(env):
    revision = ""
    try:
        revision = "\\\"{}\\\"".format(git("rev-parse", "--short=8", "HEAD"))
    except: # pylint: disable=broad-except
        pass

    # Note: code expects this as undefined when empty
    if revision:
        env.Append(CPPDEFINES=[
            ("APP_REVISION", revision)
        ])
