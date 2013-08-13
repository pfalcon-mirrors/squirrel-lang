TOP = ../..
VPATH = $(TOP)

# For VPATH to work as expected, there should be NO compiled
# objects in VPATH directory, so let's force clean them.
dummy := $(shell cd $(TOP); make -f makefile clean)

include $(TOP)/makefile
