cc := gcc
cflags := -Wall -Wextra -pedantic -g

main:=src/main.c

lib_dir:=out/lib
obj_dir:=out/objs

sources:=$(wildcard src/*.c)

objects:=$(subst src,$(obj_dir),$(sources))
objects:=$(objects:.c=.o)

examples:=$(wildcard examples/*.c)

.PHONY: dirs

all: dirs libkovsh.a

libkovsh.a: $(objects)
	ar rcs $(lib_dir)/libkovsh.a $(objects)

$(obj_dir)/%.o: src/%.c
	$(cc) $(cflags) -o $@ -c $<

examples/%.test: libkovsh.a examples/%.c
	$(cc) $(cflags) -o $@ $(@:.test=.c) -L$(lib_dir) -lkovsh

dirs:
	mkdir -p $(lib_dir)
	mkdir -p $(obj_dir)

clean:
	rm $(lib_dir)/libkovsh.a $(objects)
