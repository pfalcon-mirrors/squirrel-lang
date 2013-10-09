#
# This is better makefile for Squirrel, part of General-Purpose Squirrel
# project. The trick is that GNU Make looks for "makefile", then "Makefile",
# so this file overrides upstream Makefile (which is not ideal and laeft in
# place only to make it easier merge with upstream). This however means
# that there may be problems with case-insensitive file systems, so don't
# use such, use FSes with proper POSIX semantics.
#
# This file support specifying compile and link - type options in one places,
# easy overriding of them, and supporting build of different versions of
# targets with different options - in different dirs, using VPATH feature.
#

TOP ?= .
OPT = -s -O2
# Some Squirrel compile-time options:
# -DNO_GARBAGE_COLLECTOR
# -DNO_COMPILER
# -D_DEBUG_DUMP
INCLUDE = -I$(TOP)/include $(INCLUDE_EXTRA)
CFLAGS = $(OPT) $(OPT_EXTRA) $(INCLUDE) -fno-strict-aliasing -fno-unwind-tables -fno-asynchronous-unwind-tables
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

SQUIRREL_OBJS = \
squirrel/sqapi.o \
squirrel/sqbaselib.o \
squirrel/sqclass.o \
squirrel/sqcompiler.o \
squirrel/sqdebug.o \
squirrel/sqfuncstate.o \
squirrel/sqlexer.o \
squirrel/sqmem.o \
squirrel/sqobject.o \
squirrel/sqstate.o \
squirrel/sqtable.o \
squirrel/sqvm.o

SQSTDLIB_OBJS = \
sqstdlib/sqstdaux.o \
sqstdlib/sqstdblob.o \
sqstdlib/sqstdio.o \
sqstdlib/sqstdmath.o \
sqstdlib/sqstdrex.o \
sqstdlib/sqstdstream.o \
sqstdlib/sqstdstring.o \
sqstdlib/sqstdsystem.o

SQ_OBJS = sq/sq.o $(SQUIRREL_OBJS) $(SQSTDLIB_OBJS)

# Override of builtin make rules to properly support VPATHed builds
# (create intermediate directory for target files).
%.o: %.c
	mkdir -p $$(dirname $@)
	$(COMPILE.c) $(OUTPUT_OPTION) $<
%.o: %.cpp
	mkdir -p $$(dirname $@)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

# LINK.SQRL is there to make it easier to reuse when building interpreter
# with different names.
# -lm is needed for some libc's.
LINK.SQRL = $(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) -lm
sqrl: $(SQ_OBJS) $(SQ_OBJS_EXTRA)
	$(LINK.SQRL)

clean:
	rm -f $(SQ_OBJS) $(SQ_OBJS_EXTRA)
