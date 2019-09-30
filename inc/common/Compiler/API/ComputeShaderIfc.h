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

#include "ShaderTypes.h"
#include "ShaderToken.h"
#include "ErrorCode.h"

namespace iSTD
{
template<class Type>
struct SVector3;
}

namespace USC
{
// Forward declarations
class CComputeShader; 
class CShaderInstruction;
class CShaderCompiler;
class CShaderDecl;
class CShaderResourceDecl;
class CShaderSamplerDecl;
class CShaderConstantBufferDecl;
class CShaderInterfaceDecl;
class CShaderFunctionTableDecl;
class CShaderFunctionBodyDecl;
class CShaderInputDecl;
class CShaderOutputDecl;
class CShaderIndexedTemporaryArrayDecl;
class CShaderDebugLocationInfo;
struct SCompilerOutputComputeShader_Gen7;
typedef iSTD::SVector3<unsigned int> SThreadGroup;

class CComputeShaderIfc
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Shader Declarations
    //////////////////////////////////////////////////////////////////////////

    virtual ~CComputeShaderIfc() {};

    // Shader Input Declarations
    virtual void    DeclareInput(
        const unsigned int number,
        const SHADER_USAGE usage,
        const SHADER_MASK mask ) = 0;

    virtual void    DeclareIndexedInput(
                        const unsigned int min,
                        const unsigned int max ) = 0;

    virtual void    DeclareIndexedOutput(
                        const unsigned int min,
                        const unsigned int max ) = 0;

    virtual void    DeclareOutput(
                        const unsigned int number,
                        const SHADER_USAGE usage,
                        const unsigned int usageIndex,
                        const SHADER_MASK mask ) = 0;

    // Shader register declarations 
    virtual void    DeclareTemporaryRegister(
                const unsigned int number ) = 0;

    virtual void    DeclareTemporaryRegisters(
                const unsigned int count ) = 0;

    virtual void    DeclareIndexedTemporaryArray(
                const unsigned int number,
                const unsigned int size ) = 0;

    virtual void    DeclareConstantRegister(
                const unsigned int number ) = 0;

    virtual void    DeclareCycleCounter( void ) = 0;

    // Shader resource declarations 
    virtual void     DeclareConstantBuffer(
                const unsigned int number ) = 0;

    virtual void DeclareImmConstantBuffer(
        const unsigned int sizeInUints,
        const unsigned int* srcBuffer) = 0;

    virtual void    DeclareSampler( 
                const unsigned int number,
                const SHADER_SAMPLER_TYPE type ) = 0;

    virtual void    DeclareUAVTyped(
                const unsigned int number, 
                const SHADER_RESOURCE_TYPE dimension,
                const SHADER_RESOURCE_RETURN_TYPE returnType,
                const bool globallyCoherent ) = 0;
    
    virtual void    DeclareUAVRaw(
                const unsigned int number,
                const bool globallyCoherent ) = 0;
                
    virtual void    DeclareUAVStructured(
                const unsigned int number,
                const unsigned int stride,
                const bool globallyCoherent ) = 0;

    virtual void    DeclareJournalStructured(
                const unsigned int number,
                const unsigned int stride ) = 0;

    virtual void    DeclareResource( 
                const unsigned int number, 
                const SHADER_RESOURCE_TYPE resourceType,
                const SHADER_RESOURCE_RETURN_TYPE returnType ) = 0;
    
    virtual void    DeclareResourceRaw(
                const unsigned int number ) = 0;
                        
    virtual void    DeclareResourceStructured(
                const unsigned int number,
                const unsigned int stride ) = 0;

    // Shader Thread Group Dimensions Declaration
    virtual void    DeclareThreadGroupSize(
                const unsigned int dimX,
                const unsigned int dimY,
                const unsigned int dimZ ) = 0;

    virtual void    DeclareVariableThreadGroupSize( void ) = 0;

    // Shader Simd Size Declaration
    virtual void    DeclareSimdSize(
                const unsigned int size ) = 0;

    // Shader Thread Group Shared Memory Declarations
    virtual void    DeclareTGSMRaw(
                const unsigned int number,
                const unsigned int byteCount,
                const unsigned int alignment ) = 0;

    virtual void    DeclareTGSMStructured(
                const unsigned int number,
                const unsigned int stride,
                const unsigned int structCount ) = 0;

    virtual void    DeclareTGSMVariable(
                const unsigned int number,
                const unsigned int alignment ) = 0;

    virtual void    DeclareTPM(
                const unsigned int number,
                const unsigned int byteCount,
                const unsigned int alignment ) = 0;

    virtual void    DeclarePointer( const unsigned int number ) = 0;

    virtual void    DeclareString( const unsigned int number ) = 0;

    virtual void    DeclareFloatDenormMode(
                const USC_FLOAT_PRECISION floatPrecision,
                const USC_FLOAT_DENORM_MODE denormMode ) = 0;

    virtual void    DeclareEarlyDepthStencilTestMode( USC::USC_EARLY_DEPTH_STENCIL_TEST_MODE testMode ) = 0;

    // Shader Thread Group Dimensions Declaration Accessors Functions
    virtual bool DeclaresFixedThreadGroupDimensions( void ) const = 0;
    virtual bool DeclaresVariableThreadGroupDimensions( void ) const = 0;
    virtual const SThreadGroup&  GetThreadGroupDimensions( void ) const = 0;

    virtual ErrorCode AddInstruction(
                CShaderInstruction& instruction,
                unsigned int lineColumnNumber = 0, 
                unsigned int fileIndex = 0 ) = 0;

    virtual ErrorCode GetInstructionCopy(
                const unsigned int index,
                CShaderInstruction& instruction ) const = 0;

    virtual ErrorCode GetInstructionPointer(
        const unsigned int index,
        CShaderInstruction*& instruction ) const = 0;

    virtual bool    SetShaderCompiler( CShaderCompiler* pCompiler ) = 0;

    virtual bool    IsCompiled( void ) const = 0;

    virtual bool    IsGenericsUsed( void ) const = 0;

    virtual bool    HasPrintfConstantRegister( void ) const = 0;

    // These two functions are for checking compiler options
    virtual bool    IsKernelDebugEnableSet( void ) const = 0;
    virtual void    SetKernelDebugEnable( void ) = 0;

    // Shader Thread Group Shared Memory Declarations Object Accessor Function
    virtual const   CShaderResourceDecl* GetTGSMDeclarations( void ) const = 0;

    // Shader Thread Private Memory Declarations Object Accessor Function
    virtual const CShaderResourceDecl* GetTPMDeclarations( void ) const = 0;

    virtual void    SetTracingOptions( 
        const char*         pKernelName,
        const unsigned int  kernelIndex, 
        const unsigned int  kernelsCount,
        const unsigned int  optionsCount,
        const void*         pOptions ) = 0;

    // Shader GHAL3D IL debug printfs
    virtual void    Print( void ) const = 0;

    virtual void    SetIsApiShader( bool isApi ) = 0;
    virtual void    SetHashCode( const unsigned long long hash ) = 0;
    virtual void    SetClientString( const char* str ) = 0;
    virtual void    SetShaderOrdinal( const unsigned int id ) = 0;
    virtual unsigned long long   GetHashCode( void ) const = 0;

    virtual const CShaderInterfaceDecl*      GetInterfaceDeclarations( void ) const = 0;
    virtual const CShaderFunctionTableDecl*  GetFunctionTableDeclarations( void ) const = 0;
    virtual const CShaderFunctionBodyDecl*   GetFunctionBodyDeclarations( void ) const = 0;

    virtual const CShaderInputDecl*          GetInputRegisterDeclarations( void ) const = 0;
    virtual const CShaderOutputDecl*         GetOutputRegisterDeclarations( void ) const = 0;

    virtual const CShaderDecl*               GetTemporaryRegisterDeclarations( void ) const = 0;
    virtual const CShaderIndexedTemporaryArrayDecl*     GetIndexedTemporaryArrayDeclarations( void ) const = 0;

    virtual const CShaderResourceDecl*       GetResourceDeclarations( void ) const = 0;
    virtual const CShaderSamplerDecl*        GetSamplerDeclarations( void ) const = 0;

    virtual const CShaderResourceDecl*       GetUAViewDeclarations( void ) const = 0;
    virtual const CShaderResourceDecl*       GetJournalDeclarations( void ) const = 0;


    virtual const CShaderDecl*               GetConstantRegisterDeclarations( void ) const = 0;
    virtual const CShaderConstantBufferDecl* GetConstantBufferDeclarations( void ) const = 0;

    virtual const CShaderDecl*               GetStringDeclarations( void ) const = 0;

    virtual unsigned int                     GetPrintfConstantRegister( void ) const = 0;

    virtual const CShaderDebugLocationInfo*  GetShaderDebugLocationInfo( void ) const = 0;

    virtual void                             GetInstructionRawSourceLocation(
                                                 unsigned int ilInstructionIndex, 
                                                 unsigned int& lineCol, 
                                                 unsigned int& fileIndex ) const = 0;

    virtual void                             GetInstructionSourceLocation(
                                                 unsigned int ilInstructionIndex, 
                                                 unsigned int& line, 
                                                 unsigned int& col, 
                                                 unsigned int& fileIndex ) const = 0;

    virtual ErrorCode       BuildLocationMap() = 0;

    virtual void            SetLineNumber( const unsigned int ) = 0;
    virtual unsigned int    GetLineNumber( void ) const  = 0;
    virtual void            SetFileIndex( const unsigned int )  = 0;
    virtual unsigned int    GetFileIndex( void ) const = 0;
};

}
