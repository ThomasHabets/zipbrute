// $Id: zipbrute.c,v 1.4 2001/10/14 13:35:45 thomas Exp $

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "zipbrute.h"
#include "pwgen.h"
#include "crc.h"
#include "zip.h"

static u_int32_t key[3];
static u_int8_t key3;
static u_int8_t tkey3;
static u_int32_t tkey[3];

#define USE_CACHE 1

static void updatekeys(u_int8_t byte)
{
	u_int32_t temp;
	key[0] = CRC32(key[0], byte);
	key[1] = (key[1] + (key[0] & 0xff)) * 134775813 + 1;
	key[2] = CRC32(key[2], (key[1] >> 24) & 0xff);
	temp = (key[2] & 0xffff) | 3;
	key3 = ((temp * (temp ^ 1)) >> 8) & 0xff;
}

static void updatetkeys(u_int8_t byte)
{
	u_int32_t temp;
	tkey[0] = CRC32(tkey[0], byte);
	tkey[1] = (tkey[1] + (tkey[0] & 0xff)) * 134775813 + 1;
	tkey[2] = CRC32(tkey[2], (tkey[1] >> 24) & 0xff);
	temp = (tkey[2] & 0xffff) | 3;
	tkey3 = ((temp * (temp ^ 1)) >> 8) & 0xff;
}

int zipbrute_crack_backend(pwgen_handle_t *state, char *password)
{
	int change = 99;
	int n = 0;
	int cachelen;
	int cachepos;
	int c;
	int file;
	int decC;
	u_int32_t ckey[3];
	u_int8_t ckey3;
	int len;

	do { // 92.5% of execution is in this loop
		n++;
#if USE_CACHE
		if (__builtin_expect(change == 1, 1)) {
			key[0] = ckey[0];
			key[1] = ckey[1];
			key[2] = ckey[2];
			key3 = ckey3;
		  } else {
			key[0] = 0x12345678;
			key[1] = 0x23456789;
			key[2] = 0x34567890;
			for (c = 0; c < password[c+1]; c++) {
				updatekeys(password[c]);
			}
			cachepos = c;
			ckey[0] = key[0];
			ckey[1] = key[1];
			ckey[2] = key[2];
			ckey3 = key3;
		}
		updatekeys(password[cachepos]);
#else
		key[0] = 0x12345678;
		key[1] = 0x23456789;
		key[2] = 0x34567890;
		for (c = 0; c < password[c]; c++) {
			updatekeys(password[c]);
		}
#endif
		/*
		 * key[] and key3 is now set up properly for the key
		 */	
		/*
		 * Now it's time to check all the files
		 * NOTE that this loop is like 85-90 percent of execution time
		 */
		for (file = 0; file < filecount; file++) {
			int d;
			tkey[0] = key[0];
			tkey[1] = key[1];
			tkey[2] = key[2];
			tkey3 = key3;

			// 23.9% of execution time is in this loop
			for (decC = 0; decC < 11; decC++) {
				updatetkeys(filedata[file][decC] ^ tkey3);
			}

			if (fileheaders[file]
			    != (char)(filedata[file][11] ^ tkey3)) {
				break;
			}
		}
		if (file == filecount) {
			if (zipbrute_possible(state, password)) {
				//return 1;
			}
		}
	} while ((change = state->pwgen(state, password)));
	return 0;
}
