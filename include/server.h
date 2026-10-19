#ifndef SERVER_H
#define SERVER_H

#include "ht.h"
#include "sds.h"

// everything the command handlers share
typedef struct {
    ht_t db; // key -> sds_t * (heap allocated string value)
} server_t;

void server_init(server_t *srv);
void server_free(server_t *srv);

#endif
