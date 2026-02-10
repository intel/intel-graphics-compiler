/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/WorkaroundAnalysisPass.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/Intrinsics.h>
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

static cl::opt<bool> EnableFMaxFMinPlusZero("enable-fmax-fmin-plus-zero", cl::init(false), cl::Hidden,
                                            cl::desc("Enable fmax/fmin + 0.0f flag"));

// Register pass to igc-opt
#define PASS_FLAG "igc-workaround"
#define PASS_DESCRIPTION "Workaround pass used to fix functionality of special cases"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WorkaroundAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(WorkaroundAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_ANALYSIS
#undef PASS_CFG_ONLY
#undef PASS_DESCRIPTION
#undef PASS_FLAG

char WorkaroundAnalysis::ID = 0;

// Returns BTI of the texture when resource is not dynamically indexed and when
// resource is not bindless.
int GetSampleCResourceIdx(llvm::CallInst &I) {
  int textLocation = -1;
  if (SampleIntrinsic *pSamplerLoadInst = dyn_cast<SampleIntrinsic>(&I)) {
    textLocation = pSamplerLoadInst->getTextureIndex();
    llvm::Value *pArgLocation = pSamplerLoadInst->getOperand(textLocation);
    if (pArgLocation->getType()->isPointerTy()) {
      uint as = pArgLocation->getType()->getPointerAddressSpace();
      uint bufferIndex;
      bool directIdx;
      const BufferType resourceType = DecodeAS4GFXResource(as, directIdx, bufferIndex);
      if (resourceType == RESOURCE && directIdx) {
        return bufferIndex;
      }
    }
  }
  return textLocation;
}

WorkaroundAnalysis::WorkaroundAnalysis() : FunctionPass(ID) {
  initializeWorkaroundAnalysisPass(*PassRegistry::getPassRegistry());
}

bool WorkaroundAnalysis::runOnFunction(Function &F) {
  m_pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
  m_pDataLayout = &F.getParent()->getDataLayout();
  m_DeferredInstructions.clear();

  LLVM3DBuilder<> builder(F.getContext(), m_pCtxWrapper->getCodeGenContext()->platform.getPlatformInfo());
  m_builder = &builder;
  m_pModule = F.getParent();
  visit(F);
  for (auto I : m_DeferredInstructions) {
    m_builder->SetInsertPoint(I);
    processDeferredInstruction(I);
  }
  return true;
}

const unsigned MaxTrackingDepth = 8;

/// IsKnownNotSNaN() - Check whether a value won't be an SNaN. By definition,
/// all Gen instructions on floating pointer values (except FMAX/FMIN, MOV,
/// LOAD) will return QNaN is one of the operand is NaN (either QNaN or SNaN).
static bool IsKnownNotSNaN(Value *V, unsigned Depth = 0) {
  // Is unknown if the maximal depth reaches.
  if (Depth > MaxTrackingDepth)
    return false;

  // SNaN is only possible for floating point values.
  if (!V->getType()->isFloatingPointTy())
    return true;

  // With FP constant, check whether it's SNaN directly.
  if (ConstantFP *CFP = dyn_cast<ConstantFP>(V)) {
    APFloat FVal = CFP->getValueAPF();

    if (!FVal.isNaN())
      return true;

    APInt IVal = FVal.bitcastToAPInt();
    switch (IVal.getBitWidth()) {
    default:
      break;
    case 16:
      return !IVal[9];
    case 32:
      return !IVal[22];
    case 64:
      return !IVal[51];
    }
    return false;
  }

  Instruction *I = dyn_cast<Instruction>(V);
  if (!I)
    return false;

  // Phi-node is known not SNaN if all its incoming values are not SNaN.
  if (PHINode *PN = dyn_cast<PHINode>(V)) {
    bool NotSNaN = true;
    for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
      Value *InVal = PN->getIncomingValue(i);
      if (V == InVal)
        continue;
      NotSNaN = NotSNaN && IsKnownNotSNaN(InVal, Depth + 1);
    }
    return NotSNaN;
  }

  switch (I->getOpcode()) {
  case Instruction::BitCast:
    // SNaN is possible returned after BitCast.
    return false;
  case Instruction::FSub:
    // TODO: Revisit later after unsafe math is considered during source
    // modifier folding in pattern match and/or signed zeros are telled in
    // safe math mode.
    // Skip 0 - x, which may be lowered as '-' source modifier, which won't
    // quietize the NaN.
    if (ConstantFP *CFP = dyn_cast<ConstantFP>(I->getOperand(0))) {
      if (CFP->isZero())
        return false;
    }
    return true;
  case Instruction::Select:
    return IsKnownNotSNaN(I->getOperand(1), Depth + 1) && IsKnownNotSNaN(I->getOperand(2), Depth + 1);
  case Instruction::FAdd:
  case Instruction::FMul:
  case Instruction::FDiv:
    // TODO: List all instructions finally lowered into Gen insts quietize
    // the NaN.
    return true;
  case Instruction::Call:
    if (GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(I)) {
      switch (GII->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_rsq:
        return true;
      default:
        break;
      }
    } else if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(I)) {
      switch (II->getIntrinsicID()) {
      case Intrinsic::sqrt:
      case Intrinsic::powi:
      case Intrinsic::sin:
      case Intrinsic::cos:
      case Intrinsic::pow:
      case Intrinsic::log:
      case Intrinsic::log10:
      case Intrinsic::log2:
      case Intrinsic::exp:
      case Intrinsic::exp2:
      case Intrinsic::floor:
      case Intrinsic::ceil:
      case Intrinsic::trunc:
      case Intrinsic::rint:
      case Intrinsic::nearbyint:
      case Intrinsic::fma:
      case Intrinsic::fmuladd:
      case Intrinsic::maxnum:
      case Intrinsic::minnum:
        // TODO: Do we need to add various conversions to FP?
        // NOTE: fabs since it may be folded as a source modifier.
        // TODO: Revisit fabs later after unsafe math mode is
        // considered during source modifier folding in pattern match.
        return true;
      default:
        break;
      }
    }
    break;
  default:
    break;
  }

  return false;
}

static Constant *getQNaN(Type *Ty) {
  APFloat QNaN = APFloat::getQNaN(Ty->getFltSemantics());
  return ConstantFP::get(Ty->getContext(), QNaN);
}

void WorkaroundAnalysis::visitCallInst(llvm::CallInst &I) {
  CodeGenContext *pCodeGenCtx = m_pCtxWrapper->getCodeGenContext();

  if (pCodeGenCtx->getModuleMetaData()->enableRangeReduce || IGC_IS_FLAG_ENABLED(EnableTrigFuncRangeReduction)) {
    if (IntrinsicInst *intrinsicInst = dyn_cast<IntrinsicInst>(&I)) {
      if (intrinsicInst->getIntrinsicID() == Intrinsic::sin || intrinsicInst->getIntrinsicID() == Intrinsic::cos) {
        m_builder->SetInsertPoint(intrinsicInst);
        Value *angle = intrinsicInst->getOperand(0);
        Value *int1_res_s1 = m_builder->CreateFCmpOGE(angle, llvm::ConstantFP::get(m_builder->getFloatTy(), 0.0));
        Value *float_selRes_s =
            m_builder->CreateSelect(int1_res_s1, llvm::ConstantFP::get(m_builder->getFloatTy(), 6.283185f),
                                    llvm::ConstantFP::get(m_builder->getFloatTy(), -6.283185f));
        Value *float_selRes_s4 =
            m_builder->CreateSelect(int1_res_s1, llvm::ConstantFP::get(m_builder->getFloatTy(), 0.159155f),
                                    llvm::ConstantFP::get(m_builder->getFloatTy(), -0.159155f));
        Value *float_res_s5 = m_builder->CreateFMul(float_selRes_s4, angle);
        Value *funcFloor = m_builder->CreateFPToSI(float_res_s5, m_builder->getInt32Ty());
        Value *funcfloorfloat = m_builder->CreateSIToFP(funcFloor, m_builder->getFloatTy());
        Value *float_res_s_i_s = m_builder->CreateFSub(float_res_s5, funcfloorfloat);
        Value *float_res_s6 = m_builder->CreateFMul(float_res_s_i_s, float_selRes_s);
        intrinsicInst->setOperand(0, float_res_s6);
      }
    }
  }


  if (const GenIntrinsicInst *intr = dyn_cast<GenIntrinsicInst>(&I)) {
    switch (intr->getIntrinsicID()) {
    case llvm::GenISAIntrinsic::GenISA_gather4POCptr:
    case llvm::GenISAIntrinsic::GenISA_gather4POptr:
    case GenISAIntrinsic::GenISA_gather4IPOptr:
    case GenISAIntrinsic::GenISA_gather4BPOptr:
    case GenISAIntrinsic::GenISA_gather4LPOptr:
    case GenISAIntrinsic::GenISA_gather4ICPOptr:
    case GenISAIntrinsic::GenISA_gather4LCPOptr:
      GatherOffsetWorkaround(cast<SamplerGatherIntrinsic>(&I));
      break;
    case GenISAIntrinsic::GenISA_ldmsptr:
      ldmsOffsetWorkaournd(cast<LdMSIntrinsic>(&I));
      break;
    case llvm::GenISAIntrinsic::GenISA_RenderTargetReadSampleFreq:
    case llvm::GenISAIntrinsic::GenISA_RenderTargetReadSampleFreqPtr: {
      // Render target read should return 0 when the sample is outside primitive processed.
      //     R0.xyzw = RTRead(RTi, SampleIndex);
      //     R1 = 1<<SamplexIndex
      //     R2 = R1 & InputCoverage
      //     F0 = Cmp ne R2, 0
      //     Dst.w = (F0) Sel R0.w, 1
      CodeGenContext *pCodeGenCtx = m_pCtxWrapper->getCodeGenContext();
      if (pCodeGenCtx->platform.WaReturnZeroforRTReadOutsidePrimitive()) {
        RenderTargetReadIntrinsic *inst = dyn_cast<RenderTargetReadIntrinsic>(&I);

        Value *one = llvm::ConstantFP::get(m_builder->getFloatTy(), 1.0);
        llvm::Instruction *cloneinst = inst->clone();
        cloneinst->insertAfter(inst);
        m_builder->SetInsertPoint(&(*std::next(BasicBlock::iterator(cloneinst))));
        Value *inputCoverage = m_builder->CreateCoverage();

        Value *shiftLeft = m_builder->CreateShl(m_builder->getInt32(1), inst->getSampleIndex());
        Value *andWithInputCoverage = m_builder->CreateAnd(shiftLeft, inputCoverage);
        Value *cmpInst = m_builder->CreateICmpNE(andWithInputCoverage, m_builder->getInt32(0));

        Value *valueX = m_builder->CreateExtractElement(cloneinst, m_builder->getInt32(0));
        Value *valueY = m_builder->CreateExtractElement(cloneinst, m_builder->getInt32(1));
        Value *valueZ = m_builder->CreateExtractElement(cloneinst, m_builder->getInt32(2));
        Value *valueW = m_builder->CreateExtractElement(cloneinst, m_builder->getInt32(3));

        Value *selW = m_builder->CreateSelect(cmpInst, valueW, one);

        llvm::Value *newValue = llvm::UndefValue::get(IGCLLVM::FixedVectorType::get(m_builder->getFloatTy(), 4));
        newValue = m_builder->CreateInsertElement(newValue, valueX, m_builder->getInt32(0));
        newValue = m_builder->CreateInsertElement(newValue, valueY, m_builder->getInt32(1));
        newValue = m_builder->CreateInsertElement(newValue, valueZ, m_builder->getInt32(2));
        newValue = m_builder->CreateInsertElement(newValue, selW, m_builder->getInt32(3));

        inst->replaceAllUsesWith(newValue);
        inst->eraseFromParent();
      }
    } break;
    case llvm::GenISAIntrinsic::GenISA_readsurfacetypeandformat: {
      CodeGenContext *pCodeGenCtx = m_pCtxWrapper->getCodeGenContext();
      if (pCodeGenCtx && pCodeGenCtx->platform.supportsReadStateInfo()) {
        convertReadSurfaceTypeAndFormatToA64(I, pCodeGenCtx);
      }
    } break;
    default:
      break;
    }
  }
}

void WorkaroundAnalysis::convertReadSurfaceTypeAndFormatToA64(llvm::CallInst &I, CodeGenContext *pCodeGenCtx) {
  // Intrinsic signature: <2 x i32> @llvm.genx.GenISA.readsurfacetypeandformat(ptr addrspace(N))
  // Returns: [SurfaceType, SurfaceFormat] (element 0 = type, element 1 = format)
  // In 64B format, both are extracted from DWORD 0; in 32B format, both are extracted from DWORD 3.

  GenIntrinsicInst *GenISACall = cast<GenIntrinsicInst>(&I);
  m_builder->SetInsertPoint(&I);

  // Get the resource pointer operand (address space is BINDLESS or similar)
  Value *resourcePtr = GenISACall->getOperand(0);

  // Convert pointer to integer, then back to pointer in ADDRESS_SPACE_CONSTANT
  Value *ptrAsInt = m_builder->CreatePtrToInt(resourcePtr, m_builder->getInt64Ty());
  llvm::PointerType *ptrType = llvm::PointerType::get(m_builder->getInt32Ty(), ADDRESS_SPACE_CONSTANT);
  Value *surfaceStatePtr = m_builder->CreateIntToPtr(ptrAsInt, ptrType);

  Value *dwordPtr;
  unsigned typeShiftAmount, formatShiftAmount;

  {
    // 64B format: DWORD 0 bits [31:29] = Type, bits [26:18] = Format
    dwordPtr = surfaceStatePtr;
    typeShiftAmount = 29;
    formatShiftAmount = 18;
  }

  // Load the DWORD with 4-byte alignment
  LoadInst *dword = m_builder->CreateLoad(m_builder->getInt32Ty(), dwordPtr, "readsurfacetypeandformatA64");
  dword->setAlignment(IGCLLVM::getCorrectAlign(4));

  // Set cache control: LSC_LDCC_L1C_L2C_L3C (25) = L1 cached, L2 cached, L3 cached
  LLVMContext &Ctx = dword->getContext();
  Metadata *cacheCtrlMD[] = {ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(Ctx), LSC_LDCC_L1C_L2C_L3C))};
  MDNode *cacheCtrlNode = MDNode::get(Ctx, cacheCtrlMD);
  dword->setMetadata("lsc.cache.ctrl", cacheCtrlNode);

  // Extract Surface Type (3 bits) and Surface Format (9 bits)
  Value *typeShifted = m_builder->CreateLShr(dword, m_builder->getInt32(typeShiftAmount));
  Value *surfaceType = m_builder->CreateAnd(typeShifted, m_builder->getInt32(0x7)); // 3 bits

  Value *formatShifted = m_builder->CreateLShr(dword, m_builder->getInt32(formatShiftAmount));
  Value *surfaceFormat = m_builder->CreateAnd(formatShifted, m_builder->getInt32(0x1FF)); // 9 bits

  // Build the return vector <2 x i32> to match the original intrinsic return type
  // Element 0 = SurfaceType, Element 1 = SurfaceFormat
  Value *result = PoisonValue::get(I.getType());
  result = m_builder->CreateInsertElement(result, surfaceType, m_builder->getInt32(0));
  result = m_builder->CreateInsertElement(result, surfaceFormat, m_builder->getInt32(1));

  // Replace all uses and delete the old intrinsic
  I.replaceAllUsesWith(result);
  I.eraseFromParent();
}
void WorkaroundAnalysis::ldmsOffsetWorkaournd(LdMSIntrinsic *ldms) {
  // In some cases immediate offsets are not working in hardware for ldms message
  // to solve it we add directly the offset to the integer coordinate
  Value *zero = m_builder->getInt32(0);
  if (ldms->getImmOffset(0) == zero && ldms->getImmOffset(1) == zero && ldms->getImmOffset(2) == zero) {
    return;
  }
  for (unsigned int i = 0; i < 2; i++) {
    m_builder->SetInsertPoint(ldms);
    Value *coord = ldms->getCoordinate(i);
    Value *newCoord = m_builder->CreateAdd(coord, m_builder->CreateTrunc(ldms->getImmOffset(i), coord->getType()));
    ldms->setCoordinate(i, newCoord);
    ldms->setImmOffset(i, m_builder->getInt32(0));
  }
}

/// transform gather4poc and gatherpo into gather4c/gather4
void WorkaroundAnalysis::GatherOffsetWorkaround(SamplerGatherIntrinsic *gatherpo) {
  if (IGC_IS_FLAG_DISABLED(EnableGather4cpoWA)) {
    return;
  }
  Value *const zero = m_builder->getInt32(0);
  Value *const zeroFP = llvm::ConstantFP::get(gatherpo->getOperand(0)->getType(), 0);
  const bool hasRef = gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4POCptr ||
                      gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4ICPOptr ||
                      gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4LCPOptr;
  const bool hasImplicitLod =
      gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4IPOptr ||
      gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4ICPOptr; // does not include gather4 with bias here
  const bool hasBias = gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4BPOptr; // it has implicit lod
  const bool hasLod = gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4LPOptr ||
                      gatherpo->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4LCPOptr;
  // Implicit lod adjusted with optional bias needs to be fetched for resinfo, and gather4_l[_c] will be used
  // with this lod value
  const bool fetchLod = hasImplicitLod || hasBias;
  // Extra args preceding coords and other remained args
  const uint extraBeginArgsNo = (hasRef ? 1 : 0) + (hasLod || fetchLod ? 1 : 0);

  if (m_pCtxWrapper->getCodeGenContext()->platform.supportsGather4PO() && !hasRef) {
    return;
  }
  unsigned int offsetArgumentIndices[] = {8u + extraBeginArgsNo, 9u + extraBeginArgsNo, 10u + extraBeginArgsNo};
  if (gatherpo->getOperand(offsetArgumentIndices[0]) != zero ||
      gatherpo->getOperand(offsetArgumentIndices[1]) != zero ||
      gatherpo->getOperand(offsetArgumentIndices[2]) != zero) {
    // only apply the WA if all the immediate offsets are zero
    return;
  }
  Value *pairedResource = gatherpo->getPairedTextureValue();
  Value *resource = gatherpo->getTextureValue();
  Value *sampler = gatherpo->getSamplerValue();

  // Obtain fp32 LOD for resinfo and gather4_l
  Value *lod = nullptr;
  if (hasLod) {
    IGC_ASSERT_MESSAGE(!(hasImplicitLod || hasBias),
                       "An instruction with explicit lod cannot be marked as implicit lod at the same time.");
    // Gather4 with offsets and explicit (float) lod needs new messages to be added in the future.
    // This implementation is correct only for simple cases, like existing tests for AMD_texture_gather_bias_lod.
    lod = gatherpo->getOperand(hasRef ? 1 : 0);
  } else {
    if (fetchLod) {
      llvm::Type *funcTypes[] = {IGCLLVM::FixedVectorType::get(m_builder->getFloatTy(), 4),
                                 gatherpo->getOperand(extraBeginArgsNo)->getType(), // r coord's type
                                 resource->getType(), sampler->getType()};

      Function *lodInfo = GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_lodptr, funcTypes);
      m_builder->SetInsertPoint(gatherpo);

      Value *args[] = {gatherpo->getOperand(extraBeginArgsNo),     // u
                       gatherpo->getOperand(extraBeginArgsNo + 1), // v
                       gatherpo->getOperand(extraBeginArgsNo + 4), // r
                       zeroFP,                                     // ai
                       resource,
                       sampler};
      Value *info = m_builder->CreateCall(lodInfo, args);

      constexpr uint32_t lodArg = 1; // lod message returns fp32 lod in DW1
      lod = m_builder->CreateExtractElement(info, m_builder->getInt32(lodArg));
      if (hasBias) {
        lod = m_builder->CreateFAdd(lod, gatherpo->getOperand(0 /* bias as fp32 */));
      }
    } else {
      lod = zeroFP;
    }
  }
  IGC_ASSERT(lod);

  Function *resInfo =
      GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_resinfoptr, resource->getType());
  m_builder->SetInsertPoint(gatherpo);
  Value *info = m_builder->CreateCall2(resInfo, resource, m_builder->CreateFPToUI(lod, zero->getType()));

  std::vector<Value *> arg;
  if (extraBeginArgsNo > 0) {
    arg.push_back(gatherpo->getOperand(0));
    IGC_ASSERT(extraBeginArgsNo <= 2);
    if (extraBeginArgsNo == 2) {
      arg.push_back(gatherpo->getOperand(1));
    }
  }

  if (fetchLod) {
    IGC_ASSERT(extraBeginArgsNo == 1 + (hasRef ? 1 : 0));
    arg[hasRef ? 1 : 0] = lod;
  }

  arg.push_back(nullptr);                                                  // u
  arg.push_back(nullptr);                                                  // v
  arg.push_back(gatherpo->getOperand(4 + extraBeginArgsNo));               // r
  arg.push_back(ConstantFP::get(gatherpo->getOperand(0)->getType(), 0.0)); // ai
  arg.push_back(pairedResource);
  arg.push_back(resource);
  arg.push_back(sampler);
  arg.push_back(zero);
  arg.push_back(zero);
  arg.push_back(zero);
  arg.push_back(gatherpo->getOperand(offsetArgumentIndices[2] + 1));

  for (unsigned int i = 0; i < 2; i++) {
    Value *coord = gatherpo->getOperand(i + extraBeginArgsNo);
    Value *offset = gatherpo->getOperand(i + 2 + extraBeginArgsNo);

    Value *size = m_builder->CreateExtractElement(info, m_builder->getInt32(i));
    size = m_builder->CreateUIToFP(size, coord->getType());
    Value *invSize = m_builder->CreateFDiv(ConstantFP::get(coord->getType(), 1.0), size);

    // offset is only encoded on 6 bits
    offset = m_builder->CreateShl(offset, m_builder->getInt32(32 - 6));
    offset = m_builder->CreateAShr(offset, m_builder->getInt32(32 - 6));
    offset = m_builder->CreateSIToFP(offset, coord->getType());
    //
    Value *newCoord = m_builder->CreateFMul(offset, invSize);
    newCoord = m_builder->CreateFAdd(newCoord, coord);
    arg[i + extraBeginArgsNo] = newCoord;
  }
  Type *types[] = {
      gatherpo->getType(), gatherpo->getOperand(0)->getType(), pairedResource->getType(), resource->getType(),
      sampler->getType(),
  };
  Function *gather4Func = GenISAIntrinsic::getDeclaration(
      m_pModule,
      fetchLod || hasLod ? (hasRef ? GenISAIntrinsic::GenISA_gather4LCptr : GenISAIntrinsic::GenISA_gather4Lptr)
      : hasRef           ? GenISAIntrinsic::GenISA_gather4Cptr
                         : GenISAIntrinsic::GenISA_gather4ptr,
      types);
  Value *gather4c = m_builder->CreateCall(gather4Func, arg);
  gatherpo->replaceAllUsesWith(gather4c);
  gatherpo->eraseFromParent();
}

void WorkaroundAnalysis::processDeferredInstruction(llvm::Instruction *I) {
}


#define PASS_FLAG "igc-wa-fminmax"
#define PASS_DESCRIPTION "Workaround fmax/fmin"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WAFMinFMax, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(WAFMinFMax, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WAFMinFMax::ID = 0;

WAFMinFMax::WAFMinFMax() : FunctionPass(ID) { initializeWAFMinFMaxPass(*PassRegistry::getPassRegistry()); }

bool WAFMinFMax::runOnFunction(Function &F) {
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  llvm::IGCIRBuilder<> builder(F.getContext());
  m_builder = &builder;
  visit(F);
  return true;
}

void WAFMinFMax::visitCallInst(CallInst &I) {
  if (const IntrinsicInst *intr = dyn_cast<IntrinsicInst>(&I)) {
    switch (intr->getIntrinsicID()) {
    case Intrinsic::maxnum:
    case Intrinsic::minnum: {
      if (m_ctx->m_DriverInfo.SupportsIEEEMinMax()) {
        // OCL's fmax/fmin maps to GEN's max/min in non-IEEE mode.
        // By default, gen uses non-IEEE mode.  But, in BDW and SKL
        // prior to C step, IEEE mode is used if denorm bit is set.
        // If there are no sNaN as inputs to fmax/fmin,  IEEE mode
        // is the same as non-IEEE mode;  if there are sNaN,  non-IEEE
        // mode is NOT the same as IEEE mode. But non-IEEE mode is the
        // same as the following
        //     non-IEEE_fmax(x, y) =
        //           x1 = IEEE_fmin(x, qNaN), y1 = IEEE_fmin(y, qNaN)
        //             (or fadd(x, -0.0); y1 = fadd(y, -0.0);)
        //           IEEE_fmax(x1, y1)
        // SKL C+ has IEEE minmax bit in Control Register(CR), so far we
        // don't set it (meaning non-ieee mode).
        //
        // Therefore, if fmax/fmin is in IEEE mode, we need to workaround
        // that by converting sNAN to qNAN if one of operands is sNAN but
        // needing to preserve the original value if it's not sNAN.
        //
        // There are more than one ways to achieve that:
        //  - X + 0.0
        //    It's the simplest one. However, it cannot preserver -0.0
        //    as -0.0 + 0.0 = 0.0. It also has other issues depending
        //    on rounding mode. We could enhance it by adding -0.0 if X
        //    is negative. But that
        //    introduces additional overhead.
        //
        //  The following two are both good candidates with single
        //  instruction overhead only:
        //
        //  - x * 1.0
        //  - FMIN(x, qNAN) or FMAX(x, qNAN)
        //
        //    According to PRM, both of them should aways give x or
        //    qNAN.
        //
        // We choose FMIN to prevent other optimizations kicking in.
        //

        // Note that m_enableFMaxFMinPlusZero is used here for GEN9 only; if it
        // is set,  it means that IEEE-mode min/max is used if denorm bit is set.
        Type *Ty = intr->getType();
        ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        bool minmaxModeSetByDenormBit = (!m_ctx->platform.hasIEEEMinmaxBit() ||
                                         m_ctx->platform.WaOCLEnableFMaxFMinPlusZero() || EnableFMaxFMinPlusZero);
        bool hasNaNs = !modMD->compOpt.FiniteMathOnly;
        if (hasNaNs && minmaxModeSetByDenormBit &&
            ((Ty->isFloatTy() && (modMD->compOpt.FloatDenormMode32 == FLOAT_DENORM_RETAIN)) ||
             (Ty->isDoubleTy() && (modMD->compOpt.FloatDenormMode64 == FLOAT_DENORM_RETAIN)) ||
             (Ty->isHalfTy() && (modMD->compOpt.FloatDenormMode16 == FLOAT_DENORM_RETAIN)))) {
          m_builder->SetInsertPoint(&I);

          IGCLLVM::Intrinsic IID = Intrinsic::minnum;
          Function *IFunc = Intrinsic::getDeclaration(I.getParent()->getParent()->getParent(), IID, I.getType());
          Value *QNaN = getQNaN(I.getType());

          Value *src0 = I.getOperand(0);
          if (!IsKnownNotSNaN(src0))
            I.setArgOperand(0, m_builder->CreateCall2(IFunc, src0, QNaN));

          Value *src1 = I.getOperand(1);
          if (!IsKnownNotSNaN(src1))
            I.setArgOperand(1, m_builder->CreateCall2(IFunc, src1, QNaN));
        }
      }
      break;
    }

    default:
      break;
    }
  }
}
