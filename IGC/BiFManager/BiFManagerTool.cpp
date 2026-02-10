/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/SplitModule.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/TargetParser/Triple.h"
#include "llvmWrapper/Transforms/IPO/GlobalDCE.h"
#include "llvmWrapper/Transforms/IPO/StripSymbols.h"
#include "llvmWrapper/Transforms/IPO/StripDeadPrototypes.h"
#include "BiFManagerTool.hpp"

#include <string>
#include <list>

#include <Probe/Assertion.h>
#include "inc/common/secure_string.h"

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace IGC::BiFManager;

BiFManagerTool::BiFManagerTool(LLVMContext &Context)
    : BiFManagerCommon(Context), Bif32MaxDep(-1), Bif64MaxDep(-1), Bif32Idx(-1), Bif64Idx(-1), sectionCounter(-1) {}

BiFManagerTool::~BiFManagerTool() {}

void BiFManagerTool::writeHashMapSingle(llvm::raw_fd_ostream &fileDataHeader, BiFDictionary *ListOfFunctions,
                                        std::vector<std::string> &listFunc, int MaxDependencyList, int SizeType) {
  size_t funcNameSize = listFunc[0].size();

  std::string hashMap = "\nconst std::array<int, " + std::to_string(MaxDependencyList) + "> GetDepList" +
                        std::to_string(SizeType) + "_S" + std::to_string(funcNameSize) +
                        "(size_t FuncIDHash)\n{\n    switch (FuncIDHash)\n    {";
  fileDataHeader.write(hashMap.c_str(), hashMap.size());

  std::string hashType = sizeof(size_t) == 4 ? "UL" : "ULL";

  for (auto rec_i = listFunc.begin(); rec_i != listFunc.end();) {
    std::string funcName = *rec_i;
    std::string funcKey;
    funcKey = funcName;
    auto listOfDependency = ListOfFunctions->operator[](funcKey);

    std::string record = "\n        // bif func: " + funcName;
    record += "\n        case ";
    record += std::to_string(getHash(funcName)) + hashType;
    record += ": { return std::array<int, " + std::to_string(MaxDependencyList) + ">{ ";

    for (size_t i = 0; i < listOfDependency.size(); ++i) {
      record += std::to_string(listOfDependency[i]);

      if (i + 1 != listOfDependency.size()) {
        record += ", ";
      }
    }
    record += " }; }";

    ++rec_i;

    fileDataHeader.write(record.c_str(), record.size());
  }

  hashMap = "\n    }\n    return std::array<int, " + std::to_string(MaxDependencyList) + ">{ -1 };\n}";
  fileDataHeader.write(hashMap.c_str(), hashMap.size());
}

void BiFManagerTool::writeHashMap(llvm::raw_fd_ostream &fileDataHeader, BiFDictionary *ListOfFunctions,
                                  int MaxDependencyList, int SizeType) {
  std::map<size_t, std::vector<std::string>> sortBySizeNameFunc;

  for (auto rec_i = ListOfFunctions->begin(); rec_i != ListOfFunctions->end(); ++rec_i) {
    const auto &record_inst = *rec_i;
    const std::string &funcName =
        record_inst.first;
    auto funcNameSize = funcName.size();

    if (sortBySizeNameFunc.find(funcNameSize) == sortBySizeNameFunc.end()) {
      sortBySizeNameFunc.insert({funcNameSize, {funcName}});
    } else {
      sortBySizeNameFunc[funcNameSize].push_back(funcName);
    }
  }

  for (auto rec_i = sortBySizeNameFunc.begin(); rec_i != sortBySizeNameFunc.end(); ++rec_i) {
    writeHashMapSingle(fileDataHeader, ListOfFunctions, rec_i->second, MaxDependencyList, SizeType);
  }

  std::string hashMap = "\nconst std::array<int, " + std::to_string(MaxDependencyList) + "> GetDepList" +
                        std::to_string(SizeType) + "(std::string& FuncName)\n{\n    switch (FuncName.size())\n    {";

  fileDataHeader.write(hashMap.c_str(), hashMap.size());

  for (auto rec_i = sortBySizeNameFunc.begin(); rec_i != sortBySizeNameFunc.end(); ++rec_i) {
    size_t funcNameSize = rec_i->first;
    std::string record = "\n        // bif func name size : " + std::to_string(funcNameSize);
    record += "\n        case ";
    record += std::to_string(funcNameSize);
    record += ": { return GetDepList" + std::to_string(SizeType) + "_S" + std::to_string(funcNameSize) +
              "(BiFManagerCommon::getHash(FuncName)); }";

    fileDataHeader.write(record.c_str(), record.size());
  }

  hashMap = "\n    }\n    return std::array<int, " + std::to_string(MaxDependencyList) + ">{ -1 };\n}";
  fileDataHeader.write(hashMap.c_str(), hashMap.size());
}

void BiFManagerTool::WriteBitcode(const llvm::StringRef BitCodePath) {
  std::error_code EC;
  long currentBufferOffset = 0;
  raw_fd_ostream fileData(BitCodePath.str(), EC, sys::fs::FA_Write);
  std::vector<SmallVector<char, 0> *> listOfStreams;
  std::vector<BiFDataRecord *> listOfFilePtr;
  // Write module into stream

  printf("[BiFManager] - Start writing bitcode for sections\n");
  for (auto m_data_i = BiFSections.begin(); m_data_i != BiFSections.end(); ++m_data_i) {
    auto sectionModule = m_data_i->first;
    auto pModule = m_data_i->second.get();
    {
      SmallVector<char, 0> *Buffer = new SmallVector<char, 0>();

      Buffer->reserve(512 * 1024);

      BitcodeWriter Writer(*Buffer);

      Writer.writeModule(*pModule, false, nullptr, false, nullptr);
      Writer.writeSymtab();
      Writer.writeStrtab();

      listOfStreams.push_back(Buffer);

      listOfFilePtr.push_back(new BiFDataRecord(sectionModule, currentBufferOffset, Buffer->size()));
      currentBufferOffset += Buffer->size();
    }
  }

  printf("[BiFManager] - End of writing bitcode for sections\n");

  // Entry record keeps offset where first module starts in buffor
  BiFDataRecord entryRecord(-1, 0, sizeof(BiFDataRecord) * (BiFSections.size() + 1));

  // Write entry record
  fileData.write((char *)&entryRecord, sizeof(BiFDataRecord));

  for (unsigned i = 0; i < BiFSections.size(); ++i) {
    // Write records for ptr module in buffer
    fileData.write((char *)listOfFilePtr[i], sizeof(BiFDataRecord));
  }

  for (unsigned i = 0; i < BiFSections.size(); ++i) {
    // Write modules into stream
    fileData.write((char *)&listOfStreams[i]->front(), listOfStreams[i]->size());
  }

  printf("[BiFManager] - End of writing bifbc bitcode\n");

  for (unsigned i = 0; i < BiFSections.size(); ++i) {
    delete listOfStreams[i];
    delete listOfFilePtr[i];
  }
}

void BiFManagerTool::WriteHeader(const llvm::StringRef HeaderPath) {
  std::error_code EC;

  raw_fd_ostream fileDataHeader(HeaderPath.str(), EC, sys::fs::FA_Write);

  std::string moduleIdx = "const int Module32Idx = " + std::to_string(Bif32Idx) +
                          ";\nconst int Module64Idx = " + std::to_string(Bif64Idx) + ";\n";
  fileDataHeader.write(moduleIdx.c_str(), moduleIdx.size());

  printf("[BiFManager] - Start writing header for bifbc\n");

  writeHashMap(fileDataHeader, &Bif32, Bif32MaxDep, 32);
  printf("[BiFManager] - Done for BiF32 header for bifbc\n");

  std::string newline = "\n";
  fileDataHeader.write(newline.c_str(), newline.size());

  writeHashMap(fileDataHeader, &Bif64, Bif64MaxDep, 64);
  printf("[BiFManager] - Done for BiF32 header for bifbc\n");
}

void BiFManagerTool::prepareDependencies(BiFDictionary &BiFMapBitType, int BiFMainMaxIdx, int BiFSizeIdxStart,
                                         int BiFSizeIdxEnd) {
  for (auto m_data_i = BiFSections.begin(); m_data_i != BiFSections.end();) {
    auto sectionModule = m_data_i->first;

    if (
        // Builtin function belongs to the regular BiF group
        sectionModule <= BiFMainMaxIdx ||
        // Builtin function belongs to the Size[32|64] BiF group
        (sectionModule >= BiFSizeIdxStart && sectionModule <= BiFSizeIdxEnd)) {
      auto funcInSection = FuncPerSections[sectionModule];

      for (auto f_i = funcInSection.begin(); f_i != funcInSection.end(); ++f_i) {
        auto funcInModule = *f_i;

        if (BiFMapBitType.find(funcInModule->getName().str()) == BiFMapBitType.end()) {
          BiFMapBitType.insert({funcInModule->getName().str(), {sectionModule}});
        }
      }
    }
    ++m_data_i;
  }
}

int BiFManagerTool::findDependency(BiFDictionary &BiFMapBitType) {
  size_t MaxDependencyList = 0;

  llvm::SmallPtrSet<llvm::Function *, 16> visitedFuncs;

  function<void(llvm::Function *, std::vector<BiFSectionID> *)> collectDepModules =
      [&](llvm::Function *pFunc, std::vector<BiFSectionID> *pDepList) {
        if (visitedFuncs.count(pFunc)) {
          // To avoid infinite recursive call of collectDepModules
          return;
        }
        visitedFuncs.insert(pFunc);

        std::vector<llvm::Function *> neededBuiltinInstr;

        // Find all builtins function calls inside pFunc
        findAllBuiltins(pFunc, neededBuiltinInstr, &BiFMapBitType);

        for (auto func_dep_i : neededBuiltinInstr) {
          // Get the Module index which contains the needed builtin function
          auto depModuleIdx = BiFMapBitType[func_dep_i->getName().str()][0];

          // Do not add if we have already in the dependcy list of current
          // func_i function
          if (std::find(pDepList->begin(), pDepList->end(), depModuleIdx) == pDepList->end()) {
            // Add dependency module section needed by this function
            pDepList->push_back(depModuleIdx);
          }

          llvm::Module *moduleSection = BiFSections[depModuleIdx].get();

          auto func = moduleSection->getFunction(func_dep_i->getName());

          collectDepModules(func, pDepList);
        }
      };

  for (auto func_i = BiFMapBitType.begin(); func_i != BiFMapBitType.end(); ++func_i) {
    auto bifSectionIdx = func_i->second[0];
    llvm::Module *moduleSection = BiFSections[bifSectionIdx].get();

    auto func = moduleSection->getFunction(func_i->first);
    auto func_dep_list = &func_i->second;
    visitedFuncs.clear();
    collectDepModules(func, func_dep_list);

    if (func_dep_list->size() > MaxDependencyList) {
      MaxDependencyList = func_dep_list->size();
    }
  }

  return MaxDependencyList;
}

void BiFManagerTool::markBuiltinFunc(llvm::Module *pM) {
  for (auto &f : pM->functions()) {
    if (!f.isDeclaration()) {
      LLVMContext &C = f.getContext();
      MDNode *N = MDNode::get(C, MDString::get(C, "IGC built-in function"));

      f.setMetadata(bifMark, N);
    }
  }

  for (auto &g : pM->globals()) {
    if (!g.isDeclaration()) {
      LLVMContext &C = g.getContext();
      MDNode *N = MDNode::get(C, MDString::get(C, "IGC global variable from built-in function"));

      g.setMetadata(bifMark, N);
    }
  }
}

void BiFManagerTool::MakeBiFPackage(llvm::Module *pBiFModuleMain, llvm::Module *pBiFModuleSize32,
                                    llvm::Module *pBiFModuleSize64) {
  sectionCounter = -1;

  generateSplitedBiFModules(pBiFModuleMain);
  int BiFMainEndIdx = BiFSections.size() - 1;
  printf("[BiFManager] - End of spliting Main BiF module (generated %d sections)\n", (int)BiFSections.size());
  generateSplitedBiFModules(pBiFModuleSize32);
  int BiFSize32EndIdx = BiFSections.size() - 1;
  printf("[BiFManager] - End of spliting Size32 BiF module (generated %d sections)\n",
         (int)(BiFSections.size() - (BiFMainEndIdx + 1)));
  generateSplitedBiFModules(pBiFModuleSize64);
  int BiFSize64EndIdx = BiFSections.size() - 1;
  printf("[BiFManager] - End of spliting Size64 BiF module (generated %d sections)\n",
         (int)(BiFSections.size() - (BiFSize32EndIdx + 1)));

  printf("[BiFManager] - Prepare Bif32 dictonary dependency\n");
  prepareDependencies(Bif32, BiFMainEndIdx, BiFMainEndIdx + 1, BiFSize32EndIdx);
  printf("[BiFManager] - Prepare Bif64 dictonary dependency\n");
  prepareDependencies(Bif64, BiFMainEndIdx, BiFSize32EndIdx + 1, BiFSize64EndIdx);

  printf("[BiFManager] - Find in BiF32 function dependency\n");
  Bif32MaxDep = findDependency(Bif32);
  printf("[BiFManager] - Find in BiF64 function dependency\n");
  Bif64MaxDep = findDependency(Bif64);

  // Setup LinkOnceAnyLinkage for globals and functions
  for (auto &module_i : BiFSections) {
    llvm::Module *md = module_i.second.get();

    // Mark all built-in functions and global variables
    markBuiltinFunc(md);

    for (auto &func_i : md->functions()) {
      func_i.setLinkage(GlobalValue::LinkOnceAnyLinkage);
    }
    for (auto &global_i : md->globals()) {
      global_i.setLinkage(GlobalValue::LinkOnceAnyLinkage);
    }
  }

  Bif32Idx = BiFMainEndIdx + 1;
  Bif64Idx = BiFSize32EndIdx + 1;
}

void BiFManagerTool::splitBiFModules(llvm::Module *pMainModule) {
  sectionCounter++;
  // Split all function in module per sections
  for (Function &func : pMainModule->functions()) {
    if (func.isDeclaration()) {
      // not real built-in function
      continue;
    }

    FuncPerSections[sectionCounter].push_back(&func);

    if (FuncPerSections[sectionCounter].size() == MaxFuncInSection) {
      sectionCounter++;
    }
  }
}

void BiFManagerTool::generateSplitedBiFModules(llvm::Module *pMainModule) {
  splitBiFModules(pMainModule);

  // Per section do...
  for (const auto &setData : FuncPerSections) {
    if (BiFSections.count(setData.first) > 0) {
      continue;
    }

    SetVector<const GlobalValue *> GVs;

    for (auto func_i = setData.second.begin(); func_i != setData.second.end(); ++func_i) {
      auto func = *func_i;
      // add all functions called by the kernel, recursively
      // start with the kernel...
      GVs.insert(func);
    }

    // add all globals - it's easier to let them be removed later than search for them here
    for (auto &GV : pMainModule->globals()) {
      GVs.insert(&GV);
    }

    // create new module with selected globals and functions
    ValueToValueMapTy VMap;
    std::unique_ptr<Module> kernelM =
        CloneModule(*pMainModule, VMap, [&](const GlobalValue *GV) { return GVs.count(GV); });

    // Do cleanup.
    llvm::legacy::PassManager mpm;
    mpm.add(IGCLLVM::createLegacyWrappedGlobalDCEPass());           // Delete unreachable globals.
    mpm.add(IGCLLVM::createLegacyWrappedStripDeadDebugInfoPass());  // Remove dead debug info.
    mpm.add(IGCLLVM::createLegacyWrappedStripDeadPrototypesPass()); // Remove dead func decls.

    mpm.run(*kernelM.get());

    BiFSections[setData.first] = (std::move(kernelM));
  }
}

void BiFManagerTool::findAllBuiltins(llvm::Function *pFunction, TFunctionsVec &neededBuiltinInstr,
                                     BiFDictionary *BiFMap) {
  std::function<bool(llvm::Function *)> checker = [&](llvm::Function *pFunc) -> bool {
    return BiFMap->count(pFunc->getName().str()) > 0;
  };
  FindAllBuiltins(pFunction, checker, neededBuiltinInstr);
}
