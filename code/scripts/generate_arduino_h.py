import argparse
import pathlib

from espurna_utils import generate_arduino_h

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=pathlib.Path)
    args = parser.parse_args()

    generate_arduino_h(args.path)
