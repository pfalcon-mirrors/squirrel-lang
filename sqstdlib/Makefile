SQUIRREL= ..


OUT= $(SQUIRREL)/lib/libsqstdlib.a
INCZ= -I$(SQUIRREL)/include -I. -Iinclude

SRCS= \
	sqstdblob.cpp \
	sqstdio.cpp \
	sqstdstream.cpp \
	sqstdmath.cpp \
	sqstdsystem.cpp \
	sqstdstring.cpp \
	sqstdmodule.cpp
	
	
squirrel:
	gcc -O2 -c $(SRCS) $(INCZ)
	ar rc $(OUT) *.o
	rm *.o