nodeweb :nodeweb.o  Exception.o Field.o Parser.o BinString.o kc_class.o
	g++ -o nodeweb -levent -llog4cpp -lkyotocabinet -lcrypto -m64 Field.o Exception.o Parser.o  nodeweb.o BinString.o kc_class.o

nodeweb.o :nodeweb.cpp
Exception.o :Exception.cpp
Field.o :Field.cpp
Parser.o :Parser.cpp
BinString.o :BinString.cpp
kc_class.o :kc_class.cpp


clean :
	rm -rf adfsnode  *.o nodeweb

