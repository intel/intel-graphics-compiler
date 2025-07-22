/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/CISACodeGen/VariableReuseAnalysis.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/VectorProcess.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

CShader::CShader(Function *pFunc, CShaderProgram *pProgram, GenericShaderState &GState)
    : m_State(GState), entry(pFunc), m_parent(pProgram), encoder() {
  m_ctx = m_parent->GetContext();
  GenericShaderState::setScratchUsage(*m_ctx, m_simdProgram);
}

bool CShader::IsRecompilationRequestForced() {
  auto it =
      std::find(GetContext()->m_kernelsWithForcedRetry.begin(), GetContext()->m_kernelsWithForcedRetry.end(), entry);
  return it != GetContext()->m_kernelsWithForcedRetry.end();
}

void CShader::InitEncoder(SIMDMode simdSize, bool canAbortOnSpill, ShaderDispatchMode shaderMode) {
  m_sendStallCycle = 0;
  m_staticCycle = 0;
  m_maxBlockId = 0;
  m_ScratchSpaceSize = 0;
  m_R0 = nullptr;
  m_NULL = nullptr;
  m_TSC = nullptr;
  m_SR0 = nullptr;
  m_CR0 = nullptr;
  m_CE0 = nullptr;
  m_DBG = nullptr;
  m_MSG0 = nullptr;
  m_HW_TID = nullptr;
  m_SP = nullptr;
  m_FP = nullptr;
  m_SavedFP = nullptr;
  m_ARGV = nullptr;
  m_RETV = nullptr;
  m_SavedSRetPtr = nullptr;
  m_ImplArgBufPtr = nullptr;
  m_LocalIdBufPtr = nullptr;
  m_GlobalBufferArg = nullptr;

  // SIMD32 is a SIMD16 shader with 2 instance of each instruction
  m_SIMDSize = (simdSize == SIMDMode::SIMD8 ? SIMDMode::SIMD8 : SIMDMode::SIMD16);
  m_ShaderDispatchMode = shaderMode;
  m_numberInstance = simdSize == SIMDMode::SIMD32 ? 2 : 1;
  if (PVCLSCEnabled()) {
    m_SIMDSize = simdSize;
    m_numberInstance = 1;
  }
  m_State.m_dispatchSize = simdSize;
  globalSymbolMapping.clear();
  symbolMapping.clear();
  ccTupleMapping.clear();
  ConstantPool.clear();
  setup.clear();
  patchConstantSetup.clear();
  kernelArgToPayloadOffsetMap.clear();
  encoder.SetProgram(this);
}

// Pre-analysis pass to be executed before call to visa builder so we can pass
// scratch space offset
void CShader::PreAnalysisPass() {
  ExtractGlobalVariables();

  auto funcMDItr = m_ModuleMetadata->FuncMD.find(entry);
  if (funcMDItr != m_ModuleMetadata->FuncMD.end()) {
    if (funcMDItr->second.privateMemoryPerWI != 0) {
      if (GetContext()->getModuleMetaData()->compOpt.UseScratchSpacePrivateMemory ||
          GetContext()->getModuleMetaData()->compOpt.UseStatelessforPrivateMemory) {
        const uint32_t GRFSize = getGRFSize();
        IGC_ASSERT(0 < GRFSize);

        m_ScratchSpaceSize = funcMDItr->second.privateMemoryPerWI * numLanes(m_State.m_dispatchSize);
        m_ScratchSpaceSize =
            std::max(m_ScratchSpaceSize, m_ctx->getIntelScratchSpacePrivateMemoryMinimalSizePerThread());

        // Round up to GRF-byte aligned.
        m_ScratchSpaceSize = ((GRFSize + m_ScratchSpaceSize - 1) / GRFSize) * GRFSize;

      }
    }
  }

  for (auto BB = entry->begin(), BE = entry->end(); BB != BE; ++BB) {
    llvm::BasicBlock *pLLVMBB = &(*BB);
    for (auto &inst : *pLLVMBB) {
      ParseShaderSpecificOpcode(&inst);
    }
  }
}

SProgramOutput *CShader::ProgramOutput() { return &m_simdProgram; }

void CShader::EOTURBWrite() {

  CEncoder &encoder = GetEncoder();
  uint messageLength = 3;

  // Creating a payload of size 3 = header + channelmask + undef data
  // As EOT message cant have message length == 0, setting channel mask = 0
  // and data = undef.
  CVariable *pEOTPayload =
      GetNewVariable(messageLength * numLanes(SIMDMode::SIMD8), ISA_TYPE_D, EALIGN_GRF, false, 1, "EOTPayload");

  CVariable *zero = ImmToVariable(0x0, ISA_TYPE_D);
  // write at handle 0
  CopyVariable(pEOTPayload, zero, 0);
  // use 0 as write mask
  CopyVariable(pEOTPayload, zero, 1);

  constexpr uint exDesc = EU_MESSAGE_TARGET_URB | cMessageExtendedDescriptorEOTBit;

  const uint desc = UrbMessage(messageLength, 0, true, false, true, 0, EU_URB_OPCODE_SIMD8_WRITE);

  CVariable *pMessDesc = ImmToVariable(desc, ISA_TYPE_D);

  encoder.Send(nullptr, pEOTPayload, exDesc, pMessDesc);
  encoder.Push();
}

// Creates a URB Fence message.
// If return value is not a nullptr, the returned variable is a send message
// writeback variable that must be read in order to wait for URB Fence
// completion, e.g. the variable may be used as payload to EOTGateway.
CVariable *CShader::URBFence(LSC_SCOPE scope) {
  if (m_Platform->hasLSCUrbMessage()) {
    encoder.LSC_Fence(LSC_URB, scope, LSC_FENCE_OP_EVICT);
    encoder.Push();
    // Return nullptr as LSC URB message has no writeback register
    // to read.
    return nullptr;
  } else {
    // A legacy HDC URB fence message issued by a thread causes further
    // messages issued by the thread to be blocked until all previous URB
    // messages have completed, or the results can be globally observed from
    // the point of view of other threads in the system. The execution mask
    // is ignored.No bounds checking is performed. The URB fence message
    // signals completion by returning data into the writeback register. The
    // data returned in the writeback register is undefined. When an
    // instruction reads the writeback register value, then this thread is
    // blocked until all previous URB messages are globally observable. The
    // writeback register must be read before this thread sends another data
    // port message.
    const uint desc = ::IGC::URBFence();
    constexpr uint exDesc = EU_MESSAGE_TARGET_URB;

    // Message length is of size 1, and is ignored (thus it is left
    // uninitialized).
    CVariable *payload = GetNewVariable(1 * numLanes(SIMDMode::SIMD8), ISA_TYPE_D, EALIGN_GRF, "URBPayload");
    // Message response length is of size 1.
    CVariable *dst = GetNewVariable(1 * numLanes(SIMDMode::SIMD8), ISA_TYPE_D, EALIGN_GRF, "URBReturnValue");

    encoder.SetSimdSize(SIMDMode::SIMD1);
    encoder.Send(dst, payload, exDesc, ImmToVariable(desc, ISA_TYPE_D));
    encoder.Push();
    return dst;
  }
}

// Payload may be non-nullptr if it is e.g. a response payload from fence.
void CShader::EOTGateway(CVariable *payload) {
  const uint desc = ::IGC::EOTGateway(EU_GW_FENCE_PORTS_None);
  constexpr uint exDesc = EU_MESSAGE_TARGET_GATEWAY | cMessageExtendedDescriptorEOTBit;

  // Message length is of size 1, and is ignored.
  if (!payload) {
    // Message length is of size 1, and is ignored (thus it is left
    // uninitialized).
    payload = GetNewVariable(getGRFSize(), ISA_TYPE_D, EALIGN_GRF, "EOTPayload");
  }

  encoder.SetSimdSize(SIMDMode::SIMD1);
  encoder.Send(nullptr, payload, exDesc, ImmToVariable(desc, ISA_TYPE_D));
  encoder.Push();
}


void CShader::EOTRenderTarget(CVariable *r1, bool isPerCoarse) {
  CVariable *src[4] = {nullptr, nullptr, nullptr, nullptr};
  bool isUndefined[4] = {true, true, true, true};
  CVariable *const nullSurfaceBti = ImmToVariable(m_pBtiLayout->GetNullSurfaceIdx(), ISA_TYPE_D);
  CVariable *const blendStateIndex = ImmToVariable(0, ISA_TYPE_D);
  SetBindingTableEntryCountAndBitmap(true, BUFFER_TYPE_UNKNOWN, 0, m_pBtiLayout->GetNullSurfaceIdx());
  encoder.RenderTargetWrite(src, isUndefined,
                            true,        // lastRenderTarget,
                            true,        // Null RT
                            false,       // perSample,
                            isPerCoarse, // coarseMode,
                            false,       // isHeaderMaskFromCe0,
                            nullSurfaceBti, blendStateIndex,
                            nullptr, // source0Alpha,
                            nullptr, // oMaskOpnd,
                            nullptr, // outputDepthOpnd,
                            nullptr, // stencilOpnd,
                            nullptr, // cpscounter,
                            nullptr, // sampleIndex,
                            r1);
  encoder.Push();
}

void CShader::AddEpilogue(llvm::ReturnInst *ret) {
  encoder.EOT();
  encoder.Push();
}

void CShader::InitializeStackVariables() {
  // Set the SP/FP variable types to match the private pointer size defined in
  // the data layout
  bool isA64Private = (GetContext()->getRegisterPointerSizeInBits(ADDRESS_SPACE_PRIVATE) == 64);

  // create argument-value register, limited to 12 GRF
  m_ARGV = GetNewVariable(getGRFSize() * 3, ISA_TYPE_D, getGRFAlignment(), false, 1, "ARGV");
  encoder.GetVISAPredefinedVar(m_ARGV, PREDEFINED_ARG);
  // create return-value register, limited to 8 GRF
  m_RETV = GetNewVariable(getGRFSize() * 2, ISA_TYPE_D, getGRFAlignment(), false, 1, "RETV");
  encoder.GetVISAPredefinedVar(m_RETV, PREDEFINED_RET);
  // create stack-pointer register
  m_SP = GetNewVariable(1, (isA64Private ? ISA_TYPE_UQ : ISA_TYPE_UD), (isA64Private ? EALIGN_QWORD : EALIGN_DWORD),
                        true, 1, "SP");
  encoder.GetVISAPredefinedVar(m_SP, PREDEFINED_FE_SP);
  // create frame-pointer register
  m_FP = GetNewVariable(1, (isA64Private ? ISA_TYPE_UQ : ISA_TYPE_UD), (isA64Private ? EALIGN_QWORD : EALIGN_DWORD),
                        true, 1, "FP");
  encoder.GetVISAPredefinedVar(m_FP, PREDEFINED_FE_FP);
  // create pointers locations to buffers
  if (!m_ctx->platform.isProductChildOf(IGFX_XE_HP_SDV) && IGC_IS_FLAG_ENABLED(EnableGlobalStateBuffer)) {
    m_ImplArgBufPtr = GetNewVariable(1, ISA_TYPE_UQ, EALIGN_QWORD, true, 1, "ImplArgPtr");
    encoder.GetVISAPredefinedVar(m_ImplArgBufPtr, PREDEFINED_IMPL_ARG_BUF_PTR);
    m_LocalIdBufPtr = GetNewVariable(1, ISA_TYPE_UQ, EALIGN_QWORD, true, 1, "LocalIdPtr");
    encoder.GetVISAPredefinedVar(m_LocalIdBufPtr, PREDEFINED_LOCAL_ID_BUF_PTR);
  }
  // reserve a temp GRF for implicit arguments programmed by FE
  if (m_ctx->m_DriverInfo.SupportGlobalStackArgs() && m_ctx->type != ShaderType::RAYTRACING_SHADER) {
    m_GlobalBufferArg = GetNewVariable(2, ISA_TYPE_UQ, EALIGN_QWORD, true, 1, "GlobalBufferArg");
  }

  auto &argRegisterReservations = m_ctx->getModuleMetaData()->argRegisterReservations;
  m_ARGVReservedVariablesTotalSize = 0;

  for (int i = 0; i < ARG_SPACE_RESERVATION_SLOTS::NUM_ARG_SPACE_RESERVATION_SLOTS; i++) {
    uint32_t reservationSize = argRegisterReservations[i];
    if (reservationSize) {
      auto aligned_offset = iSTD::Align(m_ARGVReservedVariablesTotalSize, reservationSize);
      m_ARGVReservedVariables[i] = GetNewAlias(GetARGV(), ISA_TYPE_W, aligned_offset, reservationSize, true);
      encoder.MarkAsOutput(m_ARGVReservedVariables[i]);
      m_ARGVReservedVariablesTotalSize = aligned_offset + reservationSize;
    }
  }
}

// This function initializes the stack in a limited scope, only for the purpose
// of handling VLA. It is intended to be called when VLA is used but stack calls
// are not present. Only SP and PF variables are initialized. Note that these
// variables are not initialized to the predefined %sp and %fp VISA variables
// because it's not necessary. All other stack-related variables like ARGV,
// RETV, etc. are also not initialized as they are not needed for VLA handling.
void CShader::InitializeSPFPForVLA() {
  IGC_ASSERT_MESSAGE(!HasStackCalls(), "InitializeSPFPForVLA should only be called if stack "
                                       "calls are not present!");

  // Set the SP/FP variable types to match the private pointer size defined in
  // the data layout
  bool isA64Private = (GetContext()->getRegisterPointerSizeInBits(ADDRESS_SPACE_PRIVATE) == 64);

  // create stack-pointer register
  m_SP = GetNewVariable(1, (isA64Private ? ISA_TYPE_UQ : ISA_TYPE_UD), (isA64Private ? EALIGN_QWORD : EALIGN_DWORD),
                        true, 1, "SP");
  // create frame-pointer register
  m_FP = GetNewVariable(1, (isA64Private ? ISA_TYPE_UQ : ISA_TYPE_UD), (isA64Private ? EALIGN_QWORD : EALIGN_DWORD),
                        true, 1, "FP");
}

/// save FP of previous frame when entering a stack-call function
void CShader::SaveStackState() {
  IGC_ASSERT(!m_SavedFP);
  IGC_ASSERT(m_FP);
  IGC_ASSERT(m_SP);
  m_SavedFP = GetNewVariable(m_FP);
  encoder.Copy(m_SavedFP, m_FP);
  encoder.Push();
}

/// restore SP and FP when exiting a stack-call function
void CShader::RestoreStackState() {
  IGC_ASSERT(m_SavedFP);
  IGC_ASSERT(m_FP);
  IGC_ASSERT(m_SP);
  // Restore SP to current FP
  encoder.Copy(m_SP, m_FP);
  encoder.Push();
  // Restore FP to previous frame's FP
  encoder.Copy(m_FP, m_SavedFP);
  encoder.Push();
  // Reset temp variables
  m_SavedFP = nullptr;
  m_GlobalBufferArg = nullptr;

  for (auto &arg : m_ARGVReservedVariables)
    arg = nullptr;
}

void CShader::CreateImplicitArgs() {
  if (IGC::isIntelSymbolTableVoidProgram(entry))
    return;

  m_R0 = GetNewVariable(getGRFSize() / SIZE_DWORD, ISA_TYPE_D, EALIGN_GRF, false, 1, "R0");
  encoder.GetVISAPredefinedVar(m_R0, PREDEFINED_R0);

  // create variables for implicit args
  ImplicitArgs implicitArgs(*entry, m_pMdUtils);
  unsigned numImplicitArgs = implicitArgs.size();

  // Push Args are only for entry function
  const unsigned numPushArgsEntry = m_ModuleMetadata->pushInfo.pushAnalysisWIInfos.size();
  const unsigned numPushArgs =
      (isEntryFunc(m_pMdUtils, entry) && !isNonEntryMultirateShader(entry) ? numPushArgsEntry : 0);
  const int numFuncArgs = entry->arg_size() - numImplicitArgs - numPushArgs;
  IGC_ASSERT_MESSAGE(0 <= numFuncArgs, "Function arg size does not match meta data and push args.");

  // Create symbol for every arguments [5/2019]
  //   (Previously, symbols are created only for implicit args.)
  //   Since vISA requires input var (argument) to be root symbol (CVariable)
  //   and GetSymbol() does not guarantee this due to coalescing of argument
  //   values and others. Here, we handle arguments specially by creating
  //   a CVariable symbol for each argument, and use this newly-created symbol
  //   as the root symbol for its congruent class if any. This should always
  //   work as it does not matter which value in a coalesced set is going to
  //   be a root symbol.
  //
  //   Once a root symbol is created, the root value of its conguent class
  //   needs to have as its symbol an alias to this root symbol.

  // Update SymbolMapping for argument value.
  auto updateArgSymbolMapping = [&](Value *Arg, CVariable *CVarArg) {
    symbolMapping.insert(std::make_pair(Arg, CVarArg));
    Value *Node = m_deSSA ? m_deSSA->getRootValue(Arg) : nullptr;
    if (Node) {
      // If Arg isn't root, must setup symbolMapping for root.
      if (Node != Arg) {
        // 'Node' should not have a symbol entry at this moment.
        IGC_ASSERT_MESSAGE(symbolMapping.count(Node) == 0, "Root symbol of arg should not be set at this point!");
        CVariable *aV = CVarArg;
        if (IGC_IS_FLAG_ENABLED(EnableDeSSA)) {
          aV = createAliasIfNeeded(Node, CVarArg);
        }
        symbolMapping[Node] = aV;
      }
    }
  };

  llvm::Function::arg_iterator arg = entry->arg_begin();
  for (int i = 0; i < numFuncArgs; ++i, ++arg) {
    Value *ArgVal = arg;
    if (ArgVal->use_empty())
      continue;
    e_alignment algn = GetPreferredAlignment(ArgVal, m_WI, m_ctx);
    CVariable *ArgCVar = GetNewVector(ArgVal, algn);
    updateArgSymbolMapping(ArgVal, ArgCVar);
  }

  for (unsigned i = 0; i < numImplicitArgs; ++i, ++arg) {
    ImplicitArg implictArg = implicitArgs[i];
    IGC_ASSERT_MESSAGE((implictArg.getNumberElements() < (UINT16_MAX)), "getNumberElements > higher than 64k");

    bool isUniform = WIAnalysis::isDepUniform(implictArg.getDependency());
    uint16_t nbElements = (uint16_t)implictArg.getNumberElements();

    if (implictArg.isLocalIDs() && PVCLSCEnabled() && (m_SIMDSize == SIMDMode::SIMD32)) {
      nbElements = getGRFSize() / 2;
    }
    CVariable *var = GetNewVariable(nbElements, implictArg.getVISAType(*m_DL), implictArg.getAlignType(*m_DL),
                                    isUniform, isUniform ? 1 : m_numberInstance, CName(implictArg.getName()));

    if (implictArg.getArgType() == ImplicitArg::R0) {
      encoder.GetVISAPredefinedVar(var, PREDEFINED_R0);
    }

    // This is a per function symbol mapping, that is, only available for a
    // llvm function which will be cleared for each run of EmitVISAPass.
    updateArgSymbolMapping(arg, var);

    // Kernel's implicit arguments's symbols will be available for the
    // whole kernel CodeGen. With this, there is no need to pass implicit
    // arguments and this should help to reduce the register pressure with
    // presence of subroutines.
    IGC_ASSERT_MESSAGE(!globalSymbolMapping.count(&(*arg)), "should not exist already");
    globalSymbolMapping.insert(std::make_pair(&(*arg), var));
  }

  for (unsigned i = 0; i < numPushArgs; ++i, ++arg) {
    Value *ArgVal = arg;
    if (ArgVal->use_empty())
      continue;
    e_alignment algn = GetPreferredAlignment(ArgVal, m_WI, m_ctx);
    CVariable *ArgCVar = GetNewVector(ArgVal, algn);
    updateArgSymbolMapping(ArgVal, ArgCVar);
  }

  CreateAliasVars();
}

DebugInfoData &IGC::CShader::GetDebugInfoData() { return diData; }

// For sub-vector aliasing, pre-allocating cvariables for those
// valeus that have sub-vector aliasing before emit instructions.
// (The sub-vector aliasing is done in VariableReuseAnalysis.)
void CShader::CreateAliasVars() {
  // Create CVariables for vector aliasing (This is more
  // efficient than doing it on-fly inside getSymbol()).
  if (GetContext()->getVectorCoalescingControl() > 0 && !m_VRA->m_baseVecMap.empty()) {
    // For each vector alias root, generate cvariable
    // for it and all its component sub-vector
    for (auto &II : m_VRA->m_sortedBaseVec) {
      SBaseVecDesc *BV = II;
      Value *baseVal = BV->BaseVector;
      if (BV->Align != EALIGN_AUTO) {
        // Need to set align on root cvar
        Value *rV = baseVal;
        if (m_deSSA) {
          Value *dessaRoot = m_deSSA->getRootValue(baseVal);
          if (dessaRoot && dessaRoot != baseVal)
            rV = dessaRoot;
        }
        (void)GetSymbol(rV, false, BV->Align);
      }
      CVariable *rootCVar = GetSymbol(baseVal);
      Type *eltTy = BV->OrigType->getScalarType();
      uint32_t bEltBytes = (uint32_t)m_DL->getTypeStoreSize(eltTy);

      // Generate all vector aliasers and their
      // dessa root if any.
      for (int i = 0, sz = (int)BV->Aliasers.size(); i < sz; ++i) {
        SSubVecDesc *aSV = BV->Aliasers[i];
        Value *V = aSV->Aliaser;
        // Create alias cvariable for Aliaser and its dessa root if any
        Value *Vals[2] = {V, nullptr};
        if (m_deSSA) {
          Value *dessaRootVal = m_deSSA->getRootValue(V);
          if (dessaRootVal && dessaRootVal != V)
            Vals[1] = dessaRootVal;
        }
        // index to baseVal, use baseElt to compute offset
        int startIx = aSV->StartElementOffset;
        int offsetInBytes = bEltBytes * startIx;

        for (int i = 0; i < 2; ++i) {
          V = Vals[i];
          if (!V)
            continue;

          Type *Ty = V->getType();
          IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
          Type *BTy = VTy ? VTy->getElementType() : Ty;
          int nelts = (VTy ? (int)VTy->getNumElements() : 1);

          VISA_Type visaTy = GetType(BTy);
          // special handling of struct type
          if (BTy->isStructTy()) {
            IGC_ASSERT((int)CEncoder::GetCISADataTypeSize(visaTy) == 1);
            nelts *= (int)m_DL->getTypeStoreSize(BTy);
          }

          int nbelts = nelts;
          if (!rootCVar->IsUniform()) {
            int width = (int)numLanes(m_SIMDSize);
            offsetInBytes *= width;
            nbelts *= width;
          }
          CVariable *Var = GetNewAlias(rootCVar, visaTy, offsetInBytes, nbelts);
          symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(V, Var));
        }
      }
    }
  }
}

void CShader::AddPatchTempSetup(CVariable *var) { payloadTempSetup.push_back(var); }

void CShader::AddPatchPredSetup(CVariable *var) { payloadPredSetup.push_back(var); }

bool CShader::AppendPayloadSetup(CVariable *var) {
  auto v = var->GetAlias() ? var->GetAlias() : var;
  if (find(payloadLiveOutSetup.begin(), payloadLiveOutSetup.end(), v) != payloadLiveOutSetup.end()) {
    return true;
  }
  payloadLiveOutSetup.push_back(v);
  return false;
}

void CShader::AddSetup(uint index, CVariable *var) {
  if (setup.size() < index + 1) {
    setup.resize(index + 1, nullptr);
  }
  if (setup[index] == nullptr) {
    setup[index] = var;
  }
}

void CShader::AddPatchConstantSetup(uint index, CVariable *var) {
  if (patchConstantSetup.size() < index + 1) {
    patchConstantSetup.resize(index + 1, nullptr);
  }
  if (patchConstantSetup[index] == nullptr) {
    patchConstantSetup[index] = var;
  }
}

void CShader::AllocateInput(CVariable *var, uint offset, uint instance, bool forceLiveOut) {
  // the input offset must respect the variable alignment
  IGC_ASSERT(nullptr != var);
  IGC_ASSERT(offset % (1u << var->GetAlign()) == 0);
  encoder.DeclareInput(var, offset, instance);
  kernelArgToPayloadOffsetMap[var] = offset;
  // For the payload section, we need to mark inputs to be outputs
  // so that inputs will be alive across the entire payload section
  if (forceLiveOut) {
    encoder.MarkAsPayloadLiveOut(var);
  }
}

void CShader::AllocateOutput(CVariable *var, uint offset, uint instance) {
  IGC_ASSERT(nullptr != var);
  IGC_ASSERT(offset % (1u << var->GetAlign()) == 0);
  encoder.DeclareInput(var, offset, instance);
  encoder.MarkAsOutput(var);
}

void CShader::AllocatePred(CVariable *var, uint offset, bool forceLiveOut) {
  IGC_ASSERT(nullptr != var);
  IGC_ASSERT(offset % (1u << var->GetAlign()) == 0);
  encoder.DeclarePred(var, offset);
  kernelArgToPayloadOffsetMap[var] = offset;
  // For the payload section, we need to mark inputs to be outputs
  // so that inputs will be alive across the entire payload section
  if (forceLiveOut) {
    encoder.MarkAsPayloadLiveOut(var);
  }
}

void CShader::AllocateConstants3DShader(uint &offset) {
  if (m_Platform->WaForceCB0ToBeZeroWhenSendingPC() && m_DriverInfo->implementPushConstantWA()) {
    // Allocate space for constant pushed from the constant buffer
    AllocateConstants(offset);
    AllocateSimplePushConstants(offset);
    // Allocate space for constant set by driver
    AllocateNOSConstants(offset);
  } else {
    // Allocate space for constant set by driver
    AllocateNOSConstants(offset);
    // Allocate space for constant pushed from the constant buffer
    AllocateConstants(offset);
    AllocateSimplePushConstants(offset);
  }
  offset = iSTD::Align(offset, getGRFSize());
}

void CShader::AllocateConstants(uint &offset) {
  m_State.m_ConstantBufferLength = 0;
  for (auto I = pushInfo.constants.begin(), E = pushInfo.constants.end(); I != E; I++) {
    CVariable *var = GetSymbol(m_argListCache[I->second]);
    AllocateInput(var, offset + m_State.m_ConstantBufferLength, 0, encoder.IsCodePatchCandidate());
    m_State.m_ConstantBufferLength += var->GetSize();
  }

  m_State.m_ConstantBufferLength =
      iSTD::Align(m_State.m_ConstantBufferLength, getMinPushConstantBufferAlignmentInBytes());
  offset += m_State.m_ConstantBufferLength;
}

void CShader::AllocateSimplePushConstants(uint &offset) {
  for (unsigned int i = 0; i < pushInfo.simplePushBufferUsed; i++) {
    for (const auto &I : pushInfo.simplePushInfoArr[i].simplePushLoads) {
      uint subOffset = I.first;
      CVariable *var = GetSymbol(m_argListCache[I.second]);
      AllocateInput(var, subOffset - pushInfo.simplePushInfoArr[i].offset + offset, 0, encoder.IsCodePatchCandidate());
    }
    offset += pushInfo.simplePushInfoArr[i].size;
  }
}

void CShader::AllocateNOSConstants(uint &offset) {
  uint maxConstantPushed = 0;
  for (auto I = pushInfo.constantReg.begin(), E = pushInfo.constantReg.end(); I != E; I++) {
    CVariable *var = GetSymbol(m_argListCache[I->second]);
    AllocateInput(var, offset + I->first * SIZE_DWORD, 0, encoder.IsCodePatchCandidate());
    uint numConstantsPushed = int_cast<uint>(llvm::divideCeil(var->GetSize(), SIZE_DWORD));
    maxConstantPushed = std::max(maxConstantPushed, I->first + numConstantsPushed);
  }
  maxConstantPushed = iSTD::Max(maxConstantPushed, static_cast<uint>(m_ModuleMetadata->MinNOSPushConstantSize));
  m_State.m_NOSBufferSize = iSTD::Align(maxConstantPushed * SIZE_DWORD, getMinPushConstantBufferAlignmentInBytes());
  offset += m_State.m_NOSBufferSize;
}

CVariable *CShader::CreateFunctionSymbol(llvm::Function *pFunc, StringRef symbolName) {
  // Functions with uses in this module requires relocation
  CVariable *funcAddr = GetSymbol(pFunc);
  std::string funcName = pFunc->getName().str();
  if (!symbolName.empty()) {
    funcName = symbolName.str();
  }
  encoder.AddVISASymbol(funcName, funcAddr);
  encoder.Push();

  return funcAddr;
}

CVariable *CShader::CreateGlobalSymbol(llvm::GlobalVariable *pGlobal) {
  CVariable *globalAddr = GetSymbol(pGlobal);
  std::string globalName = pGlobal->getName().str();
  encoder.AddVISASymbol(globalName, globalAddr);
  encoder.Push();

  return globalAddr;
}

void CShader::CacheArgumentsList() {
  m_argListCache.clear();
  for (auto arg = entry->arg_begin(); arg != entry->arg_end(); ++arg)
    m_argListCache.push_back(&(*arg));
}

// Pixel shader has dedicated implementation of this function
void CShader::MapPushedInputs() {
  for (auto I = pushInfo.inputs.begin(), E = pushInfo.inputs.end(); I != E; I++) {
    // We need to map the value associated with the value pushed to a physical
    // register
    CVariable *var = GetSymbol(m_argListCache[I->second.argIndex]);
    AddSetup(I->second.index, var);
  }
}

bool CShader::IsPatchablePS() {
  return false;
}

CVariable *CShader::GetR0() { return m_R0; }

CVariable *CShader::GetNULL() {
  if (!m_NULL) {
    m_NULL = new (Allocator) CVariable(2, true, ISA_TYPE_D, EVARTYPE_GENERAL, EALIGN_DWORD, false, 1, CName::NONE);
    encoder.GetVISAPredefinedVar(m_NULL, PREDEFINED_NULL);
  }
  return m_NULL;
}

CVariable *CShader::GetTSC() {
  if (!m_TSC) {
    m_TSC = new (Allocator) CVariable(2, true, ISA_TYPE_UD, EVARTYPE_GENERAL, EALIGN_DWORD, false, 1, CName::NONE);
    encoder.GetVISAPredefinedVar(m_TSC, PREDEFINED_TSC);
  }
  return m_TSC;
}

CVariable *CShader::GetSR0() {
  if (!m_SR0) {
    m_SR0 = GetNewVariable(4, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);

    encoder.GetVISAPredefinedVar(m_SR0, PREDEFINED_SR0);
  }
  return m_SR0;
}

CVariable *CShader::GetCR0() {
  if (!m_CR0) {
    m_CR0 = GetNewVariable(3, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);
    encoder.GetVISAPredefinedVar(m_CR0, PREDEFINED_CR0);
  }
  return m_CR0;
}

CVariable *CShader::GetCE0() {
  if (!m_CE0) {
    m_CE0 = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);
    encoder.GetVISAPredefinedVar(m_CE0, PREDEFINED_CE0);
  }
  return m_CE0;
}

CVariable *CShader::GetDBG() {
  if (!m_DBG) {
    m_DBG = GetNewVariable(2, ISA_TYPE_D, EALIGN_DWORD, true, CName::NONE);
    encoder.GetVISAPredefinedVar(m_DBG, PREDEFINED_DBG);
  }
  return m_DBG;
}

CVariable *CShader::GetMSG0() {
  if (!m_MSG0) {
    m_MSG0 = GetNewVariable(4, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);

    encoder.GetVISAPredefinedVar(m_MSG0, PREDEFINED_MSG0);
  }
  return m_MSG0;
}
void CShader::RemoveBitRange(CVariable *&src, unsigned removebit, unsigned range) {
  CVariable *leftHalf = GetNewVariable(src);
  CVariable *rightHalf = GetNewVariable(src);
  uint32_t mask = BITMASK(removebit);
  // src = (src & mask) | ((src >> range) & ~mask)
  encoder.And(rightHalf, src, ImmToVariable(mask, ISA_TYPE_D));
  encoder.Push();
  encoder.IShr(leftHalf, src, ImmToVariable(range, ISA_TYPE_D));
  encoder.Push();

  if (IGC_IS_FLAG_ENABLED(EnableBfn) && m_Platform->supportBfnInstruction()) {
    // src = leftHalf & ~mask | rightHalf;
    // Hardcoded bfn control "s0&s1|s2": 0xF8
    encoder.Bfn(0xF8, src, leftHalf, ImmToVariable(~mask, ISA_TYPE_D), rightHalf);
    encoder.Push();
  } else {
    encoder.And(leftHalf, leftHalf, ImmToVariable(~mask, ISA_TYPE_D));
    encoder.Push();
    encoder.Or(src, rightHalf, leftHalf);
    encoder.Push();
  }
}

CVariable *CShader::GetHWTID() {
  if (!m_HW_TID) {
    if (m_Platform->getHWTIDFromSR0()) {
      if ((m_Platform->getPlatformInfo().eProductFamily == IGFX_BMG) ||
          (m_Platform->getPlatformInfo().eProductFamily == IGFX_LUNARLAKE)) {
        if (m_Platform->supportsWMTPForShaderType(m_ctx->type)) {

          m_HW_TID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "HWTID");
          encoder.SetNoMask();
          encoder.SetSrcSubReg(0, 0);

          // m_HW_TID = msg0 & BITMASK(8)
          encoder.And(m_HW_TID, GetMSG0(), ImmToVariable(BITMASK(8), ISA_TYPE_UD));
          encoder.Push();

          CVariable *srID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "SR_0_6");
          // srID = sr0 & BitMASK(7)
          encoder.And(srID, GetSR0(), ImmToVariable(BITMASK(7), ISA_TYPE_UD));
          encoder.Push();

          // m_HW_TID = m_HW_TID << 6
          encoder.Shl(m_HW_TID, m_HW_TID, ImmToVariable(6, ISA_TYPE_UD));
          encoder.Push();

          // Remove bit sr0.0[3]
          // srID = sr0.0[6:4] : sr0.0[2:0]
          RemoveBitRange(srID, 3, 1);

          // m_HW_TID = m_HW_TID | srID
          encoder.Or(m_HW_TID, m_HW_TID, srID);
          encoder.Push();
        } else {
          uint32_t bitmask = BITMASK(16);
          m_HW_TID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "HWTID");
          encoder.SetNoMask();
          encoder.SetSrcSubReg(0, 0);
          encoder.And(m_HW_TID, GetSR0(), ImmToVariable(bitmask, ISA_TYPE_UD));
          encoder.Push();

          // Remove bit [10]
          RemoveBitRange(m_HW_TID, 10, 1);
          // Remove bit [7]
          RemoveBitRange(m_HW_TID, 7, 1);
          // Remove bit [3]
          RemoveBitRange(m_HW_TID, 3, 1);
        }

        return m_HW_TID;
      }
      if (m_Platform->getPlatformInfo().eProductFamily == IGFX_PVC) {
       // [14:12] Slice ID.
       // [11:9] SubSlice ID
       // [8] : EUID[2]
       // [7:6] : Reserved
       // [5:4] EUID[1:0]
       // [3] : Reserved MBZ
       // [2:0] : TID
       //
       // HWTID is calculated using a concatenation of
       // TID:EUID:SubSliceID:SliceID

        uint32_t bitmask = BITMASK(15);
        m_HW_TID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "HWTID");
        encoder.SetNoMask();
        encoder.SetSrcSubReg(0, 0);
        encoder.And(m_HW_TID, GetSR0(), ImmToVariable(bitmask, ISA_TYPE_UD));
        encoder.Push();

        // Remove bit [7:6]
        RemoveBitRange(m_HW_TID, 6, 2);
        // Remove bit [3]
        RemoveBitRange(m_HW_TID, 3, 1);

        return m_HW_TID;
      }

      if (m_Platform->getPlatformInfo().eRenderCoreFamily == IGFX_XE3_CORE) {
       // msg0.0:
       // [7:0] : LogicalSSID
       // sr0.0:
       // [6:4] : EUID
       // [3:0] : TID
       // TID is 4 bits on XE3, but the max number of threads per EU
       // is 10 (instead of 16) so cannot concatenate EUID and TID
       // directly. HWTID is calculated by:
       // TID + 10 * [EUID:LogicalSSID]

        if (m_Platform->supportsWMTPForShaderType(m_ctx->type)) {
          m_HW_TID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "HWTID");
          encoder.SetNoMask();
          encoder.SetSrcSubReg(0, 0);

          // m_HW_TID = msg0 & BITMASK(8)
          encoder.And(m_HW_TID, GetMSG0(), ImmToVariable(BITMASK(8), ISA_TYPE_UD));
          encoder.Push();

          CVariable *euID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "SR_4_6");
          // euID = sr0 & BITMASK(7)
          encoder.And(euID, GetSR0(), ImmToVariable(BITMASK(7), ISA_TYPE_UD));
          encoder.Push();

          // m_HW_TID  = m_HW_TID << 3
          encoder.Shl(m_HW_TID, m_HW_TID, ImmToVariable(3, ISA_TYPE_UD));
          encoder.Push();

          // euID = euID >> 4
          encoder.Shr(euID, euID, ImmToVariable(4, ISA_TYPE_UD));
          encoder.Push();

          CVariable *tID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "SR_0_3");
          // tID = sr0 & BITMASK(4)
          encoder.And(tID, GetSR0(), ImmToVariable(BITMASK(4), ISA_TYPE_UD));
          encoder.Push();

          // m_HW_TID = m_HW_TID | euID
          encoder.Or(m_HW_TID, m_HW_TID, euID);
          encoder.Push();

          // m_HW_TID = m_HW_TID * 10 + tID
          encoder.Mad(m_HW_TID, m_HW_TID, ImmToVariable(10, ISA_TYPE_UD), tID);
          encoder.Push();
        } else {
          uint32_t bitmask = BITMASK(18);
          m_HW_TID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "HWTID");
          encoder.SetNoMask();
          encoder.SetSrcSubReg(0, 0);
          encoder.And(m_HW_TID, GetSR0(), ImmToVariable(bitmask, ISA_TYPE_UD));
          encoder.Push();

          // Remove bit [13:12]
          RemoveBitRange(m_HW_TID, 12, 2);
          // Remove bit [7]
          RemoveBitRange(m_HW_TID, 7, 1);
        }

        return m_HW_TID;
      }

      // XeHP_SDV
      // [13:11] Slice ID.
      // [10:9] Dual - SubSlice ID
      // [8] SubSlice ID.
      // [7] : EUID[2]
      // [6] : Reserved
      // [5:4] EUID[1:0]
      // [3] : Reserved MBZ
      // [2:0] : TID
      //
      // HWTID is calculated using a concatenation of
      // TID:EUID:SubSliceID:SliceID

      uint32_t bitmask = BITMASK(14);
      m_HW_TID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "HWTID");
      encoder.SetNoMask();
      encoder.SetSrcSubReg(0, 0);
      encoder.And(m_HW_TID, GetSR0(), ImmToVariable(bitmask, ISA_TYPE_D));
      encoder.Push();

      // Remove bit [6]
      RemoveBitRange(m_HW_TID, 6, 1);
      // Remove bit [3]
      RemoveBitRange(m_HW_TID, 3, 1);
    } else {
      m_HW_TID = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, 1, "HWTID");
      encoder.GetVISAPredefinedVar(m_HW_TID, PREDEFINED_HW_TID);
    }
  }
  return m_HW_TID;
}

CVariable *CShader::GetPrivateBase() {
  ImplicitArgs implicitArgs(*entry, m_pMdUtils);
  unsigned numPushArgs = m_ModuleMetadata->pushInfo.pushAnalysisWIInfos.size();
  unsigned numImplicitArgs = implicitArgs.size();
  IGC_ASSERT_MESSAGE(entry->arg_size() >= (numImplicitArgs + numPushArgs),
                     "Function arg size does not match meta data and push args.");
  unsigned numFuncArgs = entry->arg_size() - numImplicitArgs - numPushArgs;

  Argument *kerArg = nullptr;
  llvm::Function::arg_iterator arg = std::next(entry->arg_begin(), numFuncArgs);
  for (unsigned i = 0; i < numImplicitArgs; ++i, ++arg) {
    ImplicitArg implicitArg = implicitArgs[i];
    if (implicitArg.getArgType() == ImplicitArg::ArgType::PRIVATE_BASE) {
      kerArg = (&*arg);
      break;
    }
  }
  IGC_ASSERT(kerArg);
  return GetSymbol(kerArg);
}


CVariable *CShader::GetImplArgBufPtr() {
  IGC_ASSERT(m_ImplArgBufPtr);
  return m_ImplArgBufPtr;
}

CVariable *CShader::GetLocalIdBufPtr() {
  IGC_ASSERT(m_LocalIdBufPtr);
  return m_LocalIdBufPtr;
}

CVariable *CShader::GetGlobalBufferArg() { return m_GlobalBufferArg; }

CVariable *CShader::GetFP() {
  IGC_ASSERT(m_FP);
  return m_FP;
}
CVariable *CShader::GetPrevFP() { return m_SavedFP; }

CVariable *CShader::GetARGVReservedVariable(ARG_SPACE_RESERVATION_SLOTS slot) { return m_ARGVReservedVariables[slot]; }

uint32_t CShader::GetARGVReservedVariablesTotalSize() { return m_ARGVReservedVariablesTotalSize; }

CVariable *CShader::GetSP() {
  IGC_ASSERT(m_SP);
  return m_SP;
}

CVariable *CShader::GetARGV() {
  IGC_ASSERT(m_ARGV);
  return m_ARGV;
}

CVariable *CShader::GetRETV() {
  IGC_ASSERT(m_RETV);
  return m_RETV;
}

CEncoder &CShader::GetEncoder() { return encoder; }

void CShader::SaveSRet(CVariable *sretPtr) {
  IGC_ASSERT(m_SavedSRetPtr == nullptr);
  m_SavedSRetPtr = sretPtr;
}

CVariable *CShader::GetAndResetSRet() {
  CVariable *temp = m_SavedSRetPtr;
  m_SavedSRetPtr = nullptr;
  return temp;
}

CShader::~CShader() {
  // free all the memory allocated
  Destroy();
}

bool CShader::IsValueUsed(llvm::Value *value) {
  auto it = symbolMapping.find(value);
  if (it != symbolMapping.end()) {
    return true;
  }
  return false;
}

CVariable *CShader::GetGlobalCVar(llvm::Value *value) {
  auto it = globalSymbolMapping.find(value);
  if (it != globalSymbolMapping.end())
    return it->second;
  return nullptr;
}

CVariable *CShader::BitCast(CVariable *var, VISA_Type newType) {
  CVariable *bitCast = nullptr;
  uint32_t newEltSz = CEncoder::GetCISADataTypeSize(newType);
  uint32_t eltSz = var->GetElemSize();
  // Bitcase requires both src and dst have the same size, which means
  // one element size is the same as or multiple of the other (if they
  // are vectors with different number of elements).
  IGC_ASSERT((newEltSz >= eltSz && (newEltSz % eltSz) == 0) || (newEltSz < eltSz && (eltSz % newEltSz) == 0));
  if (var->IsImmediate()) {
    if (newEltSz == eltSz)
      bitCast = ImmToVariable(var->GetImmediateValue(), newType);
    else {
      // Need a temp. For example,  bitcast i64 0 -> 2xi32
      CVariable *tmp = GetNewVariable(1, var->GetType(), CEncoder::GetCISADataTypeAlignment(var->GetType()), true, 1,
                                      "vecImmBitCast");
      encoder.Copy(tmp, var);
      encoder.Push();

      bitCast = GetNewAlias(tmp, newType, 0, 0);
    }
  } else {
    // TODO: we need to store this bitCasted var to avoid creating many times
    bitCast = GetNewAlias(var, newType, 0, 0);
  }
  return bitCast;
}

CVariable *CShader::ImmToVariable(uint64_t immediate, VISA_Type type, bool isCodePatchCandidate) {
  VISA_Type immType = type;

  if (type == ISA_TYPE_BOOL) {
    // bool immediates cannot be inlined
    uint immediateValue = immediate ? 0xFFFFFFFF : 0;
    CVariable *immVar = new (Allocator) CVariable(immediateValue, ISA_TYPE_UD);
    // src-variable is no longer a boolean, V-ISA cannot take boolean-src immed.

    CVariable *dst = GetNewVariable(numLanes(m_State.m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE, CName::NONE);
    // FIXME: We need to pop/push the encoder context
    // encoder.save();
    if (isCodePatchCandidate) {
      encoder.SetPayloadSectionAsPrimary();
    }
    encoder.SetP(dst, immVar);
    encoder.Push();
    if (isCodePatchCandidate) {
      encoder.SetPayloadSectionAsSecondary();
    }
    return dst;
  }

  CVariable *var = new (Allocator) CVariable(immediate, immType);
  return var;
}

CVariable *CShader::GetNewVariable(uint16_t nbElement, VISA_Type type, e_alignment align, UniformArgWrap isUniform,
                                   uint16_t numberInstance, const CName &name) {
  e_varType varType;
  if (type == ISA_TYPE_BOOL) {
    varType = EVARTYPE_PREDICATE;
  } else {
    IGC_ASSERT(align >= CEncoder::GetCISADataTypeAlignment(type));
    varType = EVARTYPE_GENERAL;
  }
  CVariable *var = new (Allocator) CVariable(nbElement, isUniform, type, varType, align, false, numberInstance, name);
  encoder.CreateVISAVar(var);
  return var;
}

CVariable *CShader::GetNewVariable(const CVariable *from, const CName &name) {
  CVariable *var = new (Allocator) CVariable(*from, name);
  encoder.CreateVISAVar(var);
  return var;
}

CVariable *CShader::GetNewAddressVariable(uint16_t nbElement, VISA_Type type, UniformArgWrap isUniform,
                                          bool isVectorUniform, const CName &name) {
  CVariable *var =
      new (Allocator) CVariable(nbElement, isUniform, type, EVARTYPE_ADDRESS, EALIGN_DWORD, isVectorUniform, 1, name);
  encoder.CreateVISAVar(var);
  return var;
}

WIBaseClass::WIDependancy CShader::GetDependency(Value *v) const {
  return m_WI ? (m_WI->whichDepend(v)) : WIBaseClass::RANDOM;
}

void CShader::SetDependency(llvm::Value *v, WIBaseClass::WIDependancy dep) {
  if (m_WI)
    m_WI->incUpdateDepend(v, dep);
}

bool CShader::GetIsUniform(llvm::Value *v) const { return m_WI ? (m_WI->isUniform(v)) : false; }

bool CShader::InsideDivergentCF(const llvm::Instruction *inst) const {
  return m_WI ? m_WI->insideDivergentCF(inst) : true;
}

bool CShader::InsideWorkgroupDivergentCF(const llvm::Instruction *inst) const {
  return m_WI ? m_WI->insideWorkgroupDivergentCF(inst) : true;
}

uint CShader::GetNbVectorElementAndMask(llvm::Value *val, uint32_t &mask) {
  llvm::Type *type = val->getType();
  uint nbElement = int_cast<uint>(cast<IGCLLVM::FixedVectorType>(type)->getNumElements());
  mask = 0;
  // we don't process vector bigger than 31 elements as the mask has only 32bits
  // If we want to support longer vectors we need to extend the mask size
  //
  // If val has been coalesced, don't prune it.
  if (IsCoalesced(val) || nbElement > 31) {
    return nbElement;
  }
  bool gpgpuPreemptionWANeeded =
      ((GetShaderType() == ShaderType::OPENCL_SHADER) || (GetShaderType() == ShaderType::COMPUTE_SHADER)) &&
      (m_SIMDSize == SIMDMode::SIMD8) && m_Platform->WaSamplerResponseLengthMustBeGreaterThan1() &&
      m_Platform->supportGPGPUMidThreadPreemption();

  if (llvm::GenIntrinsicInst *inst = llvm::dyn_cast<GenIntrinsicInst>(val)) {
    // try to prune the destination size
    GenISAIntrinsic::ID IID = inst->getIntrinsicID();
    if (IID == GenISAIntrinsic::GenISA_ldstructured || IID == GenISAIntrinsic::GenISA_typedread ||
        IID == GenISAIntrinsic::GenISA_typedreadMS) {
      // prune with write-mask if possible
      uint elemCnt = 0;
      for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I) {
        if (llvm::ExtractElementInst *extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I)) {
          if (llvm::ConstantInt *index = llvm::dyn_cast<ConstantInt>(extract->getIndexOperand())) {
            elemCnt++;
            IGC_ASSERT(index->getZExtValue() < 5);
            mask |= (1 << index->getZExtValue());
            continue;
          }
        }
        // if the vector is accessed by anything else than direct Extract we
        // cannot prune it
        elemCnt = nbElement;
        mask = 0;
        break;
      }

      if (mask) {
        nbElement = elemCnt;
      }
    } else if (isSampleInstruction(inst) || isLdInstruction(inst) || isInfoInstruction(inst)) {
      // sampler can return selected channel ony with extra header, when
      // returning only 1~2 channels, it suppose to have better performance.
      uint nbExtract = 0, maxIndex = 0;
      uint8_t maskExtract = 0;
      bool allExtract = true;

      for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I) {
        ExtractElementInst *extract = llvm::dyn_cast<ExtractElementInst>(*I);
        if (extract != nullptr) {
          llvm::ConstantInt *indexVal;
          indexVal = llvm::dyn_cast<ConstantInt>(extract->getIndexOperand());
          if (indexVal != nullptr) {
            uint index = static_cast<uint>(indexVal->getZExtValue());
            maxIndex = std::max(maxIndex, index + 1);

            maskExtract |= (1 << index);
            nbExtract++;
          } else {
            // if extractlement with dynamic index
            maxIndex = nbElement;
            allExtract = false;
            break;
          }
        } else {
          // if the vector is accessed by anything else than direct Extract we
          // cannot prune it
          maxIndex = nbElement;
          allExtract = false;
          break;
        }
      }

      // TODO: there are some issues in EmitVISAPass prevents enabling
      // selected channel return for info intrinsics.
      if (!allExtract || gpgpuPreemptionWANeeded || IGC_IS_FLAG_DISABLED(EnableSamplerChannelReturn) ||
          isInfoInstruction(inst) || maskExtract > 0xf) {
        if (gpgpuPreemptionWANeeded) {
          maxIndex = std::max((uint)2, maxIndex);
        }

        mask = BIT(maxIndex) - 1;
        nbElement = maxIndex;
      } else {
        // based on return channels, decide whether do partial
        // return with addtional header
        static const bool selectReturnChannels[] = {
            false, // 0 0000 - should not happen
            false, // 1 0001 - r
            false, // 2 0010 -  g
            false, // 3 0011 - rg
            true,  // 4 0100 -   b
            false, // 5 0101 - r b
            false, // 6 0110 -  gb
            false, // 7 0111 - rgb
            true,  // 8 1000 -    a
            true,  // 9 1001 - r  a
            true,  // a 1010 -  g a
            false, // b 1011 - rg a
            true,  // c 1100 -   ba
            false, // d 1101 - r ba
            false, // e 1110 -  gba
            false  // f 1111 - rgba
        };
        IGC_ASSERT(maskExtract != 0);
        IGC_ASSERT(maskExtract <= 0xf);

        if (selectReturnChannels[maskExtract]) {
          mask = maskExtract;
          nbElement = nbExtract;
        } else {
          mask = BIT(maxIndex) - 1;
          nbElement = maxIndex;
        }
      }
    } else {
      GenISAIntrinsic::ID IID = inst->getIntrinsicID();
      if (isLdInstruction(inst) || IID == GenISAIntrinsic::GenISA_URBRead ||
          IID == GenISAIntrinsic::GenISA_URBReadOutput || IID == GenISAIntrinsic::GenISA_DCL_ShaderInputVec ||
          IID == GenISAIntrinsic::GenISA_DCL_HSinputVec) {
        // prune without write-mask
        uint maxIndex = 0;
        for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I) {
          if (llvm::ExtractElementInst *extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I)) {
            if (llvm::ConstantInt *index = llvm::dyn_cast<ConstantInt>(extract->getIndexOperand())) {
              maxIndex = std::max(maxIndex, static_cast<uint>(index->getZExtValue()) + 1);
              continue;
            }
          }
          // if the vector is accessed by anything else than direct Extract we
          // cannot prune it
          maxIndex = nbElement;
          break;
        }
        // Non-transposed LSC load messages support only 1, 2, 3, 4 and
        // 8 element vectors. Transposed loads also support 16, 32 and 64.
        if (m_Platform->hasLSCUrbMessage() &&
            (IID == GenISAIntrinsic::GenISA_URBRead || IID == GenISAIntrinsic::GenISA_URBReadOutput)) {
          maxIndex = maxIndex > 4 ? iSTD::RoundPower2((DWORD)maxIndex) : maxIndex;
        }

        mask = BIT(maxIndex) - 1;
        nbElement = maxIndex;
      }
    }
  } else if (llvm::BitCastInst *inst = dyn_cast<BitCastInst>(val)) {
    for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I) {
      if (llvm::ExtractElementInst *extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I)) {
        if (llvm::ConstantInt *index = llvm::dyn_cast<ConstantInt>(extract->getIndexOperand())) {
          uint indexBit = BIT(static_cast<uint>(index->getZExtValue()));
          mask |= indexBit;
          continue;
        }
      }
      mask = BIT(nbElement) - 1;
      break;
    }
    if (mask) {
      nbElement = iSTD::BitCount(mask);
    }
  } else if (auto *LD = dyn_cast<LoadInst>(val)) {
    do {
      if (shouldGenerateLSC(LD))
        break;
      Value *Ptr = LD->getPointerOperand();
      PointerType *PtrTy = cast<PointerType>(Ptr->getType());
      bool useA32 = !IGC::isA64Ptr(PtrTy, GetContext());

      Type *Ty = LD->getType();
      IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
      Type *eltTy = VTy ? VTy->getElementType() : Ty;
      uint32_t eltBytes = GetScalarTypeSizeInRegister(eltTy);
      // Skip if not 32-bit load.
      if (eltBytes != 4)
        break;

      uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
      uint32_t totalBytes = eltBytes * elts;

      auto align = IGCLLVM::getAlignmentValue(LD);

      uint bufferIndex = 0;
      bool directIndexing = false;
      BufferType bufType = DecodeAS4GFXResource(PtrTy->getAddressSpace(), directIndexing, bufferIndex);
      // Some driver describe constant buffer as typed which forces us to use
      // byte scatter message.
      bool forceByteScatteredRW = (bufType == CONSTANT_BUFFER) && UsesTypedConstantBuffer(GetContext(), bufType);

      // Keep this check consistent in emitpass.
      if (bufType == STATELESS_A32)
        break;

      // Keep this check consistent in emitpass.
      if (totalBytes < 4)
        break;

      // Keep this check consistent in emitpass.
      if (GetIsUniform(Ptr))
        break;

      VectorMessage VecMessInfo(this);
      VecMessInfo.getInfo(Ty, align, useA32, forceByteScatteredRW);

      // Skip if non-trival case or gather4 won't be used. So far, only
      // VectorMessage::MESSAGE_A32_UNTYPED_SURFACE_RW is considered.
      if (VecMessInfo.numInsts != 1 || VecMessInfo.insts[0].kind != VectorMessage::MESSAGE_A32_UNTYPED_SURFACE_RW)
        break;

      for (auto *User : LD->users()) {
        auto *EEI = dyn_cast<ExtractElementInst>(User);
        auto *CI = EEI ? dyn_cast<ConstantInt>(EEI->getIndexOperand()) : nullptr;
        if (!CI) {
          // Don't populate any mask so that default one could be used instead.
          mask = 0;
          break;
        }
        mask |= BIT(unsigned(CI->getZExtValue()));
      }
      if (mask)
        nbElement = iSTD::BitCount(mask);
    } while (0);
  }
  return nbElement;
}

CShader::ExtractMaskWrapper::ExtractMaskWrapper(CShader *pS, Value *VecVal) {
  auto it = pS->extractMasks.find(VecVal);
  if (it != pS->extractMasks.end()) {
    m_hasEM = true;
    m_EM = it->second;
    return;
  }
  IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(VecVal->getType());
  const unsigned int numChannels = VTy ? (unsigned)VTy->getNumElements() : 1;
  if (numChannels <= 32) {
    m_hasEM = true;
    m_EM = (uint32_t)((1ULL << numChannels) - 1);
  } else {
    m_hasEM = false;
    m_EM = 0;
  }
}

uint16_t CShader::AdjustExtractIndex(llvm::Value *vecVal, uint16_t index) {
  const ExtractMaskWrapper EMW(this, vecVal);

  uint16_t result = index;
  if (EMW.hasEM()) {
    IGC_ASSERT(index < 32);
    uint32_t mask = EMW.getEM();
    for (uint i = 0; i < index; ++i) {
      if ((mask & (1 << i)) == 0) {
        result--;
      }
    }
    return result;
  } else {
    return index;
  }
}

void CShader::GetSimdOffsetBase(CVariable *&pVar, bool dup) {
  encoder.SetSimdSize(SIMDMode::SIMD8);
  encoder.SetNoMask();
  encoder.Cast(pVar, ImmToVariable(0x76543210, ISA_TYPE_V));
  encoder.Push();

  if (m_State.m_dispatchSize >= SIMDMode::SIMD16) {
    encoder.SetSimdSize(SIMDMode::SIMD8);
    encoder.SetDstSubReg(8);
    encoder.SetNoMask();
    encoder.Add(pVar, pVar, ImmToVariable(8, ISA_TYPE_W));
    encoder.Push();
  }

  if (encoder.IsSecondHalf()) {
    if (!dup) {
      encoder.SetNoMask();
      encoder.Add(pVar, pVar, ImmToVariable(16, ISA_TYPE_W));
      encoder.Push();
    }
  } else if (m_SIMDSize == SIMDMode::SIMD32) {
    // (W) add (16) V1(16) V1(0) 16:w
    encoder.SetSimdSize(SIMDMode::SIMD16);
    encoder.SetNoMask();
    encoder.SetDstSubReg(16);
    if (dup)
      encoder.Copy(pVar, pVar);
    else
      encoder.Add(pVar, pVar, ImmToVariable(16, ISA_TYPE_W));
    encoder.Push();
  }
}

CVariable *CShader::GetPerLaneOffsetsReg(uint typeSizeInBytes) {
  CVariable *pPerLaneOffsetsRaw = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_UW, EALIGN_GRF, "PerLaneOffsetsRaw");
  GetSimdOffsetBase(pPerLaneOffsetsRaw);

  // per-lane offsets need to be added to address register
  CVariable *pConst2 = ImmToVariable(typeSizeInBytes, ISA_TYPE_UW);

  CVariable *pPerLaneOffsetsReg =
      GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_UW, EALIGN_GRF, false, "PerLaneOffsetsRawReg");

  // perLaneOffsets = 4 * perLaneOffsetsRaw
  encoder.SetNoMask();
  encoder.Mul(pPerLaneOffsetsReg, pPerLaneOffsetsRaw, pConst2);
  encoder.Push();

  return pPerLaneOffsetsReg;
}

void CShader::CreatePayload(uint regCount, uint idxOffset, CVariable *&payload, llvm::Instruction *inst,
                            uint paramOffset, uint8_t hfFactor) {
  for (uint i = 0; i < regCount; ++i) {
    uint subVarIdx = ((numLanes(m_SIMDSize) / (getGRFSize() >> 2)) >> hfFactor) * i + idxOffset;
    CopyVariable(payload, GetSymbol(inst->getOperand(i + paramOffset)), subVarIdx);
  }
}

unsigned CShader::GetIMEReturnPayloadSize(GenIntrinsicInst *I) {
  IGC_ASSERT(I->getIntrinsicID() == GenISAIntrinsic::GenISA_vmeSendIME2);

  const auto streamMode = (COMMON_ISA_VME_STREAM_MODE)(cast<ConstantInt>(I->getArgOperand(4))->getZExtValue());
  auto *refImgBTI = I->getArgOperand(2);
  auto *bwdRefImgBTI = I->getArgOperand(3);
  const bool isDualRef = (refImgBTI != bwdRefImgBTI);

  uint32_t regs2rcv = 7;
  if ((streamMode == VME_STREAM_OUT) || (streamMode == VME_STREAM_IN_OUT)) {
    regs2rcv += 2;
    if (isDualRef) {
      regs2rcv += 2;
    }
  }
  return regs2rcv;
}

uint CShader::GetNbElementAndMask(llvm::Value *value, uint32_t &mask) {
  mask = 0;
  // Special case for VME's GenISA_createMessagePhases intrinsic
  if (GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(value)) {
    GenISAIntrinsic::ID IID = inst->getIntrinsicID();
    switch (IID) {
    case GenISAIntrinsic::GenISA_createMessagePhases:
    case GenISAIntrinsic::GenISA_createMessagePhasesNoInit:
    case GenISAIntrinsic::GenISA_createMessagePhasesV:
    case GenISAIntrinsic::GenISA_createMessagePhasesNoInitV: {
      Value *numGRFs = inst->getArgOperand(0);
      IGC_ASSERT_MESSAGE(isa<ConstantInt>(numGRFs), "Number GRFs operand is expected to be constant int!");
      // Number elements = {num GRFs} * {num DWords in GRF} = {num GRFs} * 8;
      return int_cast<unsigned int>(cast<ConstantInt>(numGRFs)->getZExtValue() * 8);
    }
    case GenISAIntrinsic::GenISA_LSC2DBlockRead:
    case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload: {
       // 2D block read requires its block size to be multiple of GRF size.
      uint32_t eltBits, blkWidth, blkHeight, numBlks;
      bool isTranspose, isVnni;
      if (IID == GenISAIntrinsic::GenISA_LSC2DBlockRead) {
        eltBits = (uint32_t)cast<ConstantInt>(inst->getOperand(6))->getZExtValue();
        blkWidth = (uint32_t)cast<ConstantInt>(inst->getOperand(7))->getZExtValue();
        blkHeight = (uint32_t)cast<ConstantInt>(inst->getOperand(8))->getZExtValue();
        numBlks = (uint32_t)cast<ConstantInt>(inst->getOperand(9))->getZExtValue();
        isTranspose = (uint)cast<ConstantInt>(inst->getOperand(10))->getZExtValue();
        isVnni = (uint)cast<ConstantInt>(inst->getOperand(11))->getZExtValue();
      } else {
        IGC_ASSERT(IID == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload);
        eltBits = (uint32_t)cast<ConstantInt>(inst->getOperand(3))->getZExtValue();
        blkWidth = (uint32_t)cast<ConstantInt>(inst->getOperand(4))->getZExtValue();
        blkHeight = (uint32_t)cast<ConstantInt>(inst->getOperand(5))->getZExtValue();
        numBlks = (uint32_t)cast<ConstantInt>(inst->getOperand(6))->getZExtValue();
        isTranspose = cast<ConstantInt>(inst->getOperand(7))->getZExtValue();
        isVnni = cast<ConstantInt>(inst->getOperand(8))->getZExtValue();
      }

      // Width is padded to the next power-of-2 value
      uint32_t blkWidthCeil = (uint32_t)PowerOf2Ceil(blkWidth);
      if (blkWidthCeil != blkWidth) {
        m_ctx->EmitWarning("Block2D: block width not power of 2, zero padded.");
      }
      uint32_t blkHeightCeil = blkHeight;
      if (isTranspose) {
        blkHeightCeil = (uint32_t)PowerOf2Ceil(blkHeight);
        if (blkHeightCeil != blkHeight) {
          m_ctx->EmitWarning("Block2D: transpose block height not power of 2, zero padded.");
        }
      }
      if (isVnni) {
        IGC_ASSERT(eltBits == 16 || eltBits == 8);
        uint32_t N = 32 / eltBits;
        uint32_t origVal = blkHeightCeil;
        blkHeightCeil = (uint32_t)divideCeil(blkHeightCeil, N) * N;
        if (blkHeightCeil != origVal) {
          m_ctx->EmitWarning("Block2D: transform block height not multiple "
                             "of N (32/eltBits), zero padded.");
        }
      }
      uint32_t blkBits = blkWidthCeil * blkHeightCeil * eltBits;
      uint32_t numGRFsPerBlk = (uint32_t)divideCeil(blkBits, getGRFSize() * 8);
      uint32_t blkBitsCeil = getGRFSize() * 8 * numGRFsPerBlk;
      if (blkBitsCeil != blkBits) {
        m_ctx->EmitWarning("Block2D: block size not multiple of GRF size, zero padded.");
      }
      uint32_t numGRFs = numGRFsPerBlk * numBlks;
      VISA_Type visaTy = GetType(inst->getType());
      uint32_t eltTyBytes = CEncoder::GetCISADataTypeSize(visaTy);
      uint32_t nbElement = (uint32_t)divideCeil(numGRFs * getGRFSize(), eltTyBytes);

      return nbElement;
    }
    default:
      break;
    }
  } else if (auto *PN = dyn_cast<PHINode>(value)) {
    // We could have case like below that payload is undef on some path.
    //
    // BB1:
    //   %147 = call i32 @llvm.genx.GenISA.createMessagePhasesNoInit(i32 11)
    //   call void @llvm.genx.GenISA.vmeSendIME2(i32 % 147, ...)
    //   br label %BB2
    // BB2:
    //   ... = phi i32[%147, %BB1], [0, %BB]
    //
    for (uint i = 0, e = PN->getNumOperands(); i != e; ++i) {
      if (GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(PN->getOperand(i))) {
        GenISAIntrinsic::ID IID = inst->getIntrinsicID();
        switch (IID) {
        case GenISAIntrinsic::GenISA_createMessagePhases:
        case GenISAIntrinsic::GenISA_createMessagePhasesNoInit:
        case GenISAIntrinsic::GenISA_createMessagePhasesV:
        case GenISAIntrinsic::GenISA_createMessagePhasesNoInitV:
          return GetNbElementAndMask(inst, mask);
        default:
          break;
        }
      }
    }
  }

  uint nbElement = 0;
  uint bSize = 0;
  llvm::Type *const type = value->getType();
  IGC_ASSERT(nullptr != type);
  switch (type->getTypeID()) {
  case llvm::Type::FloatTyID:
  case llvm::Type::HalfTyID:
  case llvm::Type::BFloatTyID:
    nbElement = GetIsUniform(value) ? 1 : numLanes(m_SIMDSize);
    break;
  case llvm::Type::IntegerTyID:
    bSize = llvm::cast<llvm::IntegerType>(type)->getBitWidth();
    nbElement = GetIsUniform(value) ? 1 : numLanes(m_SIMDSize);
    if (bSize == 1 && !m_CG->canEmitAsUniformBool(value)) {
      nbElement = numLanes(m_SIMDSize);
    }
    break;
  case IGCLLVM::VectorTyID: {
    uint nElem = GetNbVectorElementAndMask(value, mask);
    nbElement = GetIsUniform(value) ? nElem : (nElem * numLanes(m_SIMDSize));
  } break;
  case llvm::Type::PointerTyID:
    // Assumes 32-bit pointers
    nbElement = GetIsUniform(value) ? 1 : numLanes(m_SIMDSize);
    break;
  case llvm::Type::DoubleTyID:
    nbElement = GetIsUniform(value) ? 1 : numLanes(m_SIMDSize);
    break;
  default:
    IGC_ASSERT(0);
    break;
  }
  return nbElement;
}

CVariable *CShader::GetUndef(VISA_Type type) {
  CVariable *var = nullptr;
  if (type == ISA_TYPE_BOOL) {
    var = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_BOOL, EALIGN_BYTE, "undef");
  } else {
    var = new (Allocator) CVariable(type);
  }
  return var;
}

// TODO: Obviously, lots of works are needed to support constant expression
// better.
uint64_t CShader::GetConstantExpr(ConstantExpr *CE) {
  IGC_ASSERT(nullptr != CE);
  switch (CE->getOpcode()) {
  default:
    break;
  case Instruction::IntToPtr: {
    Constant *C = CE->getOperand(0);
    if (isa<ConstantInt>(C) || isa<ConstantFP>(C) || isa<ConstantPointerNull>(C))
      return GetImmediateVal(C);
    if (ConstantExpr *CE1 = dyn_cast<ConstantExpr>(C))
      return GetConstantExpr(CE1);
    break;
  }
  case Instruction::PtrToInt: {
    Constant *C = CE->getOperand(0);
    if (ConstantExpr *CE1 = dyn_cast<ConstantExpr>(C))
      return GetConstantExpr(CE1);
    if (GlobalVariable *GV = dyn_cast<GlobalVariable>(C))
      return GetSLMMappingValue(GV);
    break;
  }
  case Instruction::Trunc: {
    Constant *C = CE->getOperand(0);
    if (ConstantExpr *CE1 = dyn_cast<ConstantExpr>(C)) {
      if (IntegerType *ITy = dyn_cast<IntegerType>(CE1->getType())) {
        return GetConstantExpr(CE1) & ITy->getBitMask();
      }
    }
    break;
  }
  case Instruction::LShr: {
    Constant *C = CE->getOperand(0);
    if (ConstantExpr *CE1 = dyn_cast<ConstantExpr>(C)) {
      if (dyn_cast<IntegerType>(CE1->getType())) {
        uint64_t ShAmt = GetImmediateVal(CE->getOperand(1));
        return GetConstantExpr(CE1) >> ShAmt;
      }
    }
    break;
  }
  }

  IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported constant expression!");
  return 0;
}

unsigned int CShader::GetSLMMappingValue(llvm::Value *c) {
  IGC_ASSERT_MESSAGE(0, "The global variables are not handled");

  return 0;
}

CVariable *CShader::GetSLMMapping(llvm::Value *c) {
  IGC_ASSERT_MESSAGE(0, "The global variables are not handled");

  VISA_Type type = GetType(c->getType());
  return ImmToVariable(0, type);
}

CVariable *CShader::GetScalarConstant(llvm::Value *const c) {
  IGC_ASSERT(nullptr != c);
  const VISA_Type type = GetType(c->getType());

  // Constants
  if (isa<ConstantInt>(c) || isa<ConstantFP>(c) || isa<ConstantPointerNull>(c)) {
    return ImmToVariable(GetImmediateVal(c), type);
  }

  // Undefined values
  if (isa<UndefValue>(c)) {
    return GetUndef(type);
  }

  // GlobalVariables
  if (isa<GlobalVariable>(c)) {
    return GetSLMMapping(c);
  }

  // Constant Expression
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(c))
    return ImmToVariable(GetConstantExpr(CE), type);

  IGC_ASSERT_MESSAGE(0, "Unhandled flavor of constant!");
  return 0;
}

// Return true if can be encoded as mini float and return the encoding in value
static bool getByteFloatEncoding(ConstantFP *fp, uint8_t &value) {
  value = 0;
  if (fp->getType()->isFloatTy()) {
    if (fp->isZero()) {
      value = fp->isNegative() ? 0x80 : 0;
      return true;
    }
    APInt api = fp->getValueAPF().bitcastToAPInt();
    FLOAT32 bitFloat;
    bitFloat.value.u = int_cast<unsigned int>(api.getZExtValue());
    // check that fraction doesn't have any bots set below bit 23 - 4
    // Byte float can only encode the higer 4 bits of the fraction
    if ((bitFloat.fraction & (~(0xF << (23 - 4)))) == 0 && ((bitFloat.exponent > 124 && bitFloat.exponent <= 131) ||
                                                            (bitFloat.exponent == 124 && bitFloat.fraction != 0))) {
      // convert to float 8bits format
      value |= bitFloat.sign << 7;
      value |= (bitFloat.fraction >> (23 - 4));
      value |= (bitFloat.exponent & 0x3) << 4;
      value |= (bitFloat.exponent & BIT(7)) >> 1;
      return true;
    }
  }
  return false;
}

// Return the most commonly used constant. Return null if all constant are
// different.
llvm::Constant *CShader::findCommonConstant(llvm::Constant *C, uint elts, uint currentEmitElts, bool &allSame) {
  if (elts == 1) {
    return nullptr;
  }

  llvm::MapVector<llvm::Constant *, int> constMap;
  constMap.clear();
  Constant *constC = nullptr;
  bool cannotPackVF = !m_ctx->platform.hasPackedRestrictedFloatVector();
  for (uint32_t i = currentEmitElts; i < currentEmitElts + elts; i++) {
    constC = C->getAggregateElement(i);
    if (!constC) {
      return nullptr;
    }
    constMap[constC]++;

    // check if the constant can be packed in vf.
    if (!isa<UndefValue>(constC) && elts >= 4) {
      llvm::VectorType *VTy = llvm::cast<llvm::VectorType>(C->getType());
      uint8_t encoding = 0;
      if (VTy->getScalarType()->isFloatTy() && !getByteFloatEncoding(cast<ConstantFP>(constC), encoding)) {
        cannotPackVF = true;
      }
    }
  }
  int mostUsedCount = 1;
  Constant *mostUsedValue = nullptr;
  for (auto iter = constMap.begin(); iter != constMap.end(); iter++) {
    if (iter->second > mostUsedCount) {
      mostUsedValue = iter->first;
      mostUsedCount = iter->second;
    }
  }

  constMap.clear();
  allSame = (mostUsedCount == elts);

  if (allSame) {
    return mostUsedValue;
  } else if (mostUsedCount > 1 && cannotPackVF) {
    return mostUsedValue;
  } else {
    return nullptr;
  }
}

auto sizeToSIMDMode = [](uint32_t size) {
  switch (size) {
  case 1:
    return SIMDMode::SIMD1;
  case 2:
    return SIMDMode::SIMD2;
  case 4:
    return SIMDMode::SIMD4;
  case 8:
    return SIMDMode::SIMD8;
  case 16:
    return SIMDMode::SIMD16;
  default:
    IGC_ASSERT_MESSAGE(0, "unexpected simd size");
    return SIMDMode::SIMD1;
  }
};

// Struct layout in GRF
//   The description is based on the assumption that insertvalue/extractvalue
//   emit functions have on struct layout.
//
//   A struct is laid out in SOA form, just like a vector. It starts from the
//   1st member and crosses all simd lanes; then goes to 2nd member and crosses
//   all simd lanes; and continues until the last member. The consecutive
//   members are no longer consecutive and are laid out strided in GRF.
//
//   Take the following example, assuming SIMD16.
//      struct {
//        i8    a;
//        i16   b;
//        i32   c;
//      } V;
//   where assuming that V is aligned at 4 bytes and that each type has natural
//   alignment. Thus, there is 1 byte gap between 'a' and 'b' as 'b' must be
//   aligned at 2 bytes. The total size will be
//     sizeof(V) * 16 (simd16) = 8 * 16 = 128 bytes of GRF.
//
//    0 1 2 3 4 ...15      29 30 32 31    (GRF bytes)
//    ================================
//    a a a a a ...a   <16 bytes gap>     // a: 16 bytes
//    b   b   b            b     b        // b: 32 bytes
//    c       c            c              // c: 64 bytes
//    c       c            c
//
//  Note: if a struct is packed, there is no gap in GRF, but it could have
//        a mis-aligned member that needs to be handled specially in
//        insertvalue/extractvalue emit functions.
//
//        Also, this does not apply to how to pass struct to callee as the
//        calling convension will control that.
//
//  Besides, the following rules apply for now:
//    1.  types of struct elements cannot be aggregate (no array, no struct)
//        except special layout struct. Layout struct can have struct-typed
//        member.
//    2.  a member of vector type are laid out like a normal vector type
//        (Can also be viewed as spliting vector into consecutive scalar
//         elements. 4xi32 -> {i32, i32, i32, i32})
//
CVariable *CShader::GetStructVariable(llvm::Value *v) {
  IGC_ASSERT(v->getType()->isStructTy());

  IGC_ASSERT_MESSAGE(isa<Constant>(v) || isa<InsertValueInst>(v) || isa<CallInst>(v) || isa<Argument>(v) ||
                         isa<PHINode>(v) || isa<SelectInst>(v),
                     "Invalid instruction using struct type!");

  if (isa<InsertValueInst>(v)) {
    if (IGC_IS_FLAG_ENABLED(EnableDeSSA) && m_deSSA) {
      e_alignment pAlign = EALIGN_GRF;
      Value *rVal = m_deSSA->getRootValue(v, &pAlign);

      // If a struct type is coalesced with another non-struct type,
      // need to call createAliasIfNeeded().  Otherwise, all coalesced
      // structs are of the same type (Byte).
      if (rVal && !rVal->getType()->isStructTy()) {
        CVariable *rootV = GetSymbol(rVal);
        return createAliasIfNeeded(v, rootV);
      }

      v = rVal ? rVal : v;
      auto it = symbolMapping.find(v);
      if (it != symbolMapping.end()) {
        return it->second;
      }
    } else {
      // Note: liveness is not merged when coalescing insertValue's operand 0
      // with its dst.
      //       This would have incorrect liveness info.
      //
      // Walk up all `insertvalue` instructions until we get to the constant
      // base struct. All `insertvalue` instructions that operate on the same
      // struct should be mapped to the same CVar, so just use the first
      // instruction to do all the mapping. If operand 0 isn't the sole user,
      // stop; otherwise it might coalesce two structs whose live range
      // overlaps.
      Value *baseV = v;
      InsertValueInst *FirstInsertValueInst = nullptr;
      while (InsertValueInst *II = dyn_cast_or_null<InsertValueInst>(baseV)) {
        baseV = II->getOperand(0);
        FirstInsertValueInst = II;
        if (!baseV->hasOneUse()) {
          baseV = nullptr;
        }
      }
      if (FirstInsertValueInst) {
        // Check if it's already created
        auto it = symbolMapping.find(FirstInsertValueInst);
        if (it != symbolMapping.end()) {
          return it->second;
        }
        v = FirstInsertValueInst;
      }
    }
  } else if (auto *SI = dyn_cast<SelectInst>(v)) {
    if (IGC_IS_FLAG_ENABLED(EnableDeSSA) && m_deSSA) {
      e_alignment pAlign = EALIGN_GRF;
      Value *rVal = m_deSSA->getRootValue(v, &pAlign);

      // If a struct type is coalesced with another non-struct type,
      // need to call createAliasIfNeeded().  Otherwise, all coalesced
      // structs are of the same type (Byte).
      if (rVal && !rVal->getType()->isStructTy()) {
        CVariable *rootV = GetSymbol(rVal);
        return createAliasIfNeeded(v, rootV);
      }

      v = rVal ? rVal : v;
      auto it = symbolMapping.find(v);
      if (it != symbolMapping.end()) {
        return it->second;
      }
    }
  } else if (isa<CallInst>(v) || isa<Argument>(v)) {
    // For now, special handling of bitcasttostruct intrinsic
    GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(v);
    if (IGC_IS_FLAG_ENABLED(EnableDeSSA) && m_deSSA && GII &&
        GII->getIntrinsicID() == GenISAIntrinsic::GenISA_bitcasttostruct) {
      e_alignment pAlign = EALIGN_GRF;
      Value *rVal = m_deSSA->getRootValue(v, &pAlign);
      if (rVal && rVal != v) {
        CVariable *rV = GetSymbol(rVal);
        CVariable *aliasV = GetNewAlias(rV, ISA_TYPE_B, 0, 0);
        return aliasV;
      }
    }
    // Check for function argument symbols, and return value from calls
    auto it = symbolMapping.find(v);
    if (it != symbolMapping.end()) {
      return it->second;
    }
  } else if (isa<Constant>(v)) {
    // Const cannot be mapped
    IGC_ASSERT(symbolMapping.find(v) == symbolMapping.end());
  }

  bool isUniform = m_WI->isUniform(v);
  const uint16_t Instances = isUniform ? 1 : m_numberInstance;
  StructType *sTy = cast<StructType>(v->getType());
  auto &DL = entry->getParent()->getDataLayout();
  const StructLayout *SL = DL.getStructLayout(sTy);

  // Represent the struct as a vector of BYTES
  unsigned structSizeInBytes = (unsigned)SL->getSizeInBytes();
  unsigned lanes = isUniform ? 1 : numLanes(m_SIMDSize);
  CVariable *cVar = GetNewVariable(structSizeInBytes * lanes, ISA_TYPE_B, EALIGN_GRF, isUniform, Instances, "StructV");

  // Initialize the struct default value if it has one
  Constant *C = dyn_cast<Constant>(v);
  if (C && !isa<UndefValue>(v)) {
    for (unsigned i = 0; i < sTy->getNumElements(); i++) {
      CVariable *elementSrc = GetSymbol(C->getAggregateElement(i));
      if (!elementSrc->IsUndef()) {
        unsigned elementOffset = (unsigned)SL->getElementOffset(i);
        Type *elementType = sTy->getElementType(i);
        m_EmitPass->emitMayUnalignedVectorCopy(cVar, elementOffset, elementSrc, 0, elementType);
      }
    }
  }

  // Map the original llvm value to this new CVar.
  // The original value cannot be const, since we cannot map them. They will
  // need to be initialized each time.
  if (!isa<Constant>(v))
    symbolMapping[v] = cVar;

  return cVar;
}

CVariable *CShader::GetConstant(llvm::Constant *C, CVariable *dstVar) {
  IGCLLVM::FixedVectorType *VTy = llvm::dyn_cast<IGCLLVM::FixedVectorType>(C->getType());
  if (VTy) { // Vector constant
    llvm::Type *eTy = VTy->getElementType();
    IGC_ASSERT_MESSAGE((VTy->getNumElements() < (UINT16_MAX)), "getNumElements more than 64k elements");
    uint16_t elts = (uint16_t)VTy->getNumElements();

    if (elts == 1) {
      llvm::Constant *const EC = C->getAggregateElement((uint)0);
      IGC_ASSERT_MESSAGE(nullptr != EC, "Vector Constant has no valid constant element!");
      return GetScalarConstant(EC);
    }

    // Emit a scalar move to load the element of index k.
    auto copyScalar = [this, C, eTy](int k, CVariable *Var) {
      Constant *const EC = C->getAggregateElement(k);
      IGC_ASSERT_MESSAGE(nullptr != EC, "Constant Vector: Invalid non-constant element!");
      if (isa<UndefValue>(EC))
        return;

      CVariable *eVal = GetScalarConstant(EC);
      if (Var->IsUniform()) {
        GetEncoder().SetDstSubReg(k);
      } else {
        auto input_size = GetScalarTypeSizeInRegister(eTy);
        Var = GetNewAlias(Var, Var->GetType(), k * input_size * numLanes(m_SIMDSize), 0);
      }
      GetEncoder().Copy(Var, eVal);
      GetEncoder().Push();
    };

    // Emit a simd4 move to load 4 byte float.
    auto copyV4 = [this](int k, uint32_t vfimm, CVariable *Var) {
      CVariable *Imm = ImmToVariable(vfimm, ISA_TYPE_VF);
      GetEncoder().SetUniformSIMDSize(SIMDMode::SIMD4);
      GetEncoder().SetDstSubReg(k);
      GetEncoder().Copy(Var, Imm);
      GetEncoder().Push();
    };

    if (dstVar != nullptr && !(dstVar->IsUniform())) {
      for (uint i = 0; i < elts; i++) {
        copyScalar(i, dstVar);
      }
      return dstVar;
    }

    CVariable *CVar = (dstVar == nullptr) ? GetNewVariable(elts, GetType(eTy), EALIGN_GRF, true, C->getName()) : dstVar;
    uint remainElts = elts;
    uint currentEltsOffset = 0;
    uint size = 8;
    while (remainElts != 0) {
      bool allSame = 0;

      while (size > remainElts && size != 1) {
        size /= 2;
      }

      Constant *commonConstant = findCommonConstant(C, size, currentEltsOffset, allSame);
      // case 2: all constants the same
      if (commonConstant && allSame) {
        GetEncoder().SetUniformSIMDSize(sizeToSIMDMode(size));
        GetEncoder().SetDstSubReg(currentEltsOffset);
        GetEncoder().Copy(CVar, GetScalarConstant(commonConstant));
        GetEncoder().Push();
      }

      // case 3: some constants the same
      else if (commonConstant) {
        GetEncoder().SetUniformSIMDSize(sizeToSIMDMode(size));
        GetEncoder().SetDstSubReg(currentEltsOffset);
        GetEncoder().Copy(CVar, GetScalarConstant(commonConstant));
        GetEncoder().Push();

        Constant *constC = nullptr;
        for (uint i = currentEltsOffset; i < currentEltsOffset + size; i++) {
          constC = C->getAggregateElement(i);
          if (constC != commonConstant && !isa<UndefValue>(constC)) {
            GetEncoder().SetDstSubReg(i);
            GetEncoder().Copy(CVar, GetScalarConstant(constC));
            GetEncoder().Push();
          }
        }
      }
      // case 4: VFPack
      else if (VTy->getScalarType()->isFloatTy() && size >= 4) {
        unsigned Step = 4;
        for (uint i = currentEltsOffset; i < currentEltsOffset + size; i += Step) {
          // pack into vf if possible.
          uint32_t vfimm = 0;
          bool canUseVF = m_ctx->platform.hasPackedRestrictedFloatVector();
          for (unsigned j = 0; j < Step && canUseVF; ++j) {
            Constant *EC = C->getAggregateElement(i + j);
            // Treat undef as 0.0f.
            if (isa<UndefValue>(EC))
              continue;
            uint8_t encoding = 0;
            canUseVF = getByteFloatEncoding(cast<ConstantFP>(EC), encoding);
            if (canUseVF) {
              uint32_t v = encoding;
              v <<= j * 8;
              vfimm |= v;
            } else {
              break;
            }
          }

          if (canUseVF) {
            copyV4(i, vfimm, CVar);
          } else {
            for (unsigned j = i; j < i + Step; ++j)
              copyScalar(j, CVar);
          }
        }
      }
      // case 5: single copy
      else {
        // Element-wise copy or trailing elements copy if partially packed.
        for (uint i = currentEltsOffset; i < currentEltsOffset + size; i++) {
          copyScalar(i, CVar);
        }
      }
      remainElts -= size;
      currentEltsOffset += size;
    }
    return CVar;
  }

  return GetScalarConstant(C);
}

VISA_Type IGC::GetType(llvm::Type *type, CodeGenContext *pContext) {
  IGC_ASSERT(nullptr != pContext);
  IGC_ASSERT(nullptr != type);

  switch (type->getTypeID()) {
  case llvm::Type::FloatTyID:
    return ISA_TYPE_F;
  case llvm::Type::IntegerTyID:
    switch (type->getIntegerBitWidth()) {
    case 1:
      return ISA_TYPE_BOOL;
    case 8:
      return ISA_TYPE_B;
    case 16:
      return ISA_TYPE_W;
    case 32:
      return ISA_TYPE_D;
    case 64:
      return ISA_TYPE_Q;
    default:
      IGC_ASSERT_MESSAGE(0, "illegal type");
      break;
    }
    break;
  case IGCLLVM::VectorTyID:
    return GetType(type->getContainedType(0), pContext);
  case llvm::Type::PointerTyID: {
    unsigned int AS = type->getPointerAddressSpace();
    uint numBits = pContext->getRegisterPointerSizeInBits(AS);
    if (numBits == 32) {
      return ISA_TYPE_UD;
    } else {
      return ISA_TYPE_UQ;
    }
  }
  case llvm::Type::DoubleTyID:
    return ISA_TYPE_DF;
  case llvm::Type::HalfTyID:
    return ISA_TYPE_HF;
  case llvm::Type::BFloatTyID:
    return ISA_TYPE_BF;
  case llvm::Type::StructTyID:
    // Structs are always internally represented as BYTES
    return ISA_TYPE_B;
  default:
    IGC_ASSERT(0);
    break;
  }
  IGC_ASSERT(0);
  return ISA_TYPE_F;
}

VISA_Type CShader::GetType(llvm::Type *type) { return IGC::GetType(type, GetContext()); }

uint32_t CShader::GetNumElts(llvm::Type *type, bool isUniform) {
  uint32_t numElts = isUniform ? 1 : numLanes(m_SIMDSize);

  if (type->isVectorTy()) {
    IGC_ASSERT(type->getContainedType(0)->isIntegerTy() || type->getContainedType(0)->isFloatingPointTy());

    auto VT = cast<IGCLLVM::FixedVectorType>(type);
    numElts *= (uint16_t)VT->getNumElements();
  } else if (type->isStructTy()) {
    auto &DL = entry->getParent()->getDataLayout();
    const StructLayout *SL = DL.getStructLayout(cast<StructType>(type));
    numElts *= (uint16_t)SL->getSizeInBytes();
  }
  return numElts;
}

uint64_t IGC::GetImmediateVal(llvm::Value *Const) {
  // Constant integer
  if (llvm::ConstantInt *CInt = llvm::dyn_cast<llvm::ConstantInt>(Const)) {
    return CInt->getZExtValue();
  }

  // Constant float/double
  if (llvm::ConstantFP *CFP = llvm::dyn_cast<llvm::ConstantFP>(Const)) {
    APInt api = CFP->getValueAPF().bitcastToAPInt();
    return api.getZExtValue();
  }

  // Null pointer
  if (llvm::isa<ConstantPointerNull>(Const)) {
    return 0;
  }

  IGC_ASSERT_MESSAGE(0, "Unhandled constant value!");
  return 0;
}

/// IsRawAtomicIntrinsic - Check wether it's RAW atomic, which is optimized
/// potentially by scalarized atomic operation.
static bool IsRawAtomicIntrinsic(llvm::Value *V) {
  GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(V);
  if (!GII)
    return false;

  switch (GII->getIntrinsicID()) {
  default:
    break;
  case GenISAIntrinsic::GenISA_intatomicraw:
  case GenISAIntrinsic::GenISA_floatatomicraw:
  case GenISAIntrinsic::GenISA_intatomicrawA64:
  case GenISAIntrinsic::GenISA_floatatomicrawA64:
  case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
  case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
  case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
  case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
    return true;
  }

  return false;
}

/// GetPreferredAlignmentOnUse - Return preferred alignment based on how the
/// specified value is being used.
static e_alignment GetPreferredAlignmentOnUse(llvm::Value *V, WIAnalysis *WIA, CodeGenContext *pContext) {
  auto getAlign = [](Value *aV, WIAnalysis *aWIA, CodeGenContext *pCtx) -> e_alignment {
    // If uniform variables are once used by uniform loads, stores, or atomic
    // ops, they need being GRF aligned.
    for (auto UI = aV->user_begin(), UE = aV->user_end(); UI != UE; ++UI) {
      if (LoadInst *ST = dyn_cast<LoadInst>(*UI)) {
        Value *Ptr = ST->getPointerOperand();
        if (aWIA->isUniform(Ptr)) {
          if (IGC::isA64Ptr(cast<PointerType>(Ptr->getType()), pCtx))
            return (pCtx->platform.getGRFSize() == 64) ? EALIGN_64WORD : EALIGN_32WORD;
          return (pCtx->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD;
        }
      }
      if (StoreInst *ST = dyn_cast<StoreInst>(*UI)) {
        Value *Ptr = ST->getPointerOperand();
        if (aWIA->isUniform(Ptr)) {
          if (IGC::isA64Ptr(cast<PointerType>(Ptr->getType()), pCtx))
            return (pCtx->platform.getGRFSize() == 64) ? EALIGN_64WORD : EALIGN_32WORD;
          return (pCtx->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD;
        }
      }

      // Last, check Gen intrinsic.
      GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(*UI);
      if (!GII) {
        continue;
      }

      if (IsRawAtomicIntrinsic(GII)) {
        Value *Ptr = GII->getArgOperand(1);
        if (aWIA->isUniform(Ptr)) {
          if (PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType())) {
            if (IGC::isA64Ptr(PtrTy, pCtx))
              return (pCtx->platform.getGRFSize() == 64) ? EALIGN_64WORD : EALIGN_32WORD;
          }
          return (pCtx->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD;
        }
      }
      GenISAIntrinsic::ID gid = GII->getIntrinsicID();
      if (GII && (gid == GenISAIntrinsic::GenISA_dpas || gid == GenISAIntrinsic::GenISA_sub_group_dpas)) {
        // Only oprd1 could be uniform and its alignment could
        // be less than GRF. All the others are GRF-aligned.
        if (aV == GII->getArgOperand(1)) {
          ConstantInt *pa = cast<ConstantInt>(GII->getOperand(3)); // oprd1's precision
          ConstantInt *sdepth = cast<ConstantInt>(GII->getOperand(5));

          int PA = (int)pa->getSExtValue();
          int SD = (int)sdepth->getSExtValue();
          uint32_t bits = getPrecisionInBits((PrecisionType)PA);
          uint32_t OPS_PER_CHAN = (GII->getType()->isFloatTy() ? 2 : 4);
          bits = bits * OPS_PER_CHAN;
          bits = bits * SD;
          uint32_t NDWs = bits / 32;
          switch (NDWs) {
          default:
            break;
          case 2:
            return EALIGN_QWORD;
          case 4:
            return EALIGN_OWORD;
          case 8:
            return EALIGN_HWORD;
          }
        }
      }
    }
    return EALIGN_AUTO;
  };

  e_alignment algn = getAlign(V, WIA, pContext);
  if (algn != EALIGN_AUTO) {
    return algn;
  }

  if (IGC_IS_FLAG_ENABLED(EnableDeSSA)) {
    // Check if this V is used as load/store's address via
    // inttoptr that is actually noop (aliased by dessa already).
    //    x = ...
    //    y = inttoptr x
    //    load/store y
    // To make sure not to increase register pressure, only do it if y
    // is the sole use of x!
    if (V->hasOneUse()) {
      // todo: use deSSA->isNoopAliaser() to check if it has become an alias
      User *U = V->user_back();
      IntToPtrInst *IPtr = dyn_cast<IntToPtrInst>(U);
      if (IPtr && isNoOpInst(IPtr, pContext)) {
        algn = getAlign(IPtr, WIA, pContext);
        if (algn != EALIGN_AUTO) {
          return algn;
        }
      }
    }
  }

  // Otherwise, naturally aligned is always assumed.
  return EALIGN_AUTO;
}

/// GetPreferredAlignment - Return preferred alignment based on how the
/// specified value is being defined/used.
e_alignment IGC::GetPreferredAlignment(llvm::Value *V, WIAnalysis *WIA, CodeGenContext *pContext) {
  // So far, non-uniform variables are always naturally aligned.
  if (!WIA->isUniform(V))
    return EALIGN_AUTO;

  // As the layout of argument is fixed, only naturally aligned could be
  // assumed.
  if (isa<Argument>(V))
    return CEncoder::GetCISADataTypeAlignment(GetType(V->getType(), pContext));

  // For values not being mapped to variables directly, always assume
  // natually aligned.
  if (!isa<Instruction>(V))
    return EALIGN_AUTO;

  // If uniform variables are results from uniform loads, they need being GRF
  // aligned.
  if (LoadInst *LD = dyn_cast<LoadInst>(V)) {
    Value *Ptr = LD->getPointerOperand();
    // For 64-bit load, we have to check how the loaded value being used.
    e_alignment Align = (pContext->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD;
    if (IGC::isA64Ptr(cast<PointerType>(Ptr->getType()), pContext))
      Align = GetPreferredAlignmentOnUse(V, WIA, pContext);
    return (Align == EALIGN_AUTO) ? (pContext->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD : Align;
  }

  // If uniform variables are results from uniform atomic ops, they need
  // being GRF aligned.
  if (IsRawAtomicIntrinsic(V)) {
    GenIntrinsicInst *GII = cast<GenIntrinsicInst>(V);
    Value *Ptr = GII->getArgOperand(1);
    // For 64-bit atomic ops, we have to check how the return value being
    // used.
    e_alignment Align = (pContext->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD;
    if (PointerType *PtrTy = dyn_cast<PointerType>(Ptr->getType())) {
      if (IGC::isA64Ptr(PtrTy, pContext))
        Align = GetPreferredAlignmentOnUse(V, WIA, pContext);
    }
    return (Align == EALIGN_AUTO) ? (pContext->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD : Align;
  }

  if (GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(V)) {
    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_URBRead ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_URBReadOutput) {
      return pContext->platform.getGRFSize() == 64 ? EALIGN_32WORD : EALIGN_HWORD;
    }
  }

  // Check how that value is used.
  return GetPreferredAlignmentOnUse(V, WIA, pContext);
}

CVariable *CShader::LazyCreateCCTupleBackingVariable(CoalescingEngine::CCTuple *ccTuple, VISA_Type baseVisaType) {
  CVariable *var = NULL;
  auto it = ccTupleMapping.find(ccTuple);
  if (it != ccTupleMapping.end()) {
    var = ccTupleMapping[ccTuple];
  } else {
    auto mult = (m_SIMDSize == m_Platform->getMinDispatchMode()) ? 1 : 2;
    mult = CEncoder::GetCISADataTypeSize(baseVisaType) == 2 ? 1 : mult;
    unsigned int numRows = ccTuple->GetNumElements() * mult;
    const unsigned int denominator = CEncoder::GetCISADataTypeSize(ISA_TYPE_F);
    IGC_ASSERT(denominator);
    unsigned int numElts = numRows * getGRFSize() / denominator;

    // int size = numLanes(m_SIMDSize) * ccTuple->GetNumElements();
    if (ccTuple->HasNonHomogeneousElements()) {
      numElts += m_coalescingEngine->GetLeftReservedOffset(ccTuple->GetRoot(), m_SIMDSize) / denominator;
      numElts += m_coalescingEngine->GetRightReservedOffset(ccTuple->GetRoot(), m_SIMDSize) / denominator;
    }

    IGC_ASSERT_MESSAGE((numElts < (UINT16_MAX)), "tuple byte size higher than 64k");

    // create one
    var = GetNewVariable((uint16_t)numElts, ISA_TYPE_F,
                         (GetContext()->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD, false,
                         m_numberInstance, "CCTuple");
    ccTupleMapping.insert(std::pair<CoalescingEngine::CCTuple *, CVariable *>(ccTuple, var));
  }

  return var;
}

/// F should be a non-kernel function.
///
/// For a subroutine call, symbols (CVariables) are created as follows:
///
/// (1) If subroutine returns non-void value, then a unified return CVarable
/// is created to communicate between callee and caller. Function
/// 'getOrCreateReturnSymbol' creates such a unique symbol (CVariable)
/// on-demand. This return symbol is cached inside 'globalSymbolMapping'
/// object and it is *NOT* part of local symbol table 'symbolMapping'.
/// Currently return symbols are non-uniform.
///
/// (2) Subroutine formal arguments are also created on-demand, which may be
/// created from their first call sites or ahead of any call site. Symbols for
/// subroutine formal arguments are also stored inside 'globalSymbolMapping'
/// during entire module codegen. During each subroutine vISA emission,
/// value-to-symbol mapping are also copied into 'symbolMapping' to allow
/// EmitVISAPass to emit code in a uniform way.
///
/// In some sense, all formal arguments are pre-allocated. Those symbols must
/// be non-alias cvariable (ie root cvariable) as required by visa.
///
/// Currently, all explicit arguments are non-uniform and most implicit
/// arguments are uniform. Some implicit arguments may share the same symbol
/// with their caller's implicit argument of the same kind. This is a
/// subroutine optimization implemented in 'getOrCreateArgumentSymbol'.
///
void CShader::BeginFunction(llvm::Function *F) {
  // TODO: merge InitEncoder with this function.

  symbolMapping.clear();
  ccTupleMapping.clear();
  ConstantPool.clear();

  bool useStackCall = m_FGA && m_FGA->useStackCall(F);
  if (useStackCall) {
    // Clear cached variables.
    m_HW_TID = nullptr;

    globalSymbolMapping.clear();
    encoder.BeginStackFunction(F);
    // create pre-defined r0
    m_R0 = GetNewVariable(getGRFSize() / SIZE_DWORD, ISA_TYPE_D, EALIGN_GRF, false, 1, "R0");
    encoder.GetVISAPredefinedVar(m_R0, PREDEFINED_R0);
  } else {
    encoder.BeginSubroutine(F);
  }
  // Set already created symbols for formal arguments.
  for (auto &Arg : F->args()) {
    if (!Arg.use_empty()) {
      // the treatment of argument is more complex for subroutine and simpler
      // for stack-call function
      CVariable *Var = getOrCreateArgumentSymbol(&Arg, false, useStackCall);
      symbolMapping[&Arg] = Var;

      if (Value *Node = m_deSSA->getRootValue(&Arg)) {
        if (Node != (Value *)&Arg && symbolMapping.count(Node) == 0) {
          CVariable *aV = Var;
          if (IGC_IS_FLAG_ENABLED(EnableDeSSA)) {
            aV = createAliasIfNeeded(Node, Var);
          }
          symbolMapping[Node] = aV;
        }
      }
    }
  }

  CreateAliasVars();
  PreCompileFunction(*F);
}

// This method split payload interpolations from the shader into another
// compilation unit
void CShader::SplitPayloadFromShader(llvm::Function *F) { encoder.BeginPayloadSection(); }

/// This method is used to create the vISA variable for function F's formal
/// return value
CVariable *CShader::getOrCreateReturnSymbol(llvm::Function *F) {
  IGC_ASSERT_MESSAGE(nullptr != F, "null function");
  auto it = globalSymbolMapping.find(F);
  if (it != globalSymbolMapping.end()) {
    return it->second;
  }

  auto retType = F->getReturnType();
  IGC_ASSERT(nullptr != retType);
  if (F->isDeclaration() || retType->isVoidTy())
    return nullptr;

  IGC_ASSERT(retType->isSingleValueType() || retType->isStructTy());
  VISA_Type type = GetType(retType);
  uint16_t nElts = (uint16_t)GetNumElts(retType, false);
  e_alignment align = getGRFAlignment();
  CVariable *var = GetNewVariable(nElts, type, align, false, m_numberInstance, CName(F->getName(), "_RETVAL"));
  globalSymbolMapping.insert(std::make_pair(F, var));
  return var;
}

/// This method is used to create the vISA variable for function F's formal
/// argument
CVariable *CShader::getOrCreateArgumentSymbol(Argument *Arg, bool ArgInCallee, bool useStackCall) {
  llvm::DenseMap<llvm::Value *, CVariable *> *pSymMap = &globalSymbolMapping;
  IGC_ASSERT(nullptr != pSymMap);
  auto it = pSymMap->find(Arg);
  if (it != pSymMap->end()) {
    return it->second;
  }

  CVariable *var = nullptr;

  // Stack call does not use implicit args
  if (!useStackCall) {
    // An explicit argument is not uniform, and for an implicit argument, it
    // is predefined. Note that it is not necessarily uniform.
    Function *F = Arg->getParent();
    ImplicitArgs implicitArgs(*F, m_pMdUtils);
    unsigned numImplicitArgs = implicitArgs.size();
    unsigned numPushArgsEntry = m_ModuleMetadata->pushInfo.pushAnalysisWIInfos.size();
    unsigned numPushArgs = (isEntryFunc(m_pMdUtils, F) && !isNonEntryMultirateShader(F) ? numPushArgsEntry : 0);
    IGC_ASSERT_MESSAGE(F->arg_size() >= (numImplicitArgs + numPushArgs),
                       "Function arg size does not match meta data and push args.");
    unsigned numFuncArgs = F->arg_size() - numImplicitArgs - numPushArgs;

    llvm::Function::arg_iterator arg = std::next(F->arg_begin(), numFuncArgs);
    for (unsigned i = 0; i < numImplicitArgs; ++i, ++arg) {
      Argument *argVal = &(*arg);
      if (argVal == Arg) {
        ImplicitArg implictArg = implicitArgs[i];
        auto ArgType = implictArg.getArgType();

        // Just reuse the kernel arguments for the following.
        // Note that for read only general arguments, we may do similar
        // optimization, with some advanced analysis.
        if (ArgType == ImplicitArg::ArgType::R0 || ArgType == ImplicitArg::ArgType::PAYLOAD_HEADER ||
            ArgType == ImplicitArg::ArgType::GLOBAL_OFFSET || ArgType == ImplicitArg::ArgType::WORK_DIM ||
            ArgType == ImplicitArg::ArgType::NUM_GROUPS || ArgType == ImplicitArg::ArgType::GLOBAL_SIZE ||
            ArgType == ImplicitArg::ArgType::LOCAL_SIZE || ArgType == ImplicitArg::ArgType::ENQUEUED_LOCAL_WORK_SIZE ||
            ArgType == ImplicitArg::ArgType::CONSTANT_BASE || ArgType == ImplicitArg::ArgType::GLOBAL_BASE ||
            ArgType == ImplicitArg::ArgType::PRIVATE_BASE || ArgType == ImplicitArg::ArgType::PRINTF_BUFFER) {
          Function &K = *m_FGA->getSubGroupMap(F);
          ImplicitArgs IAs(K, m_pMdUtils);
          uint32_t nIAs = (uint32_t)IAs.size();
          uint32_t iArgIx = IAs.getArgIndex(ArgType);
          uint32_t argIx = (uint32_t)K.arg_size() - nIAs + iArgIx;
          if (isEntryFunc(m_pMdUtils, &K) && !isNonEntryMultirateShader(&K)) {
            argIx = argIx - numPushArgsEntry;
          }
          Function::arg_iterator arg = std::next(K.arg_begin(), argIx);
          Argument *kerArg = &(*arg);

          // Pre-condition: all kernel arguments have been created already.
          IGC_ASSERT(pSymMap->count(kerArg));
          return (*pSymMap)[kerArg];
        } else {
          bool isUniform = WIAnalysis::isDepUniform(implictArg.getDependency());
          uint16_t nbElements = (uint16_t)implictArg.getNumberElements();

          if (implictArg.isLocalIDs() && PVCLSCEnabled() && (m_SIMDSize == SIMDMode::SIMD32)) {
            nbElements = getGRFSize() / 2;
          }

          var = GetNewVariable(nbElements, implictArg.getVISAType(*m_DL), implictArg.getAlignType(*m_DL), isUniform,
                               isUniform ? 1 : m_numberInstance, argVal->getName());
        }
        break;
      }
    }
  }

  // This is not implicit.
  if (var == nullptr) {
    // GetPreferredAlignment treats all arguments as kernel ones, which have
    // predefined alignments; but this is not true for subroutines.
    // Conservatively use GRF aligned.
    e_alignment align = getGRFAlignment();

    bool isUniform = false;
    if (!ArgInCallee) {
      // Arg is for the current function and m_WI is available
      isUniform = m_WI->isUniform(&*Arg);
    }

    VISA_Type type = GetType(Arg->getType());
    uint16_t nElts = (uint16_t)GetNumElts(Arg->getType(), isUniform);
    var = GetNewVariable(nElts, type, align, isUniform, m_numberInstance, Arg->getName());
  }
  pSymMap->insert(std::make_pair(Arg, var));
  return var;
}

void CShader::UpdateSymbolMap(llvm::Value *v, CVariable *CVar) { symbolMapping[v] = CVar; }

// Reuse a varable in the following case
// %x = op1...
// %y = op2 (%x, ...)
// with some constraints:
// - %x and %y belong to the same block
// - %x and %y do not live out of this block
// - %x does not interfere with %y
// - %x is not phi
// - %y has no phi use
// - %x and %y have the same uniformity, and the same size
// - %x is not an alias
// - alignment is OK
//
CVariable *CShader::reuseSourceVar(Instruction *UseInst, Instruction *DefInst, e_alignment preferredAlign) {
  // Only when DefInst has been assigned a CVar.
  IGC_ASSERT(nullptr != DefInst);
  IGC_ASSERT(nullptr != UseInst);
  auto It = symbolMapping.find(DefInst);
  if (It == symbolMapping.end())
    return nullptr;

  // If the def is an alias/immediate, then do not reuse.
  // TODO: allow alias.
  CVariable *DefVar = It->second;
  if (DefVar->GetAlias() || DefVar->IsImmediate())
    return nullptr;

  // LLVM IR level checks and RPE based heuristics.
  if (!m_VRA->checkDefInst(DefInst, UseInst, m_deSSA->getLiveVars()))
    return nullptr;

  // Do not reuse when variable size exceeds the threshold.
  //
  // TODO: If vISA global RA can better deal with fragmentation, this will
  // become unnecessary.
  //
  // TODO: Remove this check if register pressure is low, or very high.
  //
  unsigned Threshold = IGC_GET_FLAG_VALUE(VariableReuseByteSize);
  if (DefVar->GetSize() > Threshold)
    return nullptr;

  // Only reuse when they have the same uniformness.
  if (GetIsUniform(UseInst) != GetIsUniform(DefInst))
    return nullptr;

  // Check alignments. If UseInst has a stricter alignment then do not reuse.
  e_alignment DefAlign = DefVar->GetAlign();
  e_alignment UseAlign = preferredAlign;
  if (DefAlign == EALIGN_AUTO) {
    VISA_Type Ty = GetType(DefInst->getType());
    DefAlign = CEncoder::GetCISADataTypeAlignment(Ty);
  }
  if (UseAlign == EALIGN_AUTO) {
    VISA_Type Ty = GetType(UseInst->getType());
    UseAlign = CEncoder::GetCISADataTypeAlignment(Ty);
  }
  if (UseAlign > DefAlign)
    return nullptr;

  // Reuse this source when types match.
  if (DefInst->getType() == UseInst->getType()) {
    return DefVar;
  }

  // Check cast instructions and create an alias if necessary.
  if (CastInst *CI = dyn_cast<CastInst>(UseInst)) {
    VISA_Type UseTy = GetType(UseInst->getType());
    if (UseTy == DefVar->GetType()) {
      return DefVar;
    }

    if (encoder.GetCISADataTypeSize(UseTy) != encoder.GetCISADataTypeSize(DefVar->GetType())) {
      // trunc/zext is needed, reuse not possible
      // this extra check is needed because in code gen we implicitly convert
      // all private pointers to 32-bit when LLVM assumes it's 64-bit based on
      // DL
      return nullptr;
    }

    // TODO: allow %y = trunc i32 %x to i8
    IGC_ASSERT(CI->isNoopCast(*m_DL));
    return GetNewAlias(DefVar, UseTy, 0, 0);
  }

  // No reuse yet.
  return nullptr;
  ;
}

CVariable *CShader::GetSymbolFromSource(Instruction *UseInst, e_alignment preferredAlign) {
  if (UseInst->isBinaryOp() || isa<SelectInst>(UseInst)) {
    if (!m_VRA->checkUseInst(UseInst, m_deSSA->getLiveVars()))
      return nullptr;

    for (unsigned i = 0; i < UseInst->getNumOperands(); ++i) {
      Value *Opnd = UseInst->getOperand(i);
      auto DefInst = dyn_cast<Instruction>(Opnd);
      // Only for non-uniform binary instructions.
      if (!DefInst || GetIsUniform(DefInst))
        continue;

      if (IsCoalesced(DefInst)) {
        continue;
      }

      CVariable *Var = reuseSourceVar(UseInst, DefInst, preferredAlign);
      if (Var)
        return Var;
    }
    return nullptr;
  } else if (auto CI = dyn_cast<CastInst>(UseInst)) {
    if (!m_VRA->checkUseInst(UseInst, m_deSSA->getLiveVars()))
      return nullptr;

    Value *Opnd = UseInst->getOperand(0);
    auto DefInst = dyn_cast<Instruction>(Opnd);
    if (!DefInst)
      return nullptr;

    if (!IsCoalesced(DefInst)) {
      return nullptr;
    }

    // TODO: allow %y = trunc i32 %x to i16
    if (!CI->isNoopCast(*m_DL))
      return nullptr;

    // WA: vISA does not optimize the following reuse well yet.
    // %398 = bitcast i16 %vCastload to <2 x i8>
    // produces
    // mov (16) r7.0<1>:w r18.0<2;1,0>:w
    // mov (16) r7.0<1>:b r7.0<2;1,0>:b
    // mov (16) r20.0<1>:f r7.0<8;8,1>:ub
    // not
    // mov (16) r7.0<1>:w r18.0<2;1,0>:w
    // mov (16) r20.0<1>:f r7.0<2;1,0>:ub
    //
    if (CI->getOpcode() == Instruction::BitCast) {
      if (GetScalarTypeSizeInRegisterInBits(CI->getSrcTy()) != GetScalarTypeSizeInRegisterInBits(CI->getDestTy()))
        return nullptr;
    }

    return reuseSourceVar(UseInst, DefInst, preferredAlign);
  }

  // TODO, allow insert element/value, gep, intrinsic calls etc..
  //
  // No source for reuse.
  return nullptr;
}

unsigned int CShader::EvaluateSIMDConstExpr(Value *C) {
  if (BinaryOperator *op = dyn_cast<BinaryOperator>(C)) {
    switch (op->getOpcode()) {
    case Instruction::Add:
      return EvaluateSIMDConstExpr(op->getOperand(0)) + EvaluateSIMDConstExpr(op->getOperand(1));
    case Instruction::Sub:
      return EvaluateSIMDConstExpr(op->getOperand(0)) - EvaluateSIMDConstExpr(op->getOperand(1));
    case Instruction::Mul:
      return EvaluateSIMDConstExpr(op->getOperand(0)) * EvaluateSIMDConstExpr(op->getOperand(1));
    case Instruction::Shl:
      return EvaluateSIMDConstExpr(op->getOperand(0)) << EvaluateSIMDConstExpr(op->getOperand(1));
    case Instruction::And:
      return EvaluateSIMDConstExpr(op->getOperand(0)) & EvaluateSIMDConstExpr(op->getOperand(1));
    case Instruction::Or:
      return EvaluateSIMDConstExpr(op->getOperand(0)) | EvaluateSIMDConstExpr(op->getOperand(1));
    default:
      break;
    }
  }
  if (llvm::GenIntrinsicInst *genInst = dyn_cast<GenIntrinsicInst>(C)) {
    if (genInst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdSize) {
      return numLanes(m_State.m_dispatchSize);
    }
  }
  if (ConstantInt *constValue = dyn_cast<ConstantInt>(C)) {
    return (unsigned int)constValue->getZExtValue();
  }
  IGC_ASSERT_MESSAGE(0, "unknown SIMD constant expression");
  return 0;
}

// MinAlign: if set (meaning not EALIGN_AUTO), select the larger of this
//           and 'preferredAlign'.
CVariable *CShader::GetSymbol(llvm::Value *value, bool fromConstantPool, e_alignment MinAlign) {
  auto it = symbolMapping.find(value);

  // mapping exists, return
  if (it != symbolMapping.end()) {
    return it->second;
  }

  CVariable *var = nullptr;

  // Symbol mappings for struct types
  if (value->getType()->isStructTy()) {
    return GetStructVariable(value);
  }

  if (Constant *C = llvm::dyn_cast<llvm::Constant>(value)) {
    // Check for function and global symbols
    {
      // Function Pointer
      auto isFunctionType = [this](Value *value) {
        return isa<GlobalValue>(value) && value->getType()->isPointerTy() &&
               cast<GlobalValue>(value)->getValueType()->isFunctionTy();
      };
      // Global Variable/Constant
      auto isGlobalVarType = [this](Value *value) {
        return isa<GlobalVariable>(value) &&
               m_ModuleMetadata->inlineProgramScopeOffsets.count(cast<GlobalVariable>(value)) > 0;
      };

      bool isVecType = value->getType()->isVectorTy();
      bool isFunction = false;
      bool isGlobalVar = false;

      if (isVecType) {
        Value *element = C->getAggregateElement((unsigned)0);
        if (isFunctionType(element))
          isFunction = true;
        else if (isGlobalVarType(element))
          isGlobalVar = true;
      } else if (isFunctionType(value)) {
        isFunction = true;
      } else if (isGlobalVarType(value)) {
        isGlobalVar = true;
      }

      if (isFunction || isGlobalVar) {
        auto it = symbolMapping.find(value);
        if (it != symbolMapping.end()) {
          return it->second;
        }
        const auto &valName = value->getName();
        if (isVecType) {
          // Map the entire vector value to the CVar
          unsigned numElements = (unsigned)cast<IGCLLVM::FixedVectorType>(value->getType())->getNumElements();
          var = GetNewVariable(numElements, ISA_TYPE_UQ,
                               (GetContext()->platform.getGRFSize() == 64) ? EALIGN_32WORD : EALIGN_HWORD,
                               WIBaseClass::UNIFORM_GLOBAL, 1, valName);
          symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, var));

          // Copy over each element
          for (unsigned i = 0; i < numElements; i++) {
            Value *element = C->getAggregateElement(i);
            CVariable *elementV = GetSymbol(element);
            CVariable *offsetV = GetNewAlias(var, ISA_TYPE_UQ, i * var->GetElemSize(), 1);
            encoder.Copy(offsetV, elementV);
            encoder.Push();
          }
          return var;
        } else {
          var = GetNewVariable(1, ISA_TYPE_UQ, EALIGN_QWORD, WIBaseClass::UNIFORM_GLOBAL, 1, valName);
          symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, var));
          return var;
        }
      }
    }

    if (fromConstantPool) {
      CVariable *cvar = ConstantPool.lookup(C);
      if (cvar)
        return cvar;
      // Generate constant initialization.
      SEncoderState S = encoder.CopyEncoderState();
      encoder.Push();
      cvar = GetConstant(C);
      if (!C->getType()->isVectorTy()) {
        CVariable *dst = GetNewVector(C);
        encoder.Copy(dst, cvar);
        encoder.Push();
        cvar = dst;
      }
      encoder.SetEncoderState(S);
      addConstantInPool(C, cvar);
      return cvar;
    }
    var = GetConstant(C);
    return var;
  }

  else if (Instruction *inst = dyn_cast<Instruction>(value)) {
    if (m_CG->SIMDConstExpr(inst)) {
      return ImmToVariable(EvaluateSIMDConstExpr(inst), ISA_TYPE_D);
    }
  }

  if (IGC_IS_FLAG_ENABLED(EnableDeSSA) && m_deSSA && value != m_deSSA->getNodeValue(value)) {
    // Generate CVariable alias.
    // Value and its aliasee must be of the same size.
    Value *nodeVal = m_deSSA->getNodeValue(value);
    IGC_ASSERT_MESSAGE(nodeVal != value, "ICE: value must be aliaser!");

    // For non node value, get symbol for node value first.
    // Then, get an alias to that node value.
    CVariable *Base = GetSymbol(nodeVal, false, MinAlign);
    CVariable *AliasVar = createAliasIfNeeded(value, Base);
    symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, AliasVar));
    return AliasVar;
  }

  if (!isa<InsertElementInst>(value) && value->hasOneUse()) {
    auto IEI = dyn_cast<InsertElementInst>(value->user_back());
    if (IEI && CanTreatScalarSourceAsAlias(IEI)) {
      CVariable *Var = GetSymbol(IEI, false, MinAlign);
      llvm::ConstantInt *Idx = llvm::cast<llvm::ConstantInt>(IEI->getOperand(2));
      unsigned short NumElts = 1;
      unsigned EltSz = CEncoder::GetCISADataTypeSize(GetType(IEI->getType()->getScalarType()));
      unsigned Offset = unsigned(Idx->getZExtValue() * EltSz);
      if (!Var->IsUniform()) {
        NumElts = numLanes(m_SIMDSize);
        Offset *= Var->getOffsetMultiplier() * numLanes(m_SIMDSize);
      }
      CVariable *Alias = GetNewAlias(Var, Var->GetType(), (uint16_t)Offset, NumElts);
      // FIXME: It makes no sense to map it as this `value` is
      // single-used implied from CanTreatScalarSourceAsAlias().
      symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, Alias));
      return Alias;
    }
  }

  if (llvm::ExtractElementInst *EEI = llvm::dyn_cast<ExtractElementInst>(value)) {
    if (CanTreatAsAlias(EEI)) {
      llvm::ConstantInt *const pConstElem = llvm::dyn_cast<llvm::ConstantInt>(EEI->getIndexOperand());
      IGC_ASSERT(nullptr != pConstElem);
      Value *vecOperand = EEI->getVectorOperand();
      // need to call GetSymbol() before AdjustExtractIndex(), since
      // GetSymbol may update mask of the vector operand.
      CVariable *vec = GetSymbol(vecOperand, false, MinAlign);

      uint element = AdjustExtractIndex(vecOperand, (uint16_t)pConstElem->getZExtValue());
      IGC_ASSERT_MESSAGE((element < (UINT16_MAX)), "ExtractElementInst element index > higher than 64k");

      // see if distinct CVariables were created during vector bitcast copy
      if (auto vectorBCI = dyn_cast<BitCastInst>(vecOperand)) {
        CVariable *EEIVar = getCVarForVectorBCI(vectorBCI, element);
        if (EEIVar) {
          return EEIVar;
        }
      }

      uint offset = 0;
      unsigned EltSz = CEncoder::GetCISADataTypeSize(GetType(EEI->getType()));
      if (GetIsUniform(EEI->getOperand(0))) {
        offset = int_cast<unsigned int>(element * EltSz);
      } else {
        offset = int_cast<unsigned int>(vec->getOffsetMultiplier() * element * numLanes(m_SIMDSize) * EltSz);
      }
      IGC_ASSERT_MESSAGE((offset < (UINT16_MAX)), "computed alias offset higher than 64k");

      // You'd expect the number of elements of the extracted variable to be
      // vec->GetNumberElement() /
      // vecOperand->getType()->getVectorNumElements(). However,
      // vec->GetNumberElement() is not always what you'd expect it to be
      // because of the pruning code in GetNbVectorElement(). So, recompute the
      // number of elements from scratch.
      uint16_t numElements = 1;
      if (!vec->IsUniform()) {
        numElements = numLanes(m_SIMDSize);
      }
      var = GetNewAlias(vec, vec->GetType(), (uint16_t)offset, numElements);
      symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, var));
      return var;
    }
  }

  if (GenIntrinsicInst *genInst = dyn_cast<GenIntrinsicInst>(value)) {
    if (VMECoalescePattern(genInst)) {
      auto *Sym = GetSymbol(genInst->getOperand(0), false, MinAlign);
      auto *Alias = GetNewAlias(Sym, Sym->GetType(), 0, Sym->GetNumberElement());
      symbolMapping.insert(std::pair<Value *, CVariable *>(value, Alias));
      return Alias;
    }
  }

  if (m_coalescingEngine) {
    CoalescingEngine::CCTuple *ccTuple = m_coalescingEngine->GetValueCCTupleMapping(value);
    if (ccTuple) {
      VISA_Type type = GetType(value->getType());
      CVariable *var = LazyCreateCCTupleBackingVariable(ccTuple, type);

      int mult = 1;
      if (CEncoder::GetCISADataTypeSize(type) == 2 && m_SIMDSize == SIMDMode::SIMD8) {
        mult = 2;
      }
      if (m_Platform->isCoreChildOf(IGFX_XE2_HPG_CORE) && value->getType()->isIntegerTy(16) &&
          m_SIMDSize == SIMDMode::SIMD16) {
        IGC_ASSERT(m_Platform->getGRFSize() == 64);
        mult = 2;
      }

      // FIXME: Could improve by copying types from value

      unsigned EltSz = CEncoder::GetCISADataTypeSize(type);
      int offset = int_cast<int>(mult * (m_coalescingEngine->GetValueOffsetInCCTuple(value) - ccTuple->GetLeftBound()) *
                                 numLanes(m_SIMDSize) * EltSz);

      if (ccTuple->HasNonHomogeneousElements()) {
        offset += m_coalescingEngine->GetLeftReservedOffset(ccTuple->GetRoot(), m_SIMDSize);
      }

      TODO("NumElements in this alias is 0 to preserve previous behavior. I "
           "have no idea what it should be.");
      IGC_ASSERT_MESSAGE((offset < (UINT16_MAX)), "alias offset > higher than 64k");
      CVariable *newVar = GetNewAlias(var, type, (uint16_t)offset, 0);
      symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, newVar));
      return newVar;
    }
  }

  // If we use a value which is not marked as needed by the pattern matching,
  // then something went wrong
  IGC_ASSERT(!isa<Instruction>(value) || isa<PHINode>(value) || m_CG->NeedInstruction(cast<Instruction>(*value)));

  e_alignment preferredAlign = GetPreferredAlignment(value, m_WI, GetContext());
  if (MinAlign != EALIGN_AUTO && (preferredAlign == EALIGN_AUTO || MinAlign > preferredAlign)) {
    preferredAlign = MinAlign;
  }

  // simple de-ssa, always creates a new svar, and return
  if (!m_deSSA) {
    var = GetNewVector(value, preferredAlign);
    symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, var));
    return var;
  }

  llvm::Value *rootValue = m_deSSA->getRootValue(value, &preferredAlign);
  // belong to a congruent class
  if (rootValue) {
    it = symbolMapping.find(rootValue);
    if (it != symbolMapping.end()) {
      var = it->second;
      CVariable *aV = var;
      if (IGC_IS_FLAG_ENABLED(EnableDeSSA)) {
        aV = createAliasIfNeeded(value, var);
      }
      symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, aV));
      /*
       *  When we don't scalarize vectors, vector may come from
       * phi/insert-element We cannot adjust extract-mask
       */
      if (value->getType()->isVectorTy()) {
        extractMasks.erase(value);
      }
      return aV;
    }
  }

  if (GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(value)) {
    switch (GII->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload: {
      // Address Payload is opaque, no alias to it. It takes 8DW.
      auto thisAlign = (m_Platform->getGRFSize() == 64 ? EALIGN_32WORD : EALIGN_HWORD);
      var = GetNewVariable(8, ISA_TYPE_D, thisAlign, WIBaseClass::UNIFORM_THREAD, value->getName());
      symbolMapping.insert(std::pair<Value *, CVariable *>(value, var));
      break;
    }
    default:
      break;
    }
  }

  if (!var && IGC_IS_FLAG_ENABLED(EnableVariableReuse)) {
    // Only for instructions and do not reuse flag variables.
    if (!value->getType()->getScalarType()->isIntegerTy(1)) {
      if (auto Inst = dyn_cast<Instruction>(value)) {
        // cannot reuse source variable
        // when the instruction is inside a resource-loop
        if (m_RLA->GetResourceLoopMarker(Inst) == ResourceLoopAnalysis::MarkResourceLoopOutside) {
          var = GetSymbolFromSource(Inst, preferredAlign);
        }
      }
    }
  }

  // need to create a new mapping
  if (!var) {
    // for @llvm.stacksave returned var must be created based on SP instead of
    // LLVM value
    llvm::IntrinsicInst *IntrinsicInstruction = dyn_cast<llvm::IntrinsicInst>(value);
    if (IntrinsicInstruction && IntrinsicInstruction->getIntrinsicID() == Intrinsic::stacksave && hasSP()) {
      auto pSP = GetSP();
      var =
          GetNewVariable(pSP->GetNumberElement(), pSP->GetType(), pSP->GetAlign(), pSP->IsUniform(), value->getName());
    }
    else {
      var = GetNewVector(value, preferredAlign);
    }
  }

  symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(value, var));
  if (rootValue && rootValue != value) {
    CVariable *aV = var;
    if (IGC_IS_FLAG_ENABLED(EnableDeSSA)) {
      aV = createAliasIfNeeded(rootValue, var);
    }
    symbolMapping.insert(std::pair<llvm::Value *, CVariable *>(rootValue, aV));
  }
  return var;
}

/// WHEN implement vector-coalescing, want to be more conservative in
/// treating extract-element as alias in order to reduce the complexity of
/// the problem
bool CShader::CanTreatAsAlias(llvm::ExtractElementInst *inst) {
  llvm::Value *idxSrc = inst->getIndexOperand();
  if (!isa<llvm::ConstantInt>(idxSrc)) {
    return false;
  }

  llvm::Value *vecSrc = inst->getVectorOperand();
  if (isa<llvm::InsertElementInst>(vecSrc)) {
    return false;
  }

  if (IsCoalesced(inst) || IsCoalesced(vecSrc)) {
    return false;
  }

  for (auto I = vecSrc->user_begin(), E = vecSrc->user_end(); I != E; ++I) {
    llvm::ExtractElementInst *extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I);
    if (!extract) {
      return false;
    }
    if (!isa<ConstantInt>(extract->getIndexOperand())) {
      return false;
    }
    // If there is another component not being treated as alias, this
    // component cannot be neither. This decision should be mitigated once
    // the VISA could track the liveness of individual elements of vector
    // variables.
    if (IsCoalesced(extract))
      return false;
  }
  return true;
}

static bool isUsedInPHINode(llvm::Instruction *I) {
  for (auto U : I->users()) {
    if (isa<PHINode>(U))
      return true;
    if (auto BC = dyn_cast<BitCastInst>(U)) {
      if (isUsedInPHINode(BC))
        return true;
    }
    if (auto IEI = dyn_cast<InsertElementInst>(U)) {
      if (isUsedInPHINode(IEI))
        return true;
    }
  }
  return false;
}

bool CShader::CanTreatScalarSourceAsAlias(llvm::InsertElementInst *IEI) {
  // Skip if it's not enabled.
  if (!IGC_IS_FLAG_ENABLED(EnableInsertElementScalarCoalescing))
    return false;
  // Skip if IEI is used in PHI.
  // FIXME: Should skip PHI if this IEI is from its backedge.
  if (isUsedInPHINode(IEI))
    return false;
  // Skip if the index is not constant.
  llvm::ConstantInt *IdxOp = dyn_cast<llvm::ConstantInt>(IEI->getOperand(2));
  if (!IdxOp)
    return false;
  // Skip if the scalar operand is not single-used.
  Value *ScalarOp = IEI->getOperand(1);
  if (!ScalarOp->hasOneUse())
    return false;
  // Skip if the scalar operand is not an instruction.
  if (!isa<llvm::Instruction>(ScalarOp))
    return false;
  // Skip the scalar operand may be treated as alias.
  if (llvm::dyn_cast<llvm::PHINode>(ScalarOp))
    return false;
  if (auto EEI = llvm::dyn_cast<llvm::ExtractElementInst>(ScalarOp)) {
    if (CanTreatAsAlias(EEI))
      return false;
  }
  auto Def = cast<llvm::Instruction>(ScalarOp);
  auto BB = Def->getParent();
  // Skip that scalar value is not defined locally.
  if (BB != IEI->getParent())
    return false;
  if (!m_deSSA)
    return isa<llvm::UndefValue>(IEI->getOperand(0));
  // Since we will define that vector element ahead from the previous
  // position, check whether such hoisting is safe.
  auto BI = std::prev(llvm::BasicBlock::reverse_iterator(IEI->getIterator()));
  auto BE = std::prev(llvm::BasicBlock::reverse_iterator(Def->getIterator()));
  auto Idx = IdxOp->getZExtValue();
  for (; BI != BE && BI != BB->rend(); ++BI) {
    if (&*BI != IEI)
      continue;
    Value *VecOp = IEI->getOperand(0);
    // If the source operand is `undef`, `insertelement` could be always
    // treated as alias (of the destination of the scalar operand).
    if (isa<UndefValue>(VecOp))
      return true;
    Value *SrcRoot = m_deSSA->getRootValue(VecOp);
    Value *DstRoot = m_deSSA->getRootValue(IEI);
    // `dst` vector will be copied from `src` vector if they won't coalese.
    // Hoisting this insertion is unsafe.
    if (SrcRoot != DstRoot)
      return false;
    IEI = dyn_cast<llvm::InsertElementInst>(VecOp);
    // However, if `src` is not defined through `insertelement`, it's still
    // unsafe to hoist this insertion.
    if (!IEI)
      return false;
    // If that's dynamically indexed insertion or insertion on the same
    // index, it's unsafe to hoist this insertion.
    llvm::ConstantInt *IdxOp = dyn_cast<llvm::ConstantInt>(IEI->getOperand(2));
    if (!IdxOp)
      return false;
    if (IdxOp->getZExtValue() == Idx)
      return false;
  }
  return true;
}

bool CShader::HasBecomeNoop(Instruction *inst) { return m_VRA->m_HasBecomeNoopInsts.count(inst); }

bool CShader::IsCoalesced(Value *V) {
  if ((m_VRA && m_VRA->isAliasedValue(V)) || (m_deSSA && m_deSSA->getRootValue(V)) ||
      (m_coalescingEngine && m_coalescingEngine->GetValueCCTupleMapping(V))) {
    return true;
  }
  return false;
}

#define SET_INTRINSICS()                                                                                               \
  GenISAIntrinsic::GenISA_setMessagePhaseX : case GenISAIntrinsic::GenISA_setMessagePhaseXV:                           \
  case GenISAIntrinsic::GenISA_setMessagePhase:                                                                        \
  case GenISAIntrinsic::GenISA_setMessagePhaseV:                                                                       \
  case GenISAIntrinsic::GenISA_simdSetMessagePhase:                                                                    \
  case GenISAIntrinsic::GenISA_simdSetMessagePhaseV

static bool IsSetMessageIntrinsic(GenIntrinsicInst *I) {
  switch (I->getIntrinsicID()) {
  case SET_INTRINSICS():
    return true;
  default:
    return false;
  }
}

bool CShader::VMECoalescePattern(GenIntrinsicInst *genInst) {
  if (!IsSetMessageIntrinsic(genInst))
    return false;

  if (IsCoalesced(genInst)) {
    return false;
  }

  if (GenIntrinsicInst *argInst = dyn_cast<GenIntrinsicInst>(genInst->getOperand(0))) {
    if (IsCoalesced(argInst)) {
      return false;
    }

    switch (argInst->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_createMessagePhases:
    case GenISAIntrinsic::GenISA_createMessagePhasesV:
    case GenISAIntrinsic::GenISA_createMessagePhasesNoInit:
    case GenISAIntrinsic::GenISA_createMessagePhasesNoInitV:
    case SET_INTRINSICS(): {
      bool OneUse = argInst->hasOneUse();

      if (OneUse) {
        return (argInst->getParent() == genInst->getParent());
      }

      // If we don't succeed in the quick check above, also match if there
      // is a single set intrinsic and all of the other users dominate the
      // set intrinsic in the block.

      SmallPtrSet<Value *, 4> Users(argInst->user_begin(), argInst->user_end());

      uint32_t SetMessageCnt = 0U;
      for (auto U : Users) {
        if (!isa<GenIntrinsicInst>(U))
          return false;

        auto *GII = cast<GenIntrinsicInst>(U);
        if (GII->getParent() != argInst->getParent())
          return false;

        if (IsSetMessageIntrinsic(GII))
          SetMessageCnt++;
      }

      if (SetMessageCnt > 1)
        return false;

      uint32_t NonSetInsts = Users.size() - SetMessageCnt;

      auto E = argInst->getParent()->end();
      for (auto I = argInst->getIterator(); I != E; I++) {
        if (Users.count(&*I) != 0) {
          if (IsSetMessageIntrinsic(cast<GenIntrinsicInst>(&*I))) {
            return false;
          } else {
            if (--NonSetInsts == 0)
              break;
          }
        }
      }

      return true;
    }
    default:
      return false;
    }
  }

  return false;
}

#undef SET_INTRINSICS

bool CShader::isUnpacked(llvm::Value *value) {
  bool isUnpacked = false;
  if (m_SIMDSize == m_Platform->getMinDispatchMode()) {
    if (isa<SampleIntrinsic>(value) || isa<LdmcsInstrinsic>(value)) {
      if (cast<VectorType>(value->getType())->getElementType()->isHalfTy() ||
          cast<VectorType>(value->getType())->getElementType()->isIntegerTy(16)) {
        isUnpacked = true;
        auto uses = value->user_begin();
        auto endUses = value->user_end();
        while (uses != endUses) {
          if (llvm::ExtractElementInst *extrElement = dyn_cast<llvm::ExtractElementInst>(*uses)) {
            if (CanTreatAsAlias(extrElement)) {
              ++uses;
              continue;
            }
          }
          isUnpacked = false;
          break;
        }
      }
    }
  }
  return isUnpacked;
}
/// GetNewVector
///
CVariable *CShader::GetNewVector(llvm::Value *value, e_alignment preferredAlign) {
  VISA_Type type = GetType(value->getType());
  WIBaseClass::WIDependancy dep = GetDependency(value);
  bool uniform = WIAnalysis::isDepUniform(dep);
  uint32_t mask = 0;
  bool isUnpackedBool = isUnpacked(value);
  uint8_t multiplier = (isUnpackedBool) ? 2 : 1;
  uint nElem = GetNbElementAndMask(value, mask) * multiplier;
  IGC_ASSERT_MESSAGE((nElem < (UINT16_MAX)), "getNumElements more than 64k elements");
  const uint16_t nbElement = (uint16_t)nElem;
  // TODO: Non-uniform variable should be naturally aligned instead of GRF
  // aligned. E.g., <8 x i16> should be aligned to 16B instead of 32B or GRF.
  e_alignment align = EALIGN_GRF;
  if (uniform) {
    // So far, preferredAlign is only applied to uniform variable.
    // TODO: Add preferred alignment for non-uniform variables.
    align = preferredAlign;
    if (align == EALIGN_AUTO)
      align = CEncoder::GetCISADataTypeAlignment(type);
  }
  uint16_t numberOfInstance = m_numberInstance;
  if (uniform) {
    if (type != ISA_TYPE_BOOL || m_CG->canEmitAsUniformBool(value)) {
      numberOfInstance = 1;
    }
  }
  if (mask) {
    extractMasks[value] = mask;
  }
  const auto &valueName = value->getName();
  CVariable *var = GetNewVariable(nbElement, type, align, dep, numberOfInstance, valueName);
  if (isUnpackedBool)
    var->setisUnpacked();
  return var;
}

/// GetNewAlias
CVariable *CShader::GetNewAlias(CVariable *var, VISA_Type type, uint16_t offset, uint16_t numElements) {
  IGC_ASSERT_MESSAGE(false == var->IsImmediate(), "Trying to create an alias of an immediate");
  CVariable *alias = new (Allocator) CVariable(var, type, offset, numElements, var->IsUniform());
  encoder.CreateVISAVar(alias);
  return alias;
}

// Create a multi-instance alias of a single-instance variable.
CVariable *CShader::GetNewAlias(CVariable *var, uint16_t numInstances) {
  IGC_ASSERT_MESSAGE(false == var->IsImmediate(), "Trying to create an alias of an immediate");
  IGC_ASSERT(var->GetNumberInstance() == 1);
  IGC_ASSERT(var->GetSingleInstanceAlias() == nullptr);
  IGC_ASSERT(var->GetAlias() == nullptr);
  IGC_ASSERT(numInstances > 1);
  CVariable *alias = new (Allocator) CVariable(var, numInstances);
  encoder.CreateVISAVar(alias);
  return alias;
}

CVariable *CShader::GetNewAliasWithAliasOffset(CVariable *var) {
  IGC_ASSERT_MESSAGE(false == var->IsImmediate(), "Trying to create an alias of an immediate");
  CVariable *rootVar = var->GetAlias();
  uint32_t offset = var->GetAliasOffset();
  IGC_ASSERT(rootVar && offset > 0);
  uint32_t rootSize = rootVar->GetSize();
  uint32_t eltSize = var->GetElemSize();
  uint32_t varSize = (rootSize > offset) ? rootSize - offset : 0;
  uint16_t nelts = varSize / eltSize;
  IGC_ASSERT_MESSAGE(nelts > 0, "Error: CVar has zero size!");
  CVariable *alias = new (Allocator) CVariable(rootVar, var->GetType(), offset, nelts, var->IsUniform());
  encoder.CreateVISAVar(alias, true);
  return alias;
}

// createAliasIfNeeded() returns the Var that is either BaseVar or
// its alias of the same size.
//
// If BaseVar's type matches V's, return BaseVar; otherwise, create an
// new alias CVariable to BaseVar. The new CVariable has V's size, which
// should not be larger than BaseVar's.
//
// Note that V's type is either vector or scalar.
CVariable *CShader::createAliasIfNeeded(Value *V, CVariable *BaseVar) {
  Type *Ty = V->getType();
  VectorType *VTy = dyn_cast<VectorType>(Ty);
  Type *BTy = VTy ? VTy->getElementType() : Ty;
  VISA_Type visaTy = GetType(BTy);
  if (visaTy == BaseVar->GetType()) {
    return BaseVar;
  }

  uint16_t visaTy_sz = CEncoder::GetCISADataTypeSize(visaTy);
  IGC_ASSERT(visaTy_sz);
  uint16_t nbe = BaseVar->GetSize() / visaTy_sz;
  IGC_ASSERT_MESSAGE((BaseVar->GetSize() % visaTy_sz) == 0, "V's Var should be the same size as BaseVar!");
  CVariable *NewAliasVar = GetNewAlias(BaseVar, visaTy, 0, nbe);
  return NewAliasVar;
}

/// GetNewAlias
CVariable *CShader::GetNewAlias(CVariable *var, VISA_Type type, uint16_t offset, uint16_t numElements, bool uniform) {
  IGC_ASSERT(nullptr != var);
  IGC_ASSERT_MESSAGE(false == var->IsImmediate(), "Trying to create an alias of an immediate");
  CVariable *alias = new (Allocator) CVariable(var, type, offset, numElements, uniform);
  encoder.CreateVISAVar(alias);
  return alias;
}

CVariable *CShader::GetVarHalf(CVariable *var, unsigned int half) {
  const char *lowOrHi = half == 0 ? "Lo" : "Hi";
  IGC_ASSERT(nullptr != var);
  IGC_ASSERT_MESSAGE(false == var->IsImmediate(), "Trying to create an alias of an immediate");
  CVariable *alias =
      new (Allocator) CVariable(var->GetNumberElement(), var->IsUniform(), var->GetType(), var->GetVarType(),
                                var->GetAlign(), var->IsVectorUniform(), 1, CName(var->getName(), lowOrHi));
  alias->visaGenVariable[0] = var->visaGenVariable[half];
  return alias;
}

void CShader::GetPayloadElementSymbols(llvm::Value *inst, CVariable *payload[], int vecWidth) {
  llvm::ConstantDataVector *cv = llvm::dyn_cast<llvm::ConstantDataVector>(inst);
  if (cv) {
    IGC_ASSERT(vecWidth == cv->getNumElements());
    for (int i = 0; i < vecWidth; ++i) {
      payload[i] = GetSymbol(cv->getElementAsConstant(i));
    }
    return;
  }

  llvm::InsertElementInst *ie = llvm::dyn_cast<llvm::InsertElementInst>(inst);
  IGC_ASSERT(nullptr != ie);

  for (int i = 0; i < vecWidth; ++i) {
    payload[i] = NULL;
  }

  int count = 0;
  // Gather elements of vector
  while (ie != NULL) {
    int64_t iOffset = llvm::cast<llvm::ConstantInt>(ie->getOperand(2))->getSExtValue();
    IGC_ASSERT(iOffset >= 0);
    IGC_ASSERT(iOffset < vecWidth);

    // Get the scalar value from this insert
    if (payload[iOffset] == NULL) {
      payload[iOffset] = GetSymbol(ie->getOperand(1));
      count++;
    }

    // Do we have another insert?
    llvm::Value *insertBase = ie->getOperand(0);
    ie = llvm::dyn_cast<llvm::InsertElementInst>(insertBase);
    if (ie != NULL) {
      continue;
    }

    if (llvm::isa<llvm::UndefValue>(insertBase)) {
      break;
    }
  }
  IGC_ASSERT(count == vecWidth);
}

void CShader::Destroy() {}

// Helper function to copy raw register
void CShader::CopyVariable(CVariable *dst, CVariable *src, uint dstSubVar, uint srcSubVar) {
  CVariable *rawDst = dst;
  // The source have to match for a raw copy
  if (src->GetType() != dst->GetType()) {
    rawDst = BitCast(dst, src->GetType());
  }
  encoder.SetSrcSubVar(0, srcSubVar);
  encoder.SetDstSubVar(dstSubVar);
  encoder.Copy(rawDst, src);
  encoder.Push();
}

// Helper function to copy and pack raw register
void CShader::PackAndCopyVariable(CVariable *dst, CVariable *src, uint subVar) {
  CVariable *rawDst = dst;
  // The source have to match for a raw copy
  if (src->GetType() != dst->GetType()) {
    rawDst = BitCast(dst, src->GetType());
  }
  encoder.SetDstSubVar(subVar);
  if (!src->IsUniform()) {
    encoder.SetSrcRegion(0, 16, 8, 2);
  }
  encoder.Copy(rawDst, src);
  encoder.Push();
}

// Copies entire variable using simd32 (Xe2) or simd16 (Xe), UD type and NoMask
void CShader::CopyVariableRaw(CVariable *dst, CVariable *src) {
  // handle cases with uniform variables
  VISA_Type dataType = dst->IsUniform() ? dst->GetType() : ISA_TYPE_UD;
  uint dataTypeSizeInBytes = CEncoder::GetCISADataTypeSize(dataType);
  uint offset = 0;
  uint bytesToCopy = src->GetSize() * src->GetNumberInstance();

  while (bytesToCopy > 0) {
    bool dstSeconfHalf = offset >= dst->GetSize();
    bool srcSeconfHalf = offset >= src->GetSize();
    encoder.SetSecondHalf(dstSeconfHalf || srcSeconfHalf);
    SIMDMode simdMode = dst->IsUniform() ? SIMDMode::SIMD1 : getGRFSize() == 64 ? SIMDMode::SIMD32 : SIMDMode::SIMD16;
    uint movSize = numLanes(simdMode) * dataTypeSizeInBytes;
    while (movSize > bytesToCopy || (!srcSeconfHalf && ((offset + movSize) > src->GetSize())) ||
           (!dstSeconfHalf && ((offset + movSize) > dst->GetSize()))) {
      simdMode = lanesToSIMDMode(numLanes(simdMode) / 2);
      movSize = numLanes(simdMode) * dataTypeSizeInBytes;
    }
    CVariable *dst0 =
        GetNewAlias(dst, dataType, dstSeconfHalf ? (offset - dst->GetSize()) : offset, numLanes(simdMode));
    CVariable *src0 =
        GetNewAlias(src, dataType, srcSeconfHalf ? (offset - src->GetSize()) : offset, numLanes(simdMode));
    encoder.SetSimdSize(simdMode);
    encoder.SetNoMask();
    encoder.Copy(dst0, src0);
    encoder.Push();
    encoder.SetSecondHalf(false);
    bytesToCopy -= movSize;
    offset += movSize;
  }
}

// Creates a new variable and copies entire src using simd16, UD type and
// NoMask. If `singleInstancde` is true the new variable is a single-instance
// variable.
CVariable *CShader::CopyVariableRaw(CVariable *src, bool singleInstance) {
  uint numInstance = src->GetNumberInstance();
  uint numElements = src->GetNumberElement();
  CName name(src->getName(), singleInstance ? "SingleInstanceCopy" : "Copy");
  CVariable *dst = GetNewVariable(singleInstance ? numElements * numInstance : numElements, src->GetType(),
                                  src->GetAlign(), src->IsUniform(), singleInstance ? 1 : numInstance, name);
  CopyVariableRaw(dst, src);
  return dst;
}

bool CShader::CompileSIMDSizeInCommon(SIMDMode simdMode) {
  const uint maxPerThreadScratchSpace = m_ctx->platform.maxPerThreadScratchSpace(
  );

  bool ret =
      ((m_ScratchSpaceSize <= maxPerThreadScratchSpace) || m_ctx->m_DriverInfo.supportsStatelessSpacePrivateMemory());

  m_simdProgram.setScratchSpaceUsedByShader(m_ScratchSpaceSize);

  if (SeparateSpillAndScratch(m_ctx)) {
    ret = ((m_simdProgram.getScratchSpaceUsageInSlot0() <= maxPerThreadScratchSpace) &&
           (m_simdProgram.getScratchSpaceUsageInSlot1() <= maxPerThreadScratchSpace));
  } else {
    ret = (m_simdProgram.getScratchSpaceUsageInSlot0() <= maxPerThreadScratchSpace);
  }

  if (ret && m_ctx->hasSyncRTCalls(entry)) {
    ret = (m_Platform->getMaxRayQuerySIMDSize(m_ctx->type) >= simdMode);
  }

  return ret;
}

uint32_t CShader::GetShaderThreadUsageRate() { return m_State.GetShaderThreadUsageRate(); }

CShaderProgram::CShaderProgram(CodeGenContext *ctx, llvm::Function *kernel)
    : m_shaderStats(nullptr), m_context(ctx), m_kernel(kernel), m_SIMDshaders() {}

CShaderProgram::~CShaderProgram() {
  for (auto &shader : m_SIMDshaders) {
    delete shader;
  }
  m_context = nullptr;
}

unsigned int CShader::GetPrimitiveTypeSizeInRegisterInBits(const Type *Ty) const {
  unsigned int sizeInBits = (unsigned int)Ty->getPrimitiveSizeInBits();
  if (Ty->isPtrOrPtrVectorTy()) {
    sizeInBits = GetContext()->getRegisterPointerSizeInBits(Ty->getPointerAddressSpace());
    if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty)) {
      sizeInBits *= (unsigned)VTy->getNumElements();
    }
  }
  return sizeInBits;
}

unsigned int CShader::GetPrimitiveTypeSizeInRegister(const Type *Ty) const {
  return GetPrimitiveTypeSizeInRegisterInBits(Ty) / 8;
}

unsigned int CShader::GetScalarTypeSizeInRegisterInBits(const Type *Ty) const {
  unsigned int sizeInBits = Ty->getScalarSizeInBits();
  if (Ty->isPtrOrPtrVectorTy()) {
    sizeInBits = GetContext()->getRegisterPointerSizeInBits(Ty->getPointerAddressSpace());
  }
  return sizeInBits;
}

unsigned int CShader::GetScalarTypeSizeInRegister(const Type *Ty) const {
  return GetScalarTypeSizeInRegisterInBits(Ty) / 8;
}

bool CShader::needsEntryFence() const {
  if (IGC_IS_FLAG_ENABLED(DisableEntryFences))
    return false;

  // Only RayTracing related shaders require the UGM fences at the beginning
  // of each shader for the A0 WA.
  if (!m_Platform->WaEnableLSCBackupMode())
    return false;

  auto *Ctx = GetContext();
  if (Ctx->type == ShaderType::RAYTRACING_SHADER) {
    auto *ModuleMD = Ctx->getModuleMetaData();
    auto FI = ModuleMD->FuncMD.find(entry);
    IGC_ASSERT_MESSAGE(FI != ModuleMD->FuncMD.end(), "Missing shader info!");
    return FI->second.functionType == IGC::CallableShader;
  }
  return false;
}

bool CShader::forceCacheCtrl(llvm::Instruction *inst) {
  const auto &list = m_ModuleMetadata->forceLscCacheList;
  const auto *PtrTy = dyn_cast<PointerType>(inst->getOperand(0)->getType());

  if (PtrTy) {
    const auto pos = list.find(PtrTy->getAddressSpace());

    if (pos != list.end()) {
      auto *node =
          MDNode::get(inst->getContext(),
                      ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(inst->getContext()), pos->second)));
      inst->setMetadata("lsc.cache.ctrl", node);
      return true;
    }
  }

  return false;
}

// This function may be used in earlier passes to determine whether a given
// instruction will generate an LSC message. If it returns Unknown or False, you
// should conservatively assume that you don't know what will be generated. If
// this returns True, it is guaranteed that an LSC message will result.
Tristate CShader::shouldGenerateLSCQuery(const CodeGenContext &Ctx, Instruction *vectorLdStInst, SIMDMode Mode) {
  auto &Platform = Ctx.platform;
  auto &DriverInfo = Ctx.m_DriverInfo;

  if (!Platform.LSCEnabled(Mode)) {
    // We enable LSC load/store only when program SIMD size is >= LSC's
    // simd size.  This is to avoid increasing register pressure and
    // reduce extra moves.
    // Note, that this only applies to gather/scatter;
    // for blocked messages we can always enable LSC
    return Tristate::False;
  }

  // Generate LSC for load/store instructions as Load/store emit can
  // handle full-payload uniform non-transpose LSC on PVC A0.
  if (vectorLdStInst == nullptr || isa<LoadInst>(vectorLdStInst) || isa<StoreInst>(vectorLdStInst))
    return Tristate::True;

  if (GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(vectorLdStInst)) {
    // Generate LSC for predicated load/store instructions similar to
    // simple load/store instructions.
    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_PredicatedLoad ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_PredicatedStore) {
      return Tristate::True;
    }

    // special checks for typed r/w
    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_typedread ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_typedwrite ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_typedwriteMS ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_typedreadMS ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_floatatomictyped ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_fcmpxchgatomictyped ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_intatomictyped ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_icmpxchgatomictyped) {
      return (Platform.hasLSCTypedMessage() ? Tristate::True : Tristate::False);
    } else if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_ldraw_indexed ||
               inst->getIntrinsicID() == GenISAIntrinsic::GenISA_ldrawvector_indexed ||
               inst->getIntrinsicID() == GenISAIntrinsic::GenISA_storeraw_indexed ||
               inst->getIntrinsicID() == GenISAIntrinsic::GenISA_storerawvector_indexed) {
      IGC_ASSERT(Platform.isProductChildOf(IGFX_DG2));
      IGC_ASSERT(Platform.hasLSC());

      bool Result = DriverInfo.EnableLSCForLdRawAndStoreRawOnDG2() || Platform.isCoreChildOf(IGFX_XE_HPC_CORE);

      return (Result ? Tristate::True : Tristate::False);
    }
  }

  // in PVC A0, SIMD1 reads/writes need full payloads
  // this causes chaos for vISA (would need 4REG alignment)
  // and to make extra moves to enable the payload
  // B0 gets this feature (there is no A1)
  if (!Platform.LSCSimd1NeedFullPayload()) {
    return Tristate::True;
  }

  return Tristate::Unknown;
}

// Note that if LSCEnabled() returns true, load/store instructions must be
// generated with LSC; but some intrinsics are still generated with legacy.
bool CShader::shouldGenerateLSC(llvm::Instruction *vectorLdStInst, bool isTGM) {
  if (vectorLdStInst && m_ctx->m_DriverInfo.SupportForceRouteAndCache() &&
      (!isTGM || m_ctx->platform.supportsNonDefaultLSCCacheSetting())) {
    // check if umd specified lsc caching mode and set the metadata if needed.
    if (forceCacheCtrl(vectorLdStInst)) {
      // if umd force the caching mode, also assume it wants the resource to be
      // in lsc.
      return true;
    }
  }

  if (auto result = shouldGenerateLSCQuery(*m_ctx, vectorLdStInst, m_SIMDSize); result != Tristate::Unknown)
    return (result == Tristate::True);

  // ensure both source and destination are not uniform
  Value *addrs = nullptr;
  if (GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(vectorLdStInst)) {
    addrs = inst->getOperand(0); // operand 0 is always addr for loads and stores
  } // else others?

  // we can generate LSC only if it's not uniform (SIMD1) or A32
  bool canGenerate = true;
  if (addrs) {
    bool isA32 = false; // TODO: see below
    if (PointerType *ptrType = dyn_cast<PointerType>(addrs->getType())) {
      isA32 = !IGC::isA64Ptr(ptrType, GetContext());
    }
    canGenerate &= isA32 || !GetSymbol(addrs)->IsUniform();

    if (!isA32 && GetSymbol(addrs)->IsUniform()) {
      // This is A64 and Uniform case. The LSC is not allowed.
      // However, before exit check the total bytes to be stored or loaded.
      if (totalBytesToStoreOrLoad(vectorLdStInst) >= 4) {
        canGenerate = true;
      }
    }
  }
  return canGenerate;
} // shouldGenerateLSC

uint32_t CShader::totalBytesToStoreOrLoad(llvm::Instruction *vectorLdStInst) {
  if (dyn_cast<LoadInst>(vectorLdStInst) || dyn_cast<StoreInst>(vectorLdStInst)) {
    Type *Ty = nullptr;
    if (LoadInst *inst = dyn_cast<LoadInst>(vectorLdStInst)) {
      Ty = inst->getType();
    } else if (StoreInst *inst = dyn_cast<StoreInst>(vectorLdStInst)) {
      Value *storedVal = inst->getValueOperand();
      Ty = storedVal->getType();
    }
    if (Ty) {
      IGCLLVM::FixedVectorType *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
      Type *eltTy = VTy ? VTy->getElementType() : Ty;
      uint32_t eltBytes = GetScalarTypeSizeInRegister(eltTy);
      uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
      return (eltBytes * elts);
    }
  }
  return 0;
} // totalBytesToStoreOrLoad

// getShaderFileName() returns the shader name that will be used to form a dump
// file name.
//   Input: shader name
//   Output: either the exact input or modified input.
void CShader::getShaderFileName(std::string &ShaderName) const {
  // Use shorter shader name except for some special shaders like the following
  // for readability:
  //    Symbol_Table_Void program
  //    entry
  if (GetContext()->dumpUseShorterName() && !IsIntelSymbolTableVoidProgram() && ShaderName != "entry") {
    ShaderName = GetContext()->getFunctionDumpName(getShaderProgramID());
    return;
  }

  // Special case for "entry", use empty name to keep the old behavior
  // unchanged.
  if (ShaderName == "entry") {
    ShaderName = "";
  }
  return;
}
