/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//*****************************************************************************/
// Image Atomics (Intel Vendor Extension)
//*****************************************************************************/

INLINE int OVERLOADABLE intel_atomic_and(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_and(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_or(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_or(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xor(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xor(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xchg(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xchg(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

float OVERLOADABLE intel_atomic_xchg(image1d_buffer_t image, int coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int4 c = (int4)( coord, 0, 0, 0 );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, c, ival );
    return as_float( i );
}

INLINE int OVERLOADABLE intel_atomic_inc(image1d_buffer_t image, int coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_inc_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_inc(image1d_buffer_t image, int coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_dec(image1d_buffer_t image, int coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_dec_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_dec(image1d_buffer_t image, int coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_add(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_add(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_sub(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE uint OVERLOADABLE intel_atomic_sub(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE int OVERLOADABLE intel_atomic_cmpxchg(image1d_buffer_t image, int coord, int cmp, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE uint OVERLOADABLE intel_atomic_cmpxchg(image1d_buffer_t image, int coord, uint cmp, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE int OVERLOADABLE intel_atomic_min(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_min_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_min(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_min_u32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_max(image1d_buffer_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_max_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_max(image1d_buffer_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_max_u32( image_id, c, val );
}


// image1d_t

INLINE int OVERLOADABLE intel_atomic_and(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_and(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_or(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_or(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xor(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xor(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xchg(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xchg(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

float OVERLOADABLE intel_atomic_xchg(image1d_t image, int coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int4 c = (int4)( coord, 0, 0, 0 );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, c, ival );
    return as_float( i );
}

INLINE int OVERLOADABLE intel_atomic_inc(image1d_t image, int coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_inc_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_inc(image1d_t image, int coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_dec(image1d_t image, int coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_dec_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_dec(image1d_t image, int coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_add(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_add(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_sub(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE uint OVERLOADABLE intel_atomic_sub(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE int OVERLOADABLE intel_atomic_cmpxchg(image1d_t image, int coord, int cmp, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE uint OVERLOADABLE intel_atomic_cmpxchg(image1d_t image, int coord, uint cmp, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE int OVERLOADABLE intel_atomic_min(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_min_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_min(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_min_u32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_max(image1d_t image, int coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_max_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_max(image1d_t image, int coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0, 0 );
    return __builtin_IB_image_atomic_max_u32( image_id, c, val );
}


// image1d_array_t

INLINE int OVERLOADABLE intel_atomic_and(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_and(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_or(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_or(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xor(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xor(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xchg(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xchg(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

float OVERLOADABLE intel_atomic_xchg(image1d_array_t image, int2 coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int4 c = (int4)( coord, 0, 0 );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, c, ival );
    return as_float( i );
}

INLINE int OVERLOADABLE intel_atomic_inc(image1d_array_t image, int2 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_inc_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_inc(image1d_array_t image, int2 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_dec(image1d_array_t image, int2 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_dec_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_dec(image1d_array_t image, int2 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_add(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_add(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_sub(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE uint OVERLOADABLE intel_atomic_sub(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE int OVERLOADABLE intel_atomic_cmpxchg(image1d_array_t image, int2 coord, int cmp, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE uint OVERLOADABLE intel_atomic_cmpxchg(image1d_array_t image, int2 coord, uint cmp, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE int OVERLOADABLE intel_atomic_min(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_min_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_min(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_min_u32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_max(image1d_array_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_max_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_max(image1d_array_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_max_u32( image_id, c, val );
}


// image2d_t

INLINE int OVERLOADABLE intel_atomic_and(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_and(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_and_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_or(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_or(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_or_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xor(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xor(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xor_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_xchg(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_xchg(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_xchg_i32( image_id, c, val );
}

float OVERLOADABLE intel_atomic_xchg(image2d_t image, int2 coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int4 c = (int4)( coord, 0, 0 );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, c, ival );
    return as_float( i );
}

INLINE int OVERLOADABLE intel_atomic_inc(image2d_t image, int2 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_inc_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_inc(image2d_t image, int2 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_dec(image2d_t image, int2 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_dec_i32( image_id, c );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_dec(image2d_t image, int2 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_add(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_add(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_sub(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE uint OVERLOADABLE intel_atomic_sub(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_add_i32( image_id, c, -val );
}

INLINE int OVERLOADABLE intel_atomic_cmpxchg(image2d_t image, int2 coord, int cmp, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE uint OVERLOADABLE intel_atomic_cmpxchg(image2d_t image, int2 coord, uint cmp, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, c, cmp, val );
}

INLINE int OVERLOADABLE intel_atomic_min(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_min_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_min(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_min_u32( image_id, c, val );
}

INLINE int OVERLOADABLE intel_atomic_max(image2d_t image, int2 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_max_i32( image_id, c, val );
}

INLINE uint OVERLOADABLE intel_atomic_max(image2d_t image, int2 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int4 c = (int4)( coord, 0, 0 );
    return __builtin_IB_image_atomic_max_u32( image_id, c, val );
}


// image2d_array_t

INLINE int OVERLOADABLE intel_atomic_and(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_and_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_and(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_and_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_or(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_or_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_or(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_or_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_xor(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xor_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_xor(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xor_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_xchg(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xchg_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_xchg(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xchg_i32( image_id, coord, val );
}

float OVERLOADABLE intel_atomic_xchg(image2d_array_t image, int4 coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, coord, ival );
    return as_float( i );
}

INLINE int OVERLOADABLE intel_atomic_inc(image2d_array_t image, int4 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_inc_i32( image_id, coord );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_inc(image2d_array_t image, int4 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_dec(image2d_array_t image, int4 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_dec_i32( image_id, coord );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_dec(image2d_array_t image, int4 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_add(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_add(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_sub(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, -val );
}

INLINE uint OVERLOADABLE intel_atomic_sub(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, -val );
}

INLINE int OVERLOADABLE intel_atomic_cmpxchg(image2d_array_t image, int4 coord, int cmp, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, coord, cmp, val );
}

INLINE uint OVERLOADABLE intel_atomic_cmpxchg(image2d_array_t image, int4 coord, uint cmp, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, coord, cmp, val );
}

INLINE int OVERLOADABLE intel_atomic_min(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_min_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_min(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_min_u32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_max(image2d_array_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_max_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_max(image2d_array_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_max_u32( image_id, coord, val );
}


// image3d_t

INLINE int OVERLOADABLE intel_atomic_and(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_and_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_and(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_and_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_or(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_or_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_or(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_or_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_xor(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xor_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_xor(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xor_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_xchg(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xchg_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_xchg(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_xchg_i32( image_id, coord, val );
}

float OVERLOADABLE intel_atomic_xchg(image3d_t image, int4 coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, coord, ival );
    return as_float( i );
}

INLINE int OVERLOADABLE intel_atomic_inc(image3d_t image, int4 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_inc_i32( image_id, coord );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_inc(image3d_t image, int4 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_dec(image3d_t image, int4 coord)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_dec_i32( image_id, coord );
}

// Can't have both the int and uint versions as they differ only on return value
// TODO: Which one do we want?
//INLINE uint OVERLOADABLE intel_atomic_dec(image3d_t image, int4 coord)
//{
//    return 0; // TODO
//}

INLINE int OVERLOADABLE intel_atomic_add(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_add(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_sub(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, -val );
}

INLINE uint OVERLOADABLE intel_atomic_sub(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_add_i32( image_id, coord, -val );
}

INLINE int OVERLOADABLE intel_atomic_cmpxchg(image3d_t image, int4 coord, int cmp, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, coord, cmp, val );
}

INLINE uint OVERLOADABLE intel_atomic_cmpxchg(image3d_t image, int4 coord, uint cmp, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_cmpxchg_i32( image_id, coord, cmp, val );
}

INLINE int OVERLOADABLE intel_atomic_min(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_min_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_min(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_min_u32( image_id, coord, val );
}

INLINE int OVERLOADABLE intel_atomic_max(image3d_t image, int4 coord, int val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_max_i32( image_id, coord, val );
}

INLINE uint OVERLOADABLE intel_atomic_max(image3d_t image, int4 coord, uint val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    return __builtin_IB_image_atomic_max_u32( image_id, coord, val );
}


// image2d_depth_t

float OVERLOADABLE intel_atomic_xchg(image2d_depth_t image, int2 coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int4 c = (int4)( coord, 0, 0 );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, c, ival );
    return as_float( i );
}


// image2d_array_depth_t

float OVERLOADABLE intel_atomic_xchg(image2d_array_depth_t image, int4 coord, float val)
{
    long image_id = (long)__builtin_astype(image, __global void*);
    int ival = as_int( val );
    int i = __builtin_IB_image_atomic_xchg_i32( image_id, coord, ival );
    return as_float( i );
}

