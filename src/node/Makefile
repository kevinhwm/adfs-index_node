#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
INCLUDE = -I../include
LIBS = -lnxweb -lkyotocabinet -lrt
TARGET = nodeserver
OBJS = multipart_parser.o manager.o namespace.o function.o if_upload.o if_download.o if_status.o nodeserver.o


release:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ $(INCLUDE) -o $(TARGET) $(OBJS) $(LIBS)


debug:
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o manager.o manager.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o namespace.o namespace.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o function.o function.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_status.o if_status.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o nodeserver.o nodeserver.c
	$(CC) $(CFLAGS) -DDEBUG -Wl,-rpath=./ $(INCLUDE) -o $(TARGET) $(OBJS) $(LIBS)


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)


