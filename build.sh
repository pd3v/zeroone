#!/bin/bash

cd build
cmake ..
make
cd zeroone
cling -std=c++14