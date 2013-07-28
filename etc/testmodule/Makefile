SQUIRREL= ../..


OUT= $(SQUIRREL)/modules/libsq_testmodule.so
INCZ= -I$(SQUIRREL)/include -I. -Iinclude

testmodule:
	gcc -O2 -w -c -fPIC $(INCZ) -o testmodule.o testmodule.c
	gcc -o $(OUT) -shared testmodule.o
	rm *.o