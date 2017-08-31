#!/usr/bin/env python3

import sys, os, subprocess

def system(cmd):
    args = cmd.split()
    return subprocess.check_output(args).decode("ascii").strip()

if __name__ == "__main__":
    try:
        tag = system("git describe --abbrev=4 --dirty --always --tags")
    except:
        tag = "unknown"

    path = os.path.join(os.path.dirname(sys.argv[0]), "core/core_version.h")

    f = open(path, "w")
    f.write("#define CORE_VERSION \"%s\"" % tag)
    f.close()
