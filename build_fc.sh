#!/bin/bash
git submodule update --init --recursive
cd libraries/fc && make -j2 && cd ../..

