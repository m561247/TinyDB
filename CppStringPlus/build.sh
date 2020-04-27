#!/bin/sh
mugit select main.xml
mugit pull
mkdir build
rm -rf build/*
cd build
cmake ..
cmake --build . --config Debug
