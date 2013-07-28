SQUIRREL= ..


OUT= $(SQUIRREL)/bin/sq
INCZ= -I$(SQUIRREL)/include -I. -I$(SQUIRREL)/sqlibs
LIBZ= -L$(SQUIRREL)/lib 
LIB= -lsquirrel -lsqstdlib

OBJS= sq.o
	
SRCS= sq.c
	
	
sq:
	g++ -O3 -fno-rtti -Os -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)
	
