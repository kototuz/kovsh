#!/bin/bash

mkdir -p out/objs
mkdir -p out/bin

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
src=src/*

for i in $src; do
    $cc $cflags -o ${i/.c/.o} -c $i
done
