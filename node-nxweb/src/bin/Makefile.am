INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = -pthread $(GNUTLS_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = adfsnode
adfsnode_SOURCES = main.c post_parser/multipart_parser.c post_parser/multipart_parser.h \
	an_list.c adfsnode.c modules/upload.c modules/download.c

adfsnode_LDADD = -lnxweb -lkyotocabinet $(NXWEB_EXT_LIBS)
adfsnode_LDFLAGS = -L$(top_builddir)/src/lib
