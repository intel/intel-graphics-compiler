/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "cmkernelargoffset"

#include "llvmWrapper/Support/Alignment.h"

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

static cl::opt<bool>
    EnableKernelArgReordering("enable-kernel-arg-reordering", cl::init(true),
                              cl::Hidden,
                              cl::desc("Enable kernel argument reordering"));

namespace llvm {
void initializeCMKernelArgOffsetPass(PassRegistry &);
}

namespace {

struct GrfParamZone {
  unsigned Start;
  unsigned End;
  GrfParamZone(unsigned s, unsigned e) : Start(s), End(e){};
};

// Diagnostic information for error/warning from this pass.
class DiagnosticInfoCMKernelArgOffset : public DiagnosticInfoOptimizationBase {
private:
  static int KindID;
  static int getKindID() {
    if (KindID == 0)
      KindID = llvm::getNextAvailablePluginDiagnosticKind();
    return KindID;
  }

public:
  static void emit(Instruction *Inst, StringRef Msg,
                   DiagnosticSeverity Severity = DS_Error);
  DiagnosticInfoCMKernelArgOffset(DiagnosticSeverity Severity,
                                  const Function &Fn, const DebugLoc &DLoc,
                                  StringRef Msg)
      : DiagnosticInfoOptimizationBase((DiagnosticKind)getKindID(), Severity,
                                       /*PassName=*/nullptr, Msg, Fn, DLoc) {}
  // This kind of message is always enabled, and not affected by -rpass.
  virtual bool isEnabled() const override { return true; }
  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};
int DiagnosticInfoCMKernelArgOffset::KindID = 0;

// CMKernelArgOffset pass
class CMKernelArgOffset : public ModulePass {
  genx::KernelMetadata *KM = nullptr;

  // Emit code for OCL runtime.
  bool OCLCodeGen = false;

public:
  static char ID;
  CMKernelArgOffset(unsigned GrfByteSize = 32, bool OCLCodeGen = false)
      : ModulePass(ID), OCLCodeGen(OCLCodeGen), GrfByteSize(GrfByteSize) {
    initializeCMKernelArgOffsetPass(*PassRegistry::getPassRegistry());
    GrfMaxCount = 256;
    GrfStartOffset = GrfByteSize;
    GrfEndOffset = 128 * GrfByteSize;
  }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {}
  virtual StringRef getPassName() const { return "CM kernel arg offset"; }
  virtual bool runOnModule(Module &M);

private:
  void processKernel(MDNode *Node);
  void processKernelOnOCLRT(MDNode *Node, Function *F);

  static Value *getValue(Metadata *M) {
    if (auto VM = dyn_cast<ValueAsMetadata>(M))
      return VM->getValue();
    return nullptr;
  }

  // Check whether there is an input/output argument attribute.
  void checkArgKinds(Function *F) {
    assert(KM && KM->isKernel());
    for (unsigned i = 0, e = KM->getNumArgs(); i != e; ++i) {
      auto IOKind = KM->getArgInputOutputKind(i);
      // If there is input/output attribute, compiler will not freely reorder
      // arguments.
      if (IOKind != genx::KernelMetadata::IO_Normal) {
        EnableKernelArgReordering = false;
        break;
      }
    }
  }

  // Relayout thread paylod for OpenCL runtime.
  bool enableOCLCodeGen() const { return OCLCodeGen; }

  // Update offset MD node
  void updateOffsetMD(MDNode *KernelMD,
                      SmallDenseMap<Argument *, unsigned> &PlacedArgs) {
    assert(KM);
    Function *F = dyn_cast_or_null<Function>(
        getValue(KernelMD->getOperand(genx::KernelMDOp::FunctionRef)));
    assert(F && "nullptr kernel");

    // All arguments now have offsets. Update the metadata node containing the
    // offsets.
    assert(F->arg_size() == KM->getNumArgs() &&
           "Mismatch between metadata for kernel and number of args");
    SmallVector<Metadata *, 8> ArgOffsets;
    auto I32Ty = Type::getInt32Ty(F->getContext());
    for (auto ai = F->arg_begin(), ae = F->arg_end(); ai != ae; ++ai) {
      Argument *Arg = &*ai;
      ArgOffsets.push_back(ValueAsMetadata::getConstant(
          ConstantInt::get(I32Ty, PlacedArgs[Arg])));
    }
    MDNode *OffsetsNode = MDNode::get(F->getContext(), ArgOffsets);
    KernelMD->replaceOperandWith(genx::KernelMDOp::ArgOffsets, OffsetsNode);

    // Give an error on too many arguments.
    if (ArgOffsets.size() >= GrfMaxCount)
      DiagnosticInfoCMKernelArgOffset::emit(&F->front().front(),
                                            "Too many kernel arguments");
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

Pass *llvm::createCMKernelArgOffsetPass(unsigned GrfByteSize, bool OCLCodeGen) {
  return new CMKernelArgOffset(GrfByteSize, OCLCodeGen);
}

/***********************************************************************
 * runOnModule : run the CM kernel arg offset pass
 */
bool CMKernelArgOffset::runOnModule(Module &M) {
  NamedMDNode *Named = M.getNamedMetadata(genx::FunctionMD::GenXKernels);
  if (!Named)
    return 0;

  // Process each kernel in the CM kernel metadata.
  for (unsigned i = 0, e = Named->getNumOperands(); i != e; ++i) {
    MDNode *KernelNode = Named->getOperand(i);
    if (KernelNode)
      processKernel(KernelNode);
  }

  return true;
}

/***********************************************************************
 * processKernel : process one kernel
 *
 * Enter:   Node = metadata node for one kernel
 *
 * See GenXMetadata.h for complete list of kernel metadata
 */
void CMKernelArgOffset::processKernel(MDNode *Node) {
  Function *F = dyn_cast_or_null<Function>(
      getValue(Node->getOperand(genx::KernelMDOp::FunctionRef)));
  if (!F)
    return;

  // change the linkage attribute for the kernel
  F->setDLLStorageClass(llvm::GlobalValue::DLLExportStorageClass);

  genx::KernelMetadata KM(F);
  this->KM = &KM;
  checkArgKinds(F);

  // Layout kernel arguments differently if to run on OpenCL runtime.
  if (enableOCLCodeGen()) {
    return processKernelOnOCLRT(Node, F);
  }

  auto getTypeSizeInBytes = [=](Type *Ty) {
    const DataLayout &DL = F->getParent()->getDataLayout();
    if (auto PT = dyn_cast<PointerType>(Ty))
      return DL.getPointerTypeSize(Ty);
    return static_cast<unsigned>(Ty->getPrimitiveSizeInBits() / 8);
  };

  // setup kernel inputs, optionally reordering the assigned offsets for
  // improved packing where appropriate. The reordering algorithm replicates
  // that used in the legacy Cm compiler, as certain media walker applications
  // seem sensitive to the way the kernel inputs are laid out.
  SmallDenseMap<Argument *, unsigned> PlacedArgs;
  unsigned Offset = 0;
  if (EnableKernelArgReordering /*DoReordering*/) {
    // Reorder kernel input arguments. Arguments are placed in size order,
    // largest first (then in natural argument order where arguments are the
    // same size). Each argument is placed at the lowest unused suitably
    // aligned offset. So, in general big arguments are placed first with the
    // smaller arguments being fit opportunistically into the gaps left
    // between arguments placed earlier.
    //
    // Arguments that are at least one GRF in size must be aligned to a GRF
    // boundary. Arguments smaller than a GRF must not cross a GRF boundary.
    //
    // FreeZones describes unallocated portions of the kernel input space,
    // and is list of non-overlapping start-end pairs, ordered lowest first.
    // Initially it consists of a single pair that describes the whole space

    SmallVector<GrfParamZone, 16> FreeZones;
    FreeZones.push_back(GrfParamZone(GrfStartOffset, GrfEndOffset));

    // Repeatedly iterate over the arguments list, each time looking for the
    // largest one that hasn't been processed yet.
    // But ignore implicit args for now as they want to go after all the others.

    do {
      Argument *BestArg = nullptr;
      unsigned BestSize;
      unsigned BestElemSize;

      auto ArgKinds = KM.getArgKinds();
      auto Kind = ArgKinds.begin();
      for (Function::arg_iterator i = F->arg_begin(), e = F->arg_end(); i != e;
           ++i, ++Kind) {
        Argument *Arg = &*i;
        if (*Kind & 0xf8)
          continue; // implicit arg

        if (PlacedArgs.find(Arg) != PlacedArgs.end())
          // Already done this one.
          continue;

        Type *Ty = Arg->getType();
        unsigned Bytes = getTypeSizeInBytes(Ty);

        if (BestArg == nullptr || BestSize < Bytes) {
          BestArg = Arg;
          BestSize = Bytes;
          BestElemSize = getTypeSizeInBytes(Ty->getScalarType());
        }
      }

      if (BestArg == nullptr)
        // All done.
        break;

      // The best argument in this cycle has been found. Search FreeZones for
      // a suitably sized and aligned gap.

      unsigned Align;

      if (BestSize > GrfByteSize)
        Align = GrfByteSize;
      else
        Align = BestElemSize;

      auto zi = FreeZones.begin();
      auto ze = FreeZones.end();

      unsigned Start = 0, End = 0;

      for (; zi != ze; ++zi) {
        GrfParamZone &Zone = *zi;

        Start = alignTo(Zone.Start, Align);
        End = Start + BestSize;

        if ((Start % GrfByteSize) != 0 &&
            (Start / GrfByteSize) != (End - 1) / GrfByteSize) {
          Start = alignTo(Zone.Start, GrfByteSize);
          End = Start + BestSize;
        }

        if (End <= Zone.End)
          // Found one. This should never fail unless we have too many
          // parameters to start with.
          break;
      }

      assert(zi != ze &&
             "unable to allocate argument offset (too many arguments?)");

      // Exclude the found block from the free zones list. This may require
      // that the found zone be split in two if the start of the block is
      // not suitably aligned.

      GrfParamZone &Zone = *zi;

      if (Zone.Start == Start)
        Zone.Start = End;
      else {
        unsigned NewEnd = Zone.End;
        Zone.End = Start;
        ++zi;
        FreeZones.insert(zi, GrfParamZone(End, NewEnd));
      }

      PlacedArgs[BestArg] = Start;
    } while (true);
    // Now process the implicit args. First get the offset at the start of the
    // last free zone. Process the implicit kernel args first, then the
    // implicit thread args.
    Offset = FreeZones.back().Start;
    for (int WantThreadImplicit = 0; WantThreadImplicit != 2;
         ++WantThreadImplicit) {
      bool FirstThreadImplicit = WantThreadImplicit;
      auto ArgKinds = KM.getArgKinds();
      auto Kind = ArgKinds.begin();
      for (Function::arg_iterator i = F->arg_begin(), e = F->arg_end(); i != e;
           ++i, ++Kind) {
        Argument *Arg = &*i;
        if (!(*Kind & 0xf8))
          continue;                               // not implicit arg
        int IsThreadImplicit = (*Kind >> 3) == 3; // local_id
        if (WantThreadImplicit != IsThreadImplicit)
          continue;
        Type *Ty = Arg->getType();
        unsigned Bytes = Ty->getPrimitiveSizeInBits() / 8U;
        unsigned Align = Ty->getScalarSizeInBits() / 8U;
        // If this is the first thread implicit arg, put it in a new GRF.
        if (FirstThreadImplicit)
          Align = GrfByteSize;
        FirstThreadImplicit = false;
        Offset = alignTo(Offset, Align);
        if ((Offset & (GrfByteSize - 1)) + Bytes > GrfByteSize) {
          // GRF align if arg would cross GRF boundary
          Offset = alignTo(Offset, GrfByteSize);
        }
        PlacedArgs[Arg] = Offset;
        Offset += Bytes;
      }
    }
  } else {
    // No argument reordering. Arguments are placed at increasing offsets
    // in their natural order, aligned according to their type.
    //
    // Again, arguments that are at least one GRF in size must be aligned to
    // a GRF boundary. Arguments smaller than a GRF must not cross a GRF
    // boundary.

    // kernel input start offset
    auto &DL = F->getParent()->getDataLayout();
    Offset = GrfStartOffset;

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
      PlacedArgs[Arg] = Offset;
      Offset += ByteSize;
    };

    for (auto &Arg : F->args()) {
      Type *Ty = Arg.getType();
      unsigned Bytes = 0, Alignment = 0;
      if (Ty->isPointerTy()) {
        Bytes = DL.getPointerTypeSize(Ty);
        Alignment = IGCLLVM::getAlignmentValue(
            DL.getPointerABIAlignment(Ty->getPointerAddressSpace()));
      } else {
        Bytes = Ty->getPrimitiveSizeInBits() / 8;
        Alignment = IGCLLVM::getAlignmentValue(Ty->getScalarSizeInBits() / 8);
      }
      placeArg(&Arg, Bytes, Alignment);
    }
  }

  // Update the offset MD node.
  updateOffsetMD(Node, PlacedArgs);

  this->KM = nullptr;
}

/***********************************************************************
 * DiagnosticInfoCMKernelArgOffset::emit : emit an error or warning
 */
void DiagnosticInfoCMKernelArgOffset::emit(Instruction *Inst, StringRef Msg,
                                           DiagnosticSeverity Severity) {
  DiagnosticInfoCMKernelArgOffset Err(Severity, *Inst->getParent()->getParent(),
                                      Inst->getDebugLoc(), Msg);
  Inst->getContext().diagnose(Err);
}

void CMKernelArgOffset::processKernelOnOCLRT(MDNode *Node, Function *F) {
  assert(KM);
  // Assign BTI values.
  {
    unsigned Idx = 0;
    auto ArgKinds = KM->getArgKinds();
    auto Kind = ArgKinds.begin();
    for (auto &Arg : F->args()) {
      if (*Kind == genx::KernelMetadata::AK_SAMPLER ||
          *Kind == genx::KernelMetadata::AK_SURFACE) {
        int32_t BTI = KM->getBTI(Idx);
        assert(BTI >= 0 && "unassigned BTI");

        Type *ArgTy = Arg.getType();
        if (ArgTy->isPointerTy()) {
          SmallVector<Instruction *, 8> ToErase;

          assert(Arg.hasOneUse() && "invalid surface input");
          auto ArgUse = Arg.use_begin()->getUser();
          assert(isa<PtrToIntInst>(ArgUse) && "invalid surface input usage");
          ToErase.push_back(cast<Instruction>(ArgUse));

          for (auto ui = ArgUse->use_begin(), ue = ArgUse->use_end(); ui != ue;
               ++ui) {
            auto User = cast<Instruction>(ui->getUser());
            User->replaceAllUsesWith(
                ConstantInt::get(User->getType(), BTI));
            ToErase.push_back(User);
          }

          for (auto i = ToErase.rbegin(), e = ToErase.rend(); i != e; ++i)
            (*i)->eraseFromParent();
          ToErase.clear();
        } else {
          auto BTIConstant = ConstantInt::get(ArgTy, BTI);
          // If the number of uses for this arg more than 1 it's better to
          // create a separate instruction for the constant. Otherwise, category
          // conversion will create a separate ".categoryconv" instruction for
          // each use of the arg. As a result, each conversion instruction will
          // be materialized as movs to a surface var. This will increase
          // register pressure and the number of instructions. But if there is
          // only one use, it will be okay to wait for the replacement until
          // category conv do it.
          if (Arg.getNumUses() > 1) {
            auto ID = ArgTy->isFPOrFPVectorTy() ? GenXIntrinsic::genx_constantf
                                                : GenXIntrinsic::genx_constanti;
            Module *M = F->getParent();
            Function *Decl = GenXIntrinsic::getGenXDeclaration(M, ID, ArgTy);
            auto NewInst =
                CallInst::Create(Decl, BTIConstant, Arg.getName() + ".bti",
                                 &*F->begin()->begin());
            Arg.replaceAllUsesWith(NewInst);
          } else
            Arg.replaceAllUsesWith(BTIConstant);
        }
      }
      ++Kind, ++Idx;
    }
  }

  SmallDenseMap<Argument *, unsigned> PlacedArgs;
  {
    // OpenCL SIMD8 thread payloads are organized as follows:
    //
    //     0        1        2        3        4        5        6        7
    // R0:          GX                                           GY       GZ
    // R1: LIDx LIDy LIDz
    //
    unsigned Offset = GrfStartOffset;

    unsigned ThreadPayloads[] = {
        Offset // R1, local_id_x, local_id_y, local_id_z
    };
    auto getImpOffset = [&](genx::KernelArgInfo AI) -> int {
      if (AI.isLocalIDs())
        return ThreadPayloads[0];
      return -1;
    };

    // Starting offsets for non-implicit arguments.
    Offset += 1 * GrfByteSize;

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
      PlacedArgs[Arg] = Offset;
      Offset += ByteSize;
    };

    // First scan, assign implicit arguments.
    auto ArgKinds = KM->getArgKinds();
    auto Kind = ArgKinds.begin();
    for (auto &Arg : F->args()) {
      genx::KernelArgInfo AI(*Kind++);
      int ImpOffset = getImpOffset(AI);
      if (ImpOffset > 0) {
        PlacedArgs[&Arg] = ImpOffset;
        continue;
      }

      if (AI.isLocalSize() || AI.isGroupCount() || AI.isPrintBuffer() ||
          AI.isPrivateBase()) {
        unsigned Bytes = Arg.getType()->getPrimitiveSizeInBits() / 8;
        unsigned Align = Arg.getType()->getScalarSizeInBits() / 8;
        placeArg(&Arg, Bytes, Align);
      }
    }

    // Second scan, assign normal arguments.
    Kind = ArgKinds.begin();
    unsigned Idx = 0;
    for (auto &Arg : F->args()) {
      genx::KernelArgInfo AI(*Kind++);
      bool IsBuffer = KM->isBufferType(Idx++);

      // Skip alaready assigned arguments.
      if (PlacedArgs.count(&Arg))
        continue;

      // image/sampler arguments do not allocate vISA inputs
      // buffer arguments do allocate unused vISA inputs
      if (!AI.isNormalCategory() && !IsBuffer) {
        PlacedArgs[&Arg] = genx::KernelMetadata::SKIP_OFFSET_VAL;
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
        Bytes = DL.getPointerTypeSize(Ty);
        Alignment = IGCLLVM::getAlignmentValue(
            DL.getPointerABIAlignment(Ty->getPointerAddressSpace()));
      } else {
        Bytes = Ty->getPrimitiveSizeInBits() / 8;
        Alignment = IGCLLVM::getAlignmentValue(Ty->getScalarSizeInBits() / 8);
      }
      placeArg(&Arg, Bytes, Alignment);
    }
  }

  updateOffsetMD(Node, PlacedArgs);
}
