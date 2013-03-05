INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = -pthread $(GNUTLS_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = indexserver
indexserver_SOURCES = main.c modules/benchmark.c modules/hello.c modules/upload.c modules/download.c modules/subrequests.c post_parser/multipart_parser.c post_parser/multipart_parser.h
indexserver_LDADD = -lnxweb -lkyotocabinet $(NXWEB_EXT_LIBS)
indexserver_LDFLAGS = -L$(top_builddir)/src/lib
