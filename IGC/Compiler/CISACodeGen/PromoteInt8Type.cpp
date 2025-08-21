/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PromoteInt8Type.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvm/Transforms/Utils/Local.h"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Debug.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include <list>
#include "llvm/IR/Constants.h"
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace IGC;

//
// PromoteInt8Type pass
//
//   The pass is to promote i8 type into i16 as i8 operand usage and regioning
//   on arithmetic operations are very restricted for some hardware platforms.
//   Note that load/store instructions and mov instructions still support i8.
//   This pass will use mov instruction to promote i8 to i16 in ALU instructions.
//
//   Convert following if they involves i8 type:
//     1. Any ALU operations with i8, such as add/mul/cmp
//     2. PHI, select, call, shl, ashr, lshr
//     3. trunc/zext/sext/bitcast
//     4. sitofp/uitofp/fptosi/fptoui
//   Assume that no integer types (except boolean i1) have the size
//   that is smaller than i8, that is, no i2/i3/i4/i5/i6/i7!
//
namespace {
// ValueInfo:  info for any argument/instructions that define or use i8 values.
//    Val:  original value of i8 or an instruction that has i8 operands.
//    NewVal: the new i16 value of original Val. It's created where Val is defined.
//    NeedPromote :
//      If true, NewVal is created where Val is defined and used to replace Val's uses.
//        For any ALU instruction, NeedPromote is set to true. For non-ALU insts, it's
//        set to true if NPromotedUses > (Val's numOfUses/2).
//      otherwise, NewVal is null and if any of its uses needs i16, an i16 value is
//            created where its use is.
//    NPromotedUses : the number of Val's users that need I16 values. If it is more than
//      the half of the total number of Val's uses, NeedPromote will be set to true.
//    IsValDead:  if true, Val is dead and can be deleted.
struct ValueInfo {
  Value *Val;               // Value of I8 type or insts that use I8 values.
  Value *NewVal = nullptr;  // Val's new I16 value, defined at Val's def site.
  bool NeedPromote = false; // promoted value defined at Val's def site.
  int NPromotedUses = 0;    // the number of uses that need to be promoted
  bool IsValDead = false;   // Val can be deleted if IsValDead is true.
  ValueInfo(Value *V) : Val(V) {}
};

class PromoteInt8Type : public FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid
  PromoteInt8Type() : FunctionPass(ID) { initializePromoteInt8TypePass(*PassRegistry::getPassRegistry()); }

  StringRef getPassName() const override { return "PromoteInt8Type"; }
  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<DominatorTreeWrapperPass>();
  }
  virtual void releaseMemory() override {
    Allocator.Reset();
    m_valInfoMap.clear();
    m_valInsertOrder.clear();
  }

  /// print - print partitions in human readable form
  virtual void print(raw_ostream &OS, const Module * = 0) const override;
  /// dump - Dump the partitions to dbgs().
  void dump() const;

private:
  const DataLayout *m_DL = nullptr;
  DominatorTree *m_DT = nullptr;
  CodeGenContext *m_Ctx;
  Function *m_F;
  IRBuilder<> *m_builder;
  llvm::BumpPtrAllocator Allocator;

  // If any promotion happens, set to true
  bool m_changed;

  // ValueInfo for i8 value or insts with i8 operands
  DenseMap<Value *, ValueInfo *> m_valInfoMap;
  // Because Dense map is unordered we need something to store insert order
  std::vector<Value *> m_valInsertOrder;

  // Return true if V is i8 or V is vector of i8 element type.
  bool isI8Type(Value *V) const {
    Type *Ty = V->getType();
    VectorType *VTy = dyn_cast<VectorType>(Ty);
    Type *ETy = VTy ? VTy->getElementType() : Ty;
    return ETy->isIntegerTy(8);
  }

  // pre-handling of intrinsic promotion
  void promoteIntrinsic();

  void collectCandidates();
  void promoteInstructions();
  void deleteDeadInst();

  bool isUnsigned(GenIntrinsicInst *GII) const;
  ValueInfo *addValInfo(Value *V);
  ValueInfo *addValInfoIfI8(Value *V);
  ValueInfo *getValInfo(Value *V) const;

  // Given an i8 value, return i16 value converted from i8.
  Value *getI16Value(Value *V, bool IsZExt);
  // Given an i16 value, return i8 value trunced from i16
  Value *getI8Value(Value *V);
  // Given an i8 value, return either signed or unsigned i16 value
  Value *getSI16Value(Value *V) { return getI16Value(V, false); }
  Value *getUI16Value(Value *V) { return getI16Value(V, true); }

  // For vector value, mainly for extElt/InsElt
  Value *createI16Value(Value *V, bool IsZExt);
  Value *createSI16Value(Value *V) { return createI16Value(V, false); }
  Value *createUI16Value(Value *V) { return createI16Value(V, true); }
};

ValueInfo *PromoteInt8Type::getValInfo(Value *V) const {
  if (!V || isa<Constant>(V)) {
    return nullptr;
  }
  auto VI = m_valInfoMap.find(V);
  if (VI != m_valInfoMap.end()) {
    return VI->second;
  }
  return nullptr;
}

// Create an entry in m_valInfoMap for V
ValueInfo *PromoteInt8Type::addValInfo(Value *V) {
  auto VI = m_valInfoMap.find(V);
  if (VI == m_valInfoMap.end()) {
    ValueInfo *valinfo = new (Allocator) ValueInfo(V);
    m_valInfoMap.insert(std::make_pair(V, valinfo));
    m_valInsertOrder.push_back(V);
  }
  return m_valInfoMap[V];
}

// If V is of i8 type, create an entry in m_valInfoMap (via
// addValInfo()) if it does not exist, and return that entry.
// If V is not i8 type, do not create an entry and just return
// nullptr.
ValueInfo *PromoteInt8Type::addValInfoIfI8(Value *V) {
  ValueInfo *valinfo = nullptr;
  if (isI8Type(V)) {
    valinfo = addValInfo(V);
  }
  return valinfo;
}

} // namespace

// Register pass to igc-opt
#define PASS_FLAG "igc-promoteint8type"
#define PASS_DESCRIPTION "Promote i8 to i16 type"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteInt8Type, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PromoteInt8Type, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PromoteInt8Type::ID = 0;

FunctionPass *IGC::createPromoteInt8TypePass() { return new PromoteInt8Type(); }

bool PromoteInt8Type::runOnFunction(Function &F) {
  m_changed = false;
  m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_F = &F;
  m_DL = &m_F->getParent()->getDataLayout();
  m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  IRBuilder<> builder(m_F->getContext());
  m_builder = &builder;

  // 0. Preprocess
  //    Convert any intrinsic that might involve I8 ALU or illegal I8 mov
  //    (such as VxH mov for I8 type) to its variant that does not involve
  //    illegal I8 operations.
  promoteIntrinsic();

  // 1. collect info about i8 usage (both def and use)
  collectCandidates();

  // 2. Promote i8 type
  promoteInstructions();

  // 3. Delete dead instructions
  deleteDeadInst();

  return m_changed;
}

// Return true if Inst is an ALU that needs to be promoted to I16
bool needPromoteToI16(Instruction *Inst) {
  if (Inst && Inst->getType()->isIntegerTy(8)) {
    switch (Inst->getOpcode()) {
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
    case Instruction::Select:
    case Instruction::PHI:
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr: {
      return true;
    }
    case Instruction::ExtractElement: {
      // Promote to avoid Vx1/VxH if index is not constant
      if (!isa<Constant>(Inst->getOperand(1))) {
        return true;
      }
      break;
    }
    case Instruction::InsertElement: {
      // Promote to avoid Vx1/VxH if index is not constant
      if (!isa<Constant>(Inst->getOperand(2))) {
        return true;
      }
      break;
    }
    default:
      break;
    }
  }
  return false;
}

void PromoteInt8Type::collectCandidates() {
  // For special handling of insts such as InsElt and ExtElts
  //    The vector value promotion could be expensive, thus we try
  //    to reduce the number of instructions either by not promoting
  //    it or promoting all vector chain (insElt).
  std::list<ValueInfo *> worklist;

  // 1. All arguments of type i8
  for (auto AI = m_F->arg_begin(), AE = m_F->arg_end(); AI != AE; ++AI) {
    // Only create entry for an arg of i8 type.
    Value *V = AI;
    (void)addValInfoIfI8(V);
  }

  // 2. All instructions, either def i8 or use i8
  for (auto II = inst_begin(m_F), IE = inst_end(m_F); II != IE; ++II) {
    Instruction *I = &*II;
    ValueInfo *valinfo = addValInfoIfI8(I);

    // needPromote
    //     to indicate whether the result (and its operands) needs to be promoted
    //     to i16.  It is intended for i8 ALU instructions such as:
    //             %d = add i8 %s0,  %s1
    // If it is set to true, all I8 operands of this instructions need promotion.
    bool needPromote = false;
    if (valinfo) {
      needPromote = needPromoteToI16(I);
      valinfo->NeedPromote = needPromote;

      // Instructions for special handling later
      if (isa<InsertElementInst>(I) || isa<ExtractElementInst>(I)) {
        worklist.push_back(valinfo);
      }
    }
    for (unsigned i = 0, sz = I->getNumOperands(); i < sz; ++i) {
      Value *V = I->getOperand(i);
      if (isa<Constant>(V))
        continue;
      ValueInfo *vinfo = addValInfoIfI8(V);
      if (vinfo == nullptr)
        continue;
      if (needPromote) {
        ++(vinfo->NPromotedUses);
      }
      if (valinfo == nullptr) {
        // I isn't of i8; As it uses i8 value, create one
        valinfo = addValInfo(I);
      }
    }
  }

  // 3. Special handling of IntElt and ExtElt
  //    1. If either result or vector operand of InsElt needs to be promoted, all vectors
  //       in their insertion-chains need to be promoted.
  //    2. If the vector operand of ExtElt is promoted, the inst shall be promoted.
  bool hasChange = true;
  while (hasChange) {
    hasChange = false;
    auto LE = worklist.end();
    auto Next = worklist.begin();
    for (auto LI = Next; LI != LE; LI = Next) {
      ++Next;
      ValueInfo *valinfo = *LI;
      Value *V = valinfo->Val;
      Instruction *I = dyn_cast<Instruction>(V);
      IGC_ASSERT_MESSAGE(nullptr != I, "worklist entry must be an intruction!");
      if (ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(I)) {
        Value *VOprd = EEI->getVectorOperand();
        ValueInfo *vinfo = getValInfo(VOprd);
        if (vinfo && vinfo->NeedPromote && !valinfo->NeedPromote) {
          valinfo->NeedPromote = true;
          worklist.erase(LI);
          hasChange = true;
        } else if (vinfo && !vinfo->NeedPromote && valinfo->NeedPromote) {
          vinfo->NeedPromote = true;
          worklist.erase(LI);
          hasChange = true;
        }
      } else if (InsertElementInst *IEI = dyn_cast<InsertElementInst>(I)) {
        Value *VOprd = IEI->getOperand(0);
        ValueInfo *vinfo = getValInfo(VOprd);
        if (vinfo && ((vinfo->NeedPromote && !valinfo->NeedPromote) || (!vinfo->NeedPromote && valinfo->NeedPromote))) {
          valinfo->NeedPromote = true;
          vinfo->NeedPromote = true;

          worklist.erase(LI);
          hasChange = true;
        }
      }
    }
  }
}

void PromoteInt8Type::promoteInstructions() {
  // Use iterative algo to create a new value.
  //    The algo starts with values that does not depend upon other i8 value,
  //    such as load/argument, etc. It then iterates until new values have been
  //    created for all entries. The only circular dependency could be one caused
  //    by phi. As new i16 phi is created without waiting for its inputs to be
  //    created first, the algo will stop.
  std::list<ValueInfo *> promoteInsts;
  SmallVector<ValueInfo *, 32> phiInsts;
  // insts whose i8 values (dst and/or src) needs not extending
  SmallVector<ValueInfo *, 32> nonPromoteInsts;
  Type *i16Ty = Type::getInt16Ty(m_F->getContext());

  // Instruction Operand re-generation
  //    If an i8 value has been promoted to i16, re-generate its i8 value from
  //    the promoted i16 value. For example of call instruction has i8 arguments,
  //    its i8 arguments shall be re-generated.
  auto regenInstOpnd = [&](Instruction *I, ValueInfo *vinfo) {
    // trunc them to i8 from promoted i16 value.
    // As a same i8 value might be used more than once (for example, as call
    // arguments), make sure they uses the same new trunced value (truncValues
    // is for this purpose).
    DenseMap<Value *, Value *> truncValues;
    m_builder->SetInsertPoint(I);
    for (int i = 0, sz = (int)I->getNumOperands(); i < sz; ++i) {
      Value *V = I->getOperand(i);
      if (isI8Type(V)) {
        Value *newI8Val;
        auto MI = truncValues.find(V);
        if (MI != truncValues.end()) {
          newI8Val = MI->second;
        } else {
          newI8Val = getI8Value(V);
          truncValues.insert(std::make_pair(vinfo->Val, newI8Val));
        }
        I->setOperand(i, newI8Val);
      }
    }
  };

  // Put instructions with i8 operands (either def or use) into two lists
  //    promoteInsts: all insts whose result needs to be promoted and the promotion
  //                  happens at its def site. For example,
  //       (1)   %d = add i8 s0, s1    -->   %d_i16 = add i16 s0_i16, s1_i16
  //       (2)   %d = load i8* p    -->  %d = load i8* p;  %d_i16 = sext i8 %d, i16
  //
  //    nonPromoteInsts: ones whose operands may need to be promoted.
  //       (1)   promoted at use site:
  //             c = cmp (ult)  i8 s0, s1   -->   c = cmp (ult) i16 s0_16,  s1_i16
  //
  //       (2) re-generate i8 at use site
  //             call foo( i8 a0, ...)
  //                 -->
  //             [ a0_i8 = trunc i16 a0_i16 to i8 ]  // if a0 has been promoted to a0_i16
  //             call foo( i8 a0_i8, ...)
  //          a0_i8 is re-generated i8 value if the original a0 has been promoted;
  //          otherwise, a0_i8 is the same as the original a0.
  for (auto &MI : m_valInsertOrder) {
    ValueInfo *valinfo = m_valInfoMap[MI];
    Value *V = valinfo->Val;
    // If the number of promoted uses is larger than non-promoted uses,
    // set NeedPromote to true so that the promotion happens at its definition
    if (valinfo->NPromotedUses > 0 && !V->hasNUsesOrMore(2 * valinfo->NPromotedUses)) {
      valinfo->NeedPromote = true;
    }
    if (valinfo->NeedPromote) {
      promoteInsts.push_back(valinfo);
    } else {
      nonPromoteInsts.push_back(valinfo);
    }
  }

  BasicBlock *entryBB = &m_F->getEntryBlock();
  BasicBlock::iterator entryIP = entryBB->getFirstInsertionPt();

  size_t debugCounter = 0;
  IGC_ASSERT((debugCounter = promoteInsts.size() + 1, 1)); // debug assignment

  while (!promoteInsts.empty()) {
    IGC_ASSERT_MESSAGE(promoteInsts.size() < debugCounter, "ICE: infinite looping test");
    IGC_ASSERT((debugCounter = promoteInsts.size(), 1)); // debug assignment

    auto LE = promoteInsts.end();
    auto Next = promoteInsts.begin();
    for (auto LI = Next; LI != LE; LI = Next) {
      ++Next;
      ValueInfo *valinfo = *LI;
      Value *i8Val = valinfo->Val;
      Instruction *Inst = dyn_cast<Instruction>(i8Val);

      // Generate an incomplete PHI first, and finish up its inputs later.
      if (PHINode *PHI = dyn_cast<PHINode>(i8Val)) {
        m_builder->SetInsertPoint(PHI);
        Type *newType = i16Ty;
        if (auto *i8Vector = dyn_cast<IGCLLVM::FixedVectorType>(i8Val->getType())) {
          newType = IGCLLVM::FixedVectorType::get(i16Ty, (unsigned)i8Vector->getNumElements());
        }
        Value *nV = m_builder->CreatePHI(newType, PHI->getNumIncomingValues());
        valinfo->NewVal = nV;
        valinfo->IsValDead = true;
        replaceAllDbgUsesWith(*Inst, *nV, *Inst, *m_DT);

        // add into a list for finishing up his inputs later.
        phiInsts.push_back(valinfo);
        promoteInsts.erase(LI);
        continue;
      }

      if (Inst) {
        bool isReady = true;
        for (unsigned i = 0, sz = Inst->getNumOperands(); i < sz; ++i) {
          Value *V = Inst->getOperand(i);
          if (ValueInfo *vinfo = getValInfo(V)) {
            if (vinfo->NeedPromote && !vinfo->NewVal) {
              isReady = false;
              break;
            }
          }
        }
        if (!isReady) {
          continue;
        }
      }

      // Create a new i16 instruction
      Value *newVal = nullptr;
      bool isDead = true;
      if (Inst == nullptr) { // argument
        m_builder->SetInsertPoint(entryBB, entryIP);
        newVal = createSI16Value(i8Val);
        isDead = false;
      } else {
        BasicBlock *BB = Inst->getParent();
        BasicBlock::iterator IP = Inst->getIterator();
        ++IP;
        m_builder->SetInsertPoint(BB, IP);
        Value *v0 = (Inst->getNumOperands() > 0 ? Inst->getOperand(0) : nullptr);
        Value *v1 = (Inst->getNumOperands() > 1 ? Inst->getOperand(1) : nullptr);
        IGC_ASSERT_MESSAGE(nullptr != v0, "should have at least one source operand");

        uint32_t opc = Inst->getOpcode();
        switch (opc) {
        case Instruction::Add:
        case Instruction::Sub:
        case Instruction::Mul:
        case Instruction::And:
        case Instruction::Or:
        case Instruction::Xor: {
          IGC_ASSERT(nullptr != v1);
          BinaryOperator *BinOp = cast<BinaryOperator>(Inst);
          newVal = m_builder->CreateBinOp(BinOp->getOpcode(), getSI16Value(v0), getSI16Value(v1), "b2s");
          break;
        }
        case Instruction::SDiv:
        case Instruction::SRem: {
          IGC_ASSERT(nullptr != v1);
          // trunc first, then extend
          Value *nV0 = m_builder->CreateSExt(getI8Value(v0), i16Ty, "b2s");
          Value *nV1 = m_builder->CreateSExt(getI8Value(v1), i16Ty, "b2s");
          if (opc == Instruction::SDiv) {
            newVal = m_builder->CreateSDiv(nV0, nV1, "b2s");
          } else {
            newVal = m_builder->CreateSRem(nV0, nV1, "b2s");
          }
          break;
        }
        case Instruction::UDiv:
        case Instruction::URem: {
          IGC_ASSERT(nullptr != v1);
          // trunc first, then extend
          Value *nV0 = m_builder->CreateZExt(getI8Value(v0), i16Ty, "b2s");
          Value *nV1 = m_builder->CreateZExt(getI8Value(v1), i16Ty, "b2s");
          if (opc == Instruction::UDiv) {
            newVal = m_builder->CreateUDiv(nV0, nV1, "b2s");
          } else {
            newVal = m_builder->CreateURem(nV0, nV1, "b2s");
          }
          break;
        }
        case Instruction::AShr: {
          IGC_ASSERT(nullptr != v1);
          Value *nV0 = m_builder->CreateSExt(getI8Value(v0), i16Ty, "b2s");
          newVal = m_builder->CreateAShr(nV0, getUI16Value(v1), "b2s");
          break;
        }
        case Instruction::LShr: {
          IGC_ASSERT(nullptr != v1);
          Value *nV0 = m_builder->CreateZExt(getI8Value(v0), i16Ty, "b2s");
          newVal = m_builder->CreateLShr(nV0, getUI16Value(v1), "b2s");
          break;
        }
        case Instruction::Shl: {
          IGC_ASSERT(nullptr != v1);
          // upper 8-bit does not matter
          newVal = m_builder->CreateShl(getSI16Value(v0), getUI16Value(v1), "b2s");
          break;
        }
        case Instruction::ZExt:
        case Instruction::SExt: {
          // possible i1 -> i8
          IGC_ASSERT(nullptr != v0);
          IGC_ASSERT(nullptr != v0->getType());
          IGC_ASSERT_MESSAGE(v0->getType()->isIntegerTy(1), "Unexpected integer type i[2-7]!");

          uint16_t trueVal = (opc == Instruction::ZExt ? 0x1 : 0xFFFF);
          newVal = m_builder->CreateSelect(v0, ConstantInt::get(i16Ty, trueVal), ConstantInt::get(i16Ty, 0), "b2s");
          break;
        }
        case Instruction::FPToUI:
        case Instruction::FPToSI: {
          newVal = m_builder->CreateSExt(valinfo->Val, i16Ty, "b2s");
          isDead = false;
          break;
        }
        case Instruction::Select: {
          IGC_ASSERT(nullptr != v1);
          Value *v2 = Inst->getOperand(2);
          Value *si16v1 = getSI16Value(v1);
          Value *si16v2 = getSI16Value(v2);
          newVal = m_builder->CreateSelect(v0, si16v1, si16v2, "b2s");
          break;
        }
        case Instruction::BitCast:
        case Instruction::PtrToInt:
        case Instruction::Load:
        case Instruction::Trunc:
        case Instruction::Call: {
          newVal = createSI16Value(Inst);
          isDead = false;

          if (opc == Instruction::Call) {
            // Re-generate call's arguments.
            regenInstOpnd(Inst, valinfo);
            break;
          }
          break;
        }
        case Instruction::ExtractElement: {
          IGC_ASSERT(nullptr != v1);
          ValueInfo *vinfo = getValInfo(v0);
          // vector operand
          //    not promoted : if it isn't constant and marked to be not promoted;
          //        promoted : otherwise (constant or marked to be promoted)
          if (vinfo && !vinfo->NeedPromote) {
            // not promote vector operand, rather promote dest (EEI).
            m_builder->SetInsertPoint(BB, Inst->getIterator());
            Value *nV1 = getUI16Value(v1);
            if (nV1 != v1) {
              Inst->setOperand(1, nV1);
            }
            m_builder->SetInsertPoint(BB, IP);
            newVal = m_builder->CreateSExt(Inst, i16Ty, "b2s");
            isDead = false;
          } else {
            // Promote vector operand and create a new EEI
            newVal = m_builder->CreateExtractElement(getSI16Value(v0), getUI16Value(v1), "b2s");
          }
          break;
        }
        case Instruction::InsertElement: {
          IGC_ASSERT(nullptr != v1);
          ValueInfo *vinfo = getValInfo(Inst->getOperand(0));
          IGC_ASSERT_MESSAGE((!vinfo || vinfo->NeedPromote), "IEI's vector operands and dst shall be both promoted!");
          Value *v2 = Inst->getOperand(2);
          newVal = m_builder->CreateInsertElement(getSI16Value(v0), getSI16Value(v1), getUI16Value(v2), "b2s");
          break;
        }
        case Instruction::Freeze: {
          newVal = m_builder->CreateFreeze(getSI16Value(v0), "b2s");
          break;
        }
        default:
          IGC_ASSERT_MESSAGE(0, "Unexpected inst of i8");
        } // Switch
      }

      if (newVal) {
        valinfo->NewVal = newVal;
        valinfo->IsValDead = isDead;
        if (isDead)
          replaceAllDbgUsesWith(*Inst, *newVal, *Inst, *m_DT);
      } else {
        IGC_ASSERT_MESSAGE(0, "ICE: inst of type i8 unexpected here!");
      }
      promoteInsts.erase(LI);
    }
  }

  // finish up PHINodes that needs either sext/zext
  for (ValueInfo *valinfo : phiInsts) {
    PHINode *phi = cast<PHINode>(valinfo->Val);
    PHINode *newPhi = cast<PHINode>(valinfo->NewVal);
    unsigned nargs = phi->getNumIncomingValues();
    for (unsigned j = 0; j < nargs; ++j) {
      Value *A = phi->getIncomingValue(j);
      BasicBlock *B = phi->getIncomingBlock(j);
      m_builder->SetInsertPoint(B->getTerminator());
      Value *newA = getSI16Value(A);
      newPhi->addIncoming(newA, B);
    }
  }

  // Re-generate inputs for all insts that only use i8 values
  for (ValueInfo *valinfo : nonPromoteInsts) {
    valinfo->IsValDead = false;
    Value *Val = valinfo->Val;
    if (Instruction *Inst = dyn_cast<Instruction>(Val)) {
      BasicBlock *BB = Inst->getParent();
      BasicBlock::iterator IP = Inst->getIterator();
      m_builder->SetInsertPoint(BB, IP);
      Value *newVal = nullptr;

      // per-instruction replacement
      uint16_t opc = Inst->getOpcode();
      switch (opc) {
      case Instruction::UIToFP:
      case Instruction::SIToFP: {
        Value *v0 = Inst->getOperand(0);
        Inst->setOperand(0, getI8Value(v0));
        break;
      }
      case Instruction::ICmp: {
        Value *v0 = Inst->getOperand(0);
        Value *v1 = Inst->getOperand(1);
        Value *nV0;
        Value *nV1;
        ICmpInst *cmpInst = cast<ICmpInst>(Inst);
        if (cmpInst->isSigned()) {
          nV0 = m_builder->CreateSExt(getI8Value(v0), i16Ty, "b2s");
          nV1 = m_builder->CreateSExt(getI8Value(v1), i16Ty, "b2s");
        } else {
          nV0 = m_builder->CreateZExt(getI8Value(v0), i16Ty, "b2s");
          nV1 = m_builder->CreateZExt(getI8Value(v1), i16Ty, "b2s");
        }
        newVal = m_builder->CreateICmp(cmpInst->getPredicate(), nV0, nV1, "b2s");
        cmpInst->replaceAllUsesWith(newVal);
        valinfo->IsValDead = true;
        break;
      }
      default:
        regenInstOpnd(Inst, valinfo);
        break;
      } // switch
    }
  }

  if (IGC_IS_FLAG_ENABLED(DumpPromoteI8)) {
    const char *fname = m_F->getName().data();
    using namespace IGC::Debug;
    auto name = DumpName(GetShaderOutputName())
                    .Hash(m_Ctx->hash)
                    .Type(m_Ctx->type)
                    .Pass("promoteI8")
                    .PostFix(fname)
                    .Retry(m_Ctx->m_retryManager.GetRetryId())
                    .Extension("txt");

    Dump i8Dump(name, DumpType::DBG_MSG_TEXT);

    DumpLock();
    print(i8Dump.stream());
    DumpUnlock();
  }
}

void PromoteInt8Type::deleteDeadInst() {
  // Use ValueInfo::IsValDead to remove instructions.
  // (As Algo does not use RAUW, it would need iterative approach to delete
  //  all dead instructions. Alternatively, using IsValDead is simpler.)
  for (auto &MI : m_valInsertOrder) {
    ValueInfo *valinfo = m_valInfoMap[MI];
    if (!m_changed && valinfo->NewVal)
      m_changed = true;

    if (!valinfo->IsValDead)
      continue;
    Instruction *Inst = dyn_cast<Instruction>(valinfo->Val);
    if (!Inst)
      continue;
    // Before erase Inst, replace all its uses with a dummy value
    // as there may have dead uses that have not been deleted yet.
    Value *dummyVal = UndefValue::get(Inst->getType());
    Inst->replaceAllUsesWith(dummyVal);
    Inst->eraseFromParent();
  }
}

// Return V if V is NOT of i8; otherwise, promote i8 to i16 and return
// the promoted i16 value. If needed, instructions would be inserted at
// the current location of m_builder.
Value *PromoteInt8Type::getI16Value(Value *V, bool IsZExt) {
  if (!isI8Type(V)) {
    return V;
  }
  Type *i16Ty = Type::getInt16Ty(m_F->getContext());
  if (ConstantInt *CstI = dyn_cast<ConstantInt>(V)) {
    uint64_t n = IsZExt ? CstI->getZExtValue() : CstI->getSExtValue();
    return ConstantInt::get(i16Ty, n);
  }

  if (ConstantDataVector *CDVI = dyn_cast<ConstantDataVector>(V)) {
    IGC_ASSERT(nullptr != CDVI->getElementType());
    uint32_t nelts = CDVI->getNumElements();
    if (!CDVI->getElementType()->isIntegerTy()) {
      IGC_ASSERT_MESSAGE(0, "unsupported constant expression");
      return nullptr;
    }
    SmallVector<uint16_t, 16> Elts;
    for (unsigned idx = 0; idx < nelts; ++idx) {
      ConstantInt *CstI = cast<ConstantInt>(CDVI->getElementAsConstant(idx));
      uint64_t n = IsZExt ? CstI->getZExtValue() : CstI->getSExtValue();
      Elts.push_back((uint16_t)n);
    }
    ArrayRef<uint16_t> CElts(Elts);
    return ConstantDataVector::get(m_F->getContext(), CElts);
  }

  if (ConstantVector *CVI = dyn_cast<ConstantVector>(V)) {
    Type *Ty = dyn_cast<VectorType>(CVI->getType())->getElementType();
    IGC_ASSERT(nullptr != Ty);

    if (!Ty->isIntegerTy()) {
      IGC_ASSERT_MESSAGE(0, "unsupported constant expression");
      return nullptr;
    }

    uint32_t nelts = CVI->getNumOperands();

    SmallVector<Constant *, 16> Vals(nelts, nullptr);
    for (unsigned idx = 0; idx < nelts; ++idx) {
      auto elem = CVI->getOperand(idx);
      auto CstI = dyn_cast<ConstantInt>(elem);
      if (CstI) {
        uint64_t n = IsZExt ? CstI->getZExtValue() : CstI->getSExtValue();
        Vals[idx] = ConstantInt::get(i16Ty, n, IsZExt);
      } else {
        Vals[idx] = UndefValue::get(i16Ty);
      }
    }
    return ConstantVector::get(Vals);
  }

  if (isa<UndefValue>(V)) {
    if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType())) {
      auto *nVTy = IGCLLVM::FixedVectorType::get(i16Ty, (unsigned)VTy->getNumElements());
      return UndefValue::get(nVTy);
    }
    return UndefValue::get(i16Ty);
  }

  if (isa<Argument>(V) || isa<Instruction>(V)) {
    auto II = m_valInfoMap.find(V);
    IGC_ASSERT(II != m_valInfoMap.end());

    ValueInfo *valinfo = II->second;
    Value *newVal = valinfo->NewVal;
    if (!newVal) {
      // Need to create I16 vector at the use site here
      newVal = createI16Value(V, IsZExt);
    }
    return newVal;
  }

  IGC_ASSERT_EXIT_MESSAGE(0, "Const expr should be broken already!");
  return nullptr;
}

// An original value of i8 could have been promoted and thus has a new value.
// This function will return i8 version of the new value.
Value *PromoteInt8Type::getI8Value(Value *V) {
  if (!isI8Type(V)) {
    return V;
  }
  Type *i8Ty = Type::getInt8Ty(m_F->getContext());
  if (isa<Constant>(V)) {
    return V;
  }

  if (isa<Argument>(V) || isa<Instruction>(V)) {
    auto II = m_valInfoMap.find(V);
    IGC_ASSERT_MESSAGE(II != m_valInfoMap.end(), "ICE: Value should be in valInfoMap!");

    ValueInfo *valinfo = II->second;
    if (!valinfo->IsValDead) {
      return V;
    }
    Value *nVal = valinfo->NewVal;
    IGC_ASSERT_MESSAGE(nVal, "ICE: NewVal is null!");
    if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType())) {
      // Do the following:
      //    a0 = extElt V, 0
      //    a1 = extElt V, 1
      //    ...
      //    an = extElt V, n
      //
      //    b0 = trunc a0 to i8
      //    b1 = trunc a1 to i8
      //    ...
      //    bn = trunc an to i8
      //
      //    v0 = insElt undef, b0, 0
      //    v1 = insElt v0,    b1, 1
      //    ......
      //    vn = insElt vn-1,  bn, n
      int nElts = (int)VTy->getNumElements();
      SmallVector<Value *, 16> elts;
      for (int i = 0; i < nElts; ++i) {
        Value *eltVal = m_builder->CreateExtractElement(nVal, i);
        elts.push_back(eltVal);
      }
      for (int i = 0; i < nElts; ++i) {
        Value *nEltVal = m_builder->CreateTrunc(elts[i], i8Ty);
        elts[i] = nEltVal;
      }
      auto *nVTy = IGCLLVM::FixedVectorType::get(i8Ty, nElts);
      nVal = UndefValue::get(nVTy);
      for (int i = 0; i < nElts; ++i) {
        nVal = m_builder->CreateInsertElement(nVal, elts[i], i);
      }
    } else {
      nVal = m_builder->CreateTrunc(nVal, i8Ty);
    }
    return nVal;
  } else {
    IGC_ASSERT_MESSAGE(0, "missing cases!");
  }

  IGC_ASSERT_EXIT_MESSAGE(0, "Const expr should be broken already!");
  return nullptr;
}

// Return a new value of i16, given a value of i8
Value *PromoteInt8Type::createI16Value(Value *V, bool IsZExt) {
  IGC_ASSERT(isI8Type(V));
  IGC_ASSERT(false == isa<Constant>(V));
  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType());
  Type *i16Ty = Type::getInt16Ty(m_F->getContext());
  if (!VTy) {
    // Just doing SExt or ZExt
    Value *nVal;
    if (IsZExt) {
      nVal = m_builder->CreateZExt(V, i16Ty, "b2s");
    } else {
      nVal = m_builder->CreateSExt(V, i16Ty, "b2s");
    }
    return nVal;
  }

  int nElts = (int)VTy->getNumElements();
  SmallVector<Value *, 16> i8Elts, i16Elts;

  // Extract all elements of i8
  for (int i = 0; i < nElts; ++i) {
    Value *i8Val = m_builder->CreateExtractElement(V, i);
    i8Elts.push_back(i8Val);
  }

  // extend i8 -> i16
  for (int i = 0; i < nElts; ++i) {
    Value *i8Val = i8Elts[i];
    Value *i16Val;
    if (IsZExt) {
      i16Val = m_builder->CreateZExt(i8Val, i16Ty, "b2s");
    } else {
      i16Val = m_builder->CreateSExt(i8Val, i16Ty, "b2s");
    }
    i16Elts.push_back(i16Val);
  }

  // Create vector of i16
  auto *nVTy = IGCLLVM::FixedVectorType::get(i16Ty, nElts);
  Value *newVal = UndefValue::get(nVTy);
  for (int i = 0; i < nElts; ++i) {
    Value *i16Val = i16Elts[i];
    newVal = m_builder->CreateInsertElement(newVal, i16Val, i);
  }
  return newVal;
}

// Return true if GII is a unsigned intrinsic, ie, its operands are unsigned.
bool PromoteInt8Type::isUnsigned(GenIntrinsicInst *GII) const {
  WaveOps wop = WaveOps::UNDEF;
  if (WavePrefixIntrinsic *WPI = dyn_cast<WavePrefixIntrinsic>(GII)) {
    wop = WPI->getOpKind();
  } else if (QuadPrefixIntrinsic *QPI = dyn_cast<QuadPrefixIntrinsic>(GII)) {
    wop = QPI->getOpKind();
  }
  switch (wop) {
  case WaveOps::UMIN:
  case WaveOps::UMAX:
    return true;
  default:
    break;
  }
  return false;
}

void PromoteInt8Type::promoteIntrinsic() {
  // If ConvertAll is true, all i8 shuffle intrinsics are
  // converted to i16 version!  If this is not optimal, fine
  // tuning shall be done here.
  const bool ConvertAll = false;

  SmallVector<CallInst *, 16> Candidates;
  Module *M = m_F->getParent();
  for (auto II = inst_begin(m_F), IE = inst_end(m_F); II != IE; ++II) {
    Instruction *Inst = &*II;
    GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(Inst);
    if (!GII)
      continue;
    if (GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveShuffleIndex) ||
        GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveBroadcast) ||
        GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveClusteredBroadcast) ||
        GII->isGenIntrinsic(GenISAIntrinsic::GenISA_simdShuffleDown)) {
      // Those are mov insts. Need to promote if its operand is
      // of type I8 and index is not uniform.
      Type *Ty = GII->getType();
      Value *IndexOrDelta;
      if (GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveShuffleIndex) ||
          GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveBroadcast)) {
        IndexOrDelta = GII->getArgOperand(1);
      } else {
        IndexOrDelta = GII->getArgOperand(2);
      }
      if (Ty->isIntegerTy(8) && (!isa<ConstantInt>(IndexOrDelta) || ConvertAll)) {
        Candidates.push_back(GII);
      }
    } else if (GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveAll) ||
               GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveClustered) ||
               GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveInterleave) ||
               GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveClusteredInterleave) ||
               GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WavePrefix) ||
               GII->isGenIntrinsic(GenISAIntrinsic::GenISA_QuadPrefix) ||
               GII->isGenIntrinsic(GenISAIntrinsic::GenISA_WaveClusteredPrefix)) {
      // Those are scan or reduce functions. If the operand type
      // is of I8, need to promote it to avoid ALU on I8 type.
      Type *Ty = GII->getType();
      if (Ty->isIntegerTy(8)) {
        Candidates.push_back(GII);
      }
    }
  }

  // Now, let's promote
  Type *i16Ty = Type::getInt16Ty(m_F->getContext());
  for (auto &II : Candidates) {
    CallInst *CII = II;
    GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(CII);
    if (GII) {
      GenISAIntrinsic::ID gid = GII->getIntrinsicID();
      if (gid == GenISAIntrinsic::GenISA_WaveAll || gid == GenISAIntrinsic::GenISA_WaveClustered ||
          gid == GenISAIntrinsic::GenISA_WaveInterleave || gid == GenISAIntrinsic::GenISA_WaveClusteredInterleave ||
          gid == GenISAIntrinsic::GenISA_WavePrefix || gid == GenISAIntrinsic::GenISA_QuadPrefix ||
          gid == GenISAIntrinsic::GenISA_WaveClusteredPrefix || gid == GenISAIntrinsic::GenISA_WaveShuffleIndex ||
          gid == GenISAIntrinsic::GenISA_WaveBroadcast || gid == GenISAIntrinsic::GenISA_WaveClusteredBroadcast ||
          gid == GenISAIntrinsic::GenISA_simdShuffleDown) {
        //
        // All those intrinsic have prototype like
        //    Ty  <genIntrinsic> (Ty, ......)
        // where Ty is the sole overloaded type. The intrinsic might
        // have 2 to 4 arguments.
        //
        m_builder->SetInsertPoint(GII);

        // New intrinsic with return type I16
        Function *nFunc = GenISAIntrinsic::getDeclaration(M, gid, i16Ty);
        SmallVector<Value *, 4> iArgs; // args for call to the new intrinsic

        // It seems safe to use signed extension always, but using zext for
        // unsigne operation makes code more readable.
        Value *nS0 = nullptr;
        if (isUnsigned(GII)) {
          nS0 = m_builder->CreateZExt(GII->getArgOperand(0), i16Ty);
        } else {
          nS0 = m_builder->CreateSExt(GII->getArgOperand(0), i16Ty);
        }
        iArgs.push_back(nS0);

        switch (gid) {
        case GenISAIntrinsic::GenISA_simdShuffleDown: {
          // prototype:  Ty <shuffledown> (Ty, Ty, int)
          Value *nS1 = m_builder->CreateSExt(GII->getArgOperand(1), i16Ty);
          iArgs.push_back(nS1);
          iArgs.push_back(GII->getArgOperand(2));
          break;
        }
        case GenISAIntrinsic::GenISA_WaveClustered:
        case GenISAIntrinsic::GenISA_WaveInterleave:
        case GenISAIntrinsic::GenISA_WaveClusteredBroadcast:
        case GenISAIntrinsic::GenISA_WaveClusteredPrefix: {
          // prototype:
          //     Ty <clustered> (Ty, char, int, int)
          //     Ty <interleave> (Ty, char, int, int)
          //     Ty <clusteredbroadcast> (Ty, int, int, int)
          iArgs.push_back(GII->getArgOperand(1));
          iArgs.push_back(GII->getArgOperand(2));
          iArgs.push_back(GII->getArgOperand(3));
          break;
        }
        case GenISAIntrinsic::GenISA_WavePrefix:
        case GenISAIntrinsic::GenISA_WaveClusteredInterleave: {
          // prototype:  Ty <waveprefix> (Ty, char, bool, bool, int)
          // prototype:  Ty <clusteredInterleave> (Ty, char, int, int, int)
          iArgs.push_back(GII->getArgOperand(1));
          iArgs.push_back(GII->getArgOperand(2));
          iArgs.push_back(GII->getArgOperand(3));
          iArgs.push_back(GII->getArgOperand(4));
          break;
        }
        case GenISAIntrinsic::GenISA_QuadPrefix:
        case GenISAIntrinsic::GenISA_WaveShuffleIndex:
        case GenISAIntrinsic::GenISA_WaveBroadcast:
        case GenISAIntrinsic::GenISA_WaveAll: {
          // prototype:
          //     Ty <quadprefix> (Ty, char, bool)
          //     Ty <shuffleIndex> (Ty, int, int)
          //     Ty <waveall> (Ty, char, int)
          iArgs.push_back(GII->getArgOperand(1));
          iArgs.push_back(GII->getArgOperand(2));
          break;
        }
        default:
          IGC_ASSERT_MESSAGE(0, "intrinsic candidates should be promoted");
          break;
        }

        Value *nCall = m_builder->CreateCall(nFunc, iArgs, "PromotedShuffle");

        // Need to adjust identity value (I) for exclusive Scan imax/imin.
        //   Given (a0, a1, a2, a3, ..., ),  with exclusive scan operation 'op'
        //   (op = imax or imin), then, result is
        //        (I, (a0), (a0 op a1), (a0 op a1 op a2), ...)
        // I is dependent upon the type,  for short, it is either SHRT_MIN/SHRT_MAX;
        // for signed char,  it is either SCHAR_MIN/SCHAR/MAX.
        // Once the intrinsic is promoted to i16, I is SHRT_MIN/MAX; but the correct
        // value should be SCHAR_MIN/MAX. Here we must check if I is in the result and
        // replace it with SCHAR version if it is.
        WavePrefixIntrinsic *WPI = dyn_cast<WavePrefixIntrinsic>(GII);
        QuadPrefixIntrinsic *QPI = dyn_cast<QuadPrefixIntrinsic>(GII);
        if ((WPI && !WPI->isInclusiveScan()) || (QPI && !QPI->isInclusiveScan())) {
          WaveOps WOps = WPI ? WPI->getOpKind() : QPI->getOpKind();
          switch (WOps) {
          case WaveOps::IMIN: {
            Value *eq = m_builder->CreateICmpEQ(nCall, ConstantInt::get(i16Ty, std::numeric_limits<int16_t>::max()));
            nCall = m_builder->CreateSelect(eq, ConstantInt::get(i16Ty, std::numeric_limits<int8_t>::max()), nCall);
            break;
          }
          case WaveOps::IMAX: {
            // the neutral values for i16 and i8 are different -- replace
            Value *eq = m_builder->CreateICmpEQ(nCall, ConstantInt::get(i16Ty, std::numeric_limits<int16_t>::min()));
            nCall = m_builder->CreateSelect(eq, ConstantInt::get(i16Ty, std::numeric_limits<int8_t>::min()), nCall);
            break;
          }
          default:
            break;
          }
        }

        Value *nRes = m_builder->CreateTrunc(nCall, GII->getType());
        GII->replaceAllUsesWith(nRes);
        GII->eraseFromParent();
        m_changed = true;
      }
    }
  }
}

void PromoteInt8Type::dump() const { print(dbgs()); }

void PromoteInt8Type::print(raw_ostream &OS, const Module *) const {
  // order the output so it is easier to read
  std::list<ValueInfo *> orderedI8Vals;
  // All arguments
  auto IME = m_valInfoMap.end();
  for (auto AI = m_F->arg_begin(), AE = m_F->arg_end(); AI != AE; ++AI) {
    Value *aVal = &*AI;
    auto II = m_valInfoMap.find(aVal);
    if (II != IME) {
      ValueInfo *vinfo = II->second;
      orderedI8Vals.push_back(vinfo);
    }
  }
  // All instructions
  for (auto II = inst_begin(m_F), IE = inst_end(m_F); II != IE; ++II) {
    Instruction *Inst = &*II;
    auto II1 = m_valInfoMap.find(Inst);
    if (II1 != IME) {
      ValueInfo *vinfo = II1->second;
      orderedI8Vals.push_back(vinfo);
    }
  }

  OS << "\n---- int8 arguments and instructions ----\n";
  for (auto II : orderedI8Vals) {
    ValueInfo *valinfo = II;
    Value *V = valinfo->Val;
    OS << "\n";
    V->print(OS);
    if (!isI8Type(V)) {
      OS << "    :<not i8>";
    } else if (valinfo->NeedPromote) {
      OS << "    :<sext>";
    } else {
      OS << "    :<may promote later at use>";
    }
    OS << "\n" << "    Operands:\n";
    if (Instruction *Inst = dyn_cast<Instruction>(V)) {
      for (int i = 0, sz = (int)Inst->getNumOperands(); i < sz; ++i) {
        Value *tV = Inst->getOperand(i);
        if (ValueInfo *vinfo = getValInfo(tV)) {
          OS << "        ";
          vinfo->Val->print(OS);
          if (vinfo->NeedPromote) {
            OS << "    :<use either i8 or promoted-at-def>";
          } else if (!tV->getType()->isVectorTy()) {
            OS << "    :<use either i8 or promote here>";
          } else {
            OS << "    :<use i8>";
          }
          OS << "\n";
        }
      }
      OS << "\n";
    }
  }
}
