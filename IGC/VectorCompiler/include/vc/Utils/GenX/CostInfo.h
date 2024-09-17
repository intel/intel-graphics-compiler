/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_COSTINFO_H
#define VC_UTILS_GENX_COSTINFO_H

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Module.h>

namespace vc {
// This class contains info about kernel argument.
struct ArgSym {
  // Argument number.
  int Num = 0;
  // Argument size.
  int Size = 0;
  // Offset of the memory location for indirect
  // arguments.
  int Offset = 0;
  // Indication that symbol refers a memory location
  // and not an argument itself.
  bool IsIndirect = false;

  bool operator!=(const ArgSym &RHS) const {
    return Num != RHS.Num || Size != RHS.Size || Offset != RHS.Offset ||
           IsIndirect != RHS.IsIndirect;
  }
};

// This class represents a loop count expression (LCE) in a form
// of 'Factor * Symbol + Addend' for loop bounds and loop trip
// count when these values can be represented in this form.
struct LoopCountExpr {
  float Factor = 0.0f;
  float Addend = 0.0f;
  ArgSym Symbol;
  bool IsUndef = true;
};

bool saveLCEToMetadata(const llvm::Loop &L, llvm::Module &M, LoopCountExpr LCE);
LoopCountExpr restoreLCEFromMetadata(const llvm::Loop &L);
} // namespace vc

#endif // VC_UTILS_GENX_COSTINFO_H
