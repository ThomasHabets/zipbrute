// $Id: pvm_lib.c,v 1.2 2001/06/23 17:57:47 thomas Exp $
#include <stdio.h>

#include <pvm3.h>

#include "zipbrute.h"

const int pvm = PVM;

int zipbrute_possible(pwgen_handle_t *state, const char *password)
{
	pvm_initsend(PvmDataDefault);
	pvm_pkstr((char*)password);
	pvm_send(pvm_parent(), PVM_PASSWORD);
}

int pvmzipbrute_load(void)
{
	int bufid;
	char buf[128];
	int c;

	bufid = pvm_recv(-1, PVM_FILECOUNT);
	pvm_upkint(&filecount, 1, 1);

	for (c = 0; c < filecount; c++) {
		bufid = pvm_recv(-1, PVM_FILEDATA);
		pvm_upkbyte(buf, 14, 1);
		fileheaders[buf[0]] = buf[1];
		memcpy(filedata[buf[0]], &buf[2], 12);
	}
#if 0
	for (c = 0; c < 12; c++) {
		printf("%.2x ", (unsigned char)filedata[0][c]);
		fflush(stdout);
	}
	printf("\n");
	fflush(stdout);
#endif
	return 0;
}

int pvmzipbrute_send(int ntask, int *tids)
{
	char buf[128];
	int c;

	pvm_initsend(PvmDataDefault);
	pvm_pkint(&filecount, 1, 1);
	pvm_mcast(tids, ntask, PVM_FILECOUNT);

	for (c = 0; c < filecount; c++) {
		pvm_initsend(PvmDataDefault);

		buf[0] = c;
		buf[1] = fileheaders[c];
		memcpy(&buf[2], filedata[c], 12);

		pvm_pkbyte(buf, 14, 1);
		pvm_mcast(tids, ntask, PVM_FILEDATA);
	}
}

void pvmzipbrute_sendblock(char *charset, int tid)
{
	pvm_initsend(PvmDataDefault);
	pvm_pkstr(charset);
	pvm_send(tid, PVM_CHARSET);

	if (verbose) {
		if (charset[0]) {
			printf("Delegated %s to %x\n", charset, tid);
		} else {
			printf("Dismissed %x\n", tid);
		}
	}
}
