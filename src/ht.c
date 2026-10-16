#include "ht.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HT_INITIAL_SIZE 16

// fnv-1a: simple and spreads keys well enough
static uint64_t hash_bytes(const char *key, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)key[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static ht_entry_t *find_entry(const ht_t *t, const char *key, size_t klen) {
    size_t idx = hash_bytes(key, klen) & (t->size - 1); // size is a power of two
    for (ht_entry_t *e = t->buckets[idx]; e != NULL; e = e->next) {
        if (e->klen == klen && memcmp(e->key, key, klen) == 0) {
            return e;
        }
    }
    return NULL;
}

// moves every entry into a bigger bucket array
static void resize(ht_t *t, size_t new_size) {
    ht_entry_t **nb = calloc(new_size, sizeof(*nb));
    if (nb == NULL) {
        abort();
    }
    for (size_t i = 0; i < t->size; i++) {
        ht_entry_t *e = t->buckets[i];
        while (e != NULL) {
            ht_entry_t *next = e->next; // save it, we're about to relink e
            size_t idx = hash_bytes(e->key, e->klen) & (new_size - 1);
            e->next = nb[idx];
            nb[idx] = e;
            e = next;
        }
    }
    free(t->buckets);
    t->buckets = nb;
    t->size = new_size;
}

void ht_init(ht_t *t, void (*val_free)(void *)) {
    t->buckets = calloc(HT_INITIAL_SIZE, sizeof(*t->buckets));
    if (t->buckets == NULL) {
        abort();
    }
    t->size = HT_INITIAL_SIZE;
    t->count = 0;
    t->val_free = val_free;
}

void ht_free(ht_t *t) {
    for (size_t i = 0; i < t->size; i++) {
        ht_entry_t *e = t->buckets[i];
        while (e != NULL) {
            ht_entry_t *next = e->next;
            if (t->val_free != NULL) {
                t->val_free(e->val);
            }
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(t->buckets);
    t->buckets = NULL;
    t->size = 0;
    t->count = 0;
}

void *ht_get(const ht_t *t, const char *key, size_t klen) {
    ht_entry_t *e = find_entry(t, key, klen);
    return e != NULL ? e->val : NULL;
}

int ht_set(ht_t *t, const char *key, size_t klen, void *val) {
    ht_entry_t *e = find_entry(t, key, klen);
    if (e != NULL) {
        if (t->val_free != NULL && e->val != val) {
            t->val_free(e->val); // replacing, release the old value
        }
        e->val = val;
        return 0;
    }

    if (t->count >= t->size) {
        resize(t, t->size * 2); // load factor hit 1, double the buckets
    }

    e = malloc(sizeof(*e));
    char *kcopy = malloc(klen + 1);
    if (e == NULL || kcopy == NULL) {
        abort();
    }
    memcpy(kcopy, key, klen);
    kcopy[klen] = '\0';
    e->key = kcopy;
    e->klen = klen;
    e->val = val;

    size_t idx = hash_bytes(key, klen) & (t->size - 1);
    e->next = t->buckets[idx]; // push at the head of the chain
    t->buckets[idx] = e;
    t->count++;
    return 1;
}

int ht_del(ht_t *t, const char *key, size_t klen) {
    size_t idx = hash_bytes(key, klen) & (t->size - 1);
    ht_entry_t **link = &t->buckets[idx]; // points at whatever points to the current entry
    while (*link != NULL) {
        ht_entry_t *e = *link;
        if (e->klen == klen && memcmp(e->key, key, klen) == 0) {
            *link = e->next; // unlink it from the chain
            if (t->val_free != NULL) {
                t->val_free(e->val);
            }
            free(e->key);
            free(e);
            t->count--;
            return 1;
        }
        link = &e->next;
    }
    return 0;
}
