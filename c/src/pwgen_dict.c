// $Id: pwgen_dict.c,v 1.3 2001/06/23 17:57:47 thomas Exp $

#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "pwgen.h"

pwgen_handle_t *pwgen_dict_init(const char *charset, int len, char *password,
			       char *ebuf)
{
	pwgen_dict_state_t *state;
	struct stat st;
	char *buffer;
	if (!(state = malloc(sizeof(pwgen_inc_state_t)))) {
		snprintf(ebuf, PWGEN_MAXLEN_EBUF, "Out of memory");
		return 0;
	}

	state->pwgen = pwgen_dict;
	state->free = pwgen_dict_free;
	state->gencount = 0;

	if (!(state->file = fopen(charset, "rt"))) {
		free(state);
		snprintf(ebuf, PWGEN_MAXLEN_EBUF,
			 "Unable to open file %s", charset);
		return 0;
	}

	state->maxlen = 128;

	if (!(buffer = (char*)malloc(state->maxlen+1))) {
		free(state);
		snprintf(ebuf, PWGEN_MAXLEN_EBUF,
			 "Unable to allocate %d bytes", state->maxlen+1);
		return 0;
	}

	if (!quiet) {
		printf("Counting lines in file...");
		fflush(stdout);
	}
	state->total = 0;
	for (;!feof(state->file);) {
		state->total++;
		fgets(buffer, state->maxlen + 1, state->file);
	}
	fseek(state->file, 0, SEEK_SET);
	printf("done\n");
	
	if (0 != fstat(fileno(state->file), &st)) {
		free(state);
		snprintf(ebuf, PWGEN_MAXLEN_EBUF,
			 "Uh? Failed to fstat() open file %d",
			 fileno(state->file));
		return 0;
	}
	
	if (!st.st_size) {
		free(state);
		snprintf(ebuf, PWGEN_MAXLEN_EBUF,
			 "Empty dictionary file");
		return 0;
	}

	return (pwgen_handle_t*)state;
}

// FIXME
void pwgen_dict_free(pwgen_handle_t **_state)
{
	pwgen_dict_state_t *state = (pwgen_dict_state_t*)*_state;
	fclose(state->file);
	free(*_state);
}

/*
 * Todo: FIXME
 *   Make this data-recursive instead of code-recursive
 */
int pwgen_dict(pwgen_handle_t *_state, char *password)
{
	pwgen_dict_state_t *state = (pwgen_dict_state_t*)_state;
	int n;

	password[0] = 0;
	while (!password[0]) {
		if (feof(state->file)) {
			return 0;
		}
		fgets(password, state->maxlen, state->file);
		n = strlen(password);
		while (n && strchr("\n\r", password[n - 1])) {
			password[--n] = 0;
		}
	}
	state->gencount++;
	return n;
}
