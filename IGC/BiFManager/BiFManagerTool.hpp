/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/ADT/StringSet.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

#include "BiFManagerCommon.hpp"

namespace IGC {
namespace BiFManager {
typedef std::map<std::string, std::vector<BiFSectionID>> BiFDictionary;

class BiFManagerTool : public BiFManagerCommon {
public:
  BiFManagerTool(llvm::LLVMContext &Context);
  ~BiFManagerTool();

  // Variables
private:
  // How much we want to have built-in function in one section
  const int MaxFuncInSection = 1000;

  // Dictionary mapping which section is needed for
  // the particular built-in function
  // version for ptrsize32:
  BiFDictionary Bif32;
  // version for ptrsize64:
  BiFDictionary Bif64;

  // Counter for the max how much section needed will be by one built-in function
  int Bif32MaxDep;
  int Bif64MaxDep;

  // The first ID number (in stream) of the section for
  // the built-in function for ptr size 32
  BiFSectionID Bif32Idx;
  // The first ID number (in stream) of the section for
  // the built-in function for ptr size 64
  BiFSectionID Bif64Idx;

  // Current counter of created sections
  int sectionCounter;

  // Collecion of the built-in function in module mapped by the ID
  std::map<BiFSectionID, std::unique_ptr<llvm::Module>> BiFSections;

  // Collecion of the pointers of built-in function mapped by the ID
  std::map<BiFSectionID, TFunctionsVec> FuncPerSections;

  // Private methods
private:
  // Function looks for all calls of built-in function in the function
  // pFunction : built-in function
  // neededBuiltinInstr [OUT] : List of needed built-in functions
  // BiFMap : Mapped built-in function by the name
  void findAllBuiltins(llvm::Function *pFunction, TFunctionsVec &neededBuiltinInstr, BiFDictionary *BiFMap);

  // Function is filling the built-in function dictionary dependency.
  void prepareDependencies(BiFDictionary &BiFMapBitType, int BiFMainMaxIdx, int BiFSizeIdxStart, int BiFSizeIdxEnd);

  // Function looks for each of the built-in function, which sections will be needed
  // to load to have fully working built-in function
  int findDependency(BiFDictionary &BiFMapBitType);

  // Function which splits logically the built-in functions
  void splitBiFModules(llvm::Module *pMainModule);

  // Function generating the splited sections of built-in functions
  void generateSplitedBiFModules(llvm::Module *pMainModule);

  static void writeHashMapSingle(llvm::raw_fd_ostream &fileDataHeader, BiFDictionary *ListOfFunctions,
                                 std::vector<std::string> &listFunc, int MaxDependencyList, int SizeType);

  static void writeHashMap(llvm::raw_fd_ostream &fileDataHeader, BiFDictionary *ListOfFunctions, int MaxDependencyList,
                           int SizeType);

  static void markBuiltinFunc(llvm::Module *pM);

public:
  // Function which is making the bif-stream package
  void MakeBiFPackage(llvm::Module *pBiFModuleMain, llvm::Module *pBiFModuleSize32, llvm::Module *pBiFModuleSize64);

  // Function which writes the bif-stream package
  void WriteBitcode(const llvm::StringRef BitCodePath);
  // Function which writes the header with the mapping in bif-stream package
  void WriteHeader(const llvm::StringRef HeaderPath);
};
} // namespace BiFManager
} // namespace IGC
