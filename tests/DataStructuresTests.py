#!/usr/bin/env python3
"""Test data structures 1."""
import os
from subprocess import PIPE, Popen

FAIL = -1
PASS = 0

bin_loc = "build/"
tests = [
    "test_array_list",
    # "test_list",
    # "test_queue",
    # "test_queue_p",
    # "test_stack",
    # "test_table",
    # "test_tree",
    # "test_threadpool",
]


def test_binary(binary: str):
    """Run a binary and check the output.

    Args:
        binary (str): The binary to run.

    Returns:
        int: PASS or FAIL
    """
    p = Popen([binary], stdin=PIPE, stdout=PIPE, stderr=PIPE, cwd=bin_loc)
    output, err = p.communicate()
    str_out = output.decode("ascii")

    if p.returncode != 0:
        print(f"{binary} Failed!")
        print("Failing Output")
        print(output.decode("ascii"))
        print(err.decode("ascii"))
        return FAIL
    return PASS


def test_valgrind(binary: str):
    """Run a binary with valgrind and check the output.

    Args:
        binary (str): The binary to run.

    Returns:
        int: PASS or FAIL
    """
    p = Popen(
        [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--error-exitcode=1",
            binary,
        ],
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
        cwd=bin_loc,
    )
    output, err = p.communicate()
    str_out = output.decode("ascii")

    if p.returncode == 0:
        return PASS
    print(f"{binary} Failed!")
    print("Failing Output")
    print(output.decode("ascii"))
    print(err.decode("ascii"))
    return FAIL


def main():
    """Run main function."""
    numtests = 0
    numpassed = 0

    for test in tests:
        if not os.path.isfile(f"{bin_loc}{test}"):
            continue
        numtests += 1
        res1 = test_binary(f"./{test}")
        res2 = test_valgrind(f"./{test}")
        numpassed = (
            numpassed + 1 if (PASS == res1 and PASS == res2) else numpassed
        )

    print(f"\nPassed {numpassed} out of {numtests} ")

    ret = 0 if numtests == numpassed else -1

    exit(ret)


if __name__ == "__main__":
    main()
