#!/bin/sh

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
src=kovsh.c
term=examples/simpleterm.c
parse_cargs=examples/cargs.c

if [ "$1" = "term" ]; then
    $cc $cflags -o term.out examples/simpleterm.c $src
    ./term.out
elif [ "$1" = "cargs" ]; then
    $cc $cflags -o cargs.out examples/cargs.c $src
else
    $cc $cflags -o test.o -c $src
fi
