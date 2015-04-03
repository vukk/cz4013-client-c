#include <stdint.h>

unsigned char * pack_uint32(unsigned char *to, uint32_t intval)
{
  	/* assumes 8-bit char of course */
  	to[0] = intval >> 24;
  	to[1] = intval >> 16;
  	to[2] = intval >> 8;
  	to[3] = intval;
  	return to + 4; // return pointer to where packing ended
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

// EOF
