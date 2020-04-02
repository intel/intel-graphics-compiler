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
#ifndef _GEOMETRYSHADERPROPERTIES_H_
#define _GEOMETRYSHADERPROPERTIES_H_

#include "ShaderTypesEnum.h"
#include "usc_gen7_types.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderUnits.hpp"

namespace IGC
{

    class GeometryShaderProperties
    {
    public:

        class VertexProperties
        {
        public:
            VertexProperties();

            // ---- setters
            /// Sets whether the shader has declared output registers with clip distance semantics.
            void HasClipDistances(bool hasClipDistances);
            /// Sets whether the shader has declared output registers with cull distance semantics.
            void HasCullDistances(bool hasCullDistances);
            /// Sets the maximum count of attribute declared in the shader program.
            void MaxAttributeCount(unsigned int maxAttributeCount);
            /// Sets the bitmask corresponding to which clip distances are in use.
            /// Least significant bit means clip distance #0 is in use, etc.
            void ClipDistanceMask(unsigned int clipDistanceMask);
            /// Sets the bitmask corresponding to which cull distances are in use.
            /// Least significant bit means cull distance #0 is in use, etc.
            void CullDistanceMask(unsigned int cullDistanceMask);
            // Sets whether or not a vertex header entry is expected to be in the URB.
            void HasVertexHeader(bool hasVertexHeader);

            // ---- getters
            /// Returns true if the shader declares output registers with clip distance semantics.
            bool HasClipDistances() const;
            /// Returns true if the shader declares output registers with cull distance semantics.
            bool HasCullDistances() const;
            /// Returns the bitmask corresponding to which clip distances are in use.
            /// Least significant bit corresponds to clip plane 0, etc..
            unsigned int ClipDistanceMask() const;
            /// Returns the bitmask corresponding to which cull distances are in use.
            /// Least significant bit corresponds to cull plane 0, etc..
            unsigned int CullDistanceMask() const;
            /// Returns the maximum number of attributes defined by the shader program.
            unsigned int MaxAttributeCount() const;
            /// Returns the size of the vertex header entry in octwords.
            /// It can be either 1 or 2 (when clip or cull distances are present in the header).
            OctEltUnit HeaderSize() const;
            /// Returns the total size of the vertex that consists of the size of the vertex header
            /// plus the size of the attribute space.
            QuadEltUnit Size() const;

        private:
            bool m_hasClipDistances;
            bool m_hasCullDistances;
            bool m_hasVertexHeader;
            int m_maxAttributeCount;
            unsigned int m_clipDistanceMask;
            unsigned int m_cullDistanceMask;
        };

        class InputProperties
        {
        public:
            InputProperties();
            InputProperties(
                bool hasPrimID,
                bool hasInstanceID,
                IGC::GSHADER_INPUT_PRIMITIVE_TYPE inputPrimitiveType,
                const VertexProperties& perVertex);

            // ---- setters
            void HasPrimitiveID(bool hasPrimID);
            void HasInstanceID(bool hasInstanceID);
            void InstanceCount(unsigned int instanceCount);
            void InputPrimitiveType(IGC::GSHADER_INPUT_PRIMITIVE_TYPE inputPrimitiveType);

            // ---- getters
            /// True when the shader has declaration with semantics PrimitiveID
            bool HasPrimitiveID() const;
            /// True when the shader has declaration with semantics GSInstanceID
            bool HasInstanceID() const;
            /// True when the shader is run in instancing mode.
            bool HasInstancing() const;
            /// Returns the number of instances of GS spawned in instancing mode.
            unsigned int InstanceCount() const;
            /// Returns the number of vertices in the input primitive.
            unsigned int VertexCount() const;
            /// Returns the kind of the input primitive (e.g. point, line with adj, etc).
            IGC::GSHADER_INPUT_PRIMITIVE_TYPE InputPrimitiveType() const;
            /// Returns the reference to per-vertex properties object.
            const VertexProperties& PerVertex() const { return m_perVertex; }
            VertexProperties& PerVertex() { return m_perVertex; }
        private:
            bool m_hasPrimitiveID;
            bool m_hasInstanceID;
            unsigned int m_instanceCount;
            IGC::GSHADER_INPUT_PRIMITIVE_TYPE m_inputPrimitiveType;
            VertexProperties m_perVertex;
        };

        class OutputProperties
        {
        public:
            OutputProperties();

            // setters
            void HasPrimitiveID(bool hasPrimID);
            void HasRenderTargetArrayIndex(bool hasRTAI);
            void HasViewportArrayIndex(bool hasVPAI);
            void MaxVertexCount(unsigned int maxVertexCount);
            void TopologyType(USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE topologyType);
            void HasNontrivialCuts(bool hasCuts);
            void DefaultStreamID(int defaultStreamID);
            void ControlDataFormat(USC::GFX3DSTATE_CONTROL_DATA_FORMAT format);
            void HasNonstaticVertexCount(bool hasNonstaticVertexCount);
            void ActualStaticVertexCount(unsigned int vertexCount);
            void SetControlDataHeaderPaddingRequired(bool ctrlDataHdrRequired);
            void HasVtxCountMsgHalfCLSize(bool hasVtxCountMsgHalfCLSize);

            // getters
            bool HasPrimitiveID() const;
            bool HasViewportArrayIndex() const;
            bool HasRenderTargetArrayIndex() const;
            int DefaultStreamID() const;
            unsigned int MaxVertexCount() const;
            unsigned int ActualStaticVertexCount() const;
            USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE TopologyType() const;
            bool HasNonstaticVertexCount() const;

            USC::GFX3DSTATE_CONTROL_DATA_FORMAT ControlDataFormat() const;
            OctEltUnit OutputVertexCountSize() const;
            bool ControlDataHeaderRequired() const;
            OctEltUnit ControlDataHeaderSize() const;
            OctEltUnit GlobalHeaderSize() const;

            VertexProperties& PerVertex() { return m_perVertex; };
            const VertexProperties& PerVertex() const { return m_perVertex; };

            bool ControlDataHeaderPaddingRequired() const;
            bool HasVtxCountMsgHalfCLSize() const;


        private:
            bool m_hasPrimitiveID;
            bool m_hasRenderTargetArrayIndex;
            bool m_hasViewportArrayIndex;
            bool m_hasNontrivialCuts;
            bool m_hasNonstaticVertexCount;
            bool m_isCtrlHeaderPaddingRequired;
            bool m_hasVtxCountMsgHalfCLSize;
            unsigned int m_maxVertexCount;
            unsigned int m_actualStaticVertexCount;
            int m_defaultStreamID;
            USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE m_topologyType;
            USC::GFX3DSTATE_CONTROL_DATA_FORMAT m_controlDataFormat;
            unsigned int m_lastNonzeroCutSidDword;
            VertexProperties m_perVertex;
        };

        GeometryShaderProperties();
        GeometryShaderProperties(
            const InputProperties& inputProps,
            const OutputProperties& outputProps);

        const InputProperties& Input() const { return m_input; };
        InputProperties& Input() { return m_input; }
        const OutputProperties& Output() const { return m_output; }
        OutputProperties& Output() { return m_output; }

        void SamplerCount(unsigned int samplerCount);
        unsigned int SamplerCount() const;
    private:
        InputProperties m_input;
        OutputProperties m_output;
        unsigned int m_samplerCount;
    };

}
#endif // _GEOMETRYSHADERPROPERTIES_H_
