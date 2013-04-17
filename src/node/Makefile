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


release:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ $(INCLUDE) -o $(TARGET) $(OBJS) $(LIBS)


debug:
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o an_manager.o an_manager.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o an_namespace.o an_namespace.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o an_function.o an_function.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o nodeserver.o nodeserver.c
	$(CC) $(CFLAGS) -DDEBUG -Wl,-rpath=./ $(INCLUDE) -o $(TARGET) $(OBJS) $(LIBS)


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)


