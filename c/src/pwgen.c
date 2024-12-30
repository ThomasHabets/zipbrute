// $Id: pwgen.c,v 1.5 2001/06/23 17:57:47 thomas Exp $

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#include "pwgen.h"

int quiet = 0;

int main(int argc, char **argv)
{
	char *pwbuf;
	char *charset;
	int change = -1;
	pwgen_handle_t *state;
	unsigned int length = 0;
	int n,c;
	char ebuf[PWGEN_MAXLEN_EBUF];
	enum { null, incremental, dictionary } generator = null;
	
	while ((c = getopt(argc, argv, "hd:i:")) != EOF) {
		switch (c) {
		case 'd':
			length = atoi(optarg);
			generator = dictionary;
			break;
		case 'i':
			length = atoi(optarg);
			generator = incremental;
			break;
		default:
			break;
		}
	}
	if (generator == null || optind + 1 != argc) {
	  fprintf(stderr, "Need to specify a generator and another arg\n");
		return 1;
	}

	charset = argv[optind];

	if (!(pwbuf = (char*)malloc(length))) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	switch (generator) {
	case incremental:
		state = pwgen_inc_init(charset, length, pwbuf, ebuf);
		break;
	case dictionary:
		state = pwgen_dict_init(charset, length, pwbuf, ebuf);
		break;
	default:
	        fprintf(stderr, "Internal error: unknown generator %d\n", generator);
		break;
	}
	if (!state) {
		fprintf(stderr, "pwgen_init(): %s\n", ebuf);
		return 1;
	}

	n = 0;
	do {
		n++;
//		printf("%3d> %s\n", change, pwbuf);
	} while((change = state->pwgen(state, pwbuf)));

	state->free(&state);

	printf("Generated %d words\n", n);

	return 0;
}
