#!/bin/bash
git submodule update --init --recursive
cmake .
cd libraries/fc && make -j2 && cd ../..

