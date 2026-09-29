#include "sds.h"

#include <stdlib.h>
#include <string.h>

void sds_init(sds_t *s) {
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

void sds_free(sds_t *s) {
    free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

void sds_append(sds_t *s, const char *data, size_t len) {
    // grow the buffer if there isn't enough room left
    if (s->len + len > s->cap) {
        size_t new_cap = s->cap == 0 ? 16 : s->cap * 2; // start small, then double
        while (new_cap < s->len + len) {
            new_cap *= 2; // keep doubling until it actually fits
        }
        char *new_data = realloc(s->data, new_cap);
        if (new_data == NULL) {
            abort(); // out of memory, nothing sane to do here
        }
        s->data = new_data;
        s->cap = new_cap;
    }

    memcpy(s->data + s->len, data, len); // copy the new bytes in
    s->len += len;
}
