#!/bin/sh

mkdir -p build
cd build

if [ "$1" = "debug" ]; then
    export SANITIZERS="-fsanitize=address,undefined"
    cmake -DCMAKE_BUILD_TYPE=Debug ..
else
    cmake ..
fi
cmake --build .
./sdlgl-example
