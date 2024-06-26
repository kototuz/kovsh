#!/bin/bash

mkdir -p out/objs
mkdir -p out/bin

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
src=src/kovsh.c

$cc $cflags -o test.out $src
