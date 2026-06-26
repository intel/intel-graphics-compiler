/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "MetaDataApiUtils.h"
#include "MDFrameWork.h"

// Following classes read/write its own specific structure from/to LLVM metadata.

namespace IGC::IGCMD {

// NOTE: VectorTypeHintMetaData is retained solely for the SPIR-V input metadata
// layer (SpirMetaDataApi.h aliases it as SPIRMD::VectorTypeHintMetaData). The IGC
// output side now carries the OpenCL vec_type_hint as ModuleMetaData::FuncMD[F].vecTypeHint.
class VectorTypeHintMetaData;
using VectorTypeHintMetaDataHandle = MetaObjectHandle<VectorTypeHintMetaData>;

class FunctionInfoMetaData;
using FunctionInfoMetaDataHandle = MetaObjectHandle<FunctionInfoMetaData>;

class ArgDependencyInfoMetaData : public IMetaDataObject {
public:
  using _Mybase = IMetaDataObject;

  ArgDependencyInfoMetaData(const llvm::MDNode *pNode, bool hasId);
  ArgDependencyInfoMetaData();
  ArgDependencyInfoMetaData(const char *name);

  // Returns true if any of the ArgInfoMetaData`s members has changed
  bool dirty() const override;

  // Returns true if the structure was loaded from the metadata or was changed
  bool hasValue() const;

  // Discards the changes done to the ArgInfoMetaData instance
  void discardChanges() override;

  // Generates the new MDNode hierarchy for the given structure
  llvm::Metadata *generateNode(llvm::LLVMContext &context) const;

  // Saves the structure changes to the given MDNode
  void save(llvm::LLVMContext &context, llvm::MDNode *pNode) const;

  //
  // Data members
  //
  using ArgType = MetaDataValue<std::string>;
  using ArgDependencyType = MetaDataValue<int32_t>;

  // Arg
  ArgType::value_type getArg() const { return m_Arg.get(); }
  void setArg(const ArgType::value_type &val) { m_Arg.set(val); }
  bool isArgHasValue() const { return m_Arg.hasValue(); }

  // ArgDependency
  ArgDependencyType::value_type getArgDependency() const { return m_ArgDependency.get(); }
  void setArgDependency(const ArgDependencyType::value_type &val) { m_ArgDependency.set(val); }
  bool isArgDependencyHasValue() const { return m_ArgDependency.hasValue(); }

private:
  // parent node
  const llvm::MDNode *m_pNode;

  // data members
  ArgType m_Arg;
  ArgDependencyType m_ArgDependency;
};

// Retained for the SPIR-V input metadata layer only (see SpirMetaDataApi.h).
// The IGC output side reads/writes ModuleMetaData::FuncMD[F].vecTypeHint instead.
class VectorTypeHintMetaData : public IMetaDataObject {
public:
  using _Mybase = IMetaDataObject;

  VectorTypeHintMetaData(const llvm::MDNode *pNode, bool hasId);
  VectorTypeHintMetaData();
  VectorTypeHintMetaData(const char *name);

  // Returns true if any of the ArgInfoMetaData`s members has changed
  bool dirty() const override;

  // Returns true if the structure was loaded from the metadata or was changed
  bool hasValue() const;

  // Discards the changes done to the ArgInfoMetaData instance
  void discardChanges() override;

  // Generates the new MDNode hierarchy for the given structure
  llvm::Metadata *generateNode(llvm::LLVMContext &context) const;

  // Saves the structure changes to the given MDNode
  void save(llvm::LLVMContext &context, llvm::MDNode *pNode) const;

  //
  // Data members
  //
  using VecTypeType = MetaDataValue<llvm::UndefValue>;
  using SignType = MetaDataValue<bool>;

  // VecType
  VecTypeType::value_type getVecType() const { return m_VecType.get(); }
  void setVecType(const VecTypeType::value_type &val) { m_VecType.set(val); }
  bool isVecTypeHasValue() const { return m_VecType.hasValue(); }

  // Sign
  SignType::value_type getSign() const { return m_Sign.get(); }
  void setSign(const SignType::value_type &val) { m_Sign.set(val); }
  bool isSignHasValue() const { return m_Sign.hasValue(); }

private:
  // parent node
  const llvm::MDNode *m_pNode;

  // data members
  VecTypeType m_VecType;
  SignType m_Sign;
};

class FunctionInfoMetaData : public IMetaDataObject {
public:
  using _Mybase = IMetaDataObject;

  FunctionInfoMetaData(const llvm::MDNode *pNode, bool hasId);
  FunctionInfoMetaData();
  FunctionInfoMetaData(const char *name);

  // Returns true if any of the ArgInfoMetaData`s members has changed
  bool dirty() const override;

  // Returns true if the structure was loaded from the metadata or was changed
  bool hasValue() const;

  // Discards the changes done to the ArgInfoMetaData instance
  void discardChanges() override;

  // Generates the new MDNode hierarchy for the given structure
  llvm::Metadata *generateNode(llvm::LLVMContext &context) const;

  // Saves the structure changes to the given MDNode
  void save(llvm::LLVMContext &context, llvm::MDNode *pNode) const;

  //
  // Data members
  //
  using PrivateMemoryPerWIType = NamedMetaDataValue<int32_t>;
  using NeedBindlessHandleType = NamedMetaDataValue<int32_t>;

private:
  // parent node
  const llvm::MDNode *m_pNode;
};

class MetaDataUtils {
public:
  // data member types
  using FunctionsInfoMap = NamedMetaDataMap<llvm::Function, FunctionInfoMetaDataHandle>;

  // If using this constructor, setting the llvm module by the setModule
  // function is needed for correct operation.
  MetaDataUtils() = default;

  MetaDataUtils(llvm::Module *pModule)
      : m_FunctionsInfo(pModule->getOrInsertNamedMetadata("igc.functions")), m_pModule(pModule) {}

  void setModule(llvm::Module *pModule) {
    m_FunctionsInfo = pModule->getOrInsertNamedMetadata("igc.functions");
    m_pModule = pModule;
  }

  ~MetaDataUtils() = default;

  // FunctionsInfo
  void clearFunctionsInfo() { m_FunctionsInfo.clear(); }

  void deleteFunctionsInfo() {
    llvm::NamedMDNode *FunctionsInfoNode = m_pModule->getNamedMetadata("igc.functions");
    if (FunctionsInfoNode) {
      m_nodesToDelete.push_back(FunctionsInfoNode);
    }
  }

  FunctionsInfoMap::iterator begin_FunctionsInfo() { return m_FunctionsInfo.begin(); }

  FunctionsInfoMap::iterator end_FunctionsInfo() { return m_FunctionsInfo.end(); }

  FunctionsInfoMap::const_iterator begin_FunctionsInfo() const { return m_FunctionsInfo.begin(); }

  FunctionsInfoMap::const_iterator end_FunctionsInfo() const { return m_FunctionsInfo.end(); }

  size_t size_FunctionsInfo() const { return m_FunctionsInfo.size(); }

  bool empty_FunctionsInfo() const { return m_FunctionsInfo.empty(); }

  bool isFunctionsInfoHasValue() const { return m_FunctionsInfo.hasValue(); }

  FunctionsInfoMap::item_type getFunctionsInfoItem(const FunctionsInfoMap::key_type &index) const {
    return m_FunctionsInfo.getItem(index);
  }

  FunctionsInfoMap::item_type getOrInsertFunctionsInfoItem(const FunctionsInfoMap::key_type &index) {
    return m_FunctionsInfo.getOrInsertItem(index);
  }

  void setFunctionsInfoItem(const FunctionsInfoMap::key_type &index, const FunctionsInfoMap::item_type &item) {
    return m_FunctionsInfo.setItem(index, item);
  }

  FunctionsInfoMap::iterator findFunctionsInfoItem(const FunctionsInfoMap::key_type &key) {
    return m_FunctionsInfo.find(key);
  }
  FunctionsInfoMap::const_iterator findFunctionsInfoItem(const FunctionsInfoMap::key_type &key) const {
    return m_FunctionsInfo.find(key);
  }

  void eraseFunctionsInfoItem(FunctionsInfoMap::iterator it) { m_FunctionsInfo.erase(it); }

  void save(llvm::LLVMContext &context) {
    if (m_FunctionsInfo.dirty()) {
      llvm::NamedMDNode *pNode = m_pModule->getOrInsertNamedMetadata("igc.functions");
      m_FunctionsInfo.save(context, pNode);
    }

    for (auto node : m_nodesToDelete) {
      m_pModule->eraseNamedMetadata(node);
    }
    m_nodesToDelete.clear();

    discardChanges();
  }

  void discardChanges() {
    m_FunctionsInfo.discardChanges();
    m_nodesToDelete.clear();
  }

  void deleteMetadata() {
    llvm::NamedMDNode *FunctionsInfoNode = m_pModule->getNamedMetadata("igc.functions");
    if (FunctionsInfoNode) {
      m_nodesToDelete.push_back(FunctionsInfoNode);
    }
  }

private:
  // data members
  NamedMetaDataMap<llvm::Function, FunctionInfoMetaDataHandle> m_FunctionsInfo;
  llvm::Module *m_pModule = nullptr;
  std::vector<llvm::NamedMDNode *> m_nodesToDelete;
};
} // namespace IGC::IGCMD
