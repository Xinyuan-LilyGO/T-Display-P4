#!/bin/bash

set -e

args=$1

# build the test binary
rm -rf build
mkdir build
cd build
cmake -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} ${args}" ..
make -j4

# run it
cd ..

./build/radiolib-unittest --log_level=message --detect_memory_leaks
