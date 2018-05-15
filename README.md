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

## Sample app: vsmsocket

A sample application `vsmsocket` is also provided and built along the
`libvsmsocket.a` static library to illustrate how to use it and for debugging
purposes.  While it can be used with a regular VSM client, netcat or `nc` is a
good way of showing what it does or testing the library.  For example, with one
shell for the server `vsmsocket` and another one for the client `nc`:

```
$ ./build/vsmsocket -p12345
```

```
$ nc localhost 12345
```

Signals entered on the server shell will be sent to the client, and the other
way round when using the client shell.

White spaces are stripped around the signal and values, but there needs to be
one equal sign.  These signal strings are valid:

```
mySignal=123
hello =  good-bye
```

However, these ones aren't valid and will raise an error:

```
hello
otherSignal someValue
```
