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
//===- ZEInfoYAML.hpp -------------------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// \file
// This file declares the mapping between zeInfo structs and YAML
//===----------------------------------------------------------------------===//

#ifndef ZE_INFO_YAML_HPP
#define ZE_INFO_YAML_HPP

#include <ZEInfo.hpp>

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/YAMLTraits.h"
#include "common/LLVMWarningsPop.hpp"

LLVM_YAML_IS_SEQUENCE_VECTOR(zebin::zeInfoPayloadArgument)
LLVM_YAML_IS_SEQUENCE_VECTOR(zebin::zeInfoPerThreadPayloadArgument)
LLVM_YAML_IS_SEQUENCE_VECTOR(zebin::zeInfoBindingTableIndex)
LLVM_YAML_IS_SEQUENCE_VECTOR(zebin::zePerThreadMemoryBuffer)
LLVM_YAML_IS_SEQUENCE_VECTOR(zebin::zeInfoKernel)

namespace llvm {
    namespace yaml{

        template <>
        struct MappingTraits<zebin::zeInfoExecutionEnvironment> {
            static void mapping(IO& io, zebin::zeInfoExecutionEnvironment& info);
        };

        template <>
        struct MappingTraits<zebin::zeInfoPayloadArgument> {
            static void mapping(IO& io, zebin::zeInfoPayloadArgument& info);
        };

        template <>
        struct MappingTraits<zebin::zeInfoPerThreadPayloadArgument> {
            static void mapping(IO& io, zebin::zeInfoPerThreadPayloadArgument& info);
        };

        template <>
        struct MappingTraits<zebin::zeInfoBindingTableIndex> {
            static void mapping(IO& io, zebin::zeInfoBindingTableIndex& info);
        };

        template <>
        struct MappingTraits<zebin::zePerThreadMemoryBuffer> {
            static void mapping(IO& io, zebin::zePerThreadMemoryBuffer& info);
        };

        template <>
        struct MappingTraits<zebin::zeInfoKernel> {
            static void mapping(IO& io, zebin::zeInfoKernel& info);
        };

        template <>
        struct MappingTraits<zebin::zeInfoContainer> {
            static void mapping(IO& io, zebin::zeInfoContainer& info);
        };

    } // end namespace yaml
} // end namespace llvm

#endif // ZE_INFO_YAML_HPP
