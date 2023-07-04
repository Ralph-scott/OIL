CC      := clang
FILES   := $(wildcard src/**.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(FILES))
TARGET  := build/$(shell basename $(shell pwd))

WARN   := -Wall -Wextra
OPT    ?= 0
CFLAGS := -O$(OPT) $(WARN)

ifeq ($(DEBUG), true)
CFLAGS += -g
endif

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

build/%.o: src/%.c
	mkdir -p build
	$(CC) -c $(CFLAGS) -o $@ $^

clean: $(OBJECTS) $(TARGET)
	rm -r $(OBJECTS) $(TARGET)
