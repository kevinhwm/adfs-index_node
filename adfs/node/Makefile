# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wl,-rpath=./
LIBS = -lnxweb -lkyotocabinet -lrt

TARGET = nodeserver
OBJS = nodeserver.o an_manager.o an_namespace.o multipart_parser.o if_upload.o if_download.o 
#if_delete.o if_status.o 


all:$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)



nodeserver.o:nodeserver.c
an_manager.o:an_manager.c
an_namespace.o:an_namespace.c
multipart_parser.o:multipart_parser.c
if_upload.o:if_upload.c
if_download.o:if_download.c
#if_delete.o:if_delete.c
#if_status.o:if_status.c



.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

