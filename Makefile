cc := gcc
cflags := -Wall -Wextra -pedantic

bin := zig-out/bin
objs := zig-out/objs

kovsh_main := src/main.c
kovsh_sources := $(wildcard src/**/*.c)
kovsh_objects = $(subst src/, $(objs)/, $(kovsh_sources))
kovsh_objects := $(kovsh_objects:.c=.o)

run_file := $(kovsh_main)
run_file := $(patsubst %.c, %,$(subst src/, $(bin)/, $(run_file)))

.PHONY: dirs run

all: dirs $(kovsh_objects)
	$(cc) $(cflags) -o $(bin)/kovsh $(kovsh_main) $(kovsh_objects)

$(objs)/%.o: src/%.c
	mkdir -p -v $(dir $@)
	$(cc) $(cflags) -o $@ -c $<

dirs:
	mkdir -p src
	mkdir -p zig-out/bin
	mkdir -p zig-out/objs

$(bin)/%: src/%.c $(kovsh_objects)
	$(cc) $(flags) -o $(basename $@) $< $(kovsh_objects)

run: $(run_file)
	./$(basename $<)

clean:
	rm -rf $(bin)/* $(kovsh_objects)
