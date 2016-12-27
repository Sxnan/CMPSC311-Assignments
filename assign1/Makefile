#
# CMPSC311 - Utility Library 
# Code for the CMPSC311 Projects
#

# Make environment
INCLUDES=-I.
CC=gcc
CFLAGS=-I. -c -g -Wall $(INCLUDES)
LINKARGS=-lm
LIBS=-lm

# Files
OBJECT_FILES=	cmpsc311-f16-assign1.o \
				cmpsc311-f16-assign1-support.o

# Productions
all : cmpsc311-f16-assign1

cmpsc311-f16-assign1 : $(OBJECT_FILES)
	$(CC) $(LINKARGS) $(OBJECT_FILES) -o $@ $(LIBS)

cmpsc311-f16-assign1.o : cmpsc311-f16-assign1.c cmpsc311-f16-assign1-support.h
	$(CC) $(CFLAGS) $< -o $@

cmpsc311-f16-assign1-support.o : cmpsc311-f16-assign1-support.c cmpsc311-f16-assign1-support.h 
	$(CC) $(CFLAGS) $< -o $@

clean : 
	rm -f cmpsc311-f16-assign1 $(OBJECT_FILES)
