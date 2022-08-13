// $Id: pvm_main.c,v 1.3 2002/03/11 14:42:15 thomas Exp $

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <pvm3.h>

#include "pwgen.h"
#include "crc.h"
#include "zipbrute.h"

static int numt;
static int verbose_slave_print_charset = 0;
static int verbose_master_print_slave_report = 0;

unsigned int waitstatustime = 1;

/*
 * atexit() function for master and slave
 */
static void closedown(void)
{
//	printf("Shutting down pvm...");
	fflush(stdout);
	pvm_exit();
//	printf("done\n");
}

/*
 * SIGINT handler
 */
static void inthandler(int k)
{
	closedown();
	exit(1);
}


static char *delegate_buf;
static pwgen_handle_t *delegate_state;
static char *getcharset(void)
{
	if (!delegate_state->pwgen(delegate_state, delegate_buf)) {
		return 0;
	}
	if (!delegate_buf[0]) { // FIXME: this is an odd check
		return 0;
	}
	return delegate_buf;
}

static void mainloop(const char *global_charset, int len,int numt, int *tids)
{
	int delegated_count = 0;
	int c;
	char *charset;
	char ebuf[PWGEN_MAXLEN_EBUF];
	int *stats;
	int totalblocks = 0;

	/*
	 * prepare delegate buffer
	 */
	if (!(delegate_buf = malloc(len + 1))) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	if (!(delegate_state = pwgen_inc_init(global_charset,
					      len,
					      delegate_buf,
					      ebuf))) {
		fprintf(stderr, "Fuh");
		exit(1);
	}

	/*
	 * zero statistics
	 */
	if (!(stats = (int*)malloc(sizeof(int) * numt))) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memset(stats, 0, sizeof(int) * numt);

	/*
	 * send initial charset
	 */
	pvmzipbrute_send(numt, tids);
	for (c = 0; c < numt - 1; c++) {
		pvmzipbrute_sendblock(delegate_buf,tids[c]);
		delegated_count++;
		if (!getcharset()) {
			break;
		}
	}
	pvmzipbrute_sendblock(delegate_buf,tids[c]);
	delegated_count++;

	/*
	 * main loop
	 */
	for (;delegated_count;) {
		int bufid,tid;
		char buf[128];
		int slavenumber;

		bufid = pvm_recv(-1, PVM_PASSWORD);
		pvm_upkstr(buf);
		
		pvm_bufinfo(bufid,
			    NULL,
			    NULL,
			    &tid);
		slavenumber = 0;
		for (c = 0; c < numt; c++) {
			if (tids[c] == tid) {
				slavenumber = c;
				break;
			}
		}
		
		delegated_count--;

		if (buf[0]) {
			printf("Password found by node %x: %s\n",
			       tid, buf);
			delegated_count++;
			continue;
		} else {
			stats[slavenumber]++;
			totalblocks++;
			if (verbose_master_print_slave_report) {
				printf("Node %x wants more\n", tid);
			}
		}

		/*
		 * Generate a charset
		 */
		if (charset = getcharset()) {
			pvmzipbrute_sendblock(charset, tid);
			delegated_count++;
		} else {
			if (verbose_master_print_slave_report) {
				printf("Everything already delegated"
				       "\n");
			}
			pvmzipbrute_sendblock("", tid);
		}
		if (verbose_master_print_slave_report) {
			printf("---\n");
		}
//		printf("%d\n", delegated_count);
	} // mainloop

	printf("\n");
	printf("        Slave   Hostname        Blocks          Percent\n");
	printf("        -----------------------------------------------\n");
	for (c = 0; c < numt; c++) {
		printf("        %5d   %s           %6d         %5.2f %%\n",
		       c,
		       "FIXME",
		       stats[c],
		       (float)stats[c] / (float)totalblocks * 100.0);
	}
	printf("\t-----------------------------------------------\n");
	printf(" Total:\t%5d \t\t\t%6d \t       %5.2f %%\n",
	       numt, totalblocks, 100.0);
	printf("\n");
	free(delegate_buf);
	delegate_state->free(&delegate_state);
}

/*
 *
 */
static void runmaster(const char *zipfile, const char *global_charset, int len)
{
	char *agv[] = {
		"pvmzipbrute",
		"-s",
		NULL, // placeholder
		NULL
	};
	char ch;
	int tids[10];
	int delegated_count;
	int nhost;
	int narch;
	int c;
	struct pvmhostinfo *hostinfo;
	
	/*
	 * set up arg to slave
	 */
	if (!(agv[2] = malloc(64))) { // ints can't get bigger
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	snprintf(agv[2],64, "%d",len);

	/*
	 * load the zipfile (good to check for fail before pvm_spawn())
	 */
	if (zipbrute_load(zipfile)) {
		fprintf(stderr, "Unable to load zipfile\n");
		exit(1);
	}
#if 0
	for (c = 0; c < 12; c++) {
		printf("%.2x ", (unsigned char)filedata[0][c]);
		fflush(stdout);
	}
	printf("\n");
	fflush(stdout);
#endif
	/*
	 * get number of slaves in cluster and spawn them
	 */
	pvm_config(&nhost, &narch, &hostinfo);
	
//		pvm_catchout(stdout);
	numt = pvm_spawn("/home/thomas/cvs/cryptanalysis/"
			 "zipbrute/pvmzipbrute",
			 agv, // char**argv
			 PvmTaskDefault,//int flag,
			 NULL,//char *where,
			 nhost,//int ntask,
			 tids);//int *tids
	printf("Spawned %d slaves on %d hosts\n", numt, nhost);
	mainloop(global_charset, len, numt, tids);

	free(agv[2]);
}

/*
 *
 */
int main(int argc, char **argv)
{
	pwgen_handle_t *state;
	char *pwbuf;
	char ebuf[PWGEN_MAXLEN_EBUF];
	char *charset;
	int length = 0;
	int c;
	enum { master, slave } mode = master;

	pvm_mytid();

	atexit(closedown);
	crc_init();
	
	/*
	 *
	 */
	while ((c = getopt(argc, argv, "hs:")) != EOF) {
		switch (c) {
		case 's':
			mode = slave;
			length = atoi(optarg);
			break;
		default:
			break;
		}
	}
	
	/*
	 * master operation
	 */
	if (mode == master) {
		int length;
		char *charset;
		char *file;

		if (optind + 3 != argc) {
			fprintf(stderr, "USAGE!\n");
			return 1;
		}

		file = argv[optind++];
		length = atoi(argv[optind++]);
		charset = argv[optind++];

		runmaster(file, charset, length);
	} else {
		/*
		 * slave operation
		 */
		if (!length) {
			fprintf(stderr, "Length was 0!\n");
			exit(1);
		}
		pwbuf = (char*)malloc(length + 1);
		charset = malloc(128);
		if (pvmzipbrute_load()) {
			return 1;
		}
		for (;;) {
			int bufid;
			
			bufid = pvm_recv(-1, PVM_CHARSET);
			pvm_upkstr(charset);
			
			if (!charset[0]) {
				printf("Got dismissed\n");
				break;
			}

			if (verbose_slave_print_charset) {
				printf("Trying charset %s\n", charset);
				fflush(stdout);
			}
			printf("");
			if (!(state = pwgen_inc_init(charset,
						     length,
						     pwbuf,
						     ebuf))) {
				fprintf(stderr, "Err: %s\n", ebuf);
				fflush(stderr);
				return 1;
			}
			zipbrute_crack(state, pwbuf);
			state->free(&state);
			
			pvm_initsend(PvmDataDefault);
			pvm_pkstr("");
			pvm_send(pvm_parent(), PVM_PASSWORD);
		}
	}
	return 0;
}
