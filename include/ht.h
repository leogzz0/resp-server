#ifndef HT_H
#define HT_H

#include <stddef.h>

// one key/value pair, chained with others that land in the same bucket
typedef struct ht_entry {
    char *key;  // owned copy, null-terminated for convenience
    size_t klen;
    void *val;  // owned by the table, released with val_free
    struct ht_entry *next;
} ht_entry_t;

typedef struct {
    ht_entry_t **buckets;
    size_t size;  // number of buckets, always a power of two
    size_t count; // number of entries stored
    void (*val_free)(void *); // how to release a value, may be NULL
} ht_t;

void ht_init(ht_t *t, void (*val_free)(void *));
void ht_free(ht_t *t); // frees every entry and value
void *ht_get(const ht_t *t, const char *key, size_t klen); // NULL if missing
int ht_set(ht_t *t, const char *key, size_t klen, void *val); // 1 if new, 0 if replaced
int ht_del(ht_t *t, const char *key, size_t klen); // 1 if deleted, 0 if missing

#endif
