#include "kovsh.h"
#include <stdio.h>

void strslice_print(StrSlice ss)
{
    for (size_t i = 0; i < ss.len; i++) {
        putchar(ss.items[i]);
    }
}
