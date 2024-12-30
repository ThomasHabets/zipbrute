// $Id: pwgen_inc.c,v 1.9 2001/06/24 02:07:58 thomas Exp $

#include <malloc.h>
#include <string.h>

#include "pwgen.h"

static void pwgen_inc_free(pwgen_handle_t **_state);

static const char *regex_dot = {
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"0123456789"
};

static int max(int a, int b)
{
	return (a > b) ? a : b;
}

int pwgen_inc_countcharsetlen(const char *charset, char *ebuf)
{
	int ret = 0;
	enum { normal, choosechar, escnormal, escchoosechar } stat = normal;

	for (; *charset; charset++) {
		switch (stat) {
		case normal:
			switch(*charset) {
			case '[':
				stat = choosechar;
				ret++;
				break;
			case '\\':
				stat = escnormal;
				break;
			default:
				ret++;
				break;
			}
			break;
		case escnormal:
			ret++;
			stat = normal;
			break;
		case choosechar:
			switch(*charset) {
			case ']':
				stat = normal;
				break;
			case '\\':
				stat = escchoosechar;
				break;
			}
			break;
		case escchoosechar:
			stat = choosechar;
			break;
		}
	}
	if (stat != normal) {
		snprintf(ebuf, PWGEN_MAXLEN_EBUF,
			 "Parse error reading charset: "
			 "End of charset while not in normal "
			 "mode");
	}
	return ret;
}

void dealloc_charsets(pwgen_inc_state_t* state) {
  int c;
  for (c = 0; c < state->len; c++) {
    if (state->charset[c]) {
      free(state->charset[c]);
    }
  }
}

/*
 * Todo: FIXME
 *   More regex handling (like '\' and stuff) (extract to other functions?)
 */
static int pwgen_inc_parsecharset(pwgen_inc_state_t *state,
				  const char *charset, int len, char *ebuf)
{
	int c, pwpos;
	enum { normal, choosechar, choosechar_range  } stat = normal;
	char rangefrom;

	memset(state->charset, 0, sizeof(char*) * len);

	pwpos = 0;
	stat = normal;
	for (c = 0; ; c++) {
		int c2;
		/*
		 * If reached end of charset, do sanity check and exit loop
		 */
		if (!charset[c]) {
			if (stat != normal) {
				snprintf(ebuf, PWGEN_MAXLEN_EBUF,
					 "Parse error reading charset: "
					 "End of charset while not in normal "
					 "mode");
				dealloc_charsets(state);
				return 0;
			}
			if (pwpos != len) {
				snprintf(ebuf, PWGEN_MAXLEN_EBUF,
					 "Parse error reading charset: "
					 "Too few characters");
				dealloc_charsets(state);
				return 0;
			}
			break;
		}
		/*
		 * if too long a charset, complain
		 */
		if (pwpos >= len) {
			snprintf(ebuf, PWGEN_MAXLEN_EBUF,
				 "Parse error reading charset: "
				 "Too many characters (%d >= %d)", pwpos, len);
			dealloc_charsets(state);
			return 0;
		}
		/*
		 * if it's a new pwpos, allocate buffer
		 */
		if (!state->charset[pwpos]) {
			if (!(state->charset[pwpos] = 
			      malloc(sizeof(char*)*max(strlen(charset), 
						       strlen(regex_dot))+1))){
				// FIXME (dealloc above)
				dealloc_charsets(state);
				snprintf(ebuf, PWGEN_MAXLEN_EBUF,
					 "Out of memory");
				return 0;
			}
			memset(state->charset[pwpos], 0,
			       max(strlen(charset), strlen(regex_dot)) + 1);
		}
//		printf("Parsing %c\n", charset[c]);
		switch (stat) {
		case normal:
			if (charset[c] == '[') {
				stat = choosechar;
			} else if (charset[c] == '.') {
				strcpy(state->charset[pwpos], regex_dot);
				pwpos++;
			} else {
				strncpy(state->charset[pwpos], &charset[c], 1);
				pwpos++;
			}
#if 0
			printf("Done %d %s\n", pwpos-1,
			       state->charset[pwpos-1]);
			fflush(stdout);
#endif
			break;
		case choosechar:
			if (charset[c] == ']') {
				stat = normal;
				pwpos++;
			} else if (charset[c] == '-') {
				if (!state->charset[pwpos][0]) {
					dealloc_charsets(state);
					snprintf(ebuf, PWGEN_MAXLEN_EBUF,
						 "Parse error reading charset:"
						 " not a range");
					return 0;
				}
				rangefrom = state->charset[pwpos]
					[strlen(state->charset[pwpos])-1];
				stat = choosechar_range;
			} else {
				strncat(state->charset[pwpos], &charset[c], 1);
			}
			break;
		case choosechar_range: {
			int dir = 1;
//			printf("From %c to %c\n", rangefrom, charset[c]);
			if (rangefrom > charset[c]) { // rising
				dir = -1;
			}
			for (c2 = rangefrom+dir; c2 != charset[c]; c2+=dir) {
				strncat(state->charset[pwpos], (char*)&c2, 1);
			}
			strncat(state->charset[pwpos], &charset[c], 1);
			stat = choosechar;
			break;
		}
		default:
			snprintf(ebuf, PWGEN_MAXLEN_EBUF,
				 "Internal error (state invalid)");
			return 0;
		}
	}
#if 0
	printf("Pre\n");
	fflush(stdout);
	for (c = 0; c < len; c++) {
		printf("Charset(%d): %s\n", state->charset[pwpos]);
		fflush(stdout);
	}
	printf("Post\n");
	fflush(stdout);
#endif
	return 1;
}

pwgen_handle_t *pwgen_inc_init(const char *charset, int len, char *password,
			       char *ebuf)
{
	pwgen_inc_state_t *state;
	int c;

	if (!(state = malloc(sizeof(pwgen_inc_state_t)))) {
		snprintf(ebuf, PWGEN_MAXLEN_EBUF, "Out of memory");
		return 0;
	}

	state->pwgen = pwgen_inc;
	state->free = pwgen_inc_free;
	state->gencount = 0;

	if (!(state->charset = malloc(sizeof(char*) * len))) {
		snprintf(ebuf, PWGEN_MAXLEN_EBUF, "Out of memory");
		return 0;
	}

	if (!pwgen_inc_parsecharset(state, charset, len, ebuf)) {
		// ebuf set by function
		free(state->charset);
		free(state);
		return 0;
	}

	state->len = len;
	if (!(state->charindex = malloc(len))) {
		free(state);
		snprintf(ebuf, PWGEN_MAXLEN_EBUF,
			 "Out of memory");
		return 0;
	}
	memset(state->charindex, 0, len);
	for (c = 0; c < len; c++) {
		password[c] = state->charset[c][state->charindex[0]];
	}
	password[len] = 0;

	state->total = 1;
	for (c = 0; c < len; c++) {
//		printf("Charset(%d) = %s\n", c, state->charset[c]);
		state->total *= strlen(state->charset[c]);
	}

	return (pwgen_handle_t*)state;
}

static void pwgen_inc_free(pwgen_handle_t **_state)
{
	pwgen_inc_state_t *state = (pwgen_inc_state_t*)*_state;
	int c;
	for (c = 0; c < state->len; c++) {
		free(state->charset[c]);
	}
	free(state->charset);
	free(state->charindex);
	free(*_state);
}

/*
 * pwgen_inc()
 *
 * Pre:
 *   _state is initilized with pwgen_inc_init() and password is last
 *   password returned from this function (or inited by pwgen_inc_init() for
 *   the first password.
 * Post:
 *   password is updated to the next one, _state is updated to reflect that
 * Return:
 *   The number of characters changed in password this call, 0 if no more
 *   passwords.
 */
int pwgen_inc(pwgen_handle_t *_state, char *password)
{
	int ptr;
	pwgen_inc_state_t *state = (pwgen_inc_state_t*)_state;
	int changecount = 1;

	ptr = state->len - 1;

	while (!(password[ptr]=state->charset[ptr][++state->charindex[ptr]])) {
		if (ptr == 0) {
			return 0;
		}
		changecount++;
		state->charindex[ptr] = 0;
		password[ptr] = state->charset[ptr][0];
		ptr--;
		//fflush(stdout);
	}
	state->gencount++;
	return changecount;
}
