#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <string.h>
#include <netinet/in.h>

float unpack_float(unsigned char **value) {
    union floatint32 {
        float   f;
        int32_t i;
    };

    union floatint32 result;
    memcpy(&(result.i), value, sizeof(int32_t));
    result.i = htonl(result.i);
    *value += sizeof(int32_t);
    return result.f;
}

int32_t unpack_int32(unsigned char **value) {
    int32_t result;
    memcpy(&result, value, sizeof(int32_t));
    result = htonl(result);
    *value += sizeof(int32_t);
    return result;
}

// destination must have size n+1 to fit the end of string \0
void unpack_str(unsigned char **source, char *destination, int n) {
    memcpy(destination, *source, sizeof(char)*n);
    destination[n] = '\0';
    *source += n;
}

unsigned char * pack_int32(unsigned char *to, int32_t intval)
{
    intval = htonl(intval);
    memcpy(to, &intval, sizeof(int32_t));
    return to + sizeof(int32_t);
}

// will pack the first n chars, don't end with \0 since string lengths
// are encoded in our binary representation
unsigned char * pack_str(unsigned char *to, char *str, int n)
{
    /*
	// could memcpy...
  	// assumes 8-bit char of course
	for (int i = 0; i < n; i++) {
		to[i] = str[i];
	}
    */
    memcpy(to, str, sizeof(char)*n);
  	return to + n; // return pointer to where packing ended
}

void dumpint(int32_t n)
{
    int32_t i;
    for (i = 1 << 31; i > 0; i = i / 2) {
        (n & i) ? printf("1"): printf("0");
	}
}

void dumpchar(unsigned char n)
{
    int32_t i;
    for (i = 1 << 7; i > 0; i = i / 2) {
        (n & i) ? printf("1"): printf("0");
	}
}

// EOF
