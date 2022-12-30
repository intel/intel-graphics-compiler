/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BINARYENCODINGIGA_H_
#define _BINARYENCODINGIGA_H_

#include "FlowGraph.h"
#include "G4_IR.hpp"
#include "iga/IGALibrary/IR/Types.hpp"

#include <string>

namespace vISA {
// NOTE: IGA internals should be minimally leaked into vISA/G4

// Encodes a G4_Kernel via the IGA assembler
struct EncodeResult {
  void *binary;
  size_t binaryLen;
  std::string error; // in case of error
};

EncodeResult EncodeKernelIGA(G4_Kernel &k, const std::string &fname);

bool InstSupportsSaturationIGA(TARGET_PLATFORM p, const G4_INST &i,
                               const IR_Builder &builder);
bool InstSupportsSrcModifierIGA(TARGET_PLATFORM p, const G4_INST &i,
                                const IR_Builder &builder);

///////////////////////////////////////////////////////////////////////////
// TODO: remove these in step 2
// const iga::Model *GetModelIGA(TARGET_PLATFORM p);
//
// std::pair<const iga::OpSpec*,iga::Subfunction> GetOpInfoIGA(
//    const G4_INST *inst, iga::Platform p, bool allowUnknownOp);
} // namespace vISA

#endif //_BINARYENCODINGIGA_H_
