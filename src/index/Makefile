#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
INCLUDE = -I../include
LIBS = -lnxweb -lkyotocabinet -luuid -lrt -lcurl -pthread
TARGET = indexserver
OBJS = conf.o connect.o function.o if_delete.o if_download.o if_upload.o indexserver.o manager.o multipart_parser.o zone.o


release:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)

debug:
	$(CC) $(CFLAGS) -DDEBUG -c -o conf.o conf.c
	$(CC) $(CFLAGS) -DDEBUG -c -o connect.o connect.c
	$(CC) $(CFLAGS) -DDEBUG -c -o function.o function.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_delete.o if_delete.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) -DDEBUG -c -o indexserver.o indexserver.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_manager.o ai_manager.c
	$(CC) $(CFLAGS) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) -DDEBUG -c -o zone.o zone.c
	$(CC) $(CFLAGS) -DDEBUG -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

