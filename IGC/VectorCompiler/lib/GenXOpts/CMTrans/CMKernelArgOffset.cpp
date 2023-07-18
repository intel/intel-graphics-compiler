/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
/// CMKernelArgOffset
/// -----------------
///
/// This pass determines the offset of each CM kernel argument, and adds it to
/// the kernel metadata.
///
/// This pass also changes the linkage type for kernels, functions, and globals.
/// assuming that functions and globals has no external exposure, therefore
/// if not use, can be deleted by later GlobalDCE pass.
///
/// A CM kernel has metadata containing, amongst other things, an array of
/// *kind* bytes, one byte per kernel argument, that will be output in the vISA
/// kernel input table. This pass calculates the offset of each kernel argument,
/// and adds an array to the kernel metadata containing the calculated offsets.
///
/// Argument offsets start at 32, as r0 is reserved by the various thread
/// dispatch mechanisms.
///
/// The pass attempts to calculate the kernel argument offsets in a way that
/// minimizes space wasted by holes.
///
/// The arguments are processed in three sets, with each (non-empty) set
/// starting in a new GRF:
///
/// 1. explicit kernel arguments (i.e. ones that appeared in the CM source);
///
/// 2. implicit kernel (non-thread) arguments;
///
/// 3. implicit thread arguments.
///
/// These three sets need to be allocated as three separate chunks of whole GRF
/// registers in that order by the CM runtime. In theory, the CM runtime can
/// cope with the compiler creating a different ordering, but to do so it needs
/// to create its own ordering and insert mov instructions at the start of the
/// kernel, which is suboptimal. However, I am not clear whether that mechanism
/// works, and it has not been tested.
///
/// There is a compiler option that can be used to disable argument re-ordering.
/// This is for developers who are using the output asm files directly and want
/// to control the argument order explicitly. The option is
/// -enable-kernel-arg-reordering but is typically invoked as -mllvm
/// -enable-kernel-arg-reordering=false (the default is true)
///
/// Along with kernel argument offset calculation, it sets kernel argument
/// indexes and implicit linearization offsets in the original explicit byval
/// argument (OffsetsInArg). Argument index may differ from argument number in
/// function. For instance, all the implicit linearization arguments have the
/// index equal to the explicit argument index, because they must be mapped to
/// it in OCL/L0 runtime argument annotation.
///
///   %struct.s1 = type { [2 x i32], i8 }
///   declare i32 @foo(%struct.s1* byval(%struct.s1) "VCArgumentDesc"="svmptr_t"
///                     "VCArgumentIOKind"="0" "VCArgumentKind"="0" %_arg_, i64
///                     %_arg_1, i32 %__arg_lin__arg_0, i32 %__arg_lin__arg_1,
///                     i8 %__arg_lin__arg_2);
///
///   Argument             | Index | OffsetsInArg |
///   %_arg_               |     0 |           0  | explicit byval arg
///   %_arg_1              |     1 |           0  | explicit arg
///   %__arg_lin__arg_0.0  |     0 |           0  | linearization of %_arg_
///   %__arg_lin__arg_0.4  |     0 |           4  | linearization of %_arg_
///   %__arg_lin__arg_0.8  |     0 |           8  | linearization of %_arg_
///
/// This example shows that implicit linearization arguments
/// (%__arg_lin__arg_0.0, %__arg_lin__arg_0.4 and %__arg_lin__arg_0.8) of the
/// explicit byval %_arg_ must be mapped at argument with index = 0 (= %_arg_)
/// and their offsets in this argument are 0, 4, 8 bytes. %_arg_ has %struct.s1
/// type, consequently, %__arg_lin__arg_0.0 is the first element of the array in
/// %struct.s1 type, %__arg_lin__arg_0.4 is the second element of the array, and
/// %__arg_lin__arg_0.8 is the last i8  field. Additionally, at this point, all
/// the uses of explicit byval arguments are changed to the appropriate
/// linearization.
///
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "cmkernelargoffset"

#include <llvmWrapper/IR/Type.h>
#include "llvmWrapper/Support/Alignment.h"

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Utils/GenX/KernelInfo.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include "Probe/Assertion.h"

using namespace llvm;

namespace llvm {
unsigned getValueAlignmentInBytes(const Value &Val, const DataLayout &DL) {
  // If this is a volatile global, then its pointer
  // actually means nothing and pointee type should be
  // used instead.
  auto *GV = dyn_cast<GlobalVariable>(&Val);
  if (GV && GV->hasAttribute(genx::FunctionMD::GenXVolatile)) {
    return divideCeil(DL.getTypeSizeInBits(GV->getValueType()), 8);
  }
  Type *Ty = Val.getType();
  if (Ty->isPointerTy())
    return IGCLLVM::getAlignmentValue(
        DL.getPointerABIAlignment(Ty->getPointerAddressSpace()));

  return divideCeil(DL.getTypeSizeInBits(Ty->getScalarType()), 8);
}
} // namespace llvm

namespace {

struct GrfParamZone {
  unsigned Start;
  unsigned End;
  GrfParamZone(unsigned s, unsigned e) : Start(s), End(e){};
};

// CMKernelArgOffset pass
class CMKernelArgOffset : public ModulePass {
  vc::KernelMetadata *KM = nullptr;

public:
  static char ID;
  CMKernelArgOffset(unsigned GrfByteSize = 32)
      : ModulePass(ID), GrfByteSize(GrfByteSize) {
    initializeCMKernelArgOffsetPass(*PassRegistry::getPassRegistry());
    GrfMaxCount = 256;
    GrfStartOffset = GrfByteSize;
    GrfEndOffset = 128 * GrfByteSize;
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {}
  StringRef getPassName() const override { return "CM kernel arg offset"; }
  bool runOnModule(Module &M) override;

private:
  void processKernel(Function &Kernel);
  void processKernelOnOCLRT(Function *F);
  void resolveByValArgs(Function *F) const;

  static Value *getValue(Metadata *M) {
    if (auto VM = dyn_cast<ValueAsMetadata>(M))
      return VM->getValue();
    return nullptr;
  }

  unsigned GrfByteSize;
  unsigned GrfMaxCount;
  unsigned GrfStartOffset;
  unsigned GrfEndOffset;
};

} // namespace

char CMKernelArgOffset::ID = 0;

INITIALIZE_PASS_BEGIN(CMKernelArgOffset, "cmkernelargoffset",
                      "CM kernel arg offset determination", false, false)
INITIALIZE_PASS_END(CMKernelArgOffset, "cmkernelargoffset",
                    "CM kernel arg offset determination", false, false)

Pass *llvm::createCMKernelArgOffsetPass(unsigned GrfByteSize) {
  return new CMKernelArgOffset(GrfByteSize);
}

// Check whether there is an input/output argument attribute.
static bool canReorderArguments(const vc::KernelMetadata &KM) {
  using ArgIOKind = vc::KernelMetadata::ArgIOKind;
  return llvm::all_of(KM.getArgIOKinds(),
                      [](ArgIOKind K) { return K == ArgIOKind::Normal; });
}

/***********************************************************************
 * runOnModule : run the CM kernel arg offset pass
 */
bool CMKernelArgOffset::runOnModule(Module &M) {
  if (!vc::hasKernel(M))
    return false;

  // Process each kernel in the CM kernel metadata.
  for (Function &Kernel : vc::kernels(M))
    processKernel(Kernel);

  return true;
}

/***********************************************************************
 * processKernel : process one kernel
 *
 * Enter:   Kernel = reference for a kernel function
 *
 * See GenXMetadata.h for complete list of kernel metadata
 */
void CMKernelArgOffset::processKernel(Function &Kernel) {
  // change the linkage attribute for the kernel
  Kernel.setDLLStorageClass(llvm::GlobalValue::DLLExportStorageClass);

  vc::KernelMetadata KM{&Kernel};
  this->KM = &KM;

  resolveByValArgs(&Kernel);
  return processKernelOnOCLRT(&Kernel);
}

// CMImpParam generated byval aggregate arguments linearization metadata and
// appended implicit linearization to function arguments. Now it's time to
// change the use of the explicit byval aggregate argument to its implicit
// linearization.
void CMKernelArgOffset::resolveByValArgs(Function *F) const {
  IGC_ASSERT(KM);

  IRBuilder<> Builder(&*F->getEntryBlock().getFirstInsertionPt());
  for (auto &Arg : F->args()) {
    if (!KM->hasArgLinearization(&Arg))
      continue;

    auto *Base =
        Builder.CreateAlloca(IGCLLVM::getNonOpaquePtrEltTy(Arg.getType()), nullptr,
                             Arg.getName() + ".linearization");

    Value *BaseAsI8Ptr = Builder.CreateBitCast(Base, Builder.getInt8PtrTy(),
                                               Base->getName() + ".i8");
    for (const auto &Info : KM->arg_lin(&Arg)) {
      Type *Ty = IGCLLVM::getNonOpaquePtrEltTy(BaseAsI8Ptr->getType()->getScalarType());
      Value *StoreAddrUntyped = Builder.CreateGEP(Ty, BaseAsI8Ptr, Info.Offset);
      Value *StoreAddrTyped = Builder.CreateBitCast(
          StoreAddrUntyped, Info.Arg->getType()->getPointerTo());
      Builder.CreateStore(Info.Arg, StoreAddrTyped);
    }

    Arg.replaceNonMetadataUsesWith(Base);
  }
}

// Add entries to a container(map). A key is an implicit linearization argument
// and value is an offset  for this implicit linearization argument.
// Arg = explicit argument which has the implicit linearization
// ArgOffset = offset of Arg
template <typename OutIterT>
void setImplicitLinearizationOffset(Argument &Arg, unsigned ArgOffset,
                                    const vc::KernelMetadata &KM,
                                    OutIterT OutIt) {
  IGC_ASSERT(KM.hasArgLinearization(&Arg));
  std::transform(KM.arg_lin_begin(&Arg), KM.arg_lin_end(&Arg), OutIt,
                 [ArgOffset](const vc::ImplicitLinearizationInfo &Lin) {
                   return std::make_pair(Lin.Arg, Lin.Offset->getZExtValue() +
                                                      ArgOffset);
                 });
}

void CMKernelArgOffset::processKernelOnOCLRT(Function *F) {
  IGC_ASSERT(KM);

  SmallDenseMap<const Argument *, unsigned> PlacedArgs;
  {
    // OpenCL SIMD8 thread payloads are organized as follows:
    //
    //     0        1        2        3        4        5        6        7
    // R0:          GX                                           GY       GZ
    // R1: LIDx LIDy LIDz
    //
    unsigned Offset = GrfStartOffset;

    unsigned ThreadPayloads[] = {
      Offset, // R1: local_id_x, local_id_y, local_id_z
    };
    auto getImpOffset = [&](uint32_t ArgKind) -> int {
      if (vc::isLocalIDKind(ArgKind))
        return ThreadPayloads[0];
      return -1;
    };

    // Starting offsets for non-implicit arguments.
    Offset += 1 * GrfByteSize;

    // A map from implicit linearization argument to it's offset. The offset for
    // this type of arguments is an offset of the explicit argument (which was
    // linearized) + offset in the explicit argument.
    std::unordered_map<Argument *, unsigned> ImplicitLinearizationArgToOffset;

    // Place an argument and update offset.
    // Arguments larger than a GRF must be at least GRF-aligned. Arguments
    // smaller than a GRF may not cross GRF boundaries. This means that
    // arguments cross a GRF boundary must be GRF aligned.
    auto placeArg = [&](Argument *Arg, unsigned ByteSize, unsigned Align) {
      Offset = alignTo(Offset, Align);
      unsigned StartGRF = Offset / GrfByteSize;
      unsigned EndGRF = (Offset + ByteSize - 1) / GrfByteSize;
      if (StartGRF != EndGRF)
        Offset = alignTo(Offset, GrfByteSize);
      if (Arg->hasByValAttr()) {
        PlacedArgs[Arg] = vc::KernelMetadata::SKIP_OFFSET_VAL;
        auto InsertIt = std::inserter(ImplicitLinearizationArgToOffset,
                                      ImplicitLinearizationArgToOffset.end());
        setImplicitLinearizationOffset(*Arg, Offset, *KM, InsertIt);
        Offset += ByteSize;
      } else if (ImplicitLinearizationArgToOffset.count(Arg)) {
        // Don't update offset. This implicit arg must be mapped on an explicit
        // one.
        PlacedArgs[Arg] = ImplicitLinearizationArgToOffset[Arg];
      } else {
        PlacedArgs[Arg] = Offset;
        Offset += ByteSize;
      }
    };

    // First scan, assign implicit arguments.
    for (auto &&[Arg, ArgKind] : zip(F->args(), KM->getArgKinds())) {
      int ImpOffset = getImpOffset(ArgKind);
      if (ImpOffset > 0) {
        PlacedArgs[&Arg] = ImpOffset;
        continue;
      }

      if (vc::isLocalSizeKind(ArgKind) || vc::isGroupCountKind(ArgKind) ||
          vc::isPrintBufferKind(ArgKind) || vc::isPrivateBaseKind(ArgKind) ||
          vc::isImplicitArgsBufferKind(ArgKind)) {
        unsigned Bytes = Arg.getType()->getPrimitiveSizeInBits() / 8;
        unsigned Align = Arg.getType()->getScalarSizeInBits() / 8;
        placeArg(&Arg, Bytes, Align);
      }
    }

    // Second scan, assign normal arguments.
    unsigned Idx = 0;
    for (auto &&[Arg, ArgKind] : zip(F->args(), KM->getArgKinds())) {
      bool IsBuffer = KM->isBufferType(Idx++);

      // Skip alaready assigned arguments.
      if (PlacedArgs.count(&Arg))
        continue;

      // image/sampler arguments do not allocate vISA inputs
      // buffer arguments do allocate unused vISA inputs
      if (!vc::isNormalCategoryArgKind(ArgKind) && !IsBuffer) {
        PlacedArgs[&Arg] = vc::KernelMetadata::SKIP_OFFSET_VAL;
        continue;
      }

      Type *Ty = Arg.getType();
      auto &DL = F->getParent()->getDataLayout();
      unsigned Alignment = 0;
      unsigned Bytes = 0;
      if (IsBuffer) {
        // Buffer is treated as stateless global pointer!
        Bytes = DL.getPointerSize();
        Alignment = IGCLLVM::getAlignmentValue(DL.getPointerABIAlignment(0));
      } else if (Ty->isPointerTy()) {
        if (Arg.hasByValAttr()) {
          Ty = Ty->getContainedType(0);
          Bytes = DL.getTypeAllocSize(Ty);
          Alignment = IGCLLVM::getAlignmentValue(Bytes);
        } else {
          Bytes = DL.getPointerTypeSize(Ty);
          Alignment = IGCLLVM::getAlignmentValue(
              DL.getPointerABIAlignment(Ty->getPointerAddressSpace()));
        }
      } else if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty)) {
        auto *ETy = VTy->getElementType();
        Bytes = DL.getTypeSizeInBits(Ty) / 8;
        Alignment = IGCLLVM::getAlignmentValue(DL.getABITypeAlignment(ETy));
      } else {
        Bytes = DL.getTypeSizeInBits(Ty) / 8;
        Alignment = IGCLLVM::getAlignmentValue(DL.getABITypeAlignment(Ty));
      }
      placeArg(&Arg, Bytes, Alignment);
    }
  }

  SmallVector<unsigned, 8> ArgOffsets;
  std::transform(
      F->arg_begin(), F->arg_end(), std::back_inserter(ArgOffsets),
      [&PlacedArgs](const Argument &Arg) { return PlacedArgs[&Arg]; });
  KM->updateArgOffsetsMD(std::move(ArgOffsets));

  SmallVector<unsigned, 8> OffsetInArgs(F->arg_size(), 0);
  SmallVector<unsigned, 8> Indexes;
  std::transform(F->arg_begin(), F->arg_end(), std::back_inserter(Indexes),
                 [](const Argument &Arg) { return Arg.getArgNo(); });
  for (Argument &Arg : F->args()) {
    if (!KM->hasArgLinearization(&Arg))
      continue;
    for (const auto &Lin : KM->arg_lin(&Arg)) {
      unsigned LinArgNo = Lin.Arg->getArgNo();
      OffsetInArgs[LinArgNo] = Lin.Offset->getZExtValue();
      Indexes[LinArgNo] = Arg.getArgNo();
    }
  }

  KM->updateOffsetInArgsMD(std::move(OffsetInArgs));
  KM->updateArgIndexesMD(std::move(Indexes));
}
