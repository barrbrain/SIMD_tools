#!/bin/bash
set -e

COMPILER=aarch64-linux-gnu-gcc
FLAGS="-Wall -Werror -march=armv8-a -ftree-vectorize -ftree-vectorizer-verbose=1 -O3"

$COMPILER -S $FLAGS $1.c -o $1.s
$COMPILER $FLAGS $1.c -o $1

qemu-aarch64 -L /usr/aarch64-linux-gnu/ ./$1
 
