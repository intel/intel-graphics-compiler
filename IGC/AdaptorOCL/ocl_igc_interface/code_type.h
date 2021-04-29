/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"

namespace IGC {

namespace CodeType {
using CodeType_t = uint64_t;
using CodeTypeCoder = CIF::Coder<CodeType_t>;
constexpr auto llvmLl = CodeTypeCoder::Enc("LLVM_LL");
constexpr auto llvmBc = CodeTypeCoder::Enc("LLVM_BC");
constexpr auto spirV = CodeTypeCoder::Enc("SPIRV");
constexpr auto oclC = CodeTypeCoder::Enc("OCL_C");
constexpr auto oclCpp = CodeTypeCoder::Enc("OCL_CPP");
constexpr auto oclGenBin = CodeTypeCoder::Enc("OCL_GEN_BIN");
constexpr auto elf = CodeTypeCoder::Enc("ELF");
constexpr auto undefined = CodeTypeCoder::Enc("UNDEFINED");
constexpr auto invalid = CodeTypeCoder::Enc("INVALID");
}

}
