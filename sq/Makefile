SQUIRREL= ..


OUT= $(SQUIRREL)/bin/sq
INCZ= -I$(SQUIRREL)/include -I. -I$(SQUIRREL)/sqlibs
LIBZ= -L$(SQUIRREL)/lib 
LIB= -lsquirrel -lsqlibs -lstdc++

OBJS= sq.o
	
SRCS= sq.c
	
	
sq:
	g++ -O2 -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)
	