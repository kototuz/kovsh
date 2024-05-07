cc := gcc
cflags := -Wall -Wextra -pedantic -g

out := zig-out
bin := $(out)/bin
objs := $(out)/objs

kovsh_main := src/main.c
kovsh_sources := $(wildcard src/*.c)
kovsh_sources := $(subst src/main.c,,$(kovsh_sources))
kovsh_objects = $(subst src/, $(objs)/, $(kovsh_sources))
kovsh_objects := $(kovsh_objects:.c=.o)

export run_file := $(kovsh_main)

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
	$(cc) $(cflags) -o zig-out/bin/$(basename $(notdir $@)) $< $(kovsh_objects)

run: $(patsubst %.c, %,$(subst src/, $(bin)/, $(run_file)))
	./zig-out/bin/$(basename $(notdir $<))

clean:
	rm -rf $(out)/*/* $(kovsh_objects)
