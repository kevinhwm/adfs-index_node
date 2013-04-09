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


#nodeserver.o:nodeserver.c
#	$(CC) $(CFLAGS) -o nodeserver.o nodeserver.c -static-libgcc
#multipart_parser.o:multipart_parser.c
#	$(CC) $(CFLAGS) -o multipart_parser.o multipart_parser.c -static-libgcc
#an_manager.o:an_manager.c
#	$(CC) $(CFLAGS) -o an_manager.o an_manager.c -static-libgcc
#an_namespace.o:an_namespace.c
#	$(CC) $(CFLAGS) -o an_namespace.o an_namespace.c -static-libgcc
#an_function.o:an_function.c
#	$(CC) $(CFLAGS) -o an_function.o an_function.c -static-libgcc
#if_upload.o:if_upload.c
#	$(CC) $(CFLAGS) -o if_upload.o if_upload.c -static-libgcc
#if_download.o:if_download.c
#	$(CC) $(CFLAGS) -o if_download.o if_download.c -static-libgcc


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

