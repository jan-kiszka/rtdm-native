#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <sys/mman.h>

#if 1
#include <rtdm/rtcan.h>
#else
#include <linux/can/raw.h>
#include <linux/can/ioctl.h>
#endif

static void print_usage(char *prg)
{
    fprintf(stderr,
	    "Usage: %s [<can-interface>] [Options]\n"
	    "Options:\n"
	    " -f  --filter=id:mask[:id:mask]... apply filter\n"
	    " -e  --error=mask      receive error messages\n"
	    " -t, --timeout=MS      timeout in ms\n"
	    " -T, --timestamp       with absolute timestamp\n"
	    " -R, --timestamp-rel   with relative timestamp\n"
	    " -v, --verbose         be verbose\n"
	    " -p, --print=MODULO    print every MODULO message\n"
	    " -h, --help            this help\n",
	    prg);
}


extern int optind, opterr, optopt;

static int s = -1, verbose = 0, print = 1;
static nanosecs_rel_t timeout = 0, with_timestamp = 0, timestamp_rel = 0;

#define BUF_SIZ	255
#define MAX_FILTER 16

struct sockaddr_can recv_addr;
struct can_filter recv_filter[MAX_FILTER];
static int filter_count = 0;

int add_filter(u_int32_t id, u_int32_t mask)
{
    if (filter_count >= MAX_FILTER)
	return -1;
    recv_filter[filter_count].can_id = id;
    recv_filter[filter_count].can_mask = mask;
    printf("Filter #%d: id=0x%08x mask=0x%08x\n", filter_count, id, mask);
    filter_count++;
    return 0;
}

void cleanup(void)
{
    int ret;

    if (verbose)
	printf("Cleaning up...\n");

    usleep(100000);
    if (s >= 0) {
	ret = close(s);
	s = -1;
	if (ret)
	    perror("close");
    }
}

void cleanup_and_exit(int sig)
{
    if (verbose)
	printf("Signal %d received\n", sig);
    if (sig == 2)
	    return;
    cleanup();
    exit(0);
}

void receive_task(void)
{
    int i, ret, count = 0;
    struct can_frame frame;
    struct sockaddr_can addr;
    socklen_t addrlen = sizeof(addr);
    struct msghdr msg;
    struct iovec iov;
    nanosecs_abs_t timestamp, timestamp_prev = 0;

    if (with_timestamp) {
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = (void *)&addr;
	msg.msg_namelen = sizeof(struct sockaddr_can);
	msg.msg_control = (void *)&timestamp;
	msg.msg_controllen = sizeof(nanosecs_abs_t);
    }

    while (1) {
	if (with_timestamp) {
	    iov.iov_base = (void *)&frame;
	    iov.iov_len = sizeof(can_frame_t);
	    ret = recvmsg(s, &msg, 0);
	} else
	    ret = recvfrom(s, (void *)&frame, sizeof(can_frame_t), 0,
				  (struct sockaddr *)&addr, &addrlen);
	if (ret < 0) {
		printf("errno=%d\n", errno);
	    switch (errno) {
	    case ETIMEDOUT:
		if (verbose)
		    printf("recv: timed out");
		continue;
	    case EBADF:
		if (verbose)
		    printf("recv: aborted because socket was closed");
		break;
	    default:
		perror("recv");
	    }
	    break;
	}

	if (print && (count % print) == 0) {
	    printf("#%d: (%d) ", count, addr.can_ifindex);
	    if (with_timestamp && msg.msg_controllen) {
		if (timestamp_rel) {
		    printf("%lldns ", timestamp - timestamp_prev);
		    timestamp_prev = timestamp;
		} else
		    printf("%lldns ", timestamp);
	    }
	    if (frame.can_id & CAN_ERR_FLAG)
		printf("!0x%08x!", frame.can_id & CAN_ERR_MASK);
	    else if (frame.can_id & CAN_EFF_FLAG)
		printf("<0x%08x>", frame.can_id & CAN_EFF_MASK);
	    else
		printf("<0x%03x>", frame.can_id & CAN_SFF_MASK);

	    printf(" [%d]", frame.can_dlc);
	    if (!(frame.can_id & CAN_RTR_FLAG))
		for (i = 0; i < frame.can_dlc; i++) {
		    printf(" %02x", frame.data[i]);
		}
	    if (frame.can_id & CAN_ERR_FLAG) {
		printf(" ERROR ");
		if (frame.can_id & CAN_ERR_BUSOFF)
		    printf("bus-off");
		if (frame.can_id & CAN_ERR_CRTL)
		    printf("controller problem");
	    } else if (frame.can_id & CAN_RTR_FLAG)
		printf(" remote request");
	    printf("\n");
	}
	count++;
    }
}

int main(int argc, char **argv)
{
    int opt, ret;
    u_int32_t id, mask;
    u_int32_t err_mask = 0;
    struct ifreq ifr;
    char *ptr;

    struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "verbose", no_argument, 0, 'v'},
	{ "filter", required_argument, 0, 'f'},
	{ "error", required_argument, 0, 'e'},
	{ "timeout", required_argument, 0, 't'},
	{ "timestamp", no_argument, 0, 'T'},
	{ "timestamp-rel", no_argument, 0, 'R'},
	{ 0, 0, 0, 0},
    };

    mlockall(MCL_CURRENT | MCL_FUTURE);

    signal(SIGTERM, cleanup_and_exit);
    signal(SIGINT, cleanup_and_exit);

    while ((opt = getopt_long(argc, argv, "hve:f:t:p:RT",
			      long_options, NULL)) != -1) {
	switch (opt) {
	case 'h':
	    print_usage(argv[0]);
	    exit(0);

	case 'p':
	    print = strtoul(optarg, NULL, 0);

	case 'v':
	    verbose = 1;
	    break;

	case 'e':
	    err_mask = strtoul(optarg, NULL, 0);
	    break;

	case 'f':
	    ptr = optarg;
	    while (1) {
		id = strtoul(ptr, NULL, 0);
		ptr = strchr(ptr, ':');
		if (!ptr) {
		    fprintf(stderr, "filter must be applied in the form id:mask[:id:mask]...\n");
		    exit(1);
		}
		ptr++;
		mask = strtoul(ptr, NULL, 0);
		ptr = strchr(ptr, ':');
		add_filter(id, mask);
		if (!ptr)
		    break;
		ptr++;
	    }
	    break;

	case 't':
	    timeout = (nanosecs_rel_t)strtoul(optarg, NULL, 0) * 1000000;
	    break;

	case 'R':
	    timestamp_rel = 1;
	case 'T':
	    with_timestamp = 1;
	    break;

	default:
	    fprintf(stderr, "Unknown option %c\n", opt);
	    break;
	}
    }

    ret = socket(PF_CAN, SOCK_RAW, 0);
    if (ret < 0) {
	perror("socket");
	return -1;
    }
    s = ret;

    if (argv[optind] == NULL) {
	if (verbose)
	    printf("interface all\n");

	ifr.ifr_ifindex = 0;
    } else {
	if (verbose)
	    printf("interface %s\n", argv[optind]);

	strncpy(ifr.ifr_name, argv[optind], IFNAMSIZ);
	if (verbose)
	    printf("s=%d, ifr_name=%s\n", s, ifr.ifr_name);

	ret = ioctl(s, SIOCGIFINDEX, &ifr);
	if (ret < 0) {
	    perror("ioctl GET_IFINDEX");
	    goto failure;
	}
    }

    if (err_mask) {
	ret = setsockopt(s, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
			 &err_mask, sizeof(err_mask));
	if (ret < 0) {
	    perror("setsockopt");
	    goto failure;
	}
	if (verbose)
	    printf("Using err_mask=%#x\n", err_mask);
    }

    if (filter_count) {
	ret = setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER,
			 &recv_filter, filter_count *
			 sizeof(struct can_filter));
	if (ret < 0) {
	    perror("setsockopt");
	    goto failure;
	}
    }

    recv_addr.can_family = AF_CAN;
    recv_addr.can_ifindex = ifr.ifr_ifindex;
    ret = bind(s, (struct sockaddr *)&recv_addr,
	       sizeof(struct sockaddr_can));
    if (ret < 0) {
	perror("bind");
	goto failure;
    }

    if (timeout) {
#ifdef RTCAN_RTIOC_RCV_TIMEOUT
	if (verbose)
	    printf("Timeout: %lld ns\n", timeout);
	ret = ioctl(s, RTCAN_RTIOC_RCV_TIMEOUT, &timeout);
	if (ret) {
	    perror("ioctl RCV_TIMEOUT");
	    goto failure;
	}
#endif
    }

    if (with_timestamp) {
#ifdef RTCAN_RTIOC_TAKE_TIMESTAMP
	ret = ioctl(s, RTCAN_RTIOC_TAKE_TIMESTAMP, RTCAN_TAKE_TIMESTAMPS);
	if (ret) {
	    perror("ioctl TAKE_TIMESTAMP");
	    goto failure;
	}
#endif
    }

    receive_task();

 failure:
    cleanup();
    return -1;
}
