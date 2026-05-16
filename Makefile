CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2 -DNDEBUG -D_DEFAULT_SOURCE
LDFLAGS := -lcurl
SRC_DIR := src
BUILD_DIR := build
TARGET  := yaarp

# Find all .c files in src/ and subdirs
SRCS    := $(shell find $(SRC_DIR) -name '*.c')
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean install

all: $(TARGET)

 $(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $@"
	@ls -lh $@

 $(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)