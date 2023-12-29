/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "MetaDataApiUtils.h"
#include "MDFrameWork.h"

// Following classes read/write its own specific structure from/to LLVM metadata.

namespace IGC::IGCMD
{
    class ArgInfoMetaData;
    using ArgInfoMetaDataHandle = MetaObjectHandle<ArgInfoMetaData>;

    class SubGroupSizeMetaData;
    using SubGroupSizeMetaDataHandle = MetaObjectHandle<SubGroupSizeMetaData>;

    class VectorTypeHintMetaData;
    using VectorTypeHintMetaDataHandle = MetaObjectHandle<VectorTypeHintMetaData>;

    class ThreadGroupSizeMetaData;
    using ThreadGroupSizeMetaDataHandle = MetaObjectHandle<ThreadGroupSizeMetaData>;

    class FunctionInfoMetaData;
    using FunctionInfoMetaDataHandle = MetaObjectHandle<FunctionInfoMetaData>;

    class ArgInfoMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        ArgInfoMetaData(const llvm::MDNode* pNode, bool hasId);
        ArgInfoMetaData();
        ArgInfoMetaData(const char* name);

        // Returns true if any of the ArgInfoMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the ArgInfoMetaData instance
        void discardChanges() override;

        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

        // Saves the structure changes to the given MDNode
        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        //
        // Data members
        //
        using ArgIdType = MetaDataValue<int32_t>;
        using ExplicitArgNumType = NamedMetaDataValue<int32_t>;
        using StructArgOffsetType = NamedMetaDataValue<int32_t>;
        using ImgAccessFloatCoordsType = NamedMetaDataValue<bool>;
        using ImgAccessIntCoordsType = NamedMetaDataValue<bool>;

        // ArgId
        ArgIdType::value_type getArgId() const
        {
            return m_ArgId.get();
        }
        void setArgId(const ArgIdType::value_type& val)
        {
            m_ArgId.set(val);
        }
        bool isArgIdHasValue() const
        {
            return m_ArgId.hasValue();
        }

        // ExplicitArgNum
        ExplicitArgNumType::value_type getExplicitArgNum() const
        {
            return m_ExplicitArgNum.get();
        }
        void setExplicitArgNum(const ExplicitArgNumType::value_type& val)
        {
            m_ExplicitArgNum.set(val);
        }
        bool isExplicitArgNumHasValue() const
        {
            return m_ExplicitArgNum.hasValue();
        }

        // StructArgOffset
        StructArgOffsetType::value_type getStructArgOffset() const
        {
            return m_StructArgOffset.get();
        }
        void setStructArgOffset(const StructArgOffsetType::value_type& val)
        {
            m_StructArgOffset.set(val);
        }
        bool isStructArgOffsetHasValue() const
        {
            return m_StructArgOffset.hasValue();
        }

        // ImgAccessFloatCoords
        ImgAccessFloatCoordsType::value_type getImgAccessFloatCoords() const
        {
            return m_ImgAccessFloatCoords.get();
        }
        void setImgAccessFloatCoords(const ImgAccessFloatCoordsType::value_type& val)
        {
            m_ImgAccessFloatCoords.set(val);
        }
        bool isImgAccessFloatCoordsHasValue() const
        {
            return m_ImgAccessFloatCoords.hasValue();
        }

        // ImgAccessIntCoords
        ImgAccessIntCoordsType::value_type getImgAccessIntCoords() const
        {
            return m_ImgAccessIntCoords.get();
        }
        void setImgAccessIntCoords(const ImgAccessIntCoordsType::value_type& val)
        {
            m_ImgAccessIntCoords.set(val);
        }
        bool isImgAccessIntCoordsHasValue() const
        {
            return m_ImgAccessIntCoords.hasValue();
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        ArgIdType m_ArgId;
        ExplicitArgNumType m_ExplicitArgNum;
        StructArgOffsetType m_StructArgOffset;
        ImgAccessFloatCoordsType m_ImgAccessFloatCoords;
        ImgAccessIntCoordsType m_ImgAccessIntCoords;
    };

    class ArgDependencyInfoMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        ArgDependencyInfoMetaData(const llvm::MDNode* pNode, bool hasId);
        ArgDependencyInfoMetaData();
        ArgDependencyInfoMetaData(const char* name);

        // Returns true if any of the ArgInfoMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the ArgInfoMetaData instance
        void discardChanges() override;

        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

        // Saves the structure changes to the given MDNode
        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        //
        // Data members
        //
        using ArgType = MetaDataValue<std::string>;
        using ArgDependencyType = MetaDataValue<int32_t>;

        // Arg
        ArgType::value_type getArg() const
        {
            return m_Arg.get();
        }
        void setArg(const ArgType::value_type& val)
        {
            m_Arg.set(val);
        }
        bool isArgHasValue() const
        {
            return m_Arg.hasValue();
        }

        // ArgDependency
        ArgDependencyType::value_type getArgDependency() const
        {
            return m_ArgDependency.get();
        }
        void setArgDependency(const ArgDependencyType::value_type& val)
        {
            m_ArgDependency.set(val);
        }
        bool isArgDependencyHasValue() const
        {
            return m_ArgDependency.hasValue();
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        ArgType m_Arg;
        ArgDependencyType m_ArgDependency;
    };

    class SubGroupSizeMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        SubGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId);
        SubGroupSizeMetaData();
        SubGroupSizeMetaData(const char* name);

        // Returns true if any of the ArgInfoMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the ArgInfoMetaData instance
        void discardChanges() override;

        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

        // Saves the structure changes to the given MDNode
        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        //
        // Data members
        //
        using SIMDSizeType = MetaDataValue<int32_t>;

        // SIMDSize
        SIMDSizeType::value_type getSIMDSize() const
        {
            return m_SIMDSize.get();
        }
        void setSIMDSize(const SIMDSizeType::value_type& val)
        {
            m_SIMDSize.set(val);
        }
        bool isSIMDSizeHasValue() const
        {
            return m_SIMDSize.hasValue();
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        SIMDSizeType m_SIMDSize;
    };

    class VectorTypeHintMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        VectorTypeHintMetaData(const llvm::MDNode* pNode, bool hasId);
        VectorTypeHintMetaData();
        VectorTypeHintMetaData(const char* name);

        // Returns true if any of the ArgInfoMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the ArgInfoMetaData instance
        void discardChanges() override;

        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

        // Saves the structure changes to the given MDNode
        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        //
        // Data members
        //
        using VecTypeType = MetaDataValue<llvm::UndefValue>;
        using SignType = MetaDataValue<bool>;

        // VecType
        VecTypeType::value_type getVecType() const
        {
            return m_VecType.get();
        }
        void setVecType(const VecTypeType::value_type& val)
        {
            m_VecType.set(val);
        }
        bool isVecTypeHasValue() const
        {
            return m_VecType.hasValue();
        }

        // Sign
        SignType::value_type getSign() const
        {
            return m_Sign.get();
        }
        void setSign(const SignType::value_type& val)
        {
            m_Sign.set(val);
        }
        bool isSignHasValue() const
        {
            return m_Sign.hasValue();
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        VecTypeType m_VecType;
        SignType m_Sign;
    };

    class ThreadGroupSizeMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        ThreadGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId);
        ThreadGroupSizeMetaData();
        ThreadGroupSizeMetaData(const char* name);

        // Returns true if any of the ArgInfoMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the ArgInfoMetaData instance
        void discardChanges() override;

        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

        // Saves the structure changes to the given MDNode
        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        //
        // Data members
        //
        using DimType = MetaDataValue<int32_t>;

        // XDim
        DimType::value_type getXDim() const
        {
            return m_XDim.get();
        }
        void setXDim(const DimType::value_type& val)
        {
            m_XDim.set(val);
        }
        bool isXDimHasValue() const
        {
            return m_XDim.hasValue();
        }

        // YDim
        DimType::value_type getYDim() const
        {
            return m_YDim.get();
        }
        void setYDim(const DimType::value_type& val)
        {
            m_YDim.set(val);
        }
        bool isYDimHasValue() const
        {
            return m_YDim.hasValue();
        }

        // ZDim
        DimType::value_type getZDim() const
        {
            return m_ZDim.get();
        }
        void setZDim(const DimType::value_type& val)
        {
            m_ZDim.set(val);
        }
        bool isZDimHasValue() const
        {
            return m_ZDim.hasValue();
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        DimType m_XDim;
        DimType m_YDim;
        DimType m_ZDim;
    };

    class FunctionInfoMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        FunctionInfoMetaData(const llvm::MDNode* pNode, bool hasId);
        FunctionInfoMetaData();
        FunctionInfoMetaData(const char* name);

        // Returns true if any of the ArgInfoMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the ArgInfoMetaData instance
        void discardChanges() override;

        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

        // Saves the structure changes to the given MDNode
        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        //
        // Data members
        //
        using TypeType = NamedMetaDataValue<int32_t>;
        using ArgInfoListList = MetaDataList<ArgInfoMetaDataHandle>;
        using ImplicitArgInfoListList = MetaDataList<ArgInfoMetaDataHandle>;
        using PrivateMemoryPerWIType = NamedMetaDataValue<int32_t>;
        using NeedBindlessHandleType = NamedMetaDataValue<int32_t>;

        // Type
        TypeType::value_type getType() const
        {
            return m_Type.get();
        }
        void setType(const TypeType::value_type& val)
        {
            m_Type.set(val);
        }
        bool isTypeHasValue() const
        {
            return m_Type.hasValue();
        }

        // ArgInfoList
        ArgInfoListList::iterator begin_ArgInfoList()
        {
            return m_ArgInfoList.begin();
        }
        ArgInfoListList::iterator end_ArgInfoList()
        {
            return m_ArgInfoList.end();
        }
        ArgInfoListList::const_iterator begin_ArgInfoList() const
        {
            return m_ArgInfoList.begin();
        }
        ArgInfoListList::const_iterator end_ArgInfoList() const
        {
            return m_ArgInfoList.end();
        }
        size_t size_ArgInfoList()  const
        {
            return m_ArgInfoList.size();
        }
        bool empty_ArgInfoList()  const
        {
            return m_ArgInfoList.empty();
        }
        bool isArgInfoListHasValue() const
        {
            return m_ArgInfoList.hasValue();
        }
        ArgInfoListList::item_type getArgInfoListItem(size_t index) const
        {
            return m_ArgInfoList.getItem(index);
        }
        void clearArgInfoList()
        {
            m_ArgInfoList.clear();
        }
        void setArgInfoListItem(size_t index, const ArgInfoListList::item_type& item)
        {
            return m_ArgInfoList.setItem(index, item);
        }
        void addArgInfoListItem(const ArgInfoListList::item_type& val)
        {
            m_ArgInfoList.push_back(val);
        }
        ArgInfoListList::iterator eraseArgInfoListItem(ArgInfoListList::iterator i)
        {
            return m_ArgInfoList.erase(i);
        }

        // ImplicitArgInfoList
        ImplicitArgInfoListList::iterator begin_ImplicitArgInfoList()
        {
            return m_ImplicitArgInfoList.begin();
        }
        ImplicitArgInfoListList::iterator end_ImplicitArgInfoList()
        {
            return m_ImplicitArgInfoList.end();
        }
        ImplicitArgInfoListList::const_iterator begin_ImplicitArgInfoList() const
        {
            return m_ImplicitArgInfoList.begin();
        }
        ImplicitArgInfoListList::const_iterator end_ImplicitArgInfoList() const
        {
            return m_ImplicitArgInfoList.end();
        }
        size_t size_ImplicitArgInfoList()  const
        {
            return m_ImplicitArgInfoList.size();
        }
        bool empty_ImplicitArgInfoList()  const
        {
            return m_ImplicitArgInfoList.empty();
        }
        bool isImplicitArgInfoListHasValue() const
        {
            return m_ImplicitArgInfoList.hasValue();
        }
        ImplicitArgInfoListList::item_type getImplicitArgInfoListItem(size_t index) const
        {
            return m_ImplicitArgInfoList.getItem(index);
        }
        void clearImplicitArgInfoList()
        {
            m_ImplicitArgInfoList.clear();
        }
        void setImplicitArgInfoListItem(size_t index, const ImplicitArgInfoListList::item_type& item)
        {
            return m_ImplicitArgInfoList.setItem(index, item);
        }
        void addImplicitArgInfoListItem(const ImplicitArgInfoListList::item_type& val)
        {
            m_ImplicitArgInfoList.push_back(val);
        }
        ImplicitArgInfoListList::iterator eraseImplicitArgInfoListItem(ImplicitArgInfoListList::iterator i)
        {
            return m_ImplicitArgInfoList.erase(i);
        }

        // ThreadGroupSize
        ThreadGroupSizeMetaDataHandle getThreadGroupSize()
        {
            return m_ThreadGroupSize;
        }

        // ThreadGroupSizeHint
        ThreadGroupSizeMetaDataHandle getThreadGroupSizeHint()
        {
            return m_ThreadGroupSizeHint;
        }

        // SubGroupSize
        SubGroupSizeMetaDataHandle getSubGroupSize()
        {
            return m_SubGroupSize;
        }

        // OpenCLVectorTypeHint
        VectorTypeHintMetaDataHandle getOpenCLVectorTypeHint()
        {
            return m_OpenCLVectorTypeHint;
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        TypeType m_Type;
        ArgInfoListList m_ArgInfoList;
        ImplicitArgInfoListList m_ImplicitArgInfoList;
        ThreadGroupSizeMetaDataHandle m_ThreadGroupSize;
        ThreadGroupSizeMetaDataHandle m_ThreadGroupSizeHint;
        SubGroupSizeMetaDataHandle m_SubGroupSize;
        VectorTypeHintMetaDataHandle m_OpenCLVectorTypeHint;
    };

    class MetaDataUtils
    {
    public:
        // data member types
        using FunctionsInfoMap = NamedMetaDataMap<llvm::Function, FunctionInfoMetaDataHandle>;

        // If using this constructor, setting the llvm module by the setModule
        // function is needed for correct operation.
        MetaDataUtils() = default;

        MetaDataUtils(llvm::Module* pModule) :
            m_FunctionsInfo(pModule->getOrInsertNamedMetadata("igc.functions")),
            m_pModule(pModule)
        {}

        void setModule(llvm::Module* pModule) {
            m_FunctionsInfo = pModule->getOrInsertNamedMetadata("igc.functions");
            m_pModule = pModule;
        }

        ~MetaDataUtils() = default;

        // FunctionsInfo
        void clearFunctionsInfo()
        {
            m_FunctionsInfo.clear();
        }

        void deleteFunctionsInfo()
        {
            llvm::NamedMDNode* FunctionsInfoNode = m_pModule->getNamedMetadata("igc.functions");
            if (FunctionsInfoNode)
            {
                m_nodesToDelete.push_back(FunctionsInfoNode);
            }
        }

        FunctionsInfoMap::iterator begin_FunctionsInfo()
        {
            return m_FunctionsInfo.begin();
        }

        FunctionsInfoMap::iterator end_FunctionsInfo()
        {
            return m_FunctionsInfo.end();
        }

        FunctionsInfoMap::const_iterator begin_FunctionsInfo() const
        {
            return m_FunctionsInfo.begin();
        }

        FunctionsInfoMap::const_iterator end_FunctionsInfo() const
        {
            return m_FunctionsInfo.end();
        }

        size_t size_FunctionsInfo() const
        {
            return m_FunctionsInfo.size();
        }

        bool empty_FunctionsInfo() const
        {
            return m_FunctionsInfo.empty();
        }

        bool isFunctionsInfoHasValue() const
        {
            return m_FunctionsInfo.hasValue();
        }

        FunctionsInfoMap::item_type getFunctionsInfoItem(const FunctionsInfoMap::key_type& index) const
        {
            return m_FunctionsInfo.getItem(index);
        }

        FunctionsInfoMap::item_type getOrInsertFunctionsInfoItem(const FunctionsInfoMap::key_type& index)
        {
            return m_FunctionsInfo.getOrInsertItem(index);
        }

        void setFunctionsInfoItem(const FunctionsInfoMap::key_type& index, const FunctionsInfoMap::item_type& item)
        {
            return m_FunctionsInfo.setItem(index, item);
        }

        FunctionsInfoMap::iterator findFunctionsInfoItem(const FunctionsInfoMap::key_type& key)
        {
            return m_FunctionsInfo.find(key);
        }
        FunctionsInfoMap::const_iterator findFunctionsInfoItem(const FunctionsInfoMap::key_type& key) const
        {
            return m_FunctionsInfo.find(key);
        }

        void eraseFunctionsInfoItem(FunctionsInfoMap::iterator it)
        {
            m_FunctionsInfo.erase(it);
        }

        void save(llvm::LLVMContext& context)
        {
            if (m_FunctionsInfo.dirty())
            {
                llvm::NamedMDNode* pNode = m_pModule->getOrInsertNamedMetadata("igc.functions");
                m_FunctionsInfo.save(context, pNode);
            }

            for (auto node : m_nodesToDelete)
            {
                m_pModule->eraseNamedMetadata(node);
            }
            m_nodesToDelete.clear();

            discardChanges();
        }

        void discardChanges()
        {
            m_FunctionsInfo.discardChanges();
            m_nodesToDelete.clear();
        }

        void deleteMetadata()
        {
            llvm::NamedMDNode* FunctionsInfoNode = m_pModule->getNamedMetadata("igc.functions");
            if (FunctionsInfoNode)
            {
                m_nodesToDelete.push_back(FunctionsInfoNode);
            }
        }

    private:
        // data members
        NamedMetaDataMap<llvm::Function, FunctionInfoMetaDataHandle> m_FunctionsInfo;
        llvm::Module* m_pModule = nullptr;
        std::vector<llvm::NamedMDNode*> m_nodesToDelete;
    };
}
