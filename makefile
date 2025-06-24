include config.mk

BUILDDIR   = build
SRCDIR     = src
INCLUDEDIR = include

VERSION = $(shell git describe --tags --always)

SRC = $(shell find $(SRCDIR) -name "*.c")
OBJ = $(SRC:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

CFLAGS += -I$(INCLUDEDIR)

all : $(BUILDDIR)/libttf.a

test : $(BUILDDIR)/test.o $(BUILDDIR)/libttf.a
	@$(CC) -o $@ $^

$(BUILDDIR)/libttf.a : $(OBJ)
	@echo '[archiving $@]'
	@$(AR) rcs $@ $^

$(BUILDDIR)/test.o : test.c
	@echo '[compiling $^]'
	@mkdir -p $(shell dirname $@)
	@$(CC) -o $@ -c $^ $(CFLAGS)

$(BUILDDIR)/%.o : $(SRCDIR)/%.c 
	@echo '[compiling $^]'
	@mkdir -p $(shell dirname $@)
	@$(CC) -o $@ -c $^ $(CFLAGS)

install : all
	@echo '[installing libttf.a]'
	@mkdir -p $(PREFIX)/lib
	@cp $(BUILDDIR)/libttf.a $(PREFIX)/lib
	@echo '[installing libttf.h]'
	@mkdir -p $(PREFIX)/include
	@cp include/ttf.h $(PREFIX)/include
	@echo '[installing libttf.pc]'
	@mkdir -p $(PREFIX)/lib/pkgconfig
	@echo 'prefix=$(PREFIX)' > $(PREFIX)/lib/pkgconfig/libttf.pc
	@echo 'version=$(VERSION)' >> $(PREFIX)/lib/pkgconfig/libttf.pc
	@cat libttf.pc >> $(PREFIX)/lib/pkgconfig/libttf.pc

uninstall :
	rm -f $(PREFIX)/lib/libttf.* $(PREFIX)/include/libttf.h $(PREFIX)/lib/pkgconfig/libttf.pc

clean :
	rm -fr build
