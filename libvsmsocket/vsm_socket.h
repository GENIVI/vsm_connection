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
#include <stdio.h>

/* VSM connection instance data
 *
 * This structure should only be used via the functions defined below.
 */
struct vsm_socket {
	int server_fd;
	int read_fd;
	FILE *out;
	FILE *in;
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

/* Free all open resources
 *
 * Close any open client connection, free all resources associated with the
 * vsm_socket instance and close the server socket.  The buffer will not be
 * freed by this function as it was provided by the caller.
 */
extern void vsm_socket_free(struct vsm_socket *vsm_sock);

/* Wait for an incoming VSM connection and accept it
 *
 * Wait on the listening socket until a VSM incoming connection is made, accept
 * it and return.  Only one VSM client connection is supported.
 */
extern int vsm_socket_accept(struct vsm_socket *vsm_sock);

/* Check if the VSM client is connected
 *
 * Only one client can be connected at any given point in time.  This can be
 * used to check whether a client connection has been accepted and has not been
 * closed yet.
 */
extern int vsm_socket_is_open(struct vsm_socket *vsm_sock);

/* Close the VSM client connection
 *
 * Close any open client socket and free any associated resources.  The server
 * socket will still be listening and ready to accept new incoming client
 * connections.
 */
extern void vsm_socket_close(struct vsm_socket *vsm_sock);

/* Send a boolean signal to the VSM client
 *
 * Send a signal with the given name and boolean value to the active VSM
 * client.
 */
extern int vsm_socket_send_bool(struct vsm_socket *vsm_sock,
				const char *signal, int value);

/* Send an integer signal to the VSM client
 *
 * Send a signal with the given name and integer value to the active VSM
 * client.
 */
extern int vsm_socket_send_int(struct vsm_socket *vsm_sock,
			       const char *signal, int value);

/* Send a floating point signal to the VSM client
 *
 * Send a signal with the given name and floating point value to the active VSM
 * client.
 */
extern int vsm_socket_send_float(struct vsm_socket *vsm_sock,
				 const char *signal, double value);

/* Send a string signal to the VSM client
 *
 * Send a signal with the given name and string value to the active VSM client.
 */
extern int vsm_socket_send_str(struct vsm_socket *vsm_sock,
			       const char *signal, const char *value);

/* Send any arbitrary string
 *
 * For any other type of signal not covered by the generic functions provided
 * here, this can be used to send an arbitrary string to the VSM client.
 */
extern int vsm_socket_send(struct vsm_socket *vsm_sock, const char *msg);

/* Call the libc select() function to wait for incoming connection or data
 *
 * This is primarily useful in a multi-threaded context as it doesn't need to
 * be protected by a lock.  When not using threads, this can be ignored.
 *
 * First get the value of either vsm_sock->server_fd or vsm_sock->client_fd
 * while holding a lock to guarantee the instance is not being freed at the
 * same time.  Then call vsm_socket_select with the chosen file descriptor,
 * without the need to hold a lock.  If the file descriptor got closed by
 * another thread, it will return -1.  Then vsm_socket_select() returns, hold
 * the lock again to call other vsm_socket_ functions.
 *
 * Use server_fd to wait for an incoming connection before calling
 * vsm_sock_accept(), or with read_fd after accepting a client connection to
 * wait for any incoming data before calling vsm_socket_receive().
 */
extern int vsm_socket_select(int fd);

/* Receive a signal and its value
 *
 * Wait for some input data and parse it to return a signal, value pair.  The
 * input data needs to follow this format:
 *
 *   "signal=value\n"
 *
 * The signal and value will be kept in the internal buffer of the vsm_socket
 * instance.  If the client connection is closed, the function will return 0
 * with signal and value pointers set to NULL.
 */
extern int vsm_socket_receive(struct vsm_socket *vsm_sock,
			      const char **signal, const char **value);

/* Read a signal from an arbitrary FILE input stream
 *
 * Do the same as vsm_socket_receive but using an arbitrary FILE input stream
 * and buffer rather than a vsm_socket instance.  This is primarily useful when
 * reading signals from an alternative input stream, such as stdin.
 */
extern int vsm_socket_fread(FILE *in, char *buffer, size_t buffer_size,
			    const char **signal, const char **value);

#endif /* INCLUDE_VSM_SOCKET_H */
