CC = gcc
CFLAGS = -Wall -g \
         -Isrc \
         -Isrc/core/object \
         -Isrc/core/index \
         -Isrc/core/hash \
         -Isrc/core/config \
         -Isrc/core/refs \
         -Isrc/commands \
         -Isrc/utils \
         -Isrc/ui
LDFLAGS = -lz -lcurl

SRC = $(shell find src -name "*.c")
OBJDIR = objects
OBJ = $(SRC:src/%.c=$(OBJDIR)/%.o)
TARGET = cit

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/core/object
	mkdir -p $(OBJDIR)/core/index
	mkdir -p $(OBJDIR)/core/hash
	mkdir -p $(OBJDIR)/core/config
	mkdir -p $(OBJDIR)/core/refs
	mkdir -p $(OBJDIR)/commands
	mkdir -p $(OBJDIR)/utils
	mkdir -p $(OBJDIR)/ui

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
