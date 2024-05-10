#!/bin/bash

source .callback.build

mkdir -p build
cd build
cmake ..
make