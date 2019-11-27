#!/usr/bin/env python3
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
import sys
from threading import Event

import zeroconf

# -------------------------------------------------------------------------------

__version__ = (0, 4)

DESCRIPTION = "ESPurna OTA Manager v{}".format(".".join(str(x) for x in __version__))
DISCOVERY_TIMEOUT = 5

# -------------------------------------------------------------------------------

# Based on:
# https://github.com/balloob/pychromecast/blob/master/pychromecast/discovery.py
class Listener:
    def __init__(self, add_service_callback=None, remove_service_callback=None):
        self.devices = []
        self.add_service_callback = add_service_callback
        self.remove_service_callback = remove_service_callback

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
        device = {
            "hostname": hostname.upper(),
            "ip": socket.inet_ntoa(info.address),
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

        if self.add_service_callback:
            self.add_service_callback(name)

    def remove_service(self, browser, service_type, name):
        """ Remove a service from the collection. """
        print("remove_service %s, %s", service_type, name)

        if self.remove_service_callback:
            self.remove_service_callback(name, None)


def print_devices(devices):
    """
    Shows the list of discovered devices
    """
    output_format = "{:>3}  {:<14}  {:<15}  {:<17}  {:<12}  {:<12}  {:<20}  {:<25}  {:<8}  {:<8}  {:<10}"
    print(
        output_format.format(
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

    index = 0
    for device in devices:
        index += 1
        print(
            output_format.format(
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

    print()


def get_boards():
    """
    Grabs board types fro hardware.h file
    """
    boards = []
    for line in open("espurna/config/hardware.h"):
        match = re.search(r"defined\((\w*)\)", line)
        if match:
            boards.append(match.group(1))
    return sorted(boards)


def get_device_size(device):
    if device.get("mem_size", 0) == device.get("sdk_size", 0):
        return int(device.get("mem_size", 0)) / 1024
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
    if len(board.get("board", "")) == 0:

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
    if board.get("size", 0) == 0:
        try:
            board["size"] = int(
                input("Board memory size (1 for 1M, 2 for 2M, 4 for 4M): ")
            )
        except ValueError:
            print("Wrong memory size")
            return None

    # Choose IP of none before
    if len(board.get("ip", "")) == 0:
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
    environ["ESPURNA_PIO_SHARED_LIBRARIES"] = "y"

    command = ("platformio", "run", "--silent", "--environment", env, "-t", "upload")
    subprocess.check_call(command, env=environ)

    store(device, env)


# -------------------------------------------------------------------------------


def parse_commandline_args():
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument(
        "-c", "--core", help="flash ESPurna core", default=0, action="store_true", default=False
    )
    parser.add_argument("-f", "--flash", help="flash device", action="store_true", default=False)
    parser.add_argument("-o", "--flags", help="extra flags", default="")
    parser.add_argument("-p", "--password", help="auth password", default="")
    parser.add_argument(
        "-s", "--sort", help="sort devices list by field", default="hostname"
    )
    parser.add_argument(
        "-y", "--yes", help="do not ask for confirmation", action="store_true", default=False
    )
    parser.add_argument("hostnames", nargs="*", help="Hostnames to update")
    return parser.parse_args()


def main(args):

    print()
    print(DESCRIPTION)
    print()

    # Look for services
    discovery_complete = Event()
    listener = Listener()

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

    discovery_complete.wait(DISCOVERY_TIMEOUT)
    browser.zc.close()

    if not listener.devices:
        print("Nothing found!\n")
        sys.exit(0)

    # Sort list
    field = args.sort.lower()
    if field not in listener.devices[0]:
        print("Unknown field '{}'\n".format(field))
        sys.exit(1)
    listener.devices = sorted(
        listener.devices, key=lambda device: device.get(field, "")
    )

    # Show devices overview
    print_devices(listener.devices)

    # Flash device
    if args.flash:

        # Board(s) to flash
        queue = []

        # Check if hostnames
        for hostname in args.hostnames:
            board = get_board_by_hostname(listener.devices, hostname)
            if board:
                board["auth"] = args.password
                board["flags"] = args.flags
                queue.append(board)

        # If no boards ask the user
        if len(queue) == 0:
            board = input_board(listener.devices)
            if board:
                board["auth"] = args.password or input(
                    "Authorization key of the device to flash: "
                )
                board["flags"] = args.flags or input("Extra flags for the build: ")
                queue.append(board)

        # If still no boards quit
        if len(queue) == 0:
            sys.exit(0)

        queue = sorted(queue, key=lambda device: device.get("board", ""))

        # Flash each board
        for board in queue:

            # Flash core version?
            if args.core:
                board["flags"] = "-DESPURNA_CORE " + board["flags"]

            env = "esp8266-{:d}m-ota".format(board["size"])

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
