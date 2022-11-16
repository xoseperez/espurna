import argparse
import mmap
import os
import pytz
import sys

def zones(root):
    out = []

    for zone in pytz.all_timezones_set:
        with open(os.path.join(root, zone), "rb") as f:
            magic = f.read(4)
            if magic != b"TZif":
                continue

            m = mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ)
            newline = m.rfind(b'\n', 0, len(m) - 1)
            if newline < 0:
                continue

            m.seek(newline + 1)
            tz = m.readline().strip()

            out.append([zone, tz.decode("ascii")])

    out.sort(key=lambda x: x[0])
    return out

def table(zones):
    print("|Name|Value|")
    print("|---|---|")
    for name, value in zones:
        print(f"|{name}|{value}|")

    print()
    print(f"*{pytz.OLSON_VERSION=}*")
    print(f"*{pytz.VERSION=}*")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--root",
        help="Where do we get raw zoneinfo files from",
        default=os.path.join(os.path.dirname(pytz.__file__), "zoneinfo"))
    args = parser.parse_args()

    table(zones(args.root))
