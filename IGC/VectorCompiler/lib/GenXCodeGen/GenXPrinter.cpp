/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// GenXPrinter is a pass that prints the LLVM IR for a function, together
// GenX specific analyses (instruction baling, liveness, register allocation).
//
//===----------------------------------------------------------------------===//

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "GenXNumbering.h"
#include "GenXVisaRegAlloc.h"

#include "vc/Utils/GenX/RegCategory.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

namespace {

// GenXPrinter : an analysis to print a Function, with GenX specific analyses
class GenXPrinter : public FunctionPass {
  raw_ostream &OS;
  const std::string Banner;
public:
  static char ID;
  explicit GenXPrinter(raw_ostream &OS, const std::string &Banner)
    : FunctionPass(ID), OS(OS), Banner(Banner) { }
  StringRef getPassName() const override { return "GenX printer pass"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addUsedIfAvailable<FunctionGroupAnalysis>();
    AU.addUsedIfAvailable<GenXVisaRegAlloc>();
    AU.addUsedIfAvailable<GenXLiveness>();
    AU.addUsedIfAvailable<GenXNumbering>();
    AU.addUsedIfAvailable<GenXFuncBaling>();
    AU.addUsedIfAvailable<GenXGroupBaling>();
    AU.setPreservesAll();
  }
  bool runOnFunction(Function &F) override;
};

// GenXGroupPrinter : an analysis to print Module with all FunctionGroups, with
// GenX specific analyses
class GenXGroupPrinter : public ModulePass {
  raw_ostream &OS;
  const std::string Banner;
public:
  static char ID;
  explicit GenXGroupPrinter(raw_ostream &OS, const std::string &Banner)
      : ModulePass(ID), OS(OS), Banner(Banner) {}
  StringRef getPassName() const override {
    return "GenX FunctionGroup printer pass";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<FunctionGroupAnalysis>();
    AU.addPreserved<FunctionGroupAnalysis>();
    AU.addUsedIfAvailable<GenXVisaRegAlloc>();
    AU.addUsedIfAvailable<GenXLiveness>();
    AU.addUsedIfAvailable<GenXNumbering>();
    AU.addUsedIfAvailable<GenXGroupBaling>();
    AU.setPreservesAll();
  }
  bool runOnModule(Module &M) override {
    bool Changed = false;
    FunctionGroupAnalysis &FGA = getAnalysis<FunctionGroupAnalysis>();
    for (auto *FunctionGroup : FGA.AllGroups())
      Changed |= runOnFunctionGroup(*FunctionGroup);
    return Changed;
  }
  bool runOnFunctionGroup(FunctionGroup &FG);
};

} // end namespace llvm

char GenXPrinter::ID = 0;

FunctionPass *llvm::createGenXPrinterPass(raw_ostream &O, const std::string &Banner)
{
  FunctionPass *Created = new GenXPrinter(O, Banner);
  IGC_ASSERT(Created);
  return Created;
}

char GenXGroupPrinter::ID = 0;

ModulePass *llvm::createGenXGroupPrinterPass(raw_ostream &O,
                                             const std::string &Banner) {
  ModulePass *Created = new GenXGroupPrinter(O, Banner);
  IGC_ASSERT(Created);
  return Created;
}

// If possible, print register info and return true.
// Otherwise do nothing and return false.
static bool tryPrintRegInfo(raw_ostream &OS, SimpleValue SV,
                            GenXVisaRegAlloc *RA) {
  if (!RA)
    return false;
  auto *Reg = RA->getRegForValueUntyped(SV);
  if (!Reg)
    return false;
  Reg->print(OS);
  // FIXME: currently it is impossible to print base reg for alias.
  // Visa regalloc does not store aliases for simple values, they
  // are created during cisa builder without actual mapping update.
  // To fix this, we should either change IR to actually represent
  // register aliasing (partially implemented by bitcasts) or change
  // regalloc API to store map of Use to Reg.
  // First option is preferrable because it allows cisa builder to
  // work without modification of regalloc maps.
  return true;
}

// If possible, print category info and return true.
// Otherwise do nothing and return false.
static bool tryPrintCategoryInfo(raw_ostream &OS, SimpleValue SV,
                                 GenXLiveness *Liveness) {
  if (!Liveness)
    return false;

  auto *LR = Liveness->getLiveRangeOrNull(SV);
  if (!LR || LR->Category == vc::RegCategory::None)
    return false;

  OS << vc::getRegCategoryShortName(LR->Category);
  return true;
}

// Print additional info for simple value.
// If there is allocated register then print it.
// Otherwise if category is assigned then print it.
// Otherwise print "-".
static void printPropertiesForSimpleValue(raw_ostream &OS, SimpleValue SV,
                                          GenXLiveness *Liveness,
                                          GenXVisaRegAlloc *RA) {
  if (tryPrintRegInfo(OS, SV, RA))
    return;

  if (tryPrintCategoryInfo(OS, SV, Liveness))
    return;

  OS << "-";
}

// Show allocated register or category (if any) in brackets. If it is
// a struct type, then show info for each simple value.
static void printPropertiesForValue(raw_ostream &OS, Value &V,
                                    GenXLiveness *Liveness,
                                    GenXVisaRegAlloc *RA) {
  // No info available.
  if (!RA && !Liveness)
    return;

  const unsigned SVNum = IndexFlattener::getNumElements(V.getType());
  // Void or empty struct.
  if (SVNum == 0)
    return;

  OS << "[";
  printPropertiesForSimpleValue(OS, {&V, 0}, Liveness, RA);
  for (unsigned I = 1; I != SVNum; ++I) {
    OS << ",";
    printPropertiesForSimpleValue(OS, {&V, I}, Liveness, RA);
  }
  OS << "]";
}

/***********************************************************************
 * printFunction : print function with GenX analyses
 */
static void printFunction(raw_ostream &OS, Function &F, GenXBaling *Baling,
                          GenXLiveness *Liveness, GenXNumbering *Numbering,
                          GenXVisaRegAlloc *RA) {
  // This code is a downmarket version of AssemblyWriter::printFunction.
  // We have our own version so we can show bales.
  OS << "\ndefine ";
  F.getReturnType()->print(OS, /*IsForDebug=*/false, /*NoDetails=*/true);
  OS << " @" << F.getName() << "(";
  for (Function::arg_iterator fb = F.arg_begin(), fi = fb, fe = F.arg_end();
      fi != fe; ) {
    if (fi != fb)
      OS << ", ";
    Argument *Arg = &*fi;
    ++fi;
    Arg->getType()->print(OS, /*IsForDebug=*/false, /*NoDetails=*/true);
    OS << " ";
    printPropertiesForValue(OS, *Arg, Liveness, RA);
    OS << "%" << Arg->getName();
  }
  OS << ") {\n";
  for (Function::iterator fi = F.begin(), fe = F.end(); fi != fe; ++fi) {
    BasicBlock *BB = &*fi;
    if (!BB->use_empty())
      OS << BB->getName() << ":\n";
    for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
      Instruction *Inst = &*bi;
      if (!Baling || !Baling->isBaled(Inst)) {
        printPropertiesForValue(OS, *Inst, Liveness, RA);
        // Show instruction number in brackets.
        unsigned Num = 0;
        if (Numbering)
          Num = Numbering->getNumber(Inst);
        if (Num)
          OS << "[" << Num << "]";
        if (!Baling) {
          Inst->print(OS);
          OS << "\n";
        } else {
          Bale B;
          Baling->buildBale(Inst, &B);
          if (B.size() == 1) {
            Inst->print(OS);
            OS << "\n";
          } else {
            OS << "  bale {\n";
            for (Bale::iterator i = B.begin(),
                e = B.end(); i != e; ++i) {
              unsigned Num = 0;
              if (Numbering)
                Num = Numbering->getNumber(i->Inst);
              if (Num)
                OS << "[" << Num << "]";
              OS << "   ";
              i->Inst->print(OS);
              switch (i->Info.Type) {
                case BaleInfo::MAININST: break;
                default: OS << " {" << i->Info.getTypeString() << "}"; break;
              }
              OS << "\n";
            }
            if (Num)
              OS << "[" << Num << "]";
            OS << "  }\n";
          }
        }
      }
    }
  }
  OS << "}\n";
}

// Helper to get FG analysis for given FG if it is available.
template <typename Analysis>
static Analysis *getAnalysisForFGIfAvailable(Pass &P, FunctionGroup &FG) {
  using FGWrapperTy = FunctionGroupWrapperPass<Analysis>;
  if (auto *Wrapper = P.getAnalysisIfAvailable<FGWrapperTy>())
    return &Wrapper->getFGPassImpl(&FG);
  return nullptr;
}

// Dump function with GenX analyses.
bool GenXPrinter::runOnFunction(Function &F) {
  auto *FGA = getAnalysisIfAvailable<FunctionGroupAnalysis>();
  GenXBaling *Baling = getAnalysisIfAvailable<GenXFuncBaling>();
  GenXVisaRegAlloc *RA = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  if (FGA) {
    auto *FG = FGA->getAnyGroup(&F);
    RA = getAnalysisForFGIfAvailable<GenXVisaRegAlloc>(*this, *FG);
    Numbering = getAnalysisForFGIfAvailable<GenXNumbering>(*this, *FG);
    Liveness = getAnalysisForFGIfAvailable<GenXLiveness>(*this, *FG);
    Baling = getAnalysisForFGIfAvailable<GenXGroupBaling>(*this, *FG);
  }

  OS << Banner;
  printFunction(OS, F, Baling, Liveness, Numbering, RA);
  return false;
}

// Dump function groups with GenX analyses.
bool GenXGroupPrinter::runOnFunctionGroup(FunctionGroup &FG) {
  GenXVisaRegAlloc *RA =
      getAnalysisForFGIfAvailable<GenXVisaRegAlloc>(*this, FG);
  GenXBaling *Baling = getAnalysisForFGIfAvailable<GenXGroupBaling>(*this, FG);

  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  if (!RA) {
    Liveness = getAnalysisForFGIfAvailable<GenXLiveness>(*this, FG);
    Numbering = getAnalysisForFGIfAvailable<GenXNumbering>(*this, FG);
  }
  OS << Banner;
  if (Liveness)
    OS << " (see below for GenXLiveness)";
  for (auto i = FG.begin(), e = FG.end(); i != e; ++i)
    printFunction(OS, **i, Baling, Liveness, Numbering, RA);
  if (Liveness) {
    Liveness->print(OS);
    OS << "\n";
  }
  OS << "\n";
  return false;
}
