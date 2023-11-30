import argparse
import mmap
import os
import pathlib
import re
import sys

from importlib import resources

import tzdata  # https://tzdata.readthedocs.io/en/latest/


def tzdata_version():
    return f"Generated with *{tzdata.IANA_VERSION=} {tzdata.__version__=}*"


def maybe_fix_value(value):
    return re.sub(r"<[^>]*>", "UNK", value)


def maybe_skip_zone(zone):
    return zone.startswith("Etc/") or zone in ("GB-Eire", "W-SU")


def utc_alias(zone):
    return zone in (
        "Universal",
        "UTC",
        "UCT",
        "Zulu",
        "GMT",
        "GMT+0",
        "GMT-0",
        "GMT0",
        "Greenwich",
    )


def tzdata_resource_from_name(name):
    pair = name.rsplit("/", 1)
    if len(pair) == 1:
        return resources.files("tzdata.zoneinfo") / pair[0]

    return resources.files(f'tzdata.zoneinfo.{pair[0].replace("/", ".")}') / pair[1]


def make_zones_list(zones):
    with open(zones, "r") as f:
        zones = [zone.strip() for zone in f.readlines()]

    return zones


def make_zones(args):
    out = []

    for zone in make_zones_list(args.zones):
        target = tzdata_resource_from_name(zone)
        with tzdata_resource_from_name(zone).open("rb") as f:
            magic = f.read(4)
            if magic != b"TZif":
                continue

            m = mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ)
            newline = m.rfind(b"\n", 0, len(m) - 1)
            if newline < 0:
                continue

            m.seek(newline + 1)
            tz = m.readline().strip()

            out.append([zone, tz.decode("ascii")])

    out.sort(key=lambda x: x[0])
    return out


def markdown(zones):
    utcs = []
    rows = []

    for name, value in zones:
        if utc_alias(name):
            utcs.append(name)
            continue

        if maybe_skip_zone(name):
            continue

        rows.append(f"|{name}|{maybe_fix_value(value)}|")

    print("|Name|Value|")
    print("|---|---|")
    for name in utcs:
        print(f"|{name}|UTC0|")

    last = ""
    for row in rows:
        prefix, _, _ = row.partition("/")
        if last != prefix:
            last = prefix
            print("|||")
        print(row)
    print()
    print("---")
    print()
    print(tzdata_version())


def header(zones):
    print(f"// ! ! ! DO NOT EDIT, AUTOMATICALLY GENERATED ! ! !")
    print(f"// {tzdata_version()}")
    print()
    print("#pragma once")
    print()
    for name, value in zones:
        print(f'#define\tTZ_{name.replace("/", "_")}\tPSTR("{value}")')


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--format",
        default="markdown",
        choices=["markdown", "header"],
    )
    parser.add_argument(
        "--zones",
        help="Zone names, one per line",
        default=os.path.join(os.path.dirname(tzdata.__file__), "zones"),
    )
    parser.add_argument(
        "--root",
        help="Where do we get raw zoneinfo files from",
        default=os.path.join(os.path.dirname(tzdata.__file__), "zoneinfo"),
    )
    args = parser.parse_args()

    zones = make_zones(args)
    if args.format == "markdown":
        markdown(zones)
    elif args.format == "header":
        header(zones)
