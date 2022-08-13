// $Id: zipbrute.h,v 1.8 2001/10/14 13:35:45 thomas Exp $

#ifndef __INCLUDE_ZIPBRUTE_H__
#define __INCLUDE_ZIPBRUTE_H__

#include "zip.h"

#ifndef PVM
#define PVM 0
#endif

#if PVM
enum { PVM_FILEDATA, PVM_FILECOUNT,PVM_CHARSET, PVM_PASSWORD };
#endif
#if 0
#pragma pack(1)
typedef struct ziplocal_s {
	unsigned char version[2];
	unsigned char gpb[2];
	unsigned char compr[2];
	unsigned char time[2];
	unsigned char date[2];
	unsigned long crc;
	unsigned long csize;
	unsigned long uncsize;
	unsigned short flen;
	unsigned short extralen;
} ziplocal_t;
#pragma unpack
#endif
#include "pwgen.h"

extern int verbose;
extern int verbose_report_machine;
extern int verbose_list_files;
extern int quiet;
extern const int pvm;

extern int filecount;
extern char filedata[MAXFILES][12];
extern char fileheaders[MAXFILES];
extern const char *filename;

typedef struct zipbrute_method_s {
	char *machine;
	int (*crack)(pwgen_handle_t*,char*);
} zipbrute_method_t;

extern int zipbrute_possible(pwgen_handle_t*,const char *password);
extern zipbrute_method_t zipbrute_methods[];
//void pvmzipbrute_possible(const char *password);
//int zipbrute_load(const char *_filename);
//int zipbrute_crack(pwgen_handle_t *state, char *password);

#endif
