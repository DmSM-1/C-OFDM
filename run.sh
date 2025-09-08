#!/bin/bash
mkfifo /tmp/row_input

rm -f data.bin
make clean
make

py graph.py &
./main

rm /tmp/row_input