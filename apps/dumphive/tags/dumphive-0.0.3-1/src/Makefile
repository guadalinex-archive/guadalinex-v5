all: clean
	fpc -Sm -XD dumphive.dpr
    
debug: clean
	fpc -Sm -XD -d_incdebug_ dumphive.dpr

clean:
	rm -f *.o *.ppu *.dcu *.~?? dumphive

distclean: clean
	rm -f dumphive dumphive.exe
