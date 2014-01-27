#############################################
# kevinhwm@gmail.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall -D_GNU_SOURCE
INCLUDE = -I/usr/local/include/ 
LIBS = -lnxweb -lkyotocabinet -lrt -lm -lpcre -lcurl -pthread

idx_OBJS = function.o conf.o cJSON.o log.o multipart_parser.o \
	   ihttp_upload.o ihttp_download.o ihttp_exist.o ihttp_length.o ihttp_delete.o ihttp_setnode.o ihttp_status.o \
	   i_meta.o i_zone.o i_connect.o i_update.o i_manager.o \
	   indexserver.o

nd_OBJS = function.o conf.o cJSON.o log.o multipart_parser.o \
	  nhttp_upload.o nhttp_download.o nhttp_length.o nhttp_erase.o nhttp_status.o \
	  n_namespace.o n_update.o n_manager.o \
	  nodeserver.o

release:
	$(CC) $(CFLAGS) $(INCLUDE) -c -o function.o function.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o conf.o conf.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o cJSON.o cJSON.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o log.o log.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ihttp_upload.o index/http_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ihttp_download.o index/http_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ihttp_exist.o index/http_exist.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ihttp_length.o index/http_length.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ihttp_delete.o index/http_delete.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ihttp_setnode.o index/http_setnode.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ihttp_status.o index/http_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_meta.o index/meta.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_zone.o index/zone.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_connect.o index/connect.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_manager.o index/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_update.o index/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o indexserver.o index/indexserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib/ -o indexserver $(idx_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nhttp_upload.o node/http_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nhttp_download.o node/http_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nhttp_length.o node/http_length.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nhttp_erase.o node/http_erase.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nhttp_status.o node/http_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o n_manager.o node/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o n_namespace.o node/namespace.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o n_update.o node/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nodeserver.o node/nodeserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o nodeserver $(nd_OBJS) $(LIBS)
	mkdir -p bin
	cp indexserver index/indexserver.conf bin
	cp nodeserver node/nodeserver.conf bin

debug:
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o function.o function.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o conf.o conf.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o cJSON.o cJSON.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o log.o log.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ihttp_upload.o index/http_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ihttp_download.o index/http_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ihttp_length.o index/http_length.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ihttp_exist.o index/http_exist.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ihttp_delete.o index/http_delete.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ihttp_setnode.o index/http_setnode.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ihttp_status.o index/http_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_meta.o index/meta.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_zone.o index/zone.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_connect.o index/connect.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_manager.o index/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_update.o index/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o indexserver.o index/indexserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o indexserver $(idx_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nhttp_upload.o node/http_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nhttp_download.o node/http_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nhttp_length.o node/http_length.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nhttp_erase.o node/http_erase.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nhttp_status.o node/http_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o n_manager.o node/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o n_namespace.o node/namespace.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o n_update.o node/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nodeserver.o node/nodeserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o nodeserver $(nd_OBJS) $(LIBS)
	mkdir -p bin
	cp indexserver index/indexserver.conf bin
	cp nodeserver node/nodeserver.conf bin


.PHONY:install
install:
	mkdir -p /usr/local/adfs/index
	cp bin/indexserver.conf /usr/local/adfs/index
	cp bin/indexserver /usr/local/bin
	mkdir -p /usr/local/adfs/node
	cp bin/nodeserver.conf /usr/local/adfs/node
	cp bin/nodeserver /usr/local/bin

.PHONY:uninstall
uninstall:
	-rm -f /usr/local/bin/indexserver
	-rm -f /usr/local/bin/nodeserver
	-rm /usr/local/bin/adfs/index/indexserver.conf
	-rm /usr/local/bin/adfs/node/nodeserver.conf
	-rmdir /usr/local/bin/adfs/index
	-rmdir /usr/local/bin/adfs/node
	-rmdir /usr/local/bin/adfs

.PHONY:clean
clean:
	-rm -rf bin $(idx_OBJS) $(nd_OBJS) indexserver nodeserver

