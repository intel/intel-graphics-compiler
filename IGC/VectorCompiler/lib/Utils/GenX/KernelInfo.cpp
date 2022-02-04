/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/KernelInfo.h"

#include "llvmWrapper/IR/Function.h"

using namespace llvm;
using namespace vc;

static MDNode *findNode(const Function &F, StringRef KernelsMDName,
                        unsigned KernelRefOp, unsigned MustExceed) {
  NamedMDNode *Named = F.getParent()->getNamedMetadata(KernelsMDName);
  // It's expected that in any case internal and external metadata nodes have
  // already been created by createInternalMD() or vc-intrinsics.
  if (!Named)
    return nullptr;
  auto Res = std::find_if(
      Named->op_begin(), Named->op_end(),
      [&F, KernelRefOp, MustExceed](const MDNode *InternalMD) {
        return InternalMD->getNumOperands() >= MustExceed &&
               &F == getValueAsMetadata(InternalMD->getOperand(KernelRefOp));
      });
  return Res != Named->op_end() ? *Res : nullptr;
}

static MDNode *findInternalNode(const Function &F) {
  return findNode(F, FunctionMD::GenXKernelInternal,
                  internal::KernelMDOp::FunctionRef,
                  internal::KernelMDOp::Last);
}

static MDNode *findExternalNode(const Function &F) {
  return findNode(F, genx::FunctionMD::GenXKernels,
                  genx::KernelMDOp::FunctionRef,
                  genx::KernelMDOp::ArgTypeDescs);
}

void vc::internal::createInternalMD(Function &F) {
  IGC_ASSERT_MESSAGE(!findInternalNode(F),
                     "Internal node has already been created!");

  auto &Ctx = F.getContext();

  // Create nullptr values by default.
  SmallVector<Metadata *, internal::KernelMDOp::Last> KernelInternalMD(
      internal::KernelMDOp::Last, nullptr);
  KernelInternalMD[internal::KernelMDOp::FunctionRef] =
      ValueAsMetadata::get(&F);

  MDNode *InternalNode = MDNode::get(Ctx, KernelInternalMD);
  NamedMDNode *KernelMDs =
      F.getParent()->getOrInsertNamedMetadata(FunctionMD::GenXKernelInternal);
  KernelMDs->addOperand(InternalNode);
}

void vc::internal::replaceInternalFunctionRef(const Function &From,
                                              Function &To) {
  MDNode *Node = findInternalNode(From);
  IGC_ASSERT_MESSAGE(Node, "Replacement was called for non existing in kernel "
                           "internal metadata function");
  Node->replaceOperandWith(internal::KernelMDOp::FunctionRef,
                           ValueAsMetadata::get(&To));
}

void vc::replaceFunctionRefMD(const Function &From, Function &To) {
  Module *M = To.getParent();
  NamedMDNode *Named = M->getNamedMetadata(genx::FunctionMD::GenXKernels);
  IGC_ASSERT(Named);

  auto Res =
      std::find_if(Named->op_begin(), Named->op_end(), [&From](MDNode *Node) {
        auto *NodeVal = cast<ValueAsMetadata>(
                            Node->getOperand(genx::KernelMDOp::FunctionRef))
                            ->getValue();
        auto *F = cast<Function>(NodeVal);
        return &From == F;
      });
  IGC_ASSERT_MESSAGE(Res != Named->op_end(),
                     "Cannot find MD for 'From' function");

  MDNode *FromNode = *Res;
  FromNode->replaceOperandWith(genx::KernelMDOp::FunctionRef,
                               ValueAsMetadata::get(&To));

  internal::replaceInternalFunctionRef(From, To);
}

template <typename RetTy = unsigned>
static RetTy extractConstantIntMD(const MDOperand &Op) {
  const auto *V = getValueAsMetadata<ConstantInt>(Op);
  IGC_ASSERT_MESSAGE(V, "Unexpected null value in metadata");
  return static_cast<RetTy>(V->getZExtValue());
}

template <typename Cont>
static void extractConstantsFromMDNode(const MDNode *N, Cont &C) {
  if (!N)
    return;
  using ValTy = typename Cont::value_type;
  std::transform(
      N->op_begin(), N->op_end(), std::back_inserter(C),
      [](const MDOperand &Op) { return extractConstantIntMD<ValTy>(Op); });
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

vc::KernelMetadata::KernelMetadata(const Function *F) {
  if (!vc::isKernel(F))
    return;

  ExternalNode = findExternalNode(*F);
  if (!ExternalNode)
    return;

  // ExternalNode is the metadata node for F, and it has the required number of
  // operands.
  this->F = F;
  IsKernel = true;
  if (MDString *MDS =
          dyn_cast<MDString>(ExternalNode->getOperand(genx::KernelMDOp::Name)))
    Name = MDS->getString();
  if (ConstantInt *Sz = getValueAsMetadata<ConstantInt>(
          ExternalNode->getOperand(genx::KernelMDOp::SLMSize)))
    SLMSize = Sz->getZExtValue();
  if (ExternalNode->getNumOperands() > genx::KernelMDOp::NBarrierCnt)
    if (ConstantInt *Sz = getValueAsMetadata<ConstantInt>(
            ExternalNode->getOperand(genx::KernelMDOp::NBarrierCnt)))
      NBarrierCnt = Sz->getZExtValue();
  // Build the argument kinds and offsets arrays that should correspond to the
  // function arguments (both explicit and implicit)
  MDNode *KindsNode =
      dyn_cast<MDNode>(ExternalNode->getOperand(genx::KernelMDOp::ArgKinds));
  MDNode *OffsetsNode =
      dyn_cast<MDNode>(ExternalNode->getOperand(genx::KernelMDOp::ArgOffsets));
  MDNode *InputOutputKinds =
      dyn_cast<MDNode>(ExternalNode->getOperand(genx::KernelMDOp::ArgIOKinds));
  MDNode *ArgDescNode = dyn_cast<MDNode>(
      ExternalNode->getOperand(genx::KernelMDOp::ArgTypeDescs));

  MDNode *IndexesNode = nullptr;
  MDNode *OffsetInArgsNode = nullptr;
  MDNode *LinearizationNode = nullptr;
  MDNode *BTIndicesNode = nullptr;
  InternalNode = findInternalNode(*F);
  IGC_ASSERT_MESSAGE(InternalNode,
                     "Internal node is expected to have already been created!");

  IndexesNode = cast_or_null<MDNode>(
      InternalNode->getOperand(internal::KernelMDOp::ArgIndexes));
  OffsetInArgsNode = cast_or_null<MDNode>(
      InternalNode->getOperand(internal::KernelMDOp::OffsetInArgs));
  LinearizationNode = cast_or_null<MDNode>(
      InternalNode->getOperand(internal::KernelMDOp::LinearizationArgs));
  BTIndicesNode = cast_or_null<MDNode>(
      InternalNode->getOperand(internal::KernelMDOp::BTIndices));

  IGC_ASSERT(KindsNode);

  // These should have the same number of operands if they exist.
  IGC_ASSERT(!OffsetsNode ||
             KindsNode->getNumOperands() == OffsetsNode->getNumOperands());
  IGC_ASSERT(!OffsetInArgsNode ||
             KindsNode->getNumOperands() == OffsetInArgsNode->getNumOperands());
  IGC_ASSERT(!IndexesNode ||
             KindsNode->getNumOperands() == IndexesNode->getNumOperands());
  IGC_ASSERT(!BTIndicesNode ||
             KindsNode->getNumOperands() == BTIndicesNode->getNumOperands());

  extractConstantsFromMDNode(KindsNode, ArgKinds);
  extractConstantsFromMDNode(OffsetsNode, ArgOffsets);
  extractConstantsFromMDNode(OffsetInArgsNode, OffsetInArgs);
  extractConstantsFromMDNode(IndexesNode, ArgIndexes);
  extractConstantsFromMDNode(BTIndicesNode, BTIs);

  IGC_ASSERT(InputOutputKinds);
  IGC_ASSERT(KindsNode->getNumOperands() >= InputOutputKinds->getNumOperands());
  extractConstantsFromMDNode(InputOutputKinds, ArgIOKinds);

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

void vc::KernelMetadata::updateLinearizationMD(
    ArgToImplicitLinearization &&Lin) {
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

template <typename InputIt>
void vc::KernelMetadata::updateArgsMD(InputIt Begin, InputIt End, MDNode *Node,
                                      unsigned NodeOpNo) const {
  IGC_ASSERT(F);
  IGC_ASSERT(Node);
  IGC_ASSERT_MESSAGE(std::distance(Begin, End) == getNumArgs(),
                     "Mismatch between metadata for kernel and number of args");
  IGC_ASSERT(Node->getNumOperands() > NodeOpNo);
  auto &Ctx = F->getContext();
  auto *I32Ty = Type::getInt32Ty(Ctx);
  SmallVector<Metadata *, 8> NewMD;
  std::transform(Begin, End, std::back_inserter(NewMD), [I32Ty](auto Value) {
    return ValueAsMetadata::getConstant(ConstantInt::get(I32Ty, Value));
  });
  MDNode *NewNode = MDNode::get(Ctx, NewMD);
  Node->replaceOperandWith(NodeOpNo, NewNode);
}

void vc::KernelMetadata::updateArgOffsetsMD(
    SmallVectorImpl<unsigned> &&Offsets) {
  ArgOffsets = std::move(Offsets);
  updateArgsMD(ArgOffsets.begin(), ArgOffsets.end(), ExternalNode,
               genx::KernelMDOp::ArgOffsets);
}

void vc::KernelMetadata::updateArgKindsMD(SmallVectorImpl<unsigned> &&Kinds) {
  ArgKinds = std::move(Kinds);
  updateArgsMD(ArgKinds.begin(), ArgKinds.end(), ExternalNode,
               genx::KernelMDOp::ArgKinds);
}

void vc::KernelMetadata::updateArgIndexesMD(
    SmallVectorImpl<unsigned> &&Indexes) {
  ArgIndexes = std::move(Indexes);
  updateArgsMD(ArgIndexes.begin(), ArgIndexes.end(), InternalNode,
               internal::KernelMDOp::ArgIndexes);
}

void vc::KernelMetadata::updateOffsetInArgsMD(
    SmallVectorImpl<unsigned> &&Offsets) {
  OffsetInArgs = std::move(Offsets);
  updateArgsMD(OffsetInArgs.begin(), OffsetInArgs.end(), InternalNode,
               internal::KernelMDOp::OffsetInArgs);
}

void vc::KernelMetadata::updateBTIndicesMD(std::vector<int> &&BTIndices) {
  BTIs = std::move(BTIndices);
  updateArgsMD(BTIs.begin(), BTIs.end(), InternalNode,
               internal::KernelMDOp::BTIndices);
}

bool vc::hasKernel(const Module &M) {
  NamedMDNode *KernelsMD = M.getNamedMetadata(genx::FunctionMD::GenXKernels);
  if (!KernelsMD)
    return false;
  return KernelsMD->getNumOperands();
}
