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
#include <vector>

#include <AdaptorOCL/OCL/BuiltinResource.h>
#include <Compiler/Optimizer/BuiltInFuncImport.h>
#include "common/Stats.hpp"

#include "BiFManagerCommon.hpp"

namespace IGC {
namespace BiFManager {
typedef std::function<void(COMPILE_TIME_INTERVALS)> FuncTimer;
typedef std::function<void(llvm::Module &, const llvm::StringSet<> &)> LinkerCallback;

class BiFManagerHandler : public BiFManagerCommon {
public:
  BiFManagerHandler(llvm::LLVMContext &Context);
  ~BiFManagerHandler();

  BiFManagerHandler(const BiFManagerHandler &) = delete;
  BiFManagerHandler &operator=(const BiFManagerHandler &) = delete;

  // Indicator if we have pointer size 32 in user module
  bool isPtrSizeInBits32;

  // Ptr for functions to collect timestamps
  FuncTimer *startTimer;
  FuncTimer *endTimer;

  // Ptr to the stream with all sections of built-in functions
  std::unique_ptr<llvm::MemoryBuffer> BufferData;

  // Collection of the loaded sections of built-in functions mapped by ID
  std::map<BiFSectionID, std::unique_ptr<llvm::Module>> LoadedBiFSections;

  // Extra setup for the linked sections to the user module
  llvm::DataLayout *DL;
  std::string T;
  LinkerCallback CallbackLinker;
  llvm::StringSet<> InternalizeLinkList;

  // Private methods
private:
  // Function which gets the stream with all sections of built-in functions
  static std::unique_ptr<llvm::MemoryBuffer> getBiFModuleBuffer();

  llvm::Module *builtinSizeModule();
  static bool isModulePtrSize32(llvm::Module *pMain);

  // Function looks for all calls of built-in function in user module
  // pModule : User module
  // neededBuiltinInstr [OUT] : List of needed built-in functions
  void findAllBuiltins(llvm::Module *pModule, TFunctionsVec &neededBuiltinInstr);

  // Function loads and preapre built-in function modules for linking
  // pMainModule : User module
  // BuiltInNeeded : List of needed built-in functions
  void preapareBiFSections(llvm::Module &pMainModule, TFunctionsVec &BuiltInNeeded);

  // Function which clean-up the user module with included and not needed
  // function built-ins.
  // Base : User module
  void cleanModule(llvm::Module &Base);

  // Get all the functions called by given function.
  // pFunc : the given function.
  // calledFuncs [OUT] : The list of all functions called by pFunc.
  static void getCalledFunctions(const llvm::Function *pFunc, TFunctionsVec &calledFuncs);

public:
  void SetDataLayout(const llvm::DataLayout &DL);
  void SetTargetTriple(llvm::StringRef T);
  void SetCallbackLinker(const std::function<void(llvm::Module &, const llvm::StringSet<> &)> &CallbackLinker);

  void SetFuncTimers(FuncTimer *startTimer, FuncTimer *endTimer) {
    this->startTimer = startTimer;
    this->endTimer = endTimer;
  }

  // Function links all needed built-in function to the user module
  // Module : User module
  void LinkBiF(llvm::Module &Module);

  // Function which checks if the pFunc is a built-in function from BiFModule
  // pFunc : Function to be check
  static bool IsBiF(llvm::Function *pFunc);
  // Function which checks if the pVar is a global variable from built-in function from BiFModule
  // pVar : Global variable to be check
  static bool IsBiF(llvm::GlobalVariable *pVar);
};
} // namespace BiFManager
} // namespace IGC
