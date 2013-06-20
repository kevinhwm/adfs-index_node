#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
INCLUDE=-I/usr/local/include/ 
LIBS = -lnxweb -lkyotocabinet -lrt -lm -lcurl -pthread
TARGET = indexserver
OBJS = if_upload.o if_download.o if_delete.o if_exist.o if_status.o \
       ai_record.o ai_zone.o ai_connect.o ai_stat.o ai_manager.o ai_function.o \
       cJSON.o log.o multipart_parser.o indexserver.o


release:$(OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib/ -o $(TARGET) $(OBJS) $(LIBS)

debug:
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_delete.o if_delete.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_exist.o if_exist.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_status.o if_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_record.o ai_record.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_zone.o ai_zone.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_connect.o ai_connect.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_stat.o ai_stat.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_manager.o ai_manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_function.o ai_function.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o cJSON.o cJSON.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o log.o log.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o indexserver.o indexserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o $(TARGET) $(OBJS) $(LIBS)

.PHONY:install
install:
	mkdir -p /usr/local/adfs/index
	cp indexserver.conf /usr/local/adfs/index
	cp indexserver /usr/local/bin

.PHONY:uninstall
uninstall:
	rm -rf /usr/local/adfs/index
	rm -f /usr/local/bin/indexserver

.PHONY:clean
clean:
	-rm -rf $(OBJS) $(TARGET)

