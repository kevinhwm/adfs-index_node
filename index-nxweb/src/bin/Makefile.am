INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = -pthread $(GNUTLS_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = indexserver
indexserver_SOURCES = main.c post_parser/multipart_parser.c post_parser/multipart_parser.h \
					  modules/hello.c modules/upload.c modules/uploadkey.c modules/download.c \
					  modules/delete.c modules/status.c modules/config.c modules/manage.c \
					  modules/retnNode.c modules/retn.c modules/monitor.c modules/view.c \
					  modules/history.c modules/gethistory.c modules/exists.c


indexserver_LDADD = -lnxweb -lkyotocabinet $(NXWEB_EXT_LIBS)
indexserver_LDFLAGS = -L$(top_builddir)/src/lib
