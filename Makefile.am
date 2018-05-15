AM_CPPFLAGS = -I$(top_srcdir)/libvsmsocket

noinst_LIBRARIES := build/libvsmsocket.a
build_libvsmsocket_a_SOURCES := libvsmsocket/vsm_socket.c

bin_PROGRAMS := build/vsmsocket
build_vsmsocket_SOURCES := vsmsocket/vsmsocket.c
build_vsmsocket_LDADD := build/libvsmsocket.a
