#include "server.h"

#include <stdlib.h>

// releases a string value stored in the db
static void free_string(void *p) {
    sds_free(p);
    free(p);
}

void server_init(server_t *srv) {
    ht_init(&srv->db, free_string);
}

void server_free(server_t *srv) {
    ht_free(&srv->db);
}
