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
#include <llvm/IR/GlobalValue.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/TargetParser/Triple.h"
#include "BiFManagerHandler.hpp"

#include <string>
#include <list>
#include <AdaptorOCL/OCL/BuiltinResource.h>
#include <AdaptorOCL/OCL/LoadBuffer.h>
#include <common/BuiltinTypes.h>

#include <Probe/Assertion.h>
#include "inc/common/secure_string.h"

#define BIF_COMPILER_TIME_START(TYPE)                                                                                  \
  if (startTimer) {                                                                                                    \
    (*startTimer)(TYPE);                                                                                               \
  }

#define BIF_COMPILER_TIME_END(TYPE)                                                                                    \
  if (endTimer) {                                                                                                      \
    (*endTimer)(TYPE);                                                                                                 \
  }

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace IGC::BiFManager;

#include <OCLBiFImpl.h>

BiFManagerHandler::BiFManagerHandler(LLVMContext &Context)
    : BiFManagerCommon(Context), isPtrSizeInBits32(false), startTimer(nullptr), endTimer(nullptr), DL(nullptr), T(""),
      CallbackLinker(nullptr) {
  // OCL builtin types, such as clk_event_t/queue_t, etc., are struct (opaque) types. For
  // those types, its original names are themselves; the derived names are ones with
  // '.<digit>' appended to the original names. For example,  clk_event_t is the original
  // name, its derived names are clk_event_t.0, clk_event_t.1, etc.
  //
  // When llvm reads in multiple modules, say, M0, M1, under the same llvmcontext, if both
  // M0 and M1 has the same struct type,  M0 will have the original name and M1 the derived
  // name for that type.  For example, clk_event_t,  M0 will have clk_event_t, while M1 will
  // have clk_event_t.2 (number is arbitary). After linking, those two named types should be
  // mapped to the same type, otherwise, we could have type-mismatch (for example, OCL GAS
  // builtin_functions tests will assertion fail during inlining due to type-mismatch).  Furthermore,
  // when linking M1 into M0 (M0 : dstModule, M1 : srcModule), the final type is the type
  // used in M0.

  BufferData = getBiFModuleBuffer();

  if (BufferData.get() == nullptr) {
    IGC_ASSERT_EXIT_MESSAGE(0, "Error with loading the BiFModule ress");
  }
}

BiFManagerHandler::~BiFManagerHandler() {}

void BiFManagerHandler::LinkBiF(llvm::Module &Module) {
  BIF_COMPILER_TIME_START(TIME_OCL_BiFMgr_TOTAL);
  isPtrSizeInBits32 = isModulePtrSize32(&Module);

  llvm::Linker ld(Module);

  TFunctionsVec neededBuiltinFunc;

  BIF_COMPILER_TIME_START(TIME_OCL_BiFMgr_FindAllBuiltins);
  // Find all built-in functions which we have in user module
  findAllBuiltins(&Module, neededBuiltinFunc);
  BIF_COMPILER_TIME_END(TIME_OCL_BiFMgr_FindAllBuiltins);

  BIF_COMPILER_TIME_START(TIME_OCL_BiFMgr_PreapareBiFSections);
  // Load and prepare needed BiF modules for linking
  preapareBiFSections(Module, neededBuiltinFunc);
  BIF_COMPILER_TIME_END(TIME_OCL_BiFMgr_PreapareBiFSections);

  BIF_COMPILER_TIME_START(TIME_OCL_BiFMgr_LinkAllSections);

  // Link all BiF modules into shader module
  for (auto bifsection_i = LoadedBiFSections.begin(); bifsection_i != LoadedBiFSections.end(); ++bifsection_i) {
    BiFSectionID SeletectedID = bifsection_i->first;
    std::unique_ptr<llvm::Module> *BiFSection = &LoadedBiFSections[SeletectedID];

    if (BiFSection->get() == nullptr) {
      continue;
    }

    if (Error err = BiFSection->get()->materializeAll()) {
      IGC_ASSERT_MESSAGE(0, "materializeAll failed for generic builtin module");
    }

    if (CallbackLinker == nullptr) {
      BiFSection->get()->setDataLayout(Module.getDataLayout());
      BiFSection->get()->setTargetTriple(Module.getTargetTriple());
      if (ld.linkInModule(std::move(*BiFSection), llvm::Linker::OverrideFromSrc)) {
        IGC_ASSERT_MESSAGE(0, "Error linking generic builtin module");
      }

      bool isLastItem = std::next(bifsection_i) == LoadedBiFSections.end();
      if (isLastItem) {
        for (auto &f : Module.functions()) {
          if (IsBiF(&f)) {
            f.setLinkage(GlobalValue::ExternalLinkage);
          }
        }

        for (auto &g : Module.globals()) {
          if (IsBiF(&g)) {
            g.setLinkage(GlobalValue::ExternalLinkage);
          }
        }
      }
    } else {
      const LinkerCallback &collectInternalizeLinkList = [&](llvm::Module &M, const StringSet<> &GVS) {
        for (auto &GVS_i : GVS)
          InternalizeLinkList.insert(GVS_i.getKey());
      };

      if (ld.linkInModule(std::move(*BiFSection), llvm::Linker::OverrideFromSrc, collectInternalizeLinkList)) {
        IGC_ASSERT_MESSAGE(0, "Error linking generic builtin module");
      }

      bool isLastItem = std::next(bifsection_i) == LoadedBiFSections.end();
      if (isLastItem) {
        CallbackLinker(Module, InternalizeLinkList);
      }
    }
  }

  BIF_COMPILER_TIME_END(TIME_OCL_BiFMgr_LinkAllSections);

  BIF_COMPILER_TIME_END(TIME_OCL_BiFMgr_TOTAL);
}

bool BiFManagerHandler::IsBiF(llvm::Function *pFunc) { return pFunc->getMetadata(bifMark) != nullptr; }

bool BiFManagerHandler::IsBiF(llvm::GlobalVariable *pVar) { return pVar->getMetadata(bifMark) != nullptr; }

void BiFManagerHandler::SetCallbackLinker(
    const std::function<void(llvm::Module &, const llvm::StringSet<> &)> &CallbackLinker) {
  this->CallbackLinker = CallbackLinker;
}

void BiFManagerHandler::findAllBuiltins(llvm::Module *pModule, TFunctionsVec &neededBuiltinInstr) {
  std::function<bool(llvm::Function *)> predicate = [&](llvm::Function *pFunc) -> bool {
    auto funcName = pFunc->getName().str();

    return (isPtrSizeInBits32 ? GetDepList32(funcName) : GetDepList64(funcName))[0] > -1;
  };

  FindAllBuiltins(pModule, predicate, neededBuiltinInstr);
}

void BiFManagerHandler::preapareBiFSections(llvm::Module &pMainModule, TFunctionsVec &BuiltInNeeded) {
  const char *beginOfStream = this->BufferData.get()->getBufferStart();

  // On begining of stream we keep entry record which has pointer to begin
  // of modules
  BiFDataRecord *entryRecord = (BiFDataRecord *)(beginOfStream);

  // Exclude the begin ptr of modules
  const char *entryStreamForModules = (beginOfStream + entryRecord->bufferSize);

  std::map<BiFSectionID, BiFDataRecord *> neededModules;

  // Collect all needed Modules which should be prepared for linking
  auto getModulePtr = [&, beginOfStream, entryStreamForModules](BiFSectionID bifIndexSection) {
    BiFDataRecord *recordBifIndex = (BiFDataRecord *)(beginOfStream + (sizeof(BiFDataRecord) * (bifIndexSection + 1)));

    return recordBifIndex;
  };

  for (auto bif_i : BuiltInNeeded) {
    auto funcName = bif_i->getName().str();
    auto deps = isPtrSizeInBits32 ? GetDepList32(funcName) : GetDepList64(funcName);

    for (auto dep_i = deps.begin(); dep_i != deps.end(); ++dep_i) {
      auto bifIndexSection = *dep_i;

      if (neededModules.count(bifIndexSection) > 0) {
        // Allready loaded section
        continue;
      }

      neededModules[bifIndexSection] = getModulePtr(bifIndexSection);
    }
  }

  auto LoadModule = [&](BiFDataRecord *record) {
    // Localize the module in stream
    const char *bifModuleBegin = (entryStreamForModules + record->bufferStart);

    // Attach a MemoryBuffer in this place
    auto bifMemBuffer =
        llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(bifModuleBegin, record->bufferSize), "", false);

    // Read this llvm module
    llvm::Expected<std::unique_ptr<llvm::Module>> ModuleOrErr =
        getLazyBitcodeModule(bifMemBuffer->getMemBufferRef(), Context);

    if (llvm::Error EC = ModuleOrErr.takeError()) {
      std::string error_str = "Error lazily loading bitcode for generic builtins,"
                              "is bitcode the right version and correctly formed?";
      // SetErrorMessage(error_str, *pOutputArgs);
      // return false;
    } else {
      if (this->LoadedBiFSections.count(record->ID) == 0) {
        this->LoadedBiFSections.insert({record->ID, std::move(*ModuleOrErr)});

        llvm::Module &bifGenericSection = *this->LoadedBiFSections[record->ID].get();
#if LLVM_VERSION_MAJOR >= 16
        // LLVM 17+ or patched LLVM 16 Clang generates TargetExtTy to represent OpenCL/SPIR-V builtin types (such as
        // sampler_t). IGC expects these types to be represented using opaque pointers. Hence, here the types are
        // retyped to allow BiF linking.
        // TODO: Consider moving retyping to BiF modules generation stage to improve compilation time.
        retypeOpenCLTargetExtTyAsPointers(&bifGenericSection);
#endif

        if (record->ID < Module32Idx) {
          if (T == "") {
            bifGenericSection.setTargetTriple(builtinSizeModule()->getTargetTriple());
          } else {
            bifGenericSection.setTargetTriple(T);
          }
        }
        if (DL == nullptr) {
          bifGenericSection.setDataLayout(pMainModule.getDataLayout());
        } else {
          bifGenericSection.setDataLayout(*DL);
        }
      }
    }
  };

  BIF_COMPILER_TIME_START(TIME_OCL_BiFMgr_LazyBiFLoading);
  if (isPtrSizeInBits32) {
    auto record = getModulePtr(Module32Idx);
    neededModules[Module32Idx] = record;
    LoadModule(record);
  } else {
    auto record = getModulePtr(Module64Idx);
    neededModules[Module64Idx] = record;
    LoadModule(record);
  }

  // Load and preapre BiF Modules
  for (const auto &bif_record : neededModules) {
    // Localize the module in stream
    BiFDataRecord *record = bif_record.second;
    LoadModule(record);
  }
  BIF_COMPILER_TIME_END(TIME_OCL_BiFMgr_LazyBiFLoading);

  cleanModule(pMainModule);
}

void BiFManagerHandler::getCalledFunctions(const Function *pFunc, TFunctionsVec &calledFuncs) {
  SmallPtrSet<Function *, 8> visitedSet;
  // Iterate over function instructions and look for call instructions
  for (const_inst_iterator it = inst_begin(pFunc), e = inst_end(pFunc); it != e; ++it) {
    const CallInst *pInstCall = dyn_cast<CallInst>(&*it);
    if (!pInstCall)
      continue;
    Function *pCalledFunc = pInstCall->getCalledFunction();
    if (!pCalledFunc) {
      // This case can occur only if CallInst is calling something other than LLVM function.
      // Thus, no need to handle this case - function casting is not allowed (and not expected!)
      continue;
    }
    if (visitedSet.count(pCalledFunc))
      continue;

    visitedSet.insert(pCalledFunc);
    calledFuncs.push_back(pCalledFunc);
  }
}

void BiFManagerHandler::SetDataLayout(const llvm::DataLayout &DL) { this->DL = (llvm::DataLayout *)&DL; }
void BiFManagerHandler::SetTargetTriple(llvm::StringRef T) { this->T = T.str(); }

llvm::Module *BiFManagerHandler::builtinSizeModule() {
  return LoadedBiFSections[isPtrSizeInBits32 ? Module32Idx : Module64Idx].get();
}

std::unique_ptr<llvm::MemoryBuffer> BiFManagerHandler::getBiFModuleBuffer() {
  char Resource[5] = {'-'};
  _snprintf_s(Resource, sizeof(Resource), sizeof(Resource), "#%d", OCL_BIFBC);
  return std::unique_ptr<llvm::MemoryBuffer>{llvm::LoadBufferFromResource(Resource, "BIFBC")};
}

void BiFManagerHandler::cleanModule(llvm::Module &Base) {
  std::function<void(Function *)> Explore = [&](Function *pRoot) -> void {
    TFunctionsVec calledFuncs;
    getCalledFunctions(pRoot, calledFuncs);

    for (auto *pCallee : calledFuncs) {
      Function *pFunc = nullptr;
      if (pCallee->isDeclaration()) {
        auto funcName = pCallee->getName();
        Function *pSrcFunc = nullptr;

        for (auto bifsection_i = LoadedBiFSections.begin(); bifsection_i != LoadedBiFSections.end(); ++bifsection_i) {
          pSrcFunc = bifsection_i->second->getFunction(funcName);
          if (pSrcFunc && !pSrcFunc->isDeclaration()) {
            break;
          }
        }
        if (!pSrcFunc || pSrcFunc->isDeclaration())
          continue;

        pFunc = pSrcFunc;
      } else {
        pFunc = pCallee;
      }

      if (pFunc->isMaterializable()) {
        if (Error Err = pFunc->materialize()) {
          std::string Msg;
          handleAllErrors(std::move(Err), [&](ErrorInfoBase &EIB) {
            errs() << "===> Materialize Failure: " << EIB.message().c_str() << '\n';
          });
          IGC_ASSERT_MESSAGE(0, "Failed to materialize Global Variables");
        } else {
          pFunc->addFnAttr("OclBuiltin");
          Explore(pFunc);
        }
      }
    }
  };

  for (auto &pFunc : Base) {
    Explore(&pFunc);
  }

  // nuke the unused functions so we can materializeAll() quickly
  auto CleanUnused = [](Module *Module) {
    for (auto I = Module->begin(), E = Module->end(); I != E;) {
      auto *F = &(*I++);
      if ((F->isDeclaration() || F->isMaterializable()) &&
          // We cannot remove the llvm intrinsics, because
          // they could be collected by the BitcodeReader in
          // list UpgradedIntrinsics. If we remove it now -
          // it would crash during materializing.
          !F->isIntrinsic()) {
        if (F->materialized_use_begin() == F->use_end()) {
          F->eraseFromParent();
        }
      }
    }
  };

  for (auto bifsection_i = LoadedBiFSections.begin(); bifsection_i != LoadedBiFSections.end(); ++bifsection_i) {
    llvm::Module *Module = bifsection_i->second.get();
    CleanUnused(Module);
  }
}

bool BiFManagerHandler::isModulePtrSize32(llvm::Module *pMain) {
  return (32 == pMain->getDataLayout().getPointerSizeInBits());
}
