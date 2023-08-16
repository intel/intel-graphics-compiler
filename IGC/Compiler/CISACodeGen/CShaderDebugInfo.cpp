/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "CShaderDebugInfo.hpp"
#include "GenCodeGenModule.h"
#include "ShaderCodeGen.hpp"

#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "DebugInfo/DwarfDebug.hpp"
#include "DebugInfo/VISADebugInfo.hpp"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/VISAModule.hpp"
#include "Compiler/ScalarDebugInfo/VISAScalarModule.hpp"

// Mark privateBase aka ImplicitArg::PRIVATE_BASE as Output for debugging
void CShaderDebugInfo::markPrivateBaseAsOutput() {
  IGC_ASSERT_MESSAGE(IGC_IS_FLAG_ENABLED(UseOffsetInLocation),
                     "UseOffsetInLocation not enabled");

  if (!m_pShader->GetContext()->getModuleMetaData()->compOpt.OptDisable)
    return;

  CVariable *pVar = m_pShader->GetPrivateBase();
  if (!pVar)
    return;

  // cache privateBase as it may be destroyed if subroutine is emitted.
  m_pDebugEmitter->getCurrentVISA()->setPrivateBase(pVar);
  m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(
      pVar->visaGenVariable[0], "Output", 0, nullptr);

  if (m_pShader->m_dispatchSize == SIMDMode::SIMD32 &&
      pVar->visaGenVariable[1]) {
    IGC_ASSERT_MESSAGE(false, "Private base expected to be a scalar!");
    m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(
        pVar->visaGenVariable[1], "Output", 0, nullptr);
  }
}

void CShaderDebugInfo::markInstAsOutput(llvm::Instruction *pInst,
                                        const char *pMetaDataName) {
  Value *pValue = dyn_cast<Value>(pInst);

  IGC_ASSERT_MESSAGE(IGC_IS_FLAG_ENABLED(UseOffsetInLocation),
                     "UseOffsetInLocation not enabled");
  IGC_ASSERT_MESSAGE(pInst, "Missing instruction");

  // No dummy instruction needs to be marked with "Output"
  if (dyn_cast<GenIntrinsicInst>(pValue))
    return;

  CVariable *pVar = m_pShader->GetSymbol(pValue);
  if (pVar->GetVarType() == EVARTYPE_GENERAL) {
    // If UseOffsetInLocation is enabled, we want to attach "Output"
    // attribute to:
    // 1. Per thread offset only, and/or
    // 2. Compute thread and global identification variables.
    // So that finalizer can extend their liveness to end of the
    // program. This will help debugger examine their values anywhere in
    // the code till they are in scope. However, emit "Output" attribute
    // when -g and -cl-opt-disable are both passed -g by itself shouldnt
    // alter generated code.
    if (static_cast<OpenCLProgramContext *>(m_pShader->GetContext())
            ->m_InternalOptions.KernelDebugEnable ||
        m_pShader->GetContext()->getModuleMetaData()->compOpt.OptDisable) {

      // If "Output" attribute is emitted for perThreadOffset
      // variable(s) then debug info emission is preserved for this:
      // privateBaseMem + perThreadOffset + (simdSize*offImm +
      // simd_lane*sizeof(elem))
      if (Instruction *pPTOorImplicitGIDInst = dyn_cast<Instruction>(pValue)) {
        MDNode *pPTOorImplicitGIDInstMD =
            pPTOorImplicitGIDInst->getMetadata(pMetaDataName);

        // "perThreadOffset" or implicitGlobalID"
        if (pPTOorImplicitGIDInstMD) {
          m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(
              pVar->visaGenVariable[0], "Output", 0, nullptr);

          if (m_pShader->m_dispatchSize == SIMDMode::SIMD32 &&
              pVar->visaGenVariable[1]) {
            m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(
                pVar->visaGenVariable[1], "Output", 0, nullptr);
          }
        }
      }
    }
  } else {
    // Unexpected return empty location!
    IGC_ASSERT_MESSAGE(false, "No debug info value!");
  }
}

void CShaderDebugInfo::markAsOutputWithOffset(llvm::Function &F) {
  IGC_ASSERT_MESSAGE(m_pDebugEmitter, "Missing debug emitter");
  VISAModule *visaModule = m_pDebugEmitter->getCurrentVISA();
  IGC_ASSERT_MESSAGE(visaModule, "Missing visa module");

  for (auto &bb : F) {
    for (auto &pInst : bb) {
      if (MDNode *perThreadOffsetMD = pInst.getMetadata("perThreadOffset")) {
        // Per Thread Offset non-debug instruction must have
        // 'Output' attribute added in the function to be called.
        markInstAsOutput(&pInst, "perThreadOffset");
        if (F.getCallingConv() == CallingConv::SPIR_KERNEL) {
          // Mark privateBase aka ImplicitArg::PRIVATE_BASE as Output
          // for debugging
          markPrivateBaseAsOutput();
        } else {
          // TODO: Apply privateBase of kernel to SPIR_FUNC if its a subroutine
        }

        VISAScalarModule *scVISAModule = (VISAScalarModule *)visaModule;
        IGC_ASSERT_MESSAGE(scVISAModule->getPerThreadOffset() == nullptr,
                           "setPerThreadOffset was set earlier");
        scVISAModule->setPerThreadOffset(&pInst);
        if (((OpenCLProgramContext *)(m_pShader->GetContext()))
                ->m_InternalOptions.KernelDebugEnable == false)
          return;
      }
    }
  }

  if (((OpenCLProgramContext *)(m_pShader->GetContext()))
          ->m_InternalOptions.KernelDebugEnable) {
    // Compute thread and group identification instructions will be
    // marked here regardless of stack calls detection in this shader,
    // so not only when per thread offset as well as a private base have
    // been marked as Output earlier in this function. When stack calls
    // are in use then only these group ID instructions are marked as
    // Output.
    for (auto &bb : F) {
      for (auto &pInst : bb) {
        if (MDNode *implicitGlobalIDMD =
                pInst.getMetadata("implicitGlobalID")) {
          // Compute thread and group identification instructions
          // must have 'Output' attribute added in the function to
          // be called.
          markInstAsOutput(&pInst, "implicitGlobalID");
        }
      }
    }
  }
}

void CShaderDebugInfo::markAsOutputNoOffset(llvm::Function &F) {
  for (auto &bb : F) {
    for (auto &pInst : bb) {
      markVarAsOutput(&pInst);
    }
  }
}

void CShaderDebugInfo::markVarAsOutput(const llvm::Instruction *pInst) {
  const Value *pVal = nullptr;
  if (const DbgDeclareInst *pDbgAddrInst = dyn_cast<DbgDeclareInst>(pInst))
    pVal = pDbgAddrInst->getAddress();
  else if (const DbgValueInst *pDbgValInst = dyn_cast<DbgValueInst>(pInst))
    pVal = pDbgValInst->getValue();
  else
    return;

  if (!pVal || isa<UndefValue>(pVal))
    return;

  if (isa<Constant>(pVal) && !isa<GlobalVariable>(pVal) &&
      !isa<ConstantExpr>(pVal))
    return;

  Value *pValue = const_cast<Value *>(pVal);
  if (isa<GlobalVariable>(pValue))
    return;

  if (!m_pShader->IsValueUsed(pValue))
    return;

  CVariable *pVar = m_pShader->GetSymbol(pValue);
  if (pVar->GetVarType() == EVARTYPE_GENERAL) {
    // We want to attach "Output" attribute to all variables:
    // - if UseOffsetInLocation is disabled, or
    // - if UseOffsetInLocation is enabled but there is a stack call in use,
    // so that finalizer can extend their liveness to end of
    // the program. This will help debugger examine their
    // values anywhere in the code till they are in scope.

    if (m_outputVals.find(pVar) != m_outputVals.end())
      return;

    if (m_pShader->GetContext()->getModuleMetaData()->compOpt.OptDisable) {
      // Emit "Output" attribute only when -g and -cl-opt-disable are both
      // passed -g by itself shouldnt alter generated code
      m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(
          pVar->visaGenVariable[0], "Output", 0, nullptr);
      if (m_pShader->m_dispatchSize == SIMDMode::SIMD32 &&
          pVar->visaGenVariable[1]) {
        m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(
            pVar->visaGenVariable[1], "Output", 0, nullptr);
      }
      (void)m_outputVals.insert(pVar);
    }
  }
}

unsigned int CShaderDebugInfo::getVISADclId(const CVariable *CVar,
                                            unsigned index) {
  IGC_ASSERT(index == 0 || index == 1);
  auto it = CVarToVISADclId.find(CVar);
  if (it == CVarToVISADclId.end()) {
    IGC_ASSERT_MESSAGE(false, "Didn't find VISA dcl id");
    return 0;
  }
  if (index == 0)
    return (*it).second.first;
  else if (index == 1)
    return (*it).second.second;
  return 0;
}

void CShaderDebugInfo::addVISAModule(llvm::Function *F, VISAModule *m) {
  IGC_ASSERT_MESSAGE(m_VISAModules.find(F) == m_VISAModules.end(),
                     "Reinserting VISA module for function");

  m_VISAModules.insert(std::make_pair(F, m));
}

void CShaderDebugInfo::saveVISAIdMappings(const llvm::Function &F) {
  auto cacheMapping = [this](llvm::DenseMap<llvm::Value *, CVariable *> &Map) {
    for (auto &mapping : Map) {
      auto CVar = mapping.second;
      if (CVar->visaGenVariable[0]) {
        unsigned int lower16Channels =
            (unsigned int)m_pShader->GetEncoder()
                .GetVISAKernel()
                ->getDeclarationID(CVar->visaGenVariable[0]);
        unsigned int higher16Channels = 0;
        if (numLanes(m_pShader->m_dispatchSize) == 32 && !CVar->IsUniform() &&
            CVar->visaGenVariable[1])
          higher16Channels =
              m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(
                  CVar->visaGenVariable[1]);

        CVarToVISADclId[CVar] =
            std::make_pair(lower16Channels, higher16Channels);
      }
    }
  };

  // Store llvm::Value->CVariable mappings from CShader.
  // CShader clears these mappings before compiling a new function.
  // Debug info is computed after all functions are compiled.
  // This instance stores mappings per llvm::Function so debug
  // info generation can emit variable locations correctly.
  auto &SymbolMapping = m_pShader->GetSymbolMapping();
  m_FunctionSymbols[&F] = SymbolMapping;

  // VISA builder gets destroyed at end of EmitVISAPass.
  // Debug info pass is invoked later. We need a way to
  // preserve mapping of CVariable -> VISA reg# so that
  // we can emit location information in debug info. This
  // code below iterates over all CVariable instances and
  // retrieves and stored their VISA reg# in a map. This
  // map is later queried by debug info pass.
  cacheMapping(SymbolMapping);

  auto &GlobalSymbolMapping = m_pShader->GetGlobalMapping();
  cacheMapping(GlobalSymbolMapping);

  if (m_pShader->hasFP()) {
    auto FP = m_pShader->GetFP();
    auto VISADclIdx = m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(
        FP->visaGenVariable[0]);
    CVarToVISADclId[FP] = std::make_pair(VISADclIdx, 0);
  }
}

CVariable *CShaderDebugInfo::getMapping(const llvm::Function &F,
                                        const llvm::Value *V) {
  auto &Data = m_FunctionSymbols[&F];
  auto Iter = Data.find(V);
  if (Iter != Data.end())
    return (*Iter).second;
  return nullptr;
}
