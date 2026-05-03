#ifndef TYPES_H_
#define TYPES_H_
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#define DEF_ARR_TYPE(val_type, name) struct name { val_type* items; size_t count; }

#define DEF_DA_TYPE(val_type, name) struct name { val_type* items; size_t count; size_t capacity; }

DEF_DA_TYPE(char*, strings);

#define da_append(da, item)                                                          \
    do {                                                                             \
        if ((da)->count >= (da)->capacity) {                                         \
            (da)->capacity = (da)->capacity == 0 ? 8 : (da)->capacity*2;             \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                            \
        (da)->items[(da)->count++] = (item);                                         \
    } while (0)

#endif // TYPES_H_
