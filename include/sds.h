#ifndef SDS_H
#define SDS_H

#include <stddef.h>

// dynamic buffer: pointer + length + capacity
typedef struct {
    char *data;
    size_t len; // bytes actually used
    size_t cap; // bytes allocated
} sds_t;

void sds_init(sds_t *s); // set up an empty buffer
void sds_free(sds_t *s); // release the buffer
void sds_append(sds_t *s, const char *data, size_t len); // grow and append bytes

#endif
