#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
LIBS = -lnxweb -lkyotocabinet -lrt

TARGET = indexserver
OBJS = multipart_parser.o indexserver.o if_upload.o


all:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)


indexserver.o:indexserver.c
multipart_parser.o:multipart_parser.c
if_upload.o:if_upload.c


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

