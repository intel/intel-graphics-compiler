/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpirMetaDataApi.h"
#include "Probe/Assertion.h"

namespace IGC::SPIRMD
{
    VersionMetaData::VersionMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_Major(getNumberedNode(pNode, 0)),
        m_Minor(getNumberedNode(pNode, 1)),
        m_pNode(pNode)
    {}

    VersionMetaData::VersionMetaData() :
        m_pNode(NULL)
    {}

    VersionMetaData::VersionMetaData(const char* name) :
        _Mybase(name),
        m_pNode(NULL)
    {}

    bool VersionMetaData::hasValue() const
    {
        return m_Major.hasValue() ||
               m_Minor.hasValue() ||
               NULL != m_pNode ||
               dirty();
    }

    bool VersionMetaData::dirty() const
    {
        return m_Major.dirty() ||
               m_Minor.dirty();
    }

    void VersionMetaData::discardChanges()
    {
        m_Major.discardChanges();
        m_Minor.discardChanges();
    }

    llvm::Metadata* VersionMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = _Mybase::generateNode(context);
        if (NULL != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_Major.generateNode(context));
        args.push_back(m_Minor.generateNode(context));

        return llvm::MDNode::get(context, args);
    }

    void VersionMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_Major.save(context, getNumberedNode(pNode, 0));
        m_Minor.save(context, getNumberedNode(pNode, 1));
    }

    WorkGroupDimensionsMetaData::WorkGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_XDim(getNumberedNode(pNode, 0)),
        m_YDim(getNumberedNode(pNode, 1)),
        m_ZDim(getNumberedNode(pNode, 2)),
        m_pNode(pNode)
    {}

    WorkGroupDimensionsMetaData::WorkGroupDimensionsMetaData() :
        m_pNode(nullptr)
    {}

    WorkGroupDimensionsMetaData::WorkGroupDimensionsMetaData(const char* name) :
        _Mybase(name),
        m_pNode(nullptr)
    {}

    bool WorkGroupDimensionsMetaData::dirty() const
    {
        return m_XDim.dirty() ||
               m_YDim.dirty() ||
               m_ZDim.dirty();
    }

    bool WorkGroupDimensionsMetaData::hasValue() const
    {
        return m_XDim.hasValue() ||
               m_YDim.hasValue() ||
               m_ZDim.hasValue() ||
               nullptr != m_pNode ||
               dirty();
    }

    void WorkGroupDimensionsMetaData::discardChanges()
    {
        m_XDim.discardChanges();
        m_YDim.discardChanges();
        m_ZDim.discardChanges();
    }

    llvm::Metadata* WorkGroupDimensionsMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = IMetaDataObject::generateNode(context);
        if (nullptr != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_XDim.generateNode(context));
        args.push_back(m_YDim.generateNode(context));
        args.push_back(m_ZDim.generateNode(context));

        return llvm::MDNode::get(context, args);
    }

    void WorkGroupDimensionsMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_XDim.save(context, getNumberedNode(pNode, 0));
        m_YDim.save(context, getNumberedNode(pNode, 1));
        m_ZDim.save(context, getNumberedNode(pNode, 2));
    }

    SubGroupDimensionsMetaData::SubGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_SIMDSize(getNumberedNode(pNode, 0)),
        m_pNode(pNode)
    {}

    SubGroupDimensionsMetaData::SubGroupDimensionsMetaData() :
        m_pNode(nullptr)
    {}

    SubGroupDimensionsMetaData::SubGroupDimensionsMetaData(const char* name) :
        _Mybase(name),
        m_pNode(nullptr)
    {}

    bool SubGroupDimensionsMetaData::dirty() const
    {
        return m_SIMDSize.dirty();
    }

    bool SubGroupDimensionsMetaData::hasValue() const
    {
        return m_SIMDSize.hasValue() ||
               nullptr != m_pNode ||
               dirty();
    }

    void SubGroupDimensionsMetaData::discardChanges()
    {
        m_SIMDSize.discardChanges();
    }

    llvm::Metadata* SubGroupDimensionsMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = IMetaDataObject::generateNode(context);
        if (nullptr != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_SIMDSize.generateNode(context));

        return llvm::MDNode::get(context, args);
    }

    void SubGroupDimensionsMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_SIMDSize.save(context, getNumberedNode(pNode, 0));
    }

    WorkgroupWalkOrderMetaData::WorkgroupWalkOrderMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_Dim0(getNumberedNode(pNode, 0)),
        m_Dim1(getNumberedNode(pNode, 1)),
        m_Dim2(getNumberedNode(pNode, 2)),
        m_pNode(pNode)
    {}

    WorkgroupWalkOrderMetaData::WorkgroupWalkOrderMetaData() :
        m_pNode(NULL)
    {}

    WorkgroupWalkOrderMetaData::WorkgroupWalkOrderMetaData(const char* name) :
        _Mybase(name),
        m_pNode(NULL)
    {}

    bool WorkgroupWalkOrderMetaData::hasValue() const
    {
        return m_Dim0.hasValue() ||
               m_Dim1.hasValue() ||
               m_Dim2.hasValue() ||
               NULL != m_pNode ||
               dirty();
    }

    bool WorkgroupWalkOrderMetaData::dirty() const
    {

        return m_Dim0.dirty() ||
               m_Dim1.dirty() ||
               m_Dim2.dirty();
    }

    void WorkgroupWalkOrderMetaData::discardChanges()
    {
        m_Dim0.discardChanges();
        m_Dim1.discardChanges();
        m_Dim2.discardChanges();
    }

    llvm::Metadata* WorkgroupWalkOrderMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = _Mybase::generateNode(context);
        if (NULL != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_Dim0.generateNode(context));
        args.push_back(m_Dim1.generateNode(context));
        args.push_back(m_Dim2.generateNode(context));

        return llvm::MDNode::get(context, args);
    }

    void WorkgroupWalkOrderMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_Dim0.save(context, getNumberedNode(pNode, 0));
        m_Dim1.save(context, getNumberedNode(pNode, 1));
        m_Dim2.save(context, getNumberedNode(pNode, 2));
    }

    KernelMetaData::KernelMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_Function(getNumberedNode(pNode, 0)),
        m_WorkGroupSizeHint(new WorkGroupDimensionsMetaData(getNamedNode(pNode, "work_group_size_hint"), true)),
        m_RequiredWorkGroupSize(new WorkGroupDimensionsMetaData(getNamedNode(pNode, "reqd_work_group_size"), true)),
        m_RequiredSubGroupSize(new SubGroupDimensionsMetaData(getNamedNode(pNode, "intel_reqd_sub_group_size"), true)),
        m_WorkgroupWalkOrder(new WorkgroupWalkOrderMetaData(getNamedNode(pNode, "intel_reqd_workgroup_walk_order"), true)),
        m_VectorTypeHint(new VectorTypeHintMetaData(getNamedNode(pNode, "vec_type_hint"), true)),
        m_ArgAddressSpaces(getNamedNode(pNode, "kernel_arg_addr_space"), true),
        m_ArgAccessQualifiers(getNamedNode(pNode, "kernel_arg_access_qual"), true),
        m_ArgTypes(getNamedNode(pNode, "kernel_arg_type"), true),
        m_ArgBaseTypes(getNamedNode(pNode, "kernel_arg_base_type"), true),
        m_ArgTypeQualifiers(getNamedNode(pNode, "kernel_arg_type_qual"), true),
        m_ArgNames(getNamedNode(pNode, "kernel_arg_name"), true),
        m_pNode(pNode)
    {}

    KernelMetaData::KernelMetaData() :
        m_WorkGroupSizeHint(new WorkGroupDimensionsMetaDataHandle::ObjectType("work_group_size_hint")),
        m_RequiredWorkGroupSize(new WorkGroupDimensionsMetaDataHandle::ObjectType("reqd_work_group_size")),
        m_RequiredSubGroupSize(new SubGroupDimensionsMetaDataHandle::ObjectType("intel_reqd_sub_group_size")),
        m_WorkgroupWalkOrder(new WorkgroupWalkOrderMetaDataHandle::ObjectType("intel_reqd_workgroup_walk_order")),
        m_VectorTypeHint(new VectorTypeHintMetaDataHandle::ObjectType("vec_type_hint")),
        m_ArgAddressSpaces("kernel_arg_addr_space"),
        m_ArgAccessQualifiers("kernel_arg_access_qual"),
        m_ArgTypes("kernel_arg_type"),
        m_ArgBaseTypes("kernel_arg_base_type"),
        m_ArgTypeQualifiers("kernel_arg_type_qual"),
        m_ArgNames("kernel_arg_name"),
        m_pNode(NULL)
    {}

    KernelMetaData::KernelMetaData(const char* name) :
        _Mybase(name), m_WorkGroupSizeHint(new WorkGroupDimensionsMetaDataHandle::ObjectType("work_group_size_hint")),
        m_RequiredWorkGroupSize(new WorkGroupDimensionsMetaDataHandle::ObjectType("reqd_work_group_size")),
        m_RequiredSubGroupSize(new SubGroupDimensionsMetaDataHandle::ObjectType("intel_reqd_sub_group_size")),
        m_WorkgroupWalkOrder(new WorkgroupWalkOrderMetaDataHandle::ObjectType("intel_reqd_workgroup_walk_order")),
        m_VectorTypeHint(new VectorTypeHintMetaDataHandle::ObjectType("vec_type_hint")),
        m_ArgAddressSpaces("kernel_arg_addr_space"),
        m_ArgAccessQualifiers("kernel_arg_access_qual"),
        m_ArgTypes("kernel_arg_type"),
        m_ArgBaseTypes("kernel_arg_base_type"),
        m_ArgTypeQualifiers("kernel_arg_type_qual"),
        m_ArgNames("kernel_arg_name"),
        m_pNode(NULL)
    {}

    bool KernelMetaData::hasValue() const
    {
        return m_Function.hasValue() ||
               m_WorkGroupSizeHint->hasValue() ||
               m_RequiredWorkGroupSize->hasValue() ||
               m_RequiredSubGroupSize->hasValue() ||
               m_WorkgroupWalkOrder->hasValue() ||
               m_VectorTypeHint->hasValue() ||
               m_ArgAddressSpaces.hasValue() ||
               m_ArgAccessQualifiers.hasValue() ||
               m_ArgTypes.hasValue() ||
               m_ArgBaseTypes.hasValue() ||
               m_ArgTypeQualifiers.hasValue() ||
               m_ArgNames.hasValue() ||
               NULL != m_pNode ||
               dirty();
    }

    bool KernelMetaData::dirty() const
    {
        return m_Function.dirty() ||
               m_WorkGroupSizeHint->dirty() ||
               m_RequiredWorkGroupSize->dirty() ||
               m_RequiredSubGroupSize->dirty() ||
               m_WorkgroupWalkOrder->dirty() ||
               m_VectorTypeHint->dirty() ||
               m_ArgAddressSpaces.dirty() ||
               m_ArgAccessQualifiers.dirty() ||
               m_ArgTypes.dirty() ||
               m_ArgBaseTypes.dirty() ||
               m_ArgTypeQualifiers.dirty() ||
               m_ArgNames.dirty();
    }

    void KernelMetaData::discardChanges()
    {
        m_Function.discardChanges();
        m_WorkGroupSizeHint.discardChanges();
        m_RequiredWorkGroupSize.discardChanges();
        m_RequiredSubGroupSize.discardChanges();
        m_WorkgroupWalkOrder.discardChanges();
        m_VectorTypeHint.discardChanges();
        m_ArgAddressSpaces.discardChanges();
        m_ArgAccessQualifiers.discardChanges();
        m_ArgTypes.discardChanges();
        m_ArgBaseTypes.discardChanges();
        m_ArgTypeQualifiers.discardChanges();
        m_ArgNames.discardChanges();
    }

    llvm::Metadata* KernelMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = _Mybase::generateNode(context);
        if (NULL != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_Function.generateNode(context));
        if (m_WorkGroupSizeHint->hasValue())
        {
            args.push_back(m_WorkGroupSizeHint.generateNode(context));
        }

        if (m_RequiredWorkGroupSize->hasValue())
        {
            args.push_back(m_RequiredWorkGroupSize.generateNode(context));
        }

        if (m_RequiredSubGroupSize->hasValue())
        {
            args.push_back(m_RequiredSubGroupSize.generateNode(context));
        }

        if (m_WorkgroupWalkOrder->hasValue())
        {
            args.push_back(m_WorkgroupWalkOrder.generateNode(context));
        }

        if (m_VectorTypeHint->hasValue())
        {
            args.push_back(m_VectorTypeHint.generateNode(context));
        }

        args.push_back(m_ArgAddressSpaces.generateNode(context));
        args.push_back(m_ArgAccessQualifiers.generateNode(context));
        args.push_back(m_ArgTypes.generateNode(context));
        args.push_back(m_ArgBaseTypes.generateNode(context));
        args.push_back(m_ArgTypeQualifiers.generateNode(context));
        if (isArgNamesHasValue())
        {
            args.push_back(m_ArgNames.generateNode(context));
        }

        return llvm::MDNode::get(context, args);
    }

    void KernelMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_Function.save(context, getNumberedNode(pNode, 0)),
        m_WorkGroupSizeHint.save(context, getNamedNode(pNode, "work_group_size_hint"));
        m_RequiredWorkGroupSize.save(context, getNamedNode(pNode, "reqd_work_group_size"));
        m_RequiredSubGroupSize.save(context, getNamedNode(pNode, "intel_reqd_sub_group_size"));
        m_WorkgroupWalkOrder.save(context, getNamedNode(pNode, "intel_reqd_workgroup_walk_order"));
        m_VectorTypeHint.save(context, getNamedNode(pNode, "vec_type_hint"));
        m_ArgAddressSpaces.save(context, getNamedNode(pNode, "kernel_arg_addr_space"));
        m_ArgAccessQualifiers.save(context, getNamedNode(pNode, "kernel_arg_access_qual"));
        m_ArgTypes.save(context, getNamedNode(pNode, "kernel_arg_type"));
        m_ArgBaseTypes.save(context, getNamedNode(pNode, "kernel_arg_base_type"));
        m_ArgTypeQualifiers.save(context, getNamedNode(pNode, "kernel_arg_type_qual"));
        m_ArgNames.save(context, getNamedNode(pNode, "kernel_arg_name"));
    }
}
