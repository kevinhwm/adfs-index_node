#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
INCLUDE = -I../include
LIBS = -lnxweb -lkyotocabinet -lrt

TARGET = nodeserver
OBJS = multipart_parser.o an_manager.o an_namespace.o an_function.o if_upload.o if_download.o nodeserver.o
#if_delete.o if_status.o


all:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ $(INCLUDE) -o $(TARGET) $(OBJS) $(LIBS) -static-libgcc 

nodeserver.o:nodeserver.c
multipart_parser.o:multipart_parser.c
an_manager.o:an_manager.c
an_namespace.o:an_namespace.c
an_function.o:an_function.c
if_upload.o:if_upload.c
if_download.o:if_download.c


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

