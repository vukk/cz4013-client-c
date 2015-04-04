#include <stdint.h>
#include <string.h>
#include <stdio.h>

unsigned char * pack_uint32(unsigned char *to, uint32_t intval)
{
    memcpy(to, &intval, sizeof(uint32_t));
    return to + sizeof(uint32_t);
}

// will pack the first n chars, then end with a \0
unsigned char * pack_str(unsigned char *to, char *str, int n)
{
	// could memcpy...
  	/* assumes 8-bit char of course */
	for (int i = 0; i < n; i++) {
		to[i] = str[i];
	}
	to[n] = '\0';
  	return to + n + 1; // return pointer to where packing ended
}

void dumpint(uint32_t n)
{
    uint32_t i;
    for (i = 1 << 31; i > 0; i = i / 2) {
        (n & i) ? printf("1"): printf("0");
	}
}

void dumpchar(unsigned char n)
{
    uint32_t i;
    for (i = 1 << 7; i > 0; i = i / 2) {
        (n & i) ? printf("1"): printf("0");
	}
}

// EOF
