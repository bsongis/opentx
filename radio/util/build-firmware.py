#!/usr/bin/env python

from __future__ import print_function
import os
import sys
import subprocess
import shutil
import filelock

from fwoptions import *

# Error codes
FIRMWARE_SIZE_TOO_BIG = 1
COMPILATION_ERROR = 4
INVALID_FIRMWARE = 5
INVALID_BOARD = 6
INVALID_LANGUAGE = 7


def build_target(target, path, cmake_options):
    srcdir = os.path.dirname(os.path.realpath(__file__)) + "/../.."
    outpath = path + ".out"
    errpath = path + ".err"

    # Launch CMake
    cmd = ["cmake"]
    for option, value in cmake_options.items():
        cmd.append("-D%s=%s" % (option, value))
    if "OPENTX_VERSION_SUFFIX" in os.environ:
        suffix = os.environ["OPENTX_VERSION_SUFFIX"]
        cmd.append('-DVERSION_SUFFIX="%s"' % suffix)
        if suffix.startswith("N"):
            cmd.append('-DTEST_BUILD_WARNING=YES')
    cmd.append(srcdir)

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = proc.communicate()
    if proc.returncode == 0:
        open(outpath, "a").write("\n".join(cmd) + output.decode("utf-8") + error.decode("utf-8"))
    else:
        open(errpath, "w").write(output.decode("utf-8") + error.decode("utf-8"))
        return COMPILATION_ERROR

    # Launch make
    cmd = ["make", "-j2", target]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = proc.communicate()
    if proc.returncode == 0:
        open(outpath, "a").write(output.decode("utf-8") + error.decode("utf-8"))
    else:
        open(errpath, "w").write(output.decode("utf-8") + error.decode("utf-8"))
        return COMPILATION_ERROR

    return 0


def main():
    if len(sys.argv) != 3:
        exit(INVALID_FIRMWARE)

    target = sys.argv[1]
    directory, filename = os.path.split(sys.argv[2])
    root, ext = os.path.splitext(filename)
    options = root.split("-")

    if len(options) < 2 or options[0] != "opentx":
        exit(INVALID_FIRMWARE)

    optcount = 1
    cmake_options = {}
    board_name = options[optcount]

    if board_name == "sky9x":
        cmake_options["PCB"] = "SKY9X"
        firmware_options = options_sky9x
        maxsize = 65536 * 4
    elif board_name == "9xrpro":
        cmake_options["PCB"] = "9XRPRO"
        cmake_options["SDCARD"] = "YES"
        firmware_options = options_sky9x
        maxsize = 65536 * 4
    elif board_name == "ar9x":
        cmake_options["PCB"] = "AR9X"
        cmake_options["SDCARD"] = "YES"
        firmware_options = options_ar9x
        maxsize = 65536 * 4
    elif board_name == "x9lite":
        cmake_options["PCB"] = "X9LITE"
        firmware_options = options_taranis_x9lite
        maxsize = 65536 * 8
    elif board_name == "x9lites":
        cmake_options["PCB"] = "X9LITES"
        firmware_options = options_taranis_x9lite
        maxsize = 65536 * 8
    elif options[optcount] == "x7":
        cmake_options["PCB"] = "X7"
        firmware_options = options_taranis_x9dp
        maxsize = 65536 * 8
    elif options[optcount] == "x7access":
        cmake_options["PCB"] = "X7"
        cmake_options["PCBREV"] = "ACCESS"
        firmware_options = options_taranis_x9dp
        maxsize = 65536 * 8
    elif board_name == "xlite":
        cmake_options["PCB"] = "XLITE"
        firmware_options = options_taranis_xlite
        maxsize = 65536 * 8
    elif board_name == "xlites":
        cmake_options["PCB"] = "XLITES"
        firmware_options = options_taranis_xlites
        maxsize = 65536 * 8
    elif board_name == "x9d":
        cmake_options["PCB"] = "X9D"
        firmware_options = options_taranis_x9d
        maxsize = 65536 * 8
    elif board_name == "x9d+":
        cmake_options["PCB"] = "X9D+"
        firmware_options = options_taranis_x9dp
        maxsize = 65536 * 8
    elif board_name == "x9d+2019":
        cmake_options["PCB"] = "X9D+"
        cmake_options["PCBREV"] = "2019"
        firmware_options = options_taranis_x9dp
        maxsize = 65536 * 8
    elif board_name == "x9e":
        cmake_options["PCB"] = "X9E"
        firmware_options = options_taranis_x9e
        maxsize = 65536 * 8
    elif board_name == "x10":
        cmake_options["PCB"] = "X10"
        firmware_options = options_horus_x10
        maxsize = 2 * 1024 * 1024
    elif board_name == "x10express":
        cmake_options["PCB"] = "X10"
        cmake_options["PCBREV"] = "EXPRESS"
        firmware_options = options_horus_x10
        maxsize = 2 * 1024 * 1024
    elif board_name == "x12s":
        cmake_options["PCB"] = "X12S"
        firmware_options = options_horus_x12s
        maxsize = 2 * 1024 * 1024
    elif board_name == "t12":
        cmake_options["PCB"] = "X7"
        cmake_options["PCBREV"] = "T12"
        firmware_options = options_jumper_t12
        maxsize = 65536 * 8
    elif board_name == "t16":
        cmake_options["PCB"] = "X10"
        cmake_options["PCBREV"] = "T16"
        firmware_options = options_jumper_t16
        maxsize = 2 * 1024 * 1024
    else:
        exit(INVALID_BOARD)

    if target == "firmware":
        binary = "firmware.bin"
        ext = ".bin"
        filename = "opentx"
    elif target == "libsimulator":
        binary = "libopentx-" + board_name + "-simulator.so"
        ext = ".so"
        filename = "libopentx"
    else:
        exit(INVALID_BOARD)

    filename += "-" + board_name
    optcount += 1

    # The firmware options
    for opt, values in firmware_options.items():
        found = False
        for i in range(optcount, len(options)):
            if options[i] == opt:
                found = True
                break

        if not isinstance(values, list):
            values = [values]

        for name, value1, value2 in values:
            if found:
                value = value1
                filename += "-" + opt
            else:
                value = value2

            if value is not None:
                cmake_options[name] = value

    # The firmware display language
    language = ""
    for key in languages:
        if key == options[-1]:
            language = key
    if not language:
        exit(INVALID_LANGUAGE)
    cmake_options["TRANSLATIONS"] = language.upper()

    filename += "-" + language + ext
    path = os.path.join(directory, filename)
    errpath = path + ".err"

    if os.path.isfile(errpath):
        print(filename)
        exit(COMPILATION_ERROR)

    if os.path.isfile(path):
        print(filename)
        exit(0)

    lockpath = path + ".lock"
    lock = filelock.FileLock(lockpath)
    try:
        with lock.acquire(timeout=60 * 60):
            if not os.path.isfile(path):
                result = build_target(target, path, cmake_options)
                if result != 0:
                    print(filename)
                    return result
    except filelock.Timeout:
        print(filename)
        exit(COMPILATION_ERROR)

    if target == "firmware":
        # Check binary size
        size = os.stat(binary).st_size
        if size > maxsize:
            return FIRMWARE_SIZE_TOO_BIG

    # Copy binary to the binaries directory
    shutil.move(binary, path)

    print(filename)
    exit(0)


if __name__ == "__main__":
    main()
