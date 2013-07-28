SQUIRREL= ..


OUT= $(SQUIRREL)/lib/libsqlibs.a
INCZ= -I$(SQUIRREL)/include -I. -Iinclude
LIB=	stdc++

OBJS= \
	sqbloblib.o \
	sqiolib.o \
	sqmathlib.o \
	sqsystemlib.o 
	
SRCS= \
	sqbloblib.cpp \
	sqiolib.cpp \
	sqmathlib.cpp \
	sqsystemlib.cpp
	
	
squirrel:
	gcc -O2 -c $(SRCS) $(INCZ)
	ar rc $(OUT) *.o
	rm *.o