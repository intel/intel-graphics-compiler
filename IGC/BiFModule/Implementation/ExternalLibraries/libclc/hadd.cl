#include "../../../Headers/spirv.h"

INLINE OVERLOADABLE long libclc_hadd(long x, long y) {
    return (x >> (long)1) + (y >> (long)1) + (x & y & (long)1);
}