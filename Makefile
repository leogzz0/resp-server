CC = cc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined

SRC = src/main.c
BIN = resp-server

.PHONY: all run clean

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: all
	./$(BIN)

clean:
	rm -f $(BIN)
