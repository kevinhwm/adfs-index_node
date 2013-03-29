# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wl,-rpath=./
LIBS = -lnxweb -lkyotocabinet -lz -lrt -pthread 

TARGET = nodeserver
OBJS = nodeserver.o an.o an_list.o multipart_parser.o if_upload.o if_download.o if_delete.o if_status.o 


all:$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)



nodeserver.o:nodeserver.c
an.o:an.c
an_list.o:an_list.c
multipart_parser.o:multipart_parser.c
if_upload.o:if_upload.c
if_download.o:if_download.c
if_delete.o:if_delete.c
if_status.o:if_status.c



.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

