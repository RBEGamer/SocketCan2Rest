#!/bin/bash

echo "-- BUILD CAN2REST ----"
# BUILD CAN2REST PROGRAM
pwd
rm CMakeCache.txt && rm -Rf CMakeFiles/
cmake .
make
chmod +x ./can2rest
rm -Rf ./CMakeFiles
rm -Rf ./cmake-build-debug
rm -f ./CMakeCache.txt
rm -f ./cmake_install.cmake
rm -f Makefile
cd ..