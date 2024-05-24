#!/bin/bash

mkdir -p zig-out/objs
mkdir -p zig-out/bin

cc=gcc
main=src/main.c
sources=src/*.c
obj_dir=out/objs
bin_dir=out/bin

function build_obj {
    if [ $# -eq 1 ]; then
        src_rep=src/
        obj=$1
        obj=${obj/.c/.o}
        obj=$obj_dir/${obj/$src_rep/}
        $cc -o $obj -c $1
        echo "$cc -o $obj -c $1"
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
        "run")
            $cc -o $bin_dir/main $main $obj_dir/*.o
            echo "$cc -o $bin_dir/main $main $obj_dir/*.o"
            ./$bin_dir/main
            ;;
    esac
else
    for s in $sources
    do
        if [ $s != "src/main.c" ]; then
            build_obj $s
        fi
    done
fi
