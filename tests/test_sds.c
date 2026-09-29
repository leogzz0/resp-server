#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sds.h"

int main(void) {
    sds_t s;
    sds_init(&s);
    assert(s.len == 0);

    sds_append(&s, "hello", 5);
    assert(s.len == 5);
    assert(memcmp(s.data, "hello", 5) == 0);

    sds_append(&s, " world", 6);
    assert(s.len == 11);
    assert(memcmp(s.data, "hello world", 11) == 0);

    // force several reallocations and confirm nothing gets corrupted
    for (int i = 0; i < 1000; i++) {
        sds_append(&s, "x", 1);
    }
    assert(s.len == 11 + 1000);
    assert(memcmp(s.data, "hello world", 11) == 0); // original content survived the growth
    for (size_t i = 11; i < s.len; i++) {
        assert(s.data[i] == 'x'); // every appended byte landed correctly
    }

    sds_free(&s);
    assert(s.data == NULL);
    assert(s.len == 0);

    printf("test_sds: all assertions passed\n");
    return 0;
}
