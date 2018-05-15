# libvsmsocket

This C library provides a way to remotely exchange simple signals with a VSM
instance via a TCP socket.

## Building

To generate the `Makefile` using Autotools:

```
./autogen.sh
./configure
```

Then it's `make` as usual.

## Using

First, initialise an instance to listen on a port and accept incoming
connections with `vsm_socket_init()`.  Then wait for an incoming VSM client
connection with `vsm_socket_accept()`.

The main loop is typically run in separate threads to read and write signals
with `vsm_socket_receive()` and `vsm_socket_send()` and its variants for each
primitive type.

Then call `vsm_socket_close()` to close a client connection, and finally
`vsm_socket_free()` to dispose of the `vsm_socket` instance.

Signals are exchanged in plain text using this standard format:

```
"signal=value\n"
```

To open a client connection from the VSM instance, use the `ipc.SocketIPC`
class with the hostname and port number of where the socket is listening.
