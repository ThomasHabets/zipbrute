// $Id: nopvm_main.c,v 1.4 2002/03/11 14:42:15 thomas Exp $
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

#include "pwgen.h"
#include "crc.h"
#include "zipbrute.h"

const int pvm = 0;
pwgen_handle_t *state;
unsigned int waitstatustime = 1;
char *pwbuf;
int use_unzip = 0;
int zipbrute_load(const char *_filename);
/*
 *
 */
int zipbrute_possible(pwgen_handle_t *state, const char *password)
{
	if (use_unzip) {
		char buff[1024];
		int status;
		snprintf(buff, 1023, "unzip -qqtP '%s' %s", password,
			 filename);
		if (system(buff) == EXIT_SUCCESS) { // Flawfinder: ignore
			printf("Found the correct password: "
			       "%s\n", password);
			return 1;
		}
	} else {//        1234567890
		printf("\rPossible: %-70s\n", password);
	}
}

static void usage(const char *app, int ret)
{
	printf("%s [ -hvq ] [ -i <pattern> | -d <filename> ] [ -p <seconds> ]"
	       " <filename>\n", app);
	exit(ret);
}

static void sigalarm(int a)
{
	int c;
	static int last = 0;
	printf("\r%80s", "");

	printf("\rProgress (%'llu / %'llu = %6.2f %%, %'.0f c/s): ",
	       (long long unsigned)state->gencount,
	       (long long unsigned)state->total,
	       100*pow(10, log10(state->gencount) - log10(state->total)),
	       (float)(state->gencount-last)/(float)waitstatustime);
	for (c = 0; pwbuf[c] && c < 10; c++) {
		if (isalpha(pwbuf[c]) || isdigit(pwbuf[c])) {
			printf("%c", pwbuf[c]);
		} else {
			printf("_");
		}
	}
	fflush(stdout);
	last = state->gencount;
	alarm(waitstatustime);
}

int main(int argc, char **argv)
{
	char ebuf[PWGEN_MAXLEN_EBUF];
	char *charset;
	char *infile;
	int length = 5;
	int c;
	enum { null, dictionary, incremental } generator = null;
	setlocale(LC_NUMERIC, "");
	crc_init();
	
	while ((c = getopt(argc, argv, "hi:d:vqp:")) != EOF) {
		switch (c) {
		case 'h':
			usage(argv[0], 0);
			break;
		case 'i':
			generator = incremental;
			charset = optarg;
			break;
		case 'd':
			generator = dictionary;
			charset = "biglist.dic";
			break;
		case 'v':
			verbose++;
			break;
		case 'q':
			waitstatustime = 0;
			quiet = 1;
			break;
		case 'p':
			waitstatustime = atoi(optarg);
			if (waitstatustime > 3600) {
				fprintf(stderr, "Oh come on, you can spare "
					" than a few microseconds an hour.\n"
					"Use a lower -p value.\n");
				usage(argv[0], 1);
			}
			if (!waitstatustime) {
				fprintf(stderr, "Use -q instead of -p 0\n");
				usage(argv[0], 1);
			}
			break;
		default:
			fprintf(stderr, "FIXME\n");
			usage(argv[0], 1);
		}
	}

	verbose_report_machine = 0;
	if (verbose >= 1) {
		verbose_report_machine = 1;
	}

	if (argc != optind + 1) {
		usage(argv[0], 1);
	}
	infile = argv[optind];

	signal(SIGALRM, sigalarm);

	switch(generator) {
	case dictionary:
		length = 128;
		pwbuf = (char*)malloc(length + 1);
		state = pwgen_dict_init(charset, length, pwbuf, ebuf);
		break;
	case incremental:
		length = pwgen_inc_countcharsetlen(charset, ebuf);
		pwbuf = (char*)malloc(length+1);
		state = pwgen_inc_init(charset, length, pwbuf, ebuf);
		break;
	case null:
		usage(argv[0], 1);
		break;
	}
	
	if (!state) {
		fprintf(stderr, "Err: %s\n", ebuf);
		return 1;
	}

	if (zipbrute_load(infile)) {
		return 1;
	}

	if (verbose) {
		int c;
		printf("Data from first file: ");
		for (c = 0; c < 12; c++) {
			printf("%.2x ", (unsigned char)filedata[0][c]);
			fflush(stdout);
		}
		printf("\n");
		fflush(stdout);
	}
	alarm(waitstatustime);
	zipbrute_crack(state, pwbuf);
	alarm(0);
	if (!quiet) {
		printf("\n");
	}
	state->free(&state);

	return 0;
}
