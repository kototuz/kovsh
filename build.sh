#!/bin/bash

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
src=kovsh.c
test=examples/simpleterm.c

if [[ $1 == "test" ]]; then
     $cc $cflags -o test.out $test $src
else $cc $cflags -o test.o -c $src
fi
