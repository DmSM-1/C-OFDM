#!/bin/bash
# mkfifo /tmp/row_input

rm -f data.bin
make clean
make

# py python_code/real_time_graph.py &
./main

# rm /tmp/row_input