#include "kovsh.h"
#include <stdio.h>
#include <string.h>

void strslice_print(StrSlice ss, FILE *output)
{
    for (size_t i = 0; i < ss.len; i++) {
        fputc(ss.items[i], output);
    }
}

int strslice_cmp(StrSlice ss1, StrSlice ss2)
{
    if (ss1.len > ss2.len) return 1;
    else if (ss1.len < ss2.len) return -1;
    return strncmp(ss1.items, ss2.items, ss1.len);
}
