SOURCES=base.cpp file.cpp mem.cpp sub.cpp ptr.cpp crypt.cpp
SOURCES+=pdfio.cpp

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
LDFLAGS=

CPPFLAGS+=-I../../../lsyscpp
ADDOBJS+=../../../lsyscpp/exception.o ../../../lsyscpp/filesystem.o

include ../std.mk
