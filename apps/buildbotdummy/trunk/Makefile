all: buildbotdummy.o
	gcc -o buildbotdummy buildbotdummy.o

buildbotdummy.o:
	gcc -c buildbotdummy.c

clean:
	rm -f buildbotdummy buildbotdummy.o

install:
	dh_install buildbotdummy /usr/bin
