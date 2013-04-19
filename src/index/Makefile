#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
INCLUDE = -I../include
LIBS = -lnxweb -lkyotocabinet -lrt -lcurl -pthread
TARGET = indexserver
OBJS = indexserver.o multipart_parser.o ai_manager.o ai_zone.o ai_function.o ai_connect.o if_upload.o if_download.o


release:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)

debug:
	$(CC) $(CFLAGS) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_manager.o ai_manager.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_zone.o ai_zone.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_function.o ai_function.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_connect.o ai_connect.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) -DDEBUG -c -o indexserver.o indexserver.c
	$(CC) $(CFLAGS) -DDEBUG -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

