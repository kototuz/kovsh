#!/bin/bash

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
src=src/kovsh.c

if [[ $1 == "test" ]]; then
    $cc $cflags -o test.out $src
elif [[ $1 == "lextest" ]] then
    $cc $cflags -o lextest.out src/lexer.c src/strv.c -DMAIN
else $cc $cflags -o test.o -c $src
fi
