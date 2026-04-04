CC = gcc
CFLAGS = -Wall -g -Isrc/include
LDFLAGS = -lz

SRC = src/main.c src/commands.c src/utils.c src/sha256.c src/object.c src/index.c
OBJDIR = objects
OBJ = $(SRC:src/%.c=$(OBJDIR)/%.o)
TARGET = cit

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
