#!/bin/bash
cd programs/witness_node && make -j2 && cd ../..
cd programs/cli_wallet && make -j2 && cd ../..

