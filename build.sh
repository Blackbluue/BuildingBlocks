#!/bin/bash

CMAKE_ARGS=""
MAKE_ARGS=""
POST_CMD=""

if [[ $1 == "help" ]]; then
    echo -e "Usage: $0 [release|debug|rwdi|min|clean|analyze|test|help]\n"
    echo "  release: build in release mode"
    echo "  debug:   build in debug mode"
    echo "  rwdi:    build in RelWithDebInfo mode"
    echo "  min:     build in MinSizeRel mode"
    echo "  clean:   clean build directory"
    echo "  analyze: build in debug mode and run analyze-build"
    echo "  test:    build in debug mode and run tests"
    echo "  help:    show this message"
    exit 0
elif [[ $1 == "clean" ]]; then
    rm -rf build/*
    touch build/.gitkeep
    exit 0
elif [[ $1 == "release" ]]; then
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
elif [[ $1 == "debug" ]]; then
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug"
elif [[ $1 == "rwdi" ]]; then
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=RelWithDebInfo"
elif [[ $1 == "min" ]]; then
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=MinSizeRel"
elif [[ $1 == "analyze" ]]; then
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug"
    CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    POST_CMD="analyze-build --cdb build/compile_commands.json"
elif [[ $1 == "test" ]]; then
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug"
    CMAKE_ARGS+=" -DBUILD_TESTING=ON"
    MAKE_ARGS="CTEST_OUTPUT_ON_FAILURE=1 all test"
elif [[ -z $1 ]]; then # default
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Debug"
elif [[ -n $1 ]]; then # unknown
    echo "Unknown command: $1"
    exit 1
fi

mkdir -p build
cd build
cmake ${CMAKE_ARGS} ..
make -j$(nproc) ${MAKE_ARGS}
cd ..
# ${POST_CMD}
