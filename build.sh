#!/bin/bash

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
src=src/kovsh.c

if [[ $1 == "test" ]]
then $cc $cflags -o test.out $src
else $cc $cflags -o test.o -c $src
fi
