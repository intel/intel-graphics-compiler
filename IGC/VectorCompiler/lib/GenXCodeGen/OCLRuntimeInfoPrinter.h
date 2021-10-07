/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_LIB_GENXCODEGEN_OCLRUNTIMEINFOPRINTER_H
#define VC_LIB_GENXCODEGEN_OCLRUNTIMEINFOPRINTER_H

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"

#include <llvm/Support/raw_ostream.h>

namespace vc {

// Serializes \p CompiledModule into YAML and prints it to \p OS.
void printOCLRuntimeInfo(
    llvm::raw_ostream &OS,
    const llvm::GenXOCLRuntimeInfo::CompiledModuleT &CompiledModule);

} // namespace vc

#endif // VC_LIB_GENXCODEGEN_OCLRUNTIMEINFOPRINTER_H
