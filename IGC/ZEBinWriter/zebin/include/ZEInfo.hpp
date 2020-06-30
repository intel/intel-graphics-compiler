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
//===- ZEInfo.hpp -----------------------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// \file
// This file declares the struct representation of .ze.info section
//===----------------------------------------------------------------------===//

#ifndef ZE_INFO_HPP
#define ZE_INFO_HPP

#include <string>
#include <vector>

namespace zebin {

/// ze_info Types
typedef std::string zeinfo_str_t;
typedef int32_t     zeinfo_int32_t;
typedef bool        zeinfo_bool_t;

/// execution_env
struct zeInfoExecutionEnvironment
{
    zeinfo_int32_t actual_kernel_start_offset = 0;
    zeinfo_int32_t barrier_count = 0;
    zeinfo_bool_t  disable_mid_thread_preemption = false;
    zeinfo_int32_t grf_count = 0;
    zeinfo_bool_t has_4gb_buffers = false;
    zeinfo_bool_t has_device_enqueue = false;
    zeinfo_bool_t has_fence_for_image_access = false;
    zeinfo_bool_t has_global_atomics = false;
    zeinfo_bool_t has_multi_scratch_spaces = false;
    zeinfo_bool_t has_no_stateless_write = false;
    zeinfo_int32_t hw_preemption_mode = -1;
    zeinfo_int32_t offset_to_skip_per_thread_data_load = 0;
    zeinfo_int32_t offset_to_skip_set_ffid_gp = 0;
    zeinfo_int32_t required_sub_group_size = 0;
    std::vector<zeinfo_int32_t> required_work_group_size;
    zeinfo_int32_t simd_size = 0;
    zeinfo_int32_t slm_size = 0;
    zeinfo_bool_t subgroup_independent_forward_progress = false;
    std::vector<zeinfo_int32_t> work_group_walk_order_dimensions;
};

/// payload_argument
struct zeInfoPayloadArgument
{
    zeinfo_str_t   arg_type;
    zeinfo_int32_t offset = 0;
    zeinfo_int32_t size = 0;
    zeinfo_int32_t arg_index = -1;
    zeinfo_str_t   addrmode;
    zeinfo_str_t   addrspace;
    zeinfo_str_t   access_type;
};

/// per_thread_payload_argument
struct zeInfoPerThreadPayloadArgument
{
    zeinfo_str_t   arg_type;
    zeinfo_int32_t offset = 0;
    zeinfo_int32_t size = 0;
};

/// binding_table_index
struct zeInfoBindingTableIndex
{
    zeinfo_int32_t bti_value = 0;
    zeinfo_int32_t arg_index = 0;
};

/// per_thread_memory_buffers
struct zePerThreadMemoryBuffer
{
    zeinfo_str_t type;
    zeinfo_str_t usage;
    zeinfo_int32_t size = 0;
};

/// kernel
typedef std::vector<zeInfoPayloadArgument> PayloadArgumentsTy;
typedef std::vector<zeInfoPerThreadPayloadArgument> PerThreadPayloadArgumentsTy;
typedef std::vector<zeInfoBindingTableIndex> BindingTableIndexesTy;
typedef std::vector<zePerThreadMemoryBuffer> PerThreadMemoryBuffersTy;

struct zeInfoKernel
{
    zeinfo_str_t name;
    zeInfoExecutionEnvironment execution_env;
    PayloadArgumentsTy payload_arguments;
    PerThreadPayloadArgumentsTy per_thread_payload_arguments;
    BindingTableIndexesTy binding_table_indexes;
    PerThreadMemoryBuffersTy per_thread_memory_buffers;
};

/// kernel container
struct zeInfoContainer
{
    std::vector<zeInfoKernel> kernels;
};

/// Pre-defined attribute values getter
// Some attributes with string type are pre-defined. PreDefinedAttr is a
// helper class to manage the predefined strings.
// FIXME: Should be part of ZEInfoBuilder or somewhere else?
struct PreDefinedAttrGetter {
    // zeInfoPayloadArguments::arg_type and zeInfoPerThreadPayloadArguments::arg_type
    enum class ArgType {
        packed_local_ids,
        local_id,
        local_size,
        group_size,
        global_id_offset,
        private_base_stateless,
        arg_byvalue,
        arg_bypointer
    };
    // zeInfoPayloadArguments::addrmode
    enum class ArgAddrMode {
        stateless,
        stateful,
        bindless,
        shared_local_memory
    };
    // zeInfoPayloadArguments::addrspace
    enum class ArgAddrSpace {
        global,
        local,
        constant,
        image,
        sampler
    };
    // zeInfoPayloadArguments::access_type
    enum class ArgAccessType {
        readonly,
        writeonly,
        readwrite
    };
    // zePerThreadMemoryBuffers::type
    enum class MemBufferType {
        global,
        scratch,
        slm
    };
    // zePerThreadMemoryBuffers::usage
    enum class MemBufferUsage {
        private_space,
        spill_fill_space,
        single_space
    };

    static zeinfo_str_t get(ArgType val) {
        switch(val) {
        case ArgType::packed_local_ids:
            return "packed_local_ids";
        case ArgType::local_id:
            return "local_id";
        case ArgType::local_size:
            return "local_size";
        case ArgType::group_size:
            return "group_size";
        case ArgType::global_id_offset:
            return "global_id_offset";
        case ArgType::private_base_stateless:
            return "private_base_stateless";
        case ArgType::arg_byvalue:
            return "arg_byvalue";
        case ArgType::arg_bypointer:
            return "arg_bypointer";
        default:
            break;
        }
        return "";
    }

    static zeinfo_str_t get(ArgAddrMode val) {
        switch (val) {
        case ArgAddrMode::stateless:
            return "stateless";
        case ArgAddrMode::stateful:
            return "stateful";
        case ArgAddrMode::bindless:
            return "bindless";
        case ArgAddrMode::shared_local_memory:
            return "shared_local_memory";
        default:
            break;
        }
        return "";
    }

    static zeinfo_str_t get(ArgAddrSpace val) {
        switch(val) {
        case ArgAddrSpace::global:
            return "global";
        case ArgAddrSpace::local:
            return "local";
        case ArgAddrSpace::constant:
            return "constant";
        case ArgAddrSpace::image:
            return "image";
        case ArgAddrSpace::sampler:
            return "sampler";
        default:
            break;
        }
        return "";
    }

    static zeinfo_str_t get(ArgAccessType val) {
        switch(val) {
        case ArgAccessType::readonly:
            return "readonly";
        case ArgAccessType::writeonly:
            return "writeonly";
        case ArgAccessType::readwrite:
            return "readwrite";
        default:
            break;
        }
        return "";
    }

    static zeinfo_str_t get(MemBufferType val) {
        switch(val) {
        case MemBufferType::global:
            return "global";
        case MemBufferType::scratch:
            return "scratch";
        case MemBufferType::slm:
            return "slm";
        default:
            break;
        }
        return "";
    }

    static zeinfo_str_t get(MemBufferUsage val) {
        switch(val) {
        case MemBufferUsage::private_space:
            return "private_space";
        case MemBufferUsage::spill_fill_space:
            return "spill_fill_space";
        case MemBufferUsage::single_space:
            return "single_space";
        default:
            break;
        }
        return "";
    }
};

} // end namespace zebin
#endif //ZEINFO_HPP
