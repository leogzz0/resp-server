CC = cc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined

SRC = src/main.c
BIN = resp-server

TEST_SDS_BIN = test_sds

.PHONY: all run clean test

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: all
	./$(BIN)

test: $(TEST_SDS_BIN)
	./$(TEST_SDS_BIN)

$(TEST_SDS_BIN): tests/test_sds.c src/sds.c include/sds.h
	$(CC) $(CFLAGS) tests/test_sds.c src/sds.c -o $(TEST_SDS_BIN) $(LDFLAGS)

clean:
	rm -f $(BIN) $(TEST_SDS_BIN)
