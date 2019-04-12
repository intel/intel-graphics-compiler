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


#include "SpirMetaDataApi.h"

namespace IGC { namespace SPIRMD
{
static bool isNamedNode(const llvm::Metadata*, const char*);

///
// Ctor - loads the VectorTypeHintMetaData from the given metadata node
//
VectorTypeHintMetaData::VectorTypeHintMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_VecType(getVecTypeNode(pNode)),        
    m_Sign(getSignNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named VectorTypeHintMetaData object
//
VectorTypeHintMetaData::VectorTypeHintMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named VectorTypeHintMetaData object
//
VectorTypeHintMetaData::VectorTypeHintMetaData(const char* name):
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
    if( m_VecType.dirty() )
    {
        return true;
    }        
    if( m_Sign.dirty() )
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
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_VecType.generateNode(context));        
    args.push_back( m_Sign.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void VectorTypeHintMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
{
    assert( pNode && "The target node should be valid pointer");

    // we assume that underlying metadata node has not changed under our foot
    if( pNode == m_pNode && !dirty() )
    {
        return;
    }
#if 0
    // check that we could save the new information to the given node without regenerating it
    if( !compatibleWith(pNode) )
    {
        pNode->replaceAllUsesWith(generateNode(context));
        return;
    }
#endif

    m_VecType.save(context, llvm::cast<llvm::MDNode>(getVecTypeNode(pNode)));        
    m_Sign.save(context, llvm::cast<llvm::MDNode>(getSignNode(pNode)));
}

llvm::Metadata* VectorTypeHintMetaData::getVecTypeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
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
// Ctor - loads the VersionMetaData from the given metadata node
//
VersionMetaData::VersionMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_Major(getMajorNode(pNode)),        
    m_Minor(getMinorNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named VersionMetaData object
//
VersionMetaData::VersionMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named VersionMetaData object
//
VersionMetaData::VersionMetaData(const char* name):
    _Mybase(name),    
    m_pNode(NULL)
{}

bool VersionMetaData::hasValue() const
{
    if (m_Major.hasValue())
    {
        return true;
    }
        
    
    if (m_Minor.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the VersionMetaData`s members has changed
bool VersionMetaData::dirty() const
{
    if( m_Major.dirty() )
    {
        return true;
    }        
    if( m_Minor.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the VersionMetaData instance
void VersionMetaData::discardChanges()
{
    m_Major.discardChanges();        
    m_Minor.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* VersionMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_Major.generateNode(context));        
    args.push_back( m_Minor.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void VersionMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
{
    assert( pNode && "The target node should be valid pointer");

    // we assume that underlying metadata node has not changed under our foot
    if( pNode == m_pNode && !dirty() )
    {
        return;
    }
#if 0
    // check that we could save the new information to the given node without regenerating it
    if( !compatibleWith(pNode) )
    {
        pNode->replaceAllUsesWith(generateNode(context));
        return;
    }
#endif

    m_Major.save(context, llvm::cast<llvm::MDNode>(getMajorNode(pNode)));        
    m_Minor.save(context, llvm::cast<llvm::MDNode>(getMinorNode(pNode)));
}

llvm::Metadata* VersionMetaData::getMajorNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* VersionMetaData::getMinorNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}

                                                                                            

///
// Ctor - loads the WorkGroupDimensionsMetaData from the given metadata node
//
WorkGroupDimensionsMetaData::WorkGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_XDim(getXDimNode(pNode)),        
    m_YDim(getYDimNode(pNode)),        
    m_ZDim(getZDimNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named WorkGroupDimensionsMetaData object
//
WorkGroupDimensionsMetaData::WorkGroupDimensionsMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named WorkGroupDimensionsMetaData object
//
WorkGroupDimensionsMetaData::WorkGroupDimensionsMetaData(const char* name):
    _Mybase(name),    
    m_pNode(NULL)
{}

bool WorkGroupDimensionsMetaData::hasValue() const
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
// Returns true if any of the WorkGroupDimensionsMetaData`s members has changed
bool WorkGroupDimensionsMetaData::dirty() const
{
    if( m_XDim.dirty() )
    {
        return true;
    }        
    if( m_YDim.dirty() )
    {
        return true;
    }        
    if( m_ZDim.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the WorkGroupDimensionsMetaData instance
void WorkGroupDimensionsMetaData::discardChanges()
{
    m_XDim.discardChanges();        
    m_YDim.discardChanges();        
    m_ZDim.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* WorkGroupDimensionsMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_XDim.generateNode(context));        
    args.push_back( m_YDim.generateNode(context));        
    args.push_back( m_ZDim.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void WorkGroupDimensionsMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
{
    assert( pNode && "The target node should be valid pointer");

    // we assume that underlying metadata node has not changed under our foot
    if( pNode == m_pNode && !dirty() )
    {
        return;
    }
#if 0
    // check that we could save the new information to the given node without regenerating it
    if( !compatibleWith(pNode) )
    {
        pNode->replaceAllUsesWith(generateNode(context));
        return;
    }
#endif

    m_XDim.save(context, llvm::cast<llvm::MDNode>(getXDimNode(pNode)));        
    m_YDim.save(context, llvm::cast<llvm::MDNode>(getYDimNode(pNode)));        
    m_ZDim.save(context, llvm::cast<llvm::MDNode>(getZDimNode(pNode)));
}

llvm::Metadata* WorkGroupDimensionsMetaData::getXDimNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* WorkGroupDimensionsMetaData::getYDimNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}
    
llvm::Metadata* WorkGroupDimensionsMetaData::getZDimNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(2 + offset).get();
}

            

///
// Ctor - loads the SubGroupDimensionsMetaData from the given metadata node
//
SubGroupDimensionsMetaData::SubGroupDimensionsMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_SIMD_Size(getSIMD_SizeNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named SubGroupDimensionsMetaData object
//
SubGroupDimensionsMetaData::SubGroupDimensionsMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named SubGroupDimensionsMetaData object
//
SubGroupDimensionsMetaData::SubGroupDimensionsMetaData(const char* name):
    _Mybase(name),    
    m_pNode(NULL)
{}

bool SubGroupDimensionsMetaData::hasValue() const
{
    if (m_SIMD_Size.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the SubGroupDimensionsMetaData`s members has changed
bool SubGroupDimensionsMetaData::dirty() const
{
    if( m_SIMD_Size.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the SubGroupDimensionsMetaData instance
void SubGroupDimensionsMetaData::discardChanges()
{
    m_SIMD_Size.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* SubGroupDimensionsMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_SIMD_Size.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void SubGroupDimensionsMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
{
    assert( pNode && "The target node should be valid pointer");

    // we assume that underlying metadata node has not changed under our foot
    if( pNode == m_pNode && !dirty() )
    {
        return;
    }
#if 0
    // check that we could save the new information to the given node without regenerating it
    if( !compatibleWith(pNode) )
    {
        pNode->replaceAllUsesWith(generateNode(context));
        return;
    }
#endif

    m_SIMD_Size.save(context, llvm::cast<llvm::MDNode>(getSIMD_SizeNode(pNode)));
}

llvm::Metadata* SubGroupDimensionsMetaData::getSIMD_SizeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}


///
// Ctor - loads the WorkgroupWalkOrderMetaData from the given metadata node
//
WorkgroupWalkOrderMetaData::WorkgroupWalkOrderMetaData(const llvm::MDNode* pNode, bool hasId) :
    _Mybase(pNode, hasId),
    m_Dim0(getDim0Node(pNode)),
    m_Dim1(getDim1Node(pNode)),
    m_Dim2(getDim2Node(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named SubGroupSizeMetaData object
//
WorkgroupWalkOrderMetaData::WorkgroupWalkOrderMetaData() :
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named SubGroupSizeMetaData object
//
WorkgroupWalkOrderMetaData::WorkgroupWalkOrderMetaData(const char* name) :
    _Mybase(name),
    m_pNode(NULL)
{}

bool WorkgroupWalkOrderMetaData::hasValue() const
{
    if (m_Dim0.hasValue())
    {
        return true;
    }

    if (m_Dim1.hasValue())
    {
        return true;
    }

    if (m_Dim2.hasValue())
    {
        return true;
    }

    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the SubGroupSizeMetaData`s members has changed
bool WorkgroupWalkOrderMetaData::dirty() const
{
    if (m_Dim0.dirty())
    {
        return true;
    }

    if (m_Dim1.dirty())
    {
        return true;
    }

    if (m_Dim2.dirty())
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the SubGroupSizeMetaData instance
void WorkgroupWalkOrderMetaData::discardChanges()
{
    m_Dim0.discardChanges();
    m_Dim1.discardChanges();
    m_Dim2.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
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

///
// Saves the structure changes to the given MDNode
void WorkgroupWalkOrderMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_Dim0.save(context, llvm::cast<llvm::MDNode>(getDim0Node(pNode)));
    m_Dim1.save(context, llvm::cast<llvm::MDNode>(getDim1Node(pNode)));
    m_Dim2.save(context, llvm::cast<llvm::MDNode>(getDim2Node(pNode)));
}

llvm::Metadata* WorkgroupWalkOrderMetaData::getDim0Node(const llvm::MDNode* pParentNode) const
{
    if (!pParentNode)
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}

llvm::Metadata* WorkgroupWalkOrderMetaData::getDim1Node(const llvm::MDNode* pParentNode) const
{
    if (!pParentNode)
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}

llvm::Metadata* WorkgroupWalkOrderMetaData::getDim2Node(const llvm::MDNode* pParentNode) const
{
    if (!pParentNode)
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(2 + offset).get();
}
            

///
// Ctor - loads the KernelMetaData from the given metadata node
//
KernelMetaData::KernelMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_Function(getFunctionNode(pNode)),        
    m_WorkGroupSizeHint(WorkGroupDimensionsMetaData::get(getWorkGroupSizeHintNode(pNode), true)),        
    m_RequiredWorkGroupSize(WorkGroupDimensionsMetaData::get(getRequiredWorkGroupSizeNode(pNode), true)),        
    m_RequiredSubGroupSize(SubGroupDimensionsMetaData::get(getRequiredSubGroupSizeNode(pNode), true)),        
    m_WorkgroupWalkOrder(WorkgroupWalkOrderMetaData::get(getWorkgroupWalkOrderNode(pNode), true)),
    m_VectorTypeHint(VectorTypeHintMetaData::get(getVectorTypeHintNode(pNode), true)),        
    m_ArgAddressSpaces(getArgAddressSpacesNode(pNode), true),        
    m_ArgAccessQualifiers(getArgAccessQualifiersNode(pNode), true),        
    m_ArgTypes(getArgTypesNode(pNode), true),        
    m_ArgBaseTypes(getArgBaseTypesNode(pNode), true),        
    m_ArgTypeQualifiers(getArgTypeQualifiersNode(pNode), true),        
    m_ArgNames(getArgNamesNode(pNode), true),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named KernelMetaData object
//
KernelMetaData::KernelMetaData():    m_WorkGroupSizeHint(WorkGroupDimensionsMetaDataHandle::ObjectType::get("work_group_size_hint")),        
    m_RequiredWorkGroupSize(WorkGroupDimensionsMetaDataHandle::ObjectType::get("reqd_work_group_size")),        
    m_RequiredSubGroupSize(SubGroupDimensionsMetaDataHandle::ObjectType::get("intel_reqd_sub_group_size")),        
    m_WorkgroupWalkOrder(WorkgroupWalkOrderMetaDataHandle::ObjectType::get("intel_reqd_workgroup_walk_order")),
    m_VectorTypeHint(VectorTypeHintMetaDataHandle::ObjectType::get("vec_type_hint")),        
    m_ArgAddressSpaces("kernel_arg_addr_space"),        
    m_ArgAccessQualifiers("kernel_arg_access_qual"),        
    m_ArgTypes("kernel_arg_type"),        
    m_ArgBaseTypes("kernel_arg_base_type"),        
    m_ArgTypeQualifiers("kernel_arg_type_qual"),        
    m_ArgNames("kernel_arg_name"),
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named KernelMetaData object
//
KernelMetaData::KernelMetaData(const char* name):
    _Mybase(name),    m_WorkGroupSizeHint(WorkGroupDimensionsMetaDataHandle::ObjectType::get("work_group_size_hint")),        
    m_RequiredWorkGroupSize(WorkGroupDimensionsMetaDataHandle::ObjectType::get("reqd_work_group_size")),        
    m_RequiredSubGroupSize(SubGroupDimensionsMetaDataHandle::ObjectType::get("intel_reqd_sub_group_size")),   
    m_WorkgroupWalkOrder(WorkgroupWalkOrderMetaDataHandle::ObjectType::get("intel_reqd_workgroup_walk_order")),
    m_VectorTypeHint(VectorTypeHintMetaDataHandle::ObjectType::get("vec_type_hint")),        
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
    if (m_Function.hasValue())
    {
        return true;
    }
        
    
    if (m_WorkGroupSizeHint->hasValue())
    {
        return true;
    }
        
    
    if (m_RequiredWorkGroupSize->hasValue())
    {
        return true;
    }
        
    
    if (m_RequiredSubGroupSize->hasValue())
    {
        return true;
    }

    if (m_WorkgroupWalkOrder->hasValue())
    {
        return true;
    }
        
    
    if (m_VectorTypeHint->hasValue())
    {
        return true;
    }
        
    
    if (m_ArgAddressSpaces.hasValue())
    {
        return true;
    }
        
    
    if (m_ArgAccessQualifiers.hasValue())
    {
        return true;
    }
        
    
    if (m_ArgTypes.hasValue())
    {
        return true;
    }
        
    
    if (m_ArgBaseTypes.hasValue())
    {
        return true;
    }
        
    
    if (m_ArgTypeQualifiers.hasValue())
    {
        return true;
    }
        
    
    if (m_ArgNames.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the KernelMetaData`s members has changed
bool KernelMetaData::dirty() const
{
    if( m_Function.dirty() )
    {
        return true;
    }        
    if( m_WorkGroupSizeHint.dirty() )
    {
        return true;
    }        
    if( m_RequiredWorkGroupSize.dirty() )
    {
        return true;
    }        
    if( m_RequiredSubGroupSize.dirty() )
    {
        return true;
    }        
    if (m_WorkgroupWalkOrder.dirty())
    {
        return true;
    }
    if( m_VectorTypeHint.dirty() )
    {
        return true;
    }        
    if( m_ArgAddressSpaces.dirty() )
    {
        return true;
    }        
    if( m_ArgAccessQualifiers.dirty() )
    {
        return true;
    }        
    if( m_ArgTypes.dirty() )
    {
        return true;
    }        
    if( m_ArgBaseTypes.dirty() )
    {
        return true;
    }        
    if( m_ArgTypeQualifiers.dirty() )
    {
        return true;
    }        
    if( m_ArgNames.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the KernelMetaData instance
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

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* KernelMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_Function.generateNode(context));        
    if (m_WorkGroupSizeHint->hasValue())
    {
        args.push_back( m_WorkGroupSizeHint.generateNode(context));
    }
        
    if (m_RequiredWorkGroupSize->hasValue())
    {
        args.push_back( m_RequiredWorkGroupSize.generateNode(context));
    }
        
    if (m_RequiredSubGroupSize->hasValue())
    {
        args.push_back( m_RequiredSubGroupSize.generateNode(context));
    }

    if (m_WorkgroupWalkOrder->hasValue())
    {
        args.push_back(m_WorkgroupWalkOrder.generateNode(context));
    }
        
    if (m_VectorTypeHint->hasValue())
    {
        args.push_back( m_VectorTypeHint.generateNode(context));
    }
        
    args.push_back( m_ArgAddressSpaces.generateNode(context));        
    args.push_back( m_ArgAccessQualifiers.generateNode(context));        
    args.push_back( m_ArgTypes.generateNode(context));        
    args.push_back( m_ArgBaseTypes.generateNode(context));        
    args.push_back( m_ArgTypeQualifiers.generateNode(context));        
    if (isArgNamesHasValue())
    {
        args.push_back( m_ArgNames.generateNode(context));
    }

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void KernelMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
{
    assert( pNode && "The target node should be valid pointer");

    // we assume that underlying metadata node has not changed under our foot
    if( pNode == m_pNode && !dirty() )
    {
        return;
    }
#if 0
    // check that we could save the new information to the given node without regenerating it
    if( !compatibleWith(pNode) )
    {
        pNode->replaceAllUsesWith(generateNode(context));
        return;
    }
#endif

    m_Function.save(context, llvm::cast<llvm::MDNode>(getFunctionNode(pNode)));        
    m_WorkGroupSizeHint.save(context, llvm::cast<llvm::MDNode>(getWorkGroupSizeHintNode(pNode)));        
    m_RequiredWorkGroupSize.save(context, llvm::cast<llvm::MDNode>(getRequiredWorkGroupSizeNode(pNode)));        
    m_RequiredSubGroupSize.save(context, llvm::cast<llvm::MDNode>(getRequiredSubGroupSizeNode(pNode)));        
    m_WorkgroupWalkOrder.save(context, llvm::cast<llvm::MDNode>(getWorkgroupWalkOrderNode(pNode)));
    m_VectorTypeHint.save(context, llvm::cast<llvm::MDNode>(getVectorTypeHintNode(pNode)));        
    m_ArgAddressSpaces.save(context, llvm::cast<llvm::MDNode>(getArgAddressSpacesNode(pNode)));        
    m_ArgAccessQualifiers.save(context, llvm::cast<llvm::MDNode>(getArgAccessQualifiersNode(pNode)));        
    m_ArgTypes.save(context, llvm::cast<llvm::MDNode>(getArgTypesNode(pNode)));        
    m_ArgBaseTypes.save(context, llvm::cast<llvm::MDNode>(getArgBaseTypesNode(pNode)));        
    m_ArgTypeQualifiers.save(context, llvm::cast<llvm::MDNode>(getArgTypeQualifiersNode(pNode)));        
    m_ArgNames.save(context, llvm::cast<llvm::MDNode>(getArgNamesNode(pNode)));
}

llvm::Metadata* KernelMetaData::getFunctionNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::MDNode* KernelMetaData::getWorkGroupSizeHintNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "work_group_size_hint") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getRequiredWorkGroupSizeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "reqd_work_group_size") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getRequiredSubGroupSizeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "intel_reqd_sub_group_size") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}

llvm::MDNode* KernelMetaData::getWorkgroupWalkOrderNode(const llvm::MDNode* pParentNode) const
{
    if (!pParentNode)
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for (NodeIterator i = NodeIterator(pParentNode, 1 + offset), e = NodeIterator(pParentNode); i != e; ++i)
    {
        if (isNamedNode(i.get(), "intel_reqd_workgroup_walk_order"))
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getVectorTypeHintNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "vec_type_hint") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getArgAddressSpacesNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "kernel_arg_addr_space") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getArgAccessQualifiersNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "kernel_arg_access_qual") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getArgTypesNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "kernel_arg_type") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getArgBaseTypesNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "kernel_arg_base_type") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getArgTypeQualifiersNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "kernel_arg_type_qual") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* KernelMetaData::getArgNamesNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "kernel_arg_name") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}


///
// dtor
//SpirMetaDataUtils::~SpirMetaDataUtils()
//{
   // 
//}

static bool isNamedNode(const llvm::Metadata* pNode, const char* name)
{
    const llvm::MDNode* pMDNode = llvm::dyn_cast<llvm::MDNode>(pNode);

    if( !pMDNode )
    {
        return false;
    }

    if( pMDNode->getNumOperands() < 1 )
    {
        return false;
    }

    const llvm::MDString* pIdNode = llvm::dyn_cast<const llvm::MDString>(pMDNode->getOperand(0));
    if( !pIdNode )
    {
        return false;
    }

    llvm::StringRef id = pIdNode->getString();
    if( id.compare(name) )
    {
        return false;
    }
    return true;
}

} }
