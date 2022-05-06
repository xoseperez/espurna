#!/usr/bin/env python3
# pylint: disable=C0330
# coding=utf-8
# -------------------------------------------------------------------------------
# ESPurna OTA manager
# xose.perez@gmail.com
#
# Requires PlatformIO Core
# -------------------------------------------------------------------------------

import argparse
import os
import re
import shutil
import socket
import subprocess
import functools
import sys
import time

import zeroconf

# -------------------------------------------------------------------------------

__version__ = (0, 4, 2)

DESCRIPTION = "ESPurna OTA Manager v{}".format(".".join(str(x) for x in __version__))
DISCOVERY_TIMEOUT = 10

# -------------------------------------------------------------------------------


class Printer:

    OUTPUT_FORMAT = "{:>3}  {:<14}  {:<15}  {:<17}  {:<12}  {:<12}  {:<20}  {:<25}  {:<8}  {:<8}  {:<10}"

    def header(self):
        print(
            self.OUTPUT_FORMAT.format(
                "#",
                "HOSTNAME",
                "IP",
                "MAC",
                "APP",
                "VERSION",
                "BUILD_DATE",
                "DEVICE",
                "MEM_SIZE",
                "SDK_SIZE",
                "FREE_SPACE",
            )
        )
        print("-" * 164)

    def device(self, index, device):
        print(
            self.OUTPUT_FORMAT.format(
                index,
                device.get("hostname", ""),
                device.get("ip", ""),
                device.get("mac", ""),
                device.get("app_name", ""),
                device.get("app_version", ""),
                device.get("build_date", ""),
                device.get("target_board", ""),
                device.get("mem_size", 0),
                device.get("sdk_size", 0),
                device.get("free_space", 0),
            )
        )

    def devices(self, devices):
        """
        Shows the list of discovered devices
        """
        for index, device in enumerate(devices, 1):
            self.device(index, device)

        print()


# Based on:
# https://github.com/balloob/pychromecast/blob/master/pychromecast/discovery.py
class Listener:
    def __init__(self, print_when_discovered=True):
        self.devices = []
        self.printer = Printer()
        self.print_when_discovered = print_when_discovered

    @property
    def count(self):
        return len(self.devices)

    def add_service(self, browser, service_type, name):
        """
        Callback that adds discovered devices to "devices" list
        """

        info = None
        tries = 0
        while info is None and tries < 4:
            try:
                info = browser.get_service_info(service_type, name)
            except IOError:
                break
            tries += 1

        if not info:
            print(
                "! error getting service info {} {}".format(service_type, name),
                file=sys.stderr,
            )
            return

        hostname = info.server.split(".")[0]
        addresses = info.parsed_addresses()

        device = {
            "hostname": hostname.upper(),
            "ip": addresses[0] if addresses else info.host,
            "mac": "",
            "app_name": "",
            "app_version": "",
            "build_date": "",
            "target_board": "",
            "mem_size": 0,
            "sdk_size": 0,
            "free_space": 0,
        }

        for key, item in info.properties.items():
            device[key.decode("UTF-8")] = item.decode("UTF-8")

        # rename fields (needed for sorting by name)
        device["app"] = device["app_name"]
        device["device"] = device["target_board"]
        device["version"] = device["app_version"]

        self.devices.append(device)

        if self.print_when_discovered:
            self.printer.device(self.count, device)

    def print_devices(self, devices=None):
        if not devices:
            devices = self.devices
        self.printer.devices(devices)


def get_boards():
    """
    Grabs board types from hardware.h file
    """
    boards = []
    for line in open("espurna/config/hardware.h"):
        match = re.search(r"^#elif defined\((\w*)\)", line)
        if match:
            boards.append(match.group(1))
    return sorted(boards)


def get_device_size(device):
    if device.get("mem_size", 0) == device.get("sdk_size", 0):
        return int(device.get("mem_size", 0)) // 1024
    return 0


def get_empty_board():
    """
    Returns the empty structure of a board to flash
    """
    board = {"board": "", "ip": "", "size": 0, "auth": "", "flags": ""}
    return board


def get_board_by_index(devices, index):
    """
    Returns the required data to flash a given board
    """
    board = {}
    if 1 <= index <= len(devices):
        device = devices[index - 1]
        board["hostname"] = device.get("hostname")
        board["board"] = device.get("target_board", "")
        board["ip"] = device.get("ip", "")
        board["size"] = get_device_size(device)
    return board


def get_board_by_mac(devices, mac):
    """
    Returns the required data to flash a given board
    """
    for device in devices:
        if device.get("mac", "").lower() == mac:
            board = {}
            board["hostname"] = device.get("hostname")
            board["board"] = device.get("device")
            board["ip"] = device.get("ip")
            board["size"] = get_device_size(device)
            if not board["board"] or not board["ip"] or board["size"] == 0:
                return None
            return board
    return None


def get_board_by_hostname(devices, hostname):
    """
    Returns the required data to flash a given board
    """
    hostname = hostname.lower()
    for device in devices:
        if device.get("hostname", "").lower() == hostname:
            board = {}
            board["hostname"] = device.get("hostname")
            board["board"] = device.get("target_board")
            board["ip"] = device.get("ip")
            board["size"] = get_device_size(device)
            if not board["board"] or not board["ip"] or board["size"] == 0:
                return None
            return board
    return None


def input_board(devices):
    """
    Grabs info from the user about what device to flash
    """

    # Choose the board
    try:
        index = int(
            input("Choose the board you want to flash (empty if none of these): ")
        )
    except ValueError:
        index = 0
    if index < 0 or len(devices) < index:
        print("Board number must be between 1 and {}\n".format(str(len(devices))))
        return None

    board = get_board_by_index(devices, index)

    # Choose board type if none before
    if not board.get("board"):

        print()
        count = 1
        boards = get_boards()
        for name in boards:
            print("{:3d}\t{}".format(count, name))
            count += 1
        print()
        try:
            index = int(input("Choose the board type you want to flash: "))
        except ValueError:
            index = 0
        if index < 1 or len(boards) < index:
            print("Board number must be between 1 and {}\n".format(str(len(boards))))
            return None
        board["board"] = boards[index - 1]

    # Choose board size of none before
    if not board.get("size"):
        try:
            board["size"] = int(
                input("Board memory size (1 for 1M, 2 for 2M, 4 for 4M): ")
            )
        except ValueError:
            print("Wrong memory size")
            return None

    # Choose IP of none before
    if not board.get("ip"):
        board["ip"] = (
            input("IP of the device to flash (empty for 192.168.4.1): ")
            or "192.168.4.1"
        )

    return board


def boardname(board):
    return board.get("hostname", board["ip"])


def store(device, env):
    source = ".pio/build/{}/firmware.elf".format(env)
    destination = ".pio/build/elfs/{}.elf".format(boardname(device).lower())

    dst_dir = os.path.dirname(destination)
    if not os.path.exists(dst_dir):
        os.mkdir(dst_dir)

    shutil.move(source, destination)


def run(device, env):
    print("Building and flashing image over-the-air...")
    environ = os.environ.copy()
    environ["ESPURNA_IP"] = device["ip"]
    environ["ESPURNA_BOARD"] = device["board"]
    environ["ESPURNA_AUTH"] = device["auth"]
    environ["ESPURNA_FLAGS"] = device["flags"]

    command = ("platformio", "run", "--silent", "--environment", env, "-t", "upload")
    subprocess.check_call(command, env=environ)

    store(device, env)


# -------------------------------------------------------------------------------


def parse_commandline_args():
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument(
        "-c",
        "--core",
        help="Use ESPurna core configuration",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-f", "--flash", help="Flash device", action="store_true", default=False
    )
    parser.add_argument(
        "-a",
        "--arduino-core",
        help="Arduino ESP8266 Core version",
        default="current",
        choices=["current", "latest", "git"],
    )
    parser.add_argument("-o", "--flags", help="extra flags", default="")
    parser.add_argument("-p", "--password", help="auth password", default="")
    parser.add_argument("-s", "--sort", help="sort devices list by field", default="")
    parser.add_argument(
        "-y",
        "--yes",
        help="do not ask for confirmation",
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-t",
        "--timeout",
        type=int,
        help="how long to wait for mDNS discovery",
        default=DISCOVERY_TIMEOUT,
    )
    parser.add_argument("hostnames", nargs="*", help="Hostnames to update")
    return parser.parse_args()


def discover_devices(args):
    # Look for services and try to immediatly print the device when it is discovered
    # (unless --sort <field> is specified, then we will wait until discovery finishes
    listener = Listener(print_when_discovered=not args.sort)

    try:
        browser = zeroconf.ServiceBrowser(
            zeroconf.Zeroconf(), "_arduino._tcp.local.", listener
        )
    except (
        zeroconf.BadTypeInNameException,
        NotImplementedError,
        OSError,
        socket.error,
        zeroconf.NonUniqueNameException,
    ) as exc:
        print("! error when creating service discovery browser: {}".format(exc))
        sys.exit(1)

    try:
        time.sleep(args.timeout)
    except KeyboardInterrupt:
        sys.exit(1)

    try:
        browser.zc.close()
    except KeyboardInterrupt:
        sys.exit(1)

    if not listener.devices:
        print("Nothing found!\n")
        sys.exit(1)

    devices = listener.devices

    # Sort list by specified field name and# show devices overview
    if args.sort:
        field = args.sort.lower()
        if field not in devices[0]:
            print("Unknown field '{}'\n".format(field))
            sys.exit(1)
        devices.sort(key=lambda dev: dev.get(field, ""))

        listener.print_devices(devices)

    return devices


@functools.lru_cache(maxsize=None)
def get_platformio_env(arduino_core, size):
    prefix = "esp8266"
    if not size in [1, 2, 4]:
        raise ValueError(
            "Board memory size can only be one of: 1 for 1M, 2 for 2M, 4 for 4M"
        )
    core = ""
    if arduino_core != "current":
        core = "-{}".format(arduino_core)
    return "{prefix}-{size:d}m{core}-base".format(prefix=prefix, core=core, size=size)


def main(args):

    print()
    print(DESCRIPTION)
    print()

    devices = discover_devices(args)

    # Flash device only when --flash arg is provided
    if args.flash:

        # Board(s) to flash
        queue = []

        # Check if hostnames
        for hostname in args.hostnames:
            board = get_board_by_hostname(devices, hostname)
            if board:
                board["auth"] = args.password
                board["flags"] = args.flags
                queue.append(board)

        # If no boards ask the user
        if not len(queue):
            board = input_board(devices)
            if board:
                board["auth"] = args.password or input(
                    "Authorization key of the device to flash: "
                )
                board["flags"] = args.flags or input("Extra flags for the build: ")
                queue.append(board)

        # If still no boards quit
        if not len(queue):
            sys.exit(0)

        queue.sort(key=lambda dev: dev.get("board", ""))

        # Flash each board
        for board in queue:

            # Flash core version?
            if args.core:
                board["flags"] = "-DESPURNA_CORE " + board["flags"]

            env = get_platformio_env(args.arduino_core, board["size"])

            # Summary
            print()
            print("HOST    = {}".format(boardname(board)))
            print("IP      = {}".format(board["ip"]))
            print("BOARD   = {}".format(board["board"]))
            print("AUTH    = {}".format(board["auth"]))
            print("FLAGS   = {}".format(board["flags"]))
            print("ENV     = {}".format(env))

            response = True
            if not args.yes:
                response = input("\nAre these values right [y/N]: ") == "y"
            if response:
                print()
                run(board, env)


if __name__ == "__main__":
    main(parse_commandline_args())
