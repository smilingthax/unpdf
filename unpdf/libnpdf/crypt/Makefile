SOURCES=md5.cpp rc4.cpp sha256.cpp aescbc.cpp rand.cpp

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
LDFLAGS=-lcrypto

CPPFLAGS+=-I../../../lsyscpp
ADDOBJS+=../../../lsyscpp/exception.o ../../../lsyscpp/filesystem.o

include ../std.mk
