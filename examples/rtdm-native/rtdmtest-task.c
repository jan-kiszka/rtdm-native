/*
 * Copyright (C) 2006 Jan Kiszka <jan.kiszka@web.de>.
 *
 * Xenomai is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>

#include <rtdm/rttesting.h>

#define TESTCASE_CREATE   1
#define TESTCASE_DESTROY  2
#define TESTCASE_SET_PRIO 4

static void sighand(int signal)
{
    printf("%s: signal=%d\n", __FUNCTION__, signal);
}

int main(int argc, char *argv[])
{
    char devname[RTDM_MAX_DEVNAME_LEN];
    int benchdev_no = 0;
    int verbose = 0, priority = 40, period_us = 100000;
    int benchdev;
    int testcases = 0;
    char c;
    struct rttst_rtdmtest_config config;
 
    signal(SIGINT, sighand);
    signal(SIGTERM, sighand);
    signal(SIGHUP, sighand);
    signal(SIGALRM, sighand);

    while ((c = getopt(argc,argv,"vcdp:P:")) != EOF)
        switch (c) {
	case 'v':
	    verbose = 1;
	    break;

        case 'c':
		testcases |= TESTCASE_CREATE;
		break;

	case 'd':
	    testcases |= TESTCASE_DESTROY;
	    break;

	case 'p':
	    priority = atoi(optarg);
	    testcases |= TESTCASE_SET_PRIO;
	    break;

	case 'P':
	    period_us = atoi(optarg);
	    break;

	default:
	    fprintf(stderr, "usage: irqbench [options]\n"
		    "  [-c <count>]  # test count\n"
		    "  [-T <test_duration_seconds>] # default=0, so ^C to end\n");
	    exit(2);
	}


    mlockall(MCL_CURRENT | MCL_FUTURE);

    snprintf(devname, RTDM_MAX_DEVNAME_LEN, "/dev/rttest%d", benchdev_no);
    benchdev = open(devname, O_RDWR);
    if (benchdev < 0) {
	perror("failed to open benchmark device");
	return 1;
    }

    config.priority = priority;
    config.timeout = (nanosecs_rel_t)period_us * 1000;

    if (testcases & TESTCASE_CREATE) {
	    if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_TASK_CREATE, &config) < 0)
	    perror("ioctl TASK_CREATE");
    } else if (testcases & TESTCASE_SET_PRIO) {
	    if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_TASK_SET_PRIO, &config) < 0)
		    perror("ioctl TASK_SET_PRIO");
    }

    if (testcases & TESTCASE_DESTROY) {
	if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_TASK_DESTROY) < 0)
	    perror("ioctl TASK_DESTROY");
    }

    if (benchdev)
	close(benchdev);

    return 0;
}
