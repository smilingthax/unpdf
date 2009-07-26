SOURCES=main.c gfxfiles.c lzwcode.c
EXEC=lzwcoder

CFLAGS=-O3 -funroll-all-loops -finline-functions -Wall

OBJECTS=$(SOURCES:.c=.o)
all: $(EXEC)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(SOURCES:.c=.d)
endif 

clean:
	rm -f $(EXEC) $(OBJECTS) $(SOURCES:.c=.d) 

%.d: %.c
	@$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< \
                      | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@;\
                      [ -s $@ ] || rm -f $@'

$(EXEC): $(OBJECTS)
	$(CC) -o $@ $^
