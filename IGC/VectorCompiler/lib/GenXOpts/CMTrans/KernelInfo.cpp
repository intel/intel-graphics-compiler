/*========================== begin_copyright_notice ============================

Copyright (c) 2021-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "llvmWrapper/IR/Function.h"

namespace llvm {
namespace genx {

static MDNode *findNode(const Function *F, StringRef KernelsMDName,
                        unsigned KernelRefOp, unsigned MustExceed) {
  IGC_ASSERT(F);
  NamedMDNode *Named = F->getParent()->getNamedMetadata(KernelsMDName);
  // It's expected that in any case internal and external metadata nodes have
  // already been created by createInternalMD() or vc-intrinsics.
  if (!Named)
    return nullptr;
  auto Res = std::find_if(
      Named->op_begin(), Named->op_end(), [=](const MDNode *InternalMD) {
        return InternalMD->getNumOperands() >= MustExceed &&
               F == getValueAsMetadata(InternalMD->getOperand(KernelRefOp));
      });
  return Res != Named->op_end() ? *Res : nullptr;
}

static MDNode *findInternalNode(const Function *F) {
  return findNode(F, FunctionMD::GenXKernelInternal,
                  internal::KernelMDOp::FunctionRef,
                  internal::KernelMDOp::Last);
}

static MDNode *findExternalNode(const Function *F) {
  return findNode(F, FunctionMD::GenXKernels, KernelMDOp::FunctionRef,
                  KernelMDOp::ArgTypeDescs);
}

namespace internal {
void createInternalMD(Function *F) {
  IGC_ASSERT(F);
  IGC_ASSERT_MESSAGE(!findInternalNode(F),
                     "Internal node has already been created!");

  auto &Ctx = F->getContext();

  // Create nullptr values by default.
  SmallVector<Metadata *, internal::KernelMDOp::Last> KernelInternalMD(
      internal::KernelMDOp::Last, nullptr);
  KernelInternalMD[internal::KernelMDOp::FunctionRef] = ValueAsMetadata::get(F);

  MDNode *InternalNode = MDNode::get(Ctx, KernelInternalMD);
  NamedMDNode *KernelMDs =
      F->getParent()->getOrInsertNamedMetadata(FunctionMD::GenXKernelInternal);
  KernelMDs->addOperand(InternalNode);
}

void replaceInternalFunctionRef(const Function *From, Function *To) {
  IGC_ASSERT(From);
  IGC_ASSERT(To);

  MDNode *Node = findInternalNode(From);
  IGC_ASSERT_MESSAGE(Node, "Replacement was called for non existing in kernel "
                           "internal metadata function");
  Node->replaceOperandWith(internal::KernelMDOp::FunctionRef,
                           ValueAsMetadata::get(To));
}
} // namespace internal

static unsigned extractConstantIntMD(unsigned OpNo, unsigned OpNum,
                                     const MDNode *ValuesNode) {
  if (ValuesNode == nullptr)
    return 0;

  IGC_ASSERT_MESSAGE(ValuesNode->getNumOperands() == OpNum, "out of sync");
  return getValueAsMetadata<ConstantInt>(ValuesNode->getOperand(OpNo))
      ->getZExtValue();
}

static ImplicitLinearizationInfo
extractImplicitLinearizationArg(const Function &F,
                                const MDOperand &ImplicitArg) {
  auto *MD = cast<MDNode>(ImplicitArg.get());
  IGC_ASSERT(MD->getNumOperands() == internal::LinearizationMDOp::Last);
  Constant *ArgNoValue =
      cast<ConstantAsMetadata>(
          MD->getOperand(internal::LinearizationMDOp::Argument).get())
          ->getValue();
  unsigned ArgNo = cast<ConstantInt>(ArgNoValue)->getZExtValue();
  Argument *Arg = IGCLLVM::getArg(F, ArgNo);
  auto *OffsetMD = cast<ConstantAsMetadata>(
      MD->getOperand(internal::LinearizationMDOp::Offset).get());
  return ImplicitLinearizationInfo{Arg,
                                   cast<ConstantInt>(OffsetMD->getValue())};
}

static ArgToImplicitLinearization::value_type
extractArgLinearization(const Function &F, const MDOperand &MDOp) {
  auto *ArgLinearizationMD = cast<MDNode>(MDOp.get());
  IGC_ASSERT(ArgLinearizationMD->getNumOperands() ==
             internal::ArgLinearizationMDOp::Last);
  Constant *ExplicitArgNo =
      cast<ConstantAsMetadata>(
          ArgLinearizationMD
              ->getOperand(internal::ArgLinearizationMDOp::Explicit)
              .get())
          ->getValue();
  Argument *ExplicitArg =
      IGCLLVM::getArg(F, cast<ConstantInt>(ExplicitArgNo)->getZExtValue());
  auto *LinMD = cast<MDNode>(
      ArgLinearizationMD
          ->getOperand(internal::ArgLinearizationMDOp::Linearization)
          .get());
  LinearizedArgInfo Info;
  std::transform(LinMD->op_begin(), LinMD->op_end(), std::back_inserter(Info),
                 [&F](const MDOperand &ImplicitArg) {
                   return extractImplicitLinearizationArg(F, ImplicitArg);
                 });
  return std::make_pair(ExplicitArg, std::move(Info));
}

static ArgToImplicitLinearization
extractLinearizationMD(const Function &F, const MDNode *LinearizationNode) {
  IGC_ASSERT(LinearizationNode);
  ArgToImplicitLinearization Linearization;
  std::transform(
      LinearizationNode->op_begin(), LinearizationNode->op_end(),
      std::inserter(Linearization, Linearization.end()),
      [&F](const MDOperand &MDOp) { return extractArgLinearization(F, MDOp); });
  return Linearization;
}

KernelMetadata::KernelMetadata(const Function *F) {
  if (!genx::isKernel(F))
    return;

  ExternalNode = findExternalNode(F);
  if (!ExternalNode)
    return;

  // ExternalNode is the metadata node for F, and it has the required number of
  // operands.
  this->F = F;
  IsKernel = true;
  if (MDString *MDS =
          dyn_cast<MDString>(ExternalNode->getOperand(KernelMDOp::Name)))
    Name = MDS->getString();
  if (ConstantInt *Sz = getValueAsMetadata<ConstantInt>(
          ExternalNode->getOperand(KernelMDOp::SLMSize)))
    SLMSize = Sz->getZExtValue();
  // Build the argument kinds and offsets arrays that should correspond to the
  // function arguments (both explicit and implicit)
  MDNode *KindsNode =
      dyn_cast<MDNode>(ExternalNode->getOperand(KernelMDOp::ArgKinds));
  MDNode *OffsetsNode =
      dyn_cast<MDNode>(ExternalNode->getOperand(KernelMDOp::ArgOffsets));
  MDNode *InputOutputKinds =
      dyn_cast<MDNode>(ExternalNode->getOperand(KernelMDOp::ArgIOKinds));
  MDNode *ArgDescNode =
      dyn_cast<MDNode>(ExternalNode->getOperand(KernelMDOp::ArgTypeDescs));

  MDNode *IndexesNode = nullptr;
  MDNode *OffsetInArgsNode = nullptr;
  MDNode *LinearizationNode = nullptr;
  InternalNode = findInternalNode(F);
  IGC_ASSERT_MESSAGE(InternalNode,
                     "Internal node is expected to have already been created!");

  IndexesNode = cast_or_null<MDNode>(
      InternalNode->getOperand(internal::KernelMDOp::ArgIndexes));
  OffsetInArgsNode = cast_or_null<MDNode>(
      InternalNode->getOperand(internal::KernelMDOp::OffsetInArgs));
  LinearizationNode = cast_or_null<MDNode>(
      InternalNode->getOperand(internal::KernelMDOp::LinearizationArgs));

  IGC_ASSERT(KindsNode);

  for (unsigned i = 0, e = KindsNode->getNumOperands(); i != e; ++i) {
    ArgKinds.push_back(extractConstantIntMD(i, e, KindsNode));
    ArgOffsets.push_back(extractConstantIntMD(i, e, OffsetsNode));
    OffsetInArgs.push_back(extractConstantIntMD(i, e, OffsetInArgsNode));
    ArgIndexes.push_back(extractConstantIntMD(i, e, IndexesNode));
  }
  IGC_ASSERT(InputOutputKinds);
  IGC_ASSERT(KindsNode->getNumOperands() >= InputOutputKinds->getNumOperands());
  for (unsigned i = 0, e = InputOutputKinds->getNumOperands(); i != e; ++i)
    ArgIOKinds.push_back(extractConstantIntMD(i, e, InputOutputKinds));

  IGC_ASSERT(ArgDescNode);
  for (unsigned i = 0, e = ArgDescNode->getNumOperands(); i < e; ++i) {
    MDString *MDS = dyn_cast<MDString>(ArgDescNode->getOperand(i));
    IGC_ASSERT(MDS);
    ArgTypeDescs.push_back(MDS->getString());
  }
  if (LinearizationNode)
    Linearization = extractLinearizationMD(*F, LinearizationNode);
}

static MDNode *createArgLinearizationMD(const ImplicitLinearizationInfo &Info) {
  auto &Ctx = Info.Arg->getContext();
  auto *I32Ty = Type::getInt32Ty(Ctx);
  Metadata *ArgMD =
      ConstantAsMetadata::get(ConstantInt::get(I32Ty, Info.Arg->getArgNo()));
  Metadata *OffsetMD = ConstantAsMetadata::get(Info.Offset);
  return MDNode::get(Ctx, {ArgMD, OffsetMD});
}

void KernelMetadata::updateLinearizationMD(ArgToImplicitLinearization &&Lin) {
  Linearization = std::move(Lin);

  std::vector<Metadata *> LinMDs;
  LinMDs.reserve(Linearization.size());
  auto &Ctx = F->getContext();
  for (const auto &ArgLin : Linearization) {
    std::vector<Metadata *> ArgLinMDs;
    ArgLinMDs.reserve(ArgLin.second.size());
    std::transform(ArgLin.second.begin(), ArgLin.second.end(),
                   std::back_inserter(ArgLinMDs), createArgLinearizationMD);
    auto *I32Ty = Type::getInt32Ty(Ctx);
    Metadata *ExplicitArgMD = ConstantAsMetadata::get(
        ConstantInt::get(I32Ty, ArgLin.first->getArgNo()));
    Metadata *ExplicitArgLinMD = MDNode::get(Ctx, ArgLinMDs);
    LinMDs.push_back(MDNode::get(Ctx, {ExplicitArgMD, ExplicitArgLinMD}));
  }
  InternalNode->replaceOperandWith(internal::KernelMDOp::LinearizationArgs,
                                   MDNode::get(Ctx, LinMDs));
}

void KernelMetadata::updateArgsMD(const SmallVectorImpl<unsigned> &Values,
                                  MDNode *Node, unsigned NodeOpNo) const {
  IGC_ASSERT(F);
  IGC_ASSERT(Node);
  IGC_ASSERT_MESSAGE(Values.size() == getNumArgs(),
                     "Mismatch between metadata for kernel and number of args");
  IGC_ASSERT(Node->getNumOperands() > NodeOpNo);
  auto &Ctx = F->getContext();
  auto *I32Ty = Type::getInt32Ty(Ctx);
  SmallVector<Metadata *, 8> NewMD;
  std::transform(Values.begin(), Values.end(), std::back_inserter(NewMD),
                 [I32Ty](unsigned Value) {
                   return ValueAsMetadata::getConstant(
                       ConstantInt::get(I32Ty, Value));
                 });
  MDNode *NewNode = MDNode::get(Ctx, NewMD);
  Node->replaceOperandWith(NodeOpNo, NewNode);
}

void KernelMetadata::updateArgOffsetsMD(SmallVectorImpl<unsigned> &&Offsets) {
  ArgOffsets = std::move(Offsets);
  updateArgsMD(ArgOffsets, ExternalNode, KernelMDOp::ArgOffsets);
}
void KernelMetadata::updateArgKindsMD(SmallVectorImpl<unsigned> &&Kinds) {
  ArgKinds = std::move(Kinds);
  updateArgsMD(ArgKinds, ExternalNode, KernelMDOp::ArgKinds);
}
void KernelMetadata::updateArgIndexesMD(SmallVectorImpl<unsigned> &&Indexes) {
  ArgIndexes = std::move(Indexes);
  updateArgsMD(ArgIndexes, InternalNode, internal::KernelMDOp::ArgIndexes);
}
void KernelMetadata::updateOffsetInArgsMD(SmallVectorImpl<unsigned> &&Offsets) {
  OffsetInArgs = std::move(Offsets);
  updateArgsMD(OffsetInArgs, InternalNode, internal::KernelMDOp::OffsetInArgs);
}

} // namespace genx
} // namespace llvm
