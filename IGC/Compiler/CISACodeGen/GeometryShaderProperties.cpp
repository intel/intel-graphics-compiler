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
#include "GeometryShaderProperties.hpp"

using namespace IGC;
///////////////////// Vertex Properties ///////////////////////////////////////
GeometryShaderProperties::VertexProperties::VertexProperties()
    : m_hasClipDistances(false)
    , m_hasCullDistances(false)
    , m_hasVertexHeader(true)
    , m_maxAttributeCount(0)
    , m_clipDistanceMask(0)
    , m_cullDistanceMask(0)
{}

void GeometryShaderProperties::VertexProperties::HasClipDistances(bool hasClipDistances)
{
    m_hasClipDistances = hasClipDistances;
}

void GeometryShaderProperties::VertexProperties::HasCullDistances(bool hasCullDistances)
{
    m_hasCullDistances = hasCullDistances;
}


void GeometryShaderProperties::VertexProperties::MaxAttributeCount(unsigned int maxAttributeCount)
{
    m_maxAttributeCount = maxAttributeCount;
}

void GeometryShaderProperties::VertexProperties::ClipDistanceMask(unsigned int clipDistanceMask)
{
    m_clipDistanceMask = clipDistanceMask;
}

void GeometryShaderProperties::VertexProperties::CullDistanceMask(unsigned int cullDistanceMask)
{
    m_cullDistanceMask = cullDistanceMask;
}

void GeometryShaderProperties::VertexProperties::HasVertexHeader(bool hasVertexHeader)
{
    m_hasVertexHeader = hasVertexHeader;
}

OctEltUnit GeometryShaderProperties::VertexProperties::HeaderSize() const
{
    return OctEltUnit(!m_hasVertexHeader ? 0 : HasClipDistances() || HasCullDistances() ? 2 : 1);
}

QuadEltUnit GeometryShaderProperties::VertexProperties::Size() const
{
    const unsigned int numAttributesPadded = iSTD::Align(m_maxAttributeCount, 2);
    return QuadEltUnit(numAttributesPadded) + HeaderSize();
}

bool GeometryShaderProperties::VertexProperties::HasClipDistances() const
{
    return m_hasClipDistances;
}

bool GeometryShaderProperties::VertexProperties::HasCullDistances() const
{
    return m_hasCullDistances;
}

unsigned int GeometryShaderProperties::VertexProperties::ClipDistanceMask() const
{
    return m_clipDistanceMask;
}

unsigned int GeometryShaderProperties::VertexProperties::CullDistanceMask() const
{
    return m_cullDistanceMask;
}

unsigned int GeometryShaderProperties::VertexProperties::MaxAttributeCount() const
{
    return m_maxAttributeCount;
}

///////////////////// Input Properties ////////////////////////////////////////
GeometryShaderProperties::InputProperties::InputProperties()
    : m_hasPrimitiveID(false)
    , m_hasInstanceID(false)
    , m_instanceCount(0)
    , m_inputPrimitiveType(USC::GSHADER_INPUT_POINT)
{}

GeometryShaderProperties::InputProperties::InputProperties(
    bool hasPrimID,
    bool hasInstanceID,
    USC::GSHADER_INPUT_PRIMITIVE_TYPE inputPrimitiveType,
    const VertexProperties& perVertex)
    : m_hasPrimitiveID(hasPrimID)
    , m_hasInstanceID(hasInstanceID)
    , m_inputPrimitiveType(inputPrimitiveType)
    , m_perVertex(perVertex)
{ }

bool GeometryShaderProperties::InputProperties::HasPrimitiveID() const
{
    return m_hasPrimitiveID;
}

void GeometryShaderProperties::InputProperties::HasPrimitiveID(bool hasPrimID)
{
    m_hasPrimitiveID = hasPrimID;
}

bool GeometryShaderProperties::InputProperties::HasInstanceID() const
{
    return m_hasInstanceID;
}

void GeometryShaderProperties::InputProperties::InstanceCount(unsigned int instanceCount)
{
    m_instanceCount = instanceCount;
}

unsigned int GeometryShaderProperties::InputProperties::InstanceCount() const
{
    return m_instanceCount;
}

bool GeometryShaderProperties::InputProperties::HasInstancing() const
{
    return m_instanceCount > 1;
}


void GeometryShaderProperties::InputProperties::HasInstanceID(bool hasInstanceID)
{
    m_hasInstanceID = hasInstanceID;
}

void GeometryShaderProperties::InputProperties::InputPrimitiveType(
    USC::GSHADER_INPUT_PRIMITIVE_TYPE inputPrimitiveType)
{
    m_inputPrimitiveType = inputPrimitiveType;
}

unsigned int GeometryShaderProperties::InputProperties::VertexCount() const
{
    switch (m_inputPrimitiveType)
    {
    case USC::GSHADER_INPUT_POINT: return 1;
    case USC::GSHADER_INPUT_LINE: return 2;
    case USC::GSHADER_INPUT_LINE_ADJ: return 4;
    case USC::GSHADER_INPUT_TRIANGLE: return 3;
    case USC::GSHADER_INPUT_TRIANGLE_ADJ: return 6;

    case USC::GSHADER_INPUT_PATCHLIST_1: return 1;
    case USC::GSHADER_INPUT_PATCHLIST_2: return 2;
    case USC::GSHADER_INPUT_PATCHLIST_3: return 3;
    case USC::GSHADER_INPUT_PATCHLIST_4: return 4;
    case USC::GSHADER_INPUT_PATCHLIST_5: return 5;
    case USC::GSHADER_INPUT_PATCHLIST_6: return 6;
    case USC::GSHADER_INPUT_PATCHLIST_7: return 7;
    case USC::GSHADER_INPUT_PATCHLIST_8: return 8;
    case USC::GSHADER_INPUT_PATCHLIST_9: return 9;
    case USC::GSHADER_INPUT_PATCHLIST_10: return 10;
    case USC::GSHADER_INPUT_PATCHLIST_11: return 11;
    case USC::GSHADER_INPUT_PATCHLIST_12: return 12;
    case USC::GSHADER_INPUT_PATCHLIST_13: return 13;
    case USC::GSHADER_INPUT_PATCHLIST_14: return 14;
    case USC::GSHADER_INPUT_PATCHLIST_15: return 15;
    case USC::GSHADER_INPUT_PATCHLIST_16: return 16;
    case USC::GSHADER_INPUT_PATCHLIST_17: return 17;
    case USC::GSHADER_INPUT_PATCHLIST_18: return 18;
    case USC::GSHADER_INPUT_PATCHLIST_19: return 19;
    case USC::GSHADER_INPUT_PATCHLIST_20: return 20;
    case USC::GSHADER_INPUT_PATCHLIST_21: return 21;
    case USC::GSHADER_INPUT_PATCHLIST_22: return 22;
    case USC::GSHADER_INPUT_PATCHLIST_23: return 23;
    case USC::GSHADER_INPUT_PATCHLIST_24: return 24;
    case USC::GSHADER_INPUT_PATCHLIST_25: return 25;
    case USC::GSHADER_INPUT_PATCHLIST_26: return 26;
    case USC::GSHADER_INPUT_PATCHLIST_27: return 27;
    case USC::GSHADER_INPUT_PATCHLIST_28: return 28;
    case USC::GSHADER_INPUT_PATCHLIST_29: return 29;
    case USC::GSHADER_INPUT_PATCHLIST_30: return 30;
    case USC::GSHADER_INPUT_PATCHLIST_31: return 31;
    case USC::GSHADER_INPUT_PATCHLIST_32: return 32;

    default:
        return 0;
    }
}

USC::GSHADER_INPUT_PRIMITIVE_TYPE GeometryShaderProperties::InputProperties::InputPrimitiveType() const
{
    return m_inputPrimitiveType;
}

///////////////////// Output Properties ///////////////////////////////////////
GeometryShaderProperties::OutputProperties::OutputProperties()
    : m_hasPrimitiveID(false)
    , m_hasRenderTargetArrayIndex(false)
    , m_hasViewportArrayIndex(false)
    , m_hasNontrivialCuts(false)
    , m_hasNonstaticVertexCount(false)
    , m_isCtrlHeaderPaddingRequired(false)
    , m_hasVtxCountMsgHalfCLSize(false)
    , m_maxVertexCount(0)
    , m_actualStaticVertexCount(0)
    , m_defaultStreamID(0)
    , m_topologyType(USC::GFX3DPRIM_POINTLIST)
    , m_controlDataFormat(USC::GFX3DSTATE_CONTROL_DATA_FORMAT_CUT)
    , m_lastNonzeroCutSidDword(0)
{ }

void GeometryShaderProperties::OutputProperties::MaxVertexCount(unsigned int maxVertexCount)
{
    m_maxVertexCount = maxVertexCount;
}

void GeometryShaderProperties::OutputProperties::ActualStaticVertexCount(unsigned int actualCount)
{
    m_actualStaticVertexCount = actualCount;
}

void GeometryShaderProperties::OutputProperties::TopologyType(
    USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    m_topologyType = topologyType;
}

void GeometryShaderProperties::OutputProperties::HasPrimitiveID(bool hasPrimID)
{
    m_hasPrimitiveID = hasPrimID;
}

void GeometryShaderProperties::OutputProperties::HasRenderTargetArrayIndex(bool hasRTAI)
{
    m_hasRenderTargetArrayIndex = hasRTAI;
}

void GeometryShaderProperties::OutputProperties::HasViewportArrayIndex(bool hasVPAI)
{
    m_hasViewportArrayIndex = hasVPAI;
}

void GeometryShaderProperties::OutputProperties::HasNontrivialCuts(bool hasCuts)
{
    m_hasNontrivialCuts = hasCuts;
}

void GeometryShaderProperties::OutputProperties::DefaultStreamID(int defaultStreamID)
{
    m_defaultStreamID = defaultStreamID;
}

void GeometryShaderProperties::OutputProperties::ControlDataFormat(
    USC::GFX3DSTATE_CONTROL_DATA_FORMAT format)
{
    m_controlDataFormat = format;
}

void GeometryShaderProperties::OutputProperties::HasNonstaticVertexCount(
    bool hasNonstaticVertexCount)
{
    m_hasNonstaticVertexCount = hasNonstaticVertexCount;
}


void IGC::GeometryShaderProperties::OutputProperties::SetControlDataHeaderPaddingRequired(bool ctrlDataHdrRequired)
{
    m_isCtrlHeaderPaddingRequired = ctrlDataHdrRequired;
}

int GeometryShaderProperties::OutputProperties::DefaultStreamID() const
{
    return m_defaultStreamID;
}

unsigned int GeometryShaderProperties::OutputProperties::MaxVertexCount() const
{
    return m_maxVertexCount;
}

unsigned int GeometryShaderProperties::OutputProperties::ActualStaticVertexCount() const
{
    return m_actualStaticVertexCount;
}

/// We need OutputVerteCount field in the header if the count of emitted vertices
/// is a runtime value.
bool GeometryShaderProperties::OutputProperties::HasNonstaticVertexCount() const
{
    return m_hasNonstaticVertexCount;
}

OctEltUnit GeometryShaderProperties::OutputProperties::OutputVertexCountSize() const
{
    return OctEltUnit(m_hasNonstaticVertexCount ? 1 : 0);
}

USC::GFX3DSTATE_CONTROL_DATA_FORMAT GeometryShaderProperties::OutputProperties::ControlDataFormat() const
{
    return m_controlDataFormat;
}


bool GeometryShaderProperties::OutputProperties::HasViewportArrayIndex() const
{
    return m_hasViewportArrayIndex;
}

bool GeometryShaderProperties::OutputProperties::HasRenderTargetArrayIndex() const
{
    return m_hasRenderTargetArrayIndex;
}

/// Returns whether GS needs to write to URB control data header data.
/// Conditions: 
///  - When we emit to more than one stream, we need the header to keep stream ID bits
///  - When cuts have appeared and output topology type is different than point list.
bool GeometryShaderProperties::OutputProperties::ControlDataHeaderRequired() const
{
    return (m_hasNontrivialCuts && m_topologyType != USC::GFX3DPRIM_POINTLIST) ||
        (m_controlDataFormat == USC::GFX3DSTATE_CONTROL_DATA_FORMAT_SID) ||
        (m_isCtrlHeaderPaddingRequired);
}

bool GeometryShaderProperties::OutputProperties::ControlDataHeaderPaddingRequired() const
{
    return m_isCtrlHeaderPaddingRequired;
}

OctEltUnit GeometryShaderProperties::OutputProperties::ControlDataHeaderSize() const
{
    if (!ControlDataHeaderRequired())
    {
        return OctEltUnit(0);
    }

    if (m_isCtrlHeaderPaddingRequired)
    {
        OctEltUnit ctrlDataHdrSize = OctEltUnit(0);
        auto numVertices = MaxVertexCount();

        if (numVertices <= 256)
            ctrlDataHdrSize = OctEltUnit(1);    // + NonstaticVertexCount = 1CL
        else if (numVertices > 256 && numVertices <= 768)
            ctrlDataHdrSize = OctEltUnit(3);    // + NonstaticVertexCount = 2CLs
        else // numVertices <768, 1024>
            ctrlDataHdrSize = OctEltUnit(5);    // + NonstaticVertexCount = 3CLs

        return ctrlDataHdrSize;
    }

    auto numVertices = HasNonstaticVertexCount() ? MaxVertexCount() : ActualStaticVertexCount();
    auto bitsPerVertex = (ControlDataFormat() == USC::GFX3DSTATE_CONTROL_DATA_FORMAT_CUT) ? 1 : 2;
    return numVertices > 255 ? OctEltUnit(2 * bitsPerVertex) : OctEltUnit(1 * bitsPerVertex);
}

OctEltUnit GeometryShaderProperties::OutputProperties::GlobalHeaderSize() const
{
    return OutputVertexCountSize() + ControlDataHeaderSize();
}

USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE GeometryShaderProperties::OutputProperties::TopologyType() const
{
    return m_topologyType;
}

void IGC::GeometryShaderProperties::OutputProperties::HasVtxCountMsgHalfCLSize(bool hasVtxCountMsgHalfCLSize)
{
    m_hasVtxCountMsgHalfCLSize = hasVtxCountMsgHalfCLSize;
}
bool GeometryShaderProperties::OutputProperties::HasVtxCountMsgHalfCLSize() const
{
    return m_hasVtxCountMsgHalfCLSize;
}
///////////////////// Geometry Shader Properties //////////////////////////////
GeometryShaderProperties::GeometryShaderProperties()
    : m_samplerCount(0)
{

}

GeometryShaderProperties::GeometryShaderProperties(
    const InputProperties& inputProps,
    const OutputProperties& outputProps)
    : m_input(inputProps)
    , m_output(outputProps)
    , m_samplerCount(0)
{
}

void GeometryShaderProperties::SamplerCount(unsigned int samplerCount)
{
    m_samplerCount = samplerCount;
}

unsigned int GeometryShaderProperties::SamplerCount() const
{
    return m_samplerCount;
}
