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
#include "common/Types.hpp"

namespace IGC
{
struct PSSignature
{
    struct DispatchSignature
    {
        std::vector<unsigned int> inputOffset;
        std::map<unsigned int, unsigned int> PSOutputOffset;
        unsigned int              pixelOffset;
        unsigned int              ZWDelta;
        unsigned int              oMaskOffset;
        unsigned int              r1;
        bool                      CoarseMask;
        DispatchSignature() : CoarseMask(false) {}
    };
    DispatchSignature dispatchSign[3];
    std::map<unsigned int, uint64_t> PSConstantOutput;
};


class CPixelShader : public CShader
{
public:
    CPixelShader(llvm::Function* pFunc, CShaderProgram* pProgram);
    ~CPixelShader();
    CVariable* GetR1();
    CVariable* GetCoarseR1();
    CVariable* GetBaryReg(e_interpolation mode);
    CVariable* GetBaryRegLowered(e_interpolation mode);
    CVariable* GetInputDelta(uint index, bool loweredInput = false);
    CVariable* GetInputDeltaLowered(uint index);
    CVariable* GetZWDelta();
    CVariable* GetPositionZ();
    CVariable* GetPositionW();
    CVariable* GetPositionXYOffset();
    CVariable* GetSampleOffsetX();
    CVariable* GetSampleOffsetY();
    CVariable* GetInputCoverageMask();
    CVariable* GetCPSRequestedSizeX();
    CVariable* GetCPSRequestedSizeY();
    CVariable* GetCoarseParentIndex();
    CVariable* GetCurrentPhaseCounter();

    /// Get mask for pixels not discarded
    CVariable* GetDiscardPixelMask() { return m_KillPixelMask; }
    void SetDiscardPixelMask(CVariable* mask) { m_KillPixelMask = mask; }

    void InitEncoder(SIMDMode simdMode, bool canAbortOnSpill, ShaderDispatchMode shaderMode = ShaderDispatchMode::NOT_APPLICABLE) override;
    void PreCompile() override;
    void AllocatePayload() override;
    void AddPrologue() override;
    void PreAnalysisPass() override;
    void AddEpilogue(llvm::ReturnInst* ret) override;
    bool CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F) override;
    void ExtractGlobalVariables() override;

    void        AllocatePSPayload();
    void        AllocatePixelPhasePayload();

    void        FillProgram(SPixelShaderKernelProgram* pKernelProgram);
    void        AddRenderTarget(uint index);
    void        DeclareSGV(uint usage);
    void        ParseShaderSpecificOpcode(llvm::Instruction* inst) override;
    void        PullPixelPhasePayload();

    void        AddCoarseOutput(CVariable* var, unsigned int index);
    CVariable* GetCoarseInput(unsigned int index, uint16_t vectorSize, VISA_Type type);
    void        SetCoarseoMask(CVariable* var);
    CVariable* GetCoarseMask();
    void        OutputDepth() { m_HasoDepth = true; };
    void        OutputStencil() { m_HasoStencil = true; }
    void        OutputMask() { m_HasoMask = true; }
    bool        HasRenderTarget() { return m_RenderTargetMask != 0; }
    void        SetPhase(PixelShaderPhaseType phase) { m_phase = phase; }
    PixelShaderPhaseType GetPhase() { return m_phase; }
    void        SetLastPhase() { m_IsLastPhase = true; }
    void        SetPSSignature(PSSignature* signature) { m_Signature = signature; }
    bool        IsLastPhase() { return m_IsLastPhase; }
    bool        IsPerSample() { return m_isPerSample || m_PerspectiveSample || m_NoPerspectiveSample; }

    bool        HasDiscard() { return m_HasDiscard; }
    bool        NeedVMask() { return m_VectorMask; }
    void        MarkConstantInterpolation(unsigned int index);

    // check whether it's the last render target write
    bool        IsLastRTWrite(llvm::GenIntrinsicInst* inst);

    void emitPSInputLowering();

    std::vector<std::pair<llvm::Instruction*, bool> > rtWriteList;
    bool        m_hasEOT;
    bool        m_NeedPSSync;

    std::vector<CVariable*> setupLowered;
    std::set<uint> loweredSetupIndexes;
    bool modesUsed[NUMBER_EINTERPOLATION];
    bool LowerPSInput();
protected:
    void CreatePassThroughVar();
    bool IsReturnBlock(llvm::BasicBlock* bb);

    PSSignature::DispatchSignature& GetDispatchSignature();
    CVariable* m_R1;
    CVariable* m_CoarseR1;
    CVariable* m_PerspectivePixel;
    CVariable* m_PerspectiveCentroid;
    CVariable* m_PerspectiveSample;
    CVariable* m_NoPerspectivePixel;
    CVariable* m_NoPerspectiveCentroid;
    CVariable* m_NoPerspectiveSample;
    CVariable* m_PerspectivePixelLowered;
    CVariable* m_PerspectiveCentroidLowered;
    CVariable* m_PerspectiveSampleLowered;
    CVariable* m_NoPerspectivePixelLowered;
    CVariable* m_NoPerspectiveCentroidLowered;
    CVariable* m_NoPerspectiveSampleLowered;
    CVariable* m_KillPixelMask;
    CVariable* m_pPositionZPixel;
    CVariable* m_pPositionWPixel;
    CVariable* m_pPositionXYOffset;
    CVariable* m_ZWDelta;
    CVariable* m_pInputCoverageMask;
    CVariable* m_pCPSRequestedSizeX;
    CVariable* m_pCPSRequestedSizeY;
    uint       m_RenderTargetMask;
    bool       m_HasoDepth;
    bool       m_HasoStencil;
    bool       m_HasoMask;
    bool       m_HasInputCoverageMask;
    bool       m_isPerSample;
    bool       m_HasPullBary;
    bool       m_HasCoarseSize;
    bool       m_hasDualBlendSource;
    unsigned int m_MaxSetupIndex = 0;
    /// workaround to force SIMD8 compilation when double are present
    bool       m_HasDouble;
    bool       m_VectorMask;
    uint       m_ConstantInterpolationMask = 0;

    bool       m_HasDiscard;

    // Multi phase shader properties
    PixelShaderPhaseType m_phase;
    bool       m_IsLastPhase;
    uint       m_pixelPhaseLabel;
    uint       m_epilogueLabel;
    CVariable* m_PixelPhasePayload;
    CVariable* m_PixelPhaseCounter;
    CVariable* m_CurrentPhaseCounter;
    CVariable* m_CoarseParentIndex;
    CVariable* m_SampleOffsetX;
    CVariable* m_SampleOffsetY;
    CVariable* m_CoarseoMask;
    CVariable* m_CoarseMaskInput;
    std::map<unsigned int, CVariable*> m_CoarseOutput;
    std::map<unsigned int, CVariable*> m_CoarseInput;
    PSSignature* m_Signature;
    unsigned int m_samplerCount;
};

}//namespace IGC
