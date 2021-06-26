CC=gcc

CFLAGS=-Wall

OUTDIR=target
SRCDIR=src

HEADER_FILES = $(shell find $(SRCDIR) -type f -name '*.h')
SRC_FILES := $(shell find $(SRCDIR) -type f -name '*.c')
OBJ_FILES := $(patsubst $(SRCDIR)/%.c,$(OUTDIR)/%.o,$(SRC_FILES))

all: $(OUTDIR)/main
.PHONY: all

run: all
	./$(OUTDIR)/main
.PHONY: run

$(OUTDIR)/%.o: $(SRCDIR)/%.c $(HEADER_FILES)
	@mkdir -p $(OUTDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(SRCDIR)

$(OUTDIR)/main: $(OBJ_FILES)
	$(CC) -o $@ $^

clean:
	rm -rf $(OUTDIR)
.PHONY: clean
