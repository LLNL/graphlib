# Copyright (c) 2007, Lawrence Livermore National Security, LLC. 
# Written by Martin Schulz and Dorian Arnold
# Contact email: schulzm@llnl.gov, LLNL-CODE-231600
# All rights reserved - please read information in "LICENCSE"


# include base file

include $(PROJECTS)/common/Makefile.common

# files

TARGET   = liblnlgraph.a
TARGET-C = liblnlgraph-c.a
INCLUDES = graphlib.h
SRCFILES = graphlib.c IntegerSet.C
OBJFILES = graphlib.o IntegerSet.o
OBJFILES-C = graphlib-c.o
DEPS     = Makefile
DEMO     = graphdemo
DEMO-C   = graphdemo-c
MERGE    = grmerge
MERGE-C  = grmerge-c


ALLTARGETS = $(TARGET) $(DEMO) $(MERGE) $(TARGET-C) $(DEMO-C) $(MERGE-C)

CC=g++
CCC=gcc
#CFLAGS += -fPIC -O3 -g
CFLAGS += -fPIC -O2 -g -DSTAT_BITVECTOR  -DGRL_DYNAMIC_NODE_NAME -DGRL_BV_CHAR
COFLAGS += -fPIC -O2 -g -DNOSET # -DGRL_DYNAMIC_NODE_NAME 
CXXFLAGS += $(CFLAGS)

# default

all: $(ALLTARGETS)

# dependencies

graphlib.o: $(SRCFILES) $(INCLUDES) $(DEPS)
graphlib-c.o: $(SRCFILES) $(INCLUDES) $(DEPS)
	$(CCC) -x c -c graphlib.c -o graphlib-c.o $(COFLAGS)
graphdemo.o: $(TARGET) $(INCLUDES) $(DEPS)
graphdemo-c.o: $(TARGET) $(INCLUDES) $(DEPS)
	$(CCC) -x c -c graphdemo.c -o graphdemo-c.o $(COFLAGS)
grmerge.o: $(TARGET) $(INCLUDES) $(DEPS)

# archive rules

$(TARGET): $(INCLUDES) $(OBJFILES)
	$(AR) $(ARFLAGS) $@ $(OBJFILES)

liblnlgraph-c.a: $(INCLUDES) $(OBJFILES-C)
	$(AR) $(ARFLAGS) $@ $(OBJFILES-C)

$(DEMO): graphdemo.o 
	$(CC) -o $@ $< $(TARGET)

$(DEMO-C): graphdemo-c.o 
	$(CC) -o $@ $< $(TARGET-C)

$(MERGE): grmerge.o
	$(CC) -o $@ $< $(TARGET)

$(MERGE-C): grmerge.o
	$(CC) -o $@ $< $(TARGET-C)

$(MERGE2): grmerge2.o
	$(CC) -o $@ $< $(TARGET)

# installation

install: $(ALLTARGETS) $(INCLUDES)
	cp $(TARGET) $(LIBDIR)
	ranlib $(LIBDIR)/$(TARGET)
	cp $(TARGET-C) $(LIBDIR)
	ranlib $(LIBDIR)/$(TARGET-C)
	cp $(MERGE) $(BINDIR)
	cp $(MERGE-C) $(BINDIR)
	cp $(INCLUDES) $(INCDIR)

# cleanup

clean:
	rm -f *.o *.a $(ALLTARGETS)

democlean: clean
	rm -f *.grl *.gml *.dot *.pdot

clobber: democlean
	rm -f *~

