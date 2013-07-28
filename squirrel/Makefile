SQUIRREL= ..


OUT= $(SQUIRREL)/lib/libsquirrel.a
INCZ= -I$(SQUIRREL)/include -I. -Iinclude
LIB=	stdc++

OBJS= \
	sqapi.o \
	sqbaselib.o \
	sqcompiler.o \
	sqdebug.o \
	sqlexer.o \
	sqobject.o \
	sqparser.o \
	sqstate.o \
	sqtable.o \
	sqvm.o \
	stdafx.o 
	
SRCS= \
	sqapi.cpp \
	sqbaselib.cpp \
	sqfuncstate.cpp \
	sqdebug.cpp \
	sqlexer.cpp \
	sqobject.cpp \
	sqcompiler.cpp \
	sqstate.cpp \
	sqtable.cpp \
	sqvm.cpp

	
	
squirrellib:
	gcc -O2 -c $(SRCS) $(INCZ)
	ar rc $(OUT) *.o
	rm *.o