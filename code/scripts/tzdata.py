import argparse
import mmap
import os
import pytz
import re
import sys

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


def zones(root):
    out = []

    for zone in pytz.all_timezones_set:
        with open(os.path.join(root, zone), "rb") as f:
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


def table(zones):
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
    print(f"Generated with *{pytz.OLSON_VERSION=} {pytz.VERSION=}*")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        help="Where do we get raw zoneinfo files from",
        default=os.path.join(os.path.dirname(pytz.__file__), "zoneinfo"),
    )
    args = parser.parse_args()

    table(zones(args.root))
