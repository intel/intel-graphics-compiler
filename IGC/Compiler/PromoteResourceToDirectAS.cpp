/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PromoteResourceToDirectAS.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/Type.h>
#include <llvmWrapper/Support/Alignment.h>
#include "common/IGCIRBuilder.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;

// Register pass to igc-opt
#define PASS_FLAG "igc-promote-resources-to-direct-addrspace"
#define PASS_DESCRIPTION "Pass promotes indirect addrspace resource access to direct addrspace"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteResourceToDirectAS, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PromoteResourceToDirectAS, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PromoteResourceToDirectAS::ID = 0;

PromoteResourceToDirectAS::PromoteResourceToDirectAS() : FunctionPass(ID) {
  initializePromoteResourceToDirectASPass(*PassRegistry::getPassRegistry());
}

bool PromoteResourceToDirectAS::runOnFunction(Function &F) {
  m_pCodeGenContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  visit(F);

  return true;
}

// Determine the new buffer type
Type *GetBufferAccessType(Instruction *inst) {
  if (LoadInst *load = dyn_cast<LoadInst>(inst)) {
    return load->getType();
  } else if (StoreInst *store = dyn_cast<StoreInst>(inst)) {
    return store->getOperand(0)->getType();
  } else if (GenIntrinsicInst *pIntr = dyn_cast<GenIntrinsicInst>(inst)) {
    switch (pIntr->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_storeraw_indexed:
    case GenISAIntrinsic::GenISA_storerawvector_indexed:
      return pIntr->getOperand(2)->getType();
    case GenISAIntrinsic::GenISA_ldrawvector_indexed:
    case GenISAIntrinsic::GenISA_ldraw_indexed:
    case GenISAIntrinsic::GenISA_intatomicraw:
    case GenISAIntrinsic::GenISA_intatomictyped:
    case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_floatatomicraw:
    case GenISAIntrinsic::GenISA_floatatomictyped:
    case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_intatomicrawA64:
    case GenISAIntrinsic::GenISA_floatatomicrawA64:
    case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
      return pIntr->getType();
    default:
      break;
    }
  }

  IGC_ASSERT_MESSAGE(0, "Unsupported buffer access intrinsic");
  return inst->getType();
}

Argument *FindArrayBaseArg(AllocaInst *alloca) {
  // Search for argument that is first element of this local array, starting from alloca.
  // This is pattern match and is relying on a way array is lowered.

  // First, find GEP taking first element of an array. It is assumed to be immediate user of alloca.
  Argument *arg = nullptr;
  GetElementPtrInst *baseGep = nullptr;
  for (Value::user_iterator use_it = alloca->user_begin(), use_e = alloca->user_end(); use_it != use_e; ++use_it) {
    if (auto gep = dyn_cast<GetElementPtrInst>(*use_it)) {
      if (gep->getNumIndices() == 2) {
        if (auto gepIndexValue = dyn_cast<llvm::ConstantInt>(gep->getOperand(2))) {
          if (gepIndexValue->getZExtValue() == 0) {
            // Pointer to first element found.
            baseGep = gep;
          }
        }
      }
    }
  }

  // The only user for this GEP should be a store, which is storing function argument to an array.
  // Note that this is assuming OCL approach, which is using KernelArgs.
  if (baseGep && baseGep->hasOneUse()) {
    if (auto store = dyn_cast<StoreInst>(*baseGep->user_begin())) {
      if (auto elem = dyn_cast<Argument>(store->getValueOperand())) {
        arg = elem;
      }
    }
  }

  return arg;
}

Value *FindArrayIndex(const std::vector<Value *> &instList, IGCIRBuilder<> &builder) {
  // Find GEP instruction in the list and get arrayIndex from it, depending on GEP type.
  Value *arrayIndex = nullptr;
  for (Value *V : instList) {
    if (auto gep = dyn_cast<GetElementPtrInst>(V)) {
      if (arrayIndex != nullptr || gep->getPointerAddressSpace() != 0) {
        // It's not expected to see multiple GEPs on this path or GEPs to addrspace other than 0.
        arrayIndex = nullptr;
        break;
      }
      auto pointerElementTy = gep->getSourceElementType();
      if (pointerElementTy->isStructTy()) {
        // Example: %1 = getelementptr inbounds %"struct.texture", %"struct.texture"* %aot, i64 %arrayIndex, i32 0
        arrayIndex = gep->getOperand(1);
      } else if (pointerElementTy->isArrayTy() && gep->getOperand(1) == builder.getInt64(0)) {
        // Example: %2 = getelementptr inbounds [8 x %"struct.texture"], [8 x %"struct.texture"]* %aot, i64 0, i64
        // %arrayIndex, i32 0
        arrayIndex = gep->getOperand(2);
      } else if (pointerElementTy->isPointerTy() && IGCLLVM::getNonOpaquePtrEltTy(pointerElementTy)->isStructTy() &&
                 gep->getOperand(1) == builder.getInt64(0)) {
        // Example: %3 = getelementptr inbounds %"struct.texture", %"struct.texture"** %aot, i64 0, i64 %arrayIndex, i32
        // 0
        arrayIndex = gep->getOperand(2);
      }
    }
  }
  return arrayIndex;
}

void PromoteResourceToDirectAS::PromoteSamplerTextureToDirectAS(GenIntrinsicInst *&pIntr, Value *resourcePtr) {
  IGCIRBuilder<> builder(pIntr);

  unsigned addrSpace = resourcePtr->getType()->getPointerAddressSpace();

  if (addrSpace != 1 && addrSpace != 2 && IGC::IsDirectIdx(addrSpace)) {
    // Already direct addrspace, no need to promote
    // Only try to promote bindless pointers ( as(1) or as(2) ), or indirect buffer access
    return;
  }
  unsigned bufID = 0;
  BufferType bufTy = BufferType::BUFFER_TYPE_UNKNOWN;
  BufferAccessType accTy;
  bool needBufferOffset; // Unused
  bool canPromote = false;
  Value *arrayIndex = nullptr;

  std::vector<Value *> instList;
  Value *srcPtr = IGC::TracePointerSource(resourcePtr, false, false, true, instList);

  if (srcPtr) {
    if (auto alloca = llvm::dyn_cast<AllocaInst>(srcPtr)) {
      arrayIndex = FindArrayIndex(instList, builder);
      if (arrayIndex != nullptr) {
        // TODO: We could read igc.read_only_array metadata attached to alloca.
        // If not -1, it should contain base index of this array. In this case,
        // FindArrayBaseArg would not be needed.

        // Find input argument for the first element in this array.
        srcPtr = FindArrayBaseArg(alloca);
      }
    }
  }

  if (srcPtr) {
    // Trace the resource pointer.
    // If we can find it, we can promote the indirect access to direct access
    // by encoding the BTI as a direct addrspace
    if (srcPtr->getType()->isPointerTy() &&
        IGC::GetResourcePointerInfo(srcPtr, bufID, bufTy, accTy, needBufferOffset)) {
      canPromote = true;
    } else if (Argument *argPtr = dyn_cast<Argument>(srcPtr)) {
      // Source comes from kernel arguments
      // We only promote if the argument comes from the entry function.
      // Default to bindless if sampler called from subroutine.
      Function *function = argPtr->getParent();
      if (isEntryFunc(m_pMdUtils, function)) {
        IGC_ASSERT(m_pCodeGenContext->type == ShaderType::OPENCL_SHADER);
        ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        if (modMD->FuncMD.find(function) != modMD->FuncMD.end()) {
          FunctionMetaData *funcMD = &modMD->FuncMD[function];
          ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
          ArgAllocMD *argInfo = &resAllocMD->argAllocMDList[argPtr->getArgNo()];
          IGC_ASSERT_MESSAGE((size_t)argPtr->getArgNo() < resAllocMD->argAllocMDList.size(),
                             "ArgAllocMD List Out of Bounds Error");

          if (argInfo->type == ResourceTypeEnum::BindlessUAVResourceType &&
              !modMD->UseBindlessImage) // don't promote if bindless image is preferred
          {
            bufID = (unsigned)argInfo->indexType;
            bufTy = BufferType::UAV;
            canPromote = true;
          } else if (argInfo->type == ResourceTypeEnum::BindlessSamplerResourceType) {
            bufID = (unsigned)argInfo->indexType;
            bufTy = BufferType::SAMPLER;
            canPromote = true;
          }
        }
      }
    } else if (GlobalVariable *pGlobal = dyn_cast<GlobalVariable>(srcPtr)) {
      // Can still promote if we traced to an inline sampler with samplerID metadata attached
      if (MDNode *md = pGlobal->getMetadata("ConstSampler")) {
        if (ConstantInt *C = mdconst::extract<ConstantInt>(md->getOperand(0))) {
          bufID = (unsigned)C->getZExtValue();
          bufTy = BufferType::SAMPLER;
          canPromote = true;
        }
      }
    }
  }

  if (canPromote) {
    Value *bufferId = builder.getInt32(bufID);
    if (arrayIndex != nullptr) {
      // Add base array index:
      if (arrayIndex->getType() != bufferId->getType()) {
        arrayIndex = builder.CreateZExtOrTrunc(arrayIndex, bufferId->getType());
      }
      bufferId = builder.CreateAdd(bufferId, arrayIndex);
    }

    addrSpace = IGC::EncodeAS4GFXResource(*bufferId, bufTy);
    PointerType *newptrType = IGCLLVM::get(dyn_cast<PointerType>(resourcePtr->getType()), addrSpace);

    Value *mutePtr = nullptr;
    if (llvm::isa<llvm::ConstantInt>(bufferId)) {
      mutePtr = ConstantPointerNull::get(newptrType);
    } else {
      // Index is not a constant:
      mutePtr = builder.CreateIntToPtr(builder.CreateZExt(bufferId, builder.getInt64Ty()), newptrType);
    }
    IGC::ChangePtrTypeInIntrinsic(pIntr, resourcePtr, mutePtr);
  }
}

bool PatchGetElementPtr(const std::vector<Value *> &instList, Type *dstTy, unsigned directAS, Value *patchedSourcePtr,
                        Value *&dstPtr) {
  unsigned numInstructions = instList.size();
  Value *patchedInst = patchedSourcePtr;
  dstPtr = nullptr;
  Type *patchTy = nullptr;

  // Find all the instructions we need to patch, starting from the top.
  // If there is more than one GEP instruction, we need to patch all of them, as well
  // as any pointer casts. All other instructions are not supported.
  // %0 = getelementptr int, int addrspace(1)* %ptr, i32 3
  // %1 = bitcast int addrspace(1)* %0 to float addrspace(1)*
  // %2 = getelementptr float, float addrspace(1)* %1, i32 8
  // PROMOTED TO:
  // %0 = getelementptr int, int addrspace(131072)* null, i32 3
  // %1 = bitcast int addrspace(131072)* %0 to float addrspace(131072)*
  // %2 = getelementptr float, float addrspace(131072)* %1, i32 8
  std::vector<Value *> patchInstructions;
  for (int i = numInstructions - 1; i >= 0; i--) {
    Value *inst = instList[i];
    if (isa<GetElementPtrInst>(inst)) {
      patchInstructions.push_back(inst);
    } else if (BitCastInst *cast = dyn_cast<BitCastInst>(inst)) {
      // Bitcast from pointer to pointer
      if (cast->getType()->isPointerTy() && cast->getOperand(0)->getType()->isPointerTy())
        patchInstructions.push_back(inst);
    }
  }

  if (!patchedInst) {
    if (patchInstructions.size() > 0) {
      // Get the original pointer type before any GEPs or bitcasts modifies it
      patchTy = IGCLLVM::getNonOpaquePtrEltTy(cast<Instruction>(patchInstructions[0])->getOperand(0)->getType());
    } else {
      // If there is nothing to patch, set the pointer type to the same type as the buffer access type
      patchTy = dstTy;
    }
    PointerType *newptrType = PointerType::get(patchTy, directAS);
    patchedInst = ConstantPointerNull::get(newptrType);
  }

  for (unsigned i = 0; i < (unsigned)patchInstructions.size(); i++) {
    Value *inst = patchInstructions[i];
    if (GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(inst)) {
      llvm::SmallVector<llvm::Value *, 4> gepArgs(gepInst->idx_begin(), gepInst->idx_end());
      // Create the new GEP instruction
      if (gepInst->isInBounds())
        patchedInst = GetElementPtrInst::CreateInBounds(patchTy, patchedInst, gepArgs, "", gepInst);
      else
        patchedInst = GetElementPtrInst::Create(patchTy, patchedInst, gepArgs, "", gepInst);

      if (GetElementPtrInst *gepPatchedInst = dyn_cast<GetElementPtrInst>(patchedInst)) {
        gepPatchedInst->setDebugLoc(gepInst->getDebugLoc());
      }
    } else if (BitCastInst *cast = dyn_cast<BitCastInst>(inst)) {
      PointerType *newptrType = IGCLLVM::get(dyn_cast<PointerType>(cast->getType()), directAS);
      patchedInst = BitCastInst::Create(Instruction::BitCast, patchedInst, newptrType, "", cast);
      if (BitCastInst *castPathedInst = dyn_cast<BitCastInst>(patchedInst)) {
        castPathedInst->setDebugLoc(cast->getDebugLoc());
      }
    } else {
      IGC_ASSERT_MESSAGE(0, "Can not patch unsupported instruction!");
      return false;
    }
  }

  dstPtr = patchedInst;

  // Final types must match
  return (IGCLLVM::getNonOpaquePtrEltTy(dstPtr->getType()) == dstTy);
}

bool PatchInstructionAddressSpace(const std::vector<Value *> &instList, Type *dstTy, unsigned directAS,
                                  Value *&dstPtr) {
  unsigned numInstructions = instList.size();
  dstPtr = nullptr;
  bool success = false;

  // Find the first PHI node or select we need to patch, if any.
  // In the most simple case, we assume only one branching instruction. If there are multiple selects, phis, or any
  // combination of the two, we won't be able to handle it.

  // First, we find the phi/select instruction. We patch all the GEP and ptrcast instructions for each branch, then
  // finally any GEP and ptrcast instructions that comes after the phi/select, but before the load
  PHINode *phiNode = nullptr;
  SelectInst *selectInst = nullptr;
  std::vector<Value *> remainingInst;
  for (unsigned i = 0; i < numInstructions; i++) {
    Value *inst = instList[i];
    if (PHINode *phi = dyn_cast<PHINode>(inst)) {
      phiNode = phi;
      break;
    } else if (SelectInst *select = dyn_cast<SelectInst>(inst)) {
      selectInst = select;
      break;
    } else {
      remainingInst.push_back(inst);
    }
  }

  if (selectInst) {
    Value *newSelectInst = nullptr;
    Value *bufferPtr0 = nullptr;
    Value *bufferPtr1 = nullptr;
    std::vector<Value *> tempList0, tempList1;
    // Call trace again to get the instructions list for each branch of the select
    if (IGC::TracePointerSource(selectInst->getOperand(1), true, false, true, tempList0) &&
        IGC::TracePointerSource(selectInst->getOperand(2), true, false, true, tempList1)) {
      IGC_ASSERT(selectInst->getOperand(1)->getType()->isPointerTy() &&
                 selectInst->getOperand(2)->getType()->isPointerTy());
      Type *srcType0 = IGCLLVM::getNonOpaquePtrEltTy(selectInst->getOperand(1)->getType());
      Type *srcType1 = IGCLLVM::getNonOpaquePtrEltTy(selectInst->getOperand(1)->getType());

      // Patch both branches, then patch the select instruction
      if (PatchGetElementPtr(tempList0, srcType0, directAS, nullptr, bufferPtr0) &&
          PatchGetElementPtr(tempList1, srcType1, directAS, nullptr, bufferPtr1)) {
        newSelectInst = SelectInst::Create(selectInst->getOperand(0), bufferPtr0, bufferPtr1, "", selectInst);
      }
      // If there are any remaining GEP/bitcast instructions after the select, patch them as well
      if (newSelectInst) {
        success = PatchGetElementPtr(remainingInst, dstTy, directAS, newSelectInst, dstPtr);
      }
    }
  } else if (phiNode) {
    PointerType *newPhiTy = IGCLLVM::get(dyn_cast<PointerType>(phiNode->getType()), directAS);
    PHINode *pNewPhi = PHINode::Create(newPhiTy, phiNode->getNumIncomingValues(), "", phiNode);
    for (unsigned int i = 0; i < phiNode->getNumIncomingValues(); ++i) {
      Value *incomingVal = phiNode->getIncomingValue(i);
      IGC_ASSERT(incomingVal->getType()->isPointerTy());

      std::vector<Value *> tempList;
      [[maybe_unused]] Value *srcPtr = IGC::TracePointerSource(incomingVal, true, false, true, tempList);

      // We know srcPtr is trace-able, since it's been traced already, we just need to get the
      // list of instructions we need to patch
      IGC_ASSERT(srcPtr);

      // Patch the GEPs for each phi node path
      Value *bufferPtr = nullptr;
      Type *incomingTy = IGCLLVM::getNonOpaquePtrEltTy(incomingVal->getType());
      if (!PatchGetElementPtr(tempList, incomingTy, directAS, nullptr, bufferPtr)) {
        // Patching must succeed for all paths
        pNewPhi->eraseFromParent();
        return false;
      }
      pNewPhi->addIncoming(bufferPtr, phiNode->getIncomingBlock(i));
    }

    // If there are any remaining GEP/bitcast instructions after the PHI node, patch them as well
    success = PatchGetElementPtr(remainingInst, dstTy, directAS, pNewPhi, dstPtr);
  } else {
    // If there are no PHI nodes or selects, we can just patch the GEPs
    success = PatchGetElementPtr(instList, dstTy, directAS, nullptr, dstPtr);
  }

  if (!dstPtr || !dstPtr->getType()->isPointerTy())
    return false;
  if (IGCLLVM::getNonOpaquePtrEltTy(dstPtr->getType()) != dstTy)
    return false;

  return success;
}

Value *PromoteResourceToDirectAS::getOffsetValue(Value *srcPtr, int &bufferOffsetHandle) {
  auto offsetEntry = m_SrcPtrToBufferOffsetMap.find(srcPtr);
  if (offsetEntry != m_SrcPtrToBufferOffsetMap.end()) {
    GenIntrinsicInst *runtimevalue = dyn_cast<GenIntrinsicInst>(offsetEntry->second);
    IGC_ASSERT_MESSAGE(runtimevalue, "Buffer offset must be a runtime value");
    bufferOffsetHandle = (int)llvm::cast<llvm::ConstantInt>(runtimevalue->getOperand(0))->getZExtValue();
    return offsetEntry->second;
  } else {
    Instruction *srcPtrInst;
    srcPtrInst = dyn_cast<Instruction>(srcPtr);
    IGC_ASSERT_MESSAGE(srcPtrInst, "source pointer must have been an instruction");
    IGCIRBuilder<> builder(srcPtrInst);

    Instruction *bufferOffset;
    ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    // Create runtime value for buffer offset
    Function *pFunc = GenISAIntrinsic::getDeclaration(srcPtrInst->getParent()->getParent()->getParent(),
                                                      GenISAIntrinsic::GenISA_RuntimeValue, builder.getInt32Ty());
    bufferOffset = builder.CreateCall(pFunc, builder.getInt32(modMD->MinNOSPushConstantSize));
    bufferOffsetHandle = modMD->MinNOSPushConstantSize;
    modMD->MinNOSPushConstantSize++;
    m_SrcPtrToBufferOffsetMap[srcPtr] = bufferOffset;
    return bufferOffset;
  }
}

void PromoteResourceToDirectAS::PromoteBufferToDirectAS(Instruction *inst, Value *resourcePtr) {
  IGCIRBuilder<> builder(inst);

  unsigned addrSpace = resourcePtr->getType()->getPointerAddressSpace();

  if (addrSpace != 1 && addrSpace != 2 && IGC::IsDirectIdx(addrSpace)) {
    // Already direct addrspace, no need to promote
    // Only try to promote stateless buffer pointers ( as(1) or as(2) ), or indirect buffer access
    return;
  }

  // Vulkan encodes address space differently, with the reserve bits set.
  // TODO: Investigate how addrspace is encoded in Vulkan,
  // for now skip promoting if it's an address space we dont recognize.
  if ((addrSpace & 0xFFC00000) != 0x0) {
    return;
  }

  std::vector<Value *> instructionList;
  Value *srcPtr = IGC::TracePointerSource(resourcePtr, false, false, true, instructionList);

  if (!srcPtr) {
    // Cannot trace the resource pointer back to it's source, cannot promote
    return;
  }

  unsigned bufferID;
  BufferType bufType;
  BufferAccessType accType;
  bool needBufferOffset;
  if (!IGC::GetResourcePointerInfo(srcPtr, bufferID, bufType, accType, needBufferOffset)) {
    // Can't promote if we don't know the explicit buffer ID and type
    return;
  }

  // Get the new direct address space
  unsigned directAS = IGC::EncodeAS4GFXResource(*builder.getInt32(bufferID), bufType);

  Value *pBuffer = nullptr;
  Type *pBufferType = GetBufferAccessType(inst);

  if (!PatchInstructionAddressSpace(instructionList, pBufferType, directAS, pBuffer)) {
    // Patching failed
    return;
  }

  // If needBufferOffset set, we need to adjust stateful buffer accesses with the buffer offset from payload
  Value *pointerValue;
  int bufferOffsetHandle = -1;
  if (needBufferOffset) {
    pointerValue = builder.CreatePtrToInt(pBuffer, builder.getInt32Ty());
    pointerValue = builder.CreateAdd(pointerValue, getOffsetValue(srcPtr, bufferOffsetHandle));
    pBuffer = builder.CreateIntToPtr(pointerValue, pBuffer->getType());
  }

  bool canpromote = false;
  if (LoadInst *load = dyn_cast<LoadInst>(inst)) {
    LoadInst *newload = IGC::cloneLoad(load, pBufferType, pBuffer);
    load->replaceAllUsesWith(newload);
    load->eraseFromParent();
    canpromote = true;
  } else if (StoreInst *store = dyn_cast<StoreInst>(inst)) {
    StoreInst *newstore = IGC::cloneStore(store, store->getOperand(0), pBuffer);
    store->replaceAllUsesWith(newstore);
    store->eraseFromParent();
    canpromote = true;
  } else if (GenIntrinsicInst *pIntr = dyn_cast<GenIntrinsicInst>(inst)) {
    Value *pNewBufferAccessInst = nullptr;

    switch (pIntr->getIntrinsicID()) {
      // TODO: ldraw and storeraw currently does not support non-aligned memory, if promote fails
      // then default alignment is 4. Need to implement support for ldraw and storeraw to support
      // non-aligned memory access, to preserve the alignment of the original load/store.

      // WA:
      // %522 = load <4 x i8> addrspace(131073)* %521
      // For this example instruction, InstructionCombining pass generates align4 if alignment
      // is not set. Forcing alignment to 1 generates the correct alignment value align2.
      // TODO: Why is no alignment and align1 treated differently by InstructionCombining?
    case GenISAIntrinsic::GenISA_ldraw_indexed:
    case GenISAIntrinsic::GenISA_ldrawvector_indexed: {
      Value *offsetVal = pIntr->getOperand(1);
      PointerType *ptrType = PointerType::get(pBufferType, directAS);
      pBuffer = builder.CreateIntToPtr(offsetVal, ptrType);

      const LdRawIntrinsic *const ldRawIntr = cast<LdRawIntrinsic>(pIntr);

      // Promote ldraw back to load
      pNewBufferAccessInst = builder.CreateAlignedLoad(
          pBufferType, pBuffer, IGCLLVM::getAlign(ldRawIntr->getAlignment()), ldRawIntr->isVolatile());
      break;
    }
    case GenISAIntrinsic::GenISA_storeraw_indexed:
    case GenISAIntrinsic::GenISA_storerawvector_indexed: {
      Value *offsetVal = pIntr->getOperand(1);
      PointerType *ptrType = PointerType::get(pBufferType, directAS);
      pBuffer = builder.CreateIntToPtr(offsetVal, ptrType);

      const StoreRawIntrinsic *const storeRawIntr = cast<StoreRawIntrinsic>(pIntr);

      // Promote storeraw back to store
      Value *storeVal = pIntr->getOperand(2);
      pNewBufferAccessInst = builder.CreateAlignedStore(
          storeVal, pBuffer, IGCLLVM::getAlign(storeRawIntr->getAlignment()), storeRawIntr->isVolatile());
      break;
    }
    default: {
      bool is64BitPtr = true;
      switch (pIntr->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_intatomicraw:
      case GenISAIntrinsic::GenISA_floatatomicraw:
      case GenISAIntrinsic::GenISA_intatomictyped:
      case GenISAIntrinsic::GenISA_floatatomictyped:
      case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
      case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
      case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
      case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
        is64BitPtr = false;
        break;
      case GenISAIntrinsic::GenISA_intatomicrawA64:
      case GenISAIntrinsic::GenISA_floatatomicrawA64:
      case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
      case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
      default:
        is64BitPtr = true;
        break;
      }

      // clone atomicraw instructions
      llvm::SmallVector<llvm::Value *, 8> args;
      llvm::SmallVector<Type *, 3> types;

      PointerType *newptrType = PointerType::get(pBufferType, directAS);
      Value *sourcePointer = ConstantPointerNull::get(newptrType);
      Value *bufferAddress = nullptr;

      types.push_back(pIntr->getType());
      types.push_back(sourcePointer->getType());

      if (is64BitPtr) {
        if (!isa<ConstantPointerNull>(pBuffer)) {
          bufferAddress = pBuffer;
        } else {
          bufferAddress = sourcePointer;
        }
        types.push_back(bufferAddress->getType());
      } else {
        bufferAddress = pIntr->getArgOperand(1);
        if (!isa<ConstantPointerNull>(pBuffer)) {
          IGC_ASSERT(isa<ConstantInt>(bufferAddress) && cast<ConstantInt>(bufferAddress)->getZExtValue() == 0);
          IGC_ASSERT(pIntr->getIntrinsicID() != GenISAIntrinsic::GenISA_intatomictyped &&
                     pIntr->getIntrinsicID() != GenISAIntrinsic::GenISA_icmpxchgatomictyped);
          bufferAddress = builder.CreatePtrToInt(pBuffer, builder.getInt32Ty());
        }
      }

      args.push_back(sourcePointer);
      args.push_back(bufferAddress);
      for (unsigned i = 2; i < IGCLLVM::getNumArgOperands(pIntr); i++) {
        args.push_back(pIntr->getArgOperand(i));
      }

      Module *module = pIntr->getParent()->getParent()->getParent();
      Function *pFunc = GenISAIntrinsic::getDeclaration(module, pIntr->getIntrinsicID(), types);
      pNewBufferAccessInst = builder.CreateCall(pFunc, args);
      break;
    }
    }

    if (pNewBufferAccessInst) {
      Instruction *oldInst = inst;
      Instruction *newInst = cast<Instruction>(pNewBufferAccessInst);

      // Clone metadata
      llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 4> MDs;
      oldInst->getAllMetadata(MDs);
      for (llvm::SmallVectorImpl<std::pair<unsigned, llvm::MDNode *>>::iterator MI = MDs.begin(), ME = MDs.end();
           MI != ME; ++MI) {
        newInst->setMetadata(MI->first, MI->second);
      }
      oldInst->replaceAllUsesWith(newInst);
      oldInst->eraseFromParent();
      canpromote = true;
    }
  }
  if (canpromote) {
    int handle = needBufferOffset ? bufferOffsetHandle : -1;
    if ((m_pCodeGenContext->m_buffersPromotedToDirectAS.find(bufferID) ==
         m_pCodeGenContext->m_buffersPromotedToDirectAS.end()) ||
        (m_pCodeGenContext->m_buffersPromotedToDirectAS[bufferID] == -1)) {
      m_pCodeGenContext->m_buffersPromotedToDirectAS[bufferID] = handle;
    } else {
      IGC_ASSERT((m_pCodeGenContext->m_buffersPromotedToDirectAS[bufferID] == handle));
    }
  }
}

void PromoteResourceToDirectAS::visitInstruction(Instruction &I) {
  bool resourceAccessed = false;
  if (llvm::GenIntrinsicInst *pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(&I)) {
    // Figure out the intrinsic operands for texture & sampler
    llvm::Value *pTextureValue = nullptr, *pSamplerValue = nullptr;
    IGC::getTextureAndSamplerOperands(pIntr, pTextureValue, pSamplerValue);

    if (pTextureValue && pTextureValue->getType()->isPointerTy()) {
      PromoteSamplerTextureToDirectAS(pIntr, pTextureValue);
      resourceAccessed = true;
    }
    if (pSamplerValue && pSamplerValue->getType()->isPointerTy()) {
      PromoteSamplerTextureToDirectAS(pIntr, pSamplerValue);
      resourceAccessed = true;
    }
  }

  // Handle buffer access call instructions
  if (!resourceAccessed) {
    Value *bufptr = GetBufferOperand(&I);

    if (bufptr && bufptr->getType()->isPointerTy()) {
      PromoteBufferToDirectAS(&I, bufptr);
      resourceAccessed = true;
    }
  }
}
