/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MetaDataApi.h"
#include "Probe/Assertion.h"

namespace IGC::IGCMD {
    ArgInfoMetaData::ArgInfoMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_ArgId(getNumberedNode(pNode, 0)),
        m_ExplicitArgNum(getNamedNode(pNode, "explicit_arg_num")),
        m_StructArgOffset(getNamedNode(pNode, "struct_arg_offset")),
        m_ImgAccessFloatCoords(getNamedNode(pNode, "img_access_float_coords")),
        m_ImgAccessIntCoords(getNamedNode(pNode, "img_access_int_coords")),
        m_pNode(pNode)
    {}

    ArgInfoMetaData::ArgInfoMetaData() :
        m_ExplicitArgNum("explicit_arg_num"),
        m_StructArgOffset("struct_arg_offset"),
        m_ImgAccessFloatCoords("img_access_float_coords"),
        m_ImgAccessIntCoords("img_access_int_coords"),
        m_pNode(nullptr)
    {}

    ArgInfoMetaData::ArgInfoMetaData(const char* name) :
        _Mybase(name),
        m_ExplicitArgNum("explicit_arg_num"),
        m_StructArgOffset("struct_arg_offset"),
        m_ImgAccessFloatCoords("img_access_float_coords"),
        m_ImgAccessIntCoords("img_access_int_coords"),
        m_pNode(nullptr)
    {}

    bool ArgInfoMetaData::hasValue() const
    {
        return m_ArgId.hasValue() ||
                m_ExplicitArgNum.hasValue() ||
                m_StructArgOffset.hasValue() ||
                m_ImgAccessFloatCoords.hasValue() ||
                m_ImgAccessIntCoords.hasValue() ||
                nullptr != m_pNode ||
                dirty();
    }

    bool ArgInfoMetaData::dirty() const
    {
        return m_ArgId.dirty() ||
                m_ExplicitArgNum.dirty() ||
                m_StructArgOffset.dirty() ||
                m_ImgAccessFloatCoords.dirty() ||
                m_ImgAccessIntCoords.dirty();
    }

    void ArgInfoMetaData::discardChanges()
    {
        m_ArgId.discardChanges();
        m_ExplicitArgNum.discardChanges();
        m_StructArgOffset.discardChanges();
        m_ImgAccessFloatCoords.discardChanges();
        m_ImgAccessIntCoords.discardChanges();
    }

    llvm::Metadata* ArgInfoMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = IMetaDataObject::generateNode(context);
        if (nullptr != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_ArgId.generateNode(context));
        if (isExplicitArgNumHasValue())
        {
            args.push_back(m_ExplicitArgNum.generateNode(context));
        }

        if (isStructArgOffsetHasValue())
        {
            args.push_back(m_StructArgOffset.generateNode(context));
        }

        if (isImgAccessFloatCoordsHasValue())
        {
            args.push_back(m_ImgAccessFloatCoords.generateNode(context));
        }

        if (isImgAccessIntCoordsHasValue())
        {
            args.push_back(m_ImgAccessIntCoords.generateNode(context));
        }

        return llvm::MDNode::get(context, args);
    }

    void ArgInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_ArgId.save(context, getNumberedNode(pNode, 0));
        m_ExplicitArgNum.save(context, getNamedNode(pNode, "explicit_arg_num"));
        m_StructArgOffset.save(context, getNamedNode(pNode, "struct_arg_offset"));
        m_ImgAccessFloatCoords.save(context, getNamedNode(pNode, "img_access_float_coords"));
        m_ImgAccessIntCoords.save(context, getNamedNode(pNode, "img_access_int_coords"));
    }

    ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_Arg(getNumberedNode(pNode, 0)),
        m_ArgDependency(getNumberedNode(pNode, 1)),
        m_pNode(pNode)
    {}

    ArgDependencyInfoMetaData::ArgDependencyInfoMetaData() :
        m_pNode(nullptr)
    {}

    ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const char* name) :
        _Mybase(name),
        m_pNode(nullptr)
    {}

    bool ArgDependencyInfoMetaData::hasValue() const
    {
        return m_Arg.hasValue() ||
                m_ArgDependency.hasValue() ||
                nullptr != m_pNode ||
                dirty();
    }

    bool ArgDependencyInfoMetaData::dirty() const
    {
        return m_Arg.dirty() ||
                m_ArgDependency.dirty();
    }

    void ArgDependencyInfoMetaData::discardChanges()
    {
        m_Arg.discardChanges();
        m_ArgDependency.discardChanges();
    }

    llvm::Metadata* ArgDependencyInfoMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = IMetaDataObject::generateNode(context);
        if (nullptr != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_Arg.generateNode(context));
        args.push_back(m_ArgDependency.generateNode(context));

        return llvm::MDNode::get(context, args);
    }

    void ArgDependencyInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_Arg.save(context, getNumberedNode(pNode, 0));
        m_ArgDependency.save(context, getNumberedNode(pNode, 1));
    }

    SubGroupSizeMetaData::SubGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_SIMDSize(getNumberedNode(pNode, 0)),
        m_pNode(pNode)
    {}

    SubGroupSizeMetaData::SubGroupSizeMetaData() :
        m_pNode(nullptr)
    {}

    SubGroupSizeMetaData::SubGroupSizeMetaData(const char* name) :
        _Mybase(name),
        m_pNode(nullptr)
    {}

    bool SubGroupSizeMetaData::hasValue() const
    {
        return m_SIMDSize.hasValue() ||
                nullptr != m_pNode ||
                dirty();
    }

    bool SubGroupSizeMetaData::dirty() const
    {
        return m_SIMDSize.dirty();
    }

    void SubGroupSizeMetaData::discardChanges()
    {
        m_SIMDSize.discardChanges();
    }

    llvm::Metadata* SubGroupSizeMetaData::generateNode(llvm::LLVMContext& context) const
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

    void SubGroupSizeMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_SIMDSize.save(context, getNumberedNode(pNode, 0));
    }

    VectorTypeHintMetaData::VectorTypeHintMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_VecType(getNumberedNode(pNode, 0)),
        m_Sign(getNumberedNode(pNode, 1)),
        m_pNode(pNode)
    {}

    VectorTypeHintMetaData::VectorTypeHintMetaData() :
        m_pNode(nullptr)
    {}

    VectorTypeHintMetaData::VectorTypeHintMetaData(const char* name) :
        _Mybase(name),
        m_pNode(nullptr)
    {}

    bool VectorTypeHintMetaData::hasValue() const
    {
        return m_VecType.hasValue() ||
                m_Sign.hasValue() ||
                nullptr != m_pNode ||
                dirty();
    }

    bool VectorTypeHintMetaData::dirty() const
    {
        return m_VecType.dirty() ||
                m_Sign.dirty();
    }

    void VectorTypeHintMetaData::discardChanges()
    {
        m_VecType.discardChanges();
        m_Sign.discardChanges();
    }

    llvm::Metadata* VectorTypeHintMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = IMetaDataObject::generateNode(context);
        if (nullptr != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_VecType.generateNode(context));
        args.push_back(m_Sign.generateNode(context));

        return llvm::MDNode::get(context, args);
    }

    void VectorTypeHintMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_VecType.save(context, getNumberedNode(pNode, 0));
        m_Sign.save(context, getNumberedNode(pNode, 1));
    }

    ThreadGroupSizeMetaData::ThreadGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_XDim(getNumberedNode(pNode, 0)),
        m_YDim(getNumberedNode(pNode, 1)),
        m_ZDim(getNumberedNode(pNode, 2)),
        m_pNode(pNode)
    {}

    ThreadGroupSizeMetaData::ThreadGroupSizeMetaData() :
        m_pNode(nullptr)
    {}

    ThreadGroupSizeMetaData::ThreadGroupSizeMetaData(const char* name) :
        _Mybase(name),
        m_pNode(nullptr)
    {}

    bool ThreadGroupSizeMetaData::hasValue() const
    {
        return m_XDim.hasValue() ||
                m_YDim.hasValue() ||
                m_ZDim.hasValue() ||
                nullptr != m_pNode ||
                dirty();
    }

    bool ThreadGroupSizeMetaData::dirty() const
    {
        return m_XDim.dirty() ||
                m_YDim.dirty() ||
                m_ZDim.dirty();
    }

    void ThreadGroupSizeMetaData::discardChanges()
    {
        m_XDim.discardChanges();
        m_YDim.discardChanges();
        m_ZDim.discardChanges();
    }

    llvm::Metadata* ThreadGroupSizeMetaData::generateNode(llvm::LLVMContext& context) const
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

    void ThreadGroupSizeMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    FunctionInfoMetaData::FunctionInfoMetaData(const llvm::MDNode* pNode, bool hasId) :
        _Mybase(pNode, hasId),
        m_Type(getNamedNode(pNode, "function_type")),
        m_ArgInfoList(getNamedNode(pNode, "arg_desc"), true),
        m_ImplicitArgInfoList(getNamedNode(pNode, "implicit_arg_desc"), true),
        m_ThreadGroupSize(new ThreadGroupSizeMetaData(getNamedNode(pNode, "thread_group_size"), true)),
        m_ThreadGroupSizeHint(new ThreadGroupSizeMetaData(getNamedNode(pNode, "thread_group_size_hint"), true)),
        m_SubGroupSize(new SubGroupSizeMetaData(getNamedNode(pNode, "sub_group_size"), true)),
        m_OpenCLVectorTypeHint(new VectorTypeHintMetaData(getNamedNode(pNode, "opencl_vec_type_hint"), true)),
        m_pNode(pNode)
    {}

    FunctionInfoMetaData::FunctionInfoMetaData() :
        m_Type("function_type"),
        m_ArgInfoList("arg_desc"),
        m_ImplicitArgInfoList("implicit_arg_desc"),
        m_ThreadGroupSize(new ThreadGroupSizeMetaDataHandle::ObjectType("thread_group_size")),
        m_ThreadGroupSizeHint(new ThreadGroupSizeMetaDataHandle::ObjectType("thread_group_size_hint")),
        m_SubGroupSize(new SubGroupSizeMetaDataHandle::ObjectType("sub_group_size")),
        m_OpenCLVectorTypeHint(new VectorTypeHintMetaDataHandle::ObjectType("opencl_vec_type_hint")),
        m_pNode(nullptr)
    {}

    FunctionInfoMetaData::FunctionInfoMetaData(const char* name) :
        _Mybase(name), m_Type("function_type"),
        m_ArgInfoList("arg_desc"),
        m_ImplicitArgInfoList("implicit_arg_desc"),
        m_ThreadGroupSize(new ThreadGroupSizeMetaDataHandle::ObjectType("thread_group_size")),
        m_ThreadGroupSizeHint(new ThreadGroupSizeMetaDataHandle::ObjectType("thread_group_size_hint")),
        m_SubGroupSize(new SubGroupSizeMetaDataHandle::ObjectType("sub_group_size")),
        m_OpenCLVectorTypeHint(new VectorTypeHintMetaDataHandle::ObjectType("opencl_vec_type_hint")),
        m_pNode(nullptr)
    {}

    bool FunctionInfoMetaData::hasValue() const
    {
        return m_Type.hasValue() ||
                m_ArgInfoList.hasValue() ||
                m_ImplicitArgInfoList.hasValue() ||
                m_ThreadGroupSize->hasValue() ||
                m_ThreadGroupSizeHint->hasValue() ||
                m_SubGroupSize->hasValue() ||
                m_OpenCLVectorTypeHint->hasValue() ||
                nullptr != m_pNode ||
                dirty();
    }

    bool FunctionInfoMetaData::dirty() const
    {
        return m_Type.dirty() ||
                m_ArgInfoList.dirty() ||
                m_ImplicitArgInfoList.dirty() ||
                m_ThreadGroupSize.dirty() ||
                m_ThreadGroupSizeHint.dirty() ||
                m_SubGroupSize.dirty() ||
                m_OpenCLVectorTypeHint.dirty();
    }

    void FunctionInfoMetaData::discardChanges()
    {
        m_Type.discardChanges();
        m_ArgInfoList.discardChanges();
        m_ImplicitArgInfoList.discardChanges();
        m_ThreadGroupSize.discardChanges();
        m_ThreadGroupSizeHint.discardChanges();
        m_SubGroupSize.discardChanges();
        m_OpenCLVectorTypeHint.discardChanges();
    }

    llvm::Metadata* FunctionInfoMetaData::generateNode(llvm::LLVMContext& context) const
    {
        llvm::SmallVector<llvm::Metadata*, 5> args;

        llvm::Metadata* pIDNode = IMetaDataObject::generateNode(context);
        if (nullptr != pIDNode)
        {
            args.push_back(pIDNode);
        }

        args.push_back(m_Type.generateNode(context));
        if (isArgInfoListHasValue())
        {
            args.push_back(m_ArgInfoList.generateNode(context));
        }
        if (isImplicitArgInfoListHasValue())
        {
            args.push_back(m_ImplicitArgInfoList.generateNode(context));
        }
        if (m_ThreadGroupSize->hasValue())
        {
            args.push_back(m_ThreadGroupSize.generateNode(context));
        }
        if (m_ThreadGroupSizeHint->hasValue())
        {
            args.push_back(m_ThreadGroupSizeHint.generateNode(context));
        }
        if (m_SubGroupSize->hasValue())
        {
            args.push_back(m_SubGroupSize.generateNode(context));
        }
        if (m_OpenCLVectorTypeHint->hasValue())
        {
            args.push_back(m_OpenCLVectorTypeHint.generateNode(context));
        }
        return llvm::MDNode::get(context, args);
    }

    void FunctionInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
    {
        IGC_ASSERT_MESSAGE(nullptr != pNode, "The target node should be valid pointer");

        // we assume that underlying metadata node has not changed under our foot
        if (pNode == m_pNode && !dirty())
        {
            return;
        }

        m_Type.save(context, getNamedNode(pNode, "function_type"));
        m_ArgInfoList.save(context, getNamedNode(pNode, "arg_desc"));
        m_ImplicitArgInfoList.save(context, getNamedNode(pNode, "implicit_arg_desc"));
        m_ThreadGroupSize.save(context, getNamedNode(pNode, "thread_group_size"));
        m_ThreadGroupSizeHint.save(context, getNamedNode(pNode, "thread_group_size_hint"));
        m_SubGroupSize.save(context, getNamedNode(pNode, "sub_group_size"));
        m_OpenCLVectorTypeHint.save(context, getNamedNode(pNode, "opencl_vec_type_hint"));
    }
}
