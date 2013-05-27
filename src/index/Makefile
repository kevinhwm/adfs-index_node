#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
LIBS = -lnxweb -lkyotocabinet -lrt -lcurl -pthread
TARGET = indexserver
OBJS = if_upload.o if_download.o if_delete.o if_status.o \
       ai_record.o ai_zone.o ai_connect.o ai_stat.o ai_manager.o \
       conf.o log.o multipart_parser.o indexserver.o


release:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o $(TARGET) $(OBJS) $(LIBS)

debug:
	$(CC) $(CFLAGS) -DDEBUG -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_delete.o if_delete.c
	$(CC) $(CFLAGS) -DDEBUG -c -o if_status.o if_status.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_record.o ai_record.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_zone.o ai_zone.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_connect.o ai_connect.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_stat.o ai_stat.c
	$(CC) $(CFLAGS) -DDEBUG -c -o ai_manager.o ai_manager.c
	$(CC) $(CFLAGS) -DDEBUG -c -o conf.o conf.c
	$(CC) $(CFLAGS) -DDEBUG -c -o log.o log.c
	$(CC) $(CFLAGS) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) -DDEBUG -c -o indexserver.o indexserver.c
	$(CC) $(CFLAGS) -DDEBUG -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)

.PHONY:install
install:
	cp indexserver /usr/local/bin

.PHONY:uninstall
uninstall:
	rm -rf /usr/local/bin/indexserver

.PHONY:clean
clean:
	-rm -rf $(OBJS) $(TARGET)

