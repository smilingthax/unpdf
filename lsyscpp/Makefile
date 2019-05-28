SOURCES1=
SOURCES2=exception.cpp filesystem.cpp exec.cpp cmdline.cpp dir_iter.cpp ftool.cpp netif.cpp net.cpp
TESTSRC1=
TESTSRC2=main.cpp
LIB1=liblsys.a
EXEC1=fstest

#CC=/export/gcc3/usr/bin/gcc
#CXX=/export/gcc3/usr/bin/g++
#FLAGS=-I/export/gcc3/usr/include/g++ -I/export/gcc3/usr/include/g++/i486-suse-linux

CFLAGS=-O3 -Wall -g
#CFLAGS+=`pkg-config --cflags libxml-2.0`
#CFLAGS=-Wall -g
#LDFLAGS=-g
CPPFLAGS:=$(CFLAGS) $(FLAGS)
CFLAGS:=

OBJECTS1=$(SOURCES1:.c=.o) $(SOURCES2:.cpp=.o)
TESTOBJECTS=$(TESTSRC1:.c=.o) $(TESTSRC2:.cpp=.o)
all: $(LIB1) $(EXEC1)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(SOURCES1:.c=.d) $(SOURCES2:.cpp=.d)
  -include $(TESTSRC1:.c=.d) $(TESTSRC2:.cpp=.d)
endif

clean:
	rm -f $(LIB1) $(OBJECTS1) $(SOURCES1:.c=.d) $(SOURCES2:.cpp=.d)
	rm -f $(EXEC1) $(TESTOBJECTS) $(TESTSRC1:.c=.d) $(TESTSRC2:.cpp=.d)

%.d: %.c
	@$(CC) $(CPPFLAGS) -MM -MT"$@" -MT"$*.o" -o $@ $<  2> /dev/null

%.d: %.cpp
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MT"$@" -MT"$*.o" -o $@ $<  2> /dev/null

$(LIB1): $(OBJECTS1)
	$(AR) rcu $@ $^

$(EXEC1): $(TESTOBJECTS) $(LIB1)
	$(CXX) -o $@ $^ $(LDFLAGS)
