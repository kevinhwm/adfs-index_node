OBJS = curl_upload.o BinString.o Field.o Parser.o Exception.o indexserver.o

all: $(OBJS)
	g++ -m64 $(OBJS) -o indexserver -lcrypto -llog4cpp -ljson -lcurl -lyaml-cpp -lkyototycoon -levent

indexserver.o: indexserver.cpp
Exception.o: Exception.cpp
Field.o: Field.cpp
Parser.o: Parser.cpp
BinString.o: BinString.cpp
curl_upload.o: curl_upload.cpp


.PHONY: clean
clean:
	rm -rf indexserver $(OBJS)

