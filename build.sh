#!/bin/bash

if [[ $1 == "clean" ]]; then
    rm -rf build/*
    touch build/.gitkeep
    exit 0
fi

mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ..
