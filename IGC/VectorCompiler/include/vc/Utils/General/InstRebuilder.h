/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_INST_REBUILDER_H
#define VC_UTILS_GENERAL_INST_REBUILDER_H

#include "Probe/Assertion.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Use.h>
#include <llvm/Support/Casting.h>

#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vc {

// Creates new instruction with all the properties taken from the \p OrigInst
// except for operands that are taken from \p NewOps.
// nullptr is returned when clonning is imposible or yet unsupported.
llvm::Instruction *cloneInstWithNewOps(llvm::Instruction &OrigInst,
                                       llvm::ArrayRef<llvm::Value *> NewOps);

// This structure defines a use with \p User instruction and \p OperandNo of its
// operand. And there's new value \p NewOperand for this operand.
struct UseToRebuild {
  llvm::Instruction *User = nullptr;
  int OperandNo;
  llvm::Value *NewOperand;
  bool IsTerminal = false;
};

// This structure defines which \p OperandNos of \p User instruction should be
// rebuilt. Corresponding new values are provided in \p NewOperands.
// (OperandNos.size() == NewOperands.size())
struct InstToRebuild {
  llvm::Instruction *User = nullptr;
  std::vector<int> OperandNos;
  std::vector<llvm::Value *> NewOperands;
  bool IsTerminal = false;

  // Returns whether the structure represents a valid info to rebuild an
  // instruction.
  bool validate() const {
    if (!User)
      return false;
    if (OperandNos.size() != NewOperands.size())
      return false;
    int NumOperands = User->getNumOperands();
    return std::all_of(OperandNos.begin(), OperandNos.end(),
                       [NumOperands](auto OperandNo) {
                         return OperandNo >= 0 && OperandNo < NumOperands;
                       });
  }
};

// The info required to rebuild the instructions.
// If element's NewOperand is equal to nullptr, it means that this operand/use
// should be replaced with previously build instruction.
using RebuildInfo = std::vector<UseToRebuild>;

// A helper class to generate RebuildInfo.
// Abstract:
// One does not simply change an operand of an instruction with a value with a
// different type. In this case instruction changes its type and must be
// rebuild. That causes a chain reaction as instruction's users now has to be
// rebuild to.
//
// Usage:
// A user should provide instructions into this builder in reverse post-order.
// An instruction must be defined as entry (the one that causes chain reaction)
// or as a potential node inside the chain(user can pass all instructions that
// are not entries - that's fine, if user knows for sure that this instruction
// isn't in the chain, user is able to not pass this instruction).
// A user must provide a functor (const Instruction &)-> bool that will define
// whether an instruction is a terminal - the last instruction in the chain
// reaction. For all other instructions it is considered that they are
// continuing the reaction.
template <typename IsTerminalFunc> class RebuildInfoBuilder {
  IsTerminalFunc IsTerminal;
  RebuildInfo Info;
  std::unordered_set<llvm::Value *> ChangedOperands;

public:
  RebuildInfoBuilder(IsTerminalFunc IsTerminalIn) : IsTerminal{IsTerminalIn} {}

  void addEntry(llvm::Instruction &Inst, int OperandNo,
                llvm::Value &NewOperand) {
    addNode(Inst, OperandNo, &NewOperand);
  }

  void addNodeIfRequired(llvm::Instruction &Inst, int OperandNo) {
    if (ChangedOperands.count(Inst.getOperand(OperandNo)))
      addNode(Inst, OperandNo, nullptr);
  }

  // Emit the gathered data.
  RebuildInfo emit() && { return std::move(Info); }

private:
  void addNode(llvm::Instruction &Inst, int OperandNo,
               llvm::Value *NewOperand) {
    // Users are covered here too as phi use can be back edge, so RPO won't help
    // and it won't be covered.
    IGC_ASSERT_MESSAGE(!llvm::isa<llvm::PHINode>(Inst),
                       "phi-nodes aren't yet supported");
    IGC_ASSERT_MESSAGE(IsTerminal(Inst) ||
                           std::all_of(Inst.user_begin(), Inst.user_end(),
                                       [](const llvm::User *U) {
                                         return !llvm::isa<llvm::PHINode>(U);
                                       }),
                       "phi-nodes aren't yet supported");
    auto InstIsTerminal = IsTerminal(Inst);
    Info.push_back({&Inst, OperandNo, NewOperand, InstIsTerminal});
    if (!InstIsTerminal)
      ChangedOperands.insert(&Inst);
  }
};

template <typename IsTerminalFunc>
RebuildInfoBuilder<IsTerminalFunc>
MakeRebuildInfoBuilder(IsTerminalFunc IsTerminator) {
  return RebuildInfoBuilder{IsTerminator};
}

// Rebuilds instructions according to info provided in RebuildInfo.
// New instructions inherit all properties of original ones, only
// operands change. User can customize this behaviour with two functors:
//    IsSpecialInst: (const InstToRebuild&) -> bool - returns whether inst
//      should be processed with a custom handler
//    CreateSpecialInst: (const InstToRebuild&) -> Value* - custom handler
//      to rebuild provided instruction.
template <typename IsSpecialInstFunc, typename CreateSpecialInstFunc>
class InstructionRebuilder {
  // Pop should be called only in getNextInstToRebuild.
  std::vector<UseToRebuild> ToRebuild;
  IsSpecialInstFunc IsSpecialInst;
  CreateSpecialInstFunc CreateSpecialInst;
  // Map between original inst and its replacement.
  std::unordered_map<llvm::Instruction *, llvm::Value *> Replacement;
  std::vector<llvm::Instruction *> ToErase;

public:
  InstructionRebuilder(RebuildInfo ToRebuildIn,
                       IsSpecialInstFunc IsSpecialInstIn,
                       CreateSpecialInstFunc CreateSpecialInstIn)
      : ToRebuild{std::move(ToRebuildIn)}, IsSpecialInst{IsSpecialInstIn},
        CreateSpecialInst{CreateSpecialInstIn} {}

  void rebuild() && {
    std::vector<llvm::Instruction *> Terminals;
    for (auto First = ToRebuild.begin(), Last = ToRebuild.end();
         First != Last;) {
      InstToRebuild InstInfo;
      std::tie(InstInfo, First) = getNextInstToRebuild(First, Last);
      IGC_ASSERT_MESSAGE(!llvm::isa<llvm::PHINode>(InstInfo.User),
                         "phi-nodes aren't yet supported");
      IGC_ASSERT_MESSAGE(InstInfo.validate(),
                         "an illegal rebuild info is generated");
      rebuildNonPhiInst(InstInfo);
      if (InstInfo.IsTerminal)
        Terminals.push_back(InstInfo.User);
    }
    for (auto *Terminal : Terminals)
      Terminal->replaceAllUsesWith(Replacement[Terminal]);
    // Instructions must be deleted in post-order - uses first, than defs.
    // As ToErase is in RPO, reverse is required.
    for (auto *Inst : reverse(ToErase))
      Inst->eraseFromParent();
  }

private:
  // Takes a range of UseToRebuild - [\p First, \p Last).
  // Aggregates first uses with the same user from the range and adds collected
  // Replacement info to produce info for the next inst to rebuild. Returns
  // collected inst info and the first use with a different to returned user
  // (next user) or \p Last when there's no more users.
  template <typename InputIter>
  std::pair<InstToRebuild, InputIter> getNextInstToRebuild(InputIter First,
                                                           InputIter Last) {
    IGC_ASSERT_MESSAGE(First != Last,
                       "this method shouldn't be called when list of uses to "
                       "rebuild is already empty");
    InstToRebuild CurInst;
    CurInst.User = First->User;
    CurInst.IsTerminal = First->IsTerminal;
    auto LastUse = std::adjacent_find(
        First, Last, [](const UseToRebuild &LHS, const UseToRebuild &RHS) {
          return LHS.User != RHS.User;
        });
    if (LastUse != Last)
      ++LastUse;
    // Filling operand related fields.
    CurInst =
        std::accumulate(First, LastUse, std::move(CurInst),
                        [this](InstToRebuild Inst, const UseToRebuild &Use) {
                          return appendOperand(std::move(Inst), Use);
                        });
    return {CurInst, LastUse};
  }

  // Appends operand/use from \p CurUse to \p InstInfo.
  // Returns updated \p InstInfo.
  InstToRebuild appendOperand(InstToRebuild &&InstInfo,
                              const UseToRebuild &CurUse) {
    IGC_ASSERT_MESSAGE(InstInfo.User == CurUse.User,
                       "trying to append a wrong use with wrong user");
    IGC_ASSERT_MESSAGE(
        InstInfo.IsTerminal == CurUse.IsTerminal,
        "two uses don't agree on the instruction being terminal");
    InstInfo.OperandNos.push_back(CurUse.OperandNo);
    auto *NewOperand = CurUse.NewOperand;
    if (!NewOperand) {
      NewOperand = Replacement.at(llvm::cast<llvm::Instruction>(
          CurUse.User->getOperand(CurUse.OperandNo)));
    }
    InstInfo.NewOperands.push_back(NewOperand);
    return std::move(InstInfo);
  }

  void rebuildNonPhiInst(InstToRebuild &OrigInst) {
    auto *Replace = createNonPhiInst(OrigInst);
    Replacement[OrigInst.User] = Replace;
    ToErase.push_back(OrigInst.User);
  }

  // Unlike rebuildNonPhiInst method just creates instruction, doesn't
  // update the class state.
  llvm::Value *createNonPhiInst(InstToRebuild &OrigInst) const {
    if (IsSpecialInst(OrigInst))
      return CreateSpecialInst(OrigInst);
    llvm::Instruction *Replace =
        vc::cloneInstWithNewOps(*OrigInst.User, createNewOperands(OrigInst));
    if (!Replace)
      return coverNonCloneCase(*OrigInst.User, createNewOperands(OrigInst));
    Replace->takeName(OrigInst.User);
    Replace->insertBefore(OrigInst.User);
    Replace->setDebugLoc(OrigInst.User->getDebugLoc());
    return Replace;
  }

  static std::vector<llvm::Value *>
  getOrigOperands(llvm::Instruction &OrigInst) {
    if (llvm::isa<llvm::IntrinsicInst>(OrigInst)) {
      auto &OrigIntr = llvm::cast<llvm::IntrinsicInst>(OrigInst);
      return {OrigIntr.arg_begin(), OrigIntr.arg_end()};
    }
    return {OrigInst.value_op_begin(), OrigInst.value_op_end()};
  }

  // Takes arguments of the original instruction (OrigInst.User) and rewrites
  // the required ones with new values according to info in \p OrigInst
  static std::vector<llvm::Value *>
  createNewOperands(const InstToRebuild &OrigInst) {
    auto NewOperands = getOrigOperands(*OrigInst.User);
    for (auto &&OpReplacement :
         zip(OrigInst.OperandNos, OrigInst.NewOperands)) {
      int OperandNo = std::get<0>(OpReplacement);
      llvm::Value *NewOperand = std::get<1>(OpReplacement);
      IGC_ASSERT_MESSAGE(OperandNo >= 0, "no such operand");
      IGC_ASSERT_MESSAGE(OperandNo < static_cast<int>(NewOperands.size()),
                         "no such operand");
      NewOperands[OperandNo] = NewOperand;
    }
    return std::move(NewOperands);
  }

  // covers cases when \p OrigInst cannot be cloned by cloneInstWithNewOps
  // with the provided \p NewOps.
  // Peplacement for the \p OrigInst is returned.
  static llvm::Value *coverNonCloneCase(llvm::Instruction &OrigInst,
                                        llvm::ArrayRef<llvm::Value *> NewOps) {
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::AddrSpaceCastInst>(OrigInst),
                       "only addr space cast case is yet considered");
    IGC_ASSERT_MESSAGE(NewOps.size() == 1, "cast has only one operand");
    llvm::Value *NewOp = NewOps.front();
    auto *NewOpTy = llvm::cast<llvm::PointerType>(NewOp->getType());
    auto *CastTy = llvm::cast<llvm::PointerType>(OrigInst.getType());
    IGC_ASSERT_MESSAGE(
        NewOpTy->getAddressSpace() == CastTy->getAddressSpace(),
        "when addrspaces different clonnig helps and it should've "
        "been covered before");
    return NewOp;
  }
};

template <typename IsSpecialInstFunc, typename CreateSpecialInstFunc>
InstructionRebuilder<IsSpecialInstFunc, CreateSpecialInstFunc>
MakeInstructionRebuilder(RebuildInfo Info, IsSpecialInstFunc IsSpecialInst,
                         CreateSpecialInstFunc CreateSpecialInst) {
  return {std::move(Info), std::move(IsSpecialInst),
          std::move(CreateSpecialInst)};
}

inline auto MakeInstructionRebuilder(RebuildInfo Info) {
  return MakeInstructionRebuilder(
      std::move(Info), [](const InstToRebuild &Inst) { return false; },
      [](const InstToRebuild &Inst) { return nullptr; });
}

} // namespace vc

#endif // VC_UTILS_GENERAL_INST_REBUILDER_H
