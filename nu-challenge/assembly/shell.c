#include <stdio.h>
#include <string.h>

//const char code[] = "\xb8\x0a\x00\x00\x00\xc3";
int main() {
    int foo_value = 0;
    const char code[] = "\x48\x31\xf6\x56\x48\xbf\x2f\x62\x69\x6e\x2f\x2f\x73\x68\x57\x54\x5f\x6a\x3b\x58\x99\x0f\x05";
    int (*foo)() = (int(*)())code;
    foo_value = foo();
    printf("%d\n", foo_value);

}
