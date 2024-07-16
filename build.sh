#!/bin/bash

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
src=kovsh.c
term=examples/simpleterm.c
parse_cargs=examples/cargs.c

if [[ $1 == "term" ]]; then
    $cc $cflags -o term.out $term $src
elif [[ $1 == "cargs" ]]; then
    $cc $cflags -o cargs.out $parse_cargs $src
else $cc $cflags -o test.o -c $src
fi
