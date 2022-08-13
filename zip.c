// $Id: zip.c,v 1.1 2002/11/16 02:50:42 thomas Exp $
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "zip.h"

static char magic_header[] = "\x50\x4b\x03\x04";
static char magic_multi[] = "\x50\x4b\x07\x08";
static char magic_eof[] = "\x50\x4b\x01\x02";

zipfile_t *zipfile_open(const char *filename)
{
	zipfile_t *ret;
	int c;
	char buffer[128];
	ziplocal_t ziplocal;

	if (!(ret = malloc(sizeof(zipfile_t)))) {
		goto fail_retmalloc;
	}

	ret->filename = strdup(filename);
	if (!(ret->fp = fopen(ret->filename, "rb"))) {
		goto fail_fopen;
	}
	fseek(ret->fp, 0, SEEK_END);
	c = ftell(ret->fp);
	fseek(ret->fp, 0, SEEK_SET);
	ret->data = mmap(0, c, PROT_READ, MAP_PRIVATE, fileno(ret->fp), 0);

	// FIXME: count files
	if (!(ret->files = malloc(sizeof(zipfile_file_t)*MAXFILES))) {
		goto fail_filesmalloc;
	}

	for (c = 0; ; c++) {
//		printf("%d\n", ftell(ret->fp));
		if (1 != fread(buffer, 4, 1, ret->fp)) {
			break;
		}
		if (!memcmp(buffer, magic_multi, 4)) {
//			printf("Skipping multi\n");
			fseek(ret->fp, 12, SEEK_CUR);
			c--;
			continue;
		}
		if (!memcmp(buffer, magic_eof, 4)) {
//			printf("Skipping multi\n");
			break;
		}
		if (memcmp(buffer, magic_header, 4)) {
			printf("Unknown magic 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n",
			       (unsigned char)buffer[0],
			       (unsigned char)buffer[1],
			       (unsigned char)buffer[2],
			       (unsigned char)buffer[3]);
			goto fail_freadsig;
		}
		if (1 != fread(&ziplocal, sizeof(ziplocal_t), 1, ret->fp)) {
			goto fail_freadsig;
		}
		/*
		 * FIXME: endianify ziplocal
		 */
		buffer[ziplocal.flen] = 0;
		if (1 != fread(buffer, ziplocal.flen, 1, ret->fp)) {
			goto fail_freadsig;
		}
		fseek(ret->fp, ziplocal.extralen, SEEK_CUR);
		ret->files[c].filename = strdup(buffer);
		ret->files[c].csize = ziplocal.csize;
		ret->files[c].zipfile = ret;
		ret->files[c].it = ziplocal.time[1];
		ret->files[c].pos = ftell(ret->fp);
		fseek(ret->fp, ziplocal.csize, SEEK_CUR);
	}

	ret->filecount = c;
	return ret;

 fail_freadsig:
	
 fail_filesmalloc:
	fclose(ret->fp);
 fail_fopen:
	free(ret);
 fail_retmalloc:
	return 0;
	
}

zipfile_file_t *zipfile_getfile(zipfile_t *zf, int *c)
{
	if (*c >= zf->filecount) {
		return 0;
	}
	return &zf->files[(*c)++];
}

size_t zipfile_cread(char*buffer, size_t size,int pos,zipfile_file_t*zff)
{
	int bytes;
	bytes = size;
	if (bytes + pos > zff->csize) {
		bytes = zff->csize - pos;
	}
//	printf("copying %d bytes from ofs %d\n",
//	       bytes, zff->pos);
	memcpy(buffer, &zff->zipfile->data[zff->pos], bytes);
	return bytes;
}
#if 0
int main(void)
{
	zipfile_t *zf;
	int c;
	zipfile_file_t *zff;

	if (!(zf = zipfile_open("c/stored.zip"))) {
		perror("zipfile_open(): ");
	}

	for (c = 0; zff = zipfile_getfile(zf, &c); ) {
		char buffer[128];
		printf("File: %s\t", zff->filename);
		buffer[zipfile_cread(buffer, 10, 0, zff)] = 0;
		printf("k%sk\n", buffer);
	}

	return 0;
}
#endif
