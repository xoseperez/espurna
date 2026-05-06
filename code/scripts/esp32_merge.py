Import("env")
import os

def merge_bin(source, target, env):
    print("\n[POST-BUILD] Creating merged production binary...")
    
    # Пытаемся найти esptool
    platform = env.PioPlatform()
    esptool = os.path.join(platform.get_package_dir("tool-esptoolpy") or "", "esptool.py")
    if not os.path.exists(esptool):
        esptool = "esptool.py" # Надеемся на системный

    build_dir = env.subst("$BUILD_DIR")
    board = env.get("BOARD_MCU")
    flash_size = "4MB"
    output = os.path.join(build_dir, "espurna-esp32-relay-x2.bin")
    
    # Определяем пути к компонентам
    bootloader = os.path.join(build_dir, "bootloader.bin")
    if not os.path.exists(bootloader):
        # Если в папке сборки нет, берем из фреймворка
        framework_dir = platform.get_package_dir("framework-arduinoespressif32")
        bootloader = os.path.join(framework_dir, "tools", "sdk", "bin", "bootloader_dout_40m.bin")
    
    partitions = os.path.join(build_dir, "partitions.bin")
    boot_app0 = os.path.join(build_dir, "boot_app0.bin")
    if not os.path.exists(boot_app0):
        framework_dir = platform.get_package_dir("framework-arduinoespressif32")
        boot_app0 = os.path.join(framework_dir, "tools", "partitions", "boot_app0.bin")
        
    firmware = os.path.join(build_dir, "firmware.bin")

    # Проверяем наличие критических файлов
    for f in [bootloader, partitions, boot_app0, firmware]:
        if not os.path.exists(f):
            print(f"!!! Error: Missing component for merging: {f}")
            return

    cmd = (f"$PYTHONEXE {esptool} --chip {board} merge_bin -o {output} "
           f"--flash_mode dout --flash_size {flash_size} "
           f"0x1000 {bootloader} "
           f"0x8000 {partitions} "
           f"0xe000 {boot_app0} "
           f"0x10000 {firmware}")
           
    print(f"Running: {cmd}")
    env.Execute(cmd)
    print(f"[DONE] File ready: {output}\n")

env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bin)
