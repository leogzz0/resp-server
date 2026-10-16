#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ht.h"

static int freed = 0; // counts how many values the table released

static void counting_free(void *p) {
    freed++;
    free(p);
}

static int *make_int(int v) {
    int *p = malloc(sizeof(*p));
    *p = v;
    return p;
}

int main(void) {
    ht_t t;
    ht_init(&t, counting_free);
    assert(t.count == 0);
    assert(ht_get(&t, "nope", 4) == NULL); // missing key

    // basic set/get
    assert(ht_set(&t, "foo", 3, make_int(1)) == 1);
    assert(*(int *)ht_get(&t, "foo", 3) == 1);
    assert(t.count == 1);

    // overwrite replaces the value and frees the old one
    assert(ht_set(&t, "foo", 3, make_int(2)) == 0);
    assert(*(int *)ht_get(&t, "foo", 3) == 2);
    assert(t.count == 1);
    assert(freed == 1);

    // keys are binary safe and compared by length too
    assert(ht_set(&t, "a\0b", 3, make_int(10)) == 1);
    assert(ht_set(&t, "a\0c", 3, make_int(11)) == 1);
    assert(*(int *)ht_get(&t, "a\0b", 3) == 10);
    assert(*(int *)ht_get(&t, "a\0c", 3) == 11);
    assert(ht_get(&t, "a", 1) == NULL); // prefix is a different key

    // delete
    assert(ht_del(&t, "foo", 3) == 1);
    assert(ht_del(&t, "foo", 3) == 0); // already gone
    assert(ht_get(&t, "foo", 3) == NULL);
    assert(t.count == 2);
    assert(freed == 2);

    // many inserts force several resizes, everything must survive
    char key[32];
    for (int i = 0; i < 10000; i++) {
        int len = snprintf(key, sizeof(key), "key:%d", i);
        assert(ht_set(&t, key, (size_t)len, make_int(i)) == 1);
    }
    assert(t.count == 10002);
    assert(t.size >= 10002); // load factor stayed at or below 1
    for (int i = 0; i < 10000; i++) {
        int len = snprintf(key, sizeof(key), "key:%d", i);
        int *v = ht_get(&t, key, (size_t)len);
        assert(v != NULL && *v == i);
    }

    // delete the even ones, odd ones must remain
    for (int i = 0; i < 10000; i += 2) {
        int len = snprintf(key, sizeof(key), "key:%d", i);
        assert(ht_del(&t, key, (size_t)len) == 1);
    }
    assert(t.count == 5002);
    for (int i = 0; i < 10000; i++) {
        int len = snprintf(key, sizeof(key), "key:%d", i);
        int *v = ht_get(&t, key, (size_t)len);
        if (i % 2 == 0) {
            assert(v == NULL);
        } else {
            assert(v != NULL && *v == i);
        }
    }

    ht_free(&t); // asan checks nothing leaks here
    assert(t.buckets == NULL);

    printf("test_ht: all assertions passed\n");
    return 0;
}
