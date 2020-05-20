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
// GenXAnalysisDumper is a pass that calls the print() method on a function
// pass to dump its state out to a file.
// GenXGroupAnalysisDumper is the same, but for a function group pass.
//
//===----------------------------------------------------------------------===//

#include "FunctionGroup.h"
#include "GenX.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace genx;

namespace {

// GenXAnalysisDumper : a pass to dump an analysis to a file
class GenXAnalysisDumper : public FunctionPass {
  FunctionPass *P;
  const char *Suffix;
public:
  static char ID;
  explicit GenXAnalysisDumper(FunctionPass *P, const char *Suffix)
    : FunctionPass(ID), P(P), Suffix(Suffix) { }
  virtual StringRef getPassName() const { return "GenX analysis dumper pass"; }
  void getAnalysisUsage(AnalysisUsage &AU) const {
    FunctionPass::getAnalysisUsage(AU);
    AU.setPreservesAll();
  }
  bool runOnFunction(Function &F);
};

// GenXGroupAnalysisDumper : a pass to dump an analysis to a file
class GenXGroupAnalysisDumper : public FunctionGroupPass {
  FunctionGroupPass *P;
  const char *Suffix;
public:
  static char ID;
  explicit GenXGroupAnalysisDumper(FunctionGroupPass *P, const char *Suffix)
    : FunctionGroupPass(ID), P(P), Suffix(Suffix) { }
  virtual StringRef getPassName() const { return "GenX analysis dumper pass"; }
  void getAnalysisUsage(AnalysisUsage &AU) const {
    FunctionGroupPass::getAnalysisUsage(AU);
    AU.setPreservesAll();
  }
  bool runOnFunctionGroup(FunctionGroup &FG);
};

} // end anonymous namespace

char GenXAnalysisDumper::ID = 0;

FunctionPass *llvm::createGenXAnalysisDumperPass(
    FunctionPass *P, const char *Suffix)
{
  return new GenXAnalysisDumper(P, Suffix);
}

char GenXGroupAnalysisDumper::ID = 0;

FunctionGroupPass *llvm::createGenXGroupAnalysisDumperPass(
    FunctionGroupPass *P, const char *Suffix)
{
  return new GenXGroupAnalysisDumper(P, Suffix);
}

/***********************************************************************
 * openFileForDump : open file for dumping analysis into
 *
 * The filename is the name of the kernel, or the name of the function if
 * not a kernel, with the supplied suffix.
 *
 * On error, this function prints an error message and returns -1.
 */
static int openFileForDump(Function *F, StringRef Suffix)
{
  // Get name of kernel, or failing that, name of function.
  KernelMetadata KM(F);
  StringRef Name = KM.getName();
  if (Name.empty())
    Name = F->getName();
  int FD = -1;
  std::string Filename = (Name + Suffix).str();
  // Sanitize templated kernel names.
  std::replace_if(Filename.begin(), Filename.end(),
                  [](const char x) { return x == '<' || x == '>'; }, '_');
  auto EC = sys::fs::openFileForWrite(Filename, FD, sys::fs::CD_CreateAlways, sys::fs::F_None);
  if (EC) {
    errs() << "Error: " << EC.message() << "\n";
    return -1;
  }
  return FD;
}

/***********************************************************************
 * GenXAnalysisDumper::runOnFunction : dump analysis to file
 */
bool GenXAnalysisDumper::runOnFunction(Function &F)
{
  int FD = openFileForDump(&F, Suffix);
  raw_fd_ostream O(FD, /*shouldClose=*/ true);
  P->print(O, F.getParent());
  return false;
}

/***********************************************************************
 * GenXGroupAnalysisDumper::runOnFunctionGroup : dump analysis to file
 */
bool GenXGroupAnalysisDumper::runOnFunctionGroup(FunctionGroup &FG)
{
  int FD = openFileForDump(FG.getHead(), Suffix);
  raw_fd_ostream O(FD, /*shouldClose=*/ true);
  P->print(O, FG.getHead()->getParent());
  return false;
}

