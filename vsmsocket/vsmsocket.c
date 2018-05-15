/*
  Copyright (C) 2018 Collabora Ltd

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Authors:
    Guillaume Tucker <guillaume.tucker@collabora.com>
*/

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "vsm_socket.h"

#define BUFFER_SIZE 1024
static const uint16_t DEFAULT_PORT_NUMBER = 60000;

static const char HELP[] =
"VSM socket sample application\n"
"\n"
"This utility will use the libvsmsocket library to open a TCP socket\n"
"and listen for an incoming client connection.  It will then forward\n"
"signals to and from stdin and stdout.\n"
"\n"
"Usage:\n"
"  vsmsocket <OPTIONS>\n"
"\n"
"Options:\n"
"  -p PORT\n"
"     Listen on and send packets to port number PORT.  Default is 60000.\n"
"\n"
"  -h\n"
"     Print this help message.\n"
"\n";

static int g_abort = 0;

static void handle_sigkill(int signum)
{
	if ((signum == SIGTERM) || (signum == SIGINT))
		g_abort = 1;
}

static int listen_loop(struct vsm_socket *vsm_sock)
{
	int stat;

	while (!g_abort) {
		const char *signal;
		const char *value;

		stat = vsm_socket_receive(vsm_sock, &signal, &value);

		if (stat < 0) {
			if (g_abort)
				stat = 0;
			else
				printf("Failed to receive signal\n");
			break;
		}

		if ((signal == NULL) || (signal[0] == '\0'))
			break;

		printf("> %s=%s\n", signal, value);
	}

	return stat;
}

static int stdin_loop(struct vsm_socket *vsm_sock)
{
	int stat = 0;

	printf("Waiting for input...\n");

	while (!g_abort) {
		const char *signal;
		const char *value;

		stat = vsm_socket_select(STDIN_FILENO);

		if (stat < 0) {
			if (g_abort)
				stat = 0;
			else
				printf("Failed to wait for stdin\n");
			break;
		}

		stat = vsm_socket_fread(stdin, vsm_sock->buffer,
					vsm_sock->buffer_size, &signal, &value);

		if (stat < 0) {
			printf("Invalid signal\n");
			break;
		}

		if ((signal == NULL) || (signal[0] == '\0'))
			break;

		stat = vsm_socket_send_str(vsm_sock, signal, value);

		if (stat < 0) {
			printf("Failed to send signal");
			break;
		}
	}

	return stat;
}

static int run_loop(struct vsm_socket *vsm_sock)
{
	struct sigaction new_action, old_action_sigterm, old_action_sigint;
	int stat = 0;

	new_action.sa_handler = handle_sigkill;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction(SIGTERM, &new_action, &old_action_sigterm);
	sigaction(SIGINT, &new_action, &old_action_sigint);

	while (!g_abort) {
		pid_t pid;
		int child_status;

		printf("Waiting for VSM client...\n");

		if (vsm_socket_accept(vsm_sock) < 0) {
			if (!g_abort) {
				printf("Failed to open VSM connection\n");
				stat = -1;
				break;
			}
		}

		if (g_abort) {
			printf("Shutting down\n");
			break;
		}

		printf("VSM client connected\n");

		pid = fork();

		if (pid < 0) {
			printf("Failed to start child process\n");
			stat = -1;
			break;
		}

		if (!pid) {
			stat = stdin_loop(vsm_sock);
			break;
		}

		stat = listen_loop(vsm_sock);
		vsm_socket_close(vsm_sock);
		printf("VSM client connection closed\n");

		kill(pid, SIGTERM);
		waitpid(pid, &child_status, 0);

		if (child_status) {
			printf("stdin loop failed\n");
			stat = -1;
			break;
		}
	}

	sigaction(SIGTERM, &old_action_sigterm, NULL);
	sigaction(SIGINT, &old_action_sigint, NULL);

	return stat;
}

int main(int argc, char **argv)
{
	static const char OPTS[] = "p:h";
	uint16_t port_number = DEFAULT_PORT_NUMBER;
	char buffer[BUFFER_SIZE];
	struct vsm_socket vsm_sock;

	int c;
	while ((c = getopt(argc, argv, OPTS)) != -1) {
		switch (c) {
		case 'p':
			errno = 0;
			unsigned long port = strtoul(optarg, NULL, 0);
			if (errno || !port || (port > 65535)) {
				printf("Invalid port number: %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			port_number = (uint16_t)port;
			break;
		case 'h':
			printf(HELP);
			exit(EXIT_SUCCESS);
			break;
		case '?':
		default:
			printf("Invalid option\n");
			printf(HELP);
			exit(EXIT_FAILURE);
			break;
		}
	}

	printf("Port number: %d\n", port_number);

	if (vsm_socket_init(&vsm_sock, port_number, buffer, BUFFER_SIZE) < 0) {
		printf("Failed to initialise vsm_sock\n");
	} else if (run_loop(&vsm_sock) < 0) {
		printf("Failed to run main loop\n");
	}

	vsm_socket_free(&vsm_sock);

	return 0;
}
