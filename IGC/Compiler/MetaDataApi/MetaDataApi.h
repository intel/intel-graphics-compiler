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

#pragma once

#include "MetaDataApiUtils.h"

namespace IGC { namespace IGCMD
{
//typedefs and forward declarations
class InlineSamplerMetaData;
typedef MetaObjectHandle<InlineSamplerMetaData> InlineSamplerMetaDataHandle; 
                    
class ArgAllocMetaData;
typedef MetaObjectHandle<ArgAllocMetaData> ArgAllocMetaDataHandle; 
                    
class ArgInfoMetaData;
typedef MetaObjectHandle<ArgInfoMetaData> ArgInfoMetaDataHandle; 
                    
class InputIRVersionMetaData;
typedef MetaObjectHandle<InputIRVersionMetaData> InputIRVersionMetaDataHandle; 
                    
class LocalOffsetMetaData;
typedef MetaObjectHandle<LocalOffsetMetaData> LocalOffsetMetaDataHandle; 
            
class SubGroupSizeMetaData;
typedef MetaObjectHandle<SubGroupSizeMetaData> SubGroupSizeMetaDataHandle; 

class WorkgroupWalkOrderMetaData;
typedef MetaObjectHandle<WorkgroupWalkOrderMetaData> WorkgroupWalkOrderMetaDataHandle;
            
class VectorTypeHintMetaData;
typedef MetaObjectHandle<VectorTypeHintMetaData> VectorTypeHintMetaDataHandle; 
                    
class PointerProgramBinaryInfoMetaData;
typedef MetaObjectHandle<PointerProgramBinaryInfoMetaData> PointerProgramBinaryInfoMetaDataHandle; 
            
class ResourceAllocMetaData;
typedef MetaObjectHandle<ResourceAllocMetaData> ResourceAllocMetaDataHandle; 
            
class ThreadGroupSizeMetaData;
typedef MetaObjectHandle<ThreadGroupSizeMetaData> ThreadGroupSizeMetaDataHandle; 
                                            
class FunctionInfoMetaData;
typedef MetaObjectHandle<FunctionInfoMetaData> FunctionInfoMetaDataHandle; 

typedef MetaDataList<InlineSamplerMetaDataHandle> InlineSamplersMetaDataList;
typedef MetaObjectHandle<InlineSamplersMetaDataList> InlineSamplersMetaDataListHandle;
                                
typedef MetaDataList<ArgAllocMetaDataHandle> ArgAllocsMetaDataList;
typedef MetaObjectHandle<ArgAllocsMetaDataList> ArgAllocsMetaDataListHandle; 
                                
typedef MetaDataList<ArgInfoMetaDataHandle> ArgInfoListMetaDataList;
typedef MetaObjectHandle<ArgInfoListMetaDataList> ArgInfoListMetaDataListHandle; 
                                
typedef MetaDataList<InputIRVersionMetaDataHandle> InputIRVersionsMetaDataList;
typedef MetaObjectHandle<InputIRVersionsMetaDataList> InputIRVersionsMetaDataListHandle; 
                                
typedef MetaDataList<LocalOffsetMetaDataHandle> LocalOffsetsMetaDataList;
typedef MetaObjectHandle<LocalOffsetsMetaDataList> LocalOffsetsMetaDataListHandle; 
                                
typedef MetaDataList<PointerProgramBinaryInfoMetaDataHandle> PointerProgramBinaryInfosMetaDataList;
typedef MetaObjectHandle<PointerProgramBinaryInfosMetaDataList> PointerProgramBinaryInfosMetaDataListHandle; 
                                
typedef MetaDataList<int8_t> InlineBufferMetaDataList;
typedef MetaObjectHandle<InlineBufferMetaDataList> InlineBufferMetaDataListHandle; 
                                
typedef MetaDataList<int32_t> ArgAddressSpacesMetaDataList;
typedef MetaObjectHandle<ArgAddressSpacesMetaDataList> ArgAddressSpacesMetaDataListHandle; 
                                
typedef MetaDataList<int32_t> BufferLocationIndexMetaDataList;
typedef MetaObjectHandle<BufferLocationIndexMetaDataList> BufferLocationIndexMetaDataListHandle; 
                                
typedef MetaDataList<int32_t> BufferLocationCountMetaDataList;
typedef MetaObjectHandle<BufferLocationCountMetaDataList> BufferLocationCountMetaDataListHandle; 
                                
typedef MetaDataList<std::string> ArgAccessQualifiersMetaDataList;
typedef MetaObjectHandle<ArgAccessQualifiersMetaDataList> ArgAccessQualifiersMetaDataListHandle; 
                                
typedef MetaDataList<std::string> ArgTypesMetaDataList;
typedef MetaObjectHandle<ArgTypesMetaDataList> ArgTypesMetaDataListHandle; 
                                
typedef MetaDataList<std::string> ArgBaseTypesMetaDataList;
typedef MetaObjectHandle<ArgBaseTypesMetaDataList> ArgBaseTypesMetaDataListHandle; 
                                
typedef MetaDataList<std::string> ArgTypeQualifiersMetaDataList;
typedef MetaObjectHandle<ArgTypeQualifiersMetaDataList> ArgTypeQualifiersMetaDataListHandle; 
                                
typedef MetaDataList<std::string> ArgNamesMetaDataList;
typedef MetaObjectHandle<ArgNamesMetaDataList> ArgNamesMetaDataListHandle; 

///
// Read/Write the InlineSampler structure from/to LLVM metadata
//
class InlineSamplerMetaData:public IMetaDataObject
{
public:
    typedef InlineSamplerMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<int32_t>::value_type ValueType;        
    typedef MetaDataValue<int32_t>::value_type IndexType;        
    typedef NamedMetaDataValue<int32_t>::value_type NormalizedCoordsType;        
    typedef NamedMetaDataValue<int32_t>::value_type AddressModeType;        
    typedef NamedMetaDataValue<int32_t>::value_type TCXAddressModeType;        
    typedef NamedMetaDataValue<int32_t>::value_type TCYAddressModeType;        
    typedef NamedMetaDataValue<int32_t>::value_type TCZAddressModeType;        
    typedef NamedMetaDataValue<int32_t>::value_type MagFilterTypeType;        
    typedef NamedMetaDataValue<int32_t>::value_type MinFilterTypeType;        
    typedef NamedMetaDataValue<int32_t>::value_type MipFilterTypeType;        
    typedef NamedMetaDataValue<int32_t>::value_type CompareFuncType;        
    typedef NamedMetaDataValue<float>::value_type BorderColorRType;        
    typedef NamedMetaDataValue<float>::value_type BorderColorGType;        
    typedef NamedMetaDataValue<float>::value_type BorderColorBType;        
    typedef NamedMetaDataValue<float>::value_type BorderColorAType;

public:
    ///
    // Factory method - creates the InlineSamplerMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty InlineSamplerMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named InlineSamplerMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the InlineSamplerMetaData from the given metadata node
    //
    InlineSamplerMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named InlineSamplerMetaData object
    //
    InlineSamplerMetaData();

    ///
    // Ctor - creates the empty, named InlineSamplerMetaData object
    //
    InlineSamplerMetaData(const char* name);

    /// Value related methods
    ValueType getValue() const
    {
        return m_Value.get();
    }
    void setValue( const ValueType& val)
    {
        m_Value.set(val);
    }
    bool isValueHasValue() const
    {
        return m_Value.hasValue();
    }
        
    
    /// Index related methods
    IndexType getIndex() const
    {
        return m_Index.get();
    }
    void setIndex( const IndexType& val)
    {
        m_Index.set(val);
    }
    bool isIndexHasValue() const
    {
        return m_Index.hasValue();
    }
        
    
    /// NormalizedCoords related methods
    NormalizedCoordsType getNormalizedCoords() const
    {
        return m_NormalizedCoords.get();
    }
    void setNormalizedCoords( const NormalizedCoordsType& val)
    {
        m_NormalizedCoords.set(val);
    }
    bool isNormalizedCoordsHasValue() const
    {
        return m_NormalizedCoords.hasValue();
    }
        
    
    /// AddressMode related methods
    AddressModeType getAddressMode() const
    {
        return m_AddressMode.get();
    }
    void setAddressMode( const AddressModeType& val)
    {
        m_AddressMode.set(val);
    }
    bool isAddressModeHasValue() const
    {
        return m_AddressMode.hasValue();
    }
        
    
    /// TCXAddressMode related methods
    TCXAddressModeType getTCXAddressMode() const
    {
        return m_TCXAddressMode.get();
    }
    void setTCXAddressMode( const TCXAddressModeType& val)
    {
        m_TCXAddressMode.set(val);
    }
    bool isTCXAddressModeHasValue() const
    {
        return m_TCXAddressMode.hasValue();
    }
        
    
    /// TCYAddressMode related methods
    TCYAddressModeType getTCYAddressMode() const
    {
        return m_TCYAddressMode.get();
    }
    void setTCYAddressMode( const TCYAddressModeType& val)
    {
        m_TCYAddressMode.set(val);
    }
    bool isTCYAddressModeHasValue() const
    {
        return m_TCYAddressMode.hasValue();
    }
        
    
    /// TCZAddressMode related methods
    TCZAddressModeType getTCZAddressMode() const
    {
        return m_TCZAddressMode.get();
    }
    void setTCZAddressMode( const TCZAddressModeType& val)
    {
        m_TCZAddressMode.set(val);
    }
    bool isTCZAddressModeHasValue() const
    {
        return m_TCZAddressMode.hasValue();
    }
        
    
    /// MagFilterType related methods
    MagFilterTypeType getMagFilterType() const
    {
        return m_MagFilterType.get();
    }
    void setMagFilterType( const MagFilterTypeType& val)
    {
        m_MagFilterType.set(val);
    }
    bool isMagFilterTypeHasValue() const
    {
        return m_MagFilterType.hasValue();
    }
        
    
    /// MinFilterType related methods
    MinFilterTypeType getMinFilterType() const
    {
        return m_MinFilterType.get();
    }
    void setMinFilterType( const MinFilterTypeType& val)
    {
        m_MinFilterType.set(val);
    }
    bool isMinFilterTypeHasValue() const
    {
        return m_MinFilterType.hasValue();
    }
        
    
    /// MipFilterType related methods
    MipFilterTypeType getMipFilterType() const
    {
        return m_MipFilterType.get();
    }
    void setMipFilterType( const MipFilterTypeType& val)
    {
        m_MipFilterType.set(val);
    }
    bool isMipFilterTypeHasValue() const
    {
        return m_MipFilterType.hasValue();
    }
        
    
    /// CompareFunc related methods
    CompareFuncType getCompareFunc() const
    {
        return m_CompareFunc.get();
    }
    void setCompareFunc( const CompareFuncType& val)
    {
        m_CompareFunc.set(val);
    }
    bool isCompareFuncHasValue() const
    {
        return m_CompareFunc.hasValue();
    }
        
    
    /// BorderColorR related methods
    BorderColorRType getBorderColorR() const
    {
        return m_BorderColorR.get();
    }
    void setBorderColorR( const BorderColorRType& val)
    {
        m_BorderColorR.set(val);
    }
    bool isBorderColorRHasValue() const
    {
        return m_BorderColorR.hasValue();
    }
        
    
    /// BorderColorG related methods
    BorderColorGType getBorderColorG() const
    {
        return m_BorderColorG.get();
    }
    void setBorderColorG( const BorderColorGType& val)
    {
        m_BorderColorG.set(val);
    }
    bool isBorderColorGHasValue() const
    {
        return m_BorderColorG.hasValue();
    }
        
    
    /// BorderColorB related methods
    BorderColorBType getBorderColorB() const
    {
        return m_BorderColorB.get();
    }
    void setBorderColorB( const BorderColorBType& val)
    {
        m_BorderColorB.set(val);
    }
    bool isBorderColorBHasValue() const
    {
        return m_BorderColorB.hasValue();
    }
        
    
    /// BorderColorA related methods
    BorderColorAType getBorderColorA() const
    {
        return m_BorderColorA.get();
    }
    void setBorderColorA( const BorderColorAType& val)
    {
        m_BorderColorA.set(val);
    }
    bool isBorderColorAHasValue() const
    {
        return m_BorderColorA.hasValue();
    }

    ///
    // Returns true if any of the InlineSamplerMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the InlineSamplerMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getValueNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getIndexNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getNormalizedCoordsNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getAddressModeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getTCXAddressModeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getTCYAddressModeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getTCZAddressModeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getMagFilterTypeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getMinFilterTypeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getMipFilterTypeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getCompareFuncNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getBorderColorRNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getBorderColorGNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getBorderColorBNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getBorderColorANode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<int32_t> m_Value;        
    MetaDataValue<int32_t> m_Index;        
    NamedMetaDataValue<int32_t> m_NormalizedCoords;        
    NamedMetaDataValue<int32_t> m_AddressMode;        
    NamedMetaDataValue<int32_t> m_TCXAddressMode;        
    NamedMetaDataValue<int32_t> m_TCYAddressMode;        
    NamedMetaDataValue<int32_t> m_TCZAddressMode;        
    NamedMetaDataValue<int32_t> m_MagFilterType;        
    NamedMetaDataValue<int32_t> m_MinFilterType;        
    NamedMetaDataValue<int32_t> m_MipFilterType;        
    NamedMetaDataValue<int32_t> m_CompareFunc;        
    NamedMetaDataValue<float> m_BorderColorR;        
    NamedMetaDataValue<float> m_BorderColorG;        
    NamedMetaDataValue<float> m_BorderColorB;        
    NamedMetaDataValue<float> m_BorderColorA;
    // parent node
    const llvm::MDNode* m_pNode;
};

     
///
// Read/Write the ArgAlloc structure from/to LLVM metadata
//
class ArgAllocMetaData:public IMetaDataObject
{
public:
    typedef ArgAllocMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<int32_t>::value_type TypeType;        
    typedef MetaDataValue<int32_t>::value_type ExtenstionTypeType;        
    typedef MetaDataValue<int32_t>::value_type IndexType;

public:
    ///
    // Factory method - creates the ArgAllocMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty ArgAllocMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named ArgAllocMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the ArgAllocMetaData from the given metadata node
    //
    ArgAllocMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named ArgAllocMetaData object
    //
    ArgAllocMetaData();

    ///
    // Ctor - creates the empty, named ArgAllocMetaData object
    //
    ArgAllocMetaData(const char* name);

    /// Type related methods
    TypeType getType() const
    {
        return m_Type.get();
    }
    void setType( const TypeType& val)
    {
        m_Type.set(val);
    }
    bool isTypeHasValue() const
    {
        return m_Type.hasValue();
    }
        
    
    /// ExtenstionType related methods
    ExtenstionTypeType getExtenstionType() const
    {
        return m_ExtenstionType.get();
    }
    void setExtenstionType( const ExtenstionTypeType& val)
    {
        m_ExtenstionType.set(val);
    }
    bool isExtenstionTypeHasValue() const
    {
        return m_ExtenstionType.hasValue();
    }
        
    
    /// Index related methods
    IndexType getIndex() const
    {
        return m_Index.get();
    }
    void setIndex( const IndexType& val)
    {
        m_Index.set(val);
    }
    bool isIndexHasValue() const
    {
        return m_Index.hasValue();
    }

    ///
    // Returns true if any of the ArgAllocMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the ArgAllocMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getTypeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getExtenstionTypeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getIndexNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<int32_t> m_Type;        
    MetaDataValue<int32_t> m_ExtenstionType;        
    MetaDataValue<int32_t> m_Index;
    // parent node
    const llvm::MDNode* m_pNode;
};
                    
///
// Read/Write the ArgInfo structure from/to LLVM metadata
//
class ArgInfoMetaData:public IMetaDataObject
{
public:
    typedef ArgInfoMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<int32_t>::value_type ArgIdType;        
    typedef NamedMetaDataValue<int32_t>::value_type ExplicitArgNumType;        
    typedef NamedMetaDataValue<int32_t>::value_type StructArgOffsetType;        
    typedef NamedMetaDataValue<bool>::value_type ImgAccessFloatCoordsType;        
    typedef NamedMetaDataValue<bool>::value_type ImgAccessIntCoordsType;

public:
    ///
    // Factory method - creates the ArgInfoMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty ArgInfoMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named ArgInfoMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the ArgInfoMetaData from the given metadata node
    //
    ArgInfoMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named ArgInfoMetaData object
    //
    ArgInfoMetaData();

    ///
    // Ctor - creates the empty, named ArgInfoMetaData object
    //
    ArgInfoMetaData(const char* name);

    /// ArgId related methods
    ArgIdType getArgId() const
    {
        return m_ArgId.get();
    }
    void setArgId( const ArgIdType& val)
    {
        m_ArgId.set(val);
    }
    bool isArgIdHasValue() const
    {
        return m_ArgId.hasValue();
    }
        
    
    /// ExplicitArgNum related methods
    ExplicitArgNumType getExplicitArgNum() const
    {
        return m_ExplicitArgNum.get();
    }
    void setExplicitArgNum( const ExplicitArgNumType& val)
    {
        m_ExplicitArgNum.set(val);
    }
    bool isExplicitArgNumHasValue() const
    {
        return m_ExplicitArgNum.hasValue();
    }
        
    
    /// StructArgOffset related methods
    StructArgOffsetType getStructArgOffset() const
    {
        return m_StructArgOffset.get();
    }
    void setStructArgOffset( const StructArgOffsetType& val)
    {
        m_StructArgOffset.set(val);
    }
    bool isStructArgOffsetHasValue() const
    {
        return m_StructArgOffset.hasValue();
    }
        
    
    /// ImgAccessFloatCoords related methods
    ImgAccessFloatCoordsType getImgAccessFloatCoords() const
    {
        return m_ImgAccessFloatCoords.get();
    }
    void setImgAccessFloatCoords( const ImgAccessFloatCoordsType& val)
    {
        m_ImgAccessFloatCoords.set(val);
    }
    bool isImgAccessFloatCoordsHasValue() const
    {
        return m_ImgAccessFloatCoords.hasValue();
    }
        
    
    /// ImgAccessIntCoords related methods
    ImgAccessIntCoordsType getImgAccessIntCoords() const
    {
        return m_ImgAccessIntCoords.get();
    }
    void setImgAccessIntCoords( const ImgAccessIntCoordsType& val)
    {
        m_ImgAccessIntCoords.set(val);
    }
    bool isImgAccessIntCoordsHasValue() const
    {
        return m_ImgAccessIntCoords.hasValue();
    }

    ///
    // Returns true if any of the ArgInfoMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the ArgInfoMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getArgIdNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getExplicitArgNumNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getStructArgOffsetNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getImgAccessFloatCoordsNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getImgAccessIntCoordsNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<int32_t> m_ArgId;        
    NamedMetaDataValue<int32_t> m_ExplicitArgNum;        
    NamedMetaDataValue<int32_t> m_StructArgOffset;        
    NamedMetaDataValue<bool> m_ImgAccessFloatCoords;        
    NamedMetaDataValue<bool> m_ImgAccessIntCoords;
    // parent node
    const llvm::MDNode* m_pNode;
};
                    
///
// Read/Write the InputIRVersion structure from/to LLVM metadata
//
class InputIRVersionMetaData:public IMetaDataObject
{
public:
    typedef InputIRVersionMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<std::string>::value_type NameType;        
    typedef MetaDataValue<int32_t>::value_type MajorType;        
    typedef MetaDataValue<int32_t>::value_type MinorType;

public:
    ///
    // Factory method - creates the InputIRVersionMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty InputIRVersionMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named InputIRVersionMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the InputIRVersionMetaData from the given metadata node
    //
    InputIRVersionMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named InputIRVersionMetaData object
    //
    InputIRVersionMetaData();

    ///
    // Ctor - creates the empty, named InputIRVersionMetaData object
    //
    InputIRVersionMetaData(const char* name);

    /// Name related methods
    NameType getName() const
    {
        return m_Name.get();
    }
    void setName( const NameType& val)
    {
        m_Name.set(val);
    }
    bool isNameHasValue() const
    {
        return m_Name.hasValue();
    }
        
    
    /// Major related methods
    MajorType getMajor() const
    {
        return m_Major.get();
    }
    void setMajor( const MajorType& val)
    {
        m_Major.set(val);
    }
    bool isMajorHasValue() const
    {
        return m_Major.hasValue();
    }
        
    
    /// Minor related methods
    MinorType getMinor() const
    {
        return m_Minor.get();
    }
    void setMinor( const MinorType& val)
    {
        m_Minor.set(val);
    }
    bool isMinorHasValue() const
    {
        return m_Minor.hasValue();
    }

    ///
    // Returns true if any of the InputIRVersionMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the InputIRVersionMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getNameNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getMajorNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getMinorNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<std::string> m_Name;        
    MetaDataValue<int32_t> m_Major;        
    MetaDataValue<int32_t> m_Minor;
    // parent node
    const llvm::MDNode* m_pNode;
};
                    
///
// Read/Write the LocalOffset structure from/to LLVM metadata
//
class LocalOffsetMetaData:public IMetaDataObject
{
public:
    typedef LocalOffsetMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<llvm::GlobalVariable>::value_type VarType;        
    typedef MetaDataValue<int32_t>::value_type OffsetType;

public:
    ///
    // Factory method - creates the LocalOffsetMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty LocalOffsetMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named LocalOffsetMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the LocalOffsetMetaData from the given metadata node
    //
    LocalOffsetMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named LocalOffsetMetaData object
    //
    LocalOffsetMetaData();

    ///
    // Ctor - creates the empty, named LocalOffsetMetaData object
    //
    LocalOffsetMetaData(const char* name);

    /// Var related methods
    VarType getVar() const
    {
        return m_Var.get();
    }
    void setVar( const VarType& val)
    {
        m_Var.set(val);
    }
    bool isVarHasValue() const
    {
        return m_Var.hasValue();
    }
        
    
    /// Offset related methods
    OffsetType getOffset() const
    {
        return m_Offset.get();
    }
    void setOffset( const OffsetType& val)
    {
        m_Offset.set(val);
    }
    bool isOffsetHasValue() const
    {
        return m_Offset.hasValue();
    }

    ///
    // Returns true if any of the LocalOffsetMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the LocalOffsetMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getVarNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getOffsetNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<llvm::GlobalVariable> m_Var;        
    MetaDataValue<int32_t> m_Offset;
    // parent node
    const llvm::MDNode* m_pNode;
};
                    
///
// Read/Write the ArgDependencyInfo structure from/to LLVM metadata
//
class ArgDependencyInfoMetaData:public IMetaDataObject
{
public:
    typedef ArgDependencyInfoMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<std::string>::value_type ArgType;        
    typedef MetaDataValue<int32_t>::value_type ArgDependencyType;

public:
    ///
    // Factory method - creates the ArgDependencyInfoMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty ArgDependencyInfoMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named ArgDependencyInfoMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the ArgDependencyInfoMetaData from the given metadata node
    //
    ArgDependencyInfoMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named ArgDependencyInfoMetaData object
    //
    ArgDependencyInfoMetaData();

    ///
    // Ctor - creates the empty, named ArgDependencyInfoMetaData object
    //
    ArgDependencyInfoMetaData(const char* name);

    /// Arg related methods
    ArgType getArg() const
    {
        return m_Arg.get();
    }
    void setArg( const ArgType& val)
    {
        m_Arg.set(val);
    }
    bool isArgHasValue() const
    {
        return m_Arg.hasValue();
    }
        
    
    /// ArgDependency related methods
    ArgDependencyType getArgDependency() const
    {
        return m_ArgDependency.get();
    }
    void setArgDependency( const ArgDependencyType& val)
    {
        m_ArgDependency.set(val);
    }
    bool isArgDependencyHasValue() const
    {
        return m_ArgDependency.hasValue();
    }

    ///
    // Returns true if any of the ArgDependencyInfoMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the ArgDependencyInfoMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getArgNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getArgDependencyNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<std::string> m_Arg;        
    MetaDataValue<int32_t> m_ArgDependency;
    // parent node
    const llvm::MDNode* m_pNode;
};
            
///
// Read/Write the SubGroupSize structure from/to LLVM metadata
//
class SubGroupSizeMetaData:public IMetaDataObject
{
public:
    typedef SubGroupSizeMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<int32_t>::value_type SIMD_sizeType;

public:
    ///
    // Factory method - creates the SubGroupSizeMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty SubGroupSizeMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named SubGroupSizeMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the SubGroupSizeMetaData from the given metadata node
    //
    SubGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named SubGroupSizeMetaData object
    //
    SubGroupSizeMetaData();

    ///
    // Ctor - creates the empty, named SubGroupSizeMetaData object
    //
    SubGroupSizeMetaData(const char* name);

    /// SIMD_size related methods
    SIMD_sizeType getSIMD_size() const
    {
        return m_SIMD_size.get();
    }
    void setSIMD_size( const SIMD_sizeType& val)
    {
        m_SIMD_size.set(val);
    }
    bool isSIMD_sizeHasValue() const
    {
        return m_SIMD_size.hasValue();
    }

    ///
    // Returns true if any of the SubGroupSizeMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the SubGroupSizeMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getSIMD_sizeNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<int32_t> m_SIMD_size;
    // parent node
    const llvm::MDNode* m_pNode;
};

///
// Read/Write the WorkgroupWalkOrder structure from/to LLVM metadata
//
class WorkgroupWalkOrderMetaData :public IMetaDataObject
{
public:
	typedef WorkgroupWalkOrderMetaData _Myt;
	typedef IMetaDataObject _Mybase;
	// typedefs for data member types
	typedef MetaDataValue<int32_t>::value_type WalkOrderDimType;

public:
	///
	// Factory method - creates the SubGroupSizeMetaData from the given metadata node
	//
	static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
	{
		return new _Myt(pNode, hasId);
	}

	///
	// Factory method - create the default empty SubGroupSizeMetaData object
	static _Myt* get()
	{
		return new _Myt();
	}

	///
	// Factory method - create the default empty named SubGroupSizeMetaData object
	static _Myt* get(const char* name)
	{
		return new _Myt(name);
	}

	///
	// Ctor - loads the SubGroupSizeMetaData from the given metadata node
	//
	WorkgroupWalkOrderMetaData(const llvm::MDNode* pNode, bool hasId);

	///
	// Default Ctor - creates the empty, not named SubGroupSizeMetaData object
	//
	WorkgroupWalkOrderMetaData();

	///
	// Ctor - creates the empty, named SubGroupSizeMetaData object
	//
	WorkgroupWalkOrderMetaData(const char* name);

	/// workgroup walk order related methods
	WalkOrderDimType getDim0() const
	{
		return m_Dim0.get();
	}
	void setDim0(const WalkOrderDimType& val)
	{
		m_Dim0.set(val);
	}
	WalkOrderDimType getDim1() const
	{
		return m_Dim1.get();
	}
	void setDim1(const WalkOrderDimType& val)
	{
		 m_Dim1.set(val);
	}
	WalkOrderDimType getDim2() const
	{
		return m_Dim2.get();
	}
	void setDim2(const WalkOrderDimType& val)
	{
		 m_Dim2.set(val);
	}

	///
	// Returns true if any of the SubGroupSizeMetaData`s members has changed
	bool dirty() const;

	///
	// Returns true if the structure was loaded from the metadata or was changed
	bool hasValue() const;

	///
	// Discards the changes done to the SubGroupSizeMetaData instance
	void discardChanges();

	///
	// Generates the new MDNode hierarchy for the given structure
	llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

	///
	// Saves the structure changes to the given MDNode
	void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
	///
	// Returns true if the given MDNode could be saved to without replacement
	bool compatibleWith(const llvm::MDNode* pNode) const
	{
		return false;
	}

private:
	typedef MetaDataIterator<llvm::MDNode> NodeIterator;

	llvm::Metadata* getDim0Node(const llvm::MDNode* pParentNode) const;
	llvm::Metadata* getDim1Node(const llvm::MDNode* pParentNode) const;
	llvm::Metadata* getDim2Node(const llvm::MDNode* pParentNode) const;

private:
	// data members
	MetaDataValue<int32_t> m_Dim0;
	MetaDataValue<int32_t> m_Dim1;
	MetaDataValue<int32_t> m_Dim2;
	// parent node
	const llvm::MDNode* m_pNode;
};
            
///
// Read/Write the VectorTypeHint structure from/to LLVM metadata
//
class VectorTypeHintMetaData:public IMetaDataObject
{
public:
    typedef VectorTypeHintMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<llvm::UndefValue>::value_type VecTypeType;        
    typedef MetaDataValue<bool>::value_type SignType;

public:
    ///
    // Factory method - creates the VectorTypeHintMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty VectorTypeHintMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named VectorTypeHintMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the VectorTypeHintMetaData from the given metadata node
    //
    VectorTypeHintMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named VectorTypeHintMetaData object
    //
    VectorTypeHintMetaData();

    ///
    // Ctor - creates the empty, named VectorTypeHintMetaData object
    //
    VectorTypeHintMetaData(const char* name);

    /// VecType related methods
    VecTypeType getVecType() const
    {
        return m_VecType.get();
    }
    void setVecType( const VecTypeType& val)
    {
        m_VecType.set(val);
    }
    bool isVecTypeHasValue() const
    {
        return m_VecType.hasValue();
    }
        
    
    /// Sign related methods
    SignType getSign() const
    {
        return m_Sign.get();
    }
    void setSign( const SignType& val)
    {
        m_Sign.set(val);
    }
    bool isSignHasValue() const
    {
        return m_Sign.hasValue();
    }

    ///
    // Returns true if any of the VectorTypeHintMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the VectorTypeHintMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getVecTypeNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getSignNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<llvm::UndefValue> m_VecType;        
    MetaDataValue<bool> m_Sign;
    // parent node
    const llvm::MDNode* m_pNode;
};
                    
///
// Read/Write the PointerProgramBinaryInfo structure from/to LLVM metadata
//
class PointerProgramBinaryInfoMetaData:public IMetaDataObject
{
public:
    typedef PointerProgramBinaryInfoMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<int32_t>::value_type PointerBufferIndexType;        
    typedef MetaDataValue<int32_t>::value_type PointerOffsetType;        
    typedef MetaDataValue<int32_t>::value_type PointeeAddressSpaceType;        
    typedef MetaDataValue<int32_t>::value_type PointeeBufferIndexType;

public:
    ///
    // Factory method - creates the PointerProgramBinaryInfoMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty PointerProgramBinaryInfoMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named PointerProgramBinaryInfoMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the PointerProgramBinaryInfoMetaData from the given metadata node
    //
    PointerProgramBinaryInfoMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named PointerProgramBinaryInfoMetaData object
    //
    PointerProgramBinaryInfoMetaData();

    ///
    // Ctor - creates the empty, named PointerProgramBinaryInfoMetaData object
    //
    PointerProgramBinaryInfoMetaData(const char* name);

    /// PointerBufferIndex related methods
    PointerBufferIndexType getPointerBufferIndex() const
    {
        return m_PointerBufferIndex.get();
    }
    void setPointerBufferIndex( const PointerBufferIndexType& val)
    {
        m_PointerBufferIndex.set(val);
    }
    bool isPointerBufferIndexHasValue() const
    {
        return m_PointerBufferIndex.hasValue();
    }
        
    
    /// PointerOffset related methods
    PointerOffsetType getPointerOffset() const
    {
        return m_PointerOffset.get();
    }
    void setPointerOffset( const PointerOffsetType& val)
    {
        m_PointerOffset.set(val);
    }
    bool isPointerOffsetHasValue() const
    {
        return m_PointerOffset.hasValue();
    }
        
    
    /// PointeeAddressSpace related methods
    PointeeAddressSpaceType getPointeeAddressSpace() const
    {
        return m_PointeeAddressSpace.get();
    }
    void setPointeeAddressSpace( const PointeeAddressSpaceType& val)
    {
        m_PointeeAddressSpace.set(val);
    }
    bool isPointeeAddressSpaceHasValue() const
    {
        return m_PointeeAddressSpace.hasValue();
    }
        
    
    /// PointeeBufferIndex related methods
    PointeeBufferIndexType getPointeeBufferIndex() const
    {
        return m_PointeeBufferIndex.get();
    }
    void setPointeeBufferIndex( const PointeeBufferIndexType& val)
    {
        m_PointeeBufferIndex.set(val);
    }
    bool isPointeeBufferIndexHasValue() const
    {
        return m_PointeeBufferIndex.hasValue();
    }

    ///
    // Returns true if any of the PointerProgramBinaryInfoMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the PointerProgramBinaryInfoMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getPointerBufferIndexNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getPointerOffsetNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getPointeeAddressSpaceNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getPointeeBufferIndexNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<int32_t> m_PointerBufferIndex;        
    MetaDataValue<int32_t> m_PointerOffset;        
    MetaDataValue<int32_t> m_PointeeAddressSpace;        
    MetaDataValue<int32_t> m_PointeeBufferIndex;
    // parent node
    const llvm::MDNode* m_pNode;
};
            
///
// Read/Write the ResourceAlloc structure from/to LLVM metadata
//
class ResourceAllocMetaData:public IMetaDataObject
{
public:
    typedef ResourceAllocMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef NamedMetaDataValue<int32_t>::value_type UAVsNumType;        
    typedef NamedMetaDataValue<int32_t>::value_type SRVsNumType;        
    typedef NamedMetaDataValue<int32_t>::value_type SamplersNumType;        
    typedef MetaDataList<ArgAllocMetaDataHandle> ArgAllocsList;        
    typedef MetaDataList<InlineSamplerMetaDataHandle> InlineSamplersList;

public:
    ///
    // Factory method - creates the ResourceAllocMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty ResourceAllocMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named ResourceAllocMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the ResourceAllocMetaData from the given metadata node
    //
    ResourceAllocMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named ResourceAllocMetaData object
    //
    ResourceAllocMetaData();

    ///
    // Ctor - creates the empty, named ResourceAllocMetaData object
    //
    ResourceAllocMetaData(const char* name);

    /// UAVsNum related methods
    UAVsNumType getUAVsNum() const
    {
        return m_UAVsNum.get();
    }
    void setUAVsNum( const UAVsNumType& val)
    {
        m_UAVsNum.set(val);
    }
    bool isUAVsNumHasValue() const
    {
        return m_UAVsNum.hasValue();
    }
        
    
    /// SRVsNum related methods
    SRVsNumType getSRVsNum() const
    {
        return m_SRVsNum.get();
    }
    void setSRVsNum( const SRVsNumType& val)
    {
        m_SRVsNum.set(val);
    }
    bool isSRVsNumHasValue() const
    {
        return m_SRVsNum.hasValue();
    }
        
    
    /// SamplersNum related methods
    SamplersNumType getSamplersNum() const
    {
        return m_SamplersNum.get();
    }
    void setSamplersNum( const SamplersNumType& val)
    {
        m_SamplersNum.set(val);
    }
    bool isSamplersNumHasValue() const
    {
        return m_SamplersNum.hasValue();
    }
        
    
    /// ArgAllocs related methods
    ArgAllocsList::iterator begin_ArgAllocs()
    {
        return m_ArgAllocs.begin();
    }

    ArgAllocsList::iterator end_ArgAllocs()
    {
        return m_ArgAllocs.end();
    }
    ArgAllocsList::const_iterator begin_ArgAllocs() const
    {
        return m_ArgAllocs.begin();
    }

    ArgAllocsList::const_iterator end_ArgAllocs() const
    {
        return m_ArgAllocs.end();
    }

    size_t size_ArgAllocs()  const
    {
        return m_ArgAllocs.size();
    }

    bool empty_ArgAllocs()  const
    {
        return m_ArgAllocs.empty();
    }

    bool isArgAllocsHasValue() const
    {
        return m_ArgAllocs.hasValue();
    }
    
    ArgAllocsList::item_type getArgAllocsItem( size_t index ) const
    {
        return m_ArgAllocs.getItem(index);
    }
    void clearArgAllocs()
    {
        m_ArgAllocs.clear();
    }

    void setArgAllocsItem( size_t index, const ArgAllocsList::item_type& item  )
    {
        return m_ArgAllocs.setItem(index, item);
    }

    void addArgAllocsItem(const ArgAllocsList::item_type& val)
    {
        m_ArgAllocs.push_back(val);
    }

    ArgAllocsList::iterator eraseArgAllocsItem(ArgAllocsList::iterator i)
    {
        return m_ArgAllocs.erase(i);
    }
        
    
    /// InlineSamplers related methods
    InlineSamplersList::iterator begin_InlineSamplers()
    {
        return m_InlineSamplers.begin();
    }

    InlineSamplersList::iterator end_InlineSamplers()
    {
        return m_InlineSamplers.end();
    }
    InlineSamplersList::const_iterator begin_InlineSamplers() const
    {
        return m_InlineSamplers.begin();
    }

    InlineSamplersList::const_iterator end_InlineSamplers() const
    {
        return m_InlineSamplers.end();
    }

    size_t size_InlineSamplers()  const
    {
        return m_InlineSamplers.size();
    }

    bool empty_InlineSamplers()  const
    {
        return m_InlineSamplers.empty();
    }

    bool isInlineSamplersHasValue() const
    {
        return m_InlineSamplers.hasValue();
    }
    
    InlineSamplersList::item_type getInlineSamplersItem( size_t index ) const
    {
        return m_InlineSamplers.getItem(index);
    }
    void clearInlineSamplers()
    {
        m_InlineSamplers.clear();
    }

    void setInlineSamplersItem( size_t index, const InlineSamplersList::item_type& item  )
    {
        return m_InlineSamplers.setItem(index, item);
    }

    void addInlineSamplersItem(const InlineSamplersList::item_type& val)
    {
        m_InlineSamplers.push_back(val);
    }

    InlineSamplersList::iterator eraseInlineSamplersItem(InlineSamplersList::iterator i)
    {
        return m_InlineSamplers.erase(i);
    }

    ///
    // Returns true if any of the ResourceAllocMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the ResourceAllocMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getUAVsNumNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getSRVsNumNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getSamplersNumNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getArgAllocsNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getInlineSamplersNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    NamedMetaDataValue<int32_t> m_UAVsNum;        
    NamedMetaDataValue<int32_t> m_SRVsNum;        
    NamedMetaDataValue<int32_t> m_SamplersNum;        
    MetaDataList<ArgAllocMetaDataHandle> m_ArgAllocs;        
    MetaDataList<InlineSamplerMetaDataHandle> m_InlineSamplers;
    // parent node
    const llvm::MDNode* m_pNode;
};
            
///
// Read/Write the ThreadGroupSize structure from/to LLVM metadata
//
class ThreadGroupSizeMetaData:public IMetaDataObject
{
public:
    typedef ThreadGroupSizeMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef MetaDataValue<int32_t>::value_type XDimType;        
    typedef MetaDataValue<int32_t>::value_type YDimType;        
    typedef MetaDataValue<int32_t>::value_type ZDimType;

public:
    ///
    // Factory method - creates the ThreadGroupSizeMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty ThreadGroupSizeMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named ThreadGroupSizeMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the ThreadGroupSizeMetaData from the given metadata node
    //
    ThreadGroupSizeMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named ThreadGroupSizeMetaData object
    //
    ThreadGroupSizeMetaData();

    ///
    // Ctor - creates the empty, named ThreadGroupSizeMetaData object
    //
    ThreadGroupSizeMetaData(const char* name);

    /// XDim related methods
    XDimType getXDim() const
    {
        return m_XDim.get();
    }
    void setXDim( const XDimType& val)
    {
        m_XDim.set(val);
    }
    bool isXDimHasValue() const
    {
        return m_XDim.hasValue();
    }
        
    
    /// YDim related methods
    YDimType getYDim() const
    {
        return m_YDim.get();
    }
    void setYDim( const YDimType& val)
    {
        m_YDim.set(val);
    }
    bool isYDimHasValue() const
    {
        return m_YDim.hasValue();
    }
        
    
    /// ZDim related methods
    ZDimType getZDim() const
    {
        return m_ZDim.get();
    }
    void setZDim( const ZDimType& val)
    {
        m_ZDim.set(val);
    }
    bool isZDimHasValue() const
    {
        return m_ZDim.hasValue();
    }

    ///
    // Returns true if any of the ThreadGroupSizeMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the ThreadGroupSizeMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getXDimNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getYDimNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getZDimNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    MetaDataValue<int32_t> m_XDim;        
    MetaDataValue<int32_t> m_YDim;        
    MetaDataValue<int32_t> m_ZDim;
    // parent node
    const llvm::MDNode* m_pNode;
};
                                            
///
// Read/Write the FunctionInfo structure from/to LLVM metadata
//
class FunctionInfoMetaData:public IMetaDataObject
{
public:
    typedef FunctionInfoMetaData _Myt;
    typedef IMetaDataObject _Mybase;
    // typedefs for data member types
    typedef NamedMetaDataValue<int32_t>::value_type TypeType;        
    typedef MetaDataList<ArgInfoMetaDataHandle> ArgInfoListList;        
    typedef MetaDataList<ArgInfoMetaDataHandle> ImplicitArgInfoListList;        
        
        
        
    typedef NamedMetaDataValue<int32_t>::value_type LocalIDPresentType;        
    typedef NamedMetaDataValue<int32_t>::value_type GroupIDPresentType;        
    typedef NamedMetaDataValue<int32_t>::value_type GlobalOffsetPresentType;        
    typedef NamedMetaDataValue<int32_t>::value_type LocalSizeType;        
    typedef MetaDataList<LocalOffsetMetaDataHandle> LocalOffsetsList;        
        
    typedef NamedMetaDataValue<int32_t>::value_type PrivateMemoryPerWIType;        
        
    typedef MetaDataList<int32_t> OpenCLArgAddressSpacesList;        
    typedef MetaDataList<int32_t> BufferLocationIndexList;        
    typedef MetaDataList<int32_t> BufferLocationCountList;
    typedef MetaDataList<bool> IsEmulationArgumentList;
	typedef NamedMetaDataValue<int32_t>::value_type NeedBindlessHandleType;
    typedef MetaDataList<std::string> OpenCLArgAccessQualifiersList;        
    typedef MetaDataList<std::string> OpenCLArgTypesList;        
    typedef MetaDataList<std::string> OpenCLArgBaseTypesList;        
    typedef MetaDataList<std::string> OpenCLArgTypeQualifiersList;        
    typedef MetaDataList<std::string> OpenCLArgNamesList;

public:
    ///
    // Factory method - creates the FunctionInfoMetaData from the given metadata node
    //
    static _Myt* get(const llvm::MDNode* pNode, bool hasId = false)
    {
        return new _Myt(pNode, hasId);
    }

    ///
    // Factory method - create the default empty FunctionInfoMetaData object
    static _Myt* get()
    {
        return new _Myt();
    }

    ///
    // Factory method - create the default empty named FunctionInfoMetaData object
    static _Myt* get(const char* name)
    {
        return new _Myt(name);
    }

    ///
    // Ctor - loads the FunctionInfoMetaData from the given metadata node
    //
    FunctionInfoMetaData(const llvm::MDNode* pNode, bool hasId);

    ///
    // Default Ctor - creates the empty, not named FunctionInfoMetaData object
    //
    FunctionInfoMetaData();

    ///
    // Ctor - creates the empty, named FunctionInfoMetaData object
    //
    FunctionInfoMetaData(const char* name);

    /// Type related methods
    TypeType getType() const
    {
        return m_Type.get();
    }
    void setType( const TypeType& val)
    {
        m_Type.set(val);
    }
    bool isTypeHasValue() const
    {
        return m_Type.hasValue();
    }
        
    
    /// ArgInfoList related methods
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
    
    ArgInfoListList::item_type getArgInfoListItem( size_t index ) const
    {
        return m_ArgInfoList.getItem(index);
    }
    void clearArgInfoList()
    {
        m_ArgInfoList.clear();
    }

    void setArgInfoListItem( size_t index, const ArgInfoListList::item_type& item  )
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
        
    
    /// ImplicitArgInfoList related methods
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
    
    ImplicitArgInfoListList::item_type getImplicitArgInfoListItem( size_t index ) const
    {
        return m_ImplicitArgInfoList.getItem(index);
    }
    void clearImplicitArgInfoList()
    {
        m_ImplicitArgInfoList.clear();
    }

    void setImplicitArgInfoListItem( size_t index, const ImplicitArgInfoListList::item_type& item  )
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
        
    
    /// ThreadGroupSize related methods
    
    ThreadGroupSizeMetaDataHandle getThreadGroupSize()
    {
        return m_ThreadGroupSize;
    }
        
    
    /// ThreadGroupSizeHint related methods
    
    ThreadGroupSizeMetaDataHandle getThreadGroupSizeHint()
    {
        return m_ThreadGroupSizeHint;
    }
        
    
    /// SubGroupSize related methods
    
    SubGroupSizeMetaDataHandle getSubGroupSize()
    {
        return m_SubGroupSize;
    }

	/// SubGroupSize related methods

	WorkgroupWalkOrderMetaDataHandle getWorkgroupWalkOrder()
	{
		return m_WorkgroupWalkOrder;
	}
        
    
    /// LocalIDPresent related methods
    LocalIDPresentType getLocalIDPresent() const
    {
        return m_LocalIDPresent.get();
    }
    void setLocalIDPresent( const LocalIDPresentType& val)
    {
        m_LocalIDPresent.set(val);
    }
    bool isLocalIDPresentHasValue() const
    {
        return m_LocalIDPresent.hasValue();
    }
        
    
    /// GroupIDPresent related methods
    GroupIDPresentType getGroupIDPresent() const
    {
        return m_GroupIDPresent.get();
    }
    void setGroupIDPresent( const GroupIDPresentType& val)
    {
        m_GroupIDPresent.set(val);
    }
    bool isGroupIDPresentHasValue() const
    {
        return m_GroupIDPresent.hasValue();
    }
        
    
    /// GlobalOffsetPresent related methods
    GlobalOffsetPresentType getGlobalOffsetPresent() const
    {
        return m_GlobalOffsetPresent.get();
    }
    void setGlobalOffsetPresent( const GlobalOffsetPresentType& val)
    {
        m_GlobalOffsetPresent.set(val);
    }
    bool isGlobalOffsetPresentHasValue() const
    {
        return m_GlobalOffsetPresent.hasValue();
    }
        
    
    /// LocalSize related methods
    LocalSizeType getLocalSize() const
    {
        return m_LocalSize.get();
    }
    void setLocalSize( const LocalSizeType& val)
    {
        m_LocalSize.set(val);
    }
    bool isLocalSizeHasValue() const
    {
        return m_LocalSize.hasValue();
    }
        
    
    /// LocalOffsets related methods
    LocalOffsetsList::iterator begin_LocalOffsets()
    {
        return m_LocalOffsets.begin();
    }

    LocalOffsetsList::iterator end_LocalOffsets()
    {
        return m_LocalOffsets.end();
    }
    LocalOffsetsList::const_iterator begin_LocalOffsets() const
    {
        return m_LocalOffsets.begin();
    }

    LocalOffsetsList::const_iterator end_LocalOffsets() const
    {
        return m_LocalOffsets.end();
    }

    size_t size_LocalOffsets()  const
    {
        return m_LocalOffsets.size();
    }

    bool empty_LocalOffsets()  const
    {
        return m_LocalOffsets.empty();
    }

    bool isLocalOffsetsHasValue() const
    {
        return m_LocalOffsets.hasValue();
    }
    
    LocalOffsetsList::item_type getLocalOffsetsItem( size_t index ) const
    {
        return m_LocalOffsets.getItem(index);
    }
    void clearLocalOffsets()
    {
        m_LocalOffsets.clear();
    }

    void setLocalOffsetsItem( size_t index, const LocalOffsetsList::item_type& item  )
    {
        return m_LocalOffsets.setItem(index, item);
    }

    void addLocalOffsetsItem(const LocalOffsetsList::item_type& val)
    {
        m_LocalOffsets.push_back(val);
    }

    LocalOffsetsList::iterator eraseLocalOffsetsItem(LocalOffsetsList::iterator i)
    {
        return m_LocalOffsets.erase(i);
    }
        
    
    /// ResourceAlloc related methods
    
    ResourceAllocMetaDataHandle getResourceAlloc()
    {
        return m_ResourceAlloc;
    }
        
    
    /// PrivateMemoryPerWI related methods
    PrivateMemoryPerWIType getPrivateMemoryPerWI() const
    {
        return m_PrivateMemoryPerWI.get();
    }
    void setPrivateMemoryPerWI( const PrivateMemoryPerWIType& val)
    {
        m_PrivateMemoryPerWI.set(val);
    }
    bool isPrivateMemoryPerWIHasValue() const
    {
        return m_PrivateMemoryPerWI.hasValue();
    }
        
    
    /// OpenCLVectorTypeHint related methods
    
    VectorTypeHintMetaDataHandle getOpenCLVectorTypeHint()
    {
        return m_OpenCLVectorTypeHint;
    }
        
    
    /// OpenCLArgAddressSpaces related methods
    OpenCLArgAddressSpacesList::iterator begin_OpenCLArgAddressSpaces()
    {
        return m_OpenCLArgAddressSpaces.begin();
    }

    OpenCLArgAddressSpacesList::iterator end_OpenCLArgAddressSpaces()
    {
        return m_OpenCLArgAddressSpaces.end();
    }
    OpenCLArgAddressSpacesList::const_iterator begin_OpenCLArgAddressSpaces() const
    {
        return m_OpenCLArgAddressSpaces.begin();
    }

    OpenCLArgAddressSpacesList::const_iterator end_OpenCLArgAddressSpaces() const
    {
        return m_OpenCLArgAddressSpaces.end();
    }

    size_t size_OpenCLArgAddressSpaces()  const
    {
        return m_OpenCLArgAddressSpaces.size();
    }

    bool empty_OpenCLArgAddressSpaces()  const
    {
        return m_OpenCLArgAddressSpaces.empty();
    }

    bool isOpenCLArgAddressSpacesHasValue() const
    {
        return m_OpenCLArgAddressSpaces.hasValue();
    }
    
    OpenCLArgAddressSpacesList::item_type getOpenCLArgAddressSpacesItem( size_t index ) const
    {
        return m_OpenCLArgAddressSpaces.getItem(index);
    }
    void clearOpenCLArgAddressSpaces()
    {
        m_OpenCLArgAddressSpaces.clear();
    }

    void setOpenCLArgAddressSpacesItem( size_t index, const OpenCLArgAddressSpacesList::item_type& item  )
    {
        return m_OpenCLArgAddressSpaces.setItem(index, item);
    }

    void addOpenCLArgAddressSpacesItem(const OpenCLArgAddressSpacesList::item_type& val)
    {
        m_OpenCLArgAddressSpaces.push_back(val);
    }

    OpenCLArgAddressSpacesList::iterator eraseOpenCLArgAddressSpacesItem(OpenCLArgAddressSpacesList::iterator i)
    {
        return m_OpenCLArgAddressSpaces.erase(i);
    }
        
    
    /// BufferLocationIndex related methods
    BufferLocationIndexList::iterator begin_BufferLocationIndex()
    {
        return m_BufferLocationIndex.begin();
    }

    BufferLocationIndexList::iterator end_BufferLocationIndex()
    {
        return m_BufferLocationIndex.end();
    }
    BufferLocationIndexList::const_iterator begin_BufferLocationIndex() const
    {
        return m_BufferLocationIndex.begin();
    }

    BufferLocationIndexList::const_iterator end_BufferLocationIndex() const
    {
        return m_BufferLocationIndex.end();
    }

    size_t size_BufferLocationIndex()  const
    {
        return m_BufferLocationIndex.size();
    }

    bool empty_BufferLocationIndex()  const
    {
        return m_BufferLocationIndex.empty();
    }

    bool isBufferLocationIndexHasValue() const
    {
        return m_BufferLocationIndex.hasValue();
    }
    
    BufferLocationIndexList::item_type getBufferLocationIndexItem( size_t index ) const
    {
        return m_BufferLocationIndex.getItem(index);
    }
    void clearBufferLocationIndex()
    {
        m_BufferLocationIndex.clear();
    }

    void setBufferLocationIndexItem( size_t index, const BufferLocationIndexList::item_type& item  )
    {
        return m_BufferLocationIndex.setItem(index, item);
    }

    void addBufferLocationIndexItem(const BufferLocationIndexList::item_type& val)
    {
        m_BufferLocationIndex.push_back(val);
    }

    BufferLocationIndexList::iterator eraseBufferLocationIndexItem(BufferLocationIndexList::iterator i)
    {
        return m_BufferLocationIndex.erase(i);
    }
        
    
    /// BufferLocationCount related methods
    BufferLocationCountList::iterator begin_BufferLocationCount()
    {
        return m_BufferLocationCount.begin();
    }

    BufferLocationCountList::iterator end_BufferLocationCount()
    {
        return m_BufferLocationCount.end();
    }
    BufferLocationCountList::const_iterator begin_BufferLocationCount() const
    {
        return m_BufferLocationCount.begin();
    }

    BufferLocationCountList::const_iterator end_BufferLocationCount() const
    {
        return m_BufferLocationCount.end();
    }

    size_t size_BufferLocationCount()  const
    {
        return m_BufferLocationCount.size();
    }

    bool empty_BufferLocationCount()  const
    {
        return m_BufferLocationCount.empty();
    }

    bool isBufferLocationCountHasValue() const
    {
        return m_BufferLocationCount.hasValue();
    }
    
    BufferLocationCountList::item_type getBufferLocationCountItem( size_t index ) const
    {
        return m_BufferLocationCount.getItem(index);
    }
    void clearBufferLocationCount()
    {
        m_BufferLocationCount.clear();
    }

    void setBufferLocationCountItem( size_t index, const BufferLocationCountList::item_type& item  )
    {
        return m_BufferLocationCount.setItem(index, item);
    }

    void addBufferLocationCountItem(const BufferLocationCountList::item_type& val)
    {
        m_BufferLocationCount.push_back(val);
    }

    BufferLocationCountList::iterator eraseBufferLocationCountItem(BufferLocationCountList::iterator i)
    {
        return m_BufferLocationCount.erase(i);
    }

    /// IsEmulationArgument related methods
    IsEmulationArgumentList::iterator begin_IsEmulationArgument()
    {
        return m_IsEmulationArgument.begin();
    }

    IsEmulationArgumentList::iterator end_IsEmulationArgument()
    {
        return m_IsEmulationArgument.end();
    }
	
    IsEmulationArgumentList::const_iterator begin_IsEmulationArgument() const
    {
        return m_IsEmulationArgument.begin();
    }

    IsEmulationArgumentList::const_iterator end_IsEmulationArgument() const
    {
        return m_IsEmulationArgument.end();
    }

    size_t size_IsEmulationArgument()  const
    {
        return m_IsEmulationArgument.size();
    }

    bool empty_IsEmulationArgument()  const
    {
        return m_IsEmulationArgument.empty();
    }

    bool isIsEmulationArgumentHasValue() const
    {
        return m_IsEmulationArgument.hasValue();
    }

    IsEmulationArgumentList::item_type getIsEmulationArgumentItem(size_t index) const
    {
        return m_IsEmulationArgument.getItem(index);
    }
	
    void clearIsEmulationArgument()
    {
        m_IsEmulationArgument.clear();
    }

    void setIsEmulationArgumentItem(size_t index, const IsEmulationArgumentList::item_type& item)
    {
        return m_IsEmulationArgument.setItem(index, item);
    }

    void addIsEmulationArgumentItem(const IsEmulationArgumentList::item_type& val)
    {
        m_IsEmulationArgument.push_back(val);
    }

    IsEmulationArgumentList::iterator eraseIsEmulationArgumentItem(IsEmulationArgumentList::iterator i)
    {
        return m_IsEmulationArgument.erase(i);
    }

           
    /// OpenCLArgAccessQualifiers related methods
    OpenCLArgAccessQualifiersList::iterator begin_OpenCLArgAccessQualifiers()
    {
        return m_OpenCLArgAccessQualifiers.begin();
    }

    OpenCLArgAccessQualifiersList::iterator end_OpenCLArgAccessQualifiers()
    {
        return m_OpenCLArgAccessQualifiers.end();
    }
    OpenCLArgAccessQualifiersList::const_iterator begin_OpenCLArgAccessQualifiers() const
    {
        return m_OpenCLArgAccessQualifiers.begin();
    }

    OpenCLArgAccessQualifiersList::const_iterator end_OpenCLArgAccessQualifiers() const
    {
        return m_OpenCLArgAccessQualifiers.end();
    }

    size_t size_OpenCLArgAccessQualifiers()  const
    {
        return m_OpenCLArgAccessQualifiers.size();
    }

    bool empty_OpenCLArgAccessQualifiers()  const
    {
        return m_OpenCLArgAccessQualifiers.empty();
    }

    bool isOpenCLArgAccessQualifiersHasValue() const
    {
        return m_OpenCLArgAccessQualifiers.hasValue();
    }
    
    OpenCLArgAccessQualifiersList::item_type getOpenCLArgAccessQualifiersItem( size_t index ) const
    {
        return m_OpenCLArgAccessQualifiers.getItem(index);
    }
    void clearOpenCLArgAccessQualifiers()
    {
        m_OpenCLArgAccessQualifiers.clear();
    }

    void setOpenCLArgAccessQualifiersItem( size_t index, const OpenCLArgAccessQualifiersList::item_type& item  )
    {
        return m_OpenCLArgAccessQualifiers.setItem(index, item);
    }

    void addOpenCLArgAccessQualifiersItem(const OpenCLArgAccessQualifiersList::item_type& val)
    {
        m_OpenCLArgAccessQualifiers.push_back(val);
    }

    OpenCLArgAccessQualifiersList::iterator eraseOpenCLArgAccessQualifiersItem(OpenCLArgAccessQualifiersList::iterator i)
    {
        return m_OpenCLArgAccessQualifiers.erase(i);
    }
        
    
    /// OpenCLArgTypes related methods
    OpenCLArgTypesList::iterator begin_OpenCLArgTypes()
    {
        return m_OpenCLArgTypes.begin();
    }

    OpenCLArgTypesList::iterator end_OpenCLArgTypes()
    {
        return m_OpenCLArgTypes.end();
    }
    OpenCLArgTypesList::const_iterator begin_OpenCLArgTypes() const
    {
        return m_OpenCLArgTypes.begin();
    }

    OpenCLArgTypesList::const_iterator end_OpenCLArgTypes() const
    {
        return m_OpenCLArgTypes.end();
    }

    size_t size_OpenCLArgTypes()  const
    {
        return m_OpenCLArgTypes.size();
    }

    bool empty_OpenCLArgTypes()  const
    {
        return m_OpenCLArgTypes.empty();
    }

    bool isOpenCLArgTypesHasValue() const
    {
        return m_OpenCLArgTypes.hasValue();
    }
    
    OpenCLArgTypesList::item_type getOpenCLArgTypesItem( size_t index ) const
    {
        return m_OpenCLArgTypes.getItem(index);
    }
    void clearOpenCLArgTypes()
    {
        m_OpenCLArgTypes.clear();
    }

    void setOpenCLArgTypesItem( size_t index, const OpenCLArgTypesList::item_type& item  )
    {
        return m_OpenCLArgTypes.setItem(index, item);
    }

    void addOpenCLArgTypesItem(const OpenCLArgTypesList::item_type& val)
    {
        m_OpenCLArgTypes.push_back(val);
    }

    OpenCLArgTypesList::iterator eraseOpenCLArgTypesItem(OpenCLArgTypesList::iterator i)
    {
        return m_OpenCLArgTypes.erase(i);
    }
        
    
    /// OpenCLArgBaseTypes related methods
    OpenCLArgBaseTypesList::iterator begin_OpenCLArgBaseTypes()
    {
        return m_OpenCLArgBaseTypes.begin();
    }

    OpenCLArgBaseTypesList::iterator end_OpenCLArgBaseTypes()
    {
        return m_OpenCLArgBaseTypes.end();
    }
    OpenCLArgBaseTypesList::const_iterator begin_OpenCLArgBaseTypes() const
    {
        return m_OpenCLArgBaseTypes.begin();
    }

    OpenCLArgBaseTypesList::const_iterator end_OpenCLArgBaseTypes() const
    {
        return m_OpenCLArgBaseTypes.end();
    }

    size_t size_OpenCLArgBaseTypes()  const
    {
        return m_OpenCLArgBaseTypes.size();
    }

    bool empty_OpenCLArgBaseTypes()  const
    {
        return m_OpenCLArgBaseTypes.empty();
    }

    bool isOpenCLArgBaseTypesHasValue() const
    {
        return m_OpenCLArgBaseTypes.hasValue();
    }
    
    OpenCLArgBaseTypesList::item_type getOpenCLArgBaseTypesItem( size_t index ) const
    {
        return m_OpenCLArgBaseTypes.getItem(index);
    }
    void clearOpenCLArgBaseTypes()
    {
        m_OpenCLArgBaseTypes.clear();
    }

    void setOpenCLArgBaseTypesItem( size_t index, const OpenCLArgBaseTypesList::item_type& item  )
    {
        return m_OpenCLArgBaseTypes.setItem(index, item);
    }

    void addOpenCLArgBaseTypesItem(const OpenCLArgBaseTypesList::item_type& val)
    {
        m_OpenCLArgBaseTypes.push_back(val);
    }

    OpenCLArgBaseTypesList::iterator eraseOpenCLArgBaseTypesItem(OpenCLArgBaseTypesList::iterator i)
    {
        return m_OpenCLArgBaseTypes.erase(i);
    }
        
    
    /// OpenCLArgTypeQualifiers related methods
    OpenCLArgTypeQualifiersList::iterator begin_OpenCLArgTypeQualifiers()
    {
        return m_OpenCLArgTypeQualifiers.begin();
    }

    OpenCLArgTypeQualifiersList::iterator end_OpenCLArgTypeQualifiers()
    {
        return m_OpenCLArgTypeQualifiers.end();
    }
    OpenCLArgTypeQualifiersList::const_iterator begin_OpenCLArgTypeQualifiers() const
    {
        return m_OpenCLArgTypeQualifiers.begin();
    }

    OpenCLArgTypeQualifiersList::const_iterator end_OpenCLArgTypeQualifiers() const
    {
        return m_OpenCLArgTypeQualifiers.end();
    }

    size_t size_OpenCLArgTypeQualifiers()  const
    {
        return m_OpenCLArgTypeQualifiers.size();
    }

    bool empty_OpenCLArgTypeQualifiers()  const
    {
        return m_OpenCLArgTypeQualifiers.empty();
    }

    bool isOpenCLArgTypeQualifiersHasValue() const
    {
        return m_OpenCLArgTypeQualifiers.hasValue();
    }
    
    OpenCLArgTypeQualifiersList::item_type getOpenCLArgTypeQualifiersItem( size_t index ) const
    {
        return m_OpenCLArgTypeQualifiers.getItem(index);
    }
    void clearOpenCLArgTypeQualifiers()
    {
        m_OpenCLArgTypeQualifiers.clear();
    }

    void setOpenCLArgTypeQualifiersItem( size_t index, const OpenCLArgTypeQualifiersList::item_type& item  )
    {
        return m_OpenCLArgTypeQualifiers.setItem(index, item);
    }

    void addOpenCLArgTypeQualifiersItem(const OpenCLArgTypeQualifiersList::item_type& val)
    {
        m_OpenCLArgTypeQualifiers.push_back(val);
    }

    OpenCLArgTypeQualifiersList::iterator eraseOpenCLArgTypeQualifiersItem(OpenCLArgTypeQualifiersList::iterator i)
    {
        return m_OpenCLArgTypeQualifiers.erase(i);
    }
        
    
    /// OpenCLArgNames related methods
    OpenCLArgNamesList::iterator begin_OpenCLArgNames()
    {
        return m_OpenCLArgNames.begin();
    }

    OpenCLArgNamesList::iterator end_OpenCLArgNames()
    {
        return m_OpenCLArgNames.end();
    }
    OpenCLArgNamesList::const_iterator begin_OpenCLArgNames() const
    {
        return m_OpenCLArgNames.begin();
    }

    OpenCLArgNamesList::const_iterator end_OpenCLArgNames() const
    {
        return m_OpenCLArgNames.end();
    }

    size_t size_OpenCLArgNames()  const
    {
        return m_OpenCLArgNames.size();
    }

    bool empty_OpenCLArgNames()  const
    {
        return m_OpenCLArgNames.empty();
    }

    bool isOpenCLArgNamesHasValue() const
    {
        return m_OpenCLArgNames.hasValue();
    }
    
    OpenCLArgNamesList::item_type getOpenCLArgNamesItem( size_t index ) const
    {
        return m_OpenCLArgNames.getItem(index);
    }
    void clearOpenCLArgNames()
    {
        m_OpenCLArgNames.clear();
    }

    void setOpenCLArgNamesItem( size_t index, const OpenCLArgNamesList::item_type& item  )
    {
        return m_OpenCLArgNames.setItem(index, item);
    }

    void addOpenCLArgNamesItem(const OpenCLArgNamesList::item_type& val)
    {
        m_OpenCLArgNames.push_back(val);
    }

    OpenCLArgNamesList::iterator eraseOpenCLArgNamesItem(OpenCLArgNamesList::iterator i)
    {
        return m_OpenCLArgNames.erase(i);
    }

    ///
    // Returns true if any of the FunctionInfoMetaData`s members has changed
    bool dirty() const;

    ///
    // Returns true if the structure was loaded from the metadata or was changed
    bool hasValue() const;

    ///
    // Discards the changes done to the FunctionInfoMetaData instance
    void discardChanges();

    ///
    // Generates the new MDNode hierarchy for the given structure
    llvm::Metadata* generateNode(llvm::LLVMContext& context) const;

    ///
    // Saves the structure changes to the given MDNode
    void save(llvm::LLVMContext& context, llvm::MDNode* pNode) const;

private:
    ///
    // Returns true if the given MDNode could be saved to without replacement
    bool compatibleWith( const llvm::MDNode* pNode) const
    {
        return false;
    }

private:
    typedef MetaDataIterator<llvm::MDNode> NodeIterator;

    llvm::Metadata* getTypeNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getArgInfoListNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getImplicitArgInfoListNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getThreadGroupSizeNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getThreadGroupSizeHintNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getSubGroupSizeNode( const llvm::MDNode* pParentNode) const;    
	llvm::MDNode* getWorkgroupWalkOrderNode(const llvm::MDNode* pParentNode) const;
    llvm::Metadata* getLocalIDPresentNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getGroupIDPresentNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getGlobalOffsetPresentNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getLocalSizeNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getLocalOffsetsNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getResourceAllocNode( const llvm::MDNode* pParentNode) const;    
    llvm::Metadata* getPrivateMemoryPerWINode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getOpenCLVectorTypeHintNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getOpenCLArgAddressSpacesNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getBufferLocationIndexNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getBufferLocationCountNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getIsEmulationArgumentNode(const llvm::MDNode* pParentNode) const;
    llvm::MDNode* getOpenCLArgAccessQualifiersNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getOpenCLArgTypesNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getOpenCLArgBaseTypesNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getOpenCLArgTypeQualifiersNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getOpenCLArgNamesNode( const llvm::MDNode* pParentNode) const;

private:
    // data members
    NamedMetaDataValue<int32_t> m_Type;        
    MetaDataList<ArgInfoMetaDataHandle> m_ArgInfoList;        
    MetaDataList<ArgInfoMetaDataHandle> m_ImplicitArgInfoList;        
    ThreadGroupSizeMetaDataHandle m_ThreadGroupSize;        
    ThreadGroupSizeMetaDataHandle m_ThreadGroupSizeHint;        
    SubGroupSizeMetaDataHandle m_SubGroupSize;        
	WorkgroupWalkOrderMetaDataHandle m_WorkgroupWalkOrder;
    NamedMetaDataValue<int32_t> m_LocalIDPresent;        
    NamedMetaDataValue<int32_t> m_GroupIDPresent;        
    NamedMetaDataValue<int32_t> m_GlobalOffsetPresent;        
    NamedMetaDataValue<int32_t> m_LocalSize;        
    MetaDataList<LocalOffsetMetaDataHandle> m_LocalOffsets;        
    ResourceAllocMetaDataHandle m_ResourceAlloc;        
    NamedMetaDataValue<int32_t> m_PrivateMemoryPerWI;        
    VectorTypeHintMetaDataHandle m_OpenCLVectorTypeHint;        
    MetaDataList<int32_t> m_OpenCLArgAddressSpaces;        
    MetaDataList<int32_t> m_BufferLocationIndex;        
    MetaDataList<int32_t> m_BufferLocationCount;
    MetaDataList<bool> m_IsEmulationArgument;
    MetaDataList<std::string> m_OpenCLArgAccessQualifiers;        
    MetaDataList<std::string> m_OpenCLArgTypes;        
    MetaDataList<std::string> m_OpenCLArgBaseTypes;        
    MetaDataList<std::string> m_OpenCLArgTypeQualifiers;        
    MetaDataList<std::string> m_OpenCLArgNames;
    // parent node
    const llvm::MDNode* m_pNode;
};
                                                                                    
class MetaDataUtils
{
public:
    // typedefs for the data members types        
    typedef NamedMDNodeList<InputIRVersionMetaDataHandle> InputIRVersionsList;              
    typedef NamedMetaDataMap<llvm::Function, FunctionInfoMetaDataHandle> FunctionsInfoMap;     

public:
    // If using this constructor, setting the llvm module by the setModule 
    // function is needed for correct operation.
    MetaDataUtils(){}



    MetaDataUtils(llvm::Module* pModule):
        m_InputIRVersions(pModule->getOrInsertNamedMetadata("igc.input.ir")),
        m_FunctionsInfo(pModule->getOrInsertNamedMetadata("igc.functions")),    
        m_pModule(pModule)
    {
    }

    void setModule(llvm::Module* pModule) {
        m_InputIRVersions = pModule->getOrInsertNamedMetadata("igc.input.ir");
        m_FunctionsInfo = pModule->getOrInsertNamedMetadata("igc.functions");    
        m_pModule = pModule;
    }

    ~MetaDataUtils(){}

    /// InputIRVersions related methods
    void clearInputIRVersions()
    {
        m_InputIRVersions.clear();
    }

    void deleteInputIRVersions()
    {
        llvm::NamedMDNode* InputIRVersionsNode = m_pModule->getNamedMetadata("igc.input.ir");
        if (InputIRVersionsNode)
        {
            m_nodesToDelete.push_back(InputIRVersionsNode);
        }
    }

    InputIRVersionsList::iterator begin_InputIRVersions()
    {
        return m_InputIRVersions.begin();
    }
    InputIRVersionsList::iterator end_InputIRVersions()
    {
        return m_InputIRVersions.end();
    }

    InputIRVersionsList::const_iterator begin_InputIRVersions() const
    {
        return m_InputIRVersions.begin();
    }

    InputIRVersionsList::const_iterator end_InputIRVersions() const
    {
        return m_InputIRVersions.end();
    }

    size_t size_InputIRVersions()  const
    {
        return m_InputIRVersions.size();
    }

    bool empty_InputIRVersions()  const
    {
        return m_InputIRVersions.empty();
    }

    bool isInputIRVersionsHasValue() const
    {
        return m_InputIRVersions.hasValue();
    }
     
    InputIRVersionsList::item_type getInputIRVersionsItem( size_t index ) const
    {
        return m_InputIRVersions.getItem(index);
    }
    void setInputIRVersionsItem( size_t index, const InputIRVersionsList::item_type& item  )
    {
        return m_InputIRVersions.setItem(index, item);
    }

    void addInputIRVersionsItem(const InputIRVersionsList::item_type& val)
    {
        m_InputIRVersions.push_back(val);
    }

    InputIRVersionsList::iterator eraseInputIRVersionsItem(InputIRVersionsList::iterator it)
    {
        return m_InputIRVersions.erase(it);
    }

    /// FunctionsInfo related methods
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

    size_t size_FunctionsInfo()  const
    {
        return m_FunctionsInfo.size();
    }

    bool empty_FunctionsInfo()  const
    {
        return m_FunctionsInfo.empty();
    }

    bool isFunctionsInfoHasValue() const
    {
        return m_FunctionsInfo.hasValue();
    }

    
    FunctionsInfoMap::item_type getFunctionsInfoItem( const FunctionsInfoMap::key_type& index ) const
    {
        return m_FunctionsInfo.getItem(index);
    }

    FunctionsInfoMap::item_type getOrInsertFunctionsInfoItem( const FunctionsInfoMap::key_type& index )
    {
        return m_FunctionsInfo.getOrInsertItem(index);
    }

    void setFunctionsInfoItem( const FunctionsInfoMap::key_type& index, const FunctionsInfoMap::item_type& item  )
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
    
    bool dirty()
    {
        if (m_InputIRVersions.dirty())
        {
            return true;
        }
        if( m_FunctionsInfo.dirty() )
        {
            return true;
        }
        return false;
    }

    void save(llvm::LLVMContext& context)
    {
		if (m_InputIRVersions.dirty())
		{
			llvm::NamedMDNode* pNode = m_pModule->getOrInsertNamedMetadata("igc.input.ir");
			m_InputIRVersions.save(context, pNode);
		}
        if( m_FunctionsInfo.dirty() )
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
		m_InputIRVersions.discardChanges();
        m_FunctionsInfo.discardChanges();
        m_nodesToDelete.clear();
    }

    void deleteMetadata()
    {
		llvm::NamedMDNode* InputIRVersionsNode = m_pModule->getNamedMetadata("igc.input.ir");
		if (InputIRVersionsNode)
		{
			m_nodesToDelete.push_back(InputIRVersionsNode);
		}

        llvm::NamedMDNode* FunctionsInfoNode = m_pModule->getNamedMetadata("igc.functions");
        if (FunctionsInfoNode)
        {
            m_nodesToDelete.push_back(FunctionsInfoNode);
        }
    }

private:
    // data members
	NamedMDNodeList<InputIRVersionMetaDataHandle> m_InputIRVersions;
    NamedMetaDataMap<llvm::Function, FunctionInfoMetaDataHandle> m_FunctionsInfo;
    llvm::Module* m_pModule;
    std::vector<llvm::NamedMDNode*> m_nodesToDelete;
};


} } //namespace
