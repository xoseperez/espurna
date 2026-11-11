# ESPurna version with the git revision (1.22.0-git<sha>), same as the other platforms get from pio_main.py.
# The WebUI release list (html/src/fwinstall.mjs) finds the release the board runs by that revision
from SCons.Script import Import

from espurna_utils import app_inject_version

Import("env", "projenv")

app_inject_version(globals()["projenv"])
