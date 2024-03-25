#!/bin/bash

set -eu -o pipefail

CMAKE_ARGS=""
SOURCE_DIR="."
BUILD_DIR="build/"
INSTALL_DIR="/usr/local"
BUILD_ARGS=""
POST_CMD=""

if [[ $# -eq 0 ]]; then # default
    BUIILD_TYPE="Debug"
elif [[ $1 == "help" ]]; then
    echo -e "Usage: $0 [release|debug|rwdi|min|clean|analyze|test|install|help]\n"
    echo "  release: build in release mode"
    echo "  debug:   build in debug mode"
    echo "  rwdi:    build in RelWithDebInfo mode"
    echo "  min:     build in MinSizeRel mode"
    echo "  clean:   clean build directory"
    echo "  analyze: build in debug mode and run analyze-build"
    echo "  test:    build in debug mode and run tests"
    echo "  install: build and install"
    echo "  help:    show this message"
    exit 0
elif [[ $1 == "clean" ]]; then
    rm -rf build/*
    touch build/.gitkeep
    exit 0
elif [[ $1 == "release" ]]; then
    BUIILD_TYPE="Release"
elif [[ $1 == "debug" ]]; then
    BUIILD_TYPE="Debug"
elif [[ $1 == "rwdi" ]]; then
    BUIILD_TYPE="RelWithDebInfo"
elif [[ $1 == "min" ]]; then
    BUIILD_TYPE="MinSizeRel"
elif [[ $1 == "analyze" ]]; then
    BUIILD_TYPE="Debug"
    CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

    ANALYZE_BIN=$(find / -name "analyze-build" 2>/dev/null || echo "")
    if [[ -z $ANALYZE_BIN ]]; then
        POST_CMD="echo -e \nuse 'analyze-build --cdb build/compile_commands.json' to run analyze-build manually"
    else
        POST_CMD="${ANALYZE_BIN} --cdb build/compile_commands.json"
    fi
elif [[ $1 == "test" ]]; then
    BUIILD_TYPE="Debug"
    CMAKE_ARGS="-DBUILD_TESTING=ON"
    BUILD_ARGS+="-t all test -- CTEST_OUTPUT_ON_FAILURE=1"
elif [[ $1 == "install" ]]; then
    BUIILD_TYPE="Release"
    BUILD_ARGS+="-t install"
else # unknown
    echo "Unknown command: $1"
    exit 1
fi

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=${BUIILD_TYPE} ${CMAKE_ARGS}"

cmake ${CMAKE_ARGS} -S ${SOURCE_DIR} -B ${BUILD_DIR}
cmake --build ${BUILD_DIR} -j $(nproc) ${BUILD_ARGS}
${POST_CMD}
