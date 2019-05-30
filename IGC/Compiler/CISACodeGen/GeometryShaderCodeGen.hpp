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

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "GeometryShaderProperties.hpp"
#include "Compiler/CodeGenPublic.h"

#include "ShaderTypesEnum.h"
#include "Compiler/CISACodeGen/ShaderUnits.hpp"

namespace IGC
{

class CGeometryShader : public CShader
{
public:
    CGeometryShader(llvm::Function *pFunc, CShaderProgram* pProgram);
    ~CGeometryShader();

    /// This is a place to set up required codegen state before we start emitting
    /// instructions in EmitPass.
    virtual void PreCompile() override;

    virtual void AddPrologue() override;

    /// Allocate registers corresponding to input data sent in the payload.
    virtual void AllocatePayload() override;

    /// Set helper pass for this shader type
    virtual  void SetShaderSpecificHelper(EmitPass* emitPass) override;

    /// Fills in the kernel program structure with data determined during compilation.
    void FillProgram(SGeometryShaderKernelProgram* pKernelProgram);

    /// Returns a V-ISA variable that gets initialized to input PrimitiveID payload values.
    CVariable* GetPrimitiveID();

    /// Returns a V-ISA variable that gets initialized to Instance ID values
    CVariable* GetInstanceID();

    /// Returns the number of vertices of the particular input primitive type.
    static uint GetInputPrimitiveVertexCount(USC::GSHADER_INPUT_PRIMITIVE_TYPE inpPrimType);

    /// Returns a variable that stores URB write handle register
    CVariable* GetURBOutputHandle() override;

    /// Returns a variable that has the right URB read handle register.
    /// Note that for GS we have as many urb read handles as vertices in input primitive
    /// and we can address them indirectly.
    CVariable* GetURBInputHandle(CVariable* pVertexIndex) override;

    /// Add an epilogue which could check if we are terminating with a URB write or not.
    void AddEpilogue(llvm::ReturnInst* ret) override;

private:
    /// False if we expect to see also vertices adjacent to the input as part of the input.
    /// field 3DSTATE_GS::DiscardAdjacency.
    static bool DiscardAdjacency(USC::GSHADER_INPUT_PRIMITIVE_TYPE inpPrimType);

    /// Returns the index of a channel where the data with given SGV usage is placed.
    static Unit<Element> GetLocalOffset(SGVUsage usage);

    /// Returns the GS URB allocation size.
    /// This is the size of GS URB entry consisting of the header data and all vertex URB entries.
    /// Unit: 64B = 16 DWORDs
    URBAllocationUnit GetURBAllocationSize() const;

    /// Returns the size of the output vertex.
    /// Unit: 16B = 4 DWORDs.
    QuadEltUnit  GetOutputVertexSize() const;

    /// Returns the size of the vertex entry read used to load payload registers.
    // Unit: 32B = 8DWORDs.
    OctEltUnit  GetVertexEntryReadLength() const;

    /// Returns the size of the input vertex header (containing e.g. Position).
    OctEltUnit GetInputVertexHeaderSize() const;

    // Returns the size of the (output) Vertex URB Entry Header.
    // Unit: 32B = 8DWORDs.
    OctEltUnit  GetVertexURBEntryHeaderSize() const;

    /// Returns the length of the URB read that should be performed by SBE
    /// when reading data written by Geometry Shader.
    /// Unit: 32B = 8DWORDs.
    /// This value is used to set the corresponding field in 3DSTATE_GS.
    OctEltUnit  GetVertexURBEntryOutputReadLength() const;

    /// Returns the offset that SBE should use when reading the URB entries
    /// output by Geometry shader.
    /// This value is used to set the corresponding field in 3DSTATE_GS.
    /// Unit: 32B = 8DWORDS.
    OctEltUnit  GetVertexURBEntryOutputReadOffset() const;

    /// Returns  a(newly allocated if not already present) variable that keeps input vertex
    /// URB handles used for pull model data reads.
    CVariable* GetURBReadHandlesReg();

    //********** Input State data

    /// Stores an array of vertex URB Read handles that are used for pull model data reads.
    CVariable*             m_pURBReadHandlesReg;

    /// Variable that keeps the instance ID for the current GS instance if instancing is enabled.
    CVariable*             m_pInstanceID;

    //********* Output State data

    /// This variable holds the URB handle for the output vertices.
    CVariable*             m_pURBWriteHandleReg;

    /// Holds v-isa variable that keeps PrimitiveID value for each primitive.
    CVariable*             m_pPrimitiveID;

    /// Keeps information about all the properties of the GS program, its inputs and outputs.
    GeometryShaderProperties m_properties;
};
} // End of Namespace
