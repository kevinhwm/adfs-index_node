#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
LIBS = -lnxweb -lkyotocabinet -lrt -lcurl -pthread
TARGET = indexserver
OBJS = conf.o connect.o function.o if_upload.o if_download.o if_delete.o if_status.o indexserver.o manager.o multipart_parser.o record.o zone.o


release:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)

debug:
	$(CC) $(CFLAGS) -DDEBUG -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_delete.o if_delete.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_status.o if_status.c
	$(CC) $(CFLAGS) -DDEBUG -c -o manager.o manager.c
	$(CC) $(CFLAGS) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) -DDEBUG -c -o record.o record.c
	$(CC) $(CFLAGS) -DDEBUG -c -o zone.o zone.c
	$(CC) $(CFLAGS) -DDEBUG -c -o conf.o conf.c
	$(CC) $(CFLAGS) -DDEBUG -c -o connect.o connect.c
	$(CC) $(CFLAGS) -DDEBUG -c -o function.o function.c
	$(CC) $(CFLAGS) -DDEBUG -c -o indexserver.o indexserver.c
	$(CC) $(CFLAGS) -DDEBUG -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

