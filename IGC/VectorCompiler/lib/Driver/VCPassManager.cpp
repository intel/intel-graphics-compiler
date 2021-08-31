/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VCPassManager.h"

#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Regex.h>

#include "Probe/Assertion.h"

#include <llvm/ADT/StringMap.h>

#include <string>

using namespace llvm;

namespace {
struct PassNumber {
  int N;
  PassNumber(int n) : N(n) {}
  std::string str() { return N ? (std::string("#") + std::to_string(N)) : ""; }
};

class PassIndexSet {
  SmallVector<int, 1> Indexes;
  int Occurence;
  unsigned i = 0;

public:
  PassIndexSet(int Occ = 0) : Occurence(Occ) {}
  std::pair<bool, PassNumber> isCurrent() {
    ++Occurence;
    if (Indexes.empty())
      return {true, Occurence};
    if ((i < Indexes.size()) && (Occurence == Indexes[i])) {
      ++i;
      return {true, Occurence};
    }
    return {false, Occurence};
  }

  void add(int Val) {
    auto Begin = Indexes.begin();
    auto End = Indexes.end();
    auto Iter = std::lower_bound(Begin, End, Val);
    if (Begin == End || *Iter != Val)
      Indexes.insert(Iter, Val);
  }
};

class PassSet {
  StringMap<PassIndexSet> PassMap;

public:
  void insert(StringRef PassArg, int Index) { PassMap[PassArg].add(Index); }
  void insert(StringRef PassArg) { PassMap[PassArg]; }
  std::pair<bool, PassNumber> includes(StringRef PassArg) {
    auto It = PassMap.find(PassArg);
    auto End = PassMap.end();
    if (It == End) {
      if (PassMap.count("*")) {
        PassMap.insert({PassArg, 1});
        return {true, 1};
      }
      return {false, 0};
    }
    return It->second.isCurrent();
  }
  bool empty() { return PassMap.empty(); }
};

} // namespace

template <>
bool cl::parser<PassSet>::parse(cl::Option &O, StringRef ArgName,
                                StringRef ArgValue, PassSet &Val) {
  static auto RegExp = Regex("\\*|([^#]+)(#([[:digit:]]+))?");
  SmallVector<StringRef, 4> Matches;
  if (RegExp.match(ArgValue, &Matches)) {
    auto &Whole = Matches[0];
    auto &PassArg = Matches[1];
    auto &PassNumber = Matches[3];
    if (PassNumber.size())
      Val.insert(
          std::move(PassArg),
          std::stoi(std::string{std::make_move_iterator(PassNumber.begin()),
                                std::make_move_iterator(PassNumber.end())}));
    else
      Val.insert(std::move(Whole));
    return false;
  }
  return true;
}

namespace {

class PassSetOption {
  cl::opt<PassSet> CLOption;

public:
  template <typename... Mods>
  PassSetOption(const Mods &... Ms)
      : CLOption(Ms..., cl::ZeroOrMore, cl::CommaSeparated) {}
  auto &getValue() { return CLOption.getValue(); }
  bool empty() { return getValue().empty(); }
};

struct ExtraIRDumpBeforePass {
  static PassSetOption option;
  static auto createPass(const PassInfo *PI, PassNumber N) {
    auto PassName = PI->getPassName();
    auto PassArg = PI->getPassArgument();
    return createPrintModulePass(
        llvm::errs(), ("*** IR Dump Before " + PI->getPassArgument() +
                       " *** (" + PassArg + N.str() + ")")
                          .str());
  }
};
PassSetOption ExtraIRDumpBeforePass::option{
    "vc-dump-ir-before-pass",
    cl::desc("Debug only. Dump IR of the module before the specified pass.")};

struct ExtraVerificationBeforePass {
  static PassSetOption option;
  static auto createPass(const PassInfo *PI, PassNumber N) {
    errs() << "extra verifier shall be run before <" << PI->getPassArgument()
           << N.str() << ">\n";
    return createVerifierPass();
  }
};
PassSetOption ExtraVerificationBeforePass::option{
    "vc-run-verifier-before-pass",
    cl::desc("Debug only. Run verifier before the specified pass.")};

struct ExtraIRDumpAfterPass {
  static PassSetOption option;
  static auto createPass(const PassInfo *PI, PassNumber N) {
    auto PassName = PI->getPassName();
    auto PassArg = PI->getPassArgument();
    return createPrintModulePass(llvm::errs(),
                                 ("*** IR Dump After " + PI->getPassArgument() +
                                  " *** (" + PassArg + N.str() + ")")
                                     .str());
  }
};
PassSetOption ExtraIRDumpAfterPass::option{
    "vc-dump-ir-after-pass",
    cl::desc("Debug only. Dump IR of the module after the specified pass.")};

struct ExtraVerificationAfterPass {
  static PassSetOption option;
  static auto createPass(const PassInfo *PI, PassNumber N) {
    errs() << "extra verifier shall be run after <" << PI->getPassArgument()
           << N.str() << ">\n";
    return createVerifierPass();
  }
};
PassSetOption ExtraVerificationAfterPass::option{
    "vc-run-verifier-after-pass",
    cl::desc("Debug only. Run verifier after the specified pass.")};

static PassSetOption DisablePass{
    "vc-disable-pass", cl::desc("Debug only. Do not add the specified pass")};

bool AllOptionsAreEmpty() {
  return ExtraIRDumpBeforePass::option.empty() &&
         ExtraIRDumpAfterPass::option.empty() &&
         ExtraVerificationBeforePass::option.empty() &&
         ExtraVerificationAfterPass::option.empty() && DisablePass.empty();
}
} // namespace

template <typename PMOption>
void VCPassManager::addExtraPass(const PassInfo *CurrentPI) {
  auto res = PMOption::option.getValue().includes(CurrentPI->getPassArgument());
  if (res.first)
    legacy::PassManager::add(PMOption::createPass(CurrentPI, res.second));
}

void VCPassManager::add(Pass *P) {
  IGC_ASSERT(P);

  if (AllOptionsAreEmpty())
    return legacy::PassManager::add(P);

  const auto *PassInfo = Pass::lookupPassInfo(P->getPassID());
  if (!PassInfo) {
#ifndef NDEBUG
    llvm::errs() << "WARNING: LLVM could not find PassInfo for the <"
                 << P->getPassName() << "> pass! Extra passes "
                 << "won't be injected.\n";
#endif // NDEBUG
    return legacy::PassManager::add(P);
  }
  // Extra passes are inserted independent on whether or not the pass is ommited
  // to preserve numbering of passes inside them (if '*' is passed to
  // -vc-dump-...-pass, and one occurence of a pass is ommited, the numbering of
  // others would shift if addExtraPasses would not be called).
  addExtraPass<ExtraIRDumpBeforePass>(PassInfo);
  addExtraPass<ExtraVerificationBeforePass>(PassInfo);

  auto PassArg = PassInfo->getPassArgument();
  auto res = DisablePass.getValue().includes(PassArg);
  if (res.first)
    errs() << "Pass " << PassArg << res.second.str() << " is ommited\n";
  else
    legacy::PassManager::add(P);

  addExtraPass<ExtraIRDumpAfterPass>(PassInfo);
  addExtraPass<ExtraVerificationAfterPass>(PassInfo);
}
