#ifndef MARSHALL_H
#define MARSHALL_H

float unpack_float(char **value);
int32_t unpack_int32(char **value);
void unpack_str(char **source, char *destination, int n);

unsigned char * pack_int32(unsigned char *to, int32_t intval);
unsigned char * pack_str(unsigned char *to, char *str, int n);

void dumpint(int32_t n);
void dumpchar(unsigned char n);

#endif

// EOF
