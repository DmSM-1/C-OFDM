#!/bin/bash

rm -f data.bin
make clean
make
./main
#py graph.py