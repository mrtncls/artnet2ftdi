/*
 * Simple proxy between USB2DMX adapter cables and UDP senders
 *
 * Compile with:
 *
 *   gcc -o dmx-udp dmx-udp.c $(pkg-config --cflags --libs libftdi1) -Wall
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * (c) 2016-2018 Daniel Mack
 */

#include <assert.h>
#include <arpa/inet.h>
#include <ftdi.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BREAK_DELAY 120
#define MAB_DELAY 16
#define FRAME_DELAY 12000
#define DMX_FRAME_SIZE 513

// The number of bytes to skip in each UDP packet
// Set this to 18 for Art-Net protocol compatibility
#define UDP_HEADER_SIZE 17

int main(int argc, char **argv) {
	unsigned char buf[DMX_FRAME_SIZE] = { 0 };
	struct sockaddr_in si_me, si_other;
	socklen_t slen = sizeof(si_other);
	struct ftdi_context dmx;
	int r, port, udp_socket;

	if (argc < 2) {
		printf("Usage: %s <udp-port-number>\n", argv[0]);
		return EXIT_FAILURE;
	}

	port = atoi(argv[1]);
	assert(port > 0 && port < 65536);

	/* Serial port setup: 250kBaud, 8N2 */
	ftdi_init(&dmx);
	assert(ftdi_usb_open_desc(&dmx, 0x0403, 0x6001, NULL, NULL) == 0);
	assert(ftdi_usb_reset(&dmx) == 0);
	assert(ftdi_set_interface(&dmx, INTERFACE_ANY) == 0);
	assert(ftdi_set_baudrate(&dmx, 250000) == 0);
	assert(ftdi_set_line_property(&dmx, BITS_8, STOP_BIT_2, NONE) == 0);
	assert(ftdi_setflowctrl(&dmx, SIO_DISABLE_FLOW_CTRL) == 0);

	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	assert(udp_socket >= 0);

	memset(&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);

	r = bind(udp_socket, (struct sockaddr *) &si_me, sizeof(si_me));
	assert(r >= 0);

	printf("Waiting for packets on UDP port %d ...\n", port);

	struct pollfd pfd = {
		.fd = udp_socket,
		.events = POLLIN,
	};

	for (;;) {
		do {
			r = poll(&pfd, 1, 0);
			if (r > 0) {
				unsigned char udp[DMX_FRAME_SIZE + UDP_HEADER_SIZE] = { 0 };

				r = recvfrom(udp_socket, udp, sizeof(udp), 0, (struct sockaddr *) &si_other, &slen);
				if (r > UDP_HEADER_SIZE) {
					//printf("Got %lu bytes from %s:%d.\n",
					//	sizeof(udp), inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port)
					//	);
					memcpy(buf, udp + UDP_HEADER_SIZE, r - UDP_HEADER_SIZE);
				}
			}
		} while (r > 0);

		assert(ftdi_set_line_property2(&dmx, BITS_8, STOP_BIT_2, NONE, BREAK_ON) == 0);
		usleep(BREAK_DELAY);

		assert(ftdi_set_line_property2(&dmx, BITS_8, STOP_BIT_2, NONE, BREAK_OFF) == 0);
		usleep(MAB_DELAY);

		printf("Channels: 6:%u 7:%u 8:%u 9:%u 10:%u 11:%u\n", buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);

		assert(ftdi_write_data(&dmx, buf, sizeof(buf)) == sizeof(buf));
		usleep(FRAME_DELAY);
	}

	return EXIT_SUCCESS;
}
