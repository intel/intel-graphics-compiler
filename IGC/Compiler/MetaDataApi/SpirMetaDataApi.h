/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "MetaDataApiUtils.h"
#include "MetaDataApi.h"

// Following classes read/write its own specific structure from/to LLVM metadata.
namespace IGC::SPIRMD
{
    using VectorTypeHintMetaData = IGC::IGCMD::VectorTypeHintMetaData;
    using VectorTypeHintMetaDataHandle = MetaObjectHandle<VectorTypeHintMetaData>;

    class VersionMetaData;
    using VersionMetaDataHandle = MetaObjectHandle<VersionMetaData>;

    class WorkGroupDimensionsMetaData;
    using WorkGroupDimensionsMetaDataHandle = MetaObjectHandle<WorkGroupDimensionsMetaData>;

    class SubGroupDimensionsMetaData;
    using SubGroupDimensionsMetaDataHandle = MetaObjectHandle<SubGroupDimensionsMetaData>;

    class WorkgroupWalkOrderMetaData;
    using WorkgroupWalkOrderMetaDataHandle = MetaObjectHandle<WorkgroupWalkOrderMetaData>;

    class KernelMetaData;
    using KernelMetaDataHandle = MetaObjectHandle<KernelMetaData>;

    using InnerUsedKhrExtensionsMetaDataList = MetaDataList<std::string>;
    using InnerUsedKhrExtensionsMetaDataListHandle = MetaObjectHandle<InnerUsedKhrExtensionsMetaDataList>;

    using InnerUsedOptionalCoreFeaturesMetaDataList = MetaDataList<std::string>;
    using InnerUsedOptionalCoreFeaturesMetaDataListHandle = MetaObjectHandle<InnerUsedOptionalCoreFeaturesMetaDataList>;

    using InnerCompilerOptionsMetaDataList = MetaDataList<std::string>;
    using InnerCompilerOptionsMetaDataListHandle = MetaObjectHandle<InnerCompilerOptionsMetaDataList>;

    using InnerCompilerExternalOptionsMetaDataList = MetaDataList<std::string>;
    using InnerCompilerExternalOptionsMetaDataListHandle = MetaObjectHandle<InnerCompilerExternalOptionsMetaDataList>;

    using SPIRVExtensionsMetaDataList = MetaDataList<std::string>;
    using SPIRVExtensionsMetaDataListHandle = MetaObjectHandle<SPIRVExtensionsMetaDataList>;

    class VersionMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        VersionMetaData(const llvm::MDNode* pNode, bool hasId);
        VersionMetaData();
        VersionMetaData(const char* name);

        // Returns true if any of the VersionMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the VersionMetaData instance
        void discardChanges() override;

        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

        // Saves the structure changes to the given MDNode
        void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

        //
        // Data members
        //
        using MajorType = MetaDataValue<int32_t>;
        using MinorType = MetaDataValue<int32_t>;

        // Major
        MajorType::value_type getMajor() const
        {
            return m_Major.get();
        }
        bool isMajorHasValue() const
        {
            return m_Major.hasValue();
        }

        // Minor
        MinorType::value_type getMinor() const
        {
            return m_Minor.get();
        }
        bool isMinorHasValue() const
        {
            return m_Minor.hasValue();
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        MajorType m_Major;
        MinorType m_Minor;
    };

    class WorkGroupDimensionsMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        WorkGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId);
        WorkGroupDimensionsMetaData();
        WorkGroupDimensionsMetaData(const char* name);

        // Returns true if any of the VersionMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the VersionMetaData instance
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

    class SubGroupDimensionsMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        SubGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId);
        SubGroupDimensionsMetaData();
        SubGroupDimensionsMetaData(const char* name);

        // Returns true if any of the VersionMetaData`s members has changed
        bool dirty() const override;

        // Returns true if the structure was loaded from the metadata or was changed
        bool hasValue() const;

        // Discards the changes done to the VersionMetaData instance
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

    class WorkgroupWalkOrderMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        WorkgroupWalkOrderMetaData(const llvm::MDNode* pNode, bool hasId);
        WorkgroupWalkOrderMetaData();
        WorkgroupWalkOrderMetaData(const char* name);

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

        // workgroup walk order
        DimType::value_type getDim0() const
        {
            return m_Dim0.get();
        }
        DimType::value_type getDim1() const
        {
            return m_Dim1.get();
        }
        DimType::value_type getDim2() const
        {
            return m_Dim2.get();
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        DimType m_Dim0;
        DimType m_Dim1;
        DimType m_Dim2;
    };

    class KernelMetaData : public IMetaDataObject
    {
    public:
        using _Mybase = IMetaDataObject;

        KernelMetaData(const llvm::MDNode* pNode, bool hasId);
        KernelMetaData();
        KernelMetaData(const char* name);

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
        using FunctionType = MetaDataValue<llvm::Function>;
        using WorkGroupSizeHintType = WorkGroupDimensionsMetaDataHandle;
        using RequiredWorkGroupSizeType = WorkGroupDimensionsMetaDataHandle;
        using RequiredSubGroupSizeType = SubGroupDimensionsMetaDataHandle;
        using WorkgroupWalkOrderType = WorkgroupWalkOrderMetaDataHandle;
        using VectorTypeHintType = VectorTypeHintMetaDataHandle;
        using ArgAddressSpacesList = MetaDataList<int32_t>;
        using ArgAccessQualifiersList = MetaDataList<std::string>;
        using ArgTypesList = MetaDataList<std::string>;
        using ArgBaseTypesList = MetaDataList<std::string>;
        using ArgTypeQualifiersList = MetaDataList<std::string>;
        using ArgNamesList = MetaDataList<std::string>;

        // Function
        FunctionType::value_type getFunction() const
        {
            return m_Function.get();
        }
        bool isFunctionHasValue() const
        {
            return m_Function.hasValue();
        }

        // WorkGroupSizeHint
        const WorkGroupSizeHintType getWorkGroupSizeHint()
        {
            return m_WorkGroupSizeHint;
        }

        // RequiredWorkGroupSize
        const RequiredWorkGroupSizeType getRequiredWorkGroupSize()
        {
            return m_RequiredWorkGroupSize;
        }

        // RequiredSubGroupSize
        const RequiredSubGroupSizeType getRequiredSubGroupSize()
        {
            return m_RequiredSubGroupSize;
        }

        // WorkgroupWalkOrder
        const WorkgroupWalkOrderType getWorkgroupWalkOrder()
        {
            return m_WorkgroupWalkOrder;
        }

        // VectorTypeHint
        const VectorTypeHintType getVectorTypeHint()
        {
            return m_VectorTypeHint;
        }

        // ArgAddressSpaces
        ArgAddressSpacesList::const_iterator begin_ArgAddressSpaces() const
        {
            return m_ArgAddressSpaces.begin();
        }
        ArgAddressSpacesList::const_iterator end_ArgAddressSpaces() const
        {
            return m_ArgAddressSpaces.end();
        }
        size_t size_ArgAddressSpaces()  const
        {
            return m_ArgAddressSpaces.size();
        }
        bool empty_ArgAddressSpaces()  const
        {
            return m_ArgAddressSpaces.empty();
        }
        bool isArgAddressSpacesHasValue() const
        {
            return m_ArgAddressSpaces.hasValue();
        }
        const ArgAddressSpacesList::item_type getArgAddressSpacesItem(size_t index) const
        {
            return m_ArgAddressSpaces.getItem(index);
        }

        // ArgAccessQualifiers
        ArgAccessQualifiersList::const_iterator begin_ArgAccessQualifiers() const
        {
            return m_ArgAccessQualifiers.begin();
        }
        ArgAccessQualifiersList::const_iterator end_ArgAccessQualifiers() const
        {
            return m_ArgAccessQualifiers.end();
        }
        size_t size_ArgAccessQualifiers()  const
        {
            return m_ArgAccessQualifiers.size();
        }
        bool empty_ArgAccessQualifiers()  const
        {
            return m_ArgAccessQualifiers.empty();
        }
        bool isArgAccessQualifiersHasValue() const
        {
            return m_ArgAccessQualifiers.hasValue();
        }
        const ArgAccessQualifiersList::item_type getArgAccessQualifiersItem(size_t index) const
        {
            return m_ArgAccessQualifiers.getItem(index);
        }

        // ArgTypes
        ArgTypesList::const_iterator begin_ArgTypes() const
        {
            return m_ArgTypes.begin();
        }
        ArgTypesList::const_iterator end_ArgTypes() const
        {
            return m_ArgTypes.end();
        }
        size_t size_ArgTypes()  const
        {
            return m_ArgTypes.size();
        }
        bool empty_ArgTypes()  const
        {
            return m_ArgTypes.empty();
        }
        bool isArgTypesHasValue() const
        {
            return m_ArgTypes.hasValue();
        }
        const ArgTypesList::item_type getArgTypesItem(size_t index) const
        {
            return m_ArgTypes.getItem(index);
        }

        // ArgBaseTypes
        ArgBaseTypesList::const_iterator begin_ArgBaseTypes() const
        {
            return m_ArgBaseTypes.begin();
        }
        ArgBaseTypesList::const_iterator end_ArgBaseTypes() const
        {
            return m_ArgBaseTypes.end();
        }
        size_t size_ArgBaseTypes()  const
        {
            return m_ArgBaseTypes.size();
        }
        bool empty_ArgBaseTypes()  const
        {
            return m_ArgBaseTypes.empty();
        }
        bool isArgBaseTypesHasValue() const
        {
            return m_ArgBaseTypes.hasValue();
        }
        const ArgBaseTypesList::item_type getArgBaseTypesItem(size_t index) const
        {
            return m_ArgBaseTypes.getItem(index);
        }

        // ArgTypeQualifiers
        ArgTypeQualifiersList::const_iterator begin_ArgTypeQualifiers() const
        {
            return m_ArgTypeQualifiers.begin();
        }
        ArgTypeQualifiersList::const_iterator end_ArgTypeQualifiers() const
        {
            return m_ArgTypeQualifiers.end();
        }
        size_t size_ArgTypeQualifiers()  const
        {
            return m_ArgTypeQualifiers.size();
        }
        bool empty_ArgTypeQualifiers()  const
        {
            return m_ArgTypeQualifiers.empty();
        }
        bool isArgTypeQualifiersHasValue() const
        {
            return m_ArgTypeQualifiers.hasValue();
        }
        const ArgTypeQualifiersList::item_type getArgTypeQualifiersItem(size_t index) const
        {
            return m_ArgTypeQualifiers.getItem(index);
        }

        // ArgNames
        ArgNamesList::const_iterator begin_ArgNames() const
        {
            return m_ArgNames.begin();
        }
        ArgNamesList::const_iterator end_ArgNames() const
        {
            return m_ArgNames.end();
        }
        size_t size_ArgNames()  const
        {
            return m_ArgNames.size();
        }
        bool empty_ArgNames()  const
        {
            return m_ArgNames.empty();
        }
        bool isArgNamesHasValue() const
        {
            return m_ArgNames.hasValue();
        }
        const ArgNamesList::item_type getArgNamesItem(size_t index) const
        {
            return m_ArgNames.getItem(index);
        }

    private:
        // parent node
        const llvm::MDNode* m_pNode;

        // data members
        FunctionType m_Function;
        WorkGroupSizeHintType m_WorkGroupSizeHint;
        RequiredWorkGroupSizeType m_RequiredWorkGroupSize;
        RequiredSubGroupSizeType m_RequiredSubGroupSize;
        WorkgroupWalkOrderType m_WorkgroupWalkOrder;
        VectorTypeHintType m_VectorTypeHint;
        ArgAddressSpacesList m_ArgAddressSpaces;
        ArgAccessQualifiersList m_ArgAccessQualifiers;
        ArgTypesList m_ArgTypes;
        ArgBaseTypesList m_ArgBaseTypes;
        ArgTypeQualifiersList m_ArgTypeQualifiers;
        ArgNamesList m_ArgNames;
    };

    class SpirMetaDataUtils
    {
    public:
        // data member types
        using KernelsList = NamedMDNodeList<KernelMetaDataHandle>;
        using CompilerOptionsList = NamedMDNodeList<InnerCompilerOptionsMetaDataListHandle>;
        using CompilerExternalOptionsList = NamedMDNodeList<InnerCompilerExternalOptionsMetaDataListHandle>;
        using FloatingPointContractionsList = NamedMDNodeList<int32_t>;
        using UsedOptionalCoreFeaturesList = NamedMDNodeList<InnerUsedOptionalCoreFeaturesMetaDataListHandle>;
        using UsedKhrExtensionsList = NamedMDNodeList<InnerUsedKhrExtensionsMetaDataListHandle>;
        using SpirVersionsList = NamedMDNodeList<VersionMetaDataHandle>;
        using OpenCLVersionsList = NamedMDNodeList<VersionMetaDataHandle>;
        using SPIRVExtensionsList = NamedMDNodeList<SPIRVExtensionsMetaDataListHandle>;

        // If using this constructor, setting the llvm module by the setModule
        // function is needed for correct operation.
        SpirMetaDataUtils() {}

        SpirMetaDataUtils(llvm::Module* pModule) :
            m_Kernels(pModule->getNamedMetadata("opencl.kernels")),
            m_CompilerOptions(pModule->getNamedMetadata("opencl.compiler.options")),
            m_CompilerExternalOptions(pModule->getNamedMetadata("opencl.compiler.ext.options")),
            m_FloatingPointContractions(pModule->getNamedMetadata("opencl.enable.FP_CONTRACT")),
            m_UsedOptionalCoreFeatures(pModule->getNamedMetadata("opencl.used.optional.core.features")),
            m_UsedKhrExtensions(pModule->getNamedMetadata("opencl.used.extensions")),
            m_SpirVersions(pModule->getNamedMetadata("opencl.spir.version")),
            m_OpenCLVersions(pModule->getNamedMetadata("opencl.ocl.version")),
            m_SPIRVExtensions(pModule->getNamedMetadata("igc.spirv.extensions")),
            m_pModule(pModule)
        {}

        void setModule(llvm::Module* pModule) {
            m_Kernels = pModule->getNamedMetadata("opencl.kernels");
            m_CompilerOptions = pModule->getNamedMetadata("opencl.compiler.options");
            m_CompilerExternalOptions = pModule->getNamedMetadata("opencl.compiler.ext.options");
            m_FloatingPointContractions = pModule->getNamedMetadata("opencl.enable.FP_CONTRACT");
            m_UsedOptionalCoreFeatures = pModule->getNamedMetadata("opencl.used.optional.core.features");
            m_UsedKhrExtensions = pModule->getNamedMetadata("opencl.used.extensions");
            m_SpirVersions = pModule->getNamedMetadata("opencl.spir.version");
            m_OpenCLVersions = pModule->getNamedMetadata("opencl.ocl.version");
            m_SPIRVExtensions = pModule->getNamedMetadata("igc.spirv.extensions");
            m_pModule = pModule;
        }

        ~SpirMetaDataUtils() {}

        // Kernels
        KernelsList::const_iterator begin_Kernels() const
        {
            return m_Kernels.begin();
        }

        KernelsList::const_iterator end_Kernels() const
        {
            return m_Kernels.end();
        }

        size_t size_Kernels()  const
        {
            return m_Kernels.size();
        }

        bool empty_Kernels()  const
        {
            return m_Kernels.empty();
        }

        bool isKernelsHasValue() const
        {
            return m_Kernels.hasValue();
        }

        const KernelsList::item_type getKernelsItem(size_t index) const
        {
            return m_Kernels.getItem(index);
        }

        // CompilerOptions
        CompilerOptionsList::const_iterator begin_CompilerOptions() const
        {
            return m_CompilerOptions.begin();
        }

        CompilerOptionsList::const_iterator end_CompilerOptions() const
        {
            return m_CompilerOptions.end();
        }

        size_t size_CompilerOptions()  const
        {
            return m_CompilerOptions.size();
        }

        bool empty_CompilerOptions()  const
        {
            return m_CompilerOptions.empty();
        }

        bool isCompilerOptionsHasValue() const
        {
            return m_CompilerOptions.hasValue();
        }

        const CompilerOptionsList::item_type getCompilerOptionsItem(size_t index) const
        {
            return m_CompilerOptions.getItem(index);
        }

        // CompilerExternalOptions
        CompilerExternalOptionsList::const_iterator begin_CompilerExternalOptions() const
        {
            return m_CompilerExternalOptions.begin();
        }

        CompilerExternalOptionsList::const_iterator end_CompilerExternalOptions() const
        {
            return m_CompilerExternalOptions.end();
        }

        size_t size_CompilerExternalOptions()  const
        {
            return m_CompilerExternalOptions.size();
        }

        bool empty_CompilerExternalOptions()  const
        {
            return m_CompilerExternalOptions.empty();
        }

        bool isCompilerExternalOptionsHasValue() const
        {
            return m_CompilerExternalOptions.hasValue();
        }

        const CompilerExternalOptionsList::item_type getCompilerExternalOptionsItem(size_t index) const
        {
            return m_CompilerExternalOptions.getItem(index);
        }

        // FloatingPointContractions
        FloatingPointContractionsList::const_iterator begin_FloatingPointContractions() const
        {
            return m_FloatingPointContractions.begin();
        }

        FloatingPointContractionsList::const_iterator end_FloatingPointContractions() const
        {
            return m_FloatingPointContractions.end();
        }

        size_t size_FloatingPointContractions()  const
        {
            return m_FloatingPointContractions.size();
        }

        bool empty_FloatingPointContractions()  const
        {
            return m_FloatingPointContractions.empty();
        }

        bool isFloatingPointContractionsHasValue() const
        {
            return m_FloatingPointContractions.hasValue();
        }

        const FloatingPointContractionsList::item_type getFloatingPointContractionsItem(size_t index) const
        {
            return m_FloatingPointContractions.getItem(index);
        }

        // UsedOptionalCoreFeatures
        UsedOptionalCoreFeaturesList::const_iterator begin_UsedOptionalCoreFeatures() const
        {
            return m_UsedOptionalCoreFeatures.begin();
        }

        UsedOptionalCoreFeaturesList::const_iterator end_UsedOptionalCoreFeatures() const
        {
            return m_UsedOptionalCoreFeatures.end();
        }

        size_t size_UsedOptionalCoreFeatures()  const
        {
            return m_UsedOptionalCoreFeatures.size();
        }

        bool empty_UsedOptionalCoreFeatures()  const
        {
            return m_UsedOptionalCoreFeatures.empty();
        }

        bool isUsedOptionalCoreFeaturesHasValue() const
        {
            return m_UsedOptionalCoreFeatures.hasValue();
        }

        const UsedOptionalCoreFeaturesList::item_type getUsedOptionalCoreFeaturesItem(size_t index) const
        {
            return m_UsedOptionalCoreFeatures.getItem(index);
        }

        // UsedKhrExtensions
        UsedKhrExtensionsList::const_iterator begin_UsedKhrExtensions() const
        {
            return m_UsedKhrExtensions.begin();
        }

        UsedKhrExtensionsList::const_iterator end_UsedKhrExtensions() const
        {
            return m_UsedKhrExtensions.end();
        }

        size_t size_UsedKhrExtensions()  const
        {
            return m_UsedKhrExtensions.size();
        }

        bool empty_UsedKhrExtensions()  const
        {
            return m_UsedKhrExtensions.empty();
        }

        bool isUsedKhrExtensionsHasValue() const
        {
            return m_UsedKhrExtensions.hasValue();
        }

        const UsedKhrExtensionsList::item_type getUsedKhrExtensionsItem(size_t index) const
        {
            return m_UsedKhrExtensions.getItem(index);
        }

        // SpirVersions
        SpirVersionsList::const_iterator begin_SpirVersions() const
        {
            return m_SpirVersions.begin();
        }

        SpirVersionsList::const_iterator end_SpirVersions() const
        {
            return m_SpirVersions.end();
        }

        size_t size_SpirVersions()  const
        {
            return m_SpirVersions.size();
        }

        bool empty_SpirVersions()  const
        {
            return m_SpirVersions.empty();
        }

        bool isSpirVersionsHasValue() const
        {
            return m_SpirVersions.hasValue();
        }

        const SpirVersionsList::item_type getSpirVersionsItem(size_t index) const
        {
            return m_SpirVersions.getItem(index);
        }

        // OpenCLVersions
        OpenCLVersionsList::const_iterator begin_OpenCLVersions() const
        {
            return m_OpenCLVersions.begin();
        }

        OpenCLVersionsList::const_iterator end_OpenCLVersions() const
        {
            return m_OpenCLVersions.end();
        }

        size_t size_OpenCLVersions()  const
        {
            return m_OpenCLVersions.size();
        }

        bool empty_OpenCLVersions()  const
        {
            return m_OpenCLVersions.empty();
        }

        bool isOpenCLVersionsHasValue() const
        {
            return m_OpenCLVersions.hasValue();
        }

        const OpenCLVersionsList::item_type getOpenCLVersionsItem(size_t index) const
        {
            return m_OpenCLVersions.getItem(index);
        }

        // SPIRV Extensions
        SPIRVExtensionsList::const_iterator begin_SPIRVExtensions() const
        {
            return m_SPIRVExtensions.begin();
        }

        SPIRVExtensionsList::const_iterator end_SPIRVExtensions() const
        {
            return m_SPIRVExtensions.end();
        }

        size_t size_SPIRVExtensions()  const
        {
            return m_SPIRVExtensions.size();
        }

        bool empty_SPIRVExtensions()  const
        {
            return m_SPIRVExtensions.empty();
        }

        bool isSPIRVExtensionsHasValue() const
        {
            return m_SPIRVExtensions.hasValue();
        }

        const SPIRVExtensionsList::item_type getSPIRVExtensionsItem(size_t index) const
        {
            return m_SPIRVExtensions.getItem(index);
        }

        void deleteMetadata()
        {
            llvm::NamedMDNode* KernelsNode = m_pModule->getNamedMetadata("opencl.kernels");
            if (KernelsNode)
            {
                m_pModule->eraseNamedMetadata(KernelsNode);
            }

            llvm::NamedMDNode* CompilerOptionsNode = m_pModule->getNamedMetadata("opencl.compiler.options");
            if (CompilerOptionsNode)
            {
                m_pModule->eraseNamedMetadata(CompilerOptionsNode);
            }

            llvm::NamedMDNode* CompilerExternalOptionsNode = m_pModule->getNamedMetadata("opencl.compiler.ext.options");
            if (CompilerExternalOptionsNode)
            {
                m_pModule->eraseNamedMetadata(CompilerExternalOptionsNode);
            }

            llvm::NamedMDNode* FloatingPointContractionsNode = m_pModule->getNamedMetadata("opencl.enable.FP_CONTRACT");
            if (FloatingPointContractionsNode)
            {
                m_pModule->eraseNamedMetadata(FloatingPointContractionsNode);
            }

            llvm::NamedMDNode* UsedOptionalCoreFeaturesNode = m_pModule->getNamedMetadata("opencl.used.optional.core.features");
            if (UsedOptionalCoreFeaturesNode)
            {
                m_pModule->eraseNamedMetadata(UsedOptionalCoreFeaturesNode);
            }

            llvm::NamedMDNode* UsedKhrExtensionsNode = m_pModule->getNamedMetadata("opencl.used.extensions");
            if (UsedKhrExtensionsNode)
            {
                m_pModule->eraseNamedMetadata(UsedKhrExtensionsNode);
            }

            llvm::NamedMDNode* SpirVersionsNode = m_pModule->getNamedMetadata("opencl.spir.version");
            if (SpirVersionsNode)
            {
                m_pModule->eraseNamedMetadata(SpirVersionsNode);
            }

            llvm::NamedMDNode* OpenCLVersionsNode = m_pModule->getNamedMetadata("opencl.ocl.version");
            if (OpenCLVersionsNode)
            {
                m_pModule->eraseNamedMetadata(OpenCLVersionsNode);
            }

            llvm::NamedMDNode* SPIRVExtensionsNode = m_pModule->getNamedMetadata("igc.spirv.extensions");
            if (SPIRVExtensionsNode)
            {
                m_pModule->eraseNamedMetadata(SPIRVExtensionsNode);
            }
        }

    private:
        llvm::Module* m_pModule;

        // data members
        KernelsList m_Kernels;
        CompilerOptionsList m_CompilerOptions;
        CompilerExternalOptionsList m_CompilerExternalOptions;
        FloatingPointContractionsList m_FloatingPointContractions;
        UsedOptionalCoreFeaturesList m_UsedOptionalCoreFeatures;
        UsedKhrExtensionsList m_UsedKhrExtensions;
        SpirVersionsList m_SpirVersions;
        OpenCLVersionsList m_OpenCLVersions;
        SPIRVExtensionsList m_SPIRVExtensions;
    };
}
