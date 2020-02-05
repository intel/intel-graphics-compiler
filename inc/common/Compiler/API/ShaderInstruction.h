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
#pragma once

#include "usc_config.h"
#include "ShaderToken.h"
#include "ErrorCode.h"

namespace USC
{
class CShaderIfc;
}

namespace USC
{
class CShaderDebugLocationInfo;
}

#if defined _DEBUG && !defined __linux__
#define USC_OPTIMIZING_COMPILER_TESTS
#endif

#ifdef USC_OPTIMIZING_COMPILER_TESTS
#include "../../USC/optimizer/OptimizingCompilerTests/TestFlags.h"
#endif

namespace USC
{

const unsigned int MAX_INSTRUCTION_DESTINATION_REGISTERS = 4;
const unsigned int MAX_INSTRUCTION_SOURCE_REGISTERS = 8;

/*****************************************************************************\

Class:
    CShaderInstruction

Description:
    Abstraction class for shader tokens

\*****************************************************************************/
class USC_API CShaderInstruction
{
public:
    CShaderInstruction( void );
    ~CShaderInstruction( void );

    void    CopyDestination(
                const CShaderInstruction& sourceInstruction,
                unsigned int sourceDestination,
                unsigned int destinationDestination );
    void    CopySource(
                const CShaderInstruction& sourceInstruction,
                unsigned int sourceSource,
                unsigned int destinationSource );
    void    NegateSource( 
                const unsigned int source );
    void    ForcePositiveSource( 
                const unsigned int source );
    void    ForceNegativeSource( 
                const unsigned int source );

    void    DeallocateExtendedRegisters( void );

    void    Init( void );

    ErrorCode    CopyInstruction( const CShaderInstruction& sourceInstruction );

    ErrorCode    InitVariable(
                    const unsigned int numDestinations,
                    const unsigned int numSources );

    void    SetHeader(
                const SShaderOpcodeToken& opcode );

    void    SetDestinationRegister(
                const unsigned int index,
                const SShaderDestinationRegisterToken& destination );

    void    SetDestinationIndirectRegister(
                const unsigned int index,
                const SShaderSourceRegisterToken& source );

    void    SetDestinationIndirectRegisterOffset(
                const unsigned int index,
                const long offset );

    void    SetDestinationIndirectOffset(
                const unsigned int index,
                const long offset );

    void    SetPredicateRegister(
                const SShaderSourceRegisterToken& predicate );

    void    SetSourceRegister(
                const unsigned int index,
                const SShaderSourceRegisterToken& source );

    void    SetSourceImmediateValue(
                const unsigned int index,
                const SHADER_CHANNEL channel,
                const void* value );

    void    SetSourceIndirectRegister(
                const unsigned int index,
                const SShaderSourceRegisterToken& source );

    void    SetSourceIndirectRegisterOffset(
                const unsigned int index,
                const long offset );

    void    SetSourceIndirectOffset(
                const unsigned int index,
                const long offset );

    void    SetResourceType(
                const SHADER_RESOURCE_TYPE  resType );

    void    SetSourceAccessRegister(
                const unsigned int index,
                const SShaderSourceRegisterToken& source );

    void    SetSourceAccessRegisterOffset(
                const unsigned int index,
                const long offset );

    void    SetSourceAccessOffset(
                const unsigned int index,
                const long offset );

    void    SetSourceAddressOffset(
                const int u,
                const int v,
                const int w );

    void    SetLabel(
                const unsigned int label );

    void    SetAddressableStreamOutNo(
                const unsigned int addressableStreamOutNo );

    void    SetPrintfDataType(
                const unsigned int index,
                const unsigned int printfType );

    void    SetPrintfDataSize(
                const unsigned int index,
                const long printfSize );

    void    SetIsSinglePassLoop( 
                const bool isSinglePassLoop );

    void    SetProfileInstructionIndex(
                const unsigned int index );

    void    SetILInstructionIndex(
                const unsigned int index );
   

    const SShaderOpcodeToken&               GetHeader( void ) const;

    const SShaderDestinationRegisterToken&  GetDestinationRegister(
                                                const unsigned int index ) const;

    const SShaderSourceRegisterToken&       GetDestinationIndirectRegister(
                                                const unsigned int index ) const;

    long                                    GetDestinationIndirectRegisterOffset(
                                                const unsigned int index ) const;

    long                                    GetDestinationIndirectOffset(
                                                const unsigned int index ) const;

    const SShaderSourceRegisterToken&       GetPredicateRegister( void ) const;

    const SShaderSourceRegisterToken&       GetSourceRegister(
                                                const unsigned int index ) const;

    unsigned int                            GetSourceImmediateValue(
                                                const unsigned int index,
                                                const SHADER_CHANNEL channel ) const;

    const SShaderSourceRegisterToken&       GetSourceIndirectRegister(
                                                const unsigned int index ) const;

    long                                    GetSourceIndirectRegisterOffset(
                                                const unsigned int index ) const;

    long                                    GetSourceIndirectOffset(
                                                const unsigned int index ) const;

    SHADER_RESOURCE_TYPE                    GetResourceType( void ) const;


    const SShaderSourceRegisterToken&       GetSourceAccessRegister(
                                                const unsigned int index ) const;

    long                                    GetSourceAccessRegisterOffset(
                                                const unsigned int index ) const;

    long                                    GetSourceAccessOffset(
                                                const unsigned int index ) const;

    const int&                              GetSourceAddressOffsetU( void ) const;
    const int&                              GetSourceAddressOffsetV( void ) const;
    const int&                              GetSourceAddressOffsetW( void ) const;

    const unsigned int&                     GetLabel( void ) const;

    const unsigned int&                     GetAddressableStreamOutNo( void ) const;

    SShaderOpcodeToken&                     GetHeader( void );

    SShaderDestinationRegisterToken&    GetDestinationRegister(
                                            const unsigned int index );

    SShaderSourceRegisterToken&         GetDestinationIndirectRegister(
                                            const unsigned int index );

    SShaderSourceRegisterToken&         GetPredicateRegister( void );

    SShaderSourceRegisterToken&         GetSourceRegister(
                                            const unsigned int index );

    SShaderSourceRegisterToken&         GetSourceIndirectRegister(
                                            const unsigned int index );

    SShaderSourceRegisterToken&         GetSourceAccessRegister(
                                            const unsigned int index );
    unsigned int                        GetLength( void ) const;

    unsigned int                        GetPrintfDataType(
                                            const unsigned int index ) const;

    long                                GetPrintfDataSize(
                                            const unsigned int index ) const;

    bool                                GetIsSinglePassLoop( void ) const;

    unsigned int                        GetNumDestinations( void ) const;
    unsigned int                        GetNumSources( void ) const;

    unsigned int                        GetProfileInstructionIndex( void ) const;
    unsigned int                        GetILInstructionIndex( void ) const;
    unsigned int                        GetILLineNumber(
                                            const CShaderDebugLocationInfo* locInfo ) const;
    unsigned int                        GetILColumnNumber(
                                            const CShaderDebugLocationInfo* locInfo ) const;
    unsigned int                        GetILFileIndex( 
                                            const CShaderDebugLocationInfo* locInfo ) const;

    bool                                ReturnsMappingStatus( void ) const;

    ErrorCode                           WriteToStream( void* pTokenStream, const unsigned int streamSize ) const;
    ErrorCode                           ReadFromStream( const void* pTokenStream, SHADER_VERSION_TYPE shaderVersion );

    bool                                HasAddressSpaceArgument( void ) const;

    ErrorCode DebugPrint(
        const CShaderDebugLocationInfo* pDebugLocationInfo, 
        char *pOutputBuffer,
        const unsigned int bufferSize ) const;

    ErrorCode DebugPrint(
        char *pOutputBuffer,
        const unsigned int bufferSize ) const;

    inline bool SupportsPredicate( void )
    {
#if defined(IGC_CMAKE)
        return false;
#else
        // It works with IGCStandalone.sln, but not with the solution created by Cmake.
        return OpcodeSupportsPredicate(m_Opcode.GetOpcode()); 
#endif
    }

    unsigned long long  Hash( void ) const;

    ErrorCode  Compile( SHADER_VERSION_TYPE shaderVersion );

    bool UsesPrecision( void ) const;

    void ResetPrecision( void );
    bool ProcessPartialPrecision( void );

#ifdef USC_OPTIMIZING_COMPILER_TESTS
    // Optimizing Compiler tests
    void                SetTestFlags( const OptimizerTestFlags &testFlags );
    const OptimizerTestFlags& GetTestFlags( void ) const;
#endif

private:
    // Disabling shallow copy because the class has pointers.
    CShaderInstruction( const CShaderInstruction& );
    void operator=( const CShaderInstruction& );

protected:
    friend class CShaderInstructionList;
#if defined _USC_
    friend class USC::CShaderIfc;
#endif

#ifdef USC_OPTIMIZING_COMPILER_TESTS
    // Optimizing Compiler tests
    OptimizerTestFlags m_TestFlags;
#endif

    SShaderOpcodeToken                  m_Opcode;

    SShaderDestinationRegisterToken     m_DestinationRegister[MAX_INSTRUCTION_DESTINATION_REGISTERS];
    SShaderSourceRegisterToken          m_DestinationIndirectRegister[MAX_INSTRUCTION_DESTINATION_REGISTERS];
    long                                m_DestinationIndirectRegisterOffset[MAX_INSTRUCTION_DESTINATION_REGISTERS];
    long                                m_DestinationIndirectOffset[MAX_INSTRUCTION_DESTINATION_REGISTERS];

    SShaderSourceRegisterToken          m_PredicateRegister;

    SShaderSourceRegisterToken          m_SourceRegister[MAX_INSTRUCTION_SOURCE_REGISTERS];
    unsigned int                        m_SourceImmediateValue[MAX_INSTRUCTION_SOURCE_REGISTERS][NUM_SHADER_CHANNELS];
    SShaderSourceRegisterToken          m_SourceIndirectRegister[MAX_INSTRUCTION_SOURCE_REGISTERS];
    long                                m_SourceIndirectRegisterOffset[MAX_INSTRUCTION_SOURCE_REGISTERS];
    long                                m_SourceIndirectOffset[MAX_INSTRUCTION_SOURCE_REGISTERS];

    SShaderSourceRegisterToken          m_AccessRegister[MAX_INSTRUCTION_SOURCE_REGISTERS];
    long                                m_AccessRegisterOffset[MAX_INSTRUCTION_SOURCE_REGISTERS];
    long                                m_AccessOffset[MAX_INSTRUCTION_SOURCE_REGISTERS];

    unsigned int                        m_SourcePrintfDataType[MAX_INSTRUCTION_SOURCE_REGISTERS];
    long                                m_SourcePrintfDataSize[MAX_INSTRUCTION_SOURCE_REGISTERS];

    SShaderDestinationRegisterToken*    m_pExtendedDestinationRegister;
    SShaderSourceRegisterToken*         m_pExtendedDestinationIndirectRegister;
    long*                               m_pExtendedDestinationIndirectRegisterOffset;
    long*                               m_pExtendedDestinationIndirectOffset;

    SShaderSourceRegisterToken*         m_pExtendedSourceRegister;
    unsigned int*                       m_pExtendedSourceImmediateValue;
    SShaderSourceRegisterToken*         m_pExtendedSourceIndirectRegister;
    long*                               m_pExtendedSourceIndirectRegisterOffset;
    long*                               m_pExtendedSourceIndirectOffset;

    SShaderSourceRegisterToken*         m_pExtendedAccessRegister;
    long*                               m_pExtendedAccessRegisterOffset;
    long*                               m_pExtendedAccessOffset;

    unsigned int*                       m_pExtendedSourcePrintfDataType;
    long*                               m_pExtendedSourcePrintfDataSize;

    int                                 m_SourceAddressOffset[3];

    unsigned int                        m_Label;

    SHADER_RESOURCE_TYPE                m_ResourceType;
    unsigned int                        m_Length;

    unsigned int                        m_AddressableStreamOutNo;

    unsigned int                        m_MaxDestinationRegisters;
    unsigned int                        m_MaxSourceRegisters;
    
    bool                                m_IsSinglePassLoop;

    unsigned int                        m_ProfileInstructionIndex;
    unsigned int                        m_ILInstructionIndex;
};

} // namespace USC
