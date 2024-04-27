// $Id: main.cc,v 1.1 2001/05/08 13:35:23 thomas Exp $

#include <iostream>
#include <cstdio>
#include <string>
#include <signal.h>
#include <unistd.h>
#include "zipbrute.h"

static void save_and_exit(int sig);
static zipbrute_c zb;

static struct sigaction sigact = {
	save_and_exit
};

static void setsignalhandlers(void)
{
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGALRM, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
}

static void save_and_exit(int sig)
{
	char buffer[ZIPBRUTE_PWMAXLEN+1];

	if (!zb.GetCurrentPassword(buffer)) {
		setsignalhandlers();
		alarm(1);
		return;
	}
	cout << "Last processed password was " << buffer << endl;
	if (sig != SIGHUP) {
		cout << "Exiting gracefully\n";
		exit(1);
	}
	setsignalhandlers();
}

static void usage(int ret)
{
	cout << "Usage: zipbrute <zipfile> <pw length>\n";
	exit(ret);
}


int main(int argc, char **argv)
{
	int c;
	
	int min = 1, max = 3;

	while ((c = getopt(argc, argv, "hlum:M:")) != EOF) {
		switch(c) {
		case 'h':
			usage(0);
		case 'l':
			zb.verbose_list_files = true;
			break;
		case 'm':
			min = atoi(optarg);
			if (min < 1 || min > ZIPBRUTE_PWMAXLEN) {
				cerr << "Error: Minimum value must be between 1 and "
				     << ZIPBRUTE_PWMAXLEN << endl;
				return 1;
			}
			break;
		case 'M':
			max = atoi(optarg);
			if (max < 1 || max > ZIPBRUTE_PWMAXLEN) {
				cerr << "Error: Maximum value must be between 1 and "
				     << ZIPBRUTE_PWMAXLEN << endl;
				return 1;
			}
			break;
		case 'c':
			cout << "Charsetting not done yet\n";
			break;
		case 'u':
			zb.use_unzip = true;
			break;
		default:
			usage(1);
		}
	}
	if (min > max) {
		cerr << "Error: minimum password length is higher than maximum\n";
		return 1;
	}
	if (optind >= argc) {
                usage(1);
        }
	setsignalhandlers();
	try {
		zb.loadfile(argv[optind]);
//		printf("from %d to %d\n", min, max);
		for (int length = min; length <= max; length++) {
//			printf("Loop %d\n", length);
			zb.TestLength(length);
		}
	} catch(string msg) {
		cout << msg << endl;
	}

	return 0;
}
