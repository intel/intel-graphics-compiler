/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ShaderTypesEnum.h"
#include "usc_config.h"
#include "ShaderTypes.h"

namespace USC
{

#if defined _MSC_VER
#   pragma warning( push )
#   pragma warning( disable: 4201 ) // warning C4201: nonstandard extension used : nameless struct/union
#endif

/*****************************************************************************\
STRUCT: SShaderWriteMask
\*****************************************************************************/
union SShaderWriteMask
{
    struct
    {
        unsigned char   X         : 1;    // bool
        unsigned char   Y         : 1;    // bool
        unsigned char   Z         : 1;    // bool
        unsigned char   W         : 1;    // bool
#if defined(__GNUC__) && !defined(__clang__)
        unsigned char   _Unused   : 4;    // For GCC 4.7 bug (do not allow to static initialize anonymous members)
#else
        unsigned char             : 4;    // For clang compilation option -Wmissing-field-initializers (every non-anonymous member should be initialized)
#endif //#if defined(__GNUC__) && !defined(__clang__)
    };
#if defined _MSC_VER
    unsigned char   Value;
#else
    unsigned char   Value   : 4;
#endif

    inline void EnableChannel  ( IGC::SHADER_CHANNEL channel ) { Value |=  BIT(channel); }
    inline void DisableChannel ( IGC::SHADER_CHANNEL channel ) { Value &= ~BIT(channel); }
    inline bool Contains( const SShaderWriteMask wm ) const;
    void Set( const int i, const bool val )             { Value = ( Value & (~(1<<i)) ) + (val<<i); }

    bool operator[] ( const int i ) const               { return (Value>>i)&1; }
    bool operator== ( const SShaderWriteMask& o ) const { return 0==(Value^o.Value); }
    bool operator!= ( const SShaderWriteMask& o ) const { return 0!=(Value^o.Value); }
};

USC_API_C_ASSERT( sizeof(SShaderWriteMask) == sizeof(unsigned char) );


/*****************************************************************************\
STRUCT: SExtTextureSwizzle
    Describes a channel swizzling extended by a possibility of setting each 
    color channel to zero or one. 
    Such extended swizzling is used to define texture swizzling 
    as required by OpenGL GL_EXT_texture_swizzle extension.
\*****************************************************************************/
struct SExtTextureSwizzle
{
    IGC::EXT_TEXTURE_SWIZZLE R;
    IGC::EXT_TEXTURE_SWIZZLE G;
    IGC::EXT_TEXTURE_SWIZZLE B;
    IGC::EXT_TEXTURE_SWIZZLE A;

    inline bool IsDefault() const;
    inline const IGC::EXT_TEXTURE_SWIZZLE & operator[] (unsigned int i) const;
};

/*****************************************************************************\

Function:
    SExtTextureSwizzle::IsDefault()

Description:
    Returns true if the extended swizzle is the identity swizzle, i.e.
    it doesn't change the channels.

Input:
    none

Output:
    true if channel swizzling is identity, false otherwise.

\*****************************************************************************/
inline bool SExtTextureSwizzle::IsDefault() const
{ 
    return ( R == IGC::EXT_TEXTURE_SWIZZLE_RED   ) && 
           ( G == IGC::EXT_TEXTURE_SWIZZLE_GREEN ) && 
           ( B == IGC::EXT_TEXTURE_SWIZZLE_BLUE  ) && 
           ( A == IGC::EXT_TEXTURE_SWIZZLE_ALPHA );
}

/*****************************************************************************\

Function:
    SExtTextureSwizzle::operator[]()

Description:
    This operator assumes that R, G, B, A channels can be indexed by numbers
    0, 1, 2, 3 and returns the i-th value.

    Note that the operator returns a const ref, so it cannot be used to modify
    the value, only to read it.
Input:
    i  - index of the field in the structure.

Output:
    const reference to the i-th member of the structure.

\*****************************************************************************/
inline const IGC::EXT_TEXTURE_SWIZZLE & SExtTextureSwizzle::operator[] (unsigned int i) const
{
    switch ( i )
    {
    case 0: 
        return R;
    case 1: 
        return G;
    case 2: 
        return B;
    case 3: 
        return A;
    default: 
        // index is out of bounds, return the last element (assert?)
        return A;
    }
}

/*****************************************************************************\
CONST: g_cInitShaderWriteMask
\*****************************************************************************/
USC_API extern const SShaderWriteMask g_cInitShaderWriteMask;
const SShaderWriteMask USC_API_CALL  getInitShaderWriteMask();

/*****************************************************************************\
Other commonly used write masks
\*****************************************************************************/
extern const SShaderWriteMask g_cShaderWriteMaskX;
extern const SShaderWriteMask g_cShaderWriteMaskXY;
extern const SShaderWriteMask g_cShaderWriteMaskYZ;
extern const SShaderWriteMask g_cShaderWriteMaskXYZ;
extern const SShaderWriteMask g_cShaderWriteMaskY;
extern const SShaderWriteMask g_cShaderWriteMaskZ;
extern const SShaderWriteMask g_cShaderWriteMaskZW;
extern const SShaderWriteMask g_cShaderWriteMaskW;

/*****************************************************************************\
CONST: g_cShaderWriteMask[channel] - writemasks with only "channel" Set
\*****************************************************************************/
USC_API extern const SShaderWriteMask g_cShaderWriteMask[ IGC::NUM_SHADER_CHANNELS ];

typedef const SShaderWriteMask  (&tSShaderWriteMaskHelper)[IGC::NUM_SHADER_CHANNELS];
tSShaderWriteMaskHelper USC_API_CALL getShaderWriteMask();

/*****************************************************************************\
CONST: g_cShaderWriteMaskX
\*****************************************************************************/
extern const SShaderWriteMask g_cShaderWriteMaskEmpty;

/*****************************************************************************\
STRUCT: SShaderSwizzle
\*****************************************************************************/
union SShaderSwizzle
{
    struct
    {
        unsigned char   X   : IGC::BITS_FOR_SHADER_CHANNEL;
        unsigned char   Y   : IGC::BITS_FOR_SHADER_CHANNEL;
        unsigned char   Z   : IGC::BITS_FOR_SHADER_CHANNEL;
        unsigned char   W   : IGC::BITS_FOR_SHADER_CHANNEL;
    };
    unsigned char   Value;

    void    Replicate( IGC::SHADER_CHANNEL channel )         { X=Y=Z=W=channel; }
    bool    IsReplicate( void ) const                   { return X==Y && X==Z && X==W; }
    inline bool IsIntersecting( const SShaderWriteMask wm ) const;
    inline SShaderWriteMask Channels( void ) const;
    void Set( const int i, const IGC::SHADER_CHANNEL val )   { Value = static_cast<unsigned char>( ( Value & (~(((1<<IGC::BITS_FOR_SHADER_CHANNEL)-1)<<(i*IGC::BITS_FOR_SHADER_CHANNEL))) ) | (val<<(i*IGC::BITS_FOR_SHADER_CHANNEL)) ); }

    IGC::SHADER_CHANNEL operator[] ( const int i ) const     { return (IGC::SHADER_CHANNEL)( ( Value>>(i*IGC::BITS_FOR_SHADER_CHANNEL) )&((1<<IGC::BITS_FOR_SHADER_CHANNEL)-1) ); }
    bool operator== ( const SShaderSwizzle& o ) const   { return Value==o.Value; }
    bool operator!= ( const SShaderSwizzle& o ) const   { return Value!=o.Value; }
};

/*****************************************************************************\
STRUCT: Operator to shuffle the writemask with a swizzle
resultWm[channel] = sourceWm[srcSwizzle[channel]]
\*****************************************************************************/
inline SShaderWriteMask operator * ( SShaderSwizzle swizzle, SShaderWriteMask writemask )
{
    SShaderWriteMask resultWritemask = 
    {
        {
            static_cast<unsigned char>(writemask[ swizzle[IGC::SHADER_CHANNEL_X] ]),
            static_cast<unsigned char>(writemask[ swizzle[IGC::SHADER_CHANNEL_Y] ]),
            static_cast<unsigned char>(writemask[ swizzle[IGC::SHADER_CHANNEL_Z] ]),
            static_cast<unsigned char>(writemask[ swizzle[IGC::SHADER_CHANNEL_W] ])
        }
    };

    return resultWritemask;
}

/*****************************************************************************\
STRUCT: Operator negates the writemask
only channels that are not set in writemask are set in resulting writemask
\*****************************************************************************/
inline SShaderWriteMask operator ~ ( SShaderWriteMask writemask )
{
    SShaderWriteMask resultWritemask =
    {
        {
            static_cast<unsigned char>(!writemask.X),
            static_cast<unsigned char>(!writemask.Y),
            static_cast<unsigned char>(!writemask.Z),
            static_cast<unsigned char>(!writemask.W)
        }
    };
    return resultWritemask;
}

/*****************************************************************************\
STRUCT: Operator intersects the writemask
only channels that are set in both writemasks are set in resulting writemask
\*****************************************************************************/
inline SShaderWriteMask operator & ( SShaderWriteMask writemaskA, SShaderWriteMask writemaskB )
{
    SShaderWriteMask resultWritemask = 
    {
        {
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_X ] && writemaskB[ IGC::SHADER_CHANNEL_X ]),
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_Y ] && writemaskB[ IGC::SHADER_CHANNEL_Y ]),
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_Z ] && writemaskB[ IGC::SHADER_CHANNEL_Z ]),
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_W ] && writemaskB[ IGC::SHADER_CHANNEL_W ])
        }
    };

    return resultWritemask;
}

/*****************************************************************************\
OPERATOR: Operator returns Union of the writemasks
In resultWritemask, channel is set if it is set in any of argument Writemask.
\*****************************************************************************/
inline SShaderWriteMask operator | ( SShaderWriteMask writemaskA, SShaderWriteMask writemaskB )
{
    SShaderWriteMask resultWritemask = 
    {
        {
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_X ] || writemaskB[ IGC::SHADER_CHANNEL_X ]),
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_Y ] || writemaskB[ IGC::SHADER_CHANNEL_Y ]),
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_Z ] || writemaskB[ IGC::SHADER_CHANNEL_Z ]),
            static_cast<unsigned char>(writemaskA[ IGC::SHADER_CHANNEL_W ] || writemaskB[ IGC::SHADER_CHANNEL_W ])
        }
    };

    return resultWritemask;
}

/*****************************************************************************\
STRUCT: Operator XOR for the writemask
\*****************************************************************************/
inline SShaderWriteMask operator ^ ( SShaderWriteMask writemaskA, SShaderWriteMask writemaskB )
{
    SShaderWriteMask resultWritemask = 
    {
        {
                static_cast<unsigned char>(!( writemaskA[ IGC::SHADER_CHANNEL_X ] == writemaskB[ IGC::SHADER_CHANNEL_X ] )),
                static_cast<unsigned char>(!( writemaskA[ IGC::SHADER_CHANNEL_Y ] == writemaskB[ IGC::SHADER_CHANNEL_Y ] )),
                static_cast<unsigned char>(!( writemaskA[ IGC::SHADER_CHANNEL_Z ] == writemaskB[ IGC::SHADER_CHANNEL_Z ] )),
                static_cast<unsigned char>(!( writemaskA[ IGC::SHADER_CHANNEL_W ] == writemaskB[ IGC::SHADER_CHANNEL_W ] ))
        }
    };

    return resultWritemask;
}

/*****************************************************************************\

Function:
    SShaderWriteMask::Contains

Description:
    Returns true if all enabled channels in writemask given as input param are
    enabled in current writemask.

Input:
    SShaderWriteMask wm

Output:
    bool

\*****************************************************************************/
inline bool SShaderWriteMask::Contains( const SShaderWriteMask wm ) const
{
    if( ( wm.X && !X ) || ( wm.Y && !Y ) || ( wm.Z && !Z ) || ( wm.W && !W ) )
    {
        return false;
    }
    else
    {
        return true;
    }
}

/*****************************************************************************\
STRUCT: Operator - Composition of two swizzles (It is not commutative operation!)
        resultSw(channel) = A ( B (channel) )
\*****************************************************************************/
inline SShaderSwizzle operator * ( SShaderSwizzle A, SShaderSwizzle B )
{
    SShaderSwizzle resultSw = 
    {
        {
            static_cast<unsigned char>(A[ B[IGC::SHADER_CHANNEL_X] ]),
            static_cast<unsigned char>(A[ B[IGC::SHADER_CHANNEL_Y] ]),
            static_cast<unsigned char>(A[ B[IGC::SHADER_CHANNEL_Z] ]),
            static_cast<unsigned char>(A[ B[IGC::SHADER_CHANNEL_W] ])
        }
    };

    return resultSw;
}
USC_API_C_ASSERT( sizeof(SShaderSwizzle) == sizeof(unsigned char) );

/*****************************************************************************\
CONST: g_cInitShaderSwizzle
\*****************************************************************************/
USC_API extern const SShaderSwizzle g_cInitShaderSwizzle;
const SShaderSwizzle USC_API_CALL  getInitShaderSwizzle();

/*****************************************************************************\
Other commonly used swizzles
\*****************************************************************************/
extern const SShaderSwizzle g_cShaderSwizzleXXXX;
extern const SShaderSwizzle g_cShaderSwizzleXXXW;
extern const SShaderSwizzle g_cShaderSwizzleYYYY;
extern const SShaderSwizzle g_cShaderSwizzleZZZZ;
extern const SShaderSwizzle g_cShaderSwizzleWWWW;
extern const SShaderSwizzle g_cShaderSwizzleYZZZ;
extern const SShaderSwizzle g_cShaderSwizzleZWXY;
extern const SShaderSwizzle g_cShaderSwizzleXYXY;
extern const SShaderSwizzle g_cShaderSwizzleZWZW;
extern const SShaderSwizzle g_cReplicateShaderSwizzles[ IGC::NUM_SHADER_CHANNELS ];

/*****************************************************************************\

Function:
    SShaderSwizzle::Channels

Description:
    Returns writemask where channels used in current swizzle are enabled.

Input:
    void

Output:
    SShaderWriteMask wm

\*****************************************************************************/
inline SShaderWriteMask SShaderSwizzle::Channels( void ) const
{
    SShaderWriteMask wm;

    wm.Value = ( ( 1 << X ) | ( 1 << Y ) | ( 1 << Z ) | ( 1 << W ) );

    return wm;
}

/*****************************************************************************\

Function:
    SShaderSwizzle::IsIntersecting

Description:
    Returns true if current swizzle has any intersection with writemask given
    as input param.

Input:
    SShaderWriteMask wm

Output:
    bool

\*****************************************************************************/
inline bool SShaderSwizzle::IsIntersecting( const SShaderWriteMask wm ) const
{
    if( wm.Value & Channels().Value )
    { 
        return true;
    }
    else
    {
        return false;
    }
}

/*****************************************************************************\
STRUCT: SShaderOpcodeToken
\*****************************************************************************/
union SShaderOpcodeToken
{
    struct
    {
        // If adding new fields below, update Hash() method too.
        unsigned int   m_Opcode            : BITCOUNT( IGC::NUM_SHADER_OPCODES );       // IGC::SHADER_OPCODE
        unsigned int   m_Comparison        : BITCOUNT( IGC::NUM_SHADER_COMPARISONS );   // IGC::SHADER_COMPARISON
        unsigned int   m_Conditional       : BITCOUNT( IGC::NUM_SHADER_CONDITIONALS );  // IGC::SHADER_CONDITIONAL
        unsigned int   m_PartialPrecision  : 1;                                    // bool
        unsigned int   m_Predicate         : 1;                                    // bool
        unsigned int   m_Comment           : 1;                                    // bool
        unsigned int   m_Sync              : IGC::cNumberOfShaderSyncBits; // IGC::SHADER_SYNC
        unsigned int   m_ResourceType      : 1;                                    // bool
        unsigned int   m_Precise           : 1;                                    // bool
        unsigned int   m_Feedback          : 1;                                    // bool
    };
    unsigned int Value;

    void                SetOpcode( IGC::SHADER_OPCODE opcode )                   { m_Opcode = opcode; }
    IGC::SHADER_OPCODE       GetOpcode( void ) const                             { return (IGC::SHADER_OPCODE)m_Opcode; }

    IGC::SHADER_OPCODE_TYPE  GetOpcodeType( unsigned int registerNumber ) const;

    void                SetComparison( IGC::SHADER_COMPARISON comparison )       { m_Comparison = comparison; } 
    IGC::SHADER_COMPARISON   GetComparison( void ) const                         { return (IGC::SHADER_COMPARISON)m_Comparison; }

    void                SetConditional( IGC::SHADER_CONDITIONAL conditional )    { m_Conditional = conditional; }
    IGC::SHADER_CONDITIONAL  GetConditional( void ) const                        { return (IGC::SHADER_CONDITIONAL)m_Conditional; }

    void                SetResourceTypeEnable( bool usesResourceType )      { m_ResourceType = usesResourceType; }
    bool                GetResourceTypeEnable( void ) const                 { return m_ResourceType; }

    void                SetPartialPrecisionEnable( bool enable )            { m_PartialPrecision = enable; }
    bool                GetPartialPrecisionEnable( void ) const             { return m_PartialPrecision; }

    void                SetPredicateEnable( bool enable )                   { m_Predicate = enable; }
    bool                GetPredicateEnable( void ) const                    { return m_Predicate; }

    void                SetCommentEnable( bool enable )                     { m_Comment = enable; }
    bool                GetCommentEnable( void ) const                      { return m_Comment; }
    
    void                SetSync( IGC::SHADER_SYNC sync )                         { m_Sync = sync; }
    IGC::SHADER_SYNC         GetSync( void ) const                               { return IGC::SHADER_SYNC::Create(m_Sync); }

    void                SetPreciseEnable( const bool precise )              { m_Precise = precise; }
    bool                GetPreciseEnable( void ) const                      { return m_Precise; }

    void                SetFeedbackEnable( const bool feedback )            { m_Feedback = feedback; }
    bool                GetFeedbackEnable( void ) const                      { return m_Feedback; }

    unsigned int        GetDestinationRegisterCount( void ) const;
    unsigned int        GetSourceRegisterCount( void ) const;

    bool                HasComparison( void ) const;
    bool                HasConditional( void ) const;
    bool                HasLabel( void ) const;
    bool                IsExplicitPrecisionInstruction( void ) const;
    bool                IsAddressableStreamOut( void ) const;
    bool                IsFlowControl( void ) const;
    bool                IsLoopStart( void ) const;
    bool                IsSample( void ) const;
    bool                IsSampleWithComparison( void ) const;
    bool                HasVariableNumberOfSources( void ) const;
    unsigned int        SamplerPosition( void ) const;
    unsigned int        ResourcePosition( void ) const;
    bool                HasTGSMOrUAVOrTPMSource( unsigned int srcIndex ) const;
    bool                HasPointerDestination( unsigned int dstIndex ) const;
    bool                HasPointerSource( unsigned int sourceIndex ) const;

    unsigned long long  Hash( void ) const;
};

USC_API_C_ASSERT( sizeof(SShaderOpcodeToken) == sizeof(unsigned int) );

/*****************************************************************************\
CONST: g_cInitShaderOpcodeToken
\*****************************************************************************/
USC_API extern const SShaderOpcodeToken g_cInitShaderOpcodeToken;
const SShaderOpcodeToken USC_API_CALL  getInitShaderOpcodeToken();

/*****************************************************************************\
STRUCT: SShaderDestinationRegisterToken
\*****************************************************************************/
union SShaderDestinationRegisterToken
{
    struct
    {
        // If adding new fields below, update Hash() method too.
        unsigned int   m_Number;
        unsigned int   m_File              : BITCOUNT(IGC::NUM_SHADER_REGISTER_FILES);       // IGC::SHADER_REGISTER_FILE
        unsigned int   m_WriteMask         : BITCOUNT(IGC::NUM_SHADER_MASKS);                // SShaderWriteMask
        unsigned int   m_Saturate          : 1;                                         // bool
        unsigned int   m_IndirectRegister  : 1;                                         // bool
        unsigned int   m_IndirectOffset    : 1;                                         // bool
        unsigned int   m_Precision         : BITCOUNT(NUM_SHADER_OPERAND_PRECISIONS);   // SHADER_OPERAND_DATA_TYPES
        unsigned int   m_ExactPrecision    : 1;                                         // bool
        unsigned int   m_NonUniformIndex   : 1;                                         // bool
    };

    unsigned long long   Value;

    void                    SetFile( IGC::SHADER_REGISTER_FILE file )            { m_File = file; }
    IGC::SHADER_REGISTER_FILE    GetFile( void ) const                           { return (IGC::SHADER_REGISTER_FILE)m_File; }

    void                    SetNumber( unsigned int number )                { m_Number = number; }
    unsigned int            GetNumber( void ) const                         { return m_Number; }

    void                    SetWriteMask( SShaderWriteMask writemask )      { m_WriteMask = writemask.Value; }
    SShaderWriteMask        GetWriteMask( void ) const                      { SShaderWriteMask writemask; writemask.Value = m_WriteMask; return writemask; }

    void                    SetSaturateEnable( bool enable )                { m_Saturate = enable; }
    bool                    GetSaturateEnable( void ) const                 { return (bool)m_Saturate; }

    void                    SetIndirectRegisterEnable( bool enable )        { m_IndirectRegister = enable; }
    bool                    GetIndirectRegisterEnable( void ) const         { return (bool)m_IndirectRegister; }

    void                    SetIndirectOffsetEnable( bool enable )          { m_IndirectOffset = enable; }
    bool                    GetIndirectOffsetEnable( void ) const           { return (bool)m_IndirectOffset; }

    bool                    HasNoEnables( void ) const                      { return !m_Saturate && !m_IndirectRegister && !m_IndirectOffset; }
    bool                    HasIndirectAddressing( void ) const             { return m_IndirectOffset || m_IndirectRegister; }

    unsigned long long      Hash( void ) const;

    void                        SetPrecision( SHADER_OPERAND_PRECISION precision )  { m_Precision = precision; }
    SHADER_OPERAND_PRECISION    GetPrecision( void ) const { return (SHADER_OPERAND_PRECISION)m_Precision; }

    void                    SetExactPrecision( bool exactPrecision ) { m_ExactPrecision = exactPrecision; }
    bool                    IsPrecisionExact( void ) const { return (bool)m_ExactPrecision; }

    void                    SetNonUniformIndex( bool nonUniform ) { m_NonUniformIndex = nonUniform; }
    bool                    IsNonUniformIndex( void ) const { return (bool)m_NonUniformIndex; }
};

USC_API_C_ASSERT( sizeof(SShaderDestinationRegisterToken) == sizeof(unsigned long long) );

/*****************************************************************************\
CONST: g_cInitShaderDestinationRegisterToken
\*****************************************************************************/
USC_API extern const SShaderDestinationRegisterToken g_cInitShaderDestinationRegisterToken;
const SShaderDestinationRegisterToken USC_API_CALL  getInitShaderDestinationRegisterToken();

/*****************************************************************************\
STRUCT: SShaderSourceRegisterToken
\*****************************************************************************/
union SShaderSourceRegisterToken
{
    struct
    {
        // If adding new fields below, update Hash() method too.
        unsigned int   m_Number;
        unsigned int   m_File              : BITCOUNT( IGC::NUM_SHADER_REGISTER_FILES );                    // IGC::SHADER_REGISTER_FILE
        unsigned int   m_Swizzle           : BITCOUNT( IGC::NUM_SHADER_CHANNELS ) * IGC::NUM_SHADER_CHANNELS;    // SShaderSwizzle
        unsigned int   m_Negate            : 1;                                                        // bool
        unsigned int   m_Absolute          : 1;                                                        // bool
        unsigned int   m_AddressOffset     : 1;                                                        // bool
        unsigned int   m_IndirectRegister  : 1;                                                        // bool
        unsigned int   m_IndirectOffset    : 1;                                                        // bool
        unsigned int   m_AccessRegister    : 1;                                                        // bool
        unsigned int   m_AccessOffset      : 1;                                                        // bool
        unsigned int   m_Precision         : BITCOUNT(NUM_SHADER_OPERAND_PRECISIONS);                  // NUM_SHADER_OPERAND_PRECISIONS
        unsigned int   m_NonUniformIndex   : 1;                                                        // bool
    };
    unsigned long long   Value;

    void                    SetFile( IGC::SHADER_REGISTER_FILE file )            { m_File = file; }
    IGC::SHADER_REGISTER_FILE    GetFile( void ) const                           { return (IGC::SHADER_REGISTER_FILE)m_File; }

    void                    SetNumber( unsigned int number )                { m_Number = number; }
    unsigned int            GetNumber( void ) const                         { return m_Number; }

    void                    SetSwizzle( SShaderSwizzle swizzle )            { m_Swizzle = swizzle.Value; }
    SShaderSwizzle          GetSwizzle( void ) const                        { SShaderSwizzle swizzle; swizzle.Value = m_Swizzle; return swizzle; }

    void                    SetNegateEnable( bool enable )                  { m_Negate = enable; }
    bool                    GetNegateEnable( void ) const                   { return (bool)m_Negate; }

    void                    SetAbsoluteEnable( bool enable )                { m_Absolute = enable; }
    bool                    GetAbsoluteEnable( void ) const                 { return (bool)m_Absolute; }

    void                    SetAddressOffsetEnable( bool enable )           { m_AddressOffset = enable; }
    bool                    GetAddressOffsetEnable( void ) const            { return (bool)m_AddressOffset; }

    void                    SetIndirectRegisterEnable( bool enable )        { m_IndirectRegister = enable; }
    bool                    GetIndirectRegisterEnable( void ) const         { return (bool)m_IndirectRegister; }

    void                    SetIndirectOffsetEnable( bool enable )          { m_IndirectOffset = enable; }
    bool                    GetIndirectOffsetEnable( void ) const           { return (bool)m_IndirectOffset; }

    void                    SetAccessRegisterEnable( bool enable )          { m_AccessRegister = enable; }
    bool                    GetAccessRegisterEnable( void ) const           { return (bool)m_AccessRegister; }

    void                    SetAccessOffsetEnable( bool enable )            { m_AccessOffset = enable; }
    bool                    GetAccessOffsetEnable( void ) const             { return (bool)m_AccessOffset; }

    void                        SetPrecision( SHADER_OPERAND_PRECISION precision )  { m_Precision = precision; }
    SHADER_OPERAND_PRECISION    GetPrecision( void ) const                          { return (SHADER_OPERAND_PRECISION)m_Precision; }

    void                    SetNonUniformIndex( bool nonUniform ) { m_NonUniformIndex = nonUniform; }
    bool                    IsNonUniformIndex( void ) const { return (bool)m_NonUniformIndex; }

    bool                    HasNoEnables( void ) const                      { return !m_Negate && !m_Absolute && !m_AddressOffset && !m_IndirectRegister && !m_IndirectOffset && !m_AccessRegister && !m_AccessOffset; }
    bool                    HasIndirectAddressing( void ) const             { return m_IndirectRegister || m_IndirectOffset || m_AddressOffset || m_AccessRegister || m_AccessOffset; }

    unsigned long long      Hash( void ) const;
};

USC_API_C_ASSERT( sizeof(SShaderSourceRegisterToken) == sizeof(unsigned long long) );

/*****************************************************************************\
CONST: g_cInitShaderSourceRegisterToken
\*****************************************************************************/
USC_API extern const SShaderSourceRegisterToken g_cInitShaderSourceRegisterToken;
const SShaderSourceRegisterToken USC_API_CALL getInitShaderSourceRegisterToken();

/*****************************************************************************\
CONST: g_cShadingFuncIdNone
\*****************************************************************************/
USC_API extern const unsigned int g_cShadingFuncIdNone;

#ifdef _MSC_VER
#pragma warning( pop ) // warning C4201: nonstandard extension used : nameless struct/union
#endif

} // namespace USC
