#!/usr/bin/env python3
import sys


def raise_error(s):
    print(s)
    sys.exit(1)


def get_out_lines(file):
    result = list()
    for line in map(lambda x: x.strip(), open(file).readlines()):
        result.append(line)
    return result


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: %s <output-file> <expected>" % sys.argv[0])
        sys.exit(1)

    lines = get_out_lines(sys.argv[1])
    expected = sys.argv[2]

    if len(lines) == 0 and expected == 'unknown':
        raise_error("Expected splits")
    if lines[2] == 'unknown' and lines[5] == '0' or lines[5] == '':
        raise_error("Error in creating splits")
    if lines[2] != 'unknown' and lines[2] != lines[0]:
        raise_error("Unsound Result while partitioning")
    if expected != 'unknown' and lines[5] != '0':
        raise_error("Expected %s but got splits" % expected)

    sys.exit(0)
