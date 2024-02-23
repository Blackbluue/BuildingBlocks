#!/bin/bash

CMAKE_ARGS=""
MAKE_ARGS=""
POST_CMD=""

if [[ $1 == "clean" ]]; then
    rm -rf build/*
    touch build/.gitkeep
    exit 0
elif [[ $1 == "analyze" ]]; then
    CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    POST_CMD="analyze-build --cdb build/compile_commands.json"
elif [[ $1 == "test" ]]; then
    MAKE_ARGS="test"
fi

mkdir -p build
cd build
cmake ${CMAKE_ARGS} ..
make -j$(nproc) ${MAKE_ARGS}
cd ..
# ${POST_CMD}
