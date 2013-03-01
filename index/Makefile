obj = indexserver.o  Exception.o Field.o Parser.o curl_upload.o BinString.o 
#md5sum.o

indexserver : $(obj)
	gcc $(obj) -m64 -levent -lcrypto -llog4cpp -pthread -ljson -lkyototycoon -lcurl -lyaml-cpp -lz -o indexserver 

indexserver.o :indexserver.cpp
Exception.o :Exception.cpp
Field.o :Field.cpp
Parser.o :Parser.cpp
BinString.o :BinString.cpp

curl_upload.o : curl_upload.cpp

#md5sum.o    :md5sum.h

.PHONY : clean
clean :
	rm -rf indexserver $(obj)

