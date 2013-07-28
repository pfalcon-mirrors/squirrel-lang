
SQUIRREL=.
MAKE=make

all:
	cd squirrel; $(MAKE) 
	cd sqlibs; $(MAKE) 
	cd sq; $(MAKE) 
