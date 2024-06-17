#!/bin/bash

mkdir -p out/objs
mkdir -p out/bin

cc=gcc
cflags="-Wall -Wextra -Wpedantic -g"
main=src/main.c
sources=src/*.c
obj_dir=out/objs
bin_dir=out/bin
examples=examples/*.c

function build_obj {
    if [ $# -eq 1 ]; then
        src_rep=src/
        obj=$1
        obj=${obj/.c/.o}
        obj=$obj_dir/${obj/$src_rep/}
        $cc $cflags -o $obj -c $1
        echo "$cc $cflags -o $obj -c $1"
    fi
}

if [ $# -eq 1 ]; then
    case $1 in
        src/*.c)
            build_obj $1
            ;;
        "clean")
            rm -rf $obj_dir/* $bin_dir/*
            ;;
    esac
else
    for s in $sources
    do
        build_obj $s
    done
fi
