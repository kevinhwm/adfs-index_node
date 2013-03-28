#
#
#

CC = gcc
CFLAGS = -std=c99 
DIR_INC = 
DIR_LIBS = 
LIBS = -lnxweb -lkyotocabinet -lz -lrt -pthread 

TARGET = nodeserver
OBJS = nodeserver.o an.o an_list.o function.o multipart_parser.o if_upload.o if_download.o if_delete.o if_status.o 



all:$(OBJS)
	$(CC) $(CFLAGS) $(DIR_INC) -o $(TARGET) $(OBJS) $(DIR_LIBS) $(LIBS)



nodeserver.o:nodeserver.c
an.o:an.c
an_list.o:an_list.c
function.o:function.c
multipart_parser.o:multipart_parser.c
if_upload.o:if_upload.c
if_download.o:if_download.c
if_delete.o:if_delete.c
if_status.o:if_status.c



.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

