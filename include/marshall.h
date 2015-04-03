#ifndef MARSHALL_H
#define MARSHALL_H

unsigned char * pack_uint32(unsigned char *to, uint32_t intval);
unsigned char * pack_str(unsigned char *to, char *str, int n);

#endif

// EOF
