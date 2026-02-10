/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Support/PassManager.h"
#include "vc/Support/PassPrinters.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Mutex.h>
#include <llvm/Support/Regex.h>
#include <llvm/Support/raw_ostream.h>

#include "Probe/Assertion.h"

#include <string>
#include <sstream>
#include <utility>

using namespace llvm;

// One can either add extra passes inside vc::PassManager::add or inside
// vc::addPass function. Otherwise pass will be added multiple times.
// This option defines whether addition inside vc::PassManager should take
// place.
static cl::opt<bool> OverridePassManagerOpt(
    "vc-choose-pass-manager-override", cl::init(true), cl::Hidden,
    cl::desc("Take pass manager overrideing over addPass func"));

static cl::opt<bool>
    SplitIRDumps("vc-dump-ir-split", cl::init(false), cl::Hidden,
                 cl::desc("Split IR dumps into separate files"));

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
  PassSetOption(const Mods &...Ms)
      : CLOption(Ms..., cl::ZeroOrMore, cl::CommaSeparated) {}
  auto &getValue() { return CLOption.getValue(); }
  bool empty() { return getValue().empty(); }
};

using OutputStreamHandle = std::unique_ptr<llvm::raw_fd_ostream>;
static OutputStreamHandle createOutputStream(const llvm::Twine &Name) {
  // no error handling since this is debug output
  std::error_code EC;
  auto Result = std::make_unique<llvm::raw_fd_ostream>(Name.str(), EC);
  Result->SetUnbuffered();
  return Result;
}

// global (!!!) variable storing output stream handles for IR printer,
// guarded by mutex
static ManagedStatic<std::vector<OutputStreamHandle>> IRDumpStreams;
static ManagedStatic<sys::SmartMutex<true>> IRDumpsLock;

llvm::raw_fd_ostream &getFileStreamForIRDump(const Twine &Name) {
  sys::SmartScopedLock<true> Writer(*IRDumpsLock);
  OutputStreamHandle OS = createOutputStream(Name);
  IRDumpStreams->push_back(std::move(OS));
  return *IRDumpStreams->back();
}

enum class IRDumpType { Before, After };

llvm::raw_ostream &getOutputStreamForIRDump(IRDumpType DumpType,
                                            StringRef PassArg, PassNumber N) {
  (void)PassArg;
  static int Id = 0;
  std::stringstream ss;
  ss.fill('0');
  ss.width(3);
  ss << Id++;

  // FIXME: this won't work well for online compilations and multi-theaded
  // compilations. We need extra facilities to ensure that created
  // files are unique for every compilation process
  if (!SplitIRDumps)
    return llvm::errs();

  std::string DumpName =
      ss.str() + "_" +
      (((DumpType == IRDumpType::Before) ? Twine("before_") : Twine("after_")) +
       PassArg + N.str() + ".ll")
          .str();
  std::replace_if(
      DumpName.begin(), DumpName.end(),
      [](unsigned char c) { return !std::isalnum(c) && c != '.'; }, '_');
  return getFileStreamForIRDump(DumpName);
}

std::string getIRDumpBanner(StringRef PassName, StringRef PassArg, PassNumber N,
                            IRDumpType Type) {
  (void)PassName;
  return ("; *** " +
          (Type == IRDumpType::Before ? Twine("IR Dump Before ")
                                      : Twine("IR Dump After")) +
          "*** (" + PassArg + N.str() + ")")
      .str();
}

struct ExtraIRDumpBeforePass {
  static PassSetOption option;
  static auto createPass(const PassInfo *PI, PassKind PassKindID,
                         PassNumber N) {
    auto PassArg = PI->getPassArgument();
    return createPrintModulePass(
        getOutputStreamForIRDump(IRDumpType::Before, PassArg, N),
        getIRDumpBanner(PassArg, PassArg, N, IRDumpType::Before));
  }
};
PassSetOption ExtraIRDumpBeforePass::option{
    "vc-dump-ir-before-pass",
    cl::desc("Debug only. Dump IR of the module before the specified pass.")};

struct ExtraVerificationBeforePass {
  static PassSetOption option;
  static auto createPass(const PassInfo *PI, PassKind PassKindID,
                         PassNumber N) {
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
  static auto createPass(const PassInfo *PI, PassKind PassKindID,
                         PassNumber N) {
    auto PassArg = PI->getPassArgument();
    return createPrintModulePass(
        getOutputStreamForIRDump(IRDumpType::After, PassArg, N),
        getIRDumpBanner(PassArg, PassArg, N, IRDumpType::After));
  }
};
PassSetOption ExtraIRDumpAfterPass::option{
    "vc-dump-ir-after-pass",
    cl::desc("Debug only. Dump IR of the module after the specified pass.")};

struct ExtraVerificationAfterPass {
  static PassSetOption option;
  static auto createPass(const PassInfo *PI, PassKind PassKindID,
                         PassNumber N) {
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

struct ExtraPrinterPassAfterPass {
  static PassSetOption option;
  static Pass *createPass(const PassInfo *PI, PassKind PassKindID,
                          PassNumber N) {
    auto PassName = PI->getPassName();
    (void)PassName;
    auto PassArg = PI->getPassArgument();
    (void)PassArg;
    switch (PassKindID) {
    case PT_Region:
      return vc::createRegionPassPrinter(PI, llvm::errs());
    case PT_Loop:
      return vc::createLoopPassPrinter(PI, llvm::errs());
    case PT_Function:
      return vc::createFunctionPassPrinter(PI, llvm::errs());
    case PT_CallGraphSCC:
      return vc::createCallGraphPassPrinter(PI, llvm::errs());
    default:
      return vc::createModulePassPrinter(PI, llvm::errs());
    }
  }
};
PassSetOption ExtraPrinterPassAfterPass::option{
    "vc-analyze", cl::desc("Debug only. Print specified analyses. Behaves like "
                           "-analyze opt option.")};

bool AllOptionsAreEmpty() {
  return ExtraIRDumpBeforePass::option.empty() &&
         ExtraIRDumpAfterPass::option.empty() &&
         ExtraVerificationBeforePass::option.empty() &&
         ExtraVerificationAfterPass::option.empty() &&
         ExtraPrinterPassAfterPass::option.empty() && DisablePass.empty();
}
} // namespace

template <typename PMOption, typename PMT, typename AdderT>
void addExtraPass(PMT &PM, const PassInfo *CurrentPI, PassKind PassKindID,
                  AdderT Adder) {
  auto res = PMOption::option.getValue().includes(CurrentPI->getPassArgument());
  if (res.first)
    Adder(PM, *PMOption::createPass(CurrentPI, PassKindID, res.second));
}

// Adds pass \p P to \p PM plus some additional passes.
// Passes are added by \p Adder functor.
// AdderT: (PMT &, Pass &)
template <typename PMT, typename AdderT>
void addPassImpl(PMT &PM, Pass &P, AdderT Adder) {
  if (AllOptionsAreEmpty())
    return Adder(PM, P);

  const auto *PassInfo = Pass::lookupPassInfo(P.getPassID());
  if (!PassInfo) {
#ifndef NDEBUG
    llvm::errs() << "WARNING: LLVM could not find PassInfo for the <"
                 << P.getPassName() << "> pass! Extra passes "
                 << "won't be injected.\n";
#endif // NDEBUG
    return Adder(PM, P);
  }
  PassKind PassKindID = P.getPassKind();
  // Extra passes are inserted independent on whether or not the pass is ommited
  // to preserve numbering of passes inside them (if '*' is passed to
  // -vc-dump-...-pass, and one occurence of a pass is ommited, the numbering of
  // others would shift if addExtraPasses would not be called).
  addExtraPass<ExtraIRDumpBeforePass>(PM, PassInfo, PassKindID, Adder);
  addExtraPass<ExtraVerificationBeforePass>(PM, PassInfo, PassKindID, Adder);

  auto PassArg = PassInfo->getPassArgument();
  auto res = DisablePass.getValue().includes(PassArg);
  if (res.first)
    errs() << "Pass " << PassArg << res.second.str() << " is ommited\n";
  else
    Adder(PM, P);

  addExtraPass<ExtraIRDumpAfterPass>(PM, PassInfo, PassKindID, Adder);
  addExtraPass<ExtraPrinterPassAfterPass>(PM, PassInfo, PassKindID, Adder);
  addExtraPass<ExtraVerificationAfterPass>(PM, PassInfo, PassKindID, Adder);
}

void vc::PassManager::add(Pass *P) {
  IGC_ASSERT(P);
  auto Adder = [](vc::PassManager &PM, Pass &P) {
    PM.legacy::PassManager::add(&P);
  };
  if (OverridePassManagerOpt.getValue())
    return addPassImpl(*this, *P, Adder);
  Adder(*this, *P);
}

void vc::addPass(legacy::PassManagerBase &PM, Pass *P) {
  IGC_ASSERT(P);
  auto Adder = [](legacy::PassManagerBase &PM, Pass &P) { PM.add(&P); };
  if (OverridePassManagerOpt.getValue())
    return Adder(PM, *P);
  addPassImpl(PM, *P, Adder);
}
