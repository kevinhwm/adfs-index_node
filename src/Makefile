#############################################
# kevinhwm@gmail.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall -D_GNU_SOURCE
INCLUDE = -I/usr/local/include/ 
LIBS = -lnxweb -lkyotocabinet -lrt -lm -lpcre -lcurl -pthread

idx_OBJS = function.o cJSON.o log.o multipart_parser.o \
	   if_upload.o if_download.o if_delete.o if_exist.o if_status.o \
	   ai_record.o ai_zone.o ai_connect.o ai_stat.o ai_manager.o indexserver.o

nd_OBJS = function.o cJSON.o log.o multipart_parser.o \
	  if_upload.o if_download.o if_erase.o if_status.o \
	  an_namespace.o an_manager.o nodeserver.o

release:
	$(CC) $(CFLAGS) $(INCLUDE) -c -o function.o function.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o cJSON.o cJSON.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o log.o log.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_upload.o index/if_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_download.o index/if_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_delete.o index/if_delete.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_exist.o index/if_exist.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_status.o index/if_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ai_record.o index/ai_record.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ai_zone.o index/ai_zone.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ai_connect.o index/ai_connect.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ai_stat.o index/ai_stat.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o ai_manager.o index/ai_manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o indexserver.o index/indexserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib/ -o indexserver $(idx_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_upload.o node/if_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_download.o node/if_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_erase.o node/if_erase.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o if_status.o node/if_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o an_manager.o node/an_manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o an_namespace.o node/an_namespace.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o nodeserver.o node/nodeserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o nodeserver $(nd_OBJS) $(LIBS)
	mkdir -p bin
	cp indexserver index/indexserver.conf bin
	cp nodeserver node/nodeserver.conf bin

debug:
	mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o function.o function.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o cJSON.o cJSON.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o log.o log.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o multipart_parser.o multipart_parser.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_upload.o index/if_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_download.o index/if_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_delete.o index/if_delete.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_exist.o index/if_exist.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_status.o index/if_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_record.o index/ai_record.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_zone.o index/ai_zone.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_connect.o index/ai_connect.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_stat.o index/ai_stat.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o ai_manager.o index/ai_manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o indexserver.o index/indexserver.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -Wl,-rpath=./ -Wl,-rpath=/usr/local/lib -o indexserver $(idx_OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_upload.o node/if_upload.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_download.o node/if_download.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_erase.o node/if_erase.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o if_status.o node/if_status.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o an_manager.o node/an_manager.c
	$(CC) $(CFLAGS) $(INCLUDE) -DDEBUG -c -o an_namespace.o node/an_namespace.c
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

