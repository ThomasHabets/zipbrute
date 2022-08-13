// $Id: zipbrute_global.c,v 1.3 2001/10/14 13:35:45 thomas Exp $

#include <sys/types.h>
#include <sys/utsname.h>
#include <errno.h>

#include "zip.h"
#include "zipbrute.h"


int verbose = 0;
int quiet = 0;
int verbose_report_machine = 1;
int verbose_list_files = 0;
int filecount = 5;
char filedata[MAXFILES][12];
char fileheaders[MAXFILES];

zipfile_t *zipfile;
const char *filename;

static u_int32_t multi_magic = 0x504b0708;
static u_int32_t end_magic = 0x504b0102;
static u_int32_t zipmagic = 0x504b0304;

int zipbrute_crack_i386(pwgen_handle_t*,char*);
int zipbrute_crack_i486(pwgen_handle_t*,char*);
int zipbrute_crack_i586(pwgen_handle_t*,char*);
int zipbrute_crack_i686(pwgen_handle_t*,char*);

zipbrute_method_t zipbrute_methods[] = {
	{"i386", zipbrute_crack_i386 },
	{"i486", zipbrute_crack_i486 },
	{"i586", zipbrute_crack_i586 },
	{"i686", zipbrute_crack_i686 },
	{"x86_64", zipbrute_crack_i686 },
	{"NULL" },
};

#if 1

/*
 * 
 */
int zipbrute_load(const char *_filename)
{
	int c,d,handle;
	zipfile_file_t *zff;
	
	if (!(zipfile = zipfile_open(_filename))) {
		fprintf(stderr, "zipfile_open(): fail\n");
		return 1;
	}

	for (handle = 0, c = 0; zff = zipfile_getfile(zipfile, &handle); c++) {
		fileheaders[c] = zff->it;
		zipfile_cread(filedata[c], 12, 0, zff);
	}
	filecount = c;
//	printf("Load done\n");
	return 0;
}
#else
int zipbrute_load(const char *_filename)
{
	FILE *fp;
	u_int32_t itmp;
	int c,d;
	ziplocal_t lf;

	filename = (char*)strdup(_filename);
	if (verbose) {
		printf("File: %s\n", filename);
	}

	if (!(fp = fopen(filename, "rb"))) {
		fprintf(stderr, "Unable to open zipfile(%s): %s\n",
			filename, strerror(errno));
		return 1;
	}
	if (verbose_list_files) {
		printf("Loading files:\nCompressed size    File name\n"
		       "----------------------------\n");
	}
	for (c = 0; c < MAXFILES; c++) {
		char buffer[128];
		int got_a_file;
		do {
			got_a_file = 1;

			fseek(fp, 11, SEEK_CUR);
			fread(&fileheaders[c], 1, 1, fp);
			fseek(fp, -12, SEEK_CUR);
			
			if (!fread(&itmp, sizeof(u_int32_t), 1, fp)) {
				fprintf(stderr, "Damaged zipfile\n");
				return 1;
			}
			itmp = htonl(itmp);
			
			if (itmp == multi_magic) {
				fseek(fp, 12, SEEK_CUR);
				got_a_file = 0;
			}
		} while (!got_a_file);

		if (itmp == end_magic) {
			break;
		}
		if (itmp != zipmagic) {
			fprintf(stderr, "Bad voodoo in zipfile: "
				"0x%x != 0x%x\n", itmp, zipmagic);
			return 1;
		}
		if (!fread(&lf, sizeof(ziplocal_t), 1, fp)) {
			fprintf(stderr, "fread(): FIXME\n");
		}
		// FIXME: warn if flen > 128
		fread(buffer, lf.flen, 1, fp);
		buffer[lf.flen] = 0;

		if (verbose_list_files) {
			printf("%15d    %s\n", lf.csize, buffer);
		}
		
		fseek(fp, lf.extralen, SEEK_CUR);
		fread(filedata[c], 12, 1, fp);


		for (d = 0; d < 12; d++) {
			printf("%.2x ", (unsigned char)filedata[c][d]);
		}
		printf("\n");

		fseek(fp, lf.csize - 12, SEEK_CUR);
	}
	filecount = c;
	if (fclose(fp)) {
		fprintf(stderr, "Warning: unable to fclose() zipfile\n");
	}
	return 0;
}
#endif


int zipbrute_crack(pwgen_handle_t *state, char *pwbuf)
{
	struct utsname ut;
	int c;
	static int (*cache)(pwgen_handle_t*,char*) = 0;
	if (cache) {
		return cache(state, pwbuf);
	}

	if (-1 == uname(&ut)) {
		perror("uname()");
		exit(1);
	}
	for (c = 0; strcmp(zipbrute_methods[c].machine, "NULL"); c++) {
		if (!strcasecmp(ut.machine, zipbrute_methods[c].machine)) {
			if (verbose_report_machine) {
				printf("Machine: %s\n", ut.machine);
			}
			cache = zipbrute_methods[c].crack;
			return zipbrute_methods[c].crack(state, pwbuf);
		}
	}
	fprintf(stderr, "Unable to find suitable cracker for machine %s\n",
		ut.machine);
	exit(1);
}
