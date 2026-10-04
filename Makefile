CC = cc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined

SRC = src/main.c
BIN = resp-server

TEST_SDS_BIN = test_sds
TEST_RESP_BIN = test_resp

.PHONY: all run clean test

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: all
	./$(BIN)

test: $(TEST_SDS_BIN) $(TEST_RESP_BIN)
	./$(TEST_SDS_BIN)
	./$(TEST_RESP_BIN)

$(TEST_SDS_BIN): tests/test_sds.c src/sds.c include/sds.h
	$(CC) $(CFLAGS) tests/test_sds.c src/sds.c -o $(TEST_SDS_BIN) $(LDFLAGS)

$(TEST_RESP_BIN): tests/test_resp.c src/resp.c src/sds.c include/resp.h include/sds.h
	$(CC) $(CFLAGS) tests/test_resp.c src/resp.c src/sds.c -o $(TEST_RESP_BIN) $(LDFLAGS)

clean:
	rm -f $(BIN) $(TEST_SDS_BIN) $(TEST_RESP_BIN)
