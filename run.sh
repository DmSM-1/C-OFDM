#!/bin/bash
# mkfifo /tmp/row_input

rm -f data.bin
make clean
make

# py real_time_graph.py &
./main
py graph.py

# rm /tmp/row_input