// $Id: crc.h,v 1.1 2001/04/23 17:53:52 thomas Exp $

#include <sys/types.h>

#define CRCPOLY 0xedb88320
#define CRC32(x,c)      (((x)>>8)^crctab[((x)^(c))&0xff])
#define INVCRC32(x,c)   (((x)<<8)^crcinvtab[((x)>>24)&0xff]^((c)&0xff))

u_int32_t crctab[256], crcinvtab[256];
