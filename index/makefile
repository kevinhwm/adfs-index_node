obj = indexserver.o  Exception.o Field.o Parser.o BinString.o curl_upload.o  md5sum.o

indexserver : $(obj)
	g++ $(obj) -m64 -levent -lcrypto -llog4cpp -pthread -ljson -lkyototycoon -lcurl -lyaml-cpp -lz -o indexserver 

indexserver.o :indexserver.h
Exception.o :Exception.h
Field.o :Field.h
Parser.o :Parser.h
BinString.o :BinString.h

curl_upload.o : curl_upload.h

md5sum.o    :md5sum.h

.PHONY : clean
clean :
	rm -rf indexserver $(obj)

