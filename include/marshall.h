#ifndef MARSHALL_H
#define MARSHALL_H

unsigned char * pack_uint32(unsigned char *to, uint32_t intval);
unsigned char * pack_str(unsigned char *to, char *str, int n);

void dumpint(uint32_t n);
void dumpchar(unsigned char n);

#endif

// EOF
