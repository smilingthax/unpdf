SOURCES=ctype.cpp
SOURCES+=pdfparse.cpp  # fixme

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
LDFLAGS=

CPPFLAGS+=-I../../../lsyscpp
ADDOBJS+=../../../lsyscpp/exception.o ../../../lsyscpp/filesystem.o

include ../std.mk
