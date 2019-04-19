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

#include "usc.h"
#include "usc_gen7.h"
#include "usc_gen9.h"
#include "common/Stats.hpp"
#include "common/Types.hpp"
#include "common/allocator.h"
#include "common/igc_resourceDimTypes.h"
// hack
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include <set>
#include <string.h>
#include "Compiler/CISACodeGen/ShaderUnits.hpp"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "Compiler/CISACodeGen/DriverInfo.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "../AdaptorOCL/OCL/sp/spp_g8.h"
#include "../GenISAIntrinsics/GenIntrinsics.h"
#include "../GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/Function.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "common/LLVMWarningsPop.hpp"

#include "CodeGenPublicEnums.h"
#include "AdaptorOCL/TranslationBlock.h"

#include "common/MDFrameWork.h"

#include "CompilerStats.h"

/************************************************************************
This file contains the interface structure and functions to communicate
between front ends and code generator
************************************************************************/

namespace llvm
{
    class Module;
    class Function;
}

#define MAX_VSHADER_INPUT_REGISTERS_PACKAGEABLE 32
static const unsigned int g_c_Max_PS_attributes = 32;

namespace IGC
{
    class CodeGenContext;
    class PixelShaderContext;
    class ComputeShaderContext;

    struct SProgramOutput
    {
        void*           m_programBin;     //<! Must be 16 byte aligned, and padded to a 64 byte boundary
        unsigned int    m_programSize;    //<! Number of bytes of program data (including padding)
        unsigned int    m_unpaddedProgramSize;      //<! program size without padding used for binary linking
        unsigned int    m_startReg;                 //<! Which GRF to start with
        unsigned int    m_scratchSpaceUsedBySpills; //<! amount of scratch space needed for shader spilling
        unsigned int    m_scratchSpaceUsedByShader; //<! amount of scratch space needed by shader if allocated in scratchspace
        unsigned int    m_scratchSpaceUsedByStateless; //<! amount of scratch space needed by shader if allocated in stateless surface
        unsigned int    m_scratchSpaceUsedByGtpin; //<! amount of scratch space used by gtpin
        void*           m_debugDataVISA;            //<! VISA debug data (source -> VISA)
        unsigned int    m_debugDataVISASize;        //<! Number of bytes of VISA debug data
        void*           m_debugDataGenISA;          //<! GenISA debug data (VISA -> GenISA)
        unsigned int    m_debugDataGenISASize;      //<! Number of bytes of GenISA debug data
        unsigned int    m_InstructionCount;
        void*           m_gtpinBuffer;              // Will be populated by VISA only when special switch is passed by gtpin
        unsigned int    m_gtpinBufferSize;
        void*           m_funcSymbolTable;
        unsigned int    m_funcSymbolTableSize;
        unsigned int    m_funcSymbolTableEntries;
        void*           m_funcRelocationTable;
        unsigned int    m_funcRelocationTableSize;
        unsigned int    m_funcRelocationTableEntries;
        unsigned int    m_offsetToSkipPerThreadDataLoad = 0;
        //true means we separate pvtmem and spillfill. pvtmem could go into stateless.
        //false means all of them are together
        bool            m_separatePvtSpill = false;
    
        void Destroy()
        {
            if (m_programBin)
            {
                IGC::aligned_free(m_programBin);
            }
            if (m_debugDataVISA)
            {
                IGC::aligned_free(m_debugDataVISA);
            }
            if (m_debugDataGenISA)
            {
                IGC::aligned_free(m_debugDataGenISA);
            }
        }
        
        void setSeparatePvtSpill(bool setSeparatePvtSpillT)
        {
            m_separatePvtSpill = setSeparatePvtSpillT;
        }

        unsigned int getScratchSpaceUsage() const
        {
            return m_scratchSpaceUsedBySpills + m_scratchSpaceUsedByGtpin + (m_separatePvtSpill ? 0 : m_scratchSpaceUsedByShader);
        }

        unsigned int getScratchPrivateUsage() const
        {
            return (m_separatePvtSpill ? m_scratchSpaceUsedByShader : 0);
        }

        void setScratchPrivateUsage(unsigned int scratchPrivateUsage, unsigned int scratchSpaceSizeLimit)
        {
            if (m_separatePvtSpill && scratchPrivateUsage > scratchSpaceSizeLimit)
            {
                m_scratchSpaceUsedByStateless = scratchPrivateUsage;
            }
            else
            {
                m_scratchSpaceUsedByShader = scratchPrivateUsage;
            }
        }

        unsigned int getStatelessPrivateUsage() const
        {
            return (m_separatePvtSpill ? m_scratchSpaceUsedByStateless : 0);
        }
    };
    
    enum InstrStatTypes
    {
        SROA_PROMOTED,
        TOTAL_TYPES
    };
    enum InstrStatStage
    {
        BEGIN,
        END,
        EXCEED_THRESHOLD,
        TOTAL_STAGE
    };

    struct SInstrTypes
    {
        bool CorrelatedValuePropagationEnable;
        bool hasLoop;
        bool hasMultipleBB;
        bool hasCmp;
        bool hasSwitch;
        bool hasPhi;
        bool hasLoadStore;
        bool hasCall;
        bool hasIndirectCall;
        bool hasInlineAsm;
        bool hasInlineAsmPointerAccess;
        bool hasIndirectBranch;
        bool hasFunctionAddressTaken;
        bool hasSel;
        bool hasPointer;
        bool hasLocalLoadStore;
        bool hasSubroutines;
        bool hasPrimitiveAlloca;
        bool hasNonPrimitiveAlloca;
        bool hasReadOnlyArray;
        bool hasBuiltin;
        bool psHasSideEffect;     //<! only relevant to pixel shader, has other memory writes besides RTWrite
        bool hasGenericAddressSpacePointers;
        bool hasDebugInfo;        //<! true only if module contains debug info !llvm.dbg.cu
        bool hasAtomics;
        bool hasBarrier;        //<! true if module has thread group barrier
        bool hasDiscard;
        bool mayHaveIndirectOperands;  //<! true if code may have indirect operands like r5[a0].
        bool hasUniformAssumptions;
        unsigned int numSample;
        unsigned int numBB;
        unsigned int numLoopInsts;
        unsigned int numOfLoop;
        unsigned int numInsts;    //<! measured after optimization, used as a compiler heuristic
        bool sampleCmpToDiscardOptimizationPossible;
        unsigned int sampleCmpToDiscardOptimizationSlot;
    };

    struct SSimplePushInfo
    {
        uint m_cbIdx = 0;
        uint m_offset = 0;
        uint m_size = 0;
        bool isStateless = false;
    };

    struct SConstantAddrValue
    {
        ConstantAddress ca;
        bool anyValue;
        uint32_t value;
    };

    struct SKernelProgram
    {
        SProgramOutput simd8;
        SProgramOutput simd16;
        SProgramOutput simd32;
        unsigned int bindingTableEntryCount;

        char* gatherMap;
        unsigned int gatherMapSize;
        unsigned int ConstantBufferLength;
        unsigned int ConstantBufferMask;
        unsigned int MaxNumberOfThreads;
        bool         isMessageTargetDataCacheDataPort;

        unsigned int NOSBufferSize;
        unsigned int ConstantBufferLoaded;
        bool         hasControlFlow;
        unsigned int bufferSlot;
        unsigned int statelessCBPushedSize;

        // GenUpdateCB outputs
        void        *m_ConstantBufferReplaceShaderPatterns;
        uint        m_ConstantBufferReplaceShaderPatternsSize;
        uint        m_ConstantBufferUsageMask;
        uint        m_ConstantBufferReplaceSize;

        SSimplePushInfo simplePushInfoArr[g_c_maxNumberOfBufferPushed];

        // Interesting constants for dynamic constant folding
        std::vector<SConstantAddrValue> m_pInterestingConstants;
    };

    struct SPixelShaderKernelProgram : SKernelProgram
    {
        USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT attributeActiveComponent[g_c_Max_PS_attributes];
        DWORD m_AccessedBySampleC[4];

        unsigned int nbOfSFOutput;
        unsigned int renderTargetMask;
        unsigned int constantInterpolationEnableMask;
        unsigned int primIdLocation;
        unsigned int samplerCount;
        unsigned int BindingTableEntryBitmap;
        unsigned int sampleCmpToDiscardOptimizationSlot;

        unsigned char OutputUseMask[USC::NUM_PSHADER_OUTPUT_REGISTERS];

        bool needPerspectiveBary;
        bool needPerspectiveCentroidBary;
        bool needPerspectiveSampleBary;
        bool needNonPerspectiveBary;
        bool needNonPerspectiveCentroidBary;
        bool needNonPerspectiveSampleBary;
        bool needSourceDepth;
        bool needSourceW;
        bool hasInputCoverageMask;
        bool hasPullBary;
        bool killPixel;
        bool outputDepth;
        bool outputStencil;
        bool isPerSample;
        bool oMask;
        bool VectorMask;

        bool hasPrimID;
        bool isCoarsePS;
        bool hasCoarsePixelSize;
        bool hasSampleOffset;
        bool hasZWDelta;
        bool posXYOffsetEnable;
        bool blendToFillEnabled;
        bool forceEarlyZ;
        bool hasEvalSampler;

        bool sampleCmpToDiscardOptimizationPossible;

        bool needPSSync;
    };

    /// Gen10+, corresponds to 3DSTATE_VF_SGVS_2 as described below
    struct SVertexFetchSGVExtendedParameters
    {
        struct
        {
            bool enabled = false;      //<! XPn Enable = XPn Source Select = (*)
            unsigned int location = 0; //<! Linear offset of the 32bit component in VUE
        } extendedParameters[3] = {};  //<! Order of elements: VF_XP0, VF_XP1, VF_XP2
    };

    struct SVertexShaderKernelProgram : SKernelProgram
    {
        /// corresponds to 3DSTATE_VS Vertex URB Entry Read Length
        OctEltUnit VertexURBEntryReadLength;
        /// corresponds to 3DSTATE_VS Vertex URB Entry Read Offset
        OctEltUnit VertexURBEntryReadOffset;
        /// corresponds to 3DSTATE_VS VS Vertex URB Entry Output Length
        OctEltUnit VertexURBEntryOutputReadLength;
        /// corresponds to 3DSTATE_VS VS Vertex URB Entry Output Offset
        OctEltUnit VertexURBEntryOutputReadOffset;
        /// corresponds to 3DSTATE_SBE Vertex URB Entry Read Offset
        OctEltUnit SBEURBReadOffset;
        OctEltUnit URBAllocationSize;
        QuadEltUnit MaxNumInputRegister;
        
        bool enableElementComponentPacking;
        /// corresponds to 3DSTATE_VF_COMPONENT_PACKING
        unsigned char ElementComponentDeliverMask[32];
        /// vertex ID information
        bool         hasVertexID;
        unsigned int vertexIdLocation;
        /// instance ID information
        bool         hasInstanceID;
        unsigned int instanceIdLocation;
        bool         singleInstanceVertexShader;
        /// corresponds to 3DSTATE_VF_SGVS_2
        SVertexFetchSGVExtendedParameters vertexFetchSGVExtendedParameters;
        //RTAI and VPAI
        bool         DeclaresRTAIndex;
        bool         DeclaresVPAIndex;

        DWORD        m_AccessedBySampleC[4];
        bool         HasClipCullAsOutput;
        bool         EnableVertexReordering;

        unsigned int BindingTableEntryBitmap;
        unsigned int m_SamplerCount;
    };

    struct SGeometryShaderKernelProgram : SKernelProgram
    {
        // Gen 7 defined ones
        USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE OutputTopology;
        unsigned int SamplerCount;
        QuadEltUnit  OutputVertexSize;
        OctEltUnit   VertexEntryReadLength;   // URB Entry Read Length
        OctEltUnit   VertexEntryReadOffset;
        bool         IncludeVertexHandles;
        USC::GFX3DSTATE_CONTROL_DATA_FORMAT ControlDataHeaderFormat;
        OctEltUnit   ControlDataHeaderSize;
        unsigned int DefaultStreamID;
        unsigned int InstanceCount;
        USC::GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE DispatchMode;
        bool         IncludePrimitiveIDEnable;
        bool         ReorderEnable;
        bool         DiscardAdjacencyEnable;
        OctEltUnit   SBEVertexURBEntryReadOffset;
        URBAllocationUnit URBAllocationSize;
        unsigned int UserClipDistancesMask;
        unsigned int UserCullDistancesMask;
        unsigned int MaxOutputVertexCount;
        unsigned int BindingTableEntryBitmap;

        bool         DeclaresVPAIndex;
        bool         DeclaresRTAIndex;

        USC::GFX3DSTATE_PROGRAM_FLOW SingleProgramFlow;
        bool GSEnable;

        // Gen 8 defined ones
        unsigned int ExpectedVertexCount;
        unsigned int StaticOutputVertexCount;
        OctEltUnit GSVertexURBEntryOutputReadOffset;
        OctEltUnit GSVertexURBEntryOutputReadLength;

        bool StaticOutput;

        DWORD m_AccessedBySampleC[4];

        bool m_bCanEnableRectList;
    };

    struct SComputeShaderKernelProgram : SKernelProgram
    {
        USC::GFX3DSTATE_FLOATING_POINT_MODE FloatingPointMode;
        USC::GFX3DSTATE_PROGRAM_FLOW        SingleProgramFlow;

        unsigned int                        SamplerCount;
        unsigned int                        BindingTableEntryCount;
        unsigned int                        CurbeReadOffset;
        unsigned int                        CurbeReadLength;
        unsigned int                        PhysicalThreadsInGroup;

        bool                                BarrierUsed;

        USC::GFX3DSTATE_ROUNDING_MODE       RoundingMode;

        unsigned int                        BarrierReturnGRFOffset;

        int                                 GtwBypass;
        int                                 GtwResetTimer;

        unsigned int                        URBEntriesNum;
        unsigned int                        URBEntryAllocationSize;
        unsigned int                        CurbeTotalDataLength;

        USC::GFXMEDIA_GPUWALKER_SIMD        SimdWidth;

        unsigned int                        TgsmTotalByteCount;
        unsigned int                        ThreadGroupSize;

        void*                               ThreadPayloadData;

        unsigned int                        CSHThreadDispatchChannel;

        bool                                CompiledForIndirectPayload;

        bool                                DispatchAlongY;

        /* Output related to only the PingPong Textures */
        bool                                SecondCompile;
        bool                                IsRowMajor;
        bool                                PerformSecondCompile;

        unsigned int                        NumChannelsUsed;
        bool                                DisableMidThreadPreemption;

        DWORD m_AccessedBySampleC[4];
    };

    struct SHullShaderKernelProgram : SKernelProgram
    {
        bool                                IncludeVertexHandles;
        OctEltUnit                          URBAllocationSize;
        OctEltUnit                          PatchConstantURBSize;
        OctEltUnit                          VertexURBEntryReadLength;
        OctEltUnit                          VertexURBEntryReadOffset;
        bool                                IncludePrimitiveIDEnable;
        HullShaderDispatchModes             DispatchMode;
        unsigned int                        InstanceCount;
        DWORD m_AccessedBySampleC[4];
        unsigned int                        BindingTableEntryBitmap;
    };

    struct SDomainShaderKernelProgram : SKernelProgram
    {
        OctEltUnit                          URBAllocationSize;
        OctEltUnit                          VertexURBEntryReadLength;
        OctEltUnit                          VertexURBEntryReadOffset;
        OctEltUnit                          VertexURBEntryOutputLength;
        OctEltUnit                          VertexURBEntryOutputReadOffset;
        bool                                ComputeWAttribute;
        DomainShaderDispatchModes           DispatchMode;
        SProgramOutput                      simd8DualPatch;
        bool                                DeclaresRTAIndex;
        bool                                DeclaresVPAIndex;
        bool                                HasClipCullAsOutput;
        bool                                HasPrimitiveIDInput;
        DWORD m_AccessedBySampleC[4];
        unsigned int                        BindingTableEntryBitmap;
    };


    struct SOpenCLKernelInfo
    {
        struct SResourceInfo
        {
            enum { RES_UAV, RES_SRV, RES_OTHER } Type;
            int Index;
        };

        SOpenCLKernelInfo()
        {
            m_printfBufferAnnotation = nullptr;
            m_startGAS = nullptr;
            m_WindowSizeGAS = nullptr;
            m_PrivateMemSize = nullptr;

            memset(&m_threadPayload, 0, sizeof(m_threadPayload));
            memset(&m_executionEnivronment, 0, sizeof(m_executionEnivronment));
            memset(&m_kernelProgram, 0, sizeof(m_kernelProgram));
        };

        std::string m_kernelName;
        QWORD       m_ShaderHashCode;

        std::vector<iOpenCL::PointerInputAnnotation*>       m_pointerInput;
        std::vector<iOpenCL::PointerArgumentAnnotation*>    m_pointerArgument;
        std::vector<iOpenCL::LocalArgumentAnnotation*>      m_localPointerArgument;
        std::vector<iOpenCL::SamplerInputAnnotation*>       m_samplerInput;
        std::vector<iOpenCL::SamplerArgumentAnnotation*>    m_samplerArgument;
        std::vector<iOpenCL::ConstantInputAnnotation*>      m_constantInputAnnotation;
        std::vector<iOpenCL::ConstantArgumentAnnotation*>   m_constantArgumentAnnotation;
        std::vector<iOpenCL::ImageArgumentAnnotation*>      m_imageInputAnnotations;
        std::vector<iOpenCL::KernelArgumentInfoAnnotation*> m_kernelArgInfo;
        std::vector<iOpenCL::PrintfStringAnnotation*>       m_printfStringAnnotations;

        iOpenCL::PrintfBufferAnnotation                    *m_printfBufferAnnotation;
        iOpenCL::StartGASAnnotation                        *m_startGAS = NULL;
        iOpenCL::WindowSizeGASAnnotation                   *m_WindowSizeGAS = NULL;
        iOpenCL::PrivateMemSizeAnnotation                  *m_PrivateMemSize = NULL;
        std::string                                         m_kernelAttributeInfo;

        bool                                                m_HasInlineVmeSamplers = false;

        // This maps argument numbers to BTI and sampler indices
        // (e.g. kernel argument 3, which is is an image_2d, may be mapped to BTI 6)
        std::map<DWORD, unsigned int> m_argIndexMap;

        iOpenCL::ThreadPayload        m_threadPayload;

        iOpenCL::ExecutionEnivronment m_executionEnivronment;

        iOpenCL::KernelTypeProgramBinaryInfo m_kernelTypeInfo;

        SKernelProgram                m_kernelProgram;
    };


    struct SOpenCLProgramInfo
    {
        std::vector<std::unique_ptr<iOpenCL::InitConstantAnnotation> > m_initConstantAnnotation;
        std::vector<std::unique_ptr<iOpenCL::InitGlobalAnnotation> > m_initGlobalAnnotation;
        std::vector<std::unique_ptr<iOpenCL::ConstantPointerAnnotation> > m_initConstantPointerAnnotation;
        std::vector<std::unique_ptr<iOpenCL::GlobalPointerAnnotation> > m_initGlobalPointerAnnotation;
        std::vector<std::unique_ptr<iOpenCL::KernelTypeProgramBinaryInfo> > m_initKernelTypeAnnotation;
    };

    class CBTILayout
    {
    public:
        unsigned int GetSystemThreadBindingTableIndex(void) const;
        unsigned int GetBindingTableEntryCount(void) const;
        unsigned int GetTextureIndex(unsigned int index) const;
        unsigned int GetUavIndex(unsigned int index) const;
        unsigned int GetRenderTargetIndex(unsigned int index) const;
        unsigned int GetConstantBufferIndex(unsigned int index) const;
        unsigned int GetNullSurfaceIdx() const;
        unsigned int GetTGSMIndex() const;
        unsigned int GetScratchSurfaceBindingTableIndex() const;
        unsigned int GetStatelessBindingTableIndex() const;
        unsigned int GetImmediateConstantBufferOffset() const;
        unsigned int GetDrawIndirectBufferIndex() const;
        const USC::SShaderStageBTLayout* GetBtLayout() const { return m_pLayout; };
        const std::vector<unsigned char>& GetColorBufferMappingTable() const { return m_ColorBufferMappings; }

        CBTILayout(const USC::SShaderStageBTLayout* pLayout) : m_pLayout(pLayout)
        {}

        CBTILayout(
            const USC::SShaderStageBTLayout* pLayout,
            const std::vector<unsigned char>& colorBufferMappings) :
                m_pLayout(pLayout),
                m_ColorBufferMappings(colorBufferMappings)
        {}

    protected:
        const USC::SShaderStageBTLayout* m_pLayout;

        // Vulkan front end provides a separate vector with color buffer mappings.
        const std::vector<unsigned char> m_ColorBufferMappings;
    };

    // This is insanely ugly, but it's the pretties solution we could
    // think of that preserves the GFX code.
    // This is temporary and will go away once image access between
    // OCL and GFX is unified.
    // This happens because in GFX the layout comes from the driver and is
    // immutable, while in OCL we need to change the layout mid-codegen.
    class COCLBTILayout : public CBTILayout
    {
    public:
        COCLBTILayout(const USC::SShaderStageBTLayout* pLayout) : CBTILayout(pLayout)
        {}

        USC::SShaderStageBTLayout* getModifiableLayout();
    };

    class RetryManager
    {
    public:
        RetryManager();
        ~RetryManager();

        bool AdvanceState();
        bool AllowUnroll();
        bool AllowLICM();
        bool AllowPromotePrivateMemory();
        bool AllowPreRAScheduler();
        bool AllowCodeSinking();
        bool AllowSimd32Slicing();
        bool AllowLargeURBWrite();
        bool IsFirstTry();
        bool IsLastTry(CodeGenContext* cgCtx);
        unsigned GetRetryId() const;

        void Enable();
        void Disable();

        void SetSpillSize(unsigned int spillSize);
        unsigned int GetLastSpillSize();
        unsigned int numInstructions = 0;
        /// the set of OCL kernels that need to recompile
        std::set<std::string> kernelSet;

        void ClearSpillParams();
        // save entry for given SIMD mode, to avoid recompile for next retry.
        void SaveSIMDEntry(SIMDMode simdMode, CShader* shader);
        CShader* GetSIMDEntry(SIMDMode simdMode);
        bool AnyKernelSpills();

        // Try to pickup the simd mode & kernel based on heuristics and fill
        // programOutput.  If returning true, then stop the further retry.
        bool PickupKernels(CodeGenContext* cgCtx);

    private:
        unsigned stateId;
        // For debugging purposes, it can be useful to start on a particular
        // ID rather than id 0.
        unsigned firstStateId;

        unsigned getStateCnt();

        /// internal knob to disable retry manager.
        bool enabled;

        unsigned lastSpillSize = 0;

        // cache the compiled kernel during retry
        CShader* m_simdEntries[3];

        CShader* PickCSEntryForcedFromDriver(SIMDMode& simdMode,
            unsigned char forcedSIMDModeFromDriver);
        CShader* PickCSEntryByRegKey(SIMDMode& simdMode);
        CShader* PickCSEntryEarly(SIMDMode& simdMode,
            ComputeShaderContext* cgCtx);
        CShader* PickCSEntryFinally(SIMDMode& simdMode);
        void FreeAllocatedMemForNotPickedCS(SIMDMode simdMode);
        bool PickupCS(ComputeShaderContext* cgCtx);
    };

    /// this class adds intrinsic cache to LLVM context
    class LLVMContextWrapper : public llvm::LLVMContext
    {
        LLVMContextWrapper(LLVMContextWrapper&) = delete;
        LLVMContextWrapper& operator =(LLVMContextWrapper&) = delete;

    public:
        LLVMContextWrapper(bool createResourceDimTypes = true);
        /// ref count the LLVMContext as now CodeGenContext owns it
        unsigned int refCount = 0;
        /// IntrinsicIDCache - Cache of intrinsic pointer to numeric ID mappings
        /// requested in this context
        typedef llvm::ValueMap<const llvm::Function*, unsigned> SafeIntrinsicIDCacheTy;
        SafeIntrinsicIDCacheTy m_SafeIntrinsicIDCache;
        void AddRef();
        void Release();
    };

    class CodeGenContext
    {
    public:
        /// input: hash key
        ShaderHash    hash;
        ShaderType    type;
        /// input: Platform features supported
        const CPlatform& platform;
        /// input: binding table layout used by the driver
        const CBTILayout& btiLayout;
        /// information about the driver
        const CDriverInfo& m_DriverInfo;
        /// output: driver instrumentation
        TimeStats       *m_compilerTimeStats = nullptr;
        ShaderStats     *m_sumShaderStats = nullptr;
        // float 16, float32 and float64 denorm mode
        Float_DenormMode    m_floatDenormMode16 = FLOAT_DENORM_FLUSH_TO_ZERO;
        Float_DenormMode    m_floatDenormMode32 = FLOAT_DENORM_FLUSH_TO_ZERO;
        Float_DenormMode    m_floatDenormMode64 = FLOAT_DENORM_FLUSH_TO_ZERO;

        SInstrTypes m_instrTypes;

        /////  used for instruction statistic before/after pass
        int instrStat[TOTAL_TYPES][TOTAL_STAGE];

        /// Module level flag. This flag is false either there is an indirect call
        /// in the module or the kernel sizes are small even with complete inlining.
        bool m_enableSubroutine = false;
        bool m_enableFunctionPointer = false;

        llvm::AssemblyAnnotationWriter* annotater = nullptr;

        RetryManager m_retryManager;

        // shader stat for opt customization
        uint32_t     m_tempCount = 0;
        uint32_t     m_sampler = 0;
        uint32_t     m_inputCount = 0;
        uint32_t     m_dxbcCount = 0;
        uint32_t     m_ConstantBufferCount = 0;
        uint32_t     m_numGradientSinked = 0;
        std::vector<unsigned> m_indexableTempSize;
        bool         m_highPsRegisterPressure = 0;

        // For IR dump after pass
        unsigned     m_numPasses = 0;
        bool m_threadCombiningOptDone = false;

        //For storing error message
        std::string oclErrorMessage;

        void *m_ConstantBufferReplaceShaderPatterns = nullptr;
        uint m_ConstantBufferReplaceShaderPatternsSize = 0;
        uint m_ConstantBufferUsageMask = 0;
        uint m_ConstantBufferReplaceSize = 0;

        void* gtpin_init = nullptr;

        CompilerStats m_Stats;

    protected:
        // Objects pointed to by these pointers are owned by this class.
        LLVMContextWrapper *llvmCtxWrapper;
        /// input: LLVM module
        llvm::Module*   module = nullptr;
        /// input: IGC MetaData Utils
        IGC::IGCMD::MetaDataUtils  *m_pMdUtils = nullptr;
        IGC::ModuleMetaData *modMD = nullptr;
    public:
        CodeGenContext(
            ShaderType          _type,      ///< shader type
            const CBTILayout&   _bitLayout, ///< binding table layout to be used in code gen
            const CPlatform&    _platform,  ///< IGC HW platform description
            const CDriverInfo&  driverInfo, ///< Queries to know runtime features support
            const bool          createResourceDimTypes = true,
            LLVMContextWrapper* LLVMContext = nullptr)///< LLVM context to use, if null a new one will be created
            : type(_type), platform(_platform), btiLayout(_bitLayout), m_DriverInfo(driverInfo), llvmCtxWrapper(LLVMContext)
        {
            if (llvmCtxWrapper == nullptr)
            {
                initLLVMContextWrapper(createResourceDimTypes);
            }
            else
            {
                llvmCtxWrapper->AddRef();
            }

            m_indexableTempSize.resize(64);

            for (uint i = 0; i < TOTAL_TYPES; i++)
            {
                for (uint j = 0; j < TOTAL_STAGE; j++)
                {
                    instrStat[i][j] = 0;
                }
            }
        }

        CodeGenContext(CodeGenContext&) = delete;
        CodeGenContext& operator =(CodeGenContext&) = delete;

        void initLLVMContextWrapper(bool createResourceDimTypes = true);
        llvm::LLVMContext* getLLVMContext();
        IGC::IGCMD::MetaDataUtils* getMetaDataUtils();
        llvm::Module* getModule() const;

        void setModule(llvm::Module *m);
        // Several clients explicitly delete module without resetting module to null.
        // This causes the issue later when the dtor is invoked (trying to delete a
        // dangling pointer again). This function is used to replace any explicit
        // delete in order to prevent deleting dangling pointers happening.
        void deleteModule();
        IGC::ModuleMetaData* getModuleMetaData() const;
        unsigned int getRegisterPointerSizeInBits(unsigned int AS) const;
        bool enableFunctionCall() const;
        virtual void InitVarMetaData();
        virtual ~CodeGenContext();
        void clear();
        void EmitError(const char* errorstr);
        CompOptions& getCompilerOption();
        virtual void resetOnRetry();
        virtual uint32_t getNumGRFPerThread() const;
        bool isPOSH() const;
        
        CompilerStats& Stats()
        {
            return m_Stats;
        }
    };

    class VertexShaderContext : public CodeGenContext
    {
    public:
        // output: shader information
        SVertexShaderKernelProgram programOutput;
        VertexShaderContext(
            const CBTILayout&   btiLayout, ///< binding table layout to be used in code gen
            const CPlatform&    platform,  ///< IGC HW platform description
            const CDriverInfo&  driverInfo,
            const bool          createResourceDimTypes = true,
            LLVMContextWrapper* llvmCtxWrapper = nullptr) ///< LLVM context to use, if null a new one will be created
            : CodeGenContext(ShaderType::VERTEX_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmCtxWrapper)
        {
            memset(&programOutput, 0, sizeof(SVertexShaderKernelProgram));
        }

    };

    class PixelShaderContext : public CodeGenContext
    {
    public:
        // output: shader information
        SPixelShaderKernelProgram programOutput;
        PixelShaderContext(
            const CBTILayout&   btiLayout, ///< binding table layout to be used in code gen
            const CPlatform&    platform,  ///< IGC HW platform description
            const CDriverInfo&  driverInfo,
            const bool          createResourceDimTypes = true,
            LLVMContextWrapper* llvmCtxWrapper = nullptr) ///< LLVM context to use, if null a new one will be created
            : CodeGenContext(ShaderType::PIXEL_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmCtxWrapper)
        {
            memset(&programOutput, 0, sizeof(SPixelShaderKernelProgram));
        }
    };

    class GeometryShaderContext : public CodeGenContext
    {
    public:
        // output: shader information
        SGeometryShaderKernelProgram programOutput;
        GeometryShaderContext(
            const CBTILayout&   btiLayout, ///< binding table layout to be used in code gen
            const CPlatform&    platform,  ///< IGC HW platform description
            const CDriverInfo&  driverInfo,
            const bool          createResourceDimTypes = true,
            LLVMContextWrapper* llvmCtxWrapper = nullptr) ///< LLVM context to use, if null a new one will be created
            : CodeGenContext(ShaderType::GEOMETRY_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmCtxWrapper)
        {
            memset(&programOutput, 0, sizeof(SGeometryShaderKernelProgram));
        }
    };

    struct SComputeShaderSecondCompileInput
    {
        bool secondCompile;
        bool isRowMajor;
        int numChannelsUsed;
        int runtimeVal_LoopCount;
        int runtimeVal_ResWidthOrHeight;
        int runtimeVal_ConstBufferSize;

        SComputeShaderSecondCompileInput()
            : secondCompile(false)
            , isRowMajor(false)
            , numChannelsUsed(0)
            , runtimeVal_LoopCount(0)
            , runtimeVal_ResWidthOrHeight(0)
            , runtimeVal_ConstBufferSize(0)
        {}
    };

    class ComputeShaderContext : public CodeGenContext
    {
    public:
        SComputeShaderKernelProgram programOutput;
        bool isSecondCompile;
        bool m_IsPingPongSecond;
        unsigned m_slmSize;

        ComputeShaderContext(
            const CBTILayout&   btiLayout, ///< binding table layout to be used in code gen
            const CPlatform&    platform,  ///< IGC HW platform description
            const CDriverInfo&  driverInfo,
            const bool          createResourceDimTypes = true,
            LLVMContextWrapper* llvmCtxWrapper = nullptr) ///< LLVM context to use, if null a new one will be created
            : CodeGenContext(ShaderType::COMPUTE_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmCtxWrapper)
        {
            memset(&programOutput, 0, sizeof(SComputeShaderKernelProgram));
            isSecondCompile = false;
            m_IsPingPongSecond = false;
            m_slmSize = 0;
        }

        /** get shader's thread group size */
        unsigned GetThreadGroupSize();
        unsigned GetSlmSizePerSubslice();
        float GetThreadOccupancy(SIMDMode simdMode);
        /** get smallest SIMD mode allowed based on thread group size */
        SIMDMode GetLeastSIMDModeAllowed();
        /** get largest SIMD mode for performance based on thread group size */
        SIMDMode GetMaxSIMDMode();

        float GetSpillThreshold() const;
    };

    class HullShaderContext : public CodeGenContext
    {
    public:
        // output: shader information
        SHullShaderKernelProgram programOutput;
        HullShaderContext(
            const CBTILayout&   btiLayout, ///< binding table layout to be used in code gen
            const CPlatform&    platform,  ///< IGC HW platform description
            const CDriverInfo&  driverInfo,
            const bool          createResourceDimTypes = true,
            LLVMContextWrapper* llvmCtxWrapper = nullptr) ///< LLVM context to use, if null a new one will be created
            : CodeGenContext(ShaderType::HULL_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmCtxWrapper)
        {
            memset(&programOutput, 0, sizeof(SHullShaderKernelProgram));
        }
    };

    class DomainShaderContext : public CodeGenContext
    {
    public:
        // output: shader information
        SDomainShaderKernelProgram programOutput;
        DomainShaderContext(
            const CBTILayout&   btiLayout, ///< binding table layout to be used in code gen
            const CPlatform&    platform,  ///< IGC HW platform description
            const CDriverInfo&  driverInfo,
            const bool          createResourceDimTypes = true,
            LLVMContextWrapper* llvmCtxWrapper = nullptr) ///< LLVM context to use, if null a new one will be created
            : CodeGenContext(ShaderType::DOMAIN_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmCtxWrapper)
        {
            memset(&programOutput, 0, sizeof(SDomainShaderKernelProgram));
        }
    };
    class OpenCLProgramContext : public CodeGenContext
    {
    public:
        // We should probably replace all of this with proper option parsing,
        // like RS does
        class InternalOptions
        {
        public:
            InternalOptions(const TC::STB_TranslateInputArgs* pInputArgs) :
                KernelDebugEnable(false),
                IncludeSIPCSR(false),
                IncludeSIPKernelDebug(false),
                IntelGreaterThan4GBBufferRequired(false),
                Use32BitPtrArith(false),
                IncludeSIPKernelDebugWithLocalMemory(false),
                DoReRA(false),
                IntelHasBufferOffsetArg(false)
            {
                if (pInputArgs == nullptr)
                    return;

                if (pInputArgs->pInternalOptions == nullptr)
                    return;

                const char *options = pInputArgs->pInternalOptions;
                if (strstr(options, "-cl-replace-global-offsets-by-zero"))
                {
                    replaceGlobalOffsetsByZero = true;
                }
                if (strstr(options, "-cl-kernel-debug-enable"))
                {
                    KernelDebugEnable = true;
                }

                if (strstr(options, "-cl-include-sip-csr"))
                {
                    IncludeSIPCSR = true;
                }

                if (strstr(options, "-cl-include-sip-kernel-debug"))
                {
                    IncludeSIPKernelDebug = true;
                }
                else if (strstr(options, "-cl-include-sip-kernel-local-debug"))
                {
                    IncludeSIPKernelDebugWithLocalMemory = true;
                }

                if (strstr(options, "-cl-intel-use-32bit-ptr-arith"))
                {
                    Use32BitPtrArith = true;
                }

                if (strstr(options, "-cl-intel-greater-than-4GB-buffer-required"))
                {
                    IntelGreaterThan4GBBufferRequired = true;
                }
                else if (strstr(options, "-cl-intel-has-buffer-offset-arg"))
                {
                    IntelHasBufferOffsetArg = true;
                }

                if (strstr(options, "-cl-intel-gtpin-rera"))
                {
                    DoReRA = true;
                }
                if (strstr(options, "-cl-intel-no-prera-scheduling"))
                {
                    IntelEnablePreRAScheduling = false;
                }
                if (strstr(options, "-cl-intel-use-bindless-buffers"))
                {
                    PromoteStatelessToBindless = true;
                }
            }


            bool KernelDebugEnable;
            bool IncludeSIPCSR;
            bool IncludeSIPKernelDebug;
            bool IntelGreaterThan4GBBufferRequired;
            bool Use32BitPtrArith = false;
            bool IncludeSIPKernelDebugWithLocalMemory;
            bool DoReRA;
            bool IntelHasBufferOffsetArg;
            bool replaceGlobalOffsetsByZero = false;
            bool IntelEnablePreRAScheduling = true;
            bool PromoteStatelessToBindless = false;

        };

        class Options
        {
        public:
            Options(const TC::STB_TranslateInputArgs* pInputArgs) :
                CorrectlyRoundedSqrt(false),
                NoSubgroupIFP(false),
                UniformWGS(false)
            {
                if (pInputArgs == nullptr)
                    return;

                if (pInputArgs->pOptions == nullptr)
                    return;

                const char *options = pInputArgs->pOptions;
                if (strstr(options, "-cl-fp32-correctly-rounded-divide-sqrt"))
                {
                    CorrectlyRoundedSqrt = true;
                }

                if (strstr(options, "-cl-no-subgroup-ifp"))
                {
                    NoSubgroupIFP = true;
                }

                if (strstr(options, "-cl-uniform-work-group-size"))
                {
                    // Note that this is only available for -cl-std >= 2.0.
                    // This will be checked before we place this into the
                    // the module metadata.
                    UniformWGS = true;
                }
            }

            bool CorrectlyRoundedSqrt;
            bool NoSubgroupIFP;
            bool UniformWGS;
        };

        // output: shader information
        iOpenCL::CGen8OpenCLProgram m_programOutput;
        SOpenCLProgramInfo m_programInfo;
        const InternalOptions m_InternalOptions;
        const Options m_Options;
        bool isSpirV;
        float m_ProfilingTimerResolution;
        bool m_ShouldUseNonCoherentStatelessBTI;

        OpenCLProgramContext(
            const COCLBTILayout& btiLayout,
            const CPlatform& platform,
            const TC::STB_TranslateInputArgs* pInputArgs,
            const CDriverInfo& driverInfo,
            LLVMContextWrapper* llvmContext = nullptr,
            bool shouldUseNonCoherentStatelessBTI = false,
            const bool createResourceDimTypes = true)
            : CodeGenContext(ShaderType::OPENCL_SHADER, btiLayout, platform, driverInfo, createResourceDimTypes, llvmContext),
            m_programOutput(platform.getPlatformInfo(), *this),
            m_InternalOptions(pInputArgs),
            m_Options(pInputArgs),
            isSpirV(false),
            m_ShouldUseNonCoherentStatelessBTI(shouldUseNonCoherentStatelessBTI)
        {
        }
        bool isSPIRV() const;
        void setAsSPIRV();
        float getProfilingTimerResolution();
        uint32_t getNumGRFPerThread() const;
    private:
        llvm::DenseMap<llvm::Function*, std::string> m_hashes_per_kernel;
    };

    void CodeGen(PixelShaderContext* ctx);
    void CodeGen(ComputeShaderContext* ctx);
    void CodeGen(DomainShaderContext* ctx);
    void CodeGen(HullShaderContext* ctx);
    void CodeGen(VertexShaderContext* ctx);
    void CodeGen(GeometryShaderContext* ctx);
    void CodeGen(OpenCLProgramContext* ctx);

    void OptimizeIR(CodeGenContext* ctx);
    void UnifyIROGL(CodeGenContext* ctx);

    /**
     * Fold derived constants.  Load CB data from CBptr with index & offset,
     * calculate the new data based on LLVM bitcode and store results to pNewCB.
     * Then driver will push pNewCB to thread payload.
     */
    void FoldDerivedConstant(char* bitcode, uint bitcodeSize, void* CBptr[15],
        std::function<void (uint[4], uint, uint, bool)> getResInfoCB, uint* pNewCB);

    // Apply link optimization to shader programs.
    void LinkOptIR(CodeGenContext* ctxs[], bool usesStreamOutput);

    class LTOPSAction {
    public:
        virtual void operator()(llvm::GenIntrinsicInst* inst) = 0;
        virtual ~LTOPSAction() { }
    };
    typedef std::vector<std::unique_ptr<LTOPSAction> > LTOPSActions;

    // Apply link optimization to shader program, record and return
    // actions performed on PS.
    void LinkOptIRGetPSActions(CodeGenContext* ctxs[],
        bool usesStreamOutput, LTOPSActions& psActions);

    // Replay previous LTO actions on PS, used for PS retry. So we don't need
    // to go through the whole LTO process again.
    void LinkOptReplayPSActions(PixelShaderContext* psCtx,
        const LTOPSActions& psActions);
} // end IGC namespace
