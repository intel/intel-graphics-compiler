/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#include <ZEInfoYAML.hpp>

using namespace zebin;
using namespace llvm::yaml;

void MappingTraits<zeInfoExecutionEnvironment>::mapping(IO& io, zeInfoExecutionEnvironment& info)
{
    io.mapRequired("actual_kernel_start_offset", info.actual_kernel_start_offset);
    io.mapOptional("barrier_count", info.barrier_count, 0);
    io.mapOptional("disable_mid_thread_preemption", info.disable_mid_thread_preemption, false);
    io.mapRequired("grf_count", info.grf_count);
    io.mapOptional("has_4gb_buffers", info.has_4gb_buffers, false);
    io.mapOptional("has_device_enqueue", info.has_device_enqueue, false);
    io.mapOptional("has_fence_for_image_access", info.has_fence_for_image_access, false);
    io.mapOptional("has_global_atomics", info.has_global_atomics, false);
    io.mapOptional("has_multi_scratch_spaces", info.has_multi_scratch_spaces, false);
    io.mapOptional("has_no_stateless_write", info.has_no_stateless_write, false);
    io.mapOptional("offset_to_skip_per_thread_data_load", info.offset_to_skip_per_thread_data_load, 0);
    io.mapOptional("offset_to_skip_set_ffid_gp", info.offset_to_skip_set_ffid_gp, 0);
    io.mapOptional("required_sub_group_size", info.required_sub_group_size, 0);
    io.mapOptional("required_work_group_size", info.required_work_group_size);
    io.mapRequired("simd_size", info.simd_size);
    io.mapOptional("slm_size", info.slm_size, 0);
    io.mapOptional("subgroup_independent_forward_progress", info.subgroup_independent_forward_progress, false);
    io.mapOptional("work_group_walk_order_dimensions", info.work_group_walk_order_dimensions);
}

void MappingTraits<zebin::zeInfoPayloadArgument>::mapping(IO& io, zebin::zeInfoPayloadArgument& info)
{
    io.mapRequired("arg_type", info.arg_type);
    io.mapRequired("offset", info.offset);
    io.mapRequired("size", info.size);
    io.mapOptional("arg_index", info.arg_index, -1);
    io.mapOptional("addrmode", info.addrmode, std::string());
    io.mapOptional("addrspace", info.addrspace, std::string());
    io.mapOptional("access_type", info.access_type, std::string());
}

void MappingTraits<zebin::zeInfoPerThreadPayloadArgument>::mapping(IO& io, zebin::zeInfoPerThreadPayloadArgument& info)
{
    io.mapRequired("arg_type", info.arg_type);
    io.mapRequired("offset", info.offset);
    io.mapRequired("size", info.size);
}

void MappingTraits<zebin::zeInfoBindingTableIndex>::mapping(IO& io, zebin::zeInfoBindingTableIndex& info)
{
    io.mapRequired("bti_value", info.bti_value);
    io.mapRequired("arg_index", info.arg_index);
}

void MappingTraits<zebin::zePerThreadMemoryBuffer>::mapping(IO& io, zebin::zePerThreadMemoryBuffer& info)
{
    io.mapRequired("type", info.type);
    io.mapRequired("usage", info.usage);
    io.mapRequired("size", info.size);
}

void MappingTraits<zeInfoKernel>::mapping(IO& io, zeInfoKernel& info)
{
    io.mapRequired("name", info.name);
    io.mapRequired("execution_env", info.execution_env);
    io.mapOptional("payload_arguments", info.payload_arguments);
    io.mapOptional("per_thread_payload_arguments", info.per_thread_payload_arguments);
    io.mapOptional("binding_table_indexes", info.binding_table_indexes);
    io.mapOptional("per_thread_memory_buffers", info.per_thread_memory_buffers);
}

void MappingTraits<zebin::zeInfoContainer>::mapping(IO& io, zebin::zeInfoContainer& info)
{
    io.mapRequired("kernels", info.kernels);
}