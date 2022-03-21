/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define RADIX_SORT_SERIAL_SCAN 0  // For testing purposes, non-performant serial scan

constant uint RADIX_SORT_BITS_PER_PASS = 4;
constant uint RADIX_SORT_CHAR_BIT = 8;

/* Default devicelib sub-group sort - bitonic sorting network, value-only */

uint __builtin_sub_group_sort_mirror(uint idx, uint base)
{
    return base * (idx / base + 1) - 1 - (idx % base);
}
uint __builtin_sub_group_sort_rotate(uint idx, uint base)
{
    uint base2 = base / 2;
    return base * (idx / base) + base2 * (idx / base2 + 1) % base + idx % base2;
}
uint __builtin_sub_group_sort_sel(uint idx, uint base)
{
    return (idx % base) / (base / 2);
}


#define DEFN_DEFAULT_SUB_GROUP_STEPS(type)                                     \
type OVERLOADABLE __builtin_sub_group_sort_compare_exchange(                   \
    const type a0, const uint shuffleMask, const uint selectMask,              \
    const bool is_asc)                                                         \
{                                                                              \
    const type a1 = sub_group_shuffle(a0, shuffleMask);                        \
    const type a_min = min(a0,a1);                                             \
    const type a_max = max(a0,a1);                                             \
    return selectMask ?                                                        \
        (is_asc ? a_max : a_min) : (is_asc ? a_min : a_max);                   \
}                                                                              \
type OVERLOADABLE __builtin_sub_group_sort8(const type aa, const bool is_asc)  \
{                                                                              \
    const uint slotID = get_sub_group_local_id();                              \
    const type bb = __builtin_sub_group_sort_compare_exchange(aa,              \
        __builtin_sub_group_sort_mirror(slotID, 2),                            \
        __builtin_sub_group_sort_sel(slotID, 2), is_asc);                      \
    const type cc = __builtin_sub_group_sort_compare_exchange(bb,              \
        __builtin_sub_group_sort_mirror(slotID, 4),                            \
        __builtin_sub_group_sort_sel(slotID, 4), is_asc);                      \
    const type dd = __builtin_sub_group_sort_compare_exchange(cc,              \
        __builtin_sub_group_sort_mirror(slotID, 2),                            \
        __builtin_sub_group_sort_sel(slotID, 2), is_asc);                      \
    const type ee = __builtin_sub_group_sort_compare_exchange(dd,              \
        __builtin_sub_group_sort_mirror(slotID, 8),                            \
        __builtin_sub_group_sort_sel(slotID, 8), is_asc);                      \
    const type ff = __builtin_sub_group_sort_compare_exchange(ee,              \
        __builtin_sub_group_sort_rotate(slotID, 4),                            \
        __builtin_sub_group_sort_sel(slotID, 4), is_asc);                      \
    const type gg = __builtin_sub_group_sort_compare_exchange(ff,              \
        __builtin_sub_group_sort_rotate(slotID, 2),                            \
        __builtin_sub_group_sort_sel(slotID, 2), is_asc);                      \
    return gg;                                                                 \
}                                                                              \
type OVERLOADABLE __builtin_sub_group_sort16(const type aa, const bool is_asc) \
{                                                                              \
    const uint slotID = get_sub_group_local_id();                              \
    const type bb = __builtin_sub_group_sort8(aa, is_asc);                     \
    const type cc = __builtin_sub_group_sort_compare_exchange(bb,              \
        __builtin_sub_group_sort_mirror(slotID, 16),                           \
        __builtin_sub_group_sort_sel(slotID, 16),is_asc);                      \
    const type dd = __builtin_sub_group_sort_compare_exchange(cc,              \
        __builtin_sub_group_sort_rotate(slotID, 8),                            \
        __builtin_sub_group_sort_sel(slotID, 8), is_asc);                      \
    const type ee = __builtin_sub_group_sort_compare_exchange(dd,              \
        __builtin_sub_group_sort_rotate(slotID, 4),                            \
        __builtin_sub_group_sort_sel(slotID, 4), is_asc);                      \
    const type ff = __builtin_sub_group_sort_compare_exchange(ee,              \
        __builtin_sub_group_sort_rotate(slotID, 2),                            \
        __builtin_sub_group_sort_sel(slotID, 2), is_asc);                      \
    return ff;                                                                 \
}                                                                              \
type OVERLOADABLE __builtin_sub_group_sort32(const type aa, const bool is_asc) \
{                                                                              \
    const uint slotID = get_sub_group_local_id();                              \
    const type bb = __builtin_sub_group_sort16(aa, is_asc);                    \
    const type cc = __builtin_sub_group_sort_compare_exchange(bb,              \
        __builtin_sub_group_sort_mirror(slotID, 32),                           \
        __builtin_sub_group_sort_sel(slotID, 32), is_asc);                     \
    const type dd = __builtin_sub_group_sort_compare_exchange(cc,              \
        __builtin_sub_group_sort_rotate(slotID, 16),                           \
        __builtin_sub_group_sort_sel(slotID, 16), is_asc);                     \
    const type ee = __builtin_sub_group_sort_compare_exchange(dd,              \
        __builtin_sub_group_sort_rotate(slotID, 8),                            \
        __builtin_sub_group_sort_sel(slotID, 8), is_asc);                      \
    const type ff = __builtin_sub_group_sort_compare_exchange(ee,              \
        __builtin_sub_group_sort_rotate(slotID, 4),                            \
        __builtin_sub_group_sort_sel(slotID, 4), is_asc);                      \
    const type gg = __builtin_sub_group_sort_compare_exchange(ff,              \
        __builtin_sub_group_sort_rotate(slotID, 2),                            \
        __builtin_sub_group_sort_sel(slotID, 2), is_asc);                      \
    return gg;                                                                 \
}


DEFN_DEFAULT_SUB_GROUP_STEPS(uchar)
DEFN_DEFAULT_SUB_GROUP_STEPS(ushort)
DEFN_DEFAULT_SUB_GROUP_STEPS(uint)
DEFN_DEFAULT_SUB_GROUP_STEPS(ulong)

DEFN_DEFAULT_SUB_GROUP_STEPS(char)
DEFN_DEFAULT_SUB_GROUP_STEPS(short)
DEFN_DEFAULT_SUB_GROUP_STEPS(int)
DEFN_DEFAULT_SUB_GROUP_STEPS(long)

#if defined(cl_khr_fp16)
DEFN_DEFAULT_SUB_GROUP_STEPS(half)
#endif
DEFN_DEFAULT_SUB_GROUP_STEPS(float)
#if defined(cl_khr_fp64)
DEFN_DEFAULT_SUB_GROUP_STEPS(double)
#endif


#define DEFN_DEFAULT_SUB_GROUP_SORT_KEY_ONLY(                                  \
    type, type_abbr, direction, is_asc)                                        \
type __devicelib_default_sub_group_private_sort_##direction##_##type_abbr(     \
    type value)                                                                \
{                                                                              \
    const uint local_size = get_local_size(0);                                 \
    const uint idx = get_local_id(0);                                          \
                                                                               \
    const bool is_comp_asc = is_asc;                                           \
    uint sg_size = get_max_sub_group_size();                                   \
    type sorted;                                                               \
    switch(sg_size) {                                                          \
        case 8:                                                                \
            sorted = __builtin_sub_group_sort8(value, is_comp_asc);            \
            break;                                                             \
        case 16:                                                               \
            sorted = __builtin_sub_group_sort16(value, is_comp_asc);           \
            break;                                                             \
        case 32:                                                               \
            sorted = __builtin_sub_group_sort32(value, is_comp_asc);           \
            break;                                                             \
        default:                                                               \
            break;                                                             \
    }                                                                          \
    return sorted;                                                             \
}

/* Default devicelib work-group sort - radix, key and key-value */

//Unsigned integers
uchar OVERLOADABLE __builtin_radix_sort_convert_to_ordered(uchar value)
{
    return value;
}
ushort OVERLOADABLE __builtin_radix_sort_convert_to_ordered(ushort value)
{
    return value;
}
uint OVERLOADABLE __builtin_radix_sort_convert_to_ordered(uint value)
{
    return value;
}
ulong OVERLOADABLE __builtin_radix_sort_convert_to_ordered(ulong value)
{
    return value;
}

//Signed integers
uchar OVERLOADABLE __builtin_radix_sort_convert_to_ordered(char value)
{
    uchar uvalue = *((uchar*) &value);
    uvalue ^= 0x80;
    return uvalue;
}
ushort OVERLOADABLE __builtin_radix_sort_convert_to_ordered(short value)
{
    ushort uvalue = *((ushort*) &value);
    uvalue ^= 0x8000;
    return uvalue;
}
uint OVERLOADABLE __builtin_radix_sort_convert_to_ordered(int value)
{
    uint uvalue = *((uint *) &value);
    uvalue ^= 0x80000000;
    return uvalue;
}
ulong OVERLOADABLE __builtin_radix_sort_convert_to_ordered(long value)
{
    ulong uvalue = *((ulong *) &value);
    uvalue ^= (ulong) 0x8000000000000000;
    return uvalue;
}

//Float types
#if defined(cl_khr_fp16)
ushort OVERLOADABLE __builtin_radix_sort_convert_to_ordered(half value)
{
    ushort uvalue = *((ushort*) &value);

    bool is_negative = (uvalue >> (15)) == 1;
    ushort ordered_mask =
        (is_negative * 0xFFFF) | 0x8000;

    return uvalue ^ ordered_mask;
}
#endif
uint OVERLOADABLE __builtin_radix_sort_convert_to_ordered(float value)
{
    uint uvalue = *((uint*) &value);

    bool is_negative = (uvalue >> (31)) == 1;
    uint ordered_mask =
        (is_negative * 0xFFFFFFFF) | 0x80000000;

    return uvalue ^ ordered_mask;
}
#if defined(cl_khr_fp64)
ulong OVERLOADABLE __builtin_radix_sort_convert_to_ordered(double value)
{
    ulong uvalue = *((ulong*) &value);

    bool is_negative = (uvalue >> (63)) == 1;
    ulong ordered_mask =
        (is_negative * (ulong) 0xFFFFFFFFFFFFFFFF) | (ulong) 0x8000000000000000;

    return uvalue ^ ordered_mask;
}
#endif


#if !RADIX_SORT_SERIAL_SCAN
void __builtin_radix_sort_scan(uint radix_states, uint* scan_memory)
{
    const uint idx = get_local_id(0);

    /* 2.1 Scan. Upsweep: reduce over radix states */
    uint reduced = 0;
    for (uint i = 0; i < radix_states; ++i)
    {
        reduced += scan_memory[idx * radix_states + i];
    }

    /* 2.2. Exclusive scan: over work items */
    uint scanned = __builtin_IB_WorkGroupScanExclusive_IAdd_i32(reduced);

    /* 2.3. Exclusive downsweep: exclusive scan over radix states */
    for (uint i = 0; i < radix_states; ++i)
    {
        uint value = scan_memory[idx * radix_states + i];
        scan_memory[idx * radix_states + i] = scanned;
        scanned += value;
    }
}
#else
void __builtin_radix_sort_scan(uint radix_states, uint* scan_memory)
{
    /* 2.1 Scan (serial) */
    const uint idx = get_local_id(0);
    const uint local_size = get_local_size(0);
    if (idx == 0)
    {
        uint sum = 0;
        for (uint i = 0; i < local_size * radix_states; ++i)
        {
            uint tmp = scan_memory[i];
            scan_memory[i] = sum;
            sum += tmp;
        }
    }
}
#endif


#define DEFN_RADIX_WORK_GROUP_BUCKET_VALUE(ordered_type)                       \
uint OVERLOADABLE __builtin_radix_sort_get_bucket_value(                       \
    bool is_comp_asc, ordered_type uvalue, uint radix_iter)                    \
{                                                                              \
    /* invert value if we need to sort in descending order  */                 \
    if (!is_comp_asc) {                                                        \
        uvalue = uvalue ^ ((ordered_type) -1);                                 \
    }                                                                          \
                                                                               \
    /* get bucket offset idx from the end of bit type (least significant bits) */\
    uint bucket_offset = radix_iter * RADIX_SORT_BITS_PER_PASS;                \
                                                                               \
    /* get offset mask for one bucket */                                       \
    uint bucket_mask = (1u << RADIX_SORT_BITS_PER_PASS) - 1u;                  \
                                                                               \
    /* get bits under bucket mask */                                           \
    return ((uvalue >> bucket_offset) & bucket_mask);                          \
}


DEFN_RADIX_WORK_GROUP_BUCKET_VALUE(uchar)
DEFN_RADIX_WORK_GROUP_BUCKET_VALUE(ushort)
DEFN_RADIX_WORK_GROUP_BUCKET_VALUE(uint)
DEFN_RADIX_WORK_GROUP_BUCKET_VALUE(ulong)


/* Count step is common for key and key-value variants */
#define DEFN_DEFAULT_WORK_GROUP_SORT_STEPS(type, ordered_type)                 \
void OVERLOADABLE __builtin_radix_sort_count(                                  \
    bool is_comp_asc, bool is_temp_local_mem, uint items_per_work_item,        \
    uint radix_iter, uint n, uint start, type* keys_input, uint* scan_memory)  \
{                                                                              \
    const uint idx = get_local_id(0);                                          \
    const uint local_size = get_local_size(0);                                 \
                                                                               \
    const uint radix_states = 1U << RADIX_SORT_BITS_PER_PASS;                  \
                                                                               \
    /* 1.1. count per witem: Init local scan memory */                         \
    for (uint state = 0; state < radix_states; ++state)                        \
    {                                                                          \
        scan_memory[state * local_size + idx] = 0;                             \
    }                                                                          \
    if (is_temp_local_mem)                                                     \
        work_group_barrier(CLK_LOCAL_MEM_FENCE);                               \
    else                                                                       \
        work_group_barrier(CLK_GLOBAL_MEM_FENCE);                              \
                                                                               \
    /* 1.2. count per witem: count values and write result to private          \
            count array and count memory */                                    \
    for (uint i = 0; i < items_per_work_item; ++i)                             \
    {                                                                          \
        uint val_idx = start + i;                                              \
        if(val_idx < n)                                                        \
        {                                                                      \
            /* get value, convert it to ordered (in terms of bitness) */       \
            ordered_type val = __builtin_radix_sort_convert_to_ordered(        \
                keys_input[val_idx]);                                          \
                                                                               \
            /* get bit values in a certain bucket of a value */                \
            uint bucket_val = __builtin_radix_sort_get_bucket_value(           \
                is_comp_asc, val, radix_iter);                                 \
                                                                               \
            /* increment counter for this bit bucket */                        \
            scan_memory[bucket_val * local_size + idx]++;                      \
        }                                                                      \
    }                                                                          \
}                                                                              \
                                                                               \
/* Key-only reorder step */                                                    \
void OVERLOADABLE __builtin_radix_sort_reorder(                                \
    bool is_comp_asc, uint items_per_work_item, uint radix_iter, uint n,       \
    uint start, type* keys_input, type* keys_output, uint* scan_memory)        \
{                                                                              \
    const uint idx = get_local_id(0);                                          \
    const uint local_size = get_local_size(0);                                 \
    const uint radix_states = 1U << RADIX_SORT_BITS_PER_PASS;                  \
                                                                               \
    uint private_scan_memory[radix_states];                                    \
    for (int i = 0; i < radix_states; ++i) {                                   \
        private_scan_memory[i] = 0;                                            \
    }                                                                          \
                                                                               \
    /* 3. Reorder */                                                           \
    for (uint i = 0; i < items_per_work_item; ++i)                             \
    {                                                                          \
        uint val_idx = start + i;                                              \
        if(val_idx < n)                                                        \
        {                                                                      \
            /* get value, convert it to ordered (in terms of bitness) */       \
            ordered_type val = __builtin_radix_sort_convert_to_ordered(        \
                keys_input[val_idx]);                                          \
                                                                               \
            /* get bit values in a certain bucket of a value */                \
            uint bucket_val = __builtin_radix_sort_get_bucket_value(           \
                is_comp_asc, val, radix_iter);                                 \
                                                                               \
            uint new_offset_idx = private_scan_memory[bucket_val]++ +          \
                scan_memory[bucket_val * local_size + idx];                    \
            keys_output[new_offset_idx] = keys_input[val_idx];                 \
        }                                                                      \
    }                                                                          \
}                                                                              \
                                                                               \
/* Merge sort steps */                                                         \
uint OVERLOADABLE __builtin_merge_lower_bound(                                 \
    type* in, uint first, uint last, type value, bool is_asc)                  \
{                                                                              \
    uint n = last - first;                                                     \
    uint cur = n;                                                              \
    uint it;                                                                   \
    while (n > 0) {                                                            \
        it = first;                                                            \
        cur = n / 2;                                                           \
        it += cur;                                                             \
        if (is_asc) {                                                          \
            if (in[it] < value)                                                \
                n -= cur + 1, first = ++it;                                    \
            else                                                               \
                n = cur;                                                       \
        } else {                                                               \
            if (in[it] > value)                                                \
                n -= cur + 1, first = ++it;                                    \
            else                                                               \
                n = cur;                                                       \
        }                                                                      \
    }                                                                          \
    return first;                                                              \
}                                                                              \
                                                                               \
uint OVERLOADABLE __builtin_merge_upper_bound(                                 \
    type* in, uint first, uint last, type value, bool is_asc)                  \
{                                                                              \
    uint n = last - first;                                                     \
    uint cur = n;                                                              \
    uint it;                                                                   \
    while (n > 0) {                                                            \
        it = first;                                                            \
        cur = n / 2;                                                           \
        it += cur;                                                             \
        if (is_asc) {                                                          \
            if (in[it] > value)                                                \
                n = cur;                                                       \
            else                                                               \
                n -= cur + 1, first = ++it;                                    \
        } else {                                                               \
            if (in[it] < value)                                                \
                n = cur;                                                       \
            else                                                               \
                n -= cur + 1, first = ++it;                                    \
        }                                                                      \
    }                                                                          \
    return first;                                                              \
}                                                                              \
                                                                               \
void OVERLOADABLE __builtin_swap(type* in, const uint i1, const uint i2)       \
{                                                                              \
    type tmp = in[i1];                                                         \
    in[i1] = in[i2];                                                           \
    in[i2] = tmp;                                                              \
}                                                                              \
                                                                               \
void OVERLOADABLE __builtin_bubble_sort(                                       \
    type* first, const uint begin, const uint end, bool is_asc)                \
{                                                                              \
    for (uint i = begin; i < end; ++i) {                                       \
        for (uint idx = i + 1; idx < end; ++idx) {                             \
            if (is_asc) {                                                      \
                if(first[idx] < first[i]) {                                    \
                    __builtin_swap(first, idx, i);                             \
                }                                                              \
            } else {                                                           \
                if(first[idx] > first[i]) {                                    \
                    __builtin_swap(first, idx, i);                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }                                                                          \
}                                                                              \
                                                                               \
void OVERLOADABLE __builtin_merge_step(const uint offset, type* keys_input,    \
    type* keys_output, const uint start_1, const uint end_1, const uint end_2, \
    const uint start_out, const uint chunk, bool is_asc)                       \
{                                                                              \
    const uint start_2 = end_1;                                                \
    /* Borders of the sequences to merge within this call */                   \
    const uint local_start_1 = min(offset + start_1, end_1);                   \
    const uint local_end_1 = min(local_start_1 + chunk, end_1);                \
    const uint local_start_2 = min(offset + start_2, end_2);                   \
    const uint local_end_2 = min(local_start_2 + chunk, end_2);                \
                                                                               \
    const uint local_size_1 = local_end_1 - local_start_1;                     \
    const uint local_size_2 = local_end_2 - local_start_2;                     \
                                                                               \
    /* Process 1st sequence */                                                 \
    if (local_start_1 < local_end_1)                                           \
    {                                                                          \
        /* Reduce the range for searching within the 2nd sequence              \
           and handle bound items */                                           \
        /* find left border in 2nd sequence */                                 \
        const type local_l_item_1 = keys_input[local_start_1];                 \
        uint l_search_bound_2 = __builtin_merge_lower_bound(                   \
            keys_input, start_2, end_2, local_l_item_1, is_asc);               \
        const uint l_shift_1 = local_start_1 - start_1;                        \
        const uint l_shift_2 = l_search_bound_2 - start_2;                     \
                                                                               \
        keys_output[start_out + l_shift_1 + l_shift_2] = local_l_item_1;       \
                                                                               \
        /* find right border in 2nd sequence */                                \
        uint r_search_bound_2;                                                 \
        if (local_size_1 > 1)                                                  \
        {                                                                      \
            const type local_r_item_1 = keys_input[local_end_1 - 1];           \
            r_search_bound_2 = __builtin_merge_lower_bound(                    \
                keys_input, l_search_bound_2, end_2, local_r_item_1, is_asc);  \
            const uint r_shift_1 = local_end_1 - 1 - start_1;                  \
            const uint r_shift_2 = r_search_bound_2 - start_2;                 \
                                                                               \
            keys_output[start_out + r_shift_1 + r_shift_2] = local_r_item_1;   \
        }                                                                      \
                                                                               \
        /* Handle intermediate items */                                        \
        for (uint idx = local_start_1 + 1; idx < local_end_1 - 1; ++idx)       \
        {                                                                      \
            const type intermediate_item_1 = keys_input[idx];                  \
            /* we shouldn't seek in whole 2nd sequence.                        \
               Just for the part where the 1st sequence should be */           \
            l_search_bound_2 = __builtin_merge_lower_bound(                    \
                keys_input, l_search_bound_2, r_search_bound_2,                \
                intermediate_item_1, is_asc);                                  \
            const uint shift_1 = idx - start_1;                                \
            const uint shift_2 = l_search_bound_2 - start_2;                   \
                                                                               \
            keys_output[start_out + shift_1 + shift_2] = intermediate_item_1;  \
        }                                                                      \
    }                                                                          \
    /* Process 2nd sequence */                                                 \
    if (local_start_2 < local_end_2)                                           \
    {                                                                          \
        /* Reduce the range for searching within the 1st sequence              \
           and handle bound items */                                           \
        /* find left border in 1st sequence */                                 \
        const type local_l_item_2 = keys_input[local_start_2];                 \
        uint l_search_bound_1 = __builtin_merge_upper_bound(                   \
            keys_input, start_1, end_1, local_l_item_2, is_asc);               \
        const uint l_shift_1 = l_search_bound_1 - start_1;                     \
        const uint l_shift_2 = local_start_2 - start_2;                        \
                                                                               \
        keys_output[start_out + l_shift_1 + l_shift_2] = local_l_item_2;       \
                                                                               \
        uint r_search_bound_1;                                                 \
        /* find right border in 1st sequence */                                \
        if (local_size_2 > 1)                                                  \
        {                                                                      \
            const type local_r_item_2 = keys_input[local_end_2 - 1];           \
            r_search_bound_1 = __builtin_merge_upper_bound(                    \
                keys_input, l_search_bound_1, end_1, local_r_item_2, is_asc);  \
            const uint r_shift_1 = r_search_bound_1 - start_1;                 \
            const uint r_shift_2 = local_end_2 - 1 - start_2;                  \
                                                                               \
            keys_output[start_out + r_shift_1 + r_shift_2] = local_r_item_2;   \
        }                                                                      \
                                                                               \
        /* Handle intermediate items */                                        \
        for (uint idx = local_start_2 + 1; idx < local_end_2 - 1; ++idx)       \
        {                                                                      \
            const type intermediate_item_2 = keys_input[idx];                  \
            /* we shouldn't seek in whole 1st sequence.                        \
               Just for the part where the 2nd sequence should be */           \
            l_search_bound_1 = __builtin_merge_upper_bound(                    \
                keys_input, l_search_bound_1, r_search_bound_1,                \
                intermediate_item_2, is_asc);                                  \
            const uint shift_1 = l_search_bound_1 - start_1;                   \
            const uint shift_2 = idx - start_2;                                \
                                                                               \
            keys_output[start_out + shift_1 + shift_2] = intermediate_item_2;  \
        }                                                                      \
    }                                                                          \
}


/* Key-value reorder step */
#define DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type,             \
    values_type)                                                               \
void OVERLOADABLE __builtin_radix_sort_reorder(                                \
    bool is_comp_asc, uint items_per_work_item, uint radix_iter, uint n,       \
    uint start, type* keys_input, type* keys_output,                           \
    values_type* values_input, values_type* values_output, uint* scan_memory)  \
{                                                                              \
    const uint idx = get_local_id(0);                                          \
    const uint local_size = get_local_size(0);                                 \
    const uint radix_states = 1U << RADIX_SORT_BITS_PER_PASS;                  \
                                                                               \
    uint private_scan_memory[radix_states];                                    \
    for (int i = 0; i < radix_states; ++i) {                                   \
        private_scan_memory[i] = 0;                                            \
    }                                                                          \
                                                                               \
    /* 3. Reorder */                                                           \
    for (uint i = 0; i < items_per_work_item; ++i)                             \
    {                                                                          \
        uint val_idx = start + i;                                              \
        if(val_idx < n)                                                        \
        {                                                                      \
            /* get value, convert it to ordered (in terms of bitness) */       \
            ordered_type val = __builtin_radix_sort_convert_to_ordered(        \
                keys_input[val_idx]);                                          \
                                                                               \
            /* get bit values in a certain bucket of a value */                \
            uint bucket_val = __builtin_radix_sort_get_bucket_value(           \
                is_comp_asc, val, radix_iter);                                 \
                                                                               \
            uint new_offset_idx = private_scan_memory[bucket_val]++ +          \
                scan_memory[bucket_val * local_size + idx];                    \
            keys_output[new_offset_idx] = keys_input[val_idx];                 \
            values_output[new_offset_idx] = values_input[val_idx];             \
        }                                                                      \
    }                                                                          \
}                                                                              \
                                                                               \
void OVERLOADABLE __builtin_swap(type* in_k, values_type* in_v,                \
    const uint i1, const uint i2)                                              \
{                                                                              \
    type tmp = in_k[i1];                                                       \
    in_k[i1] = in_k[i2];                                                       \
    in_k[i2] = tmp;                                                            \
                                                                               \
    values_type tmpv = in_v[i1];                                               \
    in_v[i1] = in_v[i2];                                                       \
    in_v[i2] = tmpv;                                                           \
}                                                                              \
                                                                               \
/* merge sort key-value steps */                                               \
void OVERLOADABLE __builtin_bubble_sort(                                       \
    type* first, values_type* values_first, const uint begin, const uint end,  \
    bool is_asc)                                                               \
{                                                                              \
    for (uint i = begin; i < end; ++i) {                                       \
        for (uint idx = i + 1; idx < end; ++idx) {                             \
            if (is_asc) {                                                      \
                if(first[idx] < first[i]) {                                    \
                    __builtin_swap(first, values_first, idx, i);               \
                }                                                              \
            } else {                                                           \
                if(first[idx] > first[i]) {                                    \
                    __builtin_swap(first, values_first, idx, i);               \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }                                                                          \
}                                                                              \
                                                                               \
void OVERLOADABLE __builtin_merge_step(const uint offset, type* keys_input,    \
    type* keys_output, values_type* values_input, values_type* values_output,  \
    const uint start_1, const uint end_1, const uint end_2,                    \
    const uint start_out, const uint chunk, bool is_asc)                       \
{                                                                              \
    const uint start_2 = end_1;                                                \
    /* Borders of the sequences to merge within this call */                   \
    const uint local_start_1 = min(offset + start_1, end_1);                   \
    const uint local_end_1 = min(local_start_1 + chunk, end_1);                \
    const uint local_start_2 = min(offset + start_2, end_2);                   \
    const uint local_end_2 = min(local_start_2 + chunk, end_2);                \
                                                                               \
    const uint local_size_1 = local_end_1 - local_start_1;                     \
    const uint local_size_2 = local_end_2 - local_start_2;                     \
                                                                               \
    /* Process 1st sequence */                                                 \
    if (local_start_1 < local_end_1)                                           \
    {                                                                          \
        /* Reduce the range for searching within the 2nd sequence              \
           and handle bound items */                                           \
        /* find left border in 2nd sequence */                                 \
        const type local_l_item_1 = keys_input[local_start_1];                 \
        uint l_search_bound_2 = __builtin_merge_lower_bound(                   \
            keys_input, start_2, end_2, local_l_item_1, is_asc);               \
        const uint l_shift_1 = local_start_1 - start_1;                        \
        const uint l_shift_2 = l_search_bound_2 - start_2;                     \
                                                                               \
        keys_output[start_out + l_shift_1 + l_shift_2] = local_l_item_1;       \
        values_output[start_out + l_shift_1 + l_shift_2] =                     \
            values_input[local_start_1];                                       \
                                                                               \
        /* find right border in 2nd sequence */                                \
        uint r_search_bound_2;                                                 \
        if (local_size_1 > 1)                                                  \
        {                                                                      \
            const type local_r_item_1 = keys_input[local_end_1 - 1];           \
            r_search_bound_2 = __builtin_merge_lower_bound(                    \
                keys_input, l_search_bound_2, end_2, local_r_item_1, is_asc);  \
            const uint r_shift_1 = local_end_1 - 1 - start_1;                  \
            const uint r_shift_2 = r_search_bound_2 - start_2;                 \
                                                                               \
            keys_output[start_out + r_shift_1 + r_shift_2] = local_r_item_1;   \
            values_output[start_out + r_shift_1 + r_shift_2] =                 \
                values_input[local_end_1 - 1];                                 \
        }                                                                      \
                                                                               \
        /* Handle intermediate items */                                        \
        for (uint idx = local_start_1 + 1; idx < local_end_1 - 1; ++idx)       \
        {                                                                      \
            const type intermediate_item_1 = keys_input[idx];                  \
            /* we shouldn't seek in whole 2nd sequence.                        \
               Just for the part where the 1st sequence should be */           \
            l_search_bound_2 = __builtin_merge_lower_bound(                    \
                keys_input, l_search_bound_2, r_search_bound_2,                \
                intermediate_item_1, is_asc);                                  \
            const uint shift_1 = idx - start_1;                                \
            const uint shift_2 = l_search_bound_2 - start_2;                   \
                                                                               \
            keys_output[start_out + shift_1 + shift_2] = intermediate_item_1;  \
            values_output[start_out + shift_1 + shift_2] =                     \
                values_input[idx];                                             \
        }                                                                      \
    }                                                                          \
    /* Process 2nd sequence */                                                 \
    if (local_start_2 < local_end_2)                                           \
    {                                                                          \
        /* Reduce the range for searching within the 1st sequence              \
           and handle bound items */                                           \
        /* find left border in 1st sequence */                                 \
        const type local_l_item_2 = keys_input[local_start_2];                 \
        uint l_search_bound_1 = __builtin_merge_upper_bound(                   \
            keys_input, start_1, end_1, local_l_item_2, is_asc);               \
        const uint l_shift_1 = l_search_bound_1 - start_1;                     \
        const uint l_shift_2 = local_start_2 - start_2;                        \
                                                                               \
        keys_output[start_out + l_shift_1 + l_shift_2] = local_l_item_2;       \
        values_output[start_out + l_shift_1 + l_shift_2] =                     \
                values_input[local_start_2];                                   \
                                                                               \
        uint r_search_bound_1;                                                 \
        /* find right border in 1st sequence */                                \
        if (local_size_2 > 1)                                                  \
        {                                                                      \
            const type local_r_item_2 = keys_input[local_end_2 - 1];           \
            r_search_bound_1 = __builtin_merge_upper_bound(                    \
                keys_input, l_search_bound_1, end_1, local_r_item_2, is_asc);  \
            const uint r_shift_1 = r_search_bound_1 - start_1;                 \
            const uint r_shift_2 = local_end_2 - 1 - start_2;                  \
                                                                               \
            keys_output[start_out + r_shift_1 + r_shift_2] = local_r_item_2;   \
            values_output[start_out + r_shift_1 + r_shift_2] =                 \
                values_input[local_end_2 - 1];                                 \
        }                                                                      \
                                                                               \
        /* Handle intermediate items */                                        \
        for (uint idx = local_start_2 + 1; idx < local_end_2 - 1; ++idx)       \
        {                                                                      \
            const type intermediate_item_2 = keys_input[idx];                  \
            /* we shouldn't seek in whole 1st sequence.                        \
               Just for the part where the 2nd sequence should be */           \
            l_search_bound_1 = __builtin_merge_upper_bound(                    \
                keys_input, l_search_bound_1, r_search_bound_1,                \
                intermediate_item_2, is_asc);                                  \
            const uint shift_1 = l_search_bound_1 - start_1;                   \
            const uint shift_2 = idx - start_2;                                \
                                                                               \
            keys_output[start_out + shift_1 + shift_2] = intermediate_item_2;  \
            values_output[start_out + shift_1 + shift_2] =                     \
                values_input[idx];                                             \
        }                                                                      \
    }                                                                          \
}

#define DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(type, ordered_type)             \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_STEPS(type, ordered_type)                       \
                                                                               \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, uchar)    \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, ushort)   \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, uint)     \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, ulong)    \
                                                                               \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, char)     \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, short)    \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, int)      \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, long)     \
                                                                               \
    DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(type, ordered_type, float)    \

/* All types, without half and double */
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(uchar, uchar)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(ushort, ushort)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(uint, uint)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(ulong, ulong)

DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(char, uchar)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(short, ushort)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(int, uint)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(long, ulong)

#if defined(cl_khr_fp16)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(half, ushort)
#endif
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(float, uint)
#if defined(cl_khr_fp64)
DEFN_DEFAULT_WORK_GROUP_SORT_STEPS_ALL(double, ulong)
#endif

/* half and double separately */
#if defined(cl_khr_fp16)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(uchar, uchar, half)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(ushort, ushort, half)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(uint, uint, half)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(ulong, ulong, half)

DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(char, uchar, half)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(short, ushort, half)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(int, uint, half)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(long, ulong, half)

DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(half, ushort, half)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(float, uint, half)
#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(double, ulong, half)
#endif  //cl_khr_fp64
#endif  //cl_khr_fp16

#if defined(cl_khr_fp64)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(uchar, uchar, double)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(ushort, ushort, double)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(uint, uint, double)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(ulong, ulong, double)

DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(char, uchar, double)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(short, ushort, double)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(int, uint, double)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(long, ulong, double)

#if defined(cl_khr_fp16)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(half, ushort, double)
#endif  //cl_khr_fp16
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(float, uint, double)
DEFN_WORK_GROUP_SORT_KEY_VALUE_REORDER(double, ulong, double)
#endif  //cl_khr_fp64


/* Key-only work-group sort interfaces */
#define DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                            \
    type_abbr, direction, is_asc,                                              \
    sort_local_mem, temp_local_mem)                                            \
/* joint sort */                                                               \
void __devicelib_default_work_group_joint_sort_##direction##_##type_abbr(      \
        type* first, uint n, char* scratch)                                    \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY_BODY(                                \
        type, true, is_asc, false, sort_local_mem, temp_local_mem)             \
/* private sort close */                                                       \
void __devicelib_default_work_group_private_sort_close_##direction##_##type_abbr( \
        type* first, uint n, char* scratch)                                    \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY_BODY(                                \
        type, false, is_asc, false, sort_local_mem, temp_local_mem)            \
/* private sort spread */                                                      \
void __devicelib_default_work_group_private_sort_spread_##direction##_##type_abbr( \
        type* first, uint n, char* scratch)                                    \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY_BODY(                                \
        type, false, is_asc, true, sort_local_mem, temp_local_mem)

/* Key-only work-group sort algorithms */
#define DEFN_DEFAULT_WORK_GROUP_RADIX_SORT_KEY_ONLY_BODY(                      \
    type, is_joint, is_asc, is_spread, sort_local_mem, temp_local_mem)         \
{                                                                              \
    const uint local_size = get_local_size(0);                                 \
    const uint idx = get_local_id(0);                                          \
                                                                               \
    const bool is_comp_asc = is_asc;                                           \
    const bool is_joint_sort = is_joint;                                       \
                                                                               \
    const uint radix_states = 1U << RADIX_SORT_BITS_PER_PASS;                  \
                                                                               \
    const uint last_iter = sizeof(type) * RADIX_SORT_CHAR_BIT /                \
        RADIX_SORT_BITS_PER_PASS;                                              \
                                                                               \
    const uint items_per_work_item = is_joint_sort ?                           \
        (n - 1) / local_size + 1 : n;                                          \
                                                                               \
    type* keys_input = first;                                                  \
                                                                               \
    uint* scan_memory = (uint*) scratch;                                       \
    type* keys_output = (type*) ((char*) scratch +                             \
        radix_states * local_size * sizeof(uint));                             \
                                                                               \
    uint start = is_joint_sort ? items_per_work_item * idx : 0;                \
                                                                               \
    for (uint radix_iter = 0; radix_iter < last_iter; ++radix_iter) {          \
                                                                               \
        __builtin_radix_sort_count(is_comp_asc, temp_local_mem,                \
            items_per_work_item, radix_iter, n, start, keys_input,             \
            scan_memory);                                                      \
                                                                               \
        if (temp_local_mem)                                                    \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
                                                                               \
        __builtin_radix_sort_scan(radix_states, scan_memory);                  \
                                                                               \
        if (temp_local_mem)                                                    \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
                                                                               \
        __builtin_radix_sort_reorder(is_comp_asc, items_per_work_item,         \
            radix_iter, n, start, keys_input, keys_output, scan_memory);       \
                                                                               \
        if (sort_local_mem)                                                    \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
                                                                               \
        if (is_joint_sort) {                                                   \
            type* tmp = keys_output;                                           \
            keys_output = keys_input;                                          \
            keys_input = tmp;                                                  \
        } else {  /* is private mem sort */                                    \
            for (uint i = 0; i < items_per_work_item; ++i) {                   \
                bool spread = is_spread && (radix_iter == last_iter - 1);      \
                if (!spread) {                                                 \
                    keys_input[i] = keys_output[items_per_work_item * idx + i]; \
                }                                                              \
                else {                                                         \
                    keys_input[i] = keys_output[local_size * i + idx];         \
                }                                                              \
            }                                                                  \
        }                                                                      \
                                                                               \
    }                                                                          \
}

#define DEFN_DEFAULT_WORK_GROUP_MERGE_SORT_KEY_ONLY_BODY(                      \
    type, is_joint, is_asc, is_spread, sort_local_mem, temp_local_mem)         \
{                                                                              \
    const uint local_size = get_local_size(0);                                 \
    const uint idx = get_local_id(0);                                          \
                                                                               \
    bool is_sort_local_mem = sort_local_mem;                                   \
    bool is_temp_local_mem = temp_local_mem;                                   \
                                                                               \
    const uint items_per_work_item = is_joint ? (n - 1) / local_size + 1 : n;  \
    const uint max_n = is_joint ? n : items_per_work_item * local_size;        \
                                                                               \
    type* keys_input = (type*) first;                                          \
    type* temp = (type*) scratch;                                              \
                                                                               \
    if (is_joint) {                                                            \
        /* first sort within work-item */                                      \
        __builtin_bubble_sort(keys_input, idx * items_per_work_item,           \
            min((idx + 1) * items_per_work_item, max_n), is_asc);              \
        if (is_sort_local_mem)                                                 \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
    }                                                                          \
    else { /* private mem sort */                                              \
        /* first sort private array within work-item */                        \
        __builtin_bubble_sort(first, 0, items_per_work_item, is_asc);          \
                                                                               \
        /* copy items from private to temp memory */                           \
        for(uint i = 0; i < items_per_work_item; ++i)                          \
            temp[idx * items_per_work_item + i] = first[i];                    \
        keys_input = (type*) scratch;                                          \
        temp = &keys_input[local_size*items_per_work_item];                    \
        is_sort_local_mem = is_temp_local_mem;                                 \
        if (is_temp_local_mem)                                                 \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
    }                                                                          \
                                                                               \
    uint sorted_size = 1;                                                      \
    bool data_in_temp = false;                                                 \
    while(sorted_size * items_per_work_item < max_n)                           \
    {                                                                          \
        const uint start_1 = min(                                              \
            2 * sorted_size * items_per_work_item * (idx / sorted_size),       \
            max_n);                                                            \
        const uint end_1 = min(                                                \
            start_1 + sorted_size * items_per_work_item, max_n);               \
        const uint start_2 = end_1;                                            \
        const uint end_2 = min(                                                \
            start_2 + sorted_size * items_per_work_item, max_n);               \
        const uint offset = items_per_work_item * (idx % sorted_size);         \
                                                                               \
        if (!data_in_temp) {                                                   \
            __builtin_merge_step(                                              \
                offset, keys_input, temp, start_1, end_1, end_2, start_1,      \
                items_per_work_item, is_asc);                                  \
            if (is_temp_local_mem)                                             \
                work_group_barrier(CLK_LOCAL_MEM_FENCE);                       \
            else                                                               \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE);                      \
        }                                                                      \
        else {                                                                 \
            __builtin_merge_step(                                              \
                offset, temp, keys_input, start_1, end_1,                      \
                end_2, start_1, items_per_work_item, is_asc);                  \
            if (is_sort_local_mem)                                             \
                work_group_barrier(CLK_LOCAL_MEM_FENCE);                       \
            else                                                               \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE);                      \
        }                                                                      \
                                                                               \
        sorted_size *= 2;                                                      \
        data_in_temp = !data_in_temp;                                          \
    }                                                                          \
                                                                               \
    if (is_joint) {                                                            \
        /* copy back only if data is in a temporary storage */                 \
        if(data_in_temp) {                                                     \
            for(uint i = 0; i < items_per_work_item; ++i) {                    \
                if(idx * items_per_work_item + i < n) {                        \
                    first[idx * items_per_work_item + i] =                     \
                        temp[idx * items_per_work_item + i];                   \
                }                                                              \
            }                                                                  \
            if (is_sort_local_mem)                                             \
                work_group_barrier(CLK_LOCAL_MEM_FENCE);                       \
            else                                                               \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE);                      \
        }                                                                      \
    }                                                                          \
    else { /* private sort */                                                  \
        type* data = data_in_temp ? temp : keys_input;                         \
        for (uint i = 0; i < items_per_work_item; ++i) {                       \
            if (!is_spread) {                                                  \
                first[i] = data[items_per_work_item * idx + i];                \
            }                                                                  \
            else {                                                             \
                first[i] = data[local_size * i + idx];                         \
            }                                                                  \
        }                                                                      \
    }                                                                          \
}

/* Key-only work-group sort function body */
#define DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY_BODY(                            \
    type, is_joint, is_asc, is_spread, sort_local_mem, temp_local_mem)         \
{                                                                              \
    const uint local_size = get_local_size(0);                                 \
                                                                               \
    const uint items_per_work_item = is_joint ?                                \
        (n - 1) / local_size + 1 : n;                                          \
                                                                               \
    if (items_per_work_item > 8 &&                                             \
            items_per_work_item >= sizeof(type) * RADIX_SORT_CHAR_BIT) {       \
        DEFN_DEFAULT_WORK_GROUP_RADIX_SORT_KEY_ONLY_BODY(                      \
            type, is_joint, is_asc, is_spread, sort_local_mem, temp_local_mem) \
    } else {                                                                   \
        DEFN_DEFAULT_WORK_GROUP_MERGE_SORT_KEY_ONLY_BODY(                      \
            type, is_joint, is_asc, is_spread, sort_local_mem, temp_local_mem) \
    }                                                                          \
}


/* Key-value work-group sort interfaces */
#define DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(type, values_type,              \
    type_abbr, direction, is_asc,                                              \
    sort_local_mem, temp_local_mem)                                            \
/* joint sort */                                                               \
void __devicelib_default_work_group_joint_sort_##direction##_##type_abbr(      \
        type* first, values_type* values_first, uint n, char* scratch)         \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_BODY(type, values_type,             \
        true, is_asc, false, sort_local_mem, temp_local_mem)                   \
/* private sort close */                                                       \
void __devicelib_default_work_group_private_sort_close_##direction##_##type_abbr( \
        type* first, values_type* values_first, uint n, char* scratch)         \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_BODY(type, values_type,             \
        false, is_asc, false, sort_local_mem, temp_local_mem)                  \
/* private sort spread */                                                      \
void __devicelib_default_work_group_private_sort_spread_##direction##_##type_abbr( \
        type* first, values_type* values_first, uint n, char* scratch)         \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_BODY(type, values_type,             \
        false, is_asc, true, sort_local_mem, temp_local_mem)


/* Key-value work-group sort algorithm */
#define DEFN_DEFAULT_WORK_GROUP_RADIX_SORT_KEY_VALUE_BODY(                     \
    type, values_type, is_joint, is_asc, is_spread,                            \
    sort_local_mem, temp_local_mem)                                            \
{                                                                              \
    const uint local_size = get_local_size(0);                                 \
    const uint idx = get_local_id(0);                                          \
                                                                               \
    const bool is_comp_asc = is_asc;                                           \
    const bool is_joint_sort = is_joint;                                       \
                                                                               \
    const uint radix_states = 1U << RADIX_SORT_BITS_PER_PASS;                  \
                                                                               \
    const uint last_iter = sizeof(type) * RADIX_SORT_CHAR_BIT /                \
        RADIX_SORT_BITS_PER_PASS;                                              \
                                                                               \
    const uint items_per_work_item = is_joint_sort ?                           \
        (n - 1) / local_size + 1 : n;                                          \
    const uint keys_n = is_joint_sort ? n : local_size * n;                    \
                                                                               \
    type* keys_input = first;                                                  \
    values_type* values_input = values_first;                                  \
                                                                               \
    uint* scan_memory = (uint*) scratch;                                       \
    type* keys_output = (type*) ((char*) scratch +                             \
        radix_states * local_size * sizeof(uint));                             \
    uint values_offset = keys_n * sizeof(type);                                \
    values_offset = (((values_offset + sizeof(uint) - 1) /                     \
        sizeof(uint)) * sizeof(uint));                                         \
    values_type* values_output = (values_type*)                                \
        ((char*) keys_output + values_offset);                                 \
                                                                               \
    uint start = is_joint_sort ? items_per_work_item * idx : 0;                \
                                                                               \
    for (uint radix_iter = 0; radix_iter < last_iter; ++radix_iter) {          \
                                                                               \
        __builtin_radix_sort_count(is_comp_asc, temp_local_mem,                \
            items_per_work_item, radix_iter, n, start, keys_input,             \
            scan_memory);                                                      \
                                                                               \
        if (temp_local_mem)                                                    \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
                                                                               \
        __builtin_radix_sort_scan(radix_states, scan_memory);                  \
                                                                               \
        if (temp_local_mem)                                                    \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
                                                                               \
        __builtin_radix_sort_reorder(is_comp_asc, items_per_work_item,         \
            radix_iter, n, start, keys_input, keys_output,                     \
            values_input, values_output, scan_memory);                         \
                                                                               \
        if (sort_local_mem)                                                    \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
                                                                               \
        if (is_joint_sort) {                                                   \
            type* tmp = keys_output;                                           \
            keys_output = keys_input;                                          \
            keys_input = tmp;                                                  \
                                                                               \
            values_type* tmpv = values_output;                                 \
            values_output = values_input;                                      \
            values_input = tmpv;                                               \
        } else {  /* is private mem sort */                                    \
            for (uint i = 0; i < items_per_work_item; ++i) {                   \
                bool spread = is_spread && (radix_iter == last_iter - 1);      \
                if (!spread) {                                                 \
                    keys_input[i] = keys_output[items_per_work_item * idx + i]; \
                    values_input[i] = values_output[items_per_work_item * idx + i]; \
                } else {                                                       \
                    keys_input[i] = keys_output[local_size * i + idx];         \
                    values_input[i] = values_output[local_size * i + idx];     \
                }                                                              \
            }                                                                  \
        }                                                                      \
                                                                               \
    }                                                                          \
}

#define DEFN_DEFAULT_WORK_GROUP_MERGE_SORT_KEY_VALUE_BODY(                     \
    type, values_type, is_joint, is_asc, is_spread, sort_local_mem,            \
    temp_local_mem)                                                            \
{                                                                              \
    const uint local_size = get_local_size(0);                                 \
    const uint idx = get_local_id(0);                                          \
                                                                               \
    bool is_sort_local_mem = sort_local_mem;                                   \
    bool is_temp_local_mem = temp_local_mem;                                   \
                                                                               \
    const uint items_per_work_item = is_joint ? (n - 1) / local_size + 1 : n;  \
    const uint max_n = is_joint ? n : items_per_work_item * local_size;        \
                                                                               \
    type* keys_input = (type*) first;                                          \
    type* temp = (type*) scratch;                                              \
                                                                               \
    values_type* values_input = (values_type*) values_first;                   \
    uint values_offset = max_n * sizeof(type);                                 \
    values_offset = (((values_offset + sizeof(uint) - 1) /                     \
        sizeof(uint)) * sizeof(uint));                                         \
    values_type* values_temp = (values_type*)                                  \
        ((char*) temp + values_offset);                                        \
                                                                               \
    if (is_joint) {                                                            \
        /* first sort within work-item */                                      \
        __builtin_bubble_sort(keys_input, values_input,                        \
            idx * items_per_work_item,                                         \
            min((idx + 1) * items_per_work_item, max_n), is_asc);              \
        if (is_sort_local_mem)                                                 \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
    }                                                                          \
    else { /* private mem sort */                                              \
        /* first sort private array within work-item */                        \
        __builtin_bubble_sort(first, values_input, 0, items_per_work_item,     \
            is_asc);                                                           \
                                                                               \
        /* copy items from private to temp memory */                           \
        keys_input = (type*) scratch;                                          \
        values_input = (values_type*) ((char*) keys_input + values_offset);    \
        uint keys_offset = max_n * sizeof(values_type);                        \
        keys_offset = (((keys_offset + sizeof(uint) - 1) /                     \
            sizeof(uint)) * sizeof(uint));                                     \
        temp = (type*) ((char*) values_input + keys_offset);                   \
        values_temp = (values_type*) ((char*) temp + values_offset);           \
        for(uint i = 0; i < items_per_work_item; ++i) {                        \
            keys_input[idx * items_per_work_item + i] = first[i];              \
            values_input[idx * items_per_work_item + i] = values_first[i];     \
        }                                                                      \
        is_sort_local_mem = is_temp_local_mem;                                 \
        if (is_temp_local_mem)                                                 \
            work_group_barrier(CLK_LOCAL_MEM_FENCE);                           \
        else                                                                   \
            work_group_barrier(CLK_GLOBAL_MEM_FENCE);                          \
    }                                                                          \
                                                                               \
    uint sorted_size = 1;                                                      \
    bool data_in_temp = false;                                                 \
    while(sorted_size * items_per_work_item < max_n)                           \
    {                                                                          \
        const uint start_1 = min(                                              \
            2 * sorted_size * items_per_work_item * (idx / sorted_size),       \
            max_n);                                                            \
        const uint end_1 = min(                                                \
            start_1 + sorted_size * items_per_work_item, max_n);               \
        const uint start_2 = end_1;                                            \
        const uint end_2 = min(                                                \
            start_2 + sorted_size * items_per_work_item, max_n);               \
        const uint offset = items_per_work_item * (idx % sorted_size);         \
                                                                               \
        if (!data_in_temp) {                                                   \
            __builtin_merge_step(                                              \
                offset, keys_input, temp, values_input, values_temp, start_1,  \
                end_1, end_2, start_1, items_per_work_item, is_asc);           \
            if (is_temp_local_mem)                                             \
                work_group_barrier(CLK_LOCAL_MEM_FENCE);                       \
            else                                                               \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE);                      \
        }                                                                      \
        else {                                                                 \
            __builtin_merge_step(                                              \
                offset, temp, keys_input, values_temp, values_input, start_1,  \
                end_1, end_2, start_1, items_per_work_item, is_asc);           \
            if (is_sort_local_mem)                                             \
                work_group_barrier(CLK_LOCAL_MEM_FENCE);                       \
            else                                                               \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE);                      \
        }                                                                      \
                                                                               \
        sorted_size *= 2;                                                      \
        data_in_temp = !data_in_temp;                                          \
    }                                                                          \
                                                                               \
    if (is_joint) {                                                            \
        /* copy back only if data is in a temporary storage */                 \
        if(data_in_temp) {                                                     \
            for(uint i = 0; i < items_per_work_item; ++i) {                    \
                if(idx * items_per_work_item + i < n) {                        \
                    first[idx * items_per_work_item + i] =                     \
                        temp[idx * items_per_work_item + i];                   \
                    values_first[idx * items_per_work_item + i] =              \
                        values_temp[idx * items_per_work_item + i];            \
                }                                                              \
            }                                                                  \
            if (is_sort_local_mem)                                             \
                work_group_barrier(CLK_LOCAL_MEM_FENCE);                       \
            else                                                               \
                work_group_barrier(CLK_GLOBAL_MEM_FENCE);                      \
        }                                                                      \
    }                                                                          \
    else { /* private sort */                                                  \
        type* data = data_in_temp ? temp : keys_input;                         \
        values_type* values_data = data_in_temp ? values_temp : values_input;  \
                                                                               \
        for (uint i = 0; i < items_per_work_item; ++i) {                       \
            if (!is_spread) {                                                  \
                first[i] = data[items_per_work_item * idx + i];                \
                values_first[i] = values_data[items_per_work_item * idx + i];  \
            }                                                                  \
            else {                                                             \
                first[i] = data[local_size * i + idx];                         \
                values_first[i] = values_data[local_size * i + idx];           \
            }                                                                  \
        }                                                                      \
    }                                                                          \
}

#define DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_BODY(                           \
    type, values_type, is_joint, is_asc, is_spread,                            \
    sort_local_mem, temp_local_mem)                                            \
{                                                                              \
    const uint local_size = get_local_size(0);                                 \
                                                                               \
    const uint items_per_work_item = is_joint ?                                \
        (n - 1) / local_size + 1 : n;                                          \
                                                                               \
    if (items_per_work_item > 8 &&                                             \
            items_per_work_item >= sizeof(type) * RADIX_SORT_CHAR_BIT) {       \
        DEFN_DEFAULT_WORK_GROUP_RADIX_SORT_KEY_VALUE_BODY(                     \
            type, values_type, is_joint, is_asc, is_spread,                    \
            sort_local_mem, temp_local_mem)                                    \
    } else {                                                                   \
        DEFN_DEFAULT_WORK_GROUP_MERGE_SORT_KEY_VALUE_BODY(                     \
            type, values_type, is_joint, is_asc, is_spread,                    \
            sort_local_mem, temp_local_mem)                                    \
    }                                                                          \
}


/* instance all variants: asc-desc, global-local mem, global-local temp mem,
   all types for keys and values */
/* different mangling for signed and unsigned integers (i32, u32)
   as they are handled differently in the algorithm */
#define DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(keys_type, type_abbr,       \
        values_type, values_type_abbr)                                         \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p1##type_abbr##_p1##values_type_abbr##_u32_p1i8,                       \
        ascending, true, false, false);                                        \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p1##type_abbr##_p1##values_type_abbr##_u32_p1i8,                       \
        descending, false, false, false);                                      \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p3##type_abbr##_p3##values_type_abbr##_u32_p1i8,                       \
        ascending, true, true, false);                                         \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p3##type_abbr##_p3##values_type_abbr##_u32_p1i8,                       \
        descending, false, true, false);                                       \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p1##type_abbr##_p1##values_type_abbr##_u32_p3i8,                       \
        ascending, true, false, true);                                         \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p1##type_abbr##_p1##values_type_abbr##_u32_p3i8,                       \
        descending, false, false, true);                                       \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p3##type_abbr##_p3##values_type_abbr##_u32_p3i8,                       \
        ascending, true, true, true);                                          \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE(keys_type, values_type,             \
        p3##type_abbr##_p3##values_type_abbr##_u32_p3i8,                       \
        descending, false, true, true);


#define DEFN_DEFAULT_WORK_GROUP_SORT_ALL(type, type_abbr)                      \
    /* Key-only work-group sort */                                             \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p1##type_abbr##_u32_p1i8,                                              \
        ascending, true, false, false);                                        \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p1##type_abbr##_u32_p1i8,                                              \
        descending, false, false, false);                                      \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p3##type_abbr##_u32_p1i8,                                              \
        ascending, true, true, false);                                         \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p3##type_abbr##_u32_p1i8,                                              \
        descending, false, true, false);                                       \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p1##type_abbr##_u32_p3i8,                                              \
        ascending, true, false, true);                                         \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p1##type_abbr##_u32_p3i8,                                              \
        descending, false, false, true);                                       \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p3##type_abbr##_u32_p3i8,                                              \
        ascending, true, true, true);                                          \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_ONLY(type,                                \
        p3##type_abbr##_u32_p3i8,                                              \
        descending, false, true, true);                                        \
                                                                               \
    /* Key-value work-group sort */                                            \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, uchar, u8)     \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, ushort, u16)   \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, uint, u32)     \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, ulong, u64)    \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, char, i8)      \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, short, i16)    \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, int, i32)      \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, long, i64)     \
                                                                               \
    DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(type, type_abbr, float, f32)    \
                                                                               \
    /* Key-only sub-group sort */                                              \
    DEFN_DEFAULT_SUB_GROUP_SORT_KEY_ONLY(type, type_abbr,                      \
        ascending, true)                                                       \
    DEFN_DEFAULT_SUB_GROUP_SORT_KEY_ONLY(type, type_abbr,                      \
        descending, false)


/* All types without half and double */
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(uchar, u8)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(ushort, u16)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(uint, u32)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(ulong, u64)

DEFN_DEFAULT_WORK_GROUP_SORT_ALL(char, i8)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(short, i16)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(int, i32)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(long, i64)

#if defined(cl_khr_fp16)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(half, f16)
#endif
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(float, f32)
#if defined(cl_khr_fp64)
DEFN_DEFAULT_WORK_GROUP_SORT_ALL(double, f64)
#endif

/* half and double separately */
#if defined(cl_khr_fp16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(uchar, u8, half, f16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(ushort, u16, half, f16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(uint, u32, half, f16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(ulong, u64, half, f16)

DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(char, i8, half, f16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(short, i16, half, f16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(int, i32, half, f16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(long, i64, half, f16)

DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(half, f16, half, f16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(float, f32, half, f16)
#if defined(cl_khr_fp64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(double, f64, half, f16)
#endif  //cl_khr_fp64
#endif  //cl_khr_fp16

#if defined(cl_khr_fp64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(uchar, u8, double, f64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(ushort, u16, double, f64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(uint, u32, double, f64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(ulong, u64, double, f64)

DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(char, i8, double, f64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(short, i16, double, f64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(int, i32, double, f64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(long, i64, double, f64)

#if defined(cl_khr_fp16)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(half, f16, double, f64)
#endif  //cl_khr_fp16
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(float, f32, double, f64)
DEFN_DEFAULT_WORK_GROUP_SORT_KEY_VALUE_ALL(double, f64, double, f64)
#endif  //cl_khr_fp64
