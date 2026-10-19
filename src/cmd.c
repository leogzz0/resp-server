#include "cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void cmd_ping(server_t *srv, const resp_command_t *cmd, sds_t *out) {
    (void)srv;
    if (cmd->argc == 1) {
        resp_add_simple(out, "PONG");
    } else {
        resp_add_bulk(out, cmd->argv[1], cmd->argvlen[1]); // PING msg echoes msg back
    }
}

static void cmd_echo(server_t *srv, const resp_command_t *cmd, sds_t *out) {
    (void)srv;
    resp_add_bulk(out, cmd->argv[1], cmd->argvlen[1]);
}

// allocates a string value for the db, the table takes ownership of it
static sds_t *new_string(const char *data, size_t len) {
    sds_t *s = malloc(sizeof(*s));
    if (s == NULL) {
        abort();
    }
    sds_init(s);
    sds_append(s, data, len);
    return s;
}

static void cmd_set(server_t *srv, const resp_command_t *cmd, sds_t *out) {
    // ht_set copies the key and frees the old value if the key already existed
    ht_set(&srv->db, cmd->argv[1], cmd->argvlen[1], new_string(cmd->argv[2], cmd->argvlen[2]));
    resp_add_simple(out, "OK");
}

static void cmd_get(server_t *srv, const resp_command_t *cmd, sds_t *out) {
    sds_t *v = ht_get(&srv->db, cmd->argv[1], cmd->argvlen[1]);
    if (v == NULL) {
        resp_add_nil(out); // missing key
    } else {
        resp_add_bulk(out, v->data, v->len);
    }
}

static void cmd_del(server_t *srv, const resp_command_t *cmd, sds_t *out) {
    long long deleted = 0;
    for (size_t i = 1; i < cmd->argc; i++) {
        deleted += ht_del(&srv->db, cmd->argv[i], cmd->argvlen[i]);
    }
    resp_add_integer(out, deleted);
}

static void cmd_exists(server_t *srv, const resp_command_t *cmd, sds_t *out) {
    long long found = 0;
    for (size_t i = 1; i < cmd->argc; i++) {
        if (ht_get(&srv->db, cmd->argv[i], cmd->argvlen[i]) != NULL) {
            found++; // repeated keys count every time
        }
    }
    resp_add_integer(out, found);
}

// one row per command, add new commands here
static const cmd_def_t table[] = {
    {"PING", cmd_ping, 1, 2},
    {"ECHO", cmd_echo, 2, 2},
    {"SET", cmd_set, 3, 3},
    {"GET", cmd_get, 2, 2},
    {"DEL", cmd_del, 2, -1},
    {"EXISTS", cmd_exists, 2, -1},
};

static char to_upper(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

// case-insensitive match of the typed name against a table name
static int name_matches(const char *typed, size_t typed_len, const char *name) {
    if (typed_len != strlen(name)) {
        return 0;
    }
    for (size_t i = 0; i < typed_len; i++) {
        if (to_upper(typed[i]) != name[i]) {
            return 0;
        }
    }
    return 1;
}

static const cmd_def_t *lookup(const char *name, size_t len) {
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (name_matches(name, len, table[i].name)) {
            return &table[i];
        }
    }
    return NULL;
}

void cmd_execute(server_t *srv, const resp_command_t *cmd, sds_t *out) {
    const cmd_def_t *def = lookup(cmd->argv[0], cmd->argvlen[0]);
    char msg[128];

    if (def == NULL) {
        snprintf(msg, sizeof(msg), "ERR unknown command '%.60s'", cmd->argv[0]);
        resp_add_error(out, msg);
        return;
    }

    int argc = (int)cmd->argc;
    if (argc < def->min_argc || (def->max_argc >= 0 && argc > def->max_argc)) {
        // redis prints the name in lower case here
        char lower[16];
        size_t i = 0;
        for (; def->name[i] != '\0' && i < sizeof(lower) - 1; i++) {
            lower[i] = (char)(def->name[i] - 'A' + 'a');
        }
        lower[i] = '\0';
        snprintf(msg, sizeof(msg), "ERR wrong number of arguments for '%s' command", lower);
        resp_add_error(out, msg);
        return;
    }

    def->handler(srv, cmd, out);
}
