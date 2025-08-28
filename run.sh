#!/bin/bash

rm -f data
make clean
make
./main
# py graph.py