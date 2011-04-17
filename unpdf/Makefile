SOURCES=main.cpp pdfbase.cpp pdfio.cpp pdfparse.cpp pdffilter.cpp pdfcrypt.cpp pdfcomp.cpp pdfsec.cpp pdfcols.cpp
EXEC=unpdf
LIB=libnpdf.a

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall -g
LDFLAGS=-lz -lssl -ljpeg

CPPFLAGS+=-I../g4coder -I../lzwcoder -I../lsyscpp
ADDOBJS+=../g4coder/g4code.o ../lzwcoder/lzwcode.o ../lsyscpp/exception.o

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
	rm -f $(EXEC) $(LIB) $(OBJECTS) $(DEPENDS) 

%.d: %.c
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

%.d: %.cpp
	@$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

$(EXEC): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(ADDOBJS)

$(LIB): $(filter-out main.o,$(OBJECTS))
	$(AR) rcu $@ $^ $(ADDOBJS)
