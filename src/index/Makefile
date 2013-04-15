#############################################
# huangtao@antiy.com
#############################################

CC = gcc
CFLAGS = -std=c99 -Wall
LIBS = -lnxweb -lkyotocabinet -lrt -lcurl

TARGET = indexserver
OBJS = multipart_parser.o ai_function.o ai_manager.o ai_connect.o indexserver.o if_upload.o


all:$(OBJS)
	$(CC) $(CFLAGS) -Wl,-rpath=./ -o $(TARGET) $(OBJS) $(LIBS)


indexserver.o:indexserver.c
multipart_parser.o:multipart_parser.c
ai_function.o:ai_function.c
ai_manager.o:ai_manager.c
ai_connect.o:ai_connect.c
if_upload.o:if_upload.c


.PHONY:clean
clean:
	rm -rf $(OBJS) $(TARGET)

