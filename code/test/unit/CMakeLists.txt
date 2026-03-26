# crude test builder based on
# - https://github.com/esp8266/Arduino/blob/3.0.2/tests/host/Makefile
# - https://github.com/mcspr/rpnlib/blob/0.24.1/examples/host/CMakeLists.txt
#
# we do require certain pre-requisites
# - https://github.com/esp8266/Arduino/ git tree for both Core's and Mock files
# - https://github.com/ThrowTheSwitch/Unity git tree (or tool-unity from platformio)
#
# after everything is installed
# $ cmake -B build
# $ cmake --build build --target test

cmake_minimum_required(VERSION 3.18)
project(host-test VERSION 1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11 CACHE STRING "Global C standard version (...does not yet work with 17 though)")
set(CMAKE_CXX_STANDARD 17 "Global C++ standard version")

# required for esp8266 host mocking
set(COMMON_FLAGS
    -Os
    -g
    -fno-common
    -funsigned-char
    -DPROGMEM_STRING_ATTR=
    -DCORE_MOCK
    -DHOST_MOCK=1
    -DLWIP_IPV6=0
    -Dstrnlen_P=strnlen
    -Dmemcmp_P=memcmp
    -Dstrncasecmp_P=strncasecmp
    -DUNITY_INCLUDE_DOUBLE
)

set(ESPURNA_PATH ${CMAKE_SOURCE_DIR}/../../../ CACHE PATH "ESPurna source code repo root")

# Make sure to generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# PIO does not really make it easy to install packages outside of the 'platform' context,
# so sharing these between a normal builder might not be an option. (...big TODO though)
# for right now, just fetch these as raw repos. plus, there's no need for any extra params

set(unity_version v2.6.0)

include(FetchContent)
FetchContent_Declare(
    unitygit
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity
    GIT_TAG ${unity_version}
    GIT_CONFIG core.autocrlf=false core.eol=lf
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/cache/unitygit-${unity_version}-src
    SUBBUILD_DIR ${CMAKE_SOURCE_DIR}/cache/unitygit-${unity_version}-subbuild
)

FetchContent_MakeAvailable(unitygit)
FetchContent_GetProperties(unitygit SOURCE_DIR)

target_compile_options(unity BEFORE PRIVATE
    -DUNITY_OUTPUT_COLOR
    -DUNITY_INCLUDE_DOUBLE
)

set(arduinojson_version v5.13.5)

FetchContent_Declare(
    ArduinoJson
    GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson
    GIT_TAG ${arduinojson_version}
    GIT_CONFIG core.autocrlf=false core.eol=lf
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/cache/arduinojson-${arduinojson_version}-src
    SOURCE_SUBDIR src
    SUBBUILD_DIR ${CMAKE_SOURCE_DIR}/cache/arduinojson-${arduinojson_version}-subbuild
)

FetchContent_MakeAvailable(ArduinoJson)
FetchContent_GetProperties(ArduinoJson SOURCE_DIR)

set(esp8266_version ccea72823ac50290bc05c67350d2be6626e65547)

FetchContent_Declare(
    esp8266git
    GIT_REPOSITORY https://github.com/esp8266/Arduino/
    GIT_TAG ${esp8266_version}
    GIT_CONFIG core.autocrlf=false core.eol=lf
    GIT_SUBMODULES libraries/LittleFS/lib/littlefs libraries/ESP8266SdFat
    GIT_SUBMODULES_RECURSE FALSE
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/cache/esp8266git-src
    SUBBUILD_DIR ${CMAKE_SOURCE_DIR}/cache/esp8266git-subbuild
)

FetchContent_MakeAvailable(esp8266git)
FetchContent_GetProperties(esp8266git SOURCE_DIR)

# mock'ed Arduino Core headers sometimes expect to be included with some pre-requisites, which we obviously don't have
add_library(common INTERFACE)
target_compile_options(common INTERFACE
    "SHELL:-include ${esp8266git_SOURCE_DIR}/tests/host/common/mock.h"
    "SHELL:-include ${esp8266git_SOURCE_DIR}/tests/host/common/c_types.h"
)

# try to hack esp8266 host test layer
# - we need to specify bunch of things that the original Makefile does
# - there are a lot of cross-dependencies, we need to include a lot of .cpp files here
add_library(esp8266 STATIC
    src/ArduinoMainOverride.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/Arduino.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/ArduinoMainUdp.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/WMath.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/MockUART.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/MockTools.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/MocklwIP.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/MockEsp.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/UdpContextSocket.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/user_interface.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/HostWiring.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/md5.c
    ${esp8266git_SOURCE_DIR}/tests/host/common/noniso.c
    ${esp8266git_SOURCE_DIR}/tests/host/common/flash_hal_mock.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/spiffs_mock.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/littlefs_mock.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/sdfs_mock.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/ArduinoMainUdp.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/ArduinoMainSpiffs.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/ArduinoMainLittlefs.cpp
    ${esp8266git_SOURCE_DIR}/tests/host/common/user_interface.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/debug.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/core_esp8266_noniso.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/stdlib_noniso.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/WString.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/HardwareSerial.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/Print.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/Schedule.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/time.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/Stream.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/StreamSend.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/FS.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/spiffs_api.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/spiffs/spiffs_cache.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/spiffs/spiffs_check.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/spiffs/spiffs_gc.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/spiffs/spiffs_hydrogen.cpp
    ${esp8266git_SOURCE_DIR}/cores/esp8266/spiffs/spiffs_nucleus.cpp
    ${esp8266git_SOURCE_DIR}/libraries/LittleFS/src/LittleFS.cpp
    ${esp8266git_SOURCE_DIR}/libraries/LittleFS/src/lfs.c
    ${esp8266git_SOURCE_DIR}/libraries/LittleFS/src/lfs_util.c
)
target_include_directories(esp8266 PUBLIC
    ${esp8266git_SOURCE_DIR}/tests/host/common/
    ${esp8266git_SOURCE_DIR}/tests/host
    ${esp8266git_SOURCE_DIR}/tools/sdk/lwip2/include
    ${esp8266git_SOURCE_DIR}/tools/sdk/include
    ${esp8266git_SOURCE_DIR}/cores/esp8266/
    ${esp8266git_SOURCE_DIR}/libraries/LittleFS/src/
    ${esp8266git_SOURCE_DIR}/libraries/SPI/
    ${esp8266git_SOURCE_DIR}/libraries/ESP8266SdFat/src
)
target_compile_options(esp8266 PUBLIC
    ${COMMON_FLAGS}
    -DF_CPU=80000000
    -Wl,--defsym,_FS_start=0x40300000
    -Wl,--defsym,_FS_end=0x411FA000
    -Wl,--defsym,_FS_page=0x100
    -Wl,--defsym,_FS_block=0x2000
    -Wl,--defsym,_EEPROM_start=0x411fb000
)
target_link_libraries(esp8266 PUBLIC common)

# our library source (maybe some day this will be a simple glob)
add_library(espurna STATIC
    ${ESPURNA_PATH}/code/espurna/fs_math.c
    ${ESPURNA_PATH}/code/espurna/settings_convert.cpp
    ${ESPURNA_PATH}/code/espurna/terminal_commands.cpp
    ${ESPURNA_PATH}/code/espurna/terminal_parsing.cpp
    ${ESPURNA_PATH}/code/espurna/bits.cpp
    ${ESPURNA_PATH}/code/espurna/datetime.cpp
    ${ESPURNA_PATH}/code/espurna/types.cpp
    ${ESPURNA_PATH}/code/espurna/utils.cpp
)
target_link_libraries(espurna PUBLIC esp8266)
target_link_options(espurna PUBLIC
    -fsanitize=address
)
target_include_directories(espurna PUBLIC
    ${ESPURNA_PATH}/code/
    ${CMAKE_SOURCE_DIR}/cache/arduinojson-${arduinojson_version}-src/src
)
target_compile_options(espurna PUBLIC
    ${COMMON_FLAGS}
    -fsanitize=address
)
target_compile_options(espurna PRIVATE
    -Wall
    -Wextra
)

# extra code linked w/ the unity for every test executable
add_library(unity-local STATIC
    ${CMAKE_SOURCE_DIR}/unity/unity_fixtures.c
    ${CMAKE_SOURCE_DIR}/unity/unity_extra.cpp
)
target_link_libraries(unity-local PUBLIC espurna unity)
target_include_directories(unity-local PUBLIC
    ${CMAKE_SOURCE_DIR}/unity
)
target_compile_options(unity-local PUBLIC
    ${COMMON_FLAGS}
)

# each case is built separately, we expect these to work like a normal executable
include(CTest)
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")

function(build_tests)
    foreach(ARG IN LISTS ARGN)
        file(GLOB ${ARG}_sources "src/${ARG}/*.h" "src/${ARG}/*.cpp")
        add_executable(test-${ARG} ${${ARG}_sources})
        target_link_libraries(test-${ARG} espurna unity unity-local)
        target_compile_options(test-${ARG} PRIVATE
            ${COMMON_FLAGS}
            -Wall
            -Wextra
        )
        set_target_properties(test-${ARG} PROPERTIES COMPILE_FLAGS -g)
        add_test(NAME ${ARG} COMMAND test-${ARG})
    endforeach()
endfunction()

build_tests(
    basic
    embedis
    filters
    led
    mqtt
    scheduler
    sensor
    settings
    terminal
    tuya
    types
    url
    utils
)
