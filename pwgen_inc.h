// $Id: pwgen_inc.h,v 1.5 2001/06/24 02:07:58 thomas Exp $


typedef struct pwgen_inc_state_s {
	PWGEN_INITIAL_STATE_STUFF
	char **charset;
	char *charindex;
	int len;
	int cachelen;
} pwgen_inc_state_t;

pwgen_handle_t *pwgen_inc_init(const char *charset, int len, char *password,
			       char *ebuf);
int pwgen_inc(pwgen_handle_t *_state, char *password);

/*
 * generator-specific stuff
 */
int pwgen_inc_countcharsetlen(const char *charset, char *ebuf);
