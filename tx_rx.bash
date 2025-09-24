#!/bin/bash
set -e

rm -rf frames
mkdir -p frames

# Запускаем tx в фоне, а rx — в foreground
./tx &
# valgrind --leak-check=full --track-origins=yes ./rx
nice -n -10 ./rx 
