// $Id: pwgen.h,v 1.4 2001/06/23 17:57:47 thomas Exp $

#ifndef __INCLUDE_PWGEN_H__
#define __INCLUDE_PWGEN_H__

struct pwgen_handle_s;

typedef struct pwgen_handle_s pwgen_handle_t;

typedef int (*pwgen_generate)(pwgen_handle_t *, char*);
typedef void (*pwgen_free)(pwgen_handle_t **);

#define PWGEN_INITIAL_STATE_STUFF \
pwgen_generate pwgen; \
pwgen_free free; \
int gencount; \
int total;

struct pwgen_handle_s {
	PWGEN_INITIAL_STATE_STUFF
};

extern int quiet;
extern int verbose;

#define PWGEN_MAXLEN_EBUF 64

#include "pwgen_inc.h"
#include "pwgen_dict.h"

#endif
