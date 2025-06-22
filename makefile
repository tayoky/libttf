include config.mk

BUILDDIR   = build
SRCDIR     = src
INCLUDEDIR = include

SRC = $(shell find $(SRCDIR) -name "*.c")
OBJ = $(SRC:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

CFLAGS += -I$(INCLUDEDIR)

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

clean :
	rm -fr build
