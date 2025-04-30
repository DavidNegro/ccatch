#ifndef UNIT_NO_STDLIB_H
#include <stdlib.h>
#endif

#define __TEST_MAX__(x, y) ((x) > (y) ? (x) : (y))

#ifndef UNIT_ALLOC
#define UNIT_ALLOC(x) malloc(x)
#endif

#ifndef UNIT_FREE
#define UNIT_FREE(x) free(x)
#endif

#ifndef UNIT_MEMCPY
#define UNIT_MEMCPY(des, src, size) memcpy(des, src, size)
#endif

#ifndef UNIT_MEMSET
#define UNIT_MEMSET(des, val, size) memset(des, val, size)
#endif

#ifndef UNIT_ABORT
#define UNIT_ABORT() abort()
#endif

#ifndef UNIT_IGNORE_UNUSED
#define UNIT_IGNORE_UNUSED(x) (void*)&x;
#endif
