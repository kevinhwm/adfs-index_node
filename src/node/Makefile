#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall -D_GNU_SOURCE
INCLUDE = -I../include
LIBS = -lnxweb -lkyotocabinet -lrt -lpthread
TARGET = nodeserver
OBJS = if_upload.o if_download.o if_erase.o if_status.o \
       an_namespace.o an_manager.o \
       conf.o log.o multipart_parser.o nodeserver.o


release:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib $(INCLUDE) -o $(TARGET) $(OBJS) $(LIBS)


debug:
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_upload.o if_upload.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_download.o if_download.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_erase.o if_erase.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o if_status.o if_status.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o an_manager.o an_manager.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o an_namespace.o an_namespace.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o conf.o conf.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o log.o log.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) -DDEBUG $(INCLUDE) -c -o nodeserver.o nodeserver.c
	$(CC) $(CFLAGS) -DDEBUG -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib $(INCLUDE) -o $(TARGET) $(OBJS) $(LIBS)

.PHONY:install
install:
	mkdir -p /usr/local/adfs/node
	cp nodeserver.conf /usr/local/adfs/node
	cp nodeserver /usr/local/bin

.PHONY:uninstall
uninstall:
	rm -f /usr/local/bin/nodeserver
	rm -rf /usr/local/adfs/node

.PHONY:clean
clean:
	-rm -rf $(OBJS) $(TARGET)


