// $Id: pwgen_dict.h,v 1.1 2001/04/22 23:15:43 thomas Exp $

#include <stdio.h>

typedef struct pwgen_dict_state_s {
	PWGEN_INITIAL_STATE_STUFF
	FILE *file;
	int maxlen;
} pwgen_dict_state_t;

pwgen_handle_t *pwgen_dict_init(const char *charset, int len, char *password,
			       char *ebuf);
void pwgen_dict_free(pwgen_handle_t **_state);
int pwgen_dict(pwgen_handle_t *_state, char *password);
