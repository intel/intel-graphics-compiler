/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/LLVMContext.h>
#include "BiFManagerTool.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace std;
using namespace llvm;

// BiFManager-bin -symbols -main <list> -size32 <list> -size64 <list> -o <symbols.json>
//   lists the symbols of the modules for depgraph.py
// BiFManager-bin -pack <modules> -shared-globals <list> -o <package.bifbc>
//   packs the modules, in the order of depgraph.py
// A list file names one module or global variable per line.
static cl::opt<bool> Symbols("symbols", cl::desc("List the symbols of the modules"));
static cl::opt<std::string> MainListFilename("main", cl::desc("File listing the modules for both pointer sizes"));
static cl::opt<std::string> Size32ListFilename("size32", cl::desc("File listing the modules for pointer size 32"));
static cl::opt<std::string> Size64ListFilename("size64", cl::desc("File listing the modules for pointer size 64"));
static cl::opt<std::string> PackListFilename("pack", cl::desc("File listing the modules to pack, in package order"));
static cl::opt<std::string> SharedGlobalsFilename("shared-globals",
                                                  cl::desc("File listing the global variables used across modules"));
static cl::opt<std::string> OutputPath("o", cl::desc("Output file"), cl::Required);

// Reads the lines of the file, skipping empty ones
static bool readList(const std::string &ListPath, std::vector<std::string> &Lines) {
  std::ifstream List(ListPath);
  if (!List) {
    printf("[BiFManager] - Unable to read %s\n", ListPath.c_str());
    return false;
  }
  std::string Line;
  while (std::getline(List, Line)) {
    if (!Line.empty() && Line.back() == '\r')
      Line.pop_back();
    if (!Line.empty())
      Lines.push_back(Line);
  }
  return true;
}

int main(int argc, char *argv[]) {
  // Print the progress right away, so that the build log shows how far the
  // tool got if it dies.
  setvbuf(stdout, nullptr, _IONBF, 0);

  LLVMContext Context;
  IGCLLVM::setOpaquePointers(&Context);

  cl::ParseCommandLineOptions(argc, argv);

  IGC::BiFManager::BiFManagerTool bif(Context);
  if (Symbols) {
    std::vector<std::string> MainModules, Size32Modules, Size64Modules;
    if (!readList(MainListFilename, MainModules) || !readList(Size32ListFilename, Size32Modules) ||
        !readList(Size64ListFilename, Size64Modules) ||
        !bif.WriteSymbols(MainModules, Size32Modules, Size64Modules, OutputPath)) {
      printf("[BiFManager] - Problem with listing the symbols of the modules\n");
      return -13;
    }
    return 0;
  }

  std::vector<std::string> Modules, Globals;
  if (!readList(PackListFilename, Modules) || !readList(SharedGlobalsFilename, Globals)) {
    return -13;
  }
  StringSet<> SharedGlobals;
  for (const auto &Name : Globals)
    SharedGlobals.insert(Name);
  if (!bif.MakeBiFPackage(Modules, SharedGlobals)) {
    printf("[BiFManager] - Problem with making the BiF package\n");
    return -13;
  }
  bif.WriteBitcode(OutputPath.c_str());
  return 0;
}
