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

namespace IGC { namespace IGCMD
{
static bool isNamedNode(const llvm::Metadata*, const char*);

///
// Ctor - loads the InlineSamplerMetaData from the given metadata node
//
InlineSamplerMetaData::InlineSamplerMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_Value(getValueNode(pNode)),        
    m_Index(getIndexNode(pNode)),        
    m_NormalizedCoords(getNormalizedCoordsNode(pNode)),        
    m_AddressMode(getAddressModeNode(pNode)),        
    m_TCXAddressMode(getTCXAddressModeNode(pNode)),        
    m_TCYAddressMode(getTCYAddressModeNode(pNode)),        
    m_TCZAddressMode(getTCZAddressModeNode(pNode)),        
    m_MagFilterType(getMagFilterTypeNode(pNode)),        
    m_MinFilterType(getMinFilterTypeNode(pNode)),        
    m_MipFilterType(getMipFilterTypeNode(pNode)),        
    m_CompareFunc(getCompareFuncNode(pNode)),        
    m_BorderColorR(getBorderColorRNode(pNode)),        
    m_BorderColorG(getBorderColorGNode(pNode)),        
    m_BorderColorB(getBorderColorBNode(pNode)),        
    m_BorderColorA(getBorderColorANode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named InlineSamplerMetaData object
//
InlineSamplerMetaData::InlineSamplerMetaData():    m_NormalizedCoords("normalized_coords"),        
    m_AddressMode("address_mode"),        
    m_TCXAddressMode("tcx_address_mode"),        
    m_TCYAddressMode("tcy_address_mode"),        
    m_TCZAddressMode("tcz_address_mode"),        
    m_MagFilterType("mag_filter_type"),        
    m_MinFilterType("min_filter_type"),        
    m_MipFilterType("mip_filter_type"),        
    m_CompareFunc("compare_func"),        
    m_BorderColorR("border_color_r"),        
    m_BorderColorG("border_color_g"),        
    m_BorderColorB("border_color_b"),        
    m_BorderColorA("border_color_a"),
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named InlineSamplerMetaData object
//
InlineSamplerMetaData::InlineSamplerMetaData(const char* name):
    _Mybase(name),    m_NormalizedCoords("normalized_coords"),        
    m_AddressMode("address_mode"),        
    m_TCXAddressMode("tcx_address_mode"),        
    m_TCYAddressMode("tcy_address_mode"),        
    m_TCZAddressMode("tcz_address_mode"),        
    m_MagFilterType("mag_filter_type"),        
    m_MinFilterType("min_filter_type"),        
    m_MipFilterType("mip_filter_type"),        
    m_CompareFunc("compare_func"),        
    m_BorderColorR("border_color_r"),        
    m_BorderColorG("border_color_g"),        
    m_BorderColorB("border_color_b"),        
    m_BorderColorA("border_color_a"),
    m_pNode(NULL)
{}

bool InlineSamplerMetaData::hasValue() const
{
    if (m_Value.hasValue())
    {
        return true;
    }
        
    
    if (m_Index.hasValue())
    {
        return true;
    }
        
    
    if (m_NormalizedCoords.hasValue())
    {
        return true;
    }
        
    
    if (m_AddressMode.hasValue())
    {
        return true;
    }
        
    
    if (m_TCXAddressMode.hasValue())
    {
        return true;
    }
        
    
    if (m_TCYAddressMode.hasValue())
    {
        return true;
    }
        
    
    if (m_TCZAddressMode.hasValue())
    {
        return true;
    }
        
    
    if (m_MagFilterType.hasValue())
    {
        return true;
    }
        
    
    if (m_MinFilterType.hasValue())
    {
        return true;
    }
        
    
    if (m_MipFilterType.hasValue())
    {
        return true;
    }
        
    
    if (m_CompareFunc.hasValue())
    {
        return true;
    }
        
    
    if (m_BorderColorR.hasValue())
    {
        return true;
    }
        
    
    if (m_BorderColorG.hasValue())
    {
        return true;
    }
        
    
    if (m_BorderColorB.hasValue())
    {
        return true;
    }
        
    
    if (m_BorderColorA.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the InlineSamplerMetaData`s members has changed
bool InlineSamplerMetaData::dirty() const
{
    if( m_Value.dirty() )
    {
        return true;
    }        
    if( m_Index.dirty() )
    {
        return true;
    }        
    if( m_NormalizedCoords.dirty() )
    {
        return true;
    }        
    if( m_AddressMode.dirty() )
    {
        return true;
    }        
    if( m_TCXAddressMode.dirty() )
    {
        return true;
    }        
    if( m_TCYAddressMode.dirty() )
    {
        return true;
    }        
    if( m_TCZAddressMode.dirty() )
    {
        return true;
    }        
    if( m_MagFilterType.dirty() )
    {
        return true;
    }        
    if( m_MinFilterType.dirty() )
    {
        return true;
    }        
    if( m_MipFilterType.dirty() )
    {
        return true;
    }        
    if( m_CompareFunc.dirty() )
    {
        return true;
    }        
    if( m_BorderColorR.dirty() )
    {
        return true;
    }        
    if( m_BorderColorG.dirty() )
    {
        return true;
    }        
    if( m_BorderColorB.dirty() )
    {
        return true;
    }        
    if( m_BorderColorA.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the InlineSamplerMetaData instance
void InlineSamplerMetaData::discardChanges()
{
    m_Value.discardChanges();        
    m_Index.discardChanges();        
    m_NormalizedCoords.discardChanges();        
    m_AddressMode.discardChanges();        
    m_TCXAddressMode.discardChanges();        
    m_TCYAddressMode.discardChanges();        
    m_TCZAddressMode.discardChanges();        
    m_MagFilterType.discardChanges();        
    m_MinFilterType.discardChanges();        
    m_MipFilterType.discardChanges();        
    m_CompareFunc.discardChanges();        
    m_BorderColorR.discardChanges();        
    m_BorderColorG.discardChanges();        
    m_BorderColorB.discardChanges();        
    m_BorderColorA.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* InlineSamplerMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_Value.generateNode(context));        
    args.push_back( m_Index.generateNode(context));        
    args.push_back( m_NormalizedCoords.generateNode(context));        
    args.push_back( m_AddressMode.generateNode(context));        
    args.push_back( m_TCXAddressMode.generateNode(context));        
    args.push_back( m_TCYAddressMode.generateNode(context));        
    args.push_back( m_TCZAddressMode.generateNode(context));        
    args.push_back( m_MagFilterType.generateNode(context));        
    args.push_back( m_MinFilterType.generateNode(context));        
    args.push_back( m_MipFilterType.generateNode(context));        
    args.push_back( m_CompareFunc.generateNode(context));        
    args.push_back( m_BorderColorR.generateNode(context));        
    args.push_back( m_BorderColorG.generateNode(context));        
    args.push_back( m_BorderColorB.generateNode(context));        
    args.push_back( m_BorderColorA.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void InlineSamplerMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_Value.save(context, llvm::cast<llvm::MDNode>(getValueNode(pNode)));        
    m_Index.save(context, llvm::cast<llvm::MDNode>(getIndexNode(pNode)));        
    m_NormalizedCoords.save(context, llvm::cast<llvm::MDNode>(getNormalizedCoordsNode(pNode)));        
    m_AddressMode.save(context, llvm::cast<llvm::MDNode>(getAddressModeNode(pNode)));        
    m_TCXAddressMode.save(context, llvm::cast<llvm::MDNode>(getTCXAddressModeNode(pNode)));        
    m_TCYAddressMode.save(context, llvm::cast<llvm::MDNode>(getTCYAddressModeNode(pNode)));        
    m_TCZAddressMode.save(context, llvm::cast<llvm::MDNode>(getTCZAddressModeNode(pNode)));        
    m_MagFilterType.save(context, llvm::cast<llvm::MDNode>(getMagFilterTypeNode(pNode)));        
    m_MinFilterType.save(context, llvm::cast<llvm::MDNode>(getMinFilterTypeNode(pNode)));        
    m_MipFilterType.save(context, llvm::cast<llvm::MDNode>(getMipFilterTypeNode(pNode)));        
    m_CompareFunc.save(context, llvm::cast<llvm::MDNode>(getCompareFuncNode(pNode)));        
    m_BorderColorR.save(context, llvm::cast<llvm::MDNode>(getBorderColorRNode(pNode)));        
    m_BorderColorG.save(context, llvm::cast<llvm::MDNode>(getBorderColorGNode(pNode)));        
    m_BorderColorB.save(context, llvm::cast<llvm::MDNode>(getBorderColorBNode(pNode)));        
    m_BorderColorA.save(context, llvm::cast<llvm::MDNode>(getBorderColorANode(pNode)));
}

llvm::Metadata* InlineSamplerMetaData::getValueNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* InlineSamplerMetaData::getIndexNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* InlineSamplerMetaData::getNormalizedCoordsNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "normalized_coords") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getAddressModeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "address_mode") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getTCXAddressModeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "tcx_address_mode") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getTCYAddressModeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "tcy_address_mode") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getTCZAddressModeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "tcz_address_mode") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getMagFilterTypeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "mag_filter_type") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getMinFilterTypeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "min_filter_type") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getMipFilterTypeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "mip_filter_type") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getCompareFuncNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "compare_func") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getBorderColorRNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "border_color_r") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getBorderColorGNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "border_color_g") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getBorderColorBNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "border_color_b") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* InlineSamplerMetaData::getBorderColorANode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "border_color_a") )
        {
            return i.get();
        }
    }
    return NULL;
}
               
///
// Ctor - loads the ArgAllocMetaData from the given metadata node
//
ArgAllocMetaData::ArgAllocMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_Type(getTypeNode(pNode)),        
    m_ExtenstionType(getExtenstionTypeNode(pNode)),        
    m_Index(getIndexNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named ArgAllocMetaData object
//
ArgAllocMetaData::ArgAllocMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named ArgAllocMetaData object
//
ArgAllocMetaData::ArgAllocMetaData(const char* name):
    _Mybase(name),    
    m_pNode(NULL)
{}

bool ArgAllocMetaData::hasValue() const
{
    if (m_Type.hasValue())
    {
        return true;
    }
        
    
    if (m_ExtenstionType.hasValue())
    {
        return true;
    }
        
    
    if (m_Index.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the ArgAllocMetaData`s members has changed
bool ArgAllocMetaData::dirty() const
{
    if( m_Type.dirty() )
    {
        return true;
    }        
    if( m_ExtenstionType.dirty() )
    {
        return true;
    }        
    if( m_Index.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the ArgAllocMetaData instance
void ArgAllocMetaData::discardChanges()
{
    m_Type.discardChanges();        
    m_ExtenstionType.discardChanges();        
    m_Index.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* ArgAllocMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_Type.generateNode(context));        
    args.push_back( m_ExtenstionType.generateNode(context));        
    args.push_back( m_Index.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void ArgAllocMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_Type.save(context, llvm::cast<llvm::MDNode>(getTypeNode(pNode)));        
    m_ExtenstionType.save(context, llvm::cast<llvm::MDNode>(getExtenstionTypeNode(pNode)));        
    m_Index.save(context, llvm::cast<llvm::MDNode>(getIndexNode(pNode)));
}

llvm::Metadata* ArgAllocMetaData::getTypeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* ArgAllocMetaData::getExtenstionTypeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}
    
llvm::Metadata* ArgAllocMetaData::getIndexNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(2 + offset).get();
}

                    

///
// Ctor - loads the ArgInfoMetaData from the given metadata node
//
ArgInfoMetaData::ArgInfoMetaData(const llvm::MDNode* pNode, bool hasId):
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
ArgInfoMetaData::ArgInfoMetaData():    m_ExplicitArgNum("explicit_arg_num"),        
    m_StructArgOffset("struct_arg_offset"),        
    m_ImgAccessFloatCoords("img_access_float_coords"),        
    m_ImgAccessIntCoords("img_access_int_coords"),
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named ArgInfoMetaData object
//
ArgInfoMetaData::ArgInfoMetaData(const char* name):
    _Mybase(name),    m_ExplicitArgNum("explicit_arg_num"),        
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
    if( m_ArgId.dirty() )
    {
        return true;
    }        
    if( m_ExplicitArgNum.dirty() )
    {
        return true;
    }        
    if( m_StructArgOffset.dirty() )
    {
        return true;
    }        
    if( m_ImgAccessFloatCoords.dirty() )
    {
        return true;
    }        
    if( m_ImgAccessIntCoords.dirty() )
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
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_ArgId.generateNode(context));        
    if (isExplicitArgNumHasValue())
    {
        args.push_back( m_ExplicitArgNum.generateNode(context));
    }
        
    if (isStructArgOffsetHasValue())
    {
        args.push_back( m_StructArgOffset.generateNode(context));
    }
        
    if (isImgAccessFloatCoordsHasValue())
    {
        args.push_back( m_ImgAccessFloatCoords.generateNode(context));
    }
        
    if (isImgAccessIntCoordsHasValue())
    {
        args.push_back( m_ImgAccessIntCoords.generateNode(context));
    }

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void ArgInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_ArgId.save(context, llvm::cast<llvm::MDNode>(getArgIdNode(pNode)));        
    m_ExplicitArgNum.save(context, llvm::cast<llvm::MDNode>(getExplicitArgNumNode(pNode)));        
    m_StructArgOffset.save(context, llvm::cast<llvm::MDNode>(getStructArgOffsetNode(pNode)));        
    m_ImgAccessFloatCoords.save(context, llvm::cast<llvm::MDNode>(getImgAccessFloatCoordsNode(pNode)));        
    m_ImgAccessIntCoords.save(context, llvm::cast<llvm::MDNode>(getImgAccessIntCoordsNode(pNode)));
}

llvm::Metadata* ArgInfoMetaData::getArgIdNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* ArgInfoMetaData::getExplicitArgNumNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "explicit_arg_num") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* ArgInfoMetaData::getStructArgOffsetNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "struct_arg_offset") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* ArgInfoMetaData::getImgAccessFloatCoordsNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "img_access_float_coords") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* ArgInfoMetaData::getImgAccessIntCoordsNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 1+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "img_access_int_coords") )
        {
            return i.get();
        }
    }
    return NULL;
}

///
// Ctor - loads the LocalOffsetMetaData from the given metadata node
//
LocalOffsetMetaData::LocalOffsetMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_Var(getVarNode(pNode)),        
    m_Offset(getOffsetNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named LocalOffsetMetaData object
//
LocalOffsetMetaData::LocalOffsetMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named LocalOffsetMetaData object
//
LocalOffsetMetaData::LocalOffsetMetaData(const char* name):
    _Mybase(name),    
    m_pNode(NULL)
{}

bool LocalOffsetMetaData::hasValue() const
{
    if (m_Var.hasValue())
    {
        return true;
    }
        
    
    if (m_Offset.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the LocalOffsetMetaData`s members has changed
bool LocalOffsetMetaData::dirty() const
{
    if( m_Var.dirty() )
    {
        return true;
    }        
    if( m_Offset.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the LocalOffsetMetaData instance
void LocalOffsetMetaData::discardChanges()
{
    m_Var.discardChanges();        
    m_Offset.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* LocalOffsetMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_Var.generateNode(context));        
    args.push_back( m_Offset.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void LocalOffsetMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_Var.save(context, llvm::cast<llvm::MDNode>(getVarNode(pNode)));        
    m_Offset.save(context, llvm::cast<llvm::MDNode>(getOffsetNode(pNode)));
}

llvm::Metadata* LocalOffsetMetaData::getVarNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* LocalOffsetMetaData::getOffsetNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}


///
// Ctor - loads the ArgDependencyInfoMetaData from the given metadata node
//
ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_Arg(getArgNode(pNode)),        
    m_ArgDependency(getArgDependencyNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named ArgDependencyInfoMetaData object
//
ArgDependencyInfoMetaData::ArgDependencyInfoMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named ArgDependencyInfoMetaData object
//
ArgDependencyInfoMetaData::ArgDependencyInfoMetaData(const char* name):
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
    if( m_Arg.dirty() )
    {
        return true;
    }        
    if( m_ArgDependency.dirty() )
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
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_Arg.generateNode(context));        
    args.push_back( m_ArgDependency.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void ArgDependencyInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_Arg.save(context, llvm::cast<llvm::MDNode>(getArgNode(pNode)));        
    m_ArgDependency.save(context, llvm::cast<llvm::MDNode>(getArgDependencyNode(pNode)));
}

llvm::Metadata* ArgDependencyInfoMetaData::getArgNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* ArgDependencyInfoMetaData::getArgDependencyNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}            

///
// Ctor - loads the SubGroupSizeMetaData from the given metadata node
//
SubGroupSizeMetaData::SubGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_SIMD_size(getSIMD_sizeNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named SubGroupSizeMetaData object
//
SubGroupSizeMetaData::SubGroupSizeMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named SubGroupSizeMetaData object
//
SubGroupSizeMetaData::SubGroupSizeMetaData(const char* name):
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
    if( m_SIMD_size.dirty() )
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
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_SIMD_size.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void SubGroupSizeMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_SIMD_size.save(context, llvm::cast<llvm::MDNode>(getSIMD_sizeNode(pNode)));
}

llvm::Metadata* SubGroupSizeMetaData::getSIMD_sizeNode( const llvm::MDNode* pParentNode) const
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
    
llvm::Metadata* VectorTypeHintMetaData::getSignNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}

                    

///
// Ctor - loads the PointerProgramBinaryInfoMetaData from the given metadata node
//
PointerProgramBinaryInfoMetaData::PointerProgramBinaryInfoMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_PointerBufferIndex(getPointerBufferIndexNode(pNode)),        
    m_PointerOffset(getPointerOffsetNode(pNode)),        
    m_PointeeAddressSpace(getPointeeAddressSpaceNode(pNode)),        
    m_PointeeBufferIndex(getPointeeBufferIndexNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named PointerProgramBinaryInfoMetaData object
//
PointerProgramBinaryInfoMetaData::PointerProgramBinaryInfoMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named PointerProgramBinaryInfoMetaData object
//
PointerProgramBinaryInfoMetaData::PointerProgramBinaryInfoMetaData(const char* name):
    _Mybase(name),    
    m_pNode(NULL)
{}

bool PointerProgramBinaryInfoMetaData::hasValue() const
{
    if (m_PointerBufferIndex.hasValue())
    {
        return true;
    }
        
    
    if (m_PointerOffset.hasValue())
    {
        return true;
    }
        
    
    if (m_PointeeAddressSpace.hasValue())
    {
        return true;
    }
        
    
    if (m_PointeeBufferIndex.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the PointerProgramBinaryInfoMetaData`s members has changed
bool PointerProgramBinaryInfoMetaData::dirty() const
{
    if( m_PointerBufferIndex.dirty() )
    {
        return true;
    }        
    if( m_PointerOffset.dirty() )
    {
        return true;
    }        
    if( m_PointeeAddressSpace.dirty() )
    {
        return true;
    }        
    if( m_PointeeBufferIndex.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the PointerProgramBinaryInfoMetaData instance
void PointerProgramBinaryInfoMetaData::discardChanges()
{
    m_PointerBufferIndex.discardChanges();        
    m_PointerOffset.discardChanges();        
    m_PointeeAddressSpace.discardChanges();        
    m_PointeeBufferIndex.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* PointerProgramBinaryInfoMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_PointerBufferIndex.generateNode(context));        
    args.push_back( m_PointerOffset.generateNode(context));        
    args.push_back( m_PointeeAddressSpace.generateNode(context));        
    args.push_back( m_PointeeBufferIndex.generateNode(context));

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void PointerProgramBinaryInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_PointerBufferIndex.save(context, llvm::cast<llvm::MDNode>(getPointerBufferIndexNode(pNode)));        
    m_PointerOffset.save(context, llvm::cast<llvm::MDNode>(getPointerOffsetNode(pNode)));        
    m_PointeeAddressSpace.save(context, llvm::cast<llvm::MDNode>(getPointeeAddressSpaceNode(pNode)));        
    m_PointeeBufferIndex.save(context, llvm::cast<llvm::MDNode>(getPointeeBufferIndexNode(pNode)));
}

llvm::Metadata* PointerProgramBinaryInfoMetaData::getPointerBufferIndexNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* PointerProgramBinaryInfoMetaData::getPointerOffsetNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}
    
llvm::Metadata* PointerProgramBinaryInfoMetaData::getPointeeAddressSpaceNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(2 + offset).get();
}
    
llvm::Metadata* PointerProgramBinaryInfoMetaData::getPointeeBufferIndexNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(3 + offset).get();
}

            

///
// Ctor - loads the ResourceAllocMetaData from the given metadata node
//
ResourceAllocMetaData::ResourceAllocMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_UAVsNum(getUAVsNumNode(pNode)),        
    m_SRVsNum(getSRVsNumNode(pNode)),        
    m_SamplersNum(getSamplersNumNode(pNode)),        
    m_ArgAllocs(getArgAllocsNode(pNode), true),        
    m_InlineSamplers(getInlineSamplersNode(pNode), true),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named ResourceAllocMetaData object
//
ResourceAllocMetaData::ResourceAllocMetaData():    m_UAVsNum("uavs_num"),        
    m_SRVsNum("srvs_num"),        
    m_SamplersNum("samplers_num"),        
    m_ArgAllocs("arg_allocs"),        
    m_InlineSamplers("inline_samplers"),
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named ResourceAllocMetaData object
//
ResourceAllocMetaData::ResourceAllocMetaData(const char* name):
    _Mybase(name),    m_UAVsNum("uavs_num"),        
    m_SRVsNum("srvs_num"),        
    m_SamplersNum("samplers_num"),        
    m_ArgAllocs("arg_allocs"),        
    m_InlineSamplers("inline_samplers"),
    m_pNode(NULL)
{}

bool ResourceAllocMetaData::hasValue() const
{
    if (m_UAVsNum.hasValue())
    {
        return true;
    }
        
    
    if (m_SRVsNum.hasValue())
    {
        return true;
    }
        
    
    if (m_SamplersNum.hasValue())
    {
        return true;
    }
        
    
    if (m_ArgAllocs.hasValue())
    {
        return true;
    }
        
    
    if (m_InlineSamplers.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the ResourceAllocMetaData`s members has changed
bool ResourceAllocMetaData::dirty() const
{
    if( m_UAVsNum.dirty() )
    {
        return true;
    }        
    if( m_SRVsNum.dirty() )
    {
        return true;
    }        
    if( m_SamplersNum.dirty() )
    {
        return true;
    }        
    if( m_ArgAllocs.dirty() )
    {
        return true;
    }        
    if( m_InlineSamplers.dirty() )
    {
        return true;
    }
    return false;
}

///
// Discards the changes done to the ResourceAllocMetaData instance
void ResourceAllocMetaData::discardChanges()
{
    m_UAVsNum.discardChanges();        
    m_SRVsNum.discardChanges();        
    m_SamplersNum.discardChanges();        
    m_ArgAllocs.discardChanges();        
    m_InlineSamplers.discardChanges();
}

///
// Generates the new MDNode hierarchy for the given structure
llvm::Metadata* ResourceAllocMetaData::generateNode(llvm::LLVMContext& context) const
{
    llvm::SmallVector<llvm::Metadata*, 5> args;

    llvm::Metadata* pIDNode = _Mybase::generateNode(context);
    if( NULL != pIDNode )
    {
        args.push_back(pIDNode);
    }

    args.push_back( m_UAVsNum.generateNode(context));        
    args.push_back( m_SRVsNum.generateNode(context));        
    args.push_back( m_SamplersNum.generateNode(context));        
    args.push_back( m_ArgAllocs.generateNode(context));        
    if (isInlineSamplersHasValue())
    {
        args.push_back( m_InlineSamplers.generateNode(context));
    }

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void ResourceAllocMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_UAVsNum.save(context, llvm::cast<llvm::MDNode>(getUAVsNumNode(pNode)));        
    m_SRVsNum.save(context, llvm::cast<llvm::MDNode>(getSRVsNumNode(pNode)));        
    m_SamplersNum.save(context, llvm::cast<llvm::MDNode>(getSamplersNumNode(pNode)));        
    m_ArgAllocs.save(context, llvm::cast<llvm::MDNode>(getArgAllocsNode(pNode)));        
    m_InlineSamplers.save(context, llvm::cast<llvm::MDNode>(getInlineSamplersNode(pNode)));
}

llvm::Metadata* ResourceAllocMetaData::getUAVsNumNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "uavs_num") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* ResourceAllocMetaData::getSRVsNumNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "srvs_num") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* ResourceAllocMetaData::getSamplersNumNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "samplers_num") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::MDNode* ResourceAllocMetaData::getArgAllocsNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "arg_allocs") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* ResourceAllocMetaData::getInlineSamplersNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "inline_samplers") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}

            

///
// Ctor - loads the ThreadGroupSizeMetaData from the given metadata node
//
ThreadGroupSizeMetaData::ThreadGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_XDim(getXDimNode(pNode)),        
    m_YDim(getYDimNode(pNode)),        
    m_ZDim(getZDimNode(pNode)),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named ThreadGroupSizeMetaData object
//
ThreadGroupSizeMetaData::ThreadGroupSizeMetaData():    
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named ThreadGroupSizeMetaData object
//
ThreadGroupSizeMetaData::ThreadGroupSizeMetaData(const char* name):
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
void ThreadGroupSizeMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

llvm::Metadata* ThreadGroupSizeMetaData::getXDimNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(0 + offset).get();
}
    
llvm::Metadata* ThreadGroupSizeMetaData::getYDimNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(1 + offset).get();
}
    
llvm::Metadata* ThreadGroupSizeMetaData::getZDimNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    return pParentNode->getOperand(2 + offset).get();
}

                                            

///
// Ctor - loads the FunctionInfoMetaData from the given metadata node
//
FunctionInfoMetaData::FunctionInfoMetaData(const llvm::MDNode* pNode, bool hasId):
    _Mybase(pNode, hasId),
    m_Type(getTypeNode(pNode)),        
    m_ArgInfoList(getArgInfoListNode(pNode), true),        
    m_ImplicitArgInfoList(getImplicitArgInfoListNode(pNode), true),        
    m_ThreadGroupSize(ThreadGroupSizeMetaData::get(getThreadGroupSizeNode(pNode), true)),        
    m_ThreadGroupSizeHint(ThreadGroupSizeMetaData::get(getThreadGroupSizeHintNode(pNode), true)),        
    m_SubGroupSize(SubGroupSizeMetaData::get(getSubGroupSizeNode(pNode), true)),        
	m_WorkgroupWalkOrder(WorkgroupWalkOrderMetaData::get(getWorkgroupWalkOrderNode(pNode), true)),
    m_GlobalOffsetPresent(getGlobalOffsetPresentNode(pNode)),        
    m_LocalOffsets(getLocalOffsetsNode(pNode), true),        
    m_ResourceAlloc(ResourceAllocMetaData::get(getResourceAllocNode(pNode), true)),        
    m_OpenCLVectorTypeHint(VectorTypeHintMetaData::get(getOpenCLVectorTypeHintNode(pNode), true)),        
    m_OpenCLArgAddressSpaces(getOpenCLArgAddressSpacesNode(pNode), true),        
    m_BufferLocationIndex(getBufferLocationIndexNode(pNode), true),        
    m_BufferLocationCount(getBufferLocationCountNode(pNode), true),        
    m_OpenCLArgAccessQualifiers(getOpenCLArgAccessQualifiersNode(pNode), true),        
    m_OpenCLArgTypes(getOpenCLArgTypesNode(pNode), true),        
    m_OpenCLArgBaseTypes(getOpenCLArgBaseTypesNode(pNode), true),        
    m_OpenCLArgTypeQualifiers(getOpenCLArgTypeQualifiersNode(pNode), true),        
    m_OpenCLArgNames(getOpenCLArgNamesNode(pNode), true),
    m_pNode(pNode)
{}

///
// Default Ctor - creates the empty, not named FunctionInfoMetaData object
//
FunctionInfoMetaData::FunctionInfoMetaData():    m_Type("function_type"),        
    m_ArgInfoList("arg_desc"),        
    m_ImplicitArgInfoList("implicit_arg_desc"),        
    m_ThreadGroupSize(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size")),        
    m_ThreadGroupSizeHint(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size_hint")),        
    m_SubGroupSize(SubGroupSizeMetaDataHandle::ObjectType::get("sub_group_size")),        
	m_WorkgroupWalkOrder(WorkgroupWalkOrderMetaDataHandle::ObjectType::get("intel_reqd_workgroup_walk_order")),
    m_GlobalOffsetPresent("global_offset_present"),        
    m_LocalOffsets("local_offsets"),        
    m_ResourceAlloc(ResourceAllocMetaDataHandle::ObjectType::get("resource_alloc")),        
    m_OpenCLVectorTypeHint(VectorTypeHintMetaDataHandle::ObjectType::get("opencl_vec_type_hint")),        
    m_OpenCLArgAddressSpaces("opencl_kernel_arg_addr_space"),        
    m_BufferLocationIndex("buffer_location_index"),        
    m_BufferLocationCount("buffer_location_count"),        
    m_IsEmulationArgument("is_emulation_argument"),
    m_OpenCLArgAccessQualifiers("opencl_kernel_arg_access_qual"),        
    m_OpenCLArgTypes("opencl_kernel_arg_type"),        
    m_OpenCLArgBaseTypes("opencl_kernel_arg_base_type"),        
    m_OpenCLArgTypeQualifiers("opencl_kernel_arg_type_qual"),        
    m_OpenCLArgNames("opencl_kernel_arg_name"),
    m_pNode(NULL)
{}

///
// Ctor - creates the empty, named FunctionInfoMetaData object
//
FunctionInfoMetaData::FunctionInfoMetaData(const char* name):
    _Mybase(name),    m_Type("function_type"),        
    m_ArgInfoList("arg_desc"),        
    m_ImplicitArgInfoList("implicit_arg_desc"),        
    m_ThreadGroupSize(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size")),        
    m_ThreadGroupSizeHint(ThreadGroupSizeMetaDataHandle::ObjectType::get("thread_group_size_hint")),        
    m_SubGroupSize(SubGroupSizeMetaDataHandle::ObjectType::get("sub_group_size")),        
	m_WorkgroupWalkOrder(WorkgroupWalkOrderMetaDataHandle::ObjectType::get("intel_reqd_workgroup_walk_order")),
    m_GlobalOffsetPresent("global_offset_present"),        
    m_LocalOffsets("local_offsets"),        
    m_ResourceAlloc(ResourceAllocMetaDataHandle::ObjectType::get("resource_alloc")),        
    m_OpenCLVectorTypeHint(VectorTypeHintMetaDataHandle::ObjectType::get("opencl_vec_type_hint")),        
    m_OpenCLArgAddressSpaces("opencl_kernel_arg_addr_space"),        
    m_BufferLocationIndex("buffer_location_index"),        
    m_BufferLocationCount("buffer_location_count"),        
    m_IsEmulationArgument("is_emulation_argument"),
    m_OpenCLArgAccessQualifiers("opencl_kernel_arg_access_qual"),        
    m_OpenCLArgTypes("opencl_kernel_arg_type"),        
    m_OpenCLArgBaseTypes("opencl_kernel_arg_base_type"),        
    m_OpenCLArgTypeQualifiers("opencl_kernel_arg_type_qual"),        
    m_OpenCLArgNames("opencl_kernel_arg_name"),
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


	if (m_WorkgroupWalkOrder->hasValue())
	{
		return true;
	}

    if (m_GlobalOffsetPresent.hasValue())
    {
        return true;
    }
        
    
    if (m_LocalOffsets.hasValue())
    {
        return true;
    }
        
    
    if (m_ResourceAlloc->hasValue())
    {
        return true;
    }

    if (m_OpenCLVectorTypeHint->hasValue())
    {
        return true;
    }
        
    
    if (m_OpenCLArgAddressSpaces.hasValue())
    {
        return true;
    }
        
    
    if (m_BufferLocationIndex.hasValue())
    {
        return true;
    }
        
    
    if (m_BufferLocationCount.hasValue())
    {
        return true;
    }
        
    
    if (m_OpenCLArgAccessQualifiers.hasValue())
    {
        return true;
    }
        
    
    if (m_OpenCLArgTypes.hasValue())
    {
        return true;
    }
        
    
    if (m_OpenCLArgBaseTypes.hasValue())
    {
        return true;
    }
        
    
    if (m_OpenCLArgTypeQualifiers.hasValue())
    {
        return true;
    }
        
    
    if (m_OpenCLArgNames.hasValue())
    {
        return true;
    }
    return NULL != m_pNode || dirty();
}

///
// Returns true if any of the FunctionInfoMetaData`s members has changed
bool FunctionInfoMetaData::dirty() const
{
    if( m_Type.dirty() )
    {
        return true;
    }        
    if( m_ArgInfoList.dirty() )
    {
        return true;
    }        
    if( m_ImplicitArgInfoList.dirty() )
    {
        return true;
    }        
    if( m_ThreadGroupSize.dirty() )
    {
        return true;
    }        
    if( m_ThreadGroupSizeHint.dirty() )
    {
        return true;
    }        
    if( m_SubGroupSize.dirty() )
    {
        return true;
    }   
	if (m_WorkgroupWalkOrder.dirty())
	{
		return true;
	}         
    if( m_GlobalOffsetPresent.dirty() )
    {
        return true;
    }             
    if( m_LocalOffsets.dirty() )
    {
        return true;
    }        
    if( m_ResourceAlloc.dirty() )
    {
        return true;
    }              
    if( m_OpenCLVectorTypeHint.dirty() )
    {
        return true;
    }        
    if( m_OpenCLArgAddressSpaces.dirty() )
    {
        return true;
    }        
    if( m_BufferLocationIndex.dirty() )
    {
        return true;
    }        
    if( m_BufferLocationCount.dirty() )
    {
        return true;
    }        
    if( m_OpenCLArgAccessQualifiers.dirty() )
    {
        return true;
    }        
    if( m_OpenCLArgTypes.dirty() )
    {
        return true;
    }        
    if( m_OpenCLArgBaseTypes.dirty() )
    {
        return true;
    }        
    if( m_OpenCLArgTypeQualifiers.dirty() )
    {
        return true;
    }        
    if( m_OpenCLArgNames.dirty() )
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
	m_WorkgroupWalkOrder.discardChanges();
    m_GlobalOffsetPresent.discardChanges();        
    m_LocalOffsets.discardChanges();        
    m_ResourceAlloc.discardChanges();        
    m_OpenCLVectorTypeHint.discardChanges();        
    m_OpenCLArgAddressSpaces.discardChanges();        
    m_BufferLocationIndex.discardChanges();        
    m_BufferLocationCount.discardChanges();        
    m_IsEmulationArgument.discardChanges();
    m_OpenCLArgAccessQualifiers.discardChanges();        
    m_OpenCLArgTypes.discardChanges();        
    m_OpenCLArgBaseTypes.discardChanges();        
    m_OpenCLArgTypeQualifiers.discardChanges();        
    m_OpenCLArgNames.discardChanges();
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

	if (m_WorkgroupWalkOrder->hasValue())
	{
		args.push_back(m_WorkgroupWalkOrder.generateNode(context));
	}

    if (isGlobalOffsetPresentHasValue())
    {
        args.push_back(m_GlobalOffsetPresent.generateNode(context));
    }

    if (isLocalOffsetsHasValue())
    {
        args.push_back(m_LocalOffsets.generateNode(context));
    }

    if (m_ResourceAlloc->hasValue())
    {
        args.push_back(m_ResourceAlloc.generateNode(context));
    }

    if (m_OpenCLVectorTypeHint->hasValue())
    {
        args.push_back(m_OpenCLVectorTypeHint.generateNode(context));
    }

    if (isOpenCLArgAddressSpacesHasValue())
    {
        args.push_back(m_OpenCLArgAddressSpaces.generateNode(context));
    }

    if (isBufferLocationIndexHasValue())
    {
        args.push_back(m_BufferLocationIndex.generateNode(context));
    }

    if (isBufferLocationCountHasValue())
    {
        args.push_back(m_BufferLocationCount.generateNode(context));
    }

    if (isIsEmulationArgumentHasValue())
    {
        args.push_back(m_IsEmulationArgument.generateNode(context));
    }
        
    if (isOpenCLArgAccessQualifiersHasValue())
    {
        args.push_back( m_OpenCLArgAccessQualifiers.generateNode(context));
    }
        
    if (isOpenCLArgTypesHasValue())
    {
        args.push_back( m_OpenCLArgTypes.generateNode(context));
    }
        
    if (isOpenCLArgBaseTypesHasValue())
    {
        args.push_back( m_OpenCLArgBaseTypes.generateNode(context));
    }
        
    if (isOpenCLArgTypeQualifiersHasValue())
    {
        args.push_back( m_OpenCLArgTypeQualifiers.generateNode(context));
    }
        
    if (isOpenCLArgNamesHasValue())
    {
        args.push_back( m_OpenCLArgNames.generateNode(context));
    }

    return llvm::MDNode::get(context, args);
}

///
// Saves the structure changes to the given MDNode
void FunctionInfoMetaData::save(llvm::LLVMContext& context, llvm::MDNode* pNode) const
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

    m_Type.save(context, llvm::cast<llvm::MDNode>(getTypeNode(pNode)));        
    m_ArgInfoList.save(context, llvm::cast<llvm::MDNode>(getArgInfoListNode(pNode)));        
    m_ImplicitArgInfoList.save(context, llvm::cast<llvm::MDNode>(getImplicitArgInfoListNode(pNode)));        
    m_ThreadGroupSize.save(context, llvm::cast<llvm::MDNode>(getThreadGroupSizeNode(pNode)));        
    m_ThreadGroupSizeHint.save(context, llvm::cast<llvm::MDNode>(getThreadGroupSizeHintNode(pNode)));        
    m_SubGroupSize.save(context, llvm::cast<llvm::MDNode>(getSubGroupSizeNode(pNode)));        
	m_WorkgroupWalkOrder.save(context, llvm::cast<llvm::MDNode>(getSubGroupSizeNode(pNode)));
    m_GlobalOffsetPresent.save(context, llvm::cast<llvm::MDNode>(getGlobalOffsetPresentNode(pNode)));        
    m_LocalOffsets.save(context, llvm::cast<llvm::MDNode>(getLocalOffsetsNode(pNode)));        
    m_ResourceAlloc.save(context, llvm::cast<llvm::MDNode>(getResourceAllocNode(pNode)));        
    m_OpenCLVectorTypeHint.save(context, llvm::cast<llvm::MDNode>(getOpenCLVectorTypeHintNode(pNode)));        
    m_OpenCLArgAddressSpaces.save(context, llvm::cast<llvm::MDNode>(getOpenCLArgAddressSpacesNode(pNode)));        
    m_BufferLocationIndex.save(context, llvm::cast<llvm::MDNode>(getBufferLocationIndexNode(pNode)));        
    m_BufferLocationCount.save(context, llvm::cast<llvm::MDNode>(getBufferLocationCountNode(pNode)));        
    m_IsEmulationArgument.save(context, llvm::cast<llvm::MDNode>(getIsEmulationArgumentNode(pNode)));        
    m_OpenCLArgAccessQualifiers.save(context, llvm::cast<llvm::MDNode>(getOpenCLArgAccessQualifiersNode(pNode)));        
    m_OpenCLArgTypes.save(context, llvm::cast<llvm::MDNode>(getOpenCLArgTypesNode(pNode)));        
    m_OpenCLArgBaseTypes.save(context, llvm::cast<llvm::MDNode>(getOpenCLArgBaseTypesNode(pNode)));        
    m_OpenCLArgTypeQualifiers.save(context, llvm::cast<llvm::MDNode>(getOpenCLArgTypeQualifiersNode(pNode)));        
    m_OpenCLArgNames.save(context, llvm::cast<llvm::MDNode>(getOpenCLArgNamesNode(pNode)));
}

llvm::Metadata* FunctionInfoMetaData::getTypeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "function_type") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getArgInfoListNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "arg_desc") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getImplicitArgInfoListNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "implicit_arg_desc") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getThreadGroupSizeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "thread_group_size") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getThreadGroupSizeHintNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "thread_group_size_hint") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getSubGroupSizeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "sub_group_size") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}

llvm::MDNode* FunctionInfoMetaData::getWorkgroupWalkOrderNode(const llvm::MDNode* pParentNode) const
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
    
llvm::Metadata* FunctionInfoMetaData::getGlobalOffsetPresentNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "global_offset_present") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::Metadata* FunctionInfoMetaData::getLocalSizeNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "local_size") )
        {
            return i.get();
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getLocalOffsetsNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "local_offsets") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getResourceAllocNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "resource_alloc") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getOpenCLVectorTypeHintNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "opencl_vec_type_hint") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getOpenCLArgAddressSpacesNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "opencl_kernel_arg_addr_space") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getBufferLocationIndexNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "buffer_location_index") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getBufferLocationCountNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "buffer_location_count") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}

llvm::MDNode* FunctionInfoMetaData::getIsEmulationArgumentNode(const llvm::MDNode* pParentNode) const
{
    if (!pParentNode)
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
    {
        if (isNamedNode(i.get(), "is_emulation_argument"))
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getOpenCLArgAccessQualifiersNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "opencl_kernel_arg_access_qual") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getOpenCLArgTypesNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "opencl_kernel_arg_type") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getOpenCLArgBaseTypesNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "opencl_kernel_arg_base_type") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getOpenCLArgTypeQualifiersNode( const llvm::MDNode* pParentNode) const
{
    if( !pParentNode )
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for(NodeIterator i = NodeIterator(pParentNode, 0+offset), e = NodeIterator(pParentNode); i != e; ++i )
    {
        if( isNamedNode(i.get(), "opencl_kernel_arg_type_qual") )
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}
    
llvm::MDNode* FunctionInfoMetaData::getOpenCLArgNamesNode(const llvm::MDNode* pParentNode) const
{
    if (!pParentNode)
    {
        return NULL;
    }

    unsigned int offset = _Mybase::getStartIndex();
    for (NodeIterator i = NodeIterator(pParentNode, 0 + offset), e = NodeIterator(pParentNode); i != e; ++i)
    {
        if (isNamedNode(i.get(), "opencl_kernel_arg_name"))
        {
            return llvm::dyn_cast<llvm::MDNode>(i.get());
        }
    }
    return NULL;
}

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
