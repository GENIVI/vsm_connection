/*
  Copyright (C) 2018 Jaguar Land Rover

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Authors:
    Guillaume Tucker <guillaume.tucker@collabora.com>
*/

/* Needed for fdopen in particular */
#define _POSIX_C_SOURCE 1

#include "vsm_socket.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>


int vsm_socket_init(struct vsm_socket *vsm_sock, unsigned port,
		    char *buffer, size_t buffer_size)
{
	int opt_val = 1;
	int stat;

	vsm_sock->in = NULL;
	vsm_sock->out = NULL;
	vsm_sock->read_fd = -1;
	memset(&vsm_sock->server_addr, 0, sizeof(vsm_sock->server_addr));
	memset(&vsm_sock->client_addr, 0, sizeof(vsm_sock->client_addr));
	vsm_sock->buffer = buffer;
	vsm_sock->buffer_size = buffer_size;

	vsm_sock->server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (vsm_sock->server_fd < 0)
		return -1;

	vsm_sock->server_addr.sin_family = AF_INET;
	vsm_sock->server_addr.sin_port = htons(port);
	vsm_sock->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	setsockopt(vsm_sock->server_fd, SOL_SOCKET, SO_REUSEADDR,
		   &opt_val, sizeof(opt_val));

	stat = bind(vsm_sock->server_fd,
		    (struct sockaddr *) &vsm_sock->server_addr,
		    sizeof(vsm_sock->server_addr));

	if (stat < 0) {
		close(vsm_sock->server_fd);
		vsm_sock->server_fd = -1;
		return -1;
	}

	stat = listen(vsm_sock->server_fd, 1);

	if (stat < 0) {
		close(vsm_sock->server_fd);
		vsm_sock->server_fd = -1;
		return -1;
	}

	return 0;
}

void vsm_socket_free(struct vsm_socket *vsm_sock)
{
	vsm_socket_close(vsm_sock);

	if (vsm_sock->server_fd >= 0) {
		close(vsm_sock->server_fd);
		vsm_sock->server_fd = -1;
	}
}

int vsm_socket_accept(struct vsm_socket *vsm_sock)
{
	socklen_t client_len = sizeof(vsm_sock->client_addr);
	int client_fd;

	client_fd = accept(vsm_sock->server_fd,
			   (struct sockaddr *) &vsm_sock->client_addr,
			   &client_len);

	if (client_fd < 0)
		return -1;

	vsm_sock->out = fdopen(client_fd, "w");
	client_fd = dup(client_fd);
	vsm_sock->read_fd = client_fd;
	vsm_sock->in = fdopen(client_fd, "r");

	if ((vsm_sock->out == NULL) || (vsm_sock->in == NULL))
		return -1;

	return 0;
}

int vsm_socket_is_open(struct vsm_socket *vsm_sock)
{
	return vsm_sock->out != NULL;
}

void vsm_socket_close(struct vsm_socket *vsm_sock)
{
	if (vsm_sock->out != NULL) {
		fclose(vsm_sock->out);
		vsm_sock->out = NULL;
	}

	if (vsm_sock->in != NULL) {
		fclose(vsm_sock->in);
		vsm_sock->in = NULL;
		vsm_sock->read_fd = -1;
	}
}

static int vsm_socket_printf(struct vsm_socket *vsm_sock, const char *fmt, ...)
{
	va_list args;
	int stat;

	va_start(args, fmt);
	stat = vfprintf(vsm_sock->out, fmt, args);
	va_end(args);

	if (stat < 0)
		return -1;

	return fflush(vsm_sock->out);
}

int vsm_socket_send_bool(struct vsm_socket *vsm_sock,
			 const char *signal, int value)
{
	return vsm_socket_printf(vsm_sock, "%s=%s\n",
				 signal, value ? "True" : "False");
}

int vsm_socket_send_int(struct vsm_socket *vsm_sock,
			const char *signal, int value)
{
	return vsm_socket_printf(vsm_sock, "%s=%d\n", signal, value);
}

int vsm_socket_send_float(struct vsm_socket *vsm_sock,
			  const char *signal, double value)
{
	return vsm_socket_printf(vsm_sock, "%s=%f\n", signal, value);
}

int vsm_socket_send_str(struct vsm_socket *vsm_sock,
			const char *signal, const char *value)
{
	return vsm_socket_printf(vsm_sock, "%s=%s\n", signal, value);
}

int vsm_socket_send(struct vsm_socket *vsm_sock, const char *msg)
{
	int stat;

	stat = fputs(msg, vsm_sock->out);

	if (stat < 0)
		return -1;

	return fflush(vsm_sock->out);
}

static char *vsm_trim(char *str)
{
	char *ret = str;

	for (ret = str; *ret == ' '; ++ret);

	for (str = ret; *str != '\0'; ++str)
		if ((*str == '\n') || (*str == ' '))
			*str = '\0';

	return ret;
}

int vsm_socket_select(int fd)
{
	fd_set fds;
	int status;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	status = select(fd + 1, &fds, NULL, NULL, NULL);

	if (status <= 0)
		return -1;

	return 0;
}

int vsm_socket_receive(struct vsm_socket *vsm_sock,
		       const char **signal, const char **value)
{
	static const char delim[] = "=";
	const int buffer_size = (int)vsm_sock->buffer_size;
	char *ptr;
	char *str;
	char *tok_signal;
	char *tok_value;

	if (vsm_sock->in == NULL)
		return -1;

	str = fgets(vsm_sock->buffer, buffer_size, vsm_sock->in);

	if (str == NULL) {
		if (feof(vsm_sock->in)) {
			*signal = *value = NULL;
			return 0;
		} else {
			return -1;
		}
	}

	tok_signal = strtok_r(str, delim, &ptr);
	tok_value = strtok_r(NULL, delim, &ptr);

	if ((tok_signal == NULL) || (tok_value == NULL))
		return -1;

	*signal = vsm_trim(tok_signal);
	*value = vsm_trim(tok_value);

	return 0;
}
