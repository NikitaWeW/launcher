CC = gcc

OUT_DIR = out
OUT_EXEC = launcher

SRC = $(wildcard *.c)

OUT = $(OUT_DIR)/$(OUT_EXEC)

BUILD_TYPE ?= release

CFLAGS_DEBUG = -g -O0
CFLAGS_RELEASE = -O3 --static

CFLAGS = $(CFLAGS_$(BUILD_TYPE))

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p $(OUT_DIR)
	$(CC) $(SRC) -o $(OUT) $(CFLAGS)

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean
