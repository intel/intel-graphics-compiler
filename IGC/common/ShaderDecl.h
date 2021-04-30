/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "usc_config.h"
#include "usc.h"
#include "ShaderTypesEnum.h"
#include "ShaderToken.h"
#include "ShaderTypes.h"
#include "SurfaceFormats.h"

namespace IGC
{
    class CAllocator;
}

namespace USC
{
    struct SIndexedTempDeclType;
}

namespace iSTD
{
    template<typename Type, class CAllocatorType>
    class CArray;
}

namespace USC
{

/*****************************************************************************\

Class:
    CShaderDecl

Description:

\*****************************************************************************/
class USC_API CShaderDecl
{
public:

    static ErrorCode   Create( CShaderDecl*& pShaderDecl );
    static void     Delete( CShaderDecl*& pShaderDecl );

    virtual void    Declare( const unsigned int number );
    virtual bool    IsDeclared( const unsigned int number ) const;
    virtual void    UnDeclare( const unsigned int number );
    virtual unsigned int   GetCount( void ) const;

    unsigned int    GetMaxNumber( void ) const { return m_MaxNumber; };
    unsigned int    GetMinNumber( void ) const { return m_MinNumber; };


    unsigned int    GetNextUndeclared( const unsigned int nextUndeclared ) const;

    virtual void    Compress( void );

    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );

protected:
    CShaderDecl( );
    virtual ~CShaderDecl( void );

    virtual ErrorCode  Initialize( void );

    template<typename TOpType>
    ErrorCode  ValidateOperation( const CShaderDecl& src, unsigned char*& pStream );


    unsigned int   m_MaxNumber;
    unsigned int   m_MinNumber;
};

/*****************************************************************************\

Class:
    CShaderFiniteDecl

Description:

\*****************************************************************************/
class USC_API CShaderFiniteDecl :
    public CShaderDecl
{
public:
    static ErrorCode   Create(
                        const unsigned int maxRegisters,
                        CShaderFiniteDecl*& pShaderDecl );
    static void     Delete( CShaderFiniteDecl*& pShaderDecl );

    virtual void    Declare( const unsigned int number );
    virtual bool    IsDeclared( const unsigned int number ) const;
    virtual void    UnDeclare( const unsigned int number );
    virtual unsigned int   GetCount( void ) const;
    unsigned int    GetMaxRegisters( void ) const;

    void            GetListOfDeclared( const unsigned int** listOfDeclared, unsigned int &listSize ) const;
    bool            GetNextUndeclared( const unsigned int nextUndeclaredHint, unsigned int& nextUndeclared );
    void            GetDeclaredIndexList( const ShaderRegisterIndexType** declaredIndex, unsigned int &listSize ) const;

    const unsigned int* GetListOfDeclared( void ) const;
    const ShaderRegisterIndexType* GetDeclaredIndexList( void ) const;

    virtual void    Compress( void );

    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );

protected:
    CShaderFiniteDecl(
        const bool allowResizing,
        const unsigned int maxRegisters );
    virtual ~CShaderFiniteDecl( void );

    virtual ErrorCode  Initialize(  );
    virtual bool    Resize( unsigned int number );

    const bool                  m_cAllowResizing;

    ShaderRegisterIndexType*    m_pDeclaredIndex;  // for each register holds SHADER_REGISTER_INDEX_NONE
                                                   // if not declared or its declared index otherwise
    unsigned int* m_pDeclaredList;
    unsigned int  m_MaxRegisters;
    unsigned int  m_Count;

private:
    CShaderFiniteDecl(const CShaderFiniteDecl&);
    void operator=(const CShaderFiniteDecl&);

};

/*****************************************************************************\

Class:
    CShaderSamplerDecl

Description:

\*****************************************************************************/
class USC_API CShaderSamplerDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode    Create(
                            const unsigned int maxRegisters,
                            CShaderSamplerDecl*& pShaderDecl );
    static void         Delete(
                            CShaderSamplerDecl*& pShaderDecl );

    void                SetSamplerType(
                            const unsigned int number,
                            IGC::SHADER_SAMPLER_TYPE type );

    IGC::SHADER_SAMPLER_TYPE GetSamplerType(
                            const unsigned int number ) const;

protected:
    CShaderSamplerDecl( const unsigned int maxRegisters );
    virtual ~CShaderSamplerDecl( void );

    virtual ErrorCode  Initialize( void );

private:
    iSTD::CArray<SShaderSamplerDeclType,IGC::CAllocator>* m_pSamplerDecl;
};

/*****************************************************************************\

Class:
    CShaderResourceDecl

Description:

\*****************************************************************************/
class USC_API CShaderResourceDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode    Create(
                            const unsigned int maxRegisters,
                            CShaderResourceDecl*& pShaderDecl );
    static void         Delete(
                            CShaderResourceDecl*& pShaderDecl );

    void    SetResourceType(
                const unsigned int number,
                const IGC::SHADER_RESOURCE_TYPE type );

    void    SetSurfaceFormat(
                const unsigned int number,
                const SURFACE_FORMAT surfaceFormat );

    void    SetUAVAccessMode(
                const unsigned int number,
                const IGC::SHADER_UAV_ACCESS_MODE accessMode );

    void    SetReturnType(
                const unsigned int number,
                const IGC::SHADER_RESOURCE_RETURN_TYPE type );

    void    SetStride(
                const unsigned int number,
                const unsigned int stride );

    void    SetByteOrStructCount(
                const unsigned int number,
                const unsigned int byteOrStructCount );

    void    SetOffset(
                const unsigned int number,
                const unsigned int offset );

    void    SetAccessCoherency(
                const unsigned int number,
                const bool accessCoherency);

    void    SetTGSMVariable(
                const unsigned int number);

    void    SetAlignment(
                const unsigned int number,
                const unsigned int alignment );

    SURFACE_FORMAT
            GetSurfaceFormat(
                const unsigned int number ) const;

    IGC::SHADER_RESOURCE_TYPE
            GetResourceType(
                const unsigned int number ) const;

    unsigned int    GetResourceStride(
                const unsigned int number ) const;

    unsigned int    GetByteOrStructCount(
                const unsigned int number ) const;

    unsigned int   GetOffset(
                const unsigned int number ) const;

    bool    GetAccessCoherency(
                const unsigned int number ) const;

    unsigned int   GetAlignment(
                const unsigned int number ) const;

    IGC::SHADER_RESOURCE_RETURN_TYPE
            GetReturnType(
                const unsigned int number ) const;

    bool    IsTGSMFixed(
                const unsigned int number ) const;

    bool    IsTGSMVariable(
                const unsigned int number ) const;

    virtual void    Compress( void );

    SShaderResourceDeclType
            GetResourceDeclElement(
                const unsigned int index ) const;

    unsigned int GetResourceDeclSize( void ) const;

    bool    IsValid( void ) const;

    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );


protected:
    CShaderResourceDecl( const unsigned int maxRegisters );
    virtual ~CShaderResourceDecl( void );

    virtual ErrorCode  Initialize( void );

    iSTD::CArray<SShaderResourceDeclType,IGC::CAllocator>*   m_pResourceDecl;

private:
    CShaderResourceDecl(const CShaderResourceDecl&);
    void operator=(const CShaderResourceDecl&);
};

/*****************************************************************************\

Class:
    CShaderInputDecl

Description:

\*****************************************************************************/
class USC_API CShaderInputDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode   Create(
                        const unsigned int maxRegisters,
                        CShaderInputDecl*& pShaderDecl );
    static void     Delete( CShaderInputDecl*& pShaderDecl );

    void    SetMask(
                const unsigned int number,
                const IGC::SHADER_MASK mask );

    void    SetUsage(
                const unsigned int number,
                const IGC::SHADER_CHANNEL channel,
                const IGC::SHADER_USAGE usage );

    void    SetUsageIndex(
                const unsigned int number,
                const IGC::SHADER_CHANNEL channel,
                const unsigned int usageIndex );

    void    SetInterpolationMode(
                const unsigned int number,
                const IGC::SHADER_INTERPOLATION_MODE mode );

    void    SetIndexed(
                const unsigned int number,
                const bool indexed );

    IGC::SHADER_MASK     GetMask(
                        const unsigned int number ) const;

    IGC::SHADER_USAGE    GetUsage(
                        const unsigned int number,
                        const IGC::SHADER_CHANNEL channel ) const;

    unsigned int           GetUsageIndex(
                        const unsigned int number,
                        const IGC::SHADER_CHANNEL channel ) const;

    IGC::SHADER_INTERPOLATION_MODE   GetInterpolationMode(
                                    const unsigned int number ) const;

    bool            IsIndexed(
                        const unsigned int number ) const;

    const   SShaderInputDeclType& GetShaderInputDecl(unsigned int number) const;

    bool    HasIndexing( void ) const;

    void    SetMaxIndexedInputRegisterNum( const unsigned int number );
    void    SetMinIndexedInputRegisterNum( const unsigned int number );
    void    SetNumIndexedInputRegisters( const unsigned int number );

    unsigned int   GetMinIndexedInputRegisterNum( void ) const;
    unsigned int   GetMaxIndexedInputRegisterNum( void ) const;
    unsigned int   GetNumIndexedInputRegisters( void ) const;

    inline ShaderRegisterIndexType GetInstanceIDRegisterNum( void ) const;
    inline ShaderRegisterIndexType GetVertexIDRegisterNum( void ) const;

    virtual void    Compress( void );

    SShaderInputDeclType GetInputDeclElement( const unsigned int index ) const;

    unsigned int GetInputDeclSize( void ) const;

    bool IsValid( void ) const;

    inline bool HasFogFactor( void ) const;

    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );


protected:
    CShaderInputDecl( const unsigned int maxRegisters );
    virtual ~CShaderInputDecl( void );

    virtual ErrorCode  Initialize( void );

    iSTD::CArray<SShaderInputDeclType,IGC::CAllocator>*  m_pInputDecl;

    bool    m_HasIndexing;
    bool    m_HasFogFactor;

    ShaderRegisterIndexType m_InstanceIDRegisterNum;
    ShaderRegisterIndexType m_VertexIDRegisterNum;

    unsigned int   m_MaxIndexedInputRegisterNum;
    unsigned int   m_MinIndexedInputRegisterNum;
    unsigned int   m_NumIndexedInputRegisters;

private:
    CShaderInputDecl(const CShaderInputDecl&);
    void operator=(const CShaderInputDecl&);
};

/*****************************************************************************\

Function:
    CShaderInputDecl::HasFogFactor

Description:
    Returns true if one of the channels of one of the input declerations
    has usage of FOGFACTOR or FOGCOORDINATE.

Input:
    None

Output:
    bool

\*****************************************************************************/
inline bool CShaderInputDecl::HasFogFactor( void ) const
{
    return m_HasFogFactor;
}

/*****************************************************************************\

Function:
    CShaderInputDecl::GetInstanceIDRegisterNum

Description:
    Returns the register index which has usage IGC::SHADER_USAGE_INSTANCE_ID or
    SHADER_REGISTER_INDEX_NONE if no such register exist.

Input:
    None

Output:
    ShaderRegisterIndexType

\*****************************************************************************/
inline ShaderRegisterIndexType CShaderInputDecl::GetInstanceIDRegisterNum( void ) const
{
    return m_InstanceIDRegisterNum;
}

/*****************************************************************************\

Function:
    CShaderInputDecl::GetVertexIDRegisterNum

Description:
    Returns the register index which has usage IGC::SHADER_USAGE_VERTEX_ID or
    SHADER_REGISTER_INDEX_NONE if no such register exist.

Input:
    None

Output:
    ShaderRegisterIndexType

\*****************************************************************************/
inline ShaderRegisterIndexType CShaderInputDecl::GetVertexIDRegisterNum( void ) const
{
    return m_VertexIDRegisterNum;
}

/*****************************************************************************\

Class:
    CShaderOutputDecl

Description:

\*****************************************************************************/
class CShaderOutputDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode   Create(
                        const unsigned int maxRegisters,
                        CShaderOutputDecl*& pShaderDecl );
    static void     Delete( CShaderOutputDecl*& pShaderDecl );

    void    SetMask(
                const unsigned int number,
                const IGC::SHADER_MASK mask );

    void    SetUsage(
                const unsigned int number,
                const IGC::SHADER_CHANNEL channel,
                const IGC::SHADER_USAGE usage,
                const unsigned int usageIndex );

    void    SetIndexed(
                const unsigned int number,
                const bool indexed );

    IGC::SHADER_MASK     GetMask(
                        const unsigned int number ) const;

    IGC::SHADER_USAGE    GetUsage(
                        const unsigned int number,
                        const IGC::SHADER_CHANNEL channel ) const;

    unsigned int           GetUsageIndex(
                        const unsigned int number,
                        const IGC::SHADER_CHANNEL channel ) const;

    bool            IsIndexed(
                        const unsigned int number ) const;

    const   SShaderOutputDeclType& GetShaderOutputDecl(unsigned int number) const;

    bool    HasIndexing( void ) const;

    void    SetMaxIndexedOutputRegisterNum( const unsigned int number );
    void    SetMinIndexedOutputRegisterNum( const unsigned int number );
    void    SetNumIndexedOutputRegisters( const unsigned int number );

    unsigned int   GetMaxIndexedOutputRegisterNum( void ) const;
    unsigned int   GetMinIndexedOutputRegisterNum( void ) const;
    unsigned int   GetNumIndexedOutputRegisters( void ) const;

    void    SetActualOutputRegistersCount( const unsigned int number );
    void    SetAdditionalOutputsCount( const unsigned int number );
    void    SetActualOutputIndexNumber( const unsigned int index, const unsigned int number );
    void    SetAdditionalOutputIndexNumber( const unsigned int index, const unsigned int number );

    unsigned int   GetActualOutputRegistersCount( void ) const;
    unsigned int   GetAdditionalOutputsCount( void ) const;
    unsigned int   GetActualOutputIndexNumber( const unsigned int index ) const;
    unsigned int   GetAdditionalOutputIndexNumber( const unsigned int index ) const;

    void GetListOfDeclared( const unsigned int** listOfDeclared, unsigned int &listSize, USC_CLIENT_TYPE clientType, unsigned char renderTargetPropagationMask ) const;

    virtual void    Compress( void );

    SShaderOutputDeclType GetOutputDeclElement( const unsigned int index ) const;

    unsigned int GetOutputDeclSize( void ) const;

    bool IsValid( void ) const;

    bool     GetInvariantEnable( const unsigned int number ) const;
    void     SetInvariantEnable( const unsigned int number, const bool invariant );

    inline bool HasPointSize( void ) const;
    inline const ShaderRegisterIndexType* GetTextureCoordinateToRegisterNumMap( void ) const;
    inline const unsigned int* GetActualOutputsArray( void ) const;
    inline const unsigned int* GetAdditionalOutputsArray( void ) const;
    inline ShaderRegisterIndexType GetFogCoordinateOrFactorRegisterNum( void ) const;
    inline ShaderRegisterIndexType GetSecondaryColorRegisterNum( void ) const;

    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );


protected:
    CShaderOutputDecl( const unsigned int maxRegisters );
    virtual ~CShaderOutputDecl( void );

    virtual ErrorCode  Initialize( void );

    iSTD::CArray<SShaderOutputDeclType,IGC::CAllocator>* m_pOutputDecl;

    bool    m_HasIndexing;

    unsigned int   m_MaxIndexedOutputRegisterNum;
    unsigned int   m_MinIndexedOutputRegisterNum;
    unsigned int   m_NumIndexedOutputRegisters;

    unsigned int   m_ActualOutputRegisters[ NUM_PSHADER_OUTPUT_REGISTERS ];
    unsigned int   m_ActualOutputRegistersCount;
    unsigned int   m_AdditionalOutputRegisters[ NUM_PSHADER_OUTPUT_COLOR_REGISTERS ];
    unsigned int   m_AdditionalOutputsCount;

    bool    m_HasPointSize;
    ShaderRegisterIndexType m_texCoordRegisterNum[ NUM_TEXTURE_COORDINATES ];
    ShaderRegisterIndexType m_fogCoordRegisterNum;
    ShaderRegisterIndexType m_secondaryColorRegisterNum;

private:
    CShaderOutputDecl(const CShaderOutputDecl&);
    void operator=(const CShaderOutputDecl&);
};

/*****************************************************************************\

Function:
    CShaderOutputDecl::HasPointSize

Description:
    Returns true if one of the channels of one of the output declerations
    has usage of POINT_SIZE.

Input:
    None

Output:
    bool

\*****************************************************************************/
inline bool CShaderOutputDecl::HasPointSize( void ) const
{
    return m_HasPointSize;
}

/*****************************************************************************\

Function:
    CShaderOutputDecl::GetTextureCoordinateToRegisterNumMap

Description:
    Returns pointer to integer array of size MAX_TEXTURE_COORDINATES,
    each element in the array holds SHADER_REGISTER_INDEX_NONE if the
    corresponding texture coordinate is not defined used in any of the output registers,
    otherwise the register number of the texcoord.

Input:
    None

Output:
    ShaderRegisterIndexType*

\*****************************************************************************/
inline const ShaderRegisterIndexType*  CShaderOutputDecl::GetTextureCoordinateToRegisterNumMap( void ) const
{
    return m_texCoordRegisterNum;
}


inline const unsigned int* CShaderOutputDecl::GetActualOutputsArray( void ) const
{
    return m_ActualOutputRegisters;
}

inline const unsigned int* CShaderOutputDecl::GetAdditionalOutputsArray( void ) const
{
    return m_AdditionalOutputRegisters;
}

/*****************************************************************************\

Function:
    CShaderOutputDecl::GetFogCoordinateOrFactorRegisterNum

Description:
    Returns the first register number which has usage of fog coordinate or fog factor.
    If no such register exist, -1 is returned.

Input:
    None

Output:
    int

\*****************************************************************************/
inline ShaderRegisterIndexType CShaderOutputDecl::GetFogCoordinateOrFactorRegisterNum( void ) const
{
    return m_fogCoordRegisterNum;
}

/*****************************************************************************\

Function:
    CShaderOutputDecl::GetSecondaryColorRegisterNum

Description:
    Returns the first register number which has usage secondary color.
    If no such register exist, -1 is returned.

Input:
    None

Output:
    int

\*****************************************************************************/
inline ShaderRegisterIndexType CShaderOutputDecl::GetSecondaryColorRegisterNum( void ) const
{
    return m_secondaryColorRegisterNum;
}

/*****************************************************************************\

Class:
    CShaderIndexedTemporaryArrayDecl

Description:

\*****************************************************************************/
class CShaderIndexedTemporaryArrayDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode   Create(
                        const unsigned int maxRegisters,
                        CShaderIndexedTemporaryArrayDecl*& pShaderDecl );
    static void     Delete( CShaderIndexedTemporaryArrayDecl*& pShaderDecl );

    void    Setup(
                const unsigned int number,
                const unsigned int size );
    void    SetSize(
                const unsigned int number,
                const unsigned int size );
    void    SetCached(
                const unsigned int number,
                const SIMD_MODE simdMode );
    void    SetReadChannels(
                const unsigned int number,
                const SShaderWriteMask mask );
    void    SetHasIndirectRegisterWrites(
                const unsigned int number );

    unsigned int     GetSize(
                         const unsigned int number ) const;
    bool             IsCached(
                         const unsigned int number,
                         const SIMD_MODE simdMode ) const;
    bool             HasCachedTemps( void ) const;
    SShaderWriteMask GetReadChannels(
                         const unsigned int number ) const;
    unsigned int     GetNumReadChannels(
                         const unsigned int number ) const;
    IGC::SHADER_CHANNEL   GetFirstReadChannel(
                         const unsigned int number ) const;
    bool             HasIndirectRegisterWrites(
                         const unsigned int number ) const;

    virtual void    Compress( void );

    SIndexedTempDeclType GetIndexedTemporaryArrayDeclElement( const unsigned int index ) const;

    unsigned int GetIndexedTemporaryArrayDeclSize( void ) const;

    bool IsValid( void ) const;

    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );


protected:
    CShaderIndexedTemporaryArrayDecl( const unsigned int maxRegisters );
    virtual ~CShaderIndexedTemporaryArrayDecl( void );

    virtual ErrorCode  Initialize( void );

    const SIndexedTempDeclType* GetDeclElement( const unsigned int number ) const;
    void                        SetDeclElement( const unsigned int number,
                                                const SIndexedTempDeclType& decl );

    iSTD::CArray<SIndexedTempDeclType,IGC::CAllocator>* m_pIndexedTemporaryArrayDecl;

private:
    CShaderIndexedTemporaryArrayDecl(const CShaderIndexedTemporaryArrayDecl&);
    void operator=(const CShaderIndexedTemporaryArrayDecl&);
    bool m_HasCachedTemps;
};

/*****************************************************************************\

Class:
    CShaderInterfaceDecl

Description:

\*****************************************************************************/
class CShaderInterfaceDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode   Create(
                        const unsigned int maxRegisters,
                        CShaderInterfaceDecl*& pShaderDecl );

    static void     Delete( CShaderInterfaceDecl*& pShaderDecl );

    virtual void    Compress( void );

    bool    SetInterface( const unsigned int number,
                        const unsigned int arraySize,
                        const unsigned int numCallSites,
                        const unsigned int numFunctionTables,
                        const unsigned int*& functionTables,
                        const unsigned int constantBufferOffset,
                        SShaderInterfaceDeclType& iface );

    bool    GetInterface( const unsigned int number,
                        SShaderInterfaceDeclType& iface ) const;

    SShaderInterfaceDeclType GetIfaceDeclElement( const unsigned int index ) const;

    bool IsValid( void ) const;

    // Throwing assertion failure and returning error, since we have to declare interface manually from CShader
    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );

protected:
    CShaderInterfaceDecl( const unsigned int maxRegisters );

    virtual ~CShaderInterfaceDecl( void );

    virtual ErrorCode  Initialize( void );

    iSTD::CArray<SShaderInterfaceDeclType,IGC::CAllocator>* m_pIfaceDecls;
};

/*****************************************************************************\

Class:
    CShaderFunctionTableDecl

Description:

\*****************************************************************************/
class CShaderFunctionTableDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode   Create(
                        const unsigned int maxRegisters,
                        CShaderFunctionTableDecl*& pShaderDecl );

    static void     Delete( CShaderFunctionTableDecl*& pShaderDecl );

    virtual void    Compress( void );

    bool    SetFunctionTable( const unsigned int number,
                              const unsigned int numFunctionBodies,
                              const unsigned int* pFunctionBodies );

    bool    GetFunctionTable( const unsigned int number,
                              SShaderFunctionTableDeclType &ftDecl ) const;

    bool    GetCallSite( const unsigned int ftID,
                         const unsigned int fbID,
                         unsigned int& callSite ) const;

    bool    GetCallOffsets( const unsigned int ftID,
                            const unsigned int callSite,
                            SVFuncCallOffsets& offsets) const;

    SShaderFunctionTableDeclType GetFunctionTablesElement( const unsigned int index ) const;

    unsigned int GetFunctionTablesArraySize( void ) const;

    bool IsValid( void ) const;

    // Throwing assertion failure and returning error, since we have to declare interface manually from CShader
    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );

protected:
    CShaderFunctionTableDecl( const unsigned int maxRegisters );

    virtual ~CShaderFunctionTableDecl( void );

    virtual ErrorCode  Initialize( void );
    virtual bool    Resize( unsigned int number );

    iSTD::CArray<SShaderFunctionTableDeclType,IGC::CAllocator>* m_pFunctionTables;
};

/*****************************************************************************\

Class:
    CShaderFunctionBodyDecl

Description:

\*****************************************************************************/
class CShaderFunctionBodyDecl :
    public CShaderFiniteDecl
{
public:
    static ErrorCode   Create(
                        const unsigned int maxRegisters,
                        CShaderFunctionBodyDecl*& pShaderDecl );

    static void     Delete( CShaderFunctionBodyDecl*& pShaderDecl );

    template<typename TOpType>
    ErrorCode  Process( const CShaderDecl& src, unsigned char*& pStream, unsigned int& size );

protected:
    CShaderFunctionBodyDecl( const unsigned int maxRegisters );

    virtual ~CShaderFunctionBodyDecl( void );
};

} // namespace USC
