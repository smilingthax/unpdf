SOURCES=main.cpp ../../lsyscpp/cmdline.o
EXEC=pdfmerge

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g -I.. -I../../lsyscpp
LDFLAGS=-L../ -lnpdf -lssl -ljpeg

OBJECTS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).o,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).o,\
$(SOURCES)))
DEPENDS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).d,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).d,\
        $(filter-out %.o,""\
$(SOURCES))))

all: $(EXEC)
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

.PHONY: $(EXEC)
$(EXEC): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

