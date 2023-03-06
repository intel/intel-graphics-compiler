/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define VEC_TO_VEC8(type, vec) \
    (type##8)(vec.s0, vec.s1, vec.s2, vec.s3, \
                  vec.s4, vec.s5, vec.s6, vec.s7)

#define ARR_TO_VEC8(type, arr) \
    (type##8)(arr[0], arr[1], arr[2], arr[3], \
              arr[4], arr[5], arr[6], arr[7])

#define ARR_TO_VEC4(type, arr) \
    (type##4)(arr[0], arr[1], arr[2], arr[3])

#define ARR_TO_VEC2(type, arr) \
    (type##2)(arr[0], arr[1])

#define ARR_TO_VEC1(type, arr) \
    arr[0]

#define LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, element_type, contrib_type, M) \
    contrib_type *ptr = (contrib_type *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
    stride = stride / pack_factor; \
    contrib_type wi_contrib[M]; \
    for (int i = 0; i < M; i++) \
        wi_contrib[i] = *(ptr + slid + (i * stride)); \


#define SUB_GROUP_LOADS_8(readop, ptr, stride, result) \
    result.s0 = readop((ptr) + 0 * (stride)); \
    result.s1 = readop((ptr) + 1 * (stride)); \
    result.s2 = readop((ptr) + 2 * (stride)); \
    result.s3 = readop((ptr) + 3 * (stride)); \
    result.s4 = readop((ptr) + 4 * (stride)); \
    result.s5 = readop((ptr) + 5 * (stride)); \
    result.s6 = readop((ptr) + 6 * (stride)); \
    result.s7 = readop((ptr) + 7 * (stride)); \

/* PackedA load i16 */
INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_8x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int4 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_4x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 4)
    return ARR_TO_VEC4(int, wi_contrib);
}

INLINE int2 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_2x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 2)
    return ARR_TO_VEC2(int, wi_contrib);
}

INLINE int __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_1x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 1)
    return ARR_TO_VEC1(int, wi_contrib);
}

/* PackedA load i8 */
INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_8x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int4 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_4x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 4)
    return ARR_TO_VEC4(int, wi_contrib);
}

INLINE int2 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_2x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 2)
    return ARR_TO_VEC2(int, wi_contrib);
}

INLINE int __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_1x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 1)
    return ARR_TO_VEC1(int, wi_contrib);
}

/* PackedA load i16 SG16 */
INLINE short8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    short8 result;
    stride = stride / 2; // short to int
    SUB_GROUP_LOADS_8(intel_sub_group_block_read_us, (__global uint *)mem, stride, result)
    return result;
}

INLINE short4 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_4x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, short, 4)
    return ARR_TO_VEC4(short, wi_contrib);
}

INLINE short2 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_2x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, short, 2)
    return ARR_TO_VEC2(short, wi_contrib);
}

INLINE short __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_1x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, short, 1)
    return ARR_TO_VEC1(short, wi_contrib);
}

/* PackedA load i8 SG16 */
INLINE short8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    short8 result;
    stride = stride / 2; // char to short
    SUB_GROUP_LOADS_8(intel_sub_group_block_read_us, (__global ushort *)mem, stride, result)
    return result;
}

INLINE short4 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_4x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, short, 4)
    return ARR_TO_VEC4(short, wi_contrib);
}

INLINE short2 __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_2x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, short, 2)
    return ARR_TO_VEC2(short, wi_contrib);
}

INLINE short __builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_1x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, short, 1)
    return ARR_TO_VEC1(short, wi_contrib);
}

#define LOAD_PACKED_B_FROM_COL_MAJOR(mem, stride, element_type, N) \
    int *ptr = (int *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (int) / sizeof (element_type); \
    int wi_contrib[8]; \
    for (int i = 0; i < 8; i++) \
        wi_contrib[i] = *(ptr + i + (slid * stride)); \

INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_16x8_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_B_FROM_COL_MAJOR(mem, stride, short, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_32x8_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_B_FROM_COL_MAJOR(mem, stride, char, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_16x8_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_32x8_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_16x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    if (stride == 32)
      return VEC_TO_VEC8(int, intel_sub_group_block_read8((__global uint *)mem));

    int8 result;
    stride = stride / 2; // short to int
    SUB_GROUP_LOADS_8(intel_sub_group_block_read, (__global uint *)mem, stride, result)
    return result;
}

INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_32x16_i8_v8i8_pi32_i32(char *mem, int stride) {
    int8 result;
    stride = stride / 4; // char to int
    SUB_GROUP_LOADS_8(intel_sub_group_block_read, (__global uint *)mem, stride, result)
    return result;
}

/* Load accumulator is a special case of load packed A, both are row major: */
INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_8x8_i32_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, int, int, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_8x16_i32_v8i8_pi32_i32(char *mem, int stride) {
    if (stride == 16)
      return VEC_TO_VEC8(int, intel_sub_group_block_read8((__global uint *)mem));

    int8 result;
    SUB_GROUP_LOADS_8(intel_sub_group_block_read, (__global uint *)mem, stride, result)
    return result;
}

#define STORE_PACK_A_ROW_MAJOR(dst, stride, elem_t, contrib_t, M, N) \
    contrib_t *ptr = (contrib_t *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (contrib_t) / sizeof (elem_t); \
    stride = stride / pack_factor; \
    for (int i = 0; i < M; i++) \
        ptr[i * stride + slid] = row[i]; \

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x32_i8_pi64_v8i8(char *mem, int8 row, int stride) {
    STORE_PACK_A_ROW_MAJOR(mem, stride, char, int, 8, 32)
}

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x16_i16_pi64_v8i8(char *mem, int8 row, int stride) {
    STORE_PACK_A_ROW_MAJOR(mem, stride, short, int, 8, 16)
}

#define STORE_PACK_B_COL_MAJOR(dst, stride, elem_t, contrib_t, M, N) \
    contrib_t *ptr = (contrib_t *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (contrib_t) / sizeof (elem_t); \
    stride = stride / pack_factor; \
    for (int i = 0; i < M; i++) \
        ptr[i * stride + slid] = col[i]; \

INLINE int8 __builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_16x8_i16_pi64_v8i8(char *mem, int8 col, int stride) {
    STORE_PACK_B_COL_MAJOR(mem, stride, short, int, 8, 16)
}

INLINE int8 __builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_32x8_i8_pi64_v8i8(char *mem, int8 col, int stride) {
    STORE_PACK_B_COL_MAJOR(mem, stride, char, int, 8, 32)
}

INLINE int8 __builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_16x16_i16_pi64_v8i8(char *mem, int8 col, int stride) {
    STORE_PACK_B_COL_MAJOR(mem, stride, short, int, 8, 16)
}

#define STORE_ACC_ROW_MAJOR(dst, stride, M) \
    int *ptr = (int *)dst; \
    int slid = get_sub_group_local_id(); \
    for (int i = 0; i < M; i++) \
        ptr[slid + i * stride] = row[i]; \


#define STORE_BLOCK_ACC_ROW_MAJOR(dst, stride, M) \
    __global uint *ptr = (__global uint *)dst; \
    for (int i = 0; i < M; i++) \
        intel_sub_group_block_write(ptr + i * stride, (uint) row[i]);

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_8x8_i32_pi64_v8i8(char *mem, int8 row, int stride) {
    STORE_ACC_ROW_MAJOR(mem, stride, 8)
}

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_8x16_i32_pi64_v8i8(char *mem, int8 row, int stride) {
    if (stride == 16){
        intel_sub_group_block_write8((__global uint *)mem, VEC_TO_VEC8(uint, row));
        return;
    }
    STORE_BLOCK_ACC_ROW_MAJOR(mem, stride, 8)
}

#define STORE_ACC_COL_MAJOR(mem, stride, M) \
    int *ptr = (int *)mem; \
    int slid = get_sub_group_local_id(); \
    for (int i = 0; i < M; i++) \
        ptr[slid * stride + i] = row[i]; \

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_ColumnMajor_8x8_i32_pi64_v8i8(char *mem, int8 row, int stride) {
    STORE_ACC_COL_MAJOR(mem, stride, 8)
}

#define STORE_PACKED_A_ROW_MAJOR(mem, stride, element_type, contrib_type, M) \
    contrib_type *ptr = (contrib_type *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (contrib_type) / sizeof (element_type); \
    stride = stride / pack_factor; \
    for (int i = 0; i < M; i++) \
         ptr[slid + i * stride] = row[i]; \

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x16_i16_pi64_v8i8(char *mem, short8 row, int stride) {
    STORE_PACKED_A_ROW_MAJOR(mem, stride, short, short, 8)
}

INLINE void __builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x32_i8_pi64_v8i8(char *mem, short8 row, int stride) {
    STORE_PACKED_A_ROW_MAJOR(mem, stride, char, short, 8)
}
