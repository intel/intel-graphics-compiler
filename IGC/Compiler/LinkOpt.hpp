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

///===========================================================================
/// This file contains types, enumerations, classes and other declarations 
/// used by IGC link optimization.
#pragma once

#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

static const ShaderType LTO_LAST_SHADER_STAGE = ShaderType::PIXEL_SHADER;
static const int LTO_NUM_SHADER_STAGES = int(LTO_LAST_SHADER_STAGE) + 1;

template <typename T>
using VecOfVec = std::vector<std::vector<T> >;

typedef std::vector<llvm::GenIntrinsicInst*> IntrinVec;
typedef std::vector<IntrinVec> VecOfIntrinVec;

class LinkOptContext
{
private:
    inline void ivecSet(
        IntrinVec& vec,
        llvm::GenIntrinsicInst* inst,
        unsigned idx)
    {
        if (vec.size() < idx + 1)
        {
            vec.resize(idx + 1);
        }
        vec[idx] = inst;
    }

    inline void ivecAppend(
        VecOfIntrinVec& vec,
        llvm::GenIntrinsicInst* inst,
        unsigned idx)
    {
        if (vec.size() < idx + 1)
        {
            vec.resize(idx + 1);
        }
        vec[idx].push_back(inst);
    }

public:
    LinkOptContext()
        : m_abortLTO(false)
        , m_dump(nullptr)
    {
        memset(m_contexts, 0, sizeof(m_contexts));
    }
    ~LinkOptContext() { }

    void initDebugDump();
    void closeDebugDump();
    void debugPrint(const char* fmt, ...);

    inline void setContext(ShaderType shaderType, CodeGenContext* ctx)
    {
        assert(shaderType <= LTO_LAST_SHADER_STAGE);
        assert(m_contexts[int(shaderType)] == nullptr);

        m_contexts[int(shaderType)] = ctx;
    }

    inline CodeGenContext** getContexts()
    {
        return m_contexts;
    }
    inline CodeGenContext* getContext(ShaderType shaderType) const
    {
        assert(shaderType <= LTO_LAST_SHADER_STAGE);
        return m_contexts[int(shaderType)];
    }
    inline VertexShaderContext* getVS() const
    {
        return static_cast<VertexShaderContext*>(
            getContext(ShaderType::VERTEX_SHADER));
    }
    inline PixelShaderContext* getPS() const
    {
        return static_cast<PixelShaderContext*>(
            getContext(ShaderType::PIXEL_SHADER));
    }
    inline HullShaderContext* getHS() const
    {
        return static_cast<HullShaderContext*>(
            getContext(ShaderType::HULL_SHADER));
    }
    inline DomainShaderContext* getDS() const
    {
        return static_cast<DomainShaderContext*>(
            getContext(ShaderType::DOMAIN_SHADER));
    }
    inline GeometryShaderContext* getGS() const
    {
        return static_cast<GeometryShaderContext*>(
            getContext(ShaderType::GEOMETRY_SHADER));
    }

    bool m_abortLTO;
   
    // vs inputs & outputs
    // in[attrIdx] = { ShaderInputVec(attrIdx)};
    // vs outputs, each attr has only 1 output intrinsic
    // out[attrIdx] = { OUTPUT(xyzw, usage, attrIdx) };
    // vs non-default outputs (like POSITION) are indexed in vector
    // using values assigned to types;
    struct {
        VecOfIntrinVec inInsts;
        VecOfIntrinVec outInsts;
        VecOfIntrinVec outNonDefaultInsts;
    } vs;

    // hs inputs & outputs
    // indexed by attr index, then ctrl point index
    struct {
        VecOfIntrinVec inInsts;
        VecOfIntrinVec outInsts;
        VecOfIntrinVec pcOut;   // patch const outputs
        VecOfIntrinVec pcIn;    // patch const inputs
        VecOfIntrinVec inNonDefaultInsts;

        // track the output attrs used as own input (via HSOutputCntrlPtInputVec)
        std::map<unsigned, std::vector<llvm::GenIntrinsicInst*> > outIn;
    } hs;

    // ds inputs & outputs, inputs[elemIdx][ctrlIdx]
    // out[attrIdx] = { OUTPUT(xyzw, usage, attrIdx) }
    // each attr has only 1 output intrinsic
    struct {
        std::vector<
            std::vector<
            std::list<llvm::GenIntrinsicInst*> > > inInsts;
        VecOfIntrinVec outInsts;
        VecOfIntrinVec pcIn;
        VecOfIntrinVec outNonDefaultInsts;
    } ds;

    // gs inputs & outputs
    // in[attrIdx] = { GSinputVec(vertexIdx, attrIdx), ... }
    // out[attrIdx] = { OUTPUTGS(xyzw, usage, attrIdx, emitCount), ... }
    // each attr output may have multiple output intrinsics
    struct {
        VecOfIntrinVec inInsts;
        VecOfIntrinVec outInsts;
        VecOfIntrinVec inNonDefaultInsts;
    } gs;

    // ps inputs
    struct {
        VecOfIntrinVec inInsts;
    } ps;

    inline void addVSInput(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        ivecAppend(vs.inInsts, inst, index);
    }

    inline void addVSOutput(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        ivecAppend(vs.outInsts, inst, index);
    }

    inline void addVSOutputNonDefault(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        ivecAppend(vs.outNonDefaultInsts, inst, index);
    }

    // HS input can be access with dynamic indexed ctrl point
    inline void addHSInput(
        llvm::GenIntrinsicInst* inst,
        unsigned elemIdx)
    {
        ivecAppend(hs.inInsts, inst, elemIdx);
    }

    inline void addHSCtrlPtOutput(
        llvm::GenIntrinsicInst* inst,
        unsigned elemIdx,
        unsigned ctrlIdx)
    {
        ivecAppend(hs.outInsts, inst, elemIdx);
    }

    inline void addHSPatchConstOutput(
        llvm::GenIntrinsicInst* inst,
        unsigned idx)
    {
        ivecAppend(hs.pcOut, inst, idx);
    }

    inline void addHSPatchConstInput(
        llvm::GenIntrinsicInst* inst,
        unsigned idx)
    {
        ivecAppend(hs.pcIn, inst, idx);
    }

    inline void addHSOutputInput(
        llvm::GenIntrinsicInst* inst,
        unsigned attrIdx)
    {
        hs.outIn[attrIdx].push_back(inst);
    }

    inline void addDSCtrlPtInput(
        llvm::GenIntrinsicInst* inst,
        unsigned elemIdx,
        unsigned ctrlIdx)
    {
        if (ds.inInsts.size() < elemIdx + 1)
        {
            ds.inInsts.resize(elemIdx + 1);
        }
        if (ds.inInsts[elemIdx].size() < ctrlIdx + 1)
        {
            ds.inInsts[elemIdx].resize(ctrlIdx + 1);
        }
        ds.inInsts[elemIdx][ctrlIdx].push_back(inst);
    }

    inline void addDSPatchConstInput(
        llvm::GenIntrinsicInst* inst,
        unsigned idx)
    {
        ivecAppend(ds.pcIn, inst, idx);
    }

    inline void addDSOutput(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        ivecAppend(ds.outInsts, inst, index);
    }

    inline void addDSOutputNonDefault(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        ivecAppend(ds.outNonDefaultInsts, inst, index);
    }

    inline void addGSInputNonDefault(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        ivecAppend(gs.inNonDefaultInsts, inst, index);
    }

    inline void addGSInput(
        llvm::GenIntrinsicInst* inst,
        unsigned elemIdx)
    {
        if (gs.inInsts.size() < elemIdx + 1)
        {
            gs.inInsts.resize(elemIdx + 1);
        }
        gs.inInsts[elemIdx].push_back(inst);
    }

    inline void addGSOutput(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        ivecAppend(gs.outInsts, inst, index);
    }

    inline void addPSInput(
        llvm::GenIntrinsicInst* inst,
        unsigned index)
    {
        if (ps.inInsts.size() < index + 1)
        {
            ps.inInsts.resize(index + 1);
        }
        ps.inInsts[index].push_back(inst);
    }

protected:
    CodeGenContext* m_contexts[LTO_NUM_SHADER_STAGES];
    const Debug::Dump* m_dump;
};

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
class ShaderIOAnalysis :
    public llvm::FunctionPass
{
public:
    static char ID;
    typedef enum {
        INPUT = 0,
        OUTPUT = 1,
        UNKNOWN = 2
    } AnalysisType;

    static const int OUTPUT_USAGE_ARG = 4;
    static const int OUTPUT_ATTR_ARG = 5;
    static const int INPUT_ATTR_ARG = 0;
    static const int INPUT_INTERPMODE_ARG = 1;
    static const int HSGSINPUT_ATTR_ARG = 1;
    static const int HSOUTPUTCTRLPTINPUT_ATTR_ARG = 1;
    static const int OUTPUTTESSCTRLPT_ATTR_ARG = 4;
    static const int PATCHCONSTOUTPUT_ATTR_ARG = 4;
    static const int HSPATCHCONSTINPUT_ATTR_ARG = 0;
    static const int DSCTRLPTINPUT_CPID_ARG = 0;
    static const int DSCTRLPTINPUT_ATTR_ARG = 1;
    static const int DSPATCHCONSTINPUT_ATTR_ARG = 0;
    static const int GSSVINPUT_USAGE_ARG = 1;

    ShaderIOAnalysis(
        IGC::LinkOptContext* ctx,
        ShaderType type,
        int anaType)
        : FunctionPass(ID)
        , m_context(ctx)
        , m_shaderType(type)
        , m_analysisType(anaType)
    { }

    bool runOnFunction(llvm::Function& F) override;
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
    }

    llvm::StringRef getPassName() const  override { return "ShaderIOAnalysis"; }

protected:
    inline IGC::LinkOptContext* getContext() const { return m_context; }

    void onGenIntrinsic(llvm::GenIntrinsicInst* inst);

    void addInputDecl(
        llvm::GenIntrinsicInst* inst);

    void addDSCtrlPtInputDecl(
        llvm::GenIntrinsicInst* inst);
    void addDSPatchConstInputDecl(
        llvm::GenIntrinsicInst* inst);

    virtual void addOutputDecl(
        llvm::GenIntrinsicInst* inst);

    void addVSInputDecl(
        llvm::GenIntrinsicInst* inst);
    void addHSInputDecl(
        llvm::GenIntrinsicInst* inst);
    void addHSCtrlPtOutputDecl(
        llvm::GenIntrinsicInst* inst);
    void addHSPatchConstOutputDecl(
        llvm::GenIntrinsicInst* inst);
    void addHSOutputInputDecl(
        llvm::GenIntrinsicInst* inst);
    void addPatchConstOutput(
        llvm::GenIntrinsicInst* inst);
    void addHSPatchConstInputDecl(
        llvm::GenIntrinsicInst* inst);
    void addGSSVInputDecl(
        llvm::GenIntrinsicInst* inst);
    void addGSInputDecl(
        llvm::GenIntrinsicInst* inst);

    inline bool doInput() const
    {
        return m_analysisType == INPUT;
    }

    inline bool doOutput() const
    {
        return m_analysisType == OUTPUT;
    }

    LinkOptContext* m_context;
    ShaderType m_shaderType;
    int m_analysisType;
};

bool runPasses(CodeGenContext* ctx, ...);

inline uint getImmValueU32( const llvm::Value* value)
{
    const llvm::ConstantInt* cval;
    cval = llvm::cast<llvm::ConstantInt>(value);
    assert(cval->getBitWidth() == 32);

    uint ival;
    ival = (uint)cval->getZExtValue();
    return ival;
}

class LTOPSConstRepAction : public LTOPSAction {
    llvm::APFloat imm;

public:
    LTOPSConstRepAction(llvm::APFloat immf) : imm(immf) { }

    void operator()(llvm::GenIntrinsicInst* inst)
    {
        llvm::Value* cv;

        if (inst->getType()->isHalfTy())
        {
            // PS input is in low precision, lower the output const value
            bool isExact = false;
            llvm::APFloat immh = imm;
            immh.convert(llvm::APFloat::IEEEhalf(), llvm::APFloat::rmTowardZero, &isExact);
            cv = llvm::ConstantFP::get(inst->getContext(), immh);
        }
        else
        {
            cv = llvm::ConstantFP::get(inst->getContext(), imm);
        }
         
        inst->replaceAllUsesWith(cv);
        inst->eraseFromParent();
    }
};

class LTOPSConstInterpAction : public LTOPSAction {
    int attrIndex;
    bool setMode;

public:
    LTOPSConstInterpAction(int idx, bool _setMode)
        : attrIndex(idx), setMode(_setMode) { }

    void operator()(llvm::GenIntrinsicInst* inst)
    {
        llvm::Value* idxv = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(inst->getContext()), attrIndex);
        inst->setOperand(ShaderIOAnalysis::INPUT_ATTR_ARG, idxv);
        if (setMode)
        {
            llvm::Value* modev = llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(inst->getContext()), EINTERPOLATION_CONSTANT);
            inst->setOperand(ShaderIOAnalysis::INPUT_INTERPMODE_ARG, modev);
        }
    }
};

class LTOPSAdjustIndexAction : public LTOPSAction {
    int attrIndex;

public:
    LTOPSAdjustIndexAction(int idx) : attrIndex(idx) { }

    void operator()(llvm::GenIntrinsicInst* inst)
    {
        llvm::Value* idxv = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(inst->getContext()), attrIndex);
        inst->setOperand(ShaderIOAnalysis::INPUT_ATTR_ARG, idxv);
    }
};

} // namespace IGC
