/*
  Copyright (C) 2018 Jaguar Land Rover

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Authors:
    Guillaume Tucker <guillaume.tucker@collabora.com>
*/

#ifndef INCLUDE_VSM_SOCKET_H
#define INCLUDE_VSM_SOCKET_H 1

#include <netinet/in.h>

/* VSM connection instance data
 *
 * This structure should only be used via the functions defined below.
 */
struct vsm_socket {
	int server_fd;
	int client_fd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	char *buffer;
	size_t buffer_size;
};

/* Initialise the TCP socket to accept VSM connections
 *
 * Initialise the given vsm_socket structure with a listening socket on the
 * given port number and the given buffer used in data transfers.
 */
extern int vsm_socket_init(struct vsm_socket *vsm_sock, unsigned port,
			   char *buffer, size_t buffer_size);

/* Wait for an incoming VSM connection and accept it
 *
 * Wait on the listening socket until a VSM incoming connection is made, accept
 * it and return.  Only one VSM client connection is supported.
 */
extern int vsm_socket_accept(struct vsm_socket *vsm_sock);

/* Close the VSM connection
 *
 * Close any open sockets and free any associated resources.  The data buffer
 * being managed by the caller, it will not be freed by this function.
 */
extern void vsm_socket_close(struct vsm_socket *vsm_sock);

/* Send a boolean signal to the VSM client
 *
 * Send a signal with the given name and boolean value to the active VSM
 * client.
 */
extern int vsm_socket_send_bool(struct vsm_socket *vsm_sock,
				const char *signal, int value);

/* Send any arbitrary data currently stored in the buffer
 *
 * For any other type of signal not covered by the generic functions provided
 * here, this can be used to send an arbitrary string stored in the connection
 * buffer.
 */
extern int vsm_socket_send(struct vsm_socket *vsm_sock);

#endif /* INCLUDE_VSM_SOCKET_H */
