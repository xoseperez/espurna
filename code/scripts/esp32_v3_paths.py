# Arduino-ESP32 core 3.x: the network stack is the framework library "Networking" (libraries/Network). AsyncTCP
# includes NetworkInterface.h but does not name it as a dependency, so the LDF leaves it off its include path.
# The path is added here and not as a -I build flag: on Windows the flag loses its backslashes.
# (ESPurna's own network.h is espurna_network.h: WiFiGeneric.h includes "Network.h", and on a case-insensitive
# file system that found espurna/network.h first.)

import os

Import("env")  # noqa: F821

framework = env.PioPlatform().get_package_dir("framework-arduinoespressif32")  # noqa: F821
if framework:
    # WiFi too: the framework library Matter includes WiFi.h without naming it as a dependency.
    # FS: LittleFS (NSPanel) includes FS.h, and with chain+ the LDF leaves FS off its include path
    env.Prepend(CPPPATH=[  # noqa: F821
        os.path.join(framework, "libraries", "Network", "src"),
        os.path.join(framework, "libraries", "WiFi", "src"),
        os.path.join(framework, "libraries", "FS", "src"),
    ])
