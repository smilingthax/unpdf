SOURCES=
SOURCES+=pdffilter.cpp

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
LDFLAGS=-lz -lcrypto -ljpeg 

CPPFLAGS+=-I../../../g4coder -I../../../lzwcoder -I../../../lsyscpp
ADDOBJS+=../../../g4coder/g4code.o ../../../lzwcoder/lzwcode.o ../../../lsyscpp/exception.o ../../../lsyscpp/filesystem.o

include ../std.mk
