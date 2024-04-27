// $Id: zipbrute.cc,v 1.1 2001/05/08 13:35:23 thomas Exp $

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <error.h>
#include <errno.h>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <netinet/in.h>

#include "zipbrute.h"

namespace zipfile {
	static u_int32_t magic;
	static u_int32_t end_magic;
	static u_int32_t multi_magic;
	static u_int32_t cdir_magic;
	static u_int32_t cdirend_magic;
};

static u_int8_t key3table[65536];

/*
 * Tests all combinations of length 'length'
 *
 * This is what will be converted to assemly when the time comes.
 *
 * Things to do before bare-metal-mode:
 * 1) Include the cracking functions and optimize them.
 */

u_int32_t asmkey[3];
u_int8_t asmkey3;
u_int32_t asmcrctab[256], asmcrcinvtab[256];
char asmcharset[255];
int asmfilecount;
char asmfiles[MAXFILES * 12];
char asmfileheaders[MAXFILES];

extern "C" int asmTestLength(int leng);

int zipbrute_c::TestLength(int length, const char *init_pass)
{
	int i;
#if 0
	strcpy(asmcharset, charset);
	asmfilecount = filecount;
	for (int y = 0; y < filecount; y++) {
		memcpy(&asmfiles[12 * y], files[y], 12);
		asmfileheaders[y] = fileheaders[y];
	}
//	asmTestLength(length);
	asmTestLength(length);
#else
	char charindex[length];

	current_password = new char[length+1];

	memset(current_password, charset[0], length);
	memset(charindex, 0, length);

	int cachelen = length - 1;
	bool badcache = true;
	u_int32_t cachekey[3];
	u_int8_t cachekey3;
	int c;
	int exitnow = 0;

	current_password[length] = 0;
	for (;;) {
		/*
		 * Set up key[]
		 */
		if (badcache) {
			key[0] = 0x12345678;
			key[1] = 0x23456789;
			key[2] = 0x34567890;
			for (c = 0; c < cachelen; c++) {
				u_int32_t temp;
				key[0] = CRC32(key[0], current_password[c]);
				key[1] = (key[1] + (key[0] & 0xff))
					* 134775813 + 1;
				key[2] = CRC32(key[2], (key[1] >> 24) & 0xff);
				temp = (key[2] & 0xffff) | 3;
				key3 = ((temp * (temp ^ 1)) >> 8) & 0xff;
			}
			cachekey[0] = key[0];
			cachekey[1] = key[1];
			cachekey[2] = key[2];
			cachekey3 = key3;
			badcache = false;
		} else {
			key[0] = cachekey[0];
			key[1] = cachekey[1];
			key[2] = cachekey[2];
			key3 = cachekey3;
		}
		u_int32_t temp;
		key[0] = CRC32(key[0], current_password[length-1]);
		key[1] = (key[1] + (key[0] & 0xff)) * 134775813 + 1;
		key[2] = CRC32(key[2], (key[1] >> 24) & 0xff);
		temp = (key[2] & 0xffff) | 3;
		key3 = ((temp * (temp ^ 1)) >> 8) & 0xff;
		hex(cout);

		/*
		 * Ok, key[] now is now set up correctly
		 */
#if 0
		cout << current_password << "\t"
		     << (unsigned int)key[0] << "\t"
		     << (unsigned int)key[1] << "\t"
		     << (unsigned int)key[2] << "\t"
		     << endl;
#endif
//		if (exitnow++) {exit(0); }

		/*
		 * Now it's time to check all the files
		 * NOTE that this loop is like 85-90 percent of execution time
		 */
		int file;
		int decC;
		for (file = 0; file < filecount; file++) {
			u_int8_t tkey3;
			u_int32_t tkey[3];
			
			tkey[0] = key[0];
			tkey[1] = key[1];
			tkey[2] = key[2];
			tkey3 = key3;
			
			// decrypt without saving results
			// NOTE: oboy, this is a nasty unroll :)

			for (decC = 0; decC < 11; decC++) {
				u_int32_t temp;
#if 0
				printf("%.2x\n", files[file][decC] ^ tkey3);
#endif
				tkey[0] = CRC32(tkey[0], files[file][decC] ^ tkey3);
				tkey[1] = (tkey[1] + (tkey[0] & 0xff)) * 134775813 + 1;
				tkey[2] = CRC32(tkey[2], (tkey[1] >> 24) & 0xff);
				temp = (tkey[2] & 0xffff) | 3;
				tkey3 = ((temp * (temp ^ 1)) >> 8) & 0xff;
//				tkey3 = key3table[tkey[2] & 0xffff]; // WTF? this is 4 secs slower?
#if 0
				printf("%.2x  %.8x  %.8x  %.8x  %.2x\n",
				       files[0][0],
				       tkey[0],
				       tkey[1],
				       tkey[2],
				       tkey3);
				printf("%x\n", tkey[0]);
				exit(0);
#endif
			}
			if (fileheaders[file] != (char)(files[file][11] ^ tkey3)) {
				break;
			}
#if 1
			if (file == 0) {
				printf("Byte: %x (%x)\n", (unsigned char)fileheaders[file],
				       (unsigned char)(files[file][11] ^ tkey3));
				printf("%.2x  %.8x  %.8x  %.8x  %.2x\n",
				       files[4][0],
				       tkey[0],
				       tkey[1],
				       tkey[2],
				       tkey3);
			}
#endif
		}

		/*
		 * no we do something about the potential hits
		 */
		if (file == filecount) {
			if (use_unzip) {
				char buff[1024];
				int status;
				sprintf(buff, "unzip -qqtP '%s' %s",
					current_password,
					filename);
				if (system(buff) == EXIT_SUCCESS) {
					cout << "Found the correct password: "
					     << current_password << endl;
					return 1;
				}
			} else {
				cout << "Possible: " << current_password << endl;
			}
		}

		/*
		 * phew, now that we checked the files, make a new password to
		 * try
		 */
		int ptr;
		ptr = length - 1;
		dont_touch_password = true;
		while (!(current_password[ptr] = charset[++charindex[ptr]])) {
			current_password[ptr] = charset[0];
			charindex[ptr] = 0;
			if (ptr <= cachelen) {
				badcache = true;
			}
			if (!ptr) {
				dont_touch_password = false;
				return 0;
			}
			ptr--;
		}
		dont_touch_password = false;
	}
#endif
}

int zipbrute_c::TestKey(const char *password)
{
	init_keys(password);
	return crackkey();
}

/*
 * contructor
 */
zipbrute_c::zipbrute_c(void)
{
	zipfile::magic = 0x504b0304;
	zipfile::end_magic = 0x504b0102;
	zipfile::multi_magic = 0x504b0708;
	zipfile::cdir_magic = 0x504b0102;
	zipfile::cdirend_magic = 0x504b0606;

	charset = "abcdefghijklmnopqrstuvwxyz1234";

	mkCrcTab();

	dont_touch_password = false;
	verbose_list_files = false;
	use_unzip = false;
}
/*
 * created crctable, only called once
 */
void zipbrute_c::mkCrcTab(void)
{
	unsigned int i, j, c;

	for(i = 0; i < 256; i++) {
		c = i;
		for(j = 0; j < 8; j++) {
			if(c & 1) {
				c = (c >> 1) ^ CRCPOLY;
			} else {
				c = (c >> 1);
			}
		}
		crctab[i] = c;
		crcinvtab[c >> 24] = (c << 8) ^ i;
		asmcrctab[i] = c;
		asmcrcinvtab[c >> 24] = (c << 8) ^ i;
	}
	for (int k = 0; k < 65536; k++) {
		u_int32_t t;
		t = k | 3;
		key3table[k] = ((t * (t ^ 1)) >> 8) & 0xff;
	}
}

/*
 * updates the globally stored keys
 */
void zipbrute_c::updateKeys(char plainbyte)
{
	u_int32_t temp;

	key[0] = CRC32(key[0], plainbyte);
	key[1] = (key[1] + (key[0] & 0xff)) * 134775813 + 1;
	key[2] = CRC32(key[2], (key[1] >> 24) & 0xff);
	temp = (key[2] & 0xffff) | 3;
	key3 = ((temp * (temp ^ 1)) >> 8) & 0xff;
}

/*
 * makes a keyset out of a password
 */
void zipbrute_c::init_keys(const char *pw)
{
	key[0] = 0x12345678;
	key[1] = 0x23456789;
	key[2] = 0x34567890;
	int len = strlen(pw);
	for (int c = 0; c < len; c++) {
		updateKeys(pw[c]);
	}
}

/*
 * decrypt data
 */
int zipbrute_c::decrypt(char *dst, const char *src, int len)
{
        int c;

        for (c = 0; c < len; c++) {
                dst[c] = src[c] ^ key3;
                updateKeys(dst[c]);
        }
        return c;
}

int zipbrute_c::encrypt(char *dst, const char *src, int len)
{
        int c;
        for (c = 0; c < len; c++) {
                dst[c] = src[c] ^ key3;
                updateKeys(src[c]);
        }
        return c;
}

int zipbrute_c::trypw(int file)
{
	char clear[12];
	char target;
	u_int16_t t;
	
	decrypt(clear, files[file], 11);

	t = key[2] | 2;
	target = files[file][11] ^ (char)(((u_int16_t) (t * (t^1)) >> 8));

	clear[11] = files[file][11] ^ key3;

#if 0 // DEBUG
	printf("Byte: %x (%x)\n", (unsigned char)fileheaders[11],
	       (unsigned char)target);
#endif
	return (fileheaders[file] == target);
}

int zipbrute_c::crackkey(void)
{
	int c;
	u_int32_t tkey[3];
	u_int8_t tkey3;

	tkey[0] = key[0];
	tkey[1] = key[1];
	tkey[2] = key[2];
	tkey3 = key3;
	for (c = 0; c < filecount; c++) {
		key[0] = tkey[0];
		key[1] = tkey[1];
		key[2] = tkey[2];
		key3 = tkey3;
		if (!trypw(c)) {
			return 0;
		}
//		cout << "File ok: " << c << endl;
	}
	return 1;
}

void zipbrute_c::loadfile(const char *_filename)
{
	FILE *fp;
	u_int32_t itmp;
	zipfile::local_t lf;
	int c;

	filename = strdup(_filename);

	if (!(fp = fopen(filename, "rb"))) {
		throw (string)"fopen(): " + strerror(errno);
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
			
			fread(&itmp, sizeof(u_int32_t), 1, fp);
			itmp = htonl(itmp);
			
			if (itmp == zipfile::multi_magic) {
				fseek(fp, 12, SEEK_CUR);
				got_a_file = 0;
			}
		} while (!got_a_file);

		if (itmp == zipfile::end_magic) {
			break;
		}
		if (itmp != zipfile::magic) {
			hex(cout);
			cout << itmp << endl;
			throw (string)"Bad voodoo in zipfile";
		}
		if (!fread(&lf, sizeof(zipfile::local_t), 1, fp)) {
			throw (string)"fread(): " + strerror(errno);
		}
		// FIXME: warn if flen > 128
		fread(buffer, lf.flen, 1, fp);
		buffer[lf.flen] = 0;

		if (verbose_list_files) {
			printf("%15d    %s\n", lf.csize, buffer);
		}
		
		fseek(fp, lf.extralen, SEEK_CUR);
		fread(files[c], 12, 1, fp);
		fseek(fp, lf.csize - 12, SEEK_CUR);
	}
	filecount = c;
	if (fclose(fp)) {
		fprintf(stderr, "Warning: unable to fclose() zipfile\n");
	}
}

int zipbrute_c::GetCurrentPassword(char *pw)
{
	if (dont_touch_password) {
		return 0;
	}
	strcpy(pw, current_password);
	return 1;
}
