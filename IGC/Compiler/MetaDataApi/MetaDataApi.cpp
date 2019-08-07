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

// This file is auto-generated, all changes must go through the MetaDataApiGenerator project.


#include "MetaDataApi.h"

namespace IGC {
    namespace IGCMD
    {
        static bool isNamedNode(const llvm::Metadata*, const char*);

        ///
        // Ctor - loads the ArgInfoMetaData from the given metadata node
        //
        ArgInfoMetaData::ArgInfoMetaData(const llvm::MDNode* pNode, bool hasId) :
            _Mybase(pNode, hasId),
            m_ArgId(getArgIdNode(pNode)),
            m_ExplicitArgNum(getExplicitArgNumNode(pNode)),
            m_StructArgOffset(getStructArgOffsetNode(pNode)),
            m_ImgAccessFloatCoords(getImgAccessFloatCoordsNode(pNode)),
            m_ImgAccessIntCoords(getImgAccessIntCoordsNode(pNode)),
            m_pNode(pNode)
        {}

        ///
        // Default Ctor - creates the empty, not named ArgInfoMetaData object
        //
        ArgInfoMetaData::ArgInfoMetaData() : m_ExplicitArgNum("explicit_arg_num"),
            m_StructArgOffset("struct_arg_offset"),
            m_ImgAccessFloatCoords("img_access_float_coords"),
            m_ImgAccessIntCoords("img_access_int_coords"),
            m_pNode(NULL)
        {}

        ///
        // Ctor - creates the empty, named ArgInfoMetaData object
        //
        ArgInfoMetaData::ArgInfoMetaData(const char* name) :
            _Mybase(name), m_ExplicitArgNum("explicit_arg_num"),
            m_StructArgOffset("struct_arg_offset"),
            m_ImgAccessFloatCoords("img_access_float_coords"),
            m_ImgAccessIntCoords("img_access_int_coords"),
            m_pNode(NULL)
        {}

        bool ArgInfoMetaData::hasValue() const
        {
            if (m_ArgId.hasValue())
            {
                return true;
            }


            if (m_ExplicitArgNum.hasValue())
            {
                return true;
            }


            if (m_StructArgOffset.hasValue())
            {
                return true;
            }


            if (m_ImgAccessFloatCoords.hasValue())
            {
                return true;
            }


            if (m_ImgAccessIntCoords.hasValue())
            {
                return true;
            }
            return NULL != m_pNode || dirty();
        }

        ///
        // Returns true if any of the ArgInfoMetaData`s members has changed
        bool ArgInfoMetaData::dirty() const
        {
            if (m_ArgId.dirty())
            {
                return true;
            }
            if (m_ExplicitArgNum.dirty())
            {
                return true;
            }
            if (m_StructArgOffset.dirty())
            {
                return true;
            }
            if (m_ImgAccessFloatCoords.dirty())
            {
                return true;
            }
            if (m_ImgAccessIntCoords.dirty())
            {
                return true;
            }
            return false;
        }

        ///
        // Discards the changes done to the ArgInfoMetaData instance
        void ArgInfoMetaData::discardChanges()
        {
            m_ArgId.discardChanges();
            m_ExplicitArgNum.discardChanges();
            m_StructArgOffset.discardChanges();
            m_ImgAccessFloatCoords.discardChanges();
            m_ImgAccessIntCoords.discardChanges();
        }

        ///
        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* ArgInfoMetaData::generateNode(llvm::LLVMContext& context) const
        {
            llvm::SmallVector<llvm::Metadata*, 5> args;

            llvm::Metadata* pIDNode = _Mybase::generateNode(context);
            if (NULL != pIDNode)
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

        ///
        // Saves the structure changes to the given MDNode
        void ArgInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            assert(pNode && "The target node should be valid pointer");

            // we assume that underlying metadata node has not changed under our foot
            if (pNode == m_pNode && !dirty())
            {
                return;
            }
#if 0
            // check that we could save the new information to the given node without regenerating it
            if (!compatibleWith(pNode))
            {
                pNode->replaceAllUsesWith(generateNode(context));
                return;
            }
#endif

            m_ArgId.save(context, llvm::cast<llvm::MDNode>(getArgIdNode(pNode)));
            m_ExplicitArgNum.save(context, llvm::cast<llvm::MDNode>(getExplicitArgNumNode(pNode)));
            m_StructArgOffset.save(context, llvm::cast<llvm::MDNode>(getStructArgOffsetNode(pNode)));
            m_ImgAccessFloatCoords.save(context, llvm::cast<llvm::MDNode>(getImgAccessFloatCoordsNode(pNode)));
            m_ImgAccessIntCoords.save(context, llvm::cast<llvm::MDNode>(getImgAccessIntCoordsNode(pNode)));
        }

        llvm::Metadata* ArgInfoMetaData::getArgIdNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(0 + offset).get();
        }

        llvm::Metadata* ArgInfoMetaData::getExplicitArgNumNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 1 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "explicit_arg_num"))
                {
                    return i.get();
                }
            }
            return NULL;
        }

        llvm::Metadata* ArgInfoMetaData::getStructArgOffsetNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 1 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "struct_arg_offset"))
                {
                    return i.get();
                }
            }
            return NULL;
        }

        llvm::Metadata* ArgInfoMetaData::getImgAccessFloatCoordsNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 1 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "img_access_float_coords"))
                {
                    return i.get();
                }
            }
            return NULL;
        }

        llvm::Metadata* ArgInfoMetaData::getImgAccessIntCoordsNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 1 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "img_access_int_coords"))
                {
                    return i.get();
                }
            }
            return NULL;
        }

        ///
        // Ctor - loads the ArgDependencyInfoMetaData from the given metadata node
        //
        ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const llvm::MDNode* pNode, bool hasId) :
            _Mybase(pNode, hasId),
            m_Arg(getArgNode(pNode)),
            m_ArgDependency(getArgDependencyNode(pNode)),
            m_pNode(pNode)
        {}

        ///
        // Default Ctor - creates the empty, not named ArgDependencyInfoMetaData object
        //
        ArgDependencyInfoMetaData::ArgDependencyInfoMetaData() :
            m_pNode(NULL)
        {}

        ///
        // Ctor - creates the empty, named ArgDependencyInfoMetaData object
        //
        ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const char* name) :
            _Mybase(name),
            m_pNode(NULL)
        {}

        bool ArgDependencyInfoMetaData::hasValue() const
        {
            if (m_Arg.hasValue())
            {
                return true;
            }


            if (m_ArgDependency.hasValue())
            {
                return true;
            }
            return NULL != m_pNode || dirty();
        }

        ///
        // Returns true if any of the ArgDependencyInfoMetaData`s members has changed
        bool ArgDependencyInfoMetaData::dirty() const
        {
            if (m_Arg.dirty())
            {
                return true;
            }
            if (m_ArgDependency.dirty())
            {
                return true;
            }
            return false;
        }

        ///
        // Discards the changes done to the ArgDependencyInfoMetaData instance
        void ArgDependencyInfoMetaData::discardChanges()
        {
            m_Arg.discardChanges();
            m_ArgDependency.discardChanges();
        }

        ///
        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* ArgDependencyInfoMetaData::generateNode(llvm::LLVMContext& context) const
        {
            llvm::SmallVector<llvm::Metadata*, 5> args;

            llvm::Metadata* pIDNode = _Mybase::generateNode(context);
            if (NULL != pIDNode)
            {
                args.push_back(pIDNode);
            }

            args.push_back(m_Arg.generateNode(context));
            args.push_back(m_ArgDependency.generateNode(context));

            return llvm::MDNode::get(context, args);
        }

        ///
        // Saves the structure changes to the given MDNode
        void ArgDependencyInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            assert(pNode && "The target node should be valid pointer");

            // we assume that underlying metadata node has not changed under our foot
            if (pNode == m_pNode && !dirty())
            {
                return;
            }
#if 0
            // check that we could save the new information to the given node without regenerating it
            if (!compatibleWith(pNode))
            {
                pNode->replaceAllUsesWith(generateNode(context));
                return;
            }
#endif

            m_Arg.save(context, llvm::cast<llvm::MDNode>(getArgNode(pNode)));
            m_ArgDependency.save(context, llvm::cast<llvm::MDNode>(getArgDependencyNode(pNode)));
        }

        llvm::Metadata* ArgDependencyInfoMetaData::getArgNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(0 + offset).get();
        }

        llvm::Metadata* ArgDependencyInfoMetaData::getArgDependencyNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(1 + offset).get();
        }

        ///
        // Ctor - loads the SubGroupSizeMetaData from the given metadata node
        //
        SubGroupSizeMetaData::SubGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId) :
            _Mybase(pNode, hasId),
            m_SIMD_size(getSIMD_sizeNode(pNode)),
            m_pNode(pNode)
        {}

        ///
        // Default Ctor - creates the empty, not named SubGroupSizeMetaData object
        //
        SubGroupSizeMetaData::SubGroupSizeMetaData() :
            m_pNode(NULL)
        {}

        ///
        // Ctor - creates the empty, named SubGroupSizeMetaData object
        //
        SubGroupSizeMetaData::SubGroupSizeMetaData(const char* name) :
            _Mybase(name),
            m_pNode(NULL)
        {}

        bool SubGroupSizeMetaData::hasValue() const
        {
            if (m_SIMD_size.hasValue())
            {
                return true;
            }
            return NULL != m_pNode || dirty();
        }

        ///
        // Returns true if any of the SubGroupSizeMetaData`s members has changed
        bool SubGroupSizeMetaData::dirty() const
        {
            if (m_SIMD_size.dirty())
            {
                return true;
            }
            return false;
        }

        ///
        // Discards the changes done to the SubGroupSizeMetaData instance
        void SubGroupSizeMetaData::discardChanges()
        {
            m_SIMD_size.discardChanges();
        }

        ///
        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* SubGroupSizeMetaData::generateNode(llvm::LLVMContext& context) const
        {
            llvm::SmallVector<llvm::Metadata*, 5> args;

            llvm::Metadata* pIDNode = _Mybase::generateNode(context);
            if (NULL != pIDNode)
            {
                args.push_back(pIDNode);
            }

            args.push_back(m_SIMD_size.generateNode(context));

            return llvm::MDNode::get(context, args);
        }

        ///
        // Saves the structure changes to the given MDNode
        void SubGroupSizeMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            assert(pNode && "The target node should be valid pointer");

            // we assume that underlying metadata node has not changed under our foot
            if (pNode == m_pNode && !dirty())
            {
                return;
            }
#if 0
            // check that we could save the new information to the given node without regenerating it
            if (!compatibleWith(pNode))
            {
                pNode->replaceAllUsesWith(generateNode(context));
                return;
            }
#endif

            m_SIMD_size.save(context, llvm::cast<llvm::MDNode>(getSIMD_sizeNode(pNode)));
        }

        llvm::Metadata* SubGroupSizeMetaData::getSIMD_sizeNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(0 + offset).get();
        }

        ///
        // Ctor - loads the VectorTypeHintMetaData from the given metadata node
        //
        VectorTypeHintMetaData::VectorTypeHintMetaData(const llvm::MDNode* pNode, bool hasId) :
            _Mybase(pNode, hasId),
            m_VecType(getVecTypeNode(pNode)),
            m_Sign(getSignNode(pNode)),
            m_pNode(pNode)
        {}

        ///
        // Default Ctor - creates the empty, not named VectorTypeHintMetaData object
        //
        VectorTypeHintMetaData::VectorTypeHintMetaData() :
            m_pNode(NULL)
        {}

        ///
        // Ctor - creates the empty, named VectorTypeHintMetaData object
        //
        VectorTypeHintMetaData::VectorTypeHintMetaData(const char* name) :
            _Mybase(name),
            m_pNode(NULL)
        {}

        bool VectorTypeHintMetaData::hasValue() const
        {
            if (m_VecType.hasValue())
            {
                return true;
            }


            if (m_Sign.hasValue())
            {
                return true;
            }
            return NULL != m_pNode || dirty();
        }

        ///
        // Returns true if any of the VectorTypeHintMetaData`s members has changed
        bool VectorTypeHintMetaData::dirty() const
        {
            if (m_VecType.dirty())
            {
                return true;
            }
            if (m_Sign.dirty())
            {
                return true;
            }
            return false;
        }

        ///
        // Discards the changes done to the VectorTypeHintMetaData instance
        void VectorTypeHintMetaData::discardChanges()
        {
            m_VecType.discardChanges();
            m_Sign.discardChanges();
        }

        ///
        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* VectorTypeHintMetaData::generateNode(llvm::LLVMContext& context) const
        {
            llvm::SmallVector<llvm::Metadata*, 5> args;

            llvm::Metadata* pIDNode = _Mybase::generateNode(context);
            if (NULL != pIDNode)
            {
                args.push_back(pIDNode);
            }

            args.push_back(m_VecType.generateNode(context));
            args.push_back(m_Sign.generateNode(context));

            return llvm::MDNode::get(context, args);
        }

        ///
        // Saves the structure changes to the given MDNode
        void VectorTypeHintMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            assert(pNode && "The target node should be valid pointer");

            // we assume that underlying metadata node has not changed under our foot
            if (pNode == m_pNode && !dirty())
            {
                return;
            }
#if 0
            // check that we could save the new information to the given node without regenerating it
            if (!compatibleWith(pNode))
            {
                pNode->replaceAllUsesWith(generateNode(context));
                return;
            }
#endif

            m_VecType.save(context, llvm::cast<llvm::MDNode>(getVecTypeNode(pNode)));
            m_Sign.save(context, llvm::cast<llvm::MDNode>(getSignNode(pNode)));
        }

        llvm::Metadata* VectorTypeHintMetaData::getVecTypeNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(0 + offset).get();
        }

        llvm::Metadata* VectorTypeHintMetaData::getSignNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(1 + offset).get();
        }

        ///
        // Ctor - loads the ThreadGroupSizeMetaData from the given metadata node
        //
        ThreadGroupSizeMetaData::ThreadGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId) :
            _Mybase(pNode, hasId),
            m_XDim(getXDimNode(pNode)),
            m_YDim(getYDimNode(pNode)),
            m_ZDim(getZDimNode(pNode)),
            m_pNode(pNode)
        {}

        ///
        // Default Ctor - creates the empty, not named ThreadGroupSizeMetaData object
        //
        ThreadGroupSizeMetaData::ThreadGroupSizeMetaData() :
            m_pNode(NULL)
        {}

        ///
        // Ctor - creates the empty, named ThreadGroupSizeMetaData object
        //
        ThreadGroupSizeMetaData::ThreadGroupSizeMetaData(const char* name) :
            _Mybase(name),
            m_pNode(NULL)
        {}

        bool ThreadGroupSizeMetaData::hasValue() const
        {
            if (m_XDim.hasValue())
            {
                return true;
            }


            if (m_YDim.hasValue())
            {
                return true;
            }


            if (m_ZDim.hasValue())
            {
                return true;
            }
            return NULL != m_pNode || dirty();
        }

        ///
        // Returns true if any of the ThreadGroupSizeMetaData`s members has changed
        bool ThreadGroupSizeMetaData::dirty() const
        {
            if (m_XDim.dirty())
            {
                return true;
            }
            if (m_YDim.dirty())
            {
                return true;
            }
            if (m_ZDim.dirty())
            {
                return true;
            }
            return false;
        }

        ///
        // Discards the changes done to the ThreadGroupSizeMetaData instance
        void ThreadGroupSizeMetaData::discardChanges()
        {
            m_XDim.discardChanges();
            m_YDim.discardChanges();
            m_ZDim.discardChanges();
        }

        ///
        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* ThreadGroupSizeMetaData::generateNode(llvm::LLVMContext& context) const
        {
            llvm::SmallVector<llvm::Metadata*, 5> args;

            llvm::Metadata* pIDNode = _Mybase::generateNode(context);
            if (NULL != pIDNode)
            {
                args.push_back(pIDNode);
            }

            args.push_back(m_XDim.generateNode(context));
            args.push_back(m_YDim.generateNode(context));
            args.push_back(m_ZDim.generateNode(context));

            return llvm::MDNode::get(context, args);
        }

        ///
        // Saves the structure changes to the given MDNode
        void ThreadGroupSizeMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            assert(pNode && "The target node should be valid pointer");

            // we assume that underlying metadata node has not changed under our foot
            if (pNode == m_pNode && !dirty())
            {
                return;
            }
#if 0
            // check that we could save the new information to the given node without regenerating it
            if (!compatibleWith(pNode))
            {
                pNode->replaceAllUsesWith(generateNode(context));
                return;
            }
#endif

            m_XDim.save(context, llvm::cast<llvm::MDNode>(getXDimNode(pNode)));
            m_YDim.save(context, llvm::cast<llvm::MDNode>(getYDimNode(pNode)));
            m_ZDim.save(context, llvm::cast<llvm::MDNode>(getZDimNode(pNode)));
        }

        llvm::Metadata* ThreadGroupSizeMetaData::getXDimNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(0 + offset).get();
        }

        llvm::Metadata* ThreadGroupSizeMetaData::getYDimNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(1 + offset).get();
        }

        llvm::Metadata* ThreadGroupSizeMetaData::getZDimNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            return pParentNode->getOperand(2 + offset).get();
        }



        ///
        // Ctor - loads the FunctionInfoMetaData from the given metadata node
        //
        FunctionInfoMetaData::FunctionInfoMetaData(const llvm::MDNode* pNode, bool hasId) :
            _Mybase(pNode, hasId),
            m_Type(getTypeNode(pNode)),
            m_ArgInfoList(getArgInfoListNode(pNode), true),
            m_ImplicitArgInfoList(getImplicitArgInfoListNode(pNode), true),
            m_ThreadGroupSize(ThreadGroupSizeMetaData::get(getThreadGroupSizeNode(pNode), true)),
            m_ThreadGroupSizeHint(ThreadGroupSizeMetaData::get(getThreadGroupSizeHintNode(pNode), true)),
            m_SubGroupSize(SubGroupSizeMetaData::get(getSubGroupSizeNode(pNode), true)),
            m_OpenCLVectorTypeHint(VectorTypeHintMetaData::get(getOpenCLVectorTypeHintNode(pNode), true)),
            m_pNode(pNode)
        {}

        ///
        // Default Ctor - creates the empty, not named FunctionInfoMetaData object
        //
        FunctionInfoMetaData::FunctionInfoMetaData() : m_Type("function_type"),
            m_ArgInfoList("arg_desc"),
            m_ImplicitArgInfoList("implicit_arg_desc"),
            m_ThreadGroupSize(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size")),
            m_ThreadGroupSizeHint(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size_hint")),
            m_SubGroupSize(SubGroupSizeMetaDataHandle::ObjectType::get("sub_group_size")),
            m_OpenCLVectorTypeHint(VectorTypeHintMetaDataHandle::ObjectType::get("opencl_vec_type_hint")),

            m_pNode(NULL)
        {}

        ///
        // Ctor - creates the empty, named FunctionInfoMetaData object
        //
        FunctionInfoMetaData::FunctionInfoMetaData(const char* name) :
            _Mybase(name), m_Type("function_type"),
            m_ArgInfoList("arg_desc"),
            m_ImplicitArgInfoList("implicit_arg_desc"),
            m_ThreadGroupSize(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size")),
            m_ThreadGroupSizeHint(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size_hint")),
            m_SubGroupSize(SubGroupSizeMetaDataHandle::ObjectType::get("sub_group_size")),

            m_OpenCLVectorTypeHint(VectorTypeHintMetaDataHandle::ObjectType::get("opencl_vec_type_hint")),
            m_pNode(NULL)
        {}

        bool FunctionInfoMetaData::hasValue() const
        {
            if (m_Type.hasValue())
            {
                return true;
            }


            if (m_ArgInfoList.hasValue())
            {
                return true;
            }


            if (m_ImplicitArgInfoList.hasValue())
            {
                return true;
            }


            if (m_ThreadGroupSize->hasValue())
            {
                return true;
            }


            if (m_ThreadGroupSizeHint->hasValue())
            {
                return true;
            }


            if (m_SubGroupSize->hasValue())
            {
                return true;

            }


            if (m_OpenCLVectorTypeHint->hasValue())
            {
                return true;
            }

            return NULL != m_pNode || dirty();
        }

        ///
        // Returns true if any of the FunctionInfoMetaData`s members has changed
        bool FunctionInfoMetaData::dirty() const
        {
            if (m_Type.dirty())
            {
                return true;
            }
            if (m_ArgInfoList.dirty())
            {
                return true;
            }
            if (m_ImplicitArgInfoList.dirty())
            {
                return true;
            }
            if (m_ThreadGroupSize.dirty())
            {
                return true;
            }
            if (m_ThreadGroupSizeHint.dirty())
            {
                return true;
            }
            if (m_SubGroupSize.dirty())
            {
                return true;
            }
            if (m_OpenCLVectorTypeHint.dirty())
            {
                return true;
            }
            return false;
        }

        ///
        // Discards the changes done to the FunctionInfoMetaData instance
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

        ///
        // Generates the new MDNode hierarchy for the given structure
        llvm::Metadata* FunctionInfoMetaData::generateNode(llvm::LLVMContext& context) const
        {
            llvm::SmallVector<llvm::Metadata*, 5> args;

            llvm::Metadata* pIDNode = _Mybase::generateNode(context);
            if (NULL != pIDNode)
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

        ///
        // Saves the structure changes to the given MDNode
        void FunctionInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
        {
            assert(pNode && "The target node should be valid pointer");

            // we assume that underlying metadata node has not changed under our foot
            if (pNode == m_pNode && !dirty())
            {
                return;
            }
#if 0
            // check that we could save the new information to the given node without regenerating it
            if (!compatibleWith(pNode))
            {
                pNode->replaceAllUsesWith(generateNode(context));
                return;
            }
#endif

            m_Type.save(context, llvm::cast<llvm::MDNode>(getTypeNode(pNode)));
            m_ArgInfoList.save(context, llvm::cast<llvm::MDNode>(getArgInfoListNode(pNode)));
            m_ImplicitArgInfoList.save(context, llvm::cast<llvm::MDNode>(getImplicitArgInfoListNode(pNode)));
            m_ThreadGroupSize.save(context, llvm::cast<llvm::MDNode>(getThreadGroupSizeNode(pNode)));
            m_ThreadGroupSizeHint.save(context, llvm::cast<llvm::MDNode>(getThreadGroupSizeHintNode(pNode)));
            m_SubGroupSize.save(context, llvm::cast<llvm::MDNode>(getSubGroupSizeNode(pNode)));
            m_OpenCLVectorTypeHint.save(context, llvm::cast<llvm::MDNode>(getOpenCLVectorTypeHintNode(pNode)));
        }

        llvm::Metadata* FunctionInfoMetaData::getTypeNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "function_type"))
                {
                    return i.get();
                }
            }
            return NULL;
        }

        llvm::MDNode* FunctionInfoMetaData::getArgInfoListNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "arg_desc"))
                {
                    return llvm::dyn_cast<llvm::MDNode>(i.get());
                }
            }
            return NULL;
        }

        llvm::MDNode* FunctionInfoMetaData::getImplicitArgInfoListNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "implicit_arg_desc"))
                {
                    return llvm::dyn_cast<llvm::MDNode>(i.get());
                }
            }
            return NULL;
        }

        llvm::MDNode* FunctionInfoMetaData::getThreadGroupSizeNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "thread_group_size"))
                {
                    return llvm::dyn_cast<llvm::MDNode>(i.get());
                }
            }
            return NULL;
        }

        llvm::MDNode* FunctionInfoMetaData::getThreadGroupSizeHintNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "thread_group_size_hint"))
                {
                    return llvm::dyn_cast<llvm::MDNode>(i.get());
                }
            }
            return NULL;
        }

        llvm::MDNode* FunctionInfoMetaData::getSubGroupSizeNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "sub_group_size"))
                {
                    return llvm::dyn_cast<llvm::MDNode>(i.get());
                }
            }
            return NULL;
        }

        llvm::Metadata* FunctionInfoMetaData::getLocalSizeNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "local_size"))
                {
                    return i.get();
                }
            }
            return NULL;
        }

        llvm::MDNode* FunctionInfoMetaData::getOpenCLVectorTypeHintNode(const llvm::MDNode* pParentNode) const
        {
            if (!pParentNode)
            {
                return NULL;
            }

            unsigned int offset = _Mybase::getStartIndex();
            for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
            {
                if (isNamedNode(i.get(), "opencl_vec_type_hint"))
                {
                    return llvm::dyn_cast<llvm::MDNode>(i.get());
                }
            }
            return NULL;
        }

        static bool isNamedNode(const llvm::Metadata* pNode, const char* name)
        {
            const llvm::MDNode* pMDNode = llvm::dyn_cast<llvm::MDNode>(pNode);

            if (!pMDNode)
            {
                return false;
            }

            if (pMDNode->getNumOperands() < 1)
            {
                return false;
            }

            const llvm::MDString* pIdNode = llvm::dyn_cast<const llvm::MDString>(pMDNode->getOperand(0));
            if (!pIdNode)
            {
                return false;
            }

            llvm::StringRef id = pIdNode->getString();
            if (id.compare(name))
            {
                return false;
            }
            return true;
        }

    }
}
