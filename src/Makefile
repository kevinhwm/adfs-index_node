#############################################
# kevinhwm@gmail.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall -D_GNU_SOURCE
INCLUDE = -I/usr/local/include/ 
LIBS = -lnxweb -lkyotocabinet -lrt -lm -lpcre -lcurl -pthread

idx_OBJS = function.o conf.o cJSON.o log.o multipart_parser.o \
	   if_upload.o if_download.o if_delete.o if_exist.o if_status.o \
	   i_record.o i_zone.o i_connect.o i_update.o i_manager.o \
	   indexserver.o

nd_OBJS = function.o conf.o cJSON.o log.o multipart_parser.o \
	  nf_upload.o nf_download.o nf_erase.o nf_status.o \
	  n_namespace.o n_update.o n_manager.o \
	  nodeserver.o

release:
	$(CC) $(CFLAGS) $(INCLUDE) -c -o function.o function.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o conf.o conf.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o cJSON.o cJSON.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o log.o log.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_upload.o index/f_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_download.o index/f_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_delete.o index/f_delete.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_exist.o index/f_exist.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_status.o index/f_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_record.o index/record.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_zone.o index/zone.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_connect.o index/connect.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_manager.o index/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o i_update.o index/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o indexserver.o index/indexserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib/ -o indexserver $(idx_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nf_upload.o node/f_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nf_download.o node/f_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nf_erase.o node/f_erase.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nf_status.o node/f_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o n_manager.o node/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o n_namespace.o node/namespace.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o n_update.o node/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nodeserver.o node/nodeserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o nodeserver $(nd_OBJS) $(LIBS)
	mkdir -p bin
	cp indexserver index/indexserver.json bin
	cp nodeserver node/nodeserver.json bin

debug:
	mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o function.o function.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o conf.o conf.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o cJSON.o cJSON.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o log.o log.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_upload.o index/f_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_download.o index/f_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_delete.o index/f_delete.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_exist.o index/f_exist.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_status.o index/f_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_record.o index/record.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_zone.o index/zone.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_connect.o index/connect.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_manager.o index/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o i_update.o index/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o indexserver.o index/indexserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o indexserver $(idx_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nf_upload.o node/f_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nf_download.o node/f_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nf_erase.o node/f_erase.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nf_status.o node/f_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o n_manager.o node/manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o n_namespace.o node/namespace.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o n_update.o node/update.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o nodeserver.o node/nodeserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o nodeserver $(nd_OBJS) $(LIBS)
	mkdir -p bin
	cp indexserver index/indexserver.json bin
	cp nodeserver node/nodeserver.json bin

.PHONY:install
install:
	mkdir -p /usr/local/adfs/index
	cp bin/indexserver.json /usr/local/adfs/index
	cp bin/indexserver /usr/local/bin
	mkdir -p /usr/local/adfs/node
	cp bin/nodeserver.json /usr/local/adfs/node
	cp bin/nodeserver /usr/local/bin

.PHONY:uninstall
uninstall:
	-rm -f /usr/local/bin/indexserver
	-rm -f /usr/local/bin/nodeserver
	-rm /usr/local/bin/adfs/index/indexserver.json
	-rm /usr/local/bin/adfs/node/nodeserver.json
	-rmdir /usr/local/bin/adfs/index
	-rmdir /usr/local/bin/adfs/node
	-rmdir /usr/local/bin/adfs

.PHONY:clean
clean:
	-rm -rf bin $(idx_OBJS) $(nd_OBJS) indexserver nodeserver

