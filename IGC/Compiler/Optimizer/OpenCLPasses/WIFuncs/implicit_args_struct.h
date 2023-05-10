#ifndef IMPLICIT_ARGS_STRUCT_H_
#define IMPLICIT_ARGS_STRUCT_H_

struct implicit_args {
    uint8_t struct_size;
    uint8_t struct_version;
    uint8_t num_work_dim;
    uint8_t simd_width;
    uint32_t local_size_x;
    uint32_t local_size_y;
    uint32_t local_size_z;
    uint64_t global_size_x;
    uint64_t global_size_y;
    uint64_t global_size_z;
    uint64_t printf_buffer_ptr;
    uint64_t global_offset_x;
    uint64_t global_offset_y;
    uint64_t global_offset_z;
    uint64_t local_id_table_ptr;
    uint32_t group_count_x;
    uint32_t group_count_y;
    uint32_t group_count_z;
    uint32_t padding0;
    uint64_t rt_global_buffer_ptr;
    uint64_t assert_buffer_ptr;
};

#endif // IMPLICIT_ARGS_STRUCT_H_
