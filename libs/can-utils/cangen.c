/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
 * cangen.c - CAN frames generator
 *
 * Copyright (c) 2002-2007 Volkswagen Group Electronic Research
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Volkswagen nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Send feedback to <linux-can@vger.kernel.org>
 *
 */

#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "lib.h"

#define DEFAULT_GAP 200 /* ms */
#define DEFAULT_BURST_COUNT 1

#define MODE_RANDOM	0
#define MODE_INCREMENT	1
#define MODE_FIX	2
#define MODE_RANDOM_EVEN	3
#define MODE_RANDOM_ODD	4

extern int optind, opterr, optopt;

static volatile int running = 1;
static unsigned long long enobufs_count;

void print_usage(char *prg)
{
	fprintf(stderr, "%s - CAN frames generator.\n\n", prg);
	fprintf(stderr, "Usage: %s [options] <CAN interface>\n", prg);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "         -g <ms>       (gap in milli seconds "
		"- default: %d ms)\n", DEFAULT_GAP);
	fprintf(stderr, "         -e            (generate extended frame mode "
		"(EFF) CAN frames)\n");
	fprintf(stderr, "         -f            (generate CAN FD CAN frames)\n");
	fprintf(stderr, "         -b            (generate CAN FD CAN frames"
		" with bitrate switch (BRS))\n");
	fprintf(stderr, "         -E            (generate CAN FD CAN frames"
		" with error state (ESI))\n");
	fprintf(stderr, "         -R            (generate RTR frames)\n");
	fprintf(stderr, "         -8            (allow DLC values greater then 8 for Classic CAN frames)\n");
	fprintf(stderr, "         -m            (mix -e -f -b -E -R frames)\n");
	fprintf(stderr, "         -I <mode>     (CAN ID"
		" generation mode - see below)\n");
	fprintf(stderr, "         -L <mode>     (CAN data length code (dlc)"
		" generation mode - see below)\n");
	fprintf(stderr, "         -D <mode>     (CAN data (payload)"
		" generation mode - see below)\n");
	fprintf(stderr, "         -p <timeout>  (poll on -ENOBUFS to write frames"
		" with <timeout> ms)\n");
	fprintf(stderr, "         -n <count>    (terminate after <count> CAN frames "
		"- default infinite)\n");
	fprintf(stderr, "         -i            (ignore -ENOBUFS return values on"
		" write() syscalls)\n");
	fprintf(stderr, "         -x            (disable local loopback of "
		"generated CAN frames)\n");
	fprintf(stderr, "         -c <count>    (number of messages to send in burst, "
		"default 1)\n");
	fprintf(stderr, "         -v            (increment verbose level for "
		"printing sent CAN frames)\n\n");
	fprintf(stderr, "Generation modes:\n");
	fprintf(stderr, " 'r'     => random values (default)\n");
	fprintf(stderr, " 'e'     => random values, even ID\n");
	fprintf(stderr, " 'o'     => random values, odd ID\n");
	fprintf(stderr, " 'i'     => increment values\n");
	fprintf(stderr, " <value> => fixed value (in hexadecimal for -I and -D)\n\n");
	fprintf(stderr, "The gap value (in milliseconds) may have decimal places, e.g. '-g 4.73'\n");
	fprintf(stderr, "When incrementing the CAN data the data length code "
		"minimum is set to 1.\n");
	fprintf(stderr, "CAN IDs and data content are given and expected in hexadecimal values.\n\n");
	fprintf(stderr, "Examples:\n");
	fprintf(stderr, "%s vcan0 -g 4 -I 42A -L 1 -D i -v -v\n", prg);
	fprintf(stderr, "\t(fixed CAN ID and length, inc. data)\n");
	fprintf(stderr, "%s vcan0 -e -L i -v -v -v\n", prg);
	fprintf(stderr, "\t(generate EFF frames, incr. length)\n");
	fprintf(stderr, "%s vcan0 -D 11223344DEADBEEF -L 8\n", prg);
	fprintf(stderr, "\t(fixed CAN data payload and length)\n");
	fprintf(stderr, "%s vcan0 -I 555 -D CCCCCCCCCCCCCCCC -L 8 -g 3.75\n", prg);
	fprintf(stderr, "\t(generate a fix busload without bit-stuffing effects)\n");
	fprintf(stderr, "%s vcan0 -g 0 -i -x\n", prg);
	fprintf(stderr, "\t(full load test ignoring -ENOBUFS)\n");
	fprintf(stderr, "%s vcan0 -g 0 -p 10 -x\n", prg);
	fprintf(stderr, "\t(full load test with polling, 10ms timeout)\n");
	fprintf(stderr, "%s vcan0\n", prg);
	fprintf(stderr, "\t(my favourite default :)\n\n");
}

void sigterm(int signo)
{
	running = 0;
}

int main(int argc, char **argv)
{
	double gap = DEFAULT_GAP;
	unsigned long burst_count = DEFAULT_BURST_COUNT;
	unsigned long polltimeout = 0;
	unsigned char ignore_enobufs = 0;
	unsigned char extended = 0;
	unsigned char canfd = 0;
	unsigned char brs = 0;
	unsigned char esi = 0;
	unsigned char mix = 0;
	unsigned char id_mode = MODE_RANDOM;
	unsigned char data_mode = MODE_RANDOM;
	unsigned char dlc_mode = MODE_RANDOM;
	unsigned char loopback_disable = 0;
	unsigned char verbose = 0;
	unsigned char rtr_frame = 0;
	unsigned char len8_dlc = 0;
	int count = 0;
	unsigned long burst_sent_count = 0;
	int mtu, maxdlen;
	uint64_t incdata = 0;
	int incdlc = 0;
	unsigned long rnd;
	unsigned char fixdata[CANFD_MAX_DLEN];

	int opt;
	int s; /* socket */
	struct pollfd fds;

	struct sockaddr_can addr;
	static struct canfd_frame frame;
	struct can_frame *ccf = (struct can_frame *)&frame;
	int nbytes;
	int i;
	struct ifreq ifr;

	struct timespec ts;
	struct timeval now;

	/* set seed value for pseudo random numbers */
	gettimeofday(&now, NULL);
	srandom(now.tv_usec);

	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);
	signal(SIGINT, sigterm);

	while ((opt = getopt(argc, argv, "ig:ebEfmI:L:D:xp:n:c:vR8h?")) != -1) {
		switch (opt) {

		case 'i':
			ignore_enobufs = 1;
			break;

		case 'g':
			gap = strtod(optarg, NULL);
			break;

		case 'e':
			extended = 1;
			break;

		case 'f':
			canfd = 1;
			break;

		case 'b':
			brs = 1; /* bitrate switch implies CAN FD */
			canfd = 1;
			break;

		case 'E':
			esi = 1; /* error state indicator implies CAN FD */
			canfd = 1;
			break;

		case 'm':
			mix = 1;
			canfd = 1; /* to switch the socket into CAN FD mode */
			break;

		case 'I':
			if (optarg[0] == 'r') {
				id_mode = MODE_RANDOM;
			} else if (optarg[0] == 'i') {
				id_mode = MODE_INCREMENT;
			} else if (optarg[0] == 'e') {
				id_mode = MODE_RANDOM_EVEN;
			} else if (optarg[0] == 'o') {
				id_mode = MODE_RANDOM_ODD;
			} else {
				id_mode = MODE_FIX;
				frame.can_id = strtoul(optarg, NULL, 16);
			}
			break;

		case 'L':
			if (optarg[0] == 'r') {
				dlc_mode = MODE_RANDOM;
			} else if (optarg[0] == 'i') {
				dlc_mode = MODE_INCREMENT;
			} else {
				dlc_mode = MODE_FIX;
				frame.len = atoi(optarg) & 0xFF; /* is cut to 8 / 64 later */
			}
			break;

		case 'D':
			if (optarg[0] == 'r') {
				data_mode = MODE_RANDOM;
			} else if (optarg[0] == 'i') {
				data_mode = MODE_INCREMENT;
			} else {
				data_mode = MODE_FIX;
				if (hexstring2data(optarg, fixdata, CANFD_MAX_DLEN)) {
					printf ("wrong fix data definition\n");
					return 1;
				}
			}
			break;

		case 'c':
			burst_count = strtoul(optarg, NULL, 10);
			break;

		case 'v':
			verbose++;
			break;

		case 'x':
			loopback_disable = 1;
			break;

		case 'R':
			rtr_frame = 1;
			break;

		case '8':
			len8_dlc = 1;
			break;

		case 'p':
			polltimeout = strtoul(optarg, NULL, 10);
			break;

		case 'n':
			count = atoi(optarg);
			if (count < 1) {
				print_usage(basename(argv[0]));
				return 1;
			}
			break;

		case '?':
		case 'h':
		default:
			print_usage(basename(argv[0]));
			return 1;
			break;
		}
	}

	if (optind == argc) {
		print_usage(basename(argv[0]));
		return 1;
	}

	ts.tv_sec = gap / 1000;
	ts.tv_nsec = (long)(((long long)(gap * 1000000)) % 1000000000LL);

	/* recognize obviously missing commandline option */
	if (id_mode == MODE_FIX && frame.can_id > 0x7FF && !extended) {
		printf("The given CAN-ID is greater than 0x7FF and "
		       "the '-e' option is not set.\n");
		return 1;
	}

	if (strlen(argv[optind]) >= IFNAMSIZ) {
		printf("Name of CAN device '%s' is too long!\n\n", argv[optind]);
		return 1;
	}

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket");
		return 1;
	}

	addr.can_family = AF_CAN;

	strcpy(ifr.ifr_name, argv[optind]);
	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
		perror("SIOCGIFINDEX");
		return 1;
	}
	addr.can_ifindex = ifr.ifr_ifindex;

	/* disable default receive filter on this RAW socket */
	/* This is obsolete as we do not read from the socket at all, but for */
	/* this reason we can remove the receive list in the Kernel to save a */
	/* little (really a very little!) CPU usage.                          */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

	if (loopback_disable) {
		int loopback = 0;

		setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK,
			   &loopback, sizeof(loopback));
	}

	if (canfd) {
		int enable_canfd = 1;

		/* check if the frame fits into the CAN netdevice */
		if (ioctl(s, SIOCGIFMTU, &ifr) < 0) {
			perror("SIOCGIFMTU");
			return 1;
		}

		if (ifr.ifr_mtu != CANFD_MTU) {
			printf("CAN interface is not CAN FD capable - sorry.\n");
			return 1;
		}

		/* interface is ok - try to switch the socket into CAN FD mode */
		if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd))){
			printf("error when enabling CAN FD support\n");
			return 1;
		}

		/* ensure discrete CAN FD length values 0..8, 12, 16, 20, 24, 32, 64 */
		frame.len = can_fd_dlc2len(can_fd_len2dlc(frame.len));
	} else {
		/* sanitize Classical CAN 2.0 frame length */
		if (len8_dlc) {
			if (frame.len > CAN_MAX_RAW_DLC)
				frame.len = CAN_MAX_RAW_DLC;

			if (frame.len > CAN_MAX_DLEN)
				ccf->len8_dlc = frame.len;
		}

		if (frame.len > CAN_MAX_DLEN)
			frame.len = CAN_MAX_DLEN;
	}

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (polltimeout) {
		fds.fd = s;
		fds.events = POLLOUT;
	}

	while (running) {
		frame.flags = 0;

		if (count && (--count == 0))
			running = 0;

		if (canfd){
			mtu = CANFD_MTU;
			maxdlen = CANFD_MAX_DLEN;
			if (brs)
				frame.flags |= CANFD_BRS;
			if (esi)
				frame.flags |= CANFD_ESI;
		} else {
			mtu = CAN_MTU;
			maxdlen = CAN_MAX_DLEN;
		}

		if (id_mode == MODE_RANDOM)
			frame.can_id = random();
		else if (id_mode == MODE_RANDOM_EVEN)
			frame.can_id = random() & ~0x1;
		else if (id_mode == MODE_RANDOM_ODD)
			frame.can_id = random() | 0x1;

		if (extended) {
			frame.can_id &= CAN_EFF_MASK;
			frame.can_id |= CAN_EFF_FLAG;
		} else
			frame.can_id &= CAN_SFF_MASK;

		if (rtr_frame && !canfd)
			frame.can_id |= CAN_RTR_FLAG;

		if (dlc_mode == MODE_RANDOM) {

			if (canfd)
				frame.len = can_fd_dlc2len(random() & 0xF);
			else {
				frame.len = random() & 0xF;

				if (frame.len > CAN_MAX_DLEN) {
					/* generate Classic CAN len8 DLCs? */
					if (len8_dlc)
						ccf->len8_dlc = frame.len;

					frame.len = 8; /* for about 50% of the frames */
				} else {
					ccf->len8_dlc = 0;
				}
			}
		}

		if (data_mode == MODE_INCREMENT && !frame.len)
			frame.len = 1; /* min dlc value for incr. data */

		if (data_mode == MODE_RANDOM) {

			rnd = random();
			memcpy(&frame.data[0], &rnd, 4);
			rnd = random();
			memcpy(&frame.data[4], &rnd, 4);

			/* omit extra random number generation for CAN FD */
			if (canfd && frame.len > 8) {
				memcpy(&frame.data[8], &frame.data[0], 8);
				memcpy(&frame.data[16], &frame.data[0], 16);
				memcpy(&frame.data[32], &frame.data[0], 32);
			}
		}

		if (data_mode == MODE_FIX)
			memcpy(frame.data, fixdata, CANFD_MAX_DLEN);

		/* set unused payload data to zero like the CAN driver does it on rx */
		if (frame.len < maxdlen)
			memset(&frame.data[frame.len], 0, maxdlen - frame.len);

		if (verbose) {

			printf("  %s  ", argv[optind]);

			if (verbose > 1)
				fprint_long_canframe(stdout, &frame, "\n", (verbose > 2)?1:0, maxdlen);
			else
				fprint_canframe(stdout, &frame, "\n", 1, maxdlen);
		}

resend:
		nbytes = write(s, &frame, mtu);
		if (nbytes < 0) {
			if (errno != ENOBUFS) {
				perror("write");
				return 1;
			}
			if (!ignore_enobufs && !polltimeout) {
				perror("write");
				return 1;
			}
			if (polltimeout) {
				int ret;

				/* wait for the write socket (with timeout) */
				ret = poll(&fds, 1, polltimeout);
				if (ret == 0 || (ret == -1 && errno != -EINTR)) {
					perror("poll");
					return 1;
				}
				goto resend;
			} else
				enobufs_count++;

		} else if (nbytes < mtu) {
			fprintf(stderr, "write: incomplete CAN frame\n");
			return 1;
		}

		burst_sent_count++;
		if (gap && burst_sent_count >= burst_count) /* gap == 0 => performance test :-] */
			if (nanosleep(&ts, NULL))
				return 1;

		if (burst_sent_count >= burst_count)
			burst_sent_count = 0;

		if (id_mode == MODE_INCREMENT)
			frame.can_id++;

		if (dlc_mode == MODE_INCREMENT) {

			incdlc++;
			incdlc %= CAN_MAX_RAW_DLC + 1;

			if (canfd && !mix)
				frame.len = can_fd_dlc2len(incdlc);
			else if (len8_dlc) {
				if (incdlc > CAN_MAX_DLEN) {
					frame.len = CAN_MAX_DLEN;
					ccf->len8_dlc = incdlc;
				} else {
					frame.len = incdlc;
					ccf->len8_dlc = 0;
				}
			} else {
				incdlc %= CAN_MAX_DLEN + 1;
				frame.len = incdlc;
			}
		}

		if (data_mode == MODE_INCREMENT) {

			incdata++;

			for (i=0; i<8 ;i++)
				frame.data[i] = (incdata >> i*8) & 0xFFULL;
		}

		if (mix) {
			i = random();
			extended = i&1;
			canfd = i&2;
			if (canfd) {
				brs = i&4;
				esi = i&8;
			}
			rtr_frame = ((i&24) == 24); /* reduce RTR frames to 1/4 */
		}
	}

	if (enobufs_count)
		printf("\nCounted %llu ENOBUFS return values on write().\n\n",
		       enobufs_count);

	close(s);

	return 0;
}
