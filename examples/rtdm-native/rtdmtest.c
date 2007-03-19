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

#define RTDMTEST_MUTEX_THREADS 16

#define TIMER_RELTIME 	0

#define TESTCASE_EVENT  1
#define TESTCASE_SEM    2
#define TESTCASE_MUTEX  4

static int verbose;
static int testcases;
static int testcount;
static int testseqcount;
static int testmutex;
static int testmutexdelay;

static int benchdev;
static int terminate;
static int with_timeout;
static int timeout_us;
static int delay_us = 1000;
static unsigned long event_signals, event_waits;
static unsigned long sem_ups, sem_downs;
static unsigned long mutex_tests[RTDMTEST_MUTEX_THREADS];

static void sighand(int signal)
{
    printf("%s: signal=%d\n", __FUNCTION__, signal);
    terminate = 1;
}

static void *event_wait_thread(void *arg)
{
    struct sched_param param = { .sched_priority = (int)arg };
    struct rttst_rtdmtest_config config, *pconfig = NULL;
    int cmd;

    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    config.seqcount = testseqcount;
    config.timeout = timeout_us * 1000;
    if (timeout_us) {
	cmd = RTTST_RTIOC_RTDMTEST_EVENT_TIMEDWAIT;
	pconfig = &config;
    } else
	cmd = RTTST_RTIOC_RTDMTEST_EVENT_WAIT;

    if (verbose)
	printf("Starting %s\n", __FUNCTION__);

    while (!terminate) {
        if (ioctl(benchdev, cmd, pconfig) < 0) {
	    if (errno == ETIMEDOUT) {
		if (verbose)
		    printf("Timeout...\n");
                continue;
	    }
	    else if (errno == EWOULDBLOCK) {
		if (verbose)
		    printf("Would block...\n");
		usleep(100);
                continue;
	    }
	    perror("ioctl EVENT_WAIT");
	    terminate = 1;
	    break;
	}
	event_waits++;
    }

    param.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);

    printf("Exiting %s\n", __FUNCTION__);

    return NULL;
}

static void *event_signal_thread(void *arg)
{
    struct sched_param param = { .sched_priority = (int)arg };
    struct timespec interval;
    int cmd = RTTST_RTIOC_RTDMTEST_EVENT_SIGNAL;

    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    if (delay_us > 0) {
	interval.tv_sec = delay_us / 1000000;
	interval.tv_nsec = (delay_us % 1000000) * 1000;
    }
    if (verbose)
	printf("Starting %s\n", __FUNCTION__);

    while (!terminate) {

	if (delay_us > 0)
	    nanosleep(&interval, NULL);
	    //clock_nanosleep(CLOCK_REALTIME, TIMER_RELTIME, &interval, NULL);

	if (testcount && event_signals >= testcount) {
	    if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_EVENT_DESTROY) < 0)
		perror("ioctl EVENT_DESTROY");
            break;
	} else {
	    if (ioctl(benchdev, cmd) < 0) {
		perror("ioctl EVENT_SIGNAL");
		break;
	    }
	}
	event_signals++;
    }

    param.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);

    printf("Exiting %s\n", __FUNCTION__);

    return NULL;
}

static void *sem_down_thread(void *arg)
{
    struct sched_param param = { .sched_priority = (int)arg };
    struct rttst_rtdmtest_config config, *pconfig = NULL;
    int cmd;

    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    config.seqcount = testseqcount;
    config.timeout = timeout_us * 1000;
    if (timeout_us) {
	cmd = RTTST_RTIOC_RTDMTEST_SEM_TIMEDDOWN;
	pconfig = &config;
    } else
	cmd = RTTST_RTIOC_RTDMTEST_SEM_DOWN;

    if (verbose)
	printf("Starting %s\n", __FUNCTION__);

    while (!terminate) {
        if (ioctl(benchdev, cmd, pconfig) < 0) {
	    if (errno == ETIMEDOUT) {
		if (verbose)
		    printf("Timeout...\n");
                continue;
	    }
	    else if (errno == EWOULDBLOCK) {
		if (verbose)
		    printf("Would block...\n");
		usleep(100);
                continue;
	    }
	    perror("ioctl SEM_DOWN");
	    terminate = 1;
	    break;
	}
	sem_downs++;
    }

    param.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);

    if (verbose)
	printf("Exiting %s\n", __FUNCTION__);

    return NULL;
}

static void *sem_up_thread(void *arg)
{
    struct sched_param param = { .sched_priority = (int)arg };
    struct timespec interval;
    int cmd = RTTST_RTIOC_RTDMTEST_SEM_UP;

    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    if (delay_us > 0) {
	interval.tv_sec = delay_us / 1000000;
	interval.tv_nsec = (delay_us % 1000000) * 1000;
    }
    if (verbose)
	printf("Starting %s\n", __FUNCTION__);

    while (!terminate) {

	if (delay_us > 0)
	    nanosleep(&interval, NULL);
	    //clock_nanosleep(CLOCK_REALTIME, TIMER_RELTIME, &interval, NULL);

	if (testcount && sem_ups >= testcount) {
	    if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_SEM_DESTROY) < 0)
		perror("ioctl SEM_DESTROY");
            break;
	} else {
	    if (ioctl(benchdev, cmd) < 0) {
		perror("ioctl SEM_UP");
		break;
	    }
	}
	sem_ups++;
    }

    if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_NRTSIG_PEND) < 0)
	perror("ioctl NRTSIG_PEND");

    param.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);

    if (verbose)
	printf("Exiting %s\n", __FUNCTION__);

    return NULL;
}

static void *mutex_test_thread(void *arg)
{
    struct sched_param param = { .sched_priority = 90 };
    int num = (int)arg;
    struct rttst_rtdmtest_config config;
    struct timespec interval;
    int cmd;

    param.sched_priority -= num;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    config.seqcount = testseqcount;
    config.delay_jiffies = testmutexdelay;
    config.timeout = timeout_us * 1000;
    if (with_timeout)
	cmd = RTTST_RTIOC_RTDMTEST_MUTEX_TIMEDTEST;
    else
	cmd = RTTST_RTIOC_RTDMTEST_MUTEX_TEST;

    if (delay_us > 0) {
	interval.tv_sec = delay_us / 1000000;
	interval.tv_nsec = (delay_us % 1000000) * 1000;
    }
    if (verbose)
	printf("Starting %s[%d]\n", __FUNCTION__, num);

    while (!terminate) {
	if (delay_us > 0)
	    nanosleep(&interval, NULL);

	if (testcount && mutex_tests[num] >= testcount) {
	    if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_MUTEX_DESTROY) < 0)
		perror("ioctl MUTEX_DESTROY");
	    terminate = 1;
            break;
	}
        if (ioctl(benchdev, cmd, &config) < 0) {
	    if (errno == ETIMEDOUT) {
		if (verbose)
		    printf("Timeout...\n");
                continue;
	    }
	    else if (errno == EWOULDBLOCK) {
		if (verbose)
		    printf("Would block...\n");
		usleep(100);
                continue;
	    }
	    perror("ioctl MUTEX_TEST");
	    terminate = 1;
	    break;
	}
	mutex_tests[num]++;
    }

    param.sched_priority = 0;
    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);

    if (verbose)
	printf("Exiting %s\n", __FUNCTION__);

    return NULL;
}

int main(int argc, char *argv[])
{
    char devname[RTDM_MAX_DEVNAME_LEN];
    int benchdev_no = 0;
    pthread_t event_wait_thr, event_signal_thr;
    pthread_t sem_down_thr, sem_up_thr;
    pthread_t mutex_test_thr[RTDMTEST_MUTEX_THREADS];
    pthread_attr_t event_wait_attr, event_signal_attr;
    pthread_attr_t sem_down_attr, sem_up_attr;
    pthread_attr_t mutex_test_attr[RTDMTEST_MUTEX_THREADS];
    char c;
    int i, sum;

    signal(SIGINT, sighand);
    signal(SIGTERM, sighand);
    signal(SIGHUP, sighand);
    signal(SIGALRM, sighand);

    while ((c = getopt(argc,argv,"vd:D:t:T:c:C:m:esl")) != EOF)
        switch (c) {
	case 'v':
	    verbose = 1;
	    break;

	case 'm':
	    testcases |= TESTCASE_MUTEX;
	    testmutex = atoi(optarg);
	    break;

	case 's':
	    testcases |= TESTCASE_SEM;
	    break;

	case 'e':
	    testcases |= TESTCASE_EVENT;
	    break;

	case 'c':
	    testcount = atoi(optarg);
	    break;

	case 'C':
	    testseqcount = atoi(optarg);
	    break;

	case 'd':
	    delay_us = atoi(optarg);
	    break;

	case 'D':
	    testmutexdelay = atoi(optarg);
	    break;

	case 't':
	    with_timeout = 1;
	    timeout_us = atoi(optarg);
	    break;

	case 'T':
	    alarm(atoi(optarg));
	    break;

	default:
	    fprintf(stderr, "usage: irqbench [options]\n"
		    "  [-c <count>]  # test count\n"
		    "  [-T <test_duration_seconds>] # default=0, so ^C to end\n");
	    exit(2);
	}


    if (!testcases)
	testcases |= TESTCASE_EVENT;

    mlockall(MCL_CURRENT | MCL_FUTURE);

    snprintf(devname, RTDM_MAX_DEVNAME_LEN, "/dev/rttest%d", benchdev_no);
    benchdev = open(devname, O_RDWR);
    if (benchdev < 0) {
	perror("failed to open benchmark device");
	return 1;
    }

    if (testcases & TESTCASE_EVENT) {
	pthread_attr_init(&event_wait_attr);
	pthread_attr_setstacksize(&event_wait_attr, PTHREAD_STACK_MIN);
	pthread_create(&event_wait_thr, &event_wait_attr, event_wait_thread, (void *)2);
	if (delay_us >= 0) {
	    pthread_attr_init(&event_signal_attr);
	    pthread_attr_setstacksize(&event_signal_attr, PTHREAD_STACK_MIN);
	    pthread_create(&event_signal_thr, &event_signal_attr, event_signal_thread, (void *)1);
	}
    }

    if (testcases & TESTCASE_SEM) {
	pthread_attr_init(&sem_down_attr);
	pthread_attr_setstacksize(&sem_down_attr, PTHREAD_STACK_MIN);
	pthread_create(&sem_down_thr, &sem_down_attr, sem_down_thread, (void *)2);
	if (delay_us >= 0) {
	    pthread_attr_init(&sem_up_attr);
	    pthread_attr_setstacksize(&sem_up_attr, PTHREAD_STACK_MIN);
	    pthread_create(&sem_up_thr, &sem_up_attr, sem_up_thread, (void *)1);
	}
    }

    if (testcases & TESTCASE_MUTEX) {
	if (testmutex < 1)
	    testmutex = 1;
	else if (testmutex > RTDMTEST_MUTEX_THREADS)
	    testmutex = RTDMTEST_MUTEX_THREADS;
	for (i = 0; i < testmutex; i++) {
	    pthread_attr_init(&mutex_test_attr[i]);
	    pthread_attr_setstacksize(&mutex_test_attr[i], PTHREAD_STACK_MIN);
	    pthread_create(&mutex_test_thr[i], &mutex_test_attr[i], mutex_test_thread, (void *)i);
	}
    }

    while (!terminate) {
	if (!testcount) {
	    printf("Events %ld/%ld Sems %ld/%ld Mutex %ld",
		   event_signals, event_waits, sem_ups, sem_downs, mutex_tests[0]);
	    for (i = 1; i < testmutex; i++)
		printf("/%ld", mutex_tests[i]);
	    printf("\n");
	}
        usleep(500000);
    }

    if (testcases & TESTCASE_MUTEX) {
	if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_MUTEX_DESTROY) < 0)
	    perror("ioctl MUTEX_DESTROY");
    }

    usleep(500000);
    printf("Events %ld/%ld Sems %ld/%ld Mutex %ld",
	   event_signals, event_waits, sem_ups, sem_downs, mutex_tests[0]);
    sum = mutex_tests[0];
    for (i = 1; i < testmutex; i++) {
	sum += mutex_tests[i];
	printf("/%ld", mutex_tests[i]);
    }
    printf("\n");

    if (testcases & TESTCASE_MUTEX) {
	struct rttst_rtdmtest_config config;
	if (testseqcount == 0)
		testseqcount = 1;
	if (ioctl(benchdev, RTTST_RTIOC_RTDMTEST_MUTEX_GETSTAT, &config) < 0)
	    perror("ioctl MUTEX_GETSTAT");
	else
	    printf("Mutex lock count: %d (%d)\n",
		   config.seqcount, sum * testseqcount);
    }

    if (benchdev)
	close(benchdev);

    printf("Canceling threads\n");
    if (testcases & TESTCASE_EVENT) {
	pthread_cancel(event_wait_thr);
	if (delay_us >= 0)
	    pthread_cancel(event_signal_thr);
    }

    if (testcases & TESTCASE_SEM) {
	pthread_cancel(sem_down_thr);
	if (delay_us >= 0)
	    pthread_cancel(sem_up_thr);
    }

    if (testcases & TESTCASE_MUTEX) {
	for (i = 0; i < testmutex; i++)
	    pthread_cancel(mutex_test_thr[i]);
    }

    printf("Join wait thread\n");
    //pthread_join(thr1, NULL);
    printf("Join signal thread\n");
    //pthread_join(thr2, NULL);

    printf("Exit...\n");
    return 0;
}
