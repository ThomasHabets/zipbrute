
#ifndef __INCLUDE_ZIP_H__
#define __INCLUDE_ZIP_H__

#include<stdio.h>
#include<sys/types.h>
#define MAXFILES 30

typedef struct zipfile_s zipfile_t;
typedef struct {
	char *filename;
	int pos;
	int csize;
	int size;
	unsigned char it;
	zipfile_t *zipfile;
} zipfile_file_t;

struct zipfile_s {
	char *filename;
	FILE *fp;
	int filecount;
	char *data;
	zipfile_file_t *files;
};

#pragma pack(push, 1)
typedef struct ziplocal_s {
	unsigned char version[2];
	unsigned char gpb[2];
	unsigned char compr[2];
	unsigned char time[2];
	unsigned char date[2];
	u_int32_t crc;
	u_int32_t csize;
	u_int32_t uncsize;
	unsigned short flen;
	unsigned short extralen;
} ziplocal_t;
#pragma pack(pop)

#endif
