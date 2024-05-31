cc := gcc

export termio=TERMIO_DEFAULT
cflags := -Wall -Wextra -pedantic -g -DTERMIO=$(termio)

main:=src/main.c

bin_dir:=out/bin
obj_dir:=out/objs

sources:=$(wildcard src/*.c)
sources:=$(subst $(main),,$(sources))

objects:=$(subst src,$(obj_dir),$(sources))
objects:=$(objects:.c=.o)


.PHONY: dirs run

all: dirs $(objects)

$(obj_dir)/%.o: src/%.c
	$(cc) $(cflags) -o $@ -c $<

dirs:
	mkdir -p $(bin_dir)
	mkdir -p $(obj_dir)

run: $(main) $(objects)
	$(cc) $(cflags) -o $(bin_dir)/main $(main) $(objects) -lncurses
	./$(bin_dir)/main

clean:
	rm -rf $(bin_dir)/* $(obj_dir)/*
