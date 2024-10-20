CC = gcc

OUT_DIR = out
OUT_EXEC = launcher

SRC = $(wildcard *.c)

OUT = $(OUT_DIR)/$(OUT_EXEC)

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p $(OUT_DIR)
	$(CC) $(SRC) -o $(OUT) --static

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean
