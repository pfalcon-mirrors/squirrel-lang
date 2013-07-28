
SQUIRREL=.
MAKE=make

all:
	cd squirrel; $(MAKE) 
	cd sqstdlib; $(MAKE) 
	cd sq; $(MAKE) 

win:
	cd squirrel; $(MAKE) 
	cd sqstdlib; $(MAKE) 
	cd sq; $(MAKE) win
