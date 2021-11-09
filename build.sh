#!/bin/bash

mkdir build
cd build
cmake ..
make
cd zeroone
cling -std=c++14