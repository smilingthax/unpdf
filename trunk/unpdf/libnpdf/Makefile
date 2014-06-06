SUBDIRS=io/ objs/ util/ parse/ crypt/ security/ stream/ filter/ pages/ xref/ pdf/ color/
LIB=libnpdf.a

OBJECTS+=io/base.o io/crypt.o io/file.o io/mem.o io/pdfio.o io/ptr.o io/sub.o
OBJECTS+=objs/array.o objs/base.o objs/dict.o objs/name.o objs/num.o objs/ptr.o objs/ref.o objs/string.o
OBJECTS+=util/util.o
# parse
OBJECTS+=crypt/aescbc.o crypt/md5.o crypt/rand.o crypt/rc4.o crypt/sha256.o
OBJECTS+=security/aescrypt.o security/pdfsec.o security/rc4crypt.o
OBJECTS+=stream/pdfcomp.o
OBJECTS+=filter/pdffilter.o
OBJECTS+=pages/page.o pages/pagestree.o pages/rect.o parse/ctype.o parse/pdfparse.o
OBJECTS+=xref/xref.o
OBJECTS+=pdf/pdf.o pdf/outpdf.o
OBJECTS+=color/pdfcols.o

ADDOBJS+=../../g4coder/g4code.o ../../lzwcoder/lzwcode.o ../../lsyscpp/exception.o ../../lsyscpp/filesystem.o

all: $(SUBDIRS) $(LIB)

clean:
	@for i in $(SUBDIRS); do \
	  $(MAKE) -C $$i clean; \
	done
	rm -f $(LIB)

#$(EXEC): $(OBJECTS)
#	$(CXX) -o $@ $^ $(LDFLAGS) $(ADDOBJS)

$(LIB): $(filter-out main.o,$(OBJECTS))
	@rm -f $@    # linker
	$(AR) rcu $@ $^ $(ADDOBJS)

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	@$(MAKE) -C $@
