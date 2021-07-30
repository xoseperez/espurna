import os

def ldscripts_inject_libpath(env):
    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif8266")

    libpath_base = os.path.join("$PROJECT_DIR", "..", "dist", "ld")
    env.Prepend(LIBPATH=[libpath_base])
