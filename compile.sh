#!/bin/bash
clear
#shader/compile; echo "--------------------------------------------------"
cd build || (mkdir build && cd build && cmake .. || (echo "Failed to initialize build directory"; exit 0))
make -j12 || exit 0
cd ..
build/mini_projet.out || gdb build/mini_projet.out
