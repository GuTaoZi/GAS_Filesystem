#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdint.h>

#define BITS_PER_LONG       64
#define BIT_WORD(nr)        ((nr) / BITS_PER_LONG)
#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) % BITS_PER_LONG))
#define BITMAP_LAST_WORD_MASK(nbits) \
(	                                 \
    ((nbits) % BITS_PER_LONG) ?      \
        (1UL<<((nbits) % BITS_PER_LONG))-1 : ~0UL \
)

#define INVALID_NO      ((uint64_t) 0)

#define ffz(x)  __ffs(~(x))
#define __ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))

void bitmap_set(uint64_t *map, int start, int nr);
void bitmap_clear(uint64_t *map, int start, int nr);
uint64_t bitmap_alloc_region(uint64_t *bitmap, int size, int start, int nr);
void bitmap_free_region(uint64_t *bitmap, int pos, int nr);
#endif
