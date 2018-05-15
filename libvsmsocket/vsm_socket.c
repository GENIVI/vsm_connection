/*
  Copyright (C) 2018 Jaguar Land Rover

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Authors:
    Guillaume Tucker <guillaume.tucker@collabora.com>
*/

#include "vsm_socket.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int vsm_socket_init(struct vsm_socket *vsm_sock, unsigned port,
		    char *buffer, size_t buffer_size)
{
	int opt_val = 1;
	int status;

	vsm_sock->server_fd = -1;
	vsm_sock->client_fd = -1;
	memset(&vsm_sock->server_addr, 0, sizeof(vsm_sock->server_addr));
	memset(&vsm_sock->client_addr, 0, sizeof(vsm_sock->client_addr));
	vsm_sock->buffer = buffer;
	vsm_sock->buffer_size = buffer_size;

	vsm_sock->server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (vsm_sock->server_fd < 0) {
		return -1;
	}

	vsm_sock->server_addr.sin_family = AF_INET;
	vsm_sock->server_addr.sin_port = htons(port);
	vsm_sock->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	setsockopt(vsm_sock->server_fd, SOL_SOCKET, SO_REUSEADDR,
		   &opt_val, sizeof(opt_val));

	status = bind(vsm_sock->server_fd,
		      (struct sockaddr *) &vsm_sock->server_addr,
		      sizeof(vsm_sock->server_addr));

	if (status < 0) {
		close(vsm_sock->server_fd);
		return -1;
	}

	status = listen(vsm_sock->server_fd, 1);

	if (status < 0) {
		close(vsm_sock->server_fd);
		return -1;
	}

	return 0;
}

int vsm_socket_accept(struct vsm_socket *vsm_sock)
{
	socklen_t client_len = sizeof(vsm_sock->client_addr);

	vsm_sock->client_fd = accept(vsm_sock->server_fd,
				     (struct sockaddr *) &vsm_sock->client_addr,
				     &client_len);

	if (vsm_sock->client_fd < 0) {
		close(vsm_sock->server_fd);
		return -1;
	}

	return 0;
}

void vsm_socket_close(struct vsm_socket *vsm_sock)
{
	if (vsm_sock->client_fd >= 0) {
		close(vsm_sock->client_fd);
		vsm_sock->client_fd = -1;
	}

	if (vsm_sock->server_fd >= 0) {
		close(vsm_sock->server_fd);
		vsm_sock->server_fd = -1;
	}
}

int vsm_socket_send_bool(struct vsm_socket *vsm_sock,
			 const char *signal, int value)
{
	int status;

	status = snprintf(vsm_sock->buffer, vsm_sock->buffer_size, "%s=%s\n",
			  signal, value ? "True" : "False");

	if (status >= vsm_sock->buffer_size)
		return -1;

	return vsm_socket_send(vsm_sock);
}

int vsm_socket_send(struct vsm_socket *vsm_sock)
{
	size_t len;

	vsm_sock->buffer[vsm_sock->buffer_size - 1] = '\0';
	len = strlen(vsm_sock->buffer);

	while (len) {
		ssize_t sent;

		sent = send(vsm_sock->client_fd, vsm_sock->buffer, len, 0);

		if (sent < 0)
			return -1;

		len -= (size_t)sent;
	}

	return 0;
}
