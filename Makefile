CC = gcc

OUT_DIR = out
OUT_EXEC = launcher

SRC = $(wildcard *.c)

OUT = $(OUT_DIR)/$(OUT_EXEC)

all: $(OUT)

$(OUT): $(SRC)
	if not exist $(OUT_DIR) mkdir $(OUT_DIR)
	$(CC) $(SRC) -o $(OUT)

clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean
