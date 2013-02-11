SOURCES=main.cpp 
EXEC=unpdf
SUBDIRS=sel merge pdfget pdfput img

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
LDFLAGS=libnpdf/libnpdf.a -lz -lcrypto -ljpeg 

CPPFLAGS+=-I../g4coder -I../lzwcoder -I../lsyscpp

OBJECTS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).o,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).o,\
$(SOURCES)))
DEPENDS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).d,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).d,\
        $(filter-out %.o,""\
$(SOURCES))))

all: libnpdf $(EXEC)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(DEPENDS)
endif

clean:
	rm -f $(EXEC) $(OBJECTS) $(DEPENDS) 
	@$(MAKE) -C libnpdf clean

%.d: %.c
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s|\($*\)\.o[ :]*|\1.o $@ : |g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

%.d: %.cpp
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s|\($*\)\.o[ :]*|\1.o $@ : |g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

.PHONY: libnpdf
libnpdf:
	@$(MAKE) -C $@

$(EXEC): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

subdirs: $(SUBDIRS)
.PHONY: $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -C $@
