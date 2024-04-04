CC := gcc
CFLAGS := -Wall -Wextra -pedantic -O0 -std=c11 -g
SRC := cmnd.c

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%: %.o
	ld -o $@ $<
	./$@
	rm $@

cmnd_test: cmnd.o
	$(CC) $(CFLAGS) -o test cmnd.o
