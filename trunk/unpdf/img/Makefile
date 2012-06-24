SOURCES=main.cpp 
EXEC=imgtopng

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g -I.. -I../../lsyscpp
LDFLAGS=-L../libnpdf/ -lnpdf -lcrypto -lz -ljpeg
LDFLAGS+=-llept -lpng -ltiff

OBJECTS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).o,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).o,\
$(SOURCES)))
DEPENDS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).d,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).d,\
        $(filter-out %.o,""\
$(SOURCES))))

all: $(EXEC) $(LIB)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(DEPENDS)
endif 

clean:
	rm -f $(EXEC) $(OBJECTS) $(DEPENDS) 

%.d: %.c
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

%.d: %.cpp
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

$(EXEC): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY: $(EXEC)
