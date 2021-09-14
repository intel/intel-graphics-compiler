/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define ARR_TO_VEC8(type, arr) \
    (type##8)(wi_contrib[0], wi_contrib[1], wi_contrib[2], wi_contrib[3], \
              wi_contrib[4], wi_contrib[5], wi_contrib[6], wi_contrib[7])

#define ARR_TO_VEC4(type, arr) \
    (type##4)(wi_contrib[0], wi_contrib[1], wi_contrib[2], wi_contrib[3])

#define ARR_TO_VEC2(type, arr) \
    (type##2)(wi_contrib[0], wi_contrib[1])

#define ARR_TO_VEC1(type, arr) \
    wi_contrib[0]

#define LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, element_type, contrib_type, M, K) \
    contrib_type *ptr = (contrib_type *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (int) / sizeof (element_type); \
    stride = stride / pack_factor; \
    contrib_type wi_contrib[M]; \
    for (int i = 0; i < M; i++) \
        wi_contrib[i] = *(ptr + slid + (i * stride)); \

/* PackedA load i16 */ 
INLINE int8 __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_8x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 8, 16)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int4 __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_4x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 4, 16)
    return ARR_TO_VEC4(int, wi_contrib);
}

INLINE int2 __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_2x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 2, 16)
    return ARR_TO_VEC2(int, wi_contrib);
}

INLINE int __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_1x16_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 2, 16)
    return ARR_TO_VEC1(int, wi_contrib);
}

/* PackedA load i8 */ 
INLINE int8 __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_8x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 8, 32)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int4 __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_4x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 4, 32)
    return ARR_TO_VEC4(int, wi_contrib);
}

INLINE int2 __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_2x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 4, 32)
    return ARR_TO_VEC2(int, wi_contrib);
}

INLINE int __builtin_spriv_OpMatrixLoadINTEL_PackedA_RowMajor_1x32_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 4, 32)
    return ARR_TO_VEC1(int, wi_contrib);
}


#define LOAD_PACKED_B_FROM_COL_MAJOR(mem, stride, element_type, N, K) \
    int *ptr = (int *)mem; \
    int slid = get_sub_group_local_id(); \
    int pack_factor = sizeof (int) / sizeof (element_type); \
    int wi_contrib[8]; \
    for (int i = 0; i < 8; i++) \
        wi_contrib[i] = *(ptr + i + (slid * stride)); \

INLINE int8 __builtin_spriv_OpMatrixLoadINTEL_PackedB_ColumnMajor_16x8_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_B_FROM_COL_MAJOR(mem, stride, short, 8, 16) 
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpMatrixLoadINTEL_PackedB_ColumnMajor_32x8_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_B_FROM_COL_MAJOR(mem, stride, char, 8, 32) 
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpMatrixLoadINTEL_PackedB_PackedB_16x8_i16_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, short, int, 8, 16)
    return ARR_TO_VEC8(int, wi_contrib);
}

INLINE int8 __builtin_spriv_OpMatrixLoadINTEL_PackedB_PackedB_32x8_i8_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, char, int, 8, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

/* Load accumulator is a special case of load packed A, both are row major: */
INLINE int8 __builtin_spriv_OpMatrixLoadINTEL_Accumulator_RowMajor_8x8_i32_v8i8_pi32_i32(char *mem, int stride) {
    LOAD_PACKED_A_FROM_ROW_MAJOR(mem, stride, int, int, 8, 8)
    return ARR_TO_VEC8(int, wi_contrib);
}

#define STORE_ACC_ROW_MAJOR(dst, stride, M, N) \
    int *ptr = (int *)mem; \
    int slid = get_sub_group_local_id(); \
    for (int i = 0; i < M; i++) \
        ptr[i * stride + slid] = row[i]; \

INLINE void __builtin_spriv_OpMatrixStoreINTEL_Accumulator_RowMajor_8x8_i32_pi64_v8i8(char *mem, int8 row, int stride) {
    STORE_ACC_ROW_MAJOR(mem, stride, 8, 8)
}

#define STORE_ACC_COL_MAJOR(mem, stride, M, N) \
    int *ptr = (int *)mem; \
    int slid = get_sub_group_local_id(); \
    for (int i = 0; i < M; i++) \
        ptr[slid * stride + i] = row[i]; \

INLINE void __builtin_spriv_OpMatrixStoreINTEL_Accumulator_ColumnMajor_8x8_i32_pi64_v8i8(char *mem, int8 row, int stride) {
    STORE_ACC_COL_MAJOR(mem, stride, 8, 8)
}
