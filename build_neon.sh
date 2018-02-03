#!/bin/bash
set -e

COMPILER=arm-linux-gnueabi-gcc
FLAGS="-march=armv8-a -mfpu=neon-fp-armv8 -mfloat-abi=softfp -O3"

$COMPILER -S $FLAGS $1.c -o $1.s
$COMPILER $FLAGS $1.c -o $1

qemu-aarch64 -L /usr/arm-linux-gnueabi/ ./$1
