/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _MEM_OPT_UTILS_H_
#define _MEM_OPT_UTILS_H_

#include <optional>

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include <llvm/IR/Instruction.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/Analysis/MemoryLocation.h>

#include "common/IGCIRBuilder.h"
#include "Compiler/CISACodeGen/SLMConstProp.hpp"

namespace IGC {
// ALoadInst and AStoreInst abstract away the differences
// between Load and PredicatedLoad and between Store and PredicatedStore.
// TODO: consider refactoring to unify with AbstractLoadInst and AbstractStoreInst
//       from VectorPreProcess.cpp
class ALoadInst {
  llvm::Instruction *const m_inst;
  ALoadInst(llvm::Instruction *LI) : m_inst(LI) {}

public:
  static std::optional<ALoadInst> get(llvm::Value *value);

  llvm::Instruction *inst() const { return m_inst; }
  llvm::Value *getValue() const { return m_inst; }
  llvm::Type *getType() const;
  llvm::Value *getPointerOperand() const;
  llvm::Type *getPointerOperandType() const;
  unsigned getPointerAddressSpace() const;
  bool isVolatile() const;
  void setVolatile(bool isVolatile);
  bool isSimple() const;
  alignment_t getAlignmentValue() const;
  bool isPredicated() const;
  llvm::Value *getPredicate() const;
  llvm::PredicatedLoadIntrinsic *getPredicatedLoadIntrinsic() const;

  template <typename T>
  llvm::Instruction *CreateLoad(llvm::IGCIRBuilder<T> &IRB, llvm::Type *Ty, llvm::Value *Ptr, llvm::Value *MergeValue);
  template <typename T>
  llvm::Instruction *CreateAlignedLoad(llvm::IGCIRBuilder<T> &IRB, llvm::Type *Ty, llvm::Value *Ptr,
                                       llvm::Value *MergeValue, bool isVolatile = false);
};

class AStoreInst {
  llvm::Instruction *const m_inst;
  AStoreInst(llvm::Instruction *SI) : m_inst(SI) {}

public:
  llvm::Instruction *inst() const { return m_inst; }
  llvm::Value *getPointerOperand() const;
  llvm::Type *getPointerOperandType() const;
  unsigned getPointerAddressSpace() const;
  llvm::Value *getValueOperand() const;
  llvm::Value *getValue() const { return getValueOperand(); };
  bool isVolatile() const;
  void setVolatile(bool isVolatile);
  bool isSimple() const;
  alignment_t getAlignmentValue() const;
  bool isPredicated() const;
  llvm::Value *getPredicate() const;
  static std::optional<AStoreInst> get(llvm::Value *value);
  template <typename T>
  llvm::Instruction *CreateAlignedStore(llvm::IGCIRBuilder<T> &IRB, llvm::Value *Val, llvm::Value *Ptr,
                                        bool isVolatile = false);
};

// other utility functions, that should take into account abstract interface

llvm::MemoryLocation getLocation(llvm::Instruction *I, llvm::TargetLibraryInfo *TLI);

// Symbolic difference of two address values
// return value:
//   true  if A1 - A0 = constant in bytes, and return that constant as BO.
//   false if A1 - A0 != constant. BO will be undefined.
// BO: byte offset
bool getDiffIfConstant(llvm::Value *A0, llvm::Value *A1, int64_t &ConstBO, SymbolicEvaluation &symEval);

// If I0 and I1 are load/store insts, compare their address operands and return
// the constant difference if it is; return false otherwise.
bool getAddressDiffIfConstant(llvm::Instruction *I0, llvm::Instruction *I1, int64_t &BO, SymbolicEvaluation &symEval);
} // namespace IGC

#endif // _MEM_OPT_UTILS_H_
