OBJS=Exception.o Field.o Parser.o kc_class.o nodeweb.o

all: $(OBJS)
	g++ -m64 $(OBJS) -o nodeweb -levent -lkyotocabinet 

nodeweb.o: nodeweb.cpp
Exception.o: Exception.cpp
Field.o: Field.cpp
Parser.o: Parser.cpp
kc_class.o: kc_class.cpp


.PHONY: clean
clean :
	rm -rf adfsnode  *.o nodeweb

