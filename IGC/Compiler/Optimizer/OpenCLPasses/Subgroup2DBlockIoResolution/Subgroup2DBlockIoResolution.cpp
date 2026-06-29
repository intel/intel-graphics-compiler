/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Subgroup2DBlockIoResolution.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Utils/CacheControlsHelper.h"

#include <algorithm>
#include <cstdint>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/Regex.h>
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace IGC;

char Subgroup2DBlockIoResolutionLPM::ID = 0;

#define PASS_FLAG "igc-subgroup-2dblockio-resolution"
#define PASS_DESC "Lowering of 2d block instructions (SPV, OCL, builtins) to intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "subgroup-2dblockio-resolution"

IGC_INITIALIZE_PASS_BEGIN(Subgroup2DBlockIoResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Subgroup2DBlockIoResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

Subgroup2DBlockIoResolutionLPM::Subgroup2DBlockIoResolutionLPM() : ModulePass(ID) {
  initializeSubgroup2DBlockIoResolutionLPMPass(*PassRegistry::getPassRegistry());
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses Subgroup2DBlockIoResolutionNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto *pCtx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  auto *pMdUtils = AM.getResult<MetaDataUtilsAnalysis>(M).MdUtils;
  bool changed = Subgroup2DBlockIoResolution().run(M, pCtx, pMdUtils);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16

bool Subgroup2DBlockIoResolution::run(Module &M, CodeGenContext *pCtx, IGCMD::MetaDataUtils *pMdUtils) {
  m_Ctx = pCtx;
  m_simdResolver = std::make_unique<KernelSIMDSizeResolver>(
      m_Ctx, pMdUtils, [](int32_t SIMDSize) { return SIMDSize == 16 || SIMDSize == 32; }, []() { return 16; },
      "2D block IO");
  m_Changed = false;
  m_BuiltinsToRemove.clear();

  visit(M);

  for (auto &F : m_BuiltinsToRemove)
    F->eraseFromParent();

  return m_Changed;
}

void Subgroup2DBlockIoResolution::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  static const Regex Pattern2DBlockSPV(
      "_Z[0-9]+__spirv_Subgroup2DBlock(LoadTransform|LoadTranspose|Load|Prefetch|Store)INTEL.+");
  static const Regex Pattern2DBlockOCL(
      "_Z[0-9]+intel_sub_group_2d_block_(read_transform|read_transpose|read|prefetch|write)_(.+)");
  // static const Regex Pattern2DBlockBuiltin(
  //     "__builtin_IB_subgoup_2d_block_cacheopts_(read|read_transform|read_transpose|prefetch|write).+");

  SmallVector<StringRef, 3> Matches;
  StringRef FuncName = F->getName();

  const bool IsSPIRV = Pattern2DBlockSPV.match(FuncName, &Matches);
  const bool IsOCL = !IsSPIRV && Pattern2DBlockOCL.match(FuncName, &Matches);
  if (!IsSPIRV && !IsOCL)
    return;

  m_GRFSize = m_Ctx->platform.getGRFSize();
  // Resolve and pin the kernel's SIMD size so the backend cannot later codegen the kernel
  // at a width different from the one this 2D block IO lowering assumes. Defaults to 16
  // when the kernel carries no reqd_sub_group_size.
  m_SimdSize = m_simdResolver->resolve(CI.getParent()->getParent());
  if (m_SimdSize != 16 && m_SimdSize != 32)
    return; // resolve() already emitted an error for an unsupported forced/attribute size

  if (IsSPIRV) {
    visit2DBlockCallInst(CI, Matches, true);
  } else {
    if (m_SimdSize != 16)
      emitError(CI, "cl_intel_subgroup_2d_block_io requires subgroup size of 16");
    else
      visit2DBlockCallInst(CI, Matches, false);
  }
}

void Subgroup2DBlockIoResolution::visit2DBlockCallInst(CallInst &CI, const SmallVector<StringRef, 3> &Matches,
                                                       bool IsSPIRV) {
  auto OpType = parseOperation(Matches[1]);
  if (OpType == Op::Invalid)
    return;

  auto BlockDesc = (IsSPIRV) ? extractBlockDescription(CI) : parseBlockDescription(Matches[2]);
  if (!validateBlockDescription(CI, OpType, BlockDesc))
    return;

  auto OverloadedType = getOverloadedType(CI, OpType, BlockDesc);

  mergeBlocks(OpType, BlockDesc);

  auto MemoryDesc = extractMemoryDescription(CI, OpType, IsSPIRV);

  auto CacheControls = (OpType == Op::Write)
                           ? resolveCacheControlDecorations<StoreCacheControl>(CI, MemoryDesc.GlobalMem, OpType)
                           : resolveCacheControlDecorations<LoadCacheControl>(CI, MemoryDesc.GlobalMem, OpType);

  auto Args = getArgs(CI, MemoryDesc, BlockDesc, OpType, CacheControls);
  auto IntrinsicID = getIntrinsic(OpType);

  Function *CalledFunc = CI.getCalledFunction();
  auto *Func = GenISAIntrinsic::getDeclaration(CalledFunc->getParent(), IntrinsicID,
                                               (OverloadedType) ? OverloadedType : ArrayRef<Type *>());

  IRBuilder<> Builder(&CI);
  auto *NewInst = Builder.CreateCall(Func, Args);

  if (IntrinsicID == GenISAIntrinsic::GenISA_LSC2DBlockRead) {
    auto *StoreDataPtr = Builder.CreatePointerCast(MemoryDesc.PrivateMem,
                                                   IGCLLVM::PointerType::get(NewInst->getType(), 0), "storeDataPtr");
    Builder.CreateStore(NewInst, StoreDataPtr);
  }

  CI.eraseFromParent();
  m_Changed = true;

  if (CalledFunc->getNumUses() == 0)
    m_BuiltinsToRemove.insert(CalledFunc);
}

Subgroup2DBlockIoResolution::Op Subgroup2DBlockIoResolution::parseOperation(StringRef OpStr) {

  if (OpStr == "Load" || OpStr == "read")
    return Op::Read;
  else if (OpStr == "LoadTransform" || OpStr == "read_transform")
    return Op::ReadTransform;
  else if (OpStr == "LoadTranspose" || OpStr == "read_transpose")
    return Op::ReadTranspose;
  else if (OpStr == "Prefetch" || OpStr == "prefetch")
    return Op::Prefetch;
  else if (OpStr == "Store" || OpStr == "write")
    return Op::Write;

  IGC_ASSERT_MESSAGE(0, "Unrecognized operation type in subgroup 2d block IO.");
  return Op::Invalid;
}

Subgroup2DBlockIoResolution::BlockDescription
Subgroup2DBlockIoResolution::parseBlockDescription(StringRef Description) {
  static const Regex DescriptionPattern("([0-9]+)b_([0-9]+)r([0-9]+)x([0-9]+)c.+");
  SmallVector<StringRef, 5> Matches;
  if (!DescriptionPattern.match(Description, &Matches)) {
    IGC_ASSERT_MESSAGE(0, "Invalid block description format");
    return {};
  }
  BlockDescription Desc;
  Matches[1].getAsInteger(10, Desc.ElemBitwidth);
  Matches[2].getAsInteger(10, Desc.Height);
  Matches[3].getAsInteger(10, Desc.Width);
  Matches[4].getAsInteger(10, Desc.NumBlocksV);
  return Desc;
};

Subgroup2DBlockIoResolution::BlockDescription Subgroup2DBlockIoResolution::extractBlockDescription(CallInst &CI) {
  Value *ElemSizeConstant = CI.getArgOperand(0);
  Value *BlockWidthConstant = CI.getArgOperand(1);
  Value *BlockHeightConstant = CI.getArgOperand(2);
  Value *NumBlocksVConstant = CI.getArgOperand(3);

  if (!isConstantInt(ElemSizeConstant, "Element Size") || !isConstantInt(BlockWidthConstant, "Block Width") ||
      !isConstantInt(BlockHeightConstant, "Block Height") || !isConstantInt(NumBlocksVConstant, "Block Count")) {
    return {};
  }

  auto AsInt32 = [](Value *V) -> uint32_t { return static_cast<uint32_t>(cast<ConstantInt>(V)->getZExtValue()); };

  BlockDescription Desc;
  Desc.ElemBitwidth = 8 * AsInt32(ElemSizeConstant);
  Desc.Width = AsInt32(BlockWidthConstant);
  Desc.Height = AsInt32(BlockHeightConstant);
  Desc.NumBlocksV = AsInt32(NumBlocksVConstant);

  return Desc;
}

bool Subgroup2DBlockIoResolution::isConstantInt(Value *Val, StringRef ValName) {
  if (isa<ConstantInt>(Val))
    return true;

  emitError(*Val, "Expected ", ValName.str(), " to be constant in __spirv_Subgroup2DBlock operation\n");
  return false;
}

Subgroup2DBlockIoResolution::MemoryDesc Subgroup2DBlockIoResolution::extractMemoryDescription(CallInst &CI, Op OpType,
                                                                                              bool IsSPIRV) {
  // Indices for: globalMem, width, height, pitch, coord, privateMem
  static constexpr std::array<int, 6> OCL_INDICES = {0, 1, 2, 3, 4, 5};
  static constexpr std::array<int, 6> SPV_READ_INDICES = {4, 5, 6, 7, 8, 9};
  static constexpr std::array<int, 6> SPV_WRITE_INDICES = {5, 6, 7, 8, 9, 4};

  const auto &Indices = !IsSPIRV ? OCL_INDICES : OpType == Op::Write ? SPV_WRITE_INDICES : SPV_READ_INDICES;

  MemoryDesc Desc;
  Desc.GlobalMem = CI.getArgOperand(Indices[0]);
  Desc.Width = CI.getArgOperand(Indices[1]);
  Desc.Height = CI.getArgOperand(Indices[2]);
  Desc.Pitch = CI.getArgOperand(Indices[3]);
  Desc.Coord = CI.getArgOperand(Indices[4]);

  if (OpType != Op::Prefetch)
    Desc.PrivateMem = CI.getArgOperand(Indices[5]);

  return Desc;
}

template <typename CCT>
CacheControlFromMDNodes Subgroup2DBlockIoResolution::resolveCacheControlDecorations(CallInst &CI, Value *PointerValue,
                                                                                    Op OpType) {
  static_assert(std::is_same_v<CCT, LoadCacheControl> || std::is_same_v<CCT, StoreCacheControl>);
  auto SpirvDecorations = parseSPIRVDecorationsFromMD(PointerValue);

  CacheControlFromMDNodes Controls{.value = 0, .isEmpty = true, .isInvalid = false};

  for (auto &[DecorationId, MDNodes] : SpirvDecorations) {
    if (DecorationId == getDecorationIdCacheControl<CCT>()) {
      Controls = resolveCacheControlFromMDNodes<CCT>(m_Ctx, MDNodes);
      if (Controls.isInvalid) {
        emitWarning(CI, "Unsupported cache controls configuration requested. Applying default configuration.");
        Controls.value = 0;
      }
      break;
    }
  }

  // Applying all cached configuration for prefetch with invalid or empty cache control
  if (OpType == Op::Prefetch && m_Ctx->platform.hasLSC() &&
      (Controls.isEmpty || Controls.isInvalid ||
       !m_Ctx->platform.isSupportedLSCCacheControlsEnum(static_cast<LSC_L1_L3_CC>(Controls.value), true))) {

    Controls.value = LSC_L1C_WT_L3C_WB;
    if (m_Ctx->platform.hasNewLSCCacheEncoding())
      Controls.value = LSC_LDCC_L1C_L2C_L3C;
  }

  return Controls;
}

static bool isPowerOf2AndInRange(uint32_t value, uint32_t min, uint32_t max) {
  return value >= min && value <= max && isPowerOf2_32(value);
}

bool Subgroup2DBlockIoResolution::validateBlockDescription(CallInst &CI, const Op OpType,
                                                           const BlockDescription &Desc) {
  if (Desc.ElemBitwidth == 0 || Desc.Width == 0 || Desc.Height == 0 || Desc.NumBlocksV == 0)
    return false;

  switch (OpType) {
  case Op::Read:
    return validateBlockDescriptionForRead(CI, Desc);
  case Op::ReadTransform:
    return validateBlockDescriptionForReadTransform(CI, Desc);
  case Op::ReadTranspose:
    return validateBlockDescriptionForReadTranspose(CI, Desc);
  case Op::Prefetch:
    return validateBlockDescriptionForPrefetch(CI, Desc);
  case Op::Write:
    return validateBlockDescriptionForWrite(CI, Desc);
  default:
    IGC_ASSERT_UNREACHABLE();
  }

  IGC_ASSERT_UNREACHABLE();
}

void Subgroup2DBlockIoResolution::emitAdditionalConstraintsError(CallInst &CI, const BlockDescription &Desc,
                                                                 bool WillReadMoreThanLoaded, bool Other) {
  auto [ElemBitwidth, Width, Height, NumBlocksV] = Desc;

  if (WillReadMoreThanLoaded)
    emitError(CI, "For element size of ", ElemBitwidth, " bits, subgroup requires more data than loaded.");

  if (Other)
    emitError(CI, "For element size of ", ElemBitwidth, " bits, block configuration (width=", Width,
              ", height=", Height, ", blocks=", NumBlocksV, ") is not supported due to hardware constraints");
}

bool Subgroup2DBlockIoResolution::validateHWConstraints(const BlockDescription &Desc,
                                                        ArrayRef<Constraints> ConstraintsArray) {
  auto [ElemBitwidth, Width, Height, NumBlocksV] = Desc;

  for (const auto &C : ConstraintsArray) {
    if (ElemBitwidth != C.ElemBitwidth)
      continue;
    if (isPowerOf2AndInRange(Width, C.MinWidth, C.MaxWidth) && isPowerOf2AndInRange(Height, C.MinHeight, C.MaxHeight) &&
        isPowerOf2AndInRange(NumBlocksV, C.MinBlocksV, C.MaxBlocksV) && Width * NumBlocksV <= C.MaxElements &&
        Width * NumBlocksV >= C.MinElements)
      return true;
  }
  return false;
}

bool Subgroup2DBlockIoResolution::validateBlockDescriptionForRead(CallInst &CI, const BlockDescription &Desc) {
  auto [ElemBitwidth, Width, Height, NumBlocksV] = Desc;

  static constexpr std::array<Constraints, 4> LoadConstraints = {
      Constraints{8, 1, 32, 4, 64, 1, 4, 4, 64},
      Constraints{16, 1, 32, 2, 32, 1, 4, 2, 32},
      Constraints{32, 1, 32, 1, 16, 1, 2, 1, 16},
      Constraints{64, 1, 32, 1, 8, 1, 1, 1, 8},
  };

  if (!validateHWConstraints(Desc, LoadConstraints)) {
    emitError(CI, "For element size of ", ElemBitwidth, " bits, unsupported block configuration: width=", Width,
              ", height=", Height, ", numBlocksV=", NumBlocksV, ".");
    return false;
  }

  // Additional Constraints to avoid very inefficient reads
  auto SimdRead = m_SimdSize * ElemBitwidth / 8;
  auto BlockSizeInBytes = Width * Height * ElemBitwidth / 8;
  auto BlockSizeRoundedToGRF = llvm::alignTo(BlockSizeInBytes, m_GRFSize);

  bool WillReadMoreThanLoaded = SimdRead > BlockSizeRoundedToGRF;

  bool IsOneBlock = NumBlocksV == 1;
  bool SimdReadFitsInGrf = SimdRead == m_GRFSize;
  bool WillFillGRF = BlockSizeInBytes >= m_GRFSize;
  bool CanBeMerged = BlockSizeInBytes < m_GRFSize && NumBlocksV > 1 && Height == 1 && Width >= m_SimdSize;

  bool Valid = !WillReadMoreThanLoaded && (SimdReadFitsInGrf || WillFillGRF || IsOneBlock || CanBeMerged);
  if (!Valid)
    emitAdditionalConstraintsError(CI, Desc, WillReadMoreThanLoaded,
                                   !(SimdReadFitsInGrf || WillFillGRF || IsOneBlock || CanBeMerged));

  return Valid;
}

bool Subgroup2DBlockIoResolution::validateBlockDescriptionForReadTransform(CallInst &CI, const BlockDescription &Desc) {
  auto [ElemBitwidth, Width, Height, NumBlocksV] = Desc;

  static constexpr std::array<Constraints, 2> NarrowTransformConstraints = {
      Constraints{8, 4, 32, 4, 16, 1, 4, 4, 64},
      Constraints{16, 2, 32, 2, 16, 1, 4, 2, 32},
  };

  static constexpr std::array<Constraints, 2> WideTransformConstraints = {
      Constraints{8, 4, 32, 4, 64, 1, 4, 4, 64},
      Constraints{16, 2, 32, 2, 32, 1, 4, 2, 32},
  };

  auto &Constr =
      m_Ctx->platform.supports2dBlockTransform64ElementsWidth() ? WideTransformConstraints : NarrowTransformConstraints;

  if (!validateHWConstraints(Desc, Constr)) {
    emitError(CI, "For element size of ", ElemBitwidth, " bits, unsupported block configuration: width=", Width,
              ", height=", Height, ", numBlocksV=", NumBlocksV, ".");
    return false;
  }

  // Additional Constraints to avoid very inefficient reads
  // Transform reads always returns 4 bytes per element, so simd read is calculated based on 4 bytes, not actual element
  // size
  auto SimdRead = m_SimdSize * 4;
  auto BlockSizeInBytes = Width * Height * ElemBitwidth / 8;
  auto BlockSizeRoundedToGRF = llvm::alignTo(BlockSizeInBytes, m_GRFSize);

  bool WillReadMoreThanLoaded = SimdRead > BlockSizeRoundedToGRF;
  bool WillFillBlock = BlockSizeInBytes >= SimdRead;
  bool SimdReadFitsInGrf = SimdRead == m_GRFSize;

  bool Valid = !WillReadMoreThanLoaded && (WillFillBlock || SimdReadFitsInGrf);
  if (!Valid)
    emitAdditionalConstraintsError(CI, Desc, WillReadMoreThanLoaded, !(WillFillBlock || SimdReadFitsInGrf));
  return Valid;
}

bool Subgroup2DBlockIoResolution::validateBlockDescriptionForReadTranspose(CallInst &CI, const BlockDescription &Desc) {
  auto [ElemBitwidth, Width, Height, NumBlocksV] = Desc;

  static constexpr std::array<Constraints, 2> NarrowTransposeConstraints = {
      Constraints{32, 1, 32, 1, 8, 1, 1, 1, 8},
      Constraints{64, 8, 8, 1, 4, 1, 1, 1, 4},
  };

  static constexpr std::array<Constraints, 2> WideTransposeConstraints = {
      Constraints{32, 1, 32, 1, 16, 1, 1, 1, 16},
      Constraints{64, 8, 16, 1, 8, 1, 1, 1, 8},
  };

  auto &Constr =
      m_Ctx->platform.supports2dBlockTranspose64ByteWidth() ? WideTransposeConstraints : NarrowTransposeConstraints;

  if (!validateHWConstraints(Desc, Constr)) {
    emitError(CI, "For element size of ", ElemBitwidth, " bits, unsupported block configuration: width=", Width,
              ", height=", Height, ", numBlocksV=", NumBlocksV, ".");
    return false;
  }

  auto SimdRead = m_SimdSize * ElemBitwidth / 8;
  auto BlockSizeInBytes = Width * Height * ElemBitwidth / 8;
  auto BlockSizeRoundedToGRF = llvm::alignTo(BlockSizeInBytes, m_GRFSize);

  bool WillReadMoreThanLoaded = SimdRead > BlockSizeRoundedToGRF;

  if (WillReadMoreThanLoaded)
    emitAdditionalConstraintsError(CI, Desc, true, false);

  return !WillReadMoreThanLoaded;
}

bool Subgroup2DBlockIoResolution::validateBlockDescriptionForPrefetch(CallInst &CI, const BlockDescription &Desc) {

  static constexpr std::array<Constraints, 4> PrefetchConstraints = {
      Constraints{8, 1, 32, 4, 64, 1, 4, 4, 64},
      Constraints{16, 1, 32, 2, 32, 1, 4, 2, 32},
      Constraints{32, 1, 32, 1, 16, 1, 2, 1, 16},
      Constraints{64, 1, 32, 1, 8, 1, 1, 1, 8},
  };

  static constexpr std::array<Constraints, 4> Prefetch256BytesConstraints = {
      Constraints{8, 1, 32, 64, 256, 1, 4, 256, 256},
      Constraints{16, 1, 32, 32, 128, 1, 4, 128, 128},
      Constraints{32, 1, 32, 16, 64, 1, 4, 64, 64},
      Constraints{64, 1, 32, 8, 32, 1, 4, 32, 32},
  };

  bool Valid = validateHWConstraints(Desc, PrefetchConstraints);
  if (m_Ctx->platform.supports2dBlockPrefetch256Bytes())
    Valid |= validateHWConstraints(Desc, Prefetch256BytesConstraints);

  if (!Valid) {
    auto [ElemBitwidth, Width, Height, NumBlocksV] = Desc;
    emitError(CI, "For element size of ", ElemBitwidth, " bits, unsupported block configuration: width=", Width,
              ", height=", Height, ", numBlocksV=", NumBlocksV, ".");
  }
  return Valid;
}

bool Subgroup2DBlockIoResolution::validateBlockDescriptionForWrite(CallInst &CI, const BlockDescription &Desc) {

  static constexpr std::array<Constraints, 4> WriteConstraints = {
      Constraints{8, 1, 8, 4, 64, 1, 1, 4, 64},
      Constraints{16, 1, 8, 2, 32, 1, 1, 2, 32},
      Constraints{32, 1, 8, 1, 16, 1, 1, 1, 16},
      Constraints{64, 1, 8, 1, 8, 1, 1, 1, 8},
  };

  if (!validateHWConstraints(Desc, WriteConstraints)) {
    auto [ElemBitwidth, Width, Height, NumBlocksV] = Desc;
    emitError(CI, "For element size of ", ElemBitwidth, " bits, unsupported block configuration: width=", Width,
              ", height=", Height, ", numBlocksV=", NumBlocksV, ".");
    return false;
  }
  return true;
}

SmallVector<Value *, 14> Subgroup2DBlockIoResolution::getArgs(CallInst &CI, const MemoryDesc &MemDesc,
                                                              const BlockDescription &BlockDesc, Op OpType,
                                                              CacheControlFromMDNodes CacheControls) {
  IRBuilder<> Builder(&CI);
  SmallVector<Value *, 14> Args;
  LLVMContext &Ctx = CI.getContext();

  auto *BaseAddrI64 = Builder.CreatePtrToInt(MemDesc.GlobalMem, Type::getInt64Ty(Ctx), "baseAddr");
  Args.push_back(BaseAddrI64);

  // Surface width, height, and pitch are -1 encoded in the intrinsic
  auto *One = ConstantInt::get(Type::getInt32Ty(Ctx), 1);
  auto *WidthM1 = Builder.CreateSub(MemDesc.Width, One, "width.m1");
  auto *HeightM1 = Builder.CreateSub(MemDesc.Height, One, "height.m1");
  auto *PitchM1 = Builder.CreateSub(MemDesc.Pitch, One, "pitch.m1");
  Args.push_back(WidthM1);
  Args.push_back(HeightM1);
  Args.push_back(PitchM1);

  auto *Zero = ConstantInt::get(Type::getInt32Ty(Ctx), 0);
  auto *XOffset = Builder.CreateExtractElement(MemDesc.Coord, Zero, "xOffset");
  auto *YOffset = Builder.CreateExtractElement(MemDesc.Coord, One, "yOffset");
  Args.push_back(XOffset);
  Args.push_back(YOffset);

  auto *ElemSizeC = ConstantInt::get((Type::getInt32Ty(Ctx)), BlockDesc.ElemBitwidth);
  auto *TileWidthC = ConstantInt::get((Type::getInt32Ty(Ctx)), BlockDesc.Width);
  auto *TileHeightC = ConstantInt::get((Type::getInt32Ty(Ctx)), BlockDesc.Height);
  auto *NumBlocksVC = ConstantInt::get((Type::getInt32Ty(Ctx)), BlockDesc.NumBlocksV);
  auto *IsTransposeC = ConstantInt::get((Type::getInt1Ty(Ctx)), OpType == Op::ReadTranspose);
  auto *IsVnniTransformC = ConstantInt::get((Type::getInt1Ty(Ctx)), OpType == Op::ReadTransform);
  auto *CacheControlsC = ConstantInt::get((Type::getInt32Ty(Ctx)), CacheControls.value);
  Args.push_back(ElemSizeC);
  Args.push_back(TileWidthC);
  Args.push_back(TileHeightC);
  Args.push_back(NumBlocksVC);
  Args.push_back(IsTransposeC);
  Args.push_back(IsVnniTransformC);
  Args.push_back(CacheControlsC);

  if (OpType == Op::Write) {
    auto *LoadedType = getOverloadedType(CI, OpType, BlockDesc);
    auto *StoreDataPtr =
        Builder.CreatePointerCast(MemDesc.PrivateMem, IGCLLVM::PointerType::get(LoadedType, 0), "storeDataPtr");
    auto *LoadedVec = Builder.CreateLoad(LoadedType, StoreDataPtr, "storeData");
    Args.push_back(LoadedVec);
  }

  return Args;
}

Type *Subgroup2DBlockIoResolution::getOverloadedType(CallInst &CI, Op OpType, const BlockDescription &BlockDesc) {

  switch (OpType) {
  case Op::Write:
  case Op::Read:
  case Op::ReadTranspose:
    return getOverloadedTypeCommon(CI, BlockDesc);
  case Op::ReadTransform:
    return getOverloadedTypeForTransform(CI, BlockDesc);
  case Op::Prefetch:
    return nullptr;
  default:
    IGC_ASSERT_UNREACHABLE();
  }
}

Type *Subgroup2DBlockIoResolution::getOverloadedTypeCommon(CallInst &CI, const BlockDescription &BlockDesc) {

  uint32_t MergedElements = std::max(BlockDesc.Width / m_SimdSize, 1u);
  uint32_t MergedRows = std::max(m_SimdSize / BlockDesc.Width, 1u);
  uint32_t Rows = std::min(BlockDesc.Height / MergedRows, BlockDesc.Height);
  uint32_t NumElementsPerWI = std::max(BlockDesc.NumBlocksV * Rows, BlockDesc.NumBlocksV);

  LLVMContext &Ctx = CI.getContext();
  Type *ElemType = IntegerType::get(Ctx, BlockDesc.ElemBitwidth * MergedElements);
  return llvm::VectorType::get(ElemType, NumElementsPerWI, false);
}

Type *Subgroup2DBlockIoResolution::getOverloadedTypeForTransform(CallInst &CI, const BlockDescription &BlockDesc) {
  uint32_t NumElementsInVNNI = 32 / BlockDesc.ElemBitwidth;
  uint32_t PackedRows = BlockDesc.Height / NumElementsInVNNI;
  uint32_t NumElementsPerBlock = BlockDesc.Width * PackedRows;
  uint32_t NumElementsPerWI = std::max(NumElementsPerBlock / m_SimdSize, 1u) * BlockDesc.NumBlocksV;

  LLVMContext &Ctx = CI.getContext();
  Type *ElemType = IntegerType::get(Ctx, 32);
  return llvm::VectorType::get(ElemType, NumElementsPerWI, false);
}

void Subgroup2DBlockIoResolution::mergeBlocks(Op OpType, BlockDescription &BlockDesc) {
  uint32_t BlockSizeInBytes = BlockDesc.Width * BlockDesc.Height * BlockDesc.ElemBitwidth / 8;

  if (OpType == Op::Read && BlockSizeInBytes < m_GRFSize && BlockDesc.NumBlocksV > 1 && BlockDesc.Height == 1 &&
      BlockDesc.Width >= m_SimdSize) {
    uint32_t BlocksPerGRF = m_GRFSize / BlockSizeInBytes;
    uint32_t MergedNumBlocksV = std::min(BlockDesc.NumBlocksV, BlocksPerGRF);

    BlockDesc.NumBlocksV /= MergedNumBlocksV;
    BlockDesc.Width *= MergedNumBlocksV;
  } else if (OpType == Op::Prefetch) {
    BlockDesc.Width *= BlockDesc.NumBlocksV;
    BlockDesc.NumBlocksV = 1;
  }
}

GenISAIntrinsic::ID Subgroup2DBlockIoResolution::getIntrinsic(Op OpType) {
  switch (OpType) {
  case Op::Read:
  case Op::ReadTranspose:
  case Op::ReadTransform:
    return GenISAIntrinsic::GenISA_LSC2DBlockRead;
  case Op::Write:
    return GenISAIntrinsic::GenISA_LSC2DBlockWrite;
  case Op::Prefetch:
    return GenISAIntrinsic::GenISA_LSC2DBlockPrefetch;
  default:
    IGC_ASSERT_UNREACHABLE();
  }
}
