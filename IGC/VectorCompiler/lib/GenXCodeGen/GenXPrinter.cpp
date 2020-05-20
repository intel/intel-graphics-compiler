/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

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
  virtual StringRef getPassName() const { return "GenX printer pass"; }
  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
  }
  bool runOnFunction(Function &F);
};

// GenXGroupPrinter : an analysis to print a FunctionGroup, with GenX specific analyses
class GenXGroupPrinter : public FunctionGroupPass {
  raw_ostream &OS;
  const std::string Banner;
public:
  static char ID;
  explicit GenXGroupPrinter(raw_ostream &OS, const std::string &Banner)
    : FunctionGroupPass(ID), OS(OS), Banner(Banner) { }
  virtual StringRef getPassName() const { return "GenX FunctionGroup printer pass"; }
  void getAnalysisUsage(AnalysisUsage &AU) const {
    FunctionGroupPass::getAnalysisUsage(AU);
    AU.setPreservesAll();
  }
  bool runOnFunctionGroup(FunctionGroup &FG);
};

} // end namespace llvm

char GenXPrinter::ID = 0;

FunctionPass *llvm::createGenXPrinterPass(raw_ostream &O, const std::string &Banner)
{
  return new GenXPrinter(O, Banner);
}

char GenXGroupPrinter::ID = 0;

FunctionGroupPass *llvm::createGenXGroupPrinterPass(raw_ostream &O, const std::string &Banner)
{
  return new GenXGroupPrinter(O, Banner);
}

/***********************************************************************
 * printFunction : print function with GenX analyses
 */
static void printFunction(raw_ostream &OS, Function &F, GenXBaling *Baling,
    GenXLiveness *Liveness, GenXNumbering *Numbering, GenXVisaRegAlloc *RA)
{
  // This code is a downmarket version of AssemblyWriter::printFunction.
  // We have our own version so we can show bales.
  OS << "\ndefine ";
  cast<FunctionType>(cast<PointerType>(F.getType())->getElementType())->getReturnType()->print(OS);
  OS << " @" << F.getName() << "(";
  for (Function::arg_iterator fb = F.arg_begin(), fi = fb, fe = F.arg_end();
      fi != fe; ) {
    if (fi != fb)
      OS << ", ";
    Argument *Arg = &*fi;
    ++fi;
    Arg->getType()->print(OS);
    OS << " ";
    // Only show register number if there is a register allocator.
    GenXVisaRegAlloc::Reg* Reg = nullptr;
    if (RA)
      Reg = RA->getRegForValueOrNull(&F, SimpleValue(Arg));
    if (Reg) {
      OS << "[";
      Reg->print(OS);
      OS << "]";
    }
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
        if (RA && !Inst->getType()->isVoidTy()) {
          // Show allocated register in brackets. If it is struct type,
          // we show the multiple registers. For an alias, show its base
          // register in braces as well.
          for (unsigned i = 0,
              e = IndexFlattener::getNumElements(Inst->getType());
              i != e; ++i) {
            auto Reg = RA->getRegForValueOrNull(&F, SimpleValue(Inst, i));
            if (Reg && Reg->Category) {
              OS << (!i ? "[" : ",");
              Reg->print(OS);
              auto BaseReg = RA->getRegForValueUntyped(&F, SimpleValue(Inst, i));
              if (BaseReg != Reg) {
                OS << "{";
                assert(BaseReg);
                BaseReg->print(OS);
                OS << "}";
              }
              if (i + 1 == e)
                OS << "]";
            }
          }
        }
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

/***********************************************************************
 * GenXPrinter::runOnFunction : dump function with GenX analyses
 */
bool GenXPrinter::runOnFunction(Function &F)
{
  GenXVisaRegAlloc *RA = getAnalysisIfAvailable<GenXVisaRegAlloc>();
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  if (!RA) {
    Liveness = getAnalysisIfAvailable<GenXLiveness>();
    Numbering = getAnalysisIfAvailable<GenXNumbering>();
  }
  GenXBaling *Baling = getAnalysisIfAvailable<GenXFuncBaling>();
  OS << Banner;
  printFunction(OS, F, Baling, Liveness, Numbering, RA);
  return false;
}

/***********************************************************************
 * GenXGroupPrinter::runOnFunctionGroup : dump functions with GenX analyses
 */
bool GenXGroupPrinter::runOnFunctionGroup(FunctionGroup &FG)
{
  GenXVisaRegAlloc *RA = getAnalysisIfAvailable<GenXVisaRegAlloc>();
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  if (!RA) {
    Liveness = getAnalysisIfAvailable<GenXLiveness>();
    Numbering = getAnalysisIfAvailable<GenXNumbering>();
  }
  GenXBaling *Baling = getAnalysisIfAvailable<GenXGroupBaling>();
  if (!Baling)
    Baling = getAnalysisIfAvailable<GenXFuncBaling>();
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

