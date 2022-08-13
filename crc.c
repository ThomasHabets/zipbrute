// $Id: crc.c,v 1.1 2001/04/23 17:53:52 thomas Exp $
#include "crc.h"
void crc_init(void)
{
	unsigned int i, j, c;

	for(i = 0; i < 256; i++) {
		c = i;
		for(j = 0; j < 8; j++) {
			if(c & 1) {
				c = (c >> 1) ^ CRCPOLY;
			} else {
				c = (c >> 1);
			}
		}
		crctab[i] = c;
		crcinvtab[c >> 24] = (c << 8) ^ i;
	}
//	for (int k = 0; k < 65536; k++) {
//		u_int32_t t;
//		t = k | 3;
//		key3table[k] = ((t * (t ^ 1)) >> 8) & 0xff;
//	}
}
