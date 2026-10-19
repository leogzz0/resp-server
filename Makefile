CC = cc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined

SRC = src/main.c src/cmd.c src/ht.c src/resp.c src/sds.c src/server.c
BIN = resp-server

TEST_SDS_BIN = test_sds
TEST_RESP_BIN = test_resp
TEST_HT_BIN = test_ht
TEST_CMD_BIN = test_cmd

.PHONY: all run clean test

all: $(BIN)

$(BIN): $(SRC) $(wildcard include/*.h)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

run: all
	./$(BIN)

test: $(TEST_SDS_BIN) $(TEST_RESP_BIN) $(TEST_HT_BIN) $(TEST_CMD_BIN)
	./$(TEST_SDS_BIN)
	./$(TEST_RESP_BIN)
	./$(TEST_HT_BIN)
	./$(TEST_CMD_BIN)

$(TEST_SDS_BIN): tests/test_sds.c src/sds.c include/sds.h
	$(CC) $(CFLAGS) tests/test_sds.c src/sds.c -o $(TEST_SDS_BIN) $(LDFLAGS)

$(TEST_RESP_BIN): tests/test_resp.c src/resp.c src/sds.c include/resp.h include/sds.h
	$(CC) $(CFLAGS) tests/test_resp.c src/resp.c src/sds.c -o $(TEST_RESP_BIN) $(LDFLAGS)

$(TEST_HT_BIN): tests/test_ht.c src/ht.c include/ht.h
	$(CC) $(CFLAGS) tests/test_ht.c src/ht.c -o $(TEST_HT_BIN) $(LDFLAGS)

CMD_DEPS = src/cmd.c src/ht.c src/resp.c src/sds.c src/server.c

$(TEST_CMD_BIN): tests/test_cmd.c $(CMD_DEPS) $(wildcard include/*.h)
	$(CC) $(CFLAGS) tests/test_cmd.c $(CMD_DEPS) -o $(TEST_CMD_BIN) $(LDFLAGS)

clean:
	rm -f $(BIN) $(TEST_SDS_BIN) $(TEST_RESP_BIN) $(TEST_HT_BIN) $(TEST_CMD_BIN)
