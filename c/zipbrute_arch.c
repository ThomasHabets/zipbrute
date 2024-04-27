// $Id: zipbrute_arch.c,v 1.2 2001/06/23 17:57:47 thomas Exp $

#include <stdio.h>
#include <stdlib.h>

#include "zipbrute.h"

static u_int32_t key[3];
static u_int8_t key3;
static u_int8_t tkey3;
static u_int32_t tkey[3];

#define I386 1
#define I486 2
#define I586 3
#define I686 4

#if ZB_ARCH == I386
#define ZB_zipbrute_load 1
#define ZB_zipbrute_crack 1
#define ZB_updatekeys 1
#define ZB_updatetkeys 1
#define zipbrute_load zipbrute_load_i386
#define zipbrute_crack zipbrute_crack_i386
#include "zipbrute.c"
#undef zipbrute_load
#undef zipbrute_crack

#elif ZB_ARCH == I486
#define ZB_zipbrute_load 1
#define ZB_zipbrute_crack 1
#define ZB_updatekeys 1
#define ZB_updatetkeys 1
#define zipbrute_crack zipbrute_crack_i486
#include "zipbrute.c"
#undef zipbrute_load
#undef zipbrute_crack

#elif ZB_ARCH == I586
#define ZB_zipbrute_load 1
#define ZB_zipbrute_crack 1
#define ZB_updatekeys 1
#define ZB_updatetkeys 1
#define zipbrute_crack zipbrute_crack_i586
#include "zipbrute.c"
#undef zipbrute_load
#undef zipbrute_crack

#elif ZB_ARCH == I686
#define ZB_zipbrute_load 1
#define ZB_zipbrute_crack 1
#define ZB_updatekeys 1
#define ZB_updatetkeys 1
#define zipbrute_crack zipbrute_crack_i686
#include "zipbrute.c"
#undef zipbrute_load
#undef zipbrute_crack
#endif
