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
#include "MDFrameWork.h"

namespace IGC { namespace IGCMD
{
//typedefs and forward declarations                   
class ArgInfoMetaData;
typedef MetaObjectHandle<ArgInfoMetaData> ArgInfoMetaDataHandle; 
            
class SubGroupSizeMetaData;
typedef MetaObjectHandle<SubGroupSizeMetaData> SubGroupSizeMetaDataHandle; 
            
class VectorTypeHintMetaData;
typedef MetaObjectHandle<VectorTypeHintMetaData> VectorTypeHintMetaDataHandle; 
                    
class PointerProgramBinaryInfoMetaData;
typedef MetaObjectHandle<PointerProgramBinaryInfoMetaData> PointerProgramBinaryInfoMetaDataHandle; 
            
class ThreadGroupSizeMetaData;
typedef MetaObjectHandle<ThreadGroupSizeMetaData> ThreadGroupSizeMetaDataHandle; 
                                            
class FunctionInfoMetaData;
typedef MetaObjectHandle<FunctionInfoMetaData> FunctionInfoMetaDataHandle;                                                                           
                                
typedef MetaDataList<ArgInfoMetaDataHandle> ArgInfoListMetaDataList;
typedef MetaObjectHandle<ArgInfoListMetaDataList> ArgInfoListMetaDataListHandle; 
                                
typedef MetaDataList<PointerProgramBinaryInfoMetaDataHandle> PointerProgramBinaryInfosMetaDataList;
typedef MetaObjectHandle<PointerProgramBinaryInfosMetaDataList> PointerProgramBinaryInfosMetaDataListHandle; 
                                
typedef MetaDataList<int8_t> InlineBufferMetaDataList;
typedef MetaObjectHandle<InlineBufferMetaDataList> InlineBufferMetaDataListHandle; 

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
    typedef NamedMetaDataValue<int32_t>::value_type PrivateMemoryPerWIType;        
        
    typedef NamedMetaDataValue<int32_t>::value_type NeedBindlessHandleType;


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

    /// OpenCLVectorTypeHint related methods
    
    VectorTypeHintMetaDataHandle getOpenCLVectorTypeHint()
    {
        return m_OpenCLVectorTypeHint;
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
    llvm::Metadata* getLocalSizeNode( const llvm::MDNode* pParentNode) const;    
    llvm::MDNode* getOpenCLVectorTypeHintNode( const llvm::MDNode* pParentNode) const;    

private:
    // data members
    NamedMetaDataValue<int32_t> m_Type;        
    MetaDataList<ArgInfoMetaDataHandle> m_ArgInfoList;        
    MetaDataList<ArgInfoMetaDataHandle> m_ImplicitArgInfoList;        
    ThreadGroupSizeMetaDataHandle m_ThreadGroupSize;        
    ThreadGroupSizeMetaDataHandle m_ThreadGroupSizeHint;        
    SubGroupSizeMetaDataHandle m_SubGroupSize;        

    VectorTypeHintMetaDataHandle m_OpenCLVectorTypeHint;        
    // parent node
    const llvm::MDNode* m_pNode;
};
                                                                                    
class MetaDataUtils
{
public:
    // typedefs for the data members types                   
    typedef NamedMetaDataMap<llvm::Function, FunctionInfoMetaDataHandle> FunctionsInfoMap;     

public:
    // If using this constructor, setting the llvm module by the setModule 
    // function is needed for correct operation.
    MetaDataUtils(){}
    MetaDataUtils(llvm::Module* pModule):
        m_FunctionsInfo(pModule->getOrInsertNamedMetadata("igc.functions")),    
        m_pModule(pModule)
    {
    }

    void setModule(llvm::Module* pModule) {
        m_FunctionsInfo = pModule->getOrInsertNamedMetadata("igc.functions");    
        m_pModule = pModule;
    }

    ~MetaDataUtils(){}

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

    void save(llvm::LLVMContext& context)
    {
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
    llvm::Module* m_pModule;
    std::vector<llvm::NamedMDNode*> m_nodesToDelete;
};


} } //namespace
