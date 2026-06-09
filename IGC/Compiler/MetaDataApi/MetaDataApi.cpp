/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MetaDataApi.h"
#include "Probe/Assertion.h"

namespace IGC::IGCMD {
ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const llvm::MDNode *pNode, bool hasId)
    : _Mybase(pNode, hasId), m_Arg(getNumberedNode(pNode, 0)), m_ArgDependency(getNumberedNode(pNode, 1)),
      m_pNode(pNode) {}

ArgDependencyInfoMetaData::ArgDependencyInfoMetaData() : m_pNode(nullptr) {}

ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const char *name) : _Mybase(name), m_pNode(nullptr) {}

bool ArgDependencyInfoMetaData::hasValue() const {
  return m_Arg.hasValue() || m_ArgDependency.hasValue() || nullptr != m_pNode || dirty();
}

bool ArgDependencyInfoMetaData::dirty() const { return m_Arg.dirty() || m_ArgDependency.dirty(); }

void ArgDependencyInfoMetaData::discardChanges() {
  m_Arg.discardChanges();
  m_ArgDependency.discardChanges();
}

llvm::Metadata *ArgDependencyInfoMetaData::generateNode(llvm::LLVMContext &context) const {
  llvm::SmallVector<llvm::Metadata *, 5> args;

  llvm::Metadata *pIDNode = IMetaDataObject::generateNode(context);
  if (nullptr != pIDNode) {
    args.push_back(pIDNode);
  }

  args.push_back(m_Arg.generateNode(context));
  args.push_back(m_ArgDependency.generateNode(context));

  return llvm::MDNode::get(context, args);
}

void ArgDependencyInfoMetaData::save(llvm::LLVMContext &context, llvm::MDNode *pNode) const {
  IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

  // we assume that underlying metadata node has not changed under our foot
  if (pNode == m_pNode && !dirty()) {
    return;
  }

  m_Arg.save(context, getNumberedNode(pNode, 0));
  m_ArgDependency.save(context, getNumberedNode(pNode, 1));
}

SubGroupSizeMetaData::SubGroupSizeMetaData(const llvm::MDNode *pNode, bool hasId)
    : _Mybase(pNode, hasId), m_SIMDSize(getNumberedNode(pNode, 0)), m_pNode(pNode) {}

SubGroupSizeMetaData::SubGroupSizeMetaData() : m_pNode(nullptr) {}

SubGroupSizeMetaData::SubGroupSizeMetaData(const char *name) : _Mybase(name), m_pNode(nullptr) {}

bool SubGroupSizeMetaData::hasValue() const { return m_SIMDSize.hasValue() || nullptr != m_pNode || dirty(); }

bool SubGroupSizeMetaData::dirty() const { return m_SIMDSize.dirty(); }

void SubGroupSizeMetaData::discardChanges() { m_SIMDSize.discardChanges(); }

llvm::Metadata *SubGroupSizeMetaData::generateNode(llvm::LLVMContext &context) const {
  llvm::SmallVector<llvm::Metadata *, 5> args;

  llvm::Metadata *pIDNode = IMetaDataObject::generateNode(context);
  if (nullptr != pIDNode) {
    args.push_back(pIDNode);
  }

  args.push_back(m_SIMDSize.generateNode(context));

  return llvm::MDNode::get(context, args);
}

void SubGroupSizeMetaData::save(llvm::LLVMContext &context, llvm::MDNode *pNode) const {
  IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

  // we assume that underlying metadata node has not changed under our foot
  if (pNode == m_pNode && !dirty()) {
    return;
  }

  m_SIMDSize.save(context, getNumberedNode(pNode, 0));
}

VectorTypeHintMetaData::VectorTypeHintMetaData(const llvm::MDNode *pNode, bool hasId)
    : _Mybase(pNode, hasId), m_VecType(getNumberedNode(pNode, 0)), m_Sign(getNumberedNode(pNode, 1)), m_pNode(pNode) {}

VectorTypeHintMetaData::VectorTypeHintMetaData() : m_pNode(nullptr) {}

VectorTypeHintMetaData::VectorTypeHintMetaData(const char *name) : _Mybase(name), m_pNode(nullptr) {}

bool VectorTypeHintMetaData::hasValue() const {
  return m_VecType.hasValue() || m_Sign.hasValue() || nullptr != m_pNode || dirty();
}

bool VectorTypeHintMetaData::dirty() const { return m_VecType.dirty() || m_Sign.dirty(); }

void VectorTypeHintMetaData::discardChanges() {
  m_VecType.discardChanges();
  m_Sign.discardChanges();
}

llvm::Metadata *VectorTypeHintMetaData::generateNode(llvm::LLVMContext &context) const {
  llvm::SmallVector<llvm::Metadata *, 5> args;

  llvm::Metadata *pIDNode = IMetaDataObject::generateNode(context);
  if (nullptr != pIDNode) {
    args.push_back(pIDNode);
  }

  args.push_back(m_VecType.generateNode(context));
  args.push_back(m_Sign.generateNode(context));

  return llvm::MDNode::get(context, args);
}

void VectorTypeHintMetaData::save(llvm::LLVMContext &context, llvm::MDNode *pNode) const {
  IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

  // we assume that underlying metadata node has not changed under our foot
  if (pNode == m_pNode && !dirty()) {
    return;
  }

  m_VecType.save(context, getNumberedNode(pNode, 0));
  m_Sign.save(context, getNumberedNode(pNode, 1));
}

FunctionInfoMetaData::FunctionInfoMetaData(const llvm::MDNode *pNode, bool hasId)
    : _Mybase(pNode, hasId), m_Type(getNamedNode(pNode, "function_type")),
      m_SubGroupSize(new SubGroupSizeMetaData(getNamedNode(pNode, "sub_group_size"), true)), m_pNode(pNode) {}

FunctionInfoMetaData::FunctionInfoMetaData()
    : m_Type("function_type"), m_SubGroupSize(new SubGroupSizeMetaDataHandle::ObjectType("sub_group_size")),
      m_pNode(nullptr) {}

FunctionInfoMetaData::FunctionInfoMetaData(const char *name)
    : _Mybase(name), m_Type("function_type"),
      m_SubGroupSize(new SubGroupSizeMetaDataHandle::ObjectType("sub_group_size")), m_pNode(nullptr) {}

bool FunctionInfoMetaData::hasValue() const {
  return m_Type.hasValue() || m_SubGroupSize->hasValue() || nullptr != m_pNode || dirty();
}

bool FunctionInfoMetaData::dirty() const { return m_Type.dirty() || m_SubGroupSize.dirty(); }

void FunctionInfoMetaData::discardChanges() {
  m_Type.discardChanges();
  m_SubGroupSize.discardChanges();
}

llvm::Metadata *FunctionInfoMetaData::generateNode(llvm::LLVMContext &context) const {
  llvm::SmallVector<llvm::Metadata *, 5> args;

  llvm::Metadata *pIDNode = IMetaDataObject::generateNode(context);
  if (nullptr != pIDNode) {
    args.push_back(pIDNode);
  }

  args.push_back(m_Type.generateNode(context));
  if (m_SubGroupSize->hasValue()) {
    args.push_back(m_SubGroupSize.generateNode(context));
  }
  return llvm::MDNode::get(context, args);
}

void FunctionInfoMetaData::save(llvm::LLVMContext &context, llvm::MDNode *pNode) const {
  IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

  // we assume that underlying metadata node has not changed under our foot
  if (pNode == m_pNode && !dirty()) {
    return;
  }

  m_Type.save(context, getNamedNode(pNode, "function_type"));
  m_SubGroupSize.save(context, getNamedNode(pNode, "sub_group_size"));
}
} // namespace IGC::IGCMD
