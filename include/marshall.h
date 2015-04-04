#ifndef MARSHALL_H
#define MARSHALL_H

float unpack_float(unsigned char *value);

unsigned char * pack_int32(unsigned char *to, int32_t intval);
unsigned char * pack_str(unsigned char *to, char *str, int n);

void dumpint(int32_t n);
void dumpchar(unsigned char n);

#endif

// EOF
