# adapted boards.txt.py from esp8266/Arduino
# - single board definition, ldscripts
# - portable boards.local.txt for 2.3.0 and up

import os
import argparse
import sys
import collections

class VersionedSubstitution(collections.MutableMapping):

    def __init__(self, substitutions, targets, *args, **kwargs):
        self._targets = targets
        self._version = None
        self._store = substitutions.copy()
        self.update(dict(*args, **kwargs))

    def set_version(self, version):
        self._version = version

    def __getitem__(self, key):
        if self._version in self._targets:
            return self._store[key]
        return key

    def __setitem__(self, key, value):
        self._store[key] = value

    def __delitem__(self, key):
        del self._store[key]

    def __iter__(self):
        return iter(self._store)

    def __len__(self):
        return len(self._store)


BOARDS_LOCAL = {
    'global': collections.OrderedDict([
        ( 'menu.variant', 'ESPurna board variant' ),
        ( 'menu.float_support', 'scanf and printf float support' )
        ]),

    'defaults': collections.OrderedDict([
        ( '.name', 'ESPurna Board' ),
        ( '.build.board', 'ESP8266_ESPURNA' ),
        ( '.upload.speed' ,'115200' ),
        ( '.upload.tool', 'esptool' ),
        ( '.upload.maximum_data_size', '81920' ),
        ( '.upload.wait_for_upload_port', 'true' ),
        ( '.upload.erase_cmd', ''),
        ( '.serial.disableDTR', 'true' ),
        ( '.serial.disableRTS', 'true' ),
        ( '.build.mcu', 'esp8266' ),
        ( '.build.core', 'esp8266' ),
        ( '.build.variant', 'generic' ),
        ( '.build.spiffs_pagesize', '256' ),
        ( '.build.debug_port', '' ),
        ( '.build.debug_level', '' ),
        ( '.build.flash_mode', 'dout' ),
        ( '.build.flash_flags', '-DFLASHMODE_DOUT' ),
        ]),

    'variant': collections.OrderedDict([
        ( '.menu.variant.generic', 'Generic' ),
        ( '.menu.variant.generic.build.variant', 'generic' ),
        ( '.menu.variant.esp8285', 'ESP8285 (1M)' ),
        ( '.menu.variant.esp8285.build.variant', 'esp8285' )
        ]),

    #######################

    'cpu_freq': collections.OrderedDict([
        ( '.menu.{xtal}.80', '80 MHz' ),
        ( '.menu.{xtal}.80.build.f_cpu', '80000000L' ),
        ( '.menu.{xtal}.160', '160 MHz' ),
        ( '.menu.{xtal}.160.build.f_cpu', '160000000L' ),
        ]),

    'vtables': collections.OrderedDict([
        ( '.menu.{vt}.flash', 'Flash'),
        ( '.menu.{vt}.flash.build.vtable_flags', '-DVTABLES_IN_FLASH'),
        ( '.menu.{vt}.heap', 'Heap'),
        ( '.menu.{vt}.heap.build.vtable_flags', '-DVTABLES_IN_DRAM'),
        ( '.menu.{vt}.iram', 'IRAM'),
        ( '.menu.{vt}.iram.build.vtable_flags', '-DVTABLES_IN_IRAM'),
        ]),

    'exceptions': collections.OrderedDict([
        ( '.menu.exception.disabled', 'Disabled' ),
        ( '.menu.exception.disabled.build.exception_flags', '-fno-exceptions' ),
        ( '.menu.exception.disabled.build.stdcpp_lib', '-lstdc++' ),
        ( '.menu.exception.enabled', 'Enabled' ),
        ( '.menu.exception.enabled.build.exception_flags', '-fexceptions' ),
        ( '.menu.exception.enabled.build.stdcpp_lib', '-lstdc++-exc' ),
        ]),

    'crystal_freq': collections.OrderedDict([
        ( '.menu.CrystalFreq.26', '26 MHz' ),
        ( '.menu.CrystalFreq.40', '40 MHz' ),
        ( '.menu.CrystalFreq.40.build.extra_flags', '-DF_CRYSTAL=40000000 -DESP8266' ),
        ]),

    'flash_freq': collections.OrderedDict([
        ( '.menu.FlashFreq.40', '40MHz' ),
        ( '.menu.FlashFreq.40.build.flash_freq', '40' ),
        ( '.menu.FlashFreq.80', '80MHz' ),
        ( '.menu.FlashFreq.80.build.flash_freq', '80' ),
        ]),

    ####################### menu.resetmethod

    'reset_method': collections.OrderedDict([
        ( '.menu.ResetMethod.ck', 'ck' ),
        ( '.menu.ResetMethod.ck.upload.resetmethod', 'ck' ),
        ( '.menu.ResetMethod.nodemcu', 'nodemcu' ),
        ( '.menu.ResetMethod.nodemcu.upload.resetmethod', 'nodemcu' ),
        ( '.menu.ResetMethod.none', 'none' ),
        ( '.menu.ResetMethod.none.upload.resetmethod', 'none' ),
        ( '.menu.ResetMethod.dtrset', 'dtrset' ),
        ( '.menu.ResetMethod.dtrset.upload.resetmethod', 'dtrset' ),
        ]),

    ####################### menu.FlashMode

    'flash_mode': collections.OrderedDict([
        ( '.menu.FlashMode.dout', 'DOUT (compatible)' ),
        ( '.menu.FlashMode.dout.build.flash_mode', 'dout' ),
        ( '.menu.FlashMode.dout.build.flash_flags', '-DFLASHMODE_DOUT' ),
        ( '.menu.FlashMode.dio', 'DIO' ),
        ( '.menu.FlashMode.dio.build.flash_mode', 'dio' ),
        ( '.menu.FlashMode.dio.build.flash_flags', '-DFLASHMODE_DIO' ),
        ( '.menu.FlashMode.qout', 'QOUT' ),
        ( '.menu.FlashMode.qout.build.flash_mode', 'qout' ),
        ( '.menu.FlashMode.qout.build.flash_flags', '-DFLASHMODE_QOUT' ),
        ( '.menu.FlashMode.qio', 'QIO (fast)' ),
        ( '.menu.FlashMode.qio.build.flash_mode', 'qio' ),
        ( '.menu.FlashMode.qio.build.flash_flags', '-DFLASHMODE_QIO' ),
        ]),

    ####################### menu.FlashSize

    'flash_size': collections.OrderedDict([
        ( '.menu.{eesz}.1M', '1M1S (no SPIFFS)' ),
        ( '.menu.{eesz}.1M.build.flash_size', '1M' ),
        ( '.menu.{eesz}.1M.build.flash_size_bytes', '0x100000' ),
        ( '.menu.{eesz}.1M.build.flash_ld', 'eagle.flash.1m0m1s.ld' ),
        ( '.menu.{eesz}.1M.build.spiffs_pagesize', '256' ),
        ( '.menu.{eesz}.1M.upload.maximum_size', '1023984' ),
        ( '.menu.{eesz}.1M.build.rfcal_addr', '0xFC000' ),

        ( '.menu.{eesz}.2M', '2M4S (1M SPIFFS)' ),
        ( '.menu.{eesz}.2M.build.flash_size', '2M' ),
        ( '.menu.{eesz}.2M.build.flash_size_bytes', '0x200000' ),
        ( '.menu.{eesz}.2M.build.flash_ld', 'eagle.flash.2m1m4s.ld' ),
        ( '.menu.{eesz}.2M.build.spiffs_pagesize', '256' ),
        ( '.menu.{eesz}.2M.upload.maximum_size', '1044464' ),
        ( '.menu.{eesz}.2M.build.rfcal_addr', '0x1FC000' ),

        ( '.menu.{eesz}.4M1M', '4M4S (1M SPIFFS)' ),
        ( '.menu.{eesz}.4M1M.build.flash_size', '4M' ),
        ( '.menu.{eesz}.4M1M.build.flash_size_bytes', '0x400000' ),
        ( '.menu.{eesz}.4M1M.build.flash_ld', 'eagle.flash.4m1m4s.ld' ),
        ( '.menu.{eesz}.4M1M.build.spiffs_pagesize', '256' ),
        ( '.menu.{eesz}.4M1M.upload.maximum_size', '1044464' ),
        ( '.menu.{eesz}.4M1M.build.rfcal_addr', '0x3FC000' ),

        ( '.menu.{eesz}.4M3M', '4M4S (3M SPIFFS)' ),
        ( '.menu.{eesz}.4M3M.build.flash_size', '4M' ),
        ( '.menu.{eesz}.4M3M.build.flash_size_bytes', '0x400000' ),
        ( '.menu.{eesz}.4M3M.build.flash_ld', 'eagle.flash.4m3m4s.ld' ),
        ( '.menu.{eesz}.4M3M.build.spiffs_pagesize', '256' ),
        ( '.menu.{eesz}.4M3M.upload.maximum_size', '1044464' ),
        ( '.menu.{eesz}.4M3M.build.rfcal_addr', '0x3FC000' ),
        ]),

    ####################### lwip

    'lwip2_latest': collections.OrderedDict([
        ( '.menu.ip.lm2f', 'v2 Lower Memory' ),
        ( '.menu.ip.lm2f.build.lwip_include', 'lwip2/include' ),
        ( '.menu.ip.lm2f.build.lwip_lib', '-llwip2-536-feat' ),
        ( '.menu.ip.lm2f.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=536 -DLWIP_FEATURES=1 -DLWIP_IPV6=0' ),
        ( '.menu.ip.hb2f', 'v2 Higher Bandwidth' ),
        ( '.menu.ip.hb2f.build.lwip_include', 'lwip2/include' ),
        ( '.menu.ip.hb2f.build.lwip_lib', '-llwip2-1460-feat' ),
        ( '.menu.ip.hb2f.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=1460 -DLWIP_FEATURES=1 -DLWIP_IPV6=0' ),
        ( '.menu.ip.lm2n', 'v2 Lower Memory (no features)' ),
        ( '.menu.ip.lm2n.build.lwip_include', 'lwip2/include' ),
        ( '.menu.ip.lm2n.build.lwip_lib', '-llwip2-536' ),
        ( '.menu.ip.lm2n.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=536 -DLWIP_FEATURES=0 -DLWIP_IPV6=0' ),
        ( '.menu.ip.hb2n', 'v2 Higher Bandwidth (no features)' ),
        ( '.menu.ip.hb2n.build.lwip_include', 'lwip2/include' ),
        ( '.menu.ip.hb2n.build.lwip_lib', '-llwip2-1460' ),
        ( '.menu.ip.hb2n.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=1460 -DLWIP_FEATURES=0 -DLWIP_IPV6=0' ),
        ( '.menu.ip.lm6f', 'v2 IPv6 Lower Memory' ),
        ( '.menu.ip.lm6f.build.lwip_include', 'lwip2/include' ),
        ( '.menu.ip.lm6f.build.lwip_lib', '-llwip6-536-feat' ),
        ( '.menu.ip.lm6f.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=536 -DLWIP_FEATURES=1 -DLWIP_IPV6=1' ),
        ( '.menu.ip.hb6f', 'v2 IPv6 Higher Bandwidth' ),
        ( '.menu.ip.hb6f.build.lwip_include', 'lwip2/include' ),
        ( '.menu.ip.hb6f.build.lwip_lib', '-llwip6-1460-feat' ),
        ( '.menu.ip.hb6f.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=1460 -DLWIP_FEATURES=1 -DLWIP_IPV6=1' ),
        ]),

    'lwip2_242': collections.OrderedDict([
        ( '.menu.LwIPVariant.v2mss536', 'v2 Lower Memory' ),
        ( '.menu.LwIPVariant.v2mss536.build.lwip_include', 'lwip2/include' ),
        ( '.menu.LwIPVariant.v2mss536.build.lwip_lib', '-llwip2' ),
        ( '.menu.LwIPVariant.v2mss536.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=536' ),
        ( '.menu.LwIPVariant.v2mss1460', 'v2 Higher Bandwidth' ),
        ( '.menu.LwIPVariant.v2mss1460.build.lwip_include', 'lwip2/include' ),
        ( '.menu.LwIPVariant.v2mss1460.build.lwip_lib', '-llwip2_1460' ),
        ( '.menu.LwIPVariant.v2mss1460.build.lwip_flags', '-DLWIP_OPEN_SRC -DTCP_MSS=1460' ),
        ]),

    'lwip': collections.OrderedDict([
        ( '.menu.ip.hb1', 'v1.4 Higher Bandwidth' ),
        ( '.menu.ip.hb1.build.lwip_lib', '-llwip_gcc' ),
        ( '.menu.ip.hb1.build.lwip_flags', '-DLWIP_OPEN_SRC' ),
        #( '.menu.ip.Espressif', 'v1.4 Espressif (xcc)' ),
        #( '.menu.ip.Espressif.build.lwip_lib', '-llwip' ),
        #( '.menu.ip.Espressif.build.lwip_flags', '-DLWIP_MAYBE_XCC' ),
        ( '.menu.ip.src', 'v1.4 Compile from source' ),
        ( '.menu.ip.src.build.lwip_lib', '-llwip_src' ),
        ( '.menu.ip.src.build.lwip_flags', '-DLWIP_OPEN_SRC' ),
        ( '.menu.ip.src.recipe.hooks.sketch.prebuild.1.pattern', 'make -C "{runtime.platform.path}/tools/sdk/lwip/src" install TOOLS_PATH="{runtime.tools.xtensa-lx106-elf-gcc.path}/bin/xtensa-lx106-elf-"' ),
        ]),

    ####################### serial

    'upload': collections.OrderedDict([
        ( '.menu.{baud}.115200', '115200' ),
        ( '.menu.{baud}.115200.upload.speed', '115200' ),
        ( '.menu.{baud}.9600', '9600' ),
        ( '.menu.{baud}.9600.upload.speed', '9600' ),
        ( '.menu.{baud}.57600', '57600' ),
        ( '.menu.{baud}.57600.upload.speed', '57600' ),
        ( '.menu.{baud}.256000.windows', '256000' ),
        ( '.menu.{baud}.256000.upload.speed', '256000' ),
        ( '.menu.{baud}.230400.linux', '230400' ),
        ( '.menu.{baud}.230400.macosx', '230400' ),
        ( '.menu.{baud}.230400.upload.speed', '230400' ),
        ( '.menu.{baud}.460800.linux', '460800' ),
        ( '.menu.{baud}.460800.macosx', '460800' ),
        ( '.menu.{baud}.460800.upload.speed', '460800' ),
        ( '.menu.{baud}.512000.windows', '512000' ),
        ( '.menu.{baud}.512000.upload.speed', '512000' ),
        ( '.menu.{baud}.921600', '921600' ),
        ( '.menu.{baud}.921600.upload.speed', '921600' ),
        ]),

    ####################### flash erase

    'flash_erase': collections.OrderedDict([
        ( '.menu.{wipe}.none', 'Only Sketch' ),
        ( '.menu.{wipe}.none.upload.erase_cmd', '' ),
        ( '.menu.{wipe}.sdk', 'Sketch + WiFi Settings' ),
        ( '.menu.{wipe}.sdk.upload.erase_cmd', '-ca "{build.rfcal_addr}" -cz 0x4000' ),
        ( '.menu.{wipe}.all', 'All Flash Contents' ),
        ( '.menu.{wipe}.all.upload.erase_cmd', '-ca 0x0 -cz "{build.flash_size_bytes}"' ),
        ]),

    }


BOARD = "espurna"


CORE_VERSIONS = ["2.3.0", "2.4.2", "2.5.0"]


MENUS = {}
MENUS["2.3.0"] = [
    "defaults", "cpu_freq",
    "flash_freq", "flash_mode", "flash_size", "flash_erase",
    "reset_method", "upload"
]


MENUS["2.4.2"] = list(MENUS["2.3.0"])
MENUS["2.4.2"].extend(["crystal_freq", "lwip", "lwip2_242", "vtables"])


MENUS["2.5.0"] = list(MENUS["2.3.0"])
MENUS["2.5.0"].extend(["variant", "crystal_freq", "lwip", "lwip2_latest", "vtables", "exceptions"])


EXTRA_FLAGS = [
    ( '.compiler.cpp.extra_flags', '-DNO_GLOBAL_EEPROM -DMQTT_MAX_PACKET_SIZE=400' ),
]


SUBSTITUTIONS = dict(
    name=BOARD,
    xtal="CpuFrequency",
    eesz="FlashSize",
    wipe="FlashErase",
    baud="UploadSpeed",
    vt="VTable"
)

def generate(versions, directory, sub=VersionedSubstitution(SUBSTITUTIONS, ["2.3.0", "2.4.2"])):

    for version in versions:

        sub.set_version(version)

        result = ["#version={}\n\n".format(version)]
        result.extend("{}={}\n".format(k,v) for k,v in BOARDS_LOCAL["global"].items())
        result.append("\n")

        #print("{} unused:".format(version, set(BOARDS_LOCAL.keys()) - set(MENUS[version])))
        #continue

        for menu in MENUS[version]:
            section = []
            for k, v in BOARDS_LOCAL[menu].items():
                k = k.format_map(sub)
                section.append(BOARD + "=".join([k,v]))
            result.append("\n".join(section))
            result.append("\n\n")

        if EXTRA_FLAGS:
            result.extend(("{}{}={}".format(BOARD, k,v)) for k, v in EXTRA_FLAGS)

        f_path = os.path.join(directory, version, "boards.local.txt")

        with open(f_path, "w") as f:
            for part in result:
                f.write(part)
            f.write("\n")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--directory", default="arduino_ide")
    parser.add_argument("--versions", action="store_true", default=False, help="supported versions")
    parser.add_argument("--generate", nargs="*", action="append", default=CORE_VERSIONS)

    args = parser.parse_args()

    if args.versions:
        print("Supported versions:")
        for version in CORE_VERSIONS:
            print(version)
        sys.exit(1)

    if args.generate:
        generate(args.generate, args.directory)
