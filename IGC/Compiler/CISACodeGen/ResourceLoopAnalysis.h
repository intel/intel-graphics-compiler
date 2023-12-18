/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/CVariable.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"


namespace IGC {

    // figure out when to start a loop for lane-varying resource access,
    // and when to end such loop. So we can wrap around multiple lane-varying
    // resource access with single loop to hide latency and save instructions
  class ResourceLoopAnalysis : public llvm::FunctionPass {
  public:
    enum {
      MarkResourceLoopOutside = 0, MarkResourceLoopStart = 1,
      MarkResourceLoopInside = 2, MarkResourceLoopEnd = 4,
    };

    static char ID;

    ResourceLoopAnalysis();
    ~ResourceLoopAnalysis() {}

    virtual llvm::StringRef getPassName() const override {
      return "Resource Loop Analysis";
    }

    virtual bool runOnFunction(llvm::Function &F) override;

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
      AU.setPreservesAll();
      AU.addRequired<WIAnalysis>();
      AU.addRequired<CodeGenContextWrapper>();
    }

    void SaveStateLoopStart(ResourceDescriptor res, SamplerDescriptor samp, CVariable *f, unsigned l) {
      resource = res;
      sampler = samp;
      flag = f;
      label = l;
    }
    void ClearStateLoopEnd() {
      resource.m_resource = nullptr;
      sampler.m_sampler = nullptr;
      flag = nullptr;
      label = 0;
    }
    ResourceDescriptor& GetResourceLoopResource() { return resource; }
    SamplerDescriptor& GetResourceLoopSampler() { return sampler; }
    unsigned GetResourceLoopLabel() { return label; }
    CVariable *GetResourceLoopFlag() { return flag; }

    unsigned GetResourceLoopMarker(llvm::Instruction *inst) {
      if (LoopMap.find(inst) == LoopMap.end())
        return MarkResourceLoopOutside;
      return LoopMap[inst];
    }
    ///
    void printResourceLoops(llvm::raw_ostream &OS, llvm::Function *F = nullptr) const;

  private:
    CodeGenContext *CTX = nullptr;
    // analysis result
    llvm::DenseMap<llvm::Instruction*, unsigned> LoopMap;
    // recording state for loop emission
    ResourceDescriptor resource;
    SamplerDescriptor sampler;
    CVariable *flag = nullptr;
    uint32_t label = 0;
  };

  llvm::FunctionPass *createResourceLoopAnalysisPass();
}
