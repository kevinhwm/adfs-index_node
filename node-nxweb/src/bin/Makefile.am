INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = -pthread $(GNUTLS_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = nodeserver
nodeserver_SOURCES = main.c post_parser/multipart_parser.c post_parser/multipart_parser.h \
	an_list.c adfsnode.c modules/upload.c modules/download.c

nodeserver_LDADD = -lnxweb -lkyotocabinet $(NXWEB_EXT_LIBS)
nodeserver_LDFLAGS = -L$(top_builddir)/src/lib
