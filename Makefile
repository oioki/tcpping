all: tcpping


tcpping: tcpping.o
	cc tcpping.o -lm -o tcpping

tcpping.o: tcpping.c
	cc -c tcpping.c


debug:
	cc -ggdb -c tcpping.c
	cc tcpping.o -lm -o tcpping


clean:
	rm -f *.o tcpping

install: tcpping
	cp tcpping /usr/local/bin/
