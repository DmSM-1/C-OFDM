#!/bin/bash
set -e

rm -rf frames
mkdir -p frames

# Запускаем tx в фоне, а rx — в foreground
./tx &
./rx 
