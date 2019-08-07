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
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/IGCPassSupport.h"

#include "Compiler/LinkOpt.hpp"

#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/debug/Dump.hpp"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/ADT/SmallBitVector.h>
#include "common/LLVMWarningsPop.hpp"

#include <sstream>
#include <atomic>
#include <map>
#include <stdarg.h>

using namespace llvm;
using namespace std;

namespace IGC
{

    static const ShaderIOAnalysis::AnalysisType s_doIn = ShaderIOAnalysis::INPUT;
    static const ShaderIOAnalysis::AnalysisType s_doOut = ShaderIOAnalysis::OUTPUT;

    static const ShaderType s_vsType = ShaderType::VERTEX_SHADER;
    static const ShaderType s_hsType = ShaderType::HULL_SHADER;
    static const ShaderType s_dsType = ShaderType::DOMAIN_SHADER;
    static const ShaderType s_gsType = ShaderType::GEOMETRY_SHADER;
    static const ShaderType s_psType = ShaderType::PIXEL_SHADER;

    //////////////////////////////////////////////////////////////////////////////
    // API functions and facilities
    //////////////////////////////////////////////////////////////////////////////
    bool runPasses(CodeGenContext* ctx, ...)
    {
        llvm::legacy::PassManager mpm;
        va_list ap;
        Pass* p;
        va_start(ap, ctx);
        while ((p = va_arg(ap, Pass*)))
        {
            mpm.add(p);
        }
        va_end(ap);

        return mpm.run(*ctx->getModule());
    }

    uint getImmValueU32(const llvm::Value* value)
    {
        const llvm::ConstantInt* cval;
        cval = llvm::cast<llvm::ConstantInt>(value);
        assert(cval->getBitWidth() == 32);

        uint ival;
        ival = (uint)cval->getZExtValue();
        return ival;
    }

    static void LTODumpLLVMIR(CodeGenContext* ctx, const char* name)
    {
        if (IGC_IS_FLAG_ENABLED(EnableLTODebug))
        {
            DumpLLVMIR(ctx, name);
        }
    }


    //////////////////////////////////////////////////////////////////////////////
    // LinkOptContext
    //////////////////////////////////////////////////////////////////////////////

    static std::atomic<unsigned> NumLinkProgs;

    void LinkOptContext::initDebugDump()
    {
        if (IGC_IS_FLAG_ENABLED(EnableLTODebug))
        {
            std::ostringstream oss;

            oss << "link_" << NumLinkProgs;
            NumLinkProgs++;

            m_dump = new Debug::Dump(
                Debug::DumpName(oss.str())
                .Extension("log"),
                Debug::DumpType::DBG_MSG_TEXT);
        }
    }

    void LinkOptContext::closeDebugDump()
    {
        if (m_dump)
        {
            delete m_dump;
            m_dump = nullptr;
        }
    }

    void LinkOptContext::debugPrint(const char* fmt, ...)
    {
        if (IGC_IS_FLAG_ENABLED(EnableLTODebug) && this->m_dump)
        {
            va_list ap;
            va_start(ap, fmt);
            PrintDebugMsgV(this->m_dump, fmt, ap);
            va_end(ap);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    // ShaderIOAnalysis
    //////////////////////////////////////////////////////////////////////////////
    char ShaderIOAnalysis::ID = 0;

    void ShaderIOAnalysis::addInputDecl(llvm::GenIntrinsicInst* inst)
    {
        switch (m_shaderType)
        {
        case ShaderType::PIXEL_SHADER:
        {
            Value* val = inst->getOperand(INPUT_ATTR_ARG);
            uint imm = getImmValueU32(val);

            getContext()->addPSInput(inst, imm);
            break;
        }
        case ShaderType::DOMAIN_SHADER:
            break;
        default:
            assert(0);
            break;
        }
    }

    void ShaderIOAnalysis::addDSCtrlPtInputDecl(llvm::GenIntrinsicInst* inst)
    {
        assert(m_shaderType == ShaderType::DOMAIN_SHADER);
        if (isa<ConstantInt>(inst->getOperand(DSCTRLPTINPUT_CPID_ARG)) &&
            isa<ConstantInt>(inst->getOperand(DSCTRLPTINPUT_ATTR_ARG)))
        {
            uint ctrlIdx = getImmValueU32(inst->getOperand(DSCTRLPTINPUT_CPID_ARG));
            uint elemIdx = getImmValueU32(inst->getOperand(DSCTRLPTINPUT_ATTR_ARG));
            getContext()->addDSCtrlPtInput(inst, elemIdx, ctrlIdx);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }

    void ShaderIOAnalysis::addDSPatchConstInputDecl(llvm::GenIntrinsicInst* inst)
    {
        if (isa<ConstantInt>(inst->getOperand(DSPATCHCONSTINPUT_ATTR_ARG)))
        {
            uint idx = getImmValueU32(inst->getOperand(DSPATCHCONSTINPUT_ATTR_ARG));
            getContext()->addDSPatchConstInput(inst, idx);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }

    void ShaderIOAnalysis::addPatchConstOutput(GenIntrinsicInst* inst)
    {
        if (isa<ConstantInt>(inst->getOperand(PATCHCONSTOUTPUT_ATTR_ARG)))
        {
            Value* index = inst->getOperand(PATCHCONSTOUTPUT_ATTR_ARG);
            uint imm = getImmValueU32(index);

            getContext()->addHSPatchConstOutput(inst, imm);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }

    void ShaderIOAnalysis::addHSPatchConstInputDecl(llvm::GenIntrinsicInst* inst)
    {
        if (isa<ConstantInt>(inst->getOperand(HSPATCHCONSTINPUT_ATTR_ARG)))
        {
            uint idx = getImmValueU32(inst->getOperand(HSPATCHCONSTINPUT_ATTR_ARG));
            getContext()->addHSPatchConstInput(inst, idx);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }

    void ShaderIOAnalysis::addOutputDecl(GenIntrinsicInst* inst)
    {

        Value* usage = inst->getOperand(OUTPUT_USAGE_ARG);
        Value* index = inst->getOperand(OUTPUT_ATTR_ARG);
        ShaderOutputType otype = (ShaderOutputType)getImmValueU32(usage);

        if (!isa<ConstantInt>(index))
        {
            // GS may have variable index
            getContext()->m_abortLTO = true;
            return;
        }

        uint imm = getImmValueU32(index);

        if (otype == SHADER_OUTPUT_TYPE_DEFAULT)
        {
            switch (m_shaderType)
            {
            case ShaderType::VERTEX_SHADER:
                getContext()->addVSOutput(inst, imm);
                break;

            case ShaderType::DOMAIN_SHADER:
                getContext()->addDSOutput(inst, imm);
                break;

            case ShaderType::PIXEL_SHADER:
                break;

            case ShaderType::HULL_SHADER:
                getContext()->addHSPatchConstOutput(inst, imm);
                break;

            case ShaderType::GEOMETRY_SHADER:
                getContext()->addGSOutput(inst, imm);
                break;

            default:
                assert(false && "Unknown shader type for OUTPUT intrinsic");
            }
        }
        else if (otype == SHADER_OUTPUT_TYPE_POSITION ||
            otype == SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO ||
            otype == SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI ||
            otype == SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX ||
            otype == SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX ||
            otype == SHADER_OUTPUT_TYPE_POINTWIDTH)
        {
            switch (m_shaderType)
            {
            case ShaderType::VERTEX_SHADER:
                getContext()->addVSOutputNonDefault(inst, otype);
                break;

            case ShaderType::DOMAIN_SHADER:
                getContext()->addDSOutputNonDefault(inst, otype);
                break;
            default:
                break;
            }
        }
    }

    void ShaderIOAnalysis::addVSInputDecl(GenIntrinsicInst* inst)
    {
        assert(m_shaderType == ShaderType::VERTEX_SHADER);

        uint elemIdx = getImmValueU32(inst->getOperand(INPUT_ATTR_ARG));
        getContext()->addVSInput(inst, elemIdx);
    }

    void ShaderIOAnalysis::addHSInputDecl(GenIntrinsicInst* inst)
    {
        assert(m_shaderType == ShaderType::HULL_SHADER);
        if (isa<ConstantInt>(inst->getOperand(HSGSINPUT_ATTR_ARG)))
        {
            uint elemIdx = getImmValueU32(inst->getOperand(HSGSINPUT_ATTR_ARG));
            getContext()->addHSInput(inst, elemIdx);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }

    void ShaderIOAnalysis::addGSSVInputDecl(GenIntrinsicInst* inst)
    {
        assert(m_shaderType == ShaderType::GEOMETRY_SHADER);
        uint usage = getImmValueU32(inst->getOperand(GSSVINPUT_USAGE_ARG));
        getContext()->addGSInputNonDefault(inst, usage);
    }

    void ShaderIOAnalysis::addGSInputDecl(GenIntrinsicInst* inst)
    {
        assert(m_shaderType == ShaderType::GEOMETRY_SHADER);
        if (isa<ConstantInt>(inst->getOperand(HSGSINPUT_ATTR_ARG)))
        {
            uint elemIdx = getImmValueU32(inst->getOperand(HSGSINPUT_ATTR_ARG));
            getContext()->addGSInput(inst, elemIdx);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }

    // OutputTessControlPoint(x, y, z, w, i32 idx, i32 cpid)
    void ShaderIOAnalysis::addHSCtrlPtOutputDecl(GenIntrinsicInst* inst)
    {
        assert(m_shaderType == ShaderType::HULL_SHADER);
        if (isa<ConstantInt>(inst->getOperand(OUTPUTTESSCTRLPT_ATTR_ARG)))
        {
            uint attrIdx = getImmValueU32(inst->getOperand(OUTPUTTESSCTRLPT_ATTR_ARG));
            uint cpIdx = 0;
            getContext()->addHSCtrlPtOutput(inst, attrIdx, cpIdx);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }

    // <4 x float> = HSOutputCntrlPtInputVec(i32 vertex, i32 attr)
    void ShaderIOAnalysis::addHSOutputInputDecl(GenIntrinsicInst* inst)
    {
        assert(m_shaderType == ShaderType::HULL_SHADER);
        if (isa<ConstantInt>(inst->getOperand(HSOUTPUTCTRLPTINPUT_ATTR_ARG)))
        {
            uint attrIdx = getImmValueU32(inst->getOperand(HSOUTPUTCTRLPTINPUT_ATTR_ARG));
            getContext()->addHSOutputInput(inst, attrIdx);
        }
        else
        {
            getContext()->m_abortLTO = true;
        }
    }


    void ShaderIOAnalysis::onGenIntrinsic(GenIntrinsicInst* inst)
    {
        assert(inst != nullptr);
        switch (inst->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_OUTPUT:
        case GenISAIntrinsic::GenISA_OUTPUTGS:
            // float x, float y, float z, float w, i32 usage, i32 index_mask
            if (doOutput())
                addOutputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_PatchConstantOutput:
            if (doOutput())
                addPatchConstOutput(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_inputVec:
            // i32 input_index, i32 interpolation_mode
            if (doInput())
                addInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_Interpolate:
            // i32 input_index, v2f32 bary
            if (doInput())
                addInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_DSCntrlPtInputVec:
            if (doInput())
                addDSCtrlPtInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_ShaderInputVec:
            assert(m_shaderType == ShaderType::VERTEX_SHADER);
            if (doInput())
                addVSInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_HSinputVec:
            if (doInput())
                addHSInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_HSPatchConstInputVec:
            if (doOutput())
                addHSPatchConstInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_HSOutputCntrlPtInputVec:
            if (doOutput())
                addHSOutputInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_OutputTessControlPoint:
            if (doOutput())
                addHSCtrlPtOutputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_DSPatchConstInputVec:
            if (doInput())
                addDSPatchConstInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_GSsystemValue:
            if (doInput())
                addGSSVInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_GSinputVec:
            if (doInput())
                addGSInputDecl(inst);
            break;

        case GenISAIntrinsic::GenISA_DCL_input:
        case GenISAIntrinsic::GenISA_PHASE_INPUT:
            //Debug::PrintDebugMsg(m_dump, "%s\n", intrin_name);
            assert(false);
            break;

        case GenISAIntrinsic::GenISA_InnerScalarTessFactors:
        case GenISAIntrinsic::GenISA_OuterScalarTessFactors:
        case GenISAIntrinsic::GenISA_DCL_DSInputTessFactor:
            // todo, handle LTO on tess factors.
            break;


        default:
            break;
        }
    }

    bool ShaderIOAnalysis::runOnFunction(Function& F)
    {
        for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
        {
            for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
            {
                if (isa<GenIntrinsicInst>(II))
                {
                    onGenIntrinsic(cast<GenIntrinsicInst>(II));
                }
            }
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////////
    // IO Index Mapper - when consecutive PS input indices assignment is insufficient.
    // E.g. when is depends on some state maintained by this object.
    //////////////////////////////////////////////////////////////////////////////
    struct LTOIOIndexMapper
    {
        virtual ~LTOIOIndexMapper() {};
        // Get actual value of the index.
        virtual unsigned get() const = 0;
        // For i-th PS input, assign unique index to i-th element of the map.
        virtual void assignIndex(std::vector<int>& psIdxMap, unsigned i) = 0;
        // Ensure that the next unique index will be aligned appropriately.
        virtual void align(std::vector<int>& psIdxMap) = 0;
        // Update object's state for i-th index. If assignIndex() is about to be called for i-th index, then:
        // * finalizeIndex() should not be executed before it
        // * if finalizeIndex() is then called, the state should not change
        virtual void finalizeIndex(std::vector<int>& psIdxMap, unsigned i) = 0;
        // Call it when the whole psIn was processed.
        virtual void finalizeRemainder(std::vector<int>& psIdxMap, const VecOfIntrinVec& psIn) = 0;
    protected:
        static constexpr size_t attrAlignment = 4; // to oword boundary
    };

    // Most common variant - assign consecutive indices to PS inputs; straightforward alignment; no internal state.
    struct LTOPSIndexMapper : LTOIOIndexMapper
    {
        virtual unsigned get() const { return nPsIn; }
        virtual void assignIndex(std::vector<int>& psIdxMap, unsigned i) { psIdxMap[i] = nPsIn++; }
        virtual void align(std::vector<int>&) { nPsIn = iSTD::Align(nPsIn, attrAlignment); }
        virtual void finalizeIndex(std::vector<int>&, unsigned) {}
        virtual void finalizeRemainder(std::vector<int>&, const VecOfIntrinVec&) {}
    private:
        unsigned nPsIn = 0;
    };


    //////////////////////////////////////////////////////////////////////////////
    // Compact and link
    //////////////////////////////////////////////////////////////////////////////

    // returns true if the interpolation mode of i-th PS input attribute
    // (represented as input vector of all i-th PS input loads) is "constant interpolation"
    static bool isConstInterpolationInput(const IntrinVec& iv)
    {
        bool isConstInterpolation = false;

        for (const auto& inst : iv)
        {

            if (ConstantInt * modeVal = dyn_cast<ConstantInt>(
                inst->getOperand(ShaderIOAnalysis::INPUT_INTERPMODE_ARG)))
            {
                const e_interpolation mode =
                    static_cast<e_interpolation>(modeVal->getZExtValue());
                if (mode == EINTERPOLATION_CONSTANT)
                {
                    isConstInterpolation = true;
                }
                else
                {
                    assert(!isConstInterpolation && "Mixed const and non-const interpolation attributes?");
                }
            }
            else
            {
                assert(0 && "Dynamic interpolation mode is not allowed.");
                isConstInterpolation = true;
            }
        }

        return isConstInterpolation;
    }

    static bool isConstInterpolationOutput(const vector<Value*>& outVals)
    {
        bool constInterp = true;
        for (unsigned j = 1; j < outVals.size(); j++)
        {
            if (outVals[j] != outVals[0])
            {
                constInterp = false;
                break;
            }
        }
        return constInterp;
    }

    // If input vector elements (llvm values) refers to the same float constant,
    // return the pointer to this constant. Nullptr otherwise.
    static ConstantFP* isConstFPReplicated(const vector<Value*>& vals)
    {
        ConstantFP* const cfp = vals.size() > 0 ?
            dyn_cast<ConstantFP>(vals[0]) : nullptr;

        for (unsigned i = 1; cfp && i < vals.size(); i++)
        {
            if (cfp != vals[i])
            {
                return nullptr;
            }
        }

        return cfp;
    }

    // Compact PS input attributes. Since "Constant Interpolation Enable" field
    // in 3DSTATE_SBE will use 1 bit for 4 VS output attr components, so they
    // will be grouped together in the front, then align index by 4 and followed
    // by other input attrs.
    //
    // psIdxMap is the outputted mapping between old PS input index & new index
    //   newPSAttrIndex = psIdxMap[oldPSAttrIndex]
    static void compactPsInputs(
        CodeGenContext* psCtx,
        vector<int>& psIdxMap,
        ShaderType inputShaderType,
        const VecOfIntrinVec& psIn,
        const VecOfVec<Value*>& outVals,
        LTOPSActions& actions,
        LTOIOIndexMapper& indexMapper)
    {
        // GS only: count the number of constant interpolation attributes
        unsigned nConstInterp = 0;
        for (unsigned i = 0; inputShaderType == ShaderType::GEOMETRY_SHADER && i < psIn.size(); i++)
        {
            const IntrinVec& iv = psIn[i];

            // if last geometry stage's output is a replicated const, then do not count it
            // as constant interpolation attribute - output constants will be propagated
            if (!iv.empty() && !isConstFPReplicated(outVals[i]))
            {
                bool isConstInterpInput = isConstInterpolationInput(iv);

                if (!psCtx->m_DriverInfo.WADisableConstInterpolationPromotion() &&
                    isConstInterpolationOutput(outVals[i]))
                {
                    isConstInterpInput = true;
                }

                if (isConstInterpInput)
                {
                    nConstInterp++;
                }
            }
        }

        // Set of ps attrs that are either propagated constants, or constant interpolated attrs.
        set<int> psInClear;
        actions.resize(psIn.size());

        // cleanup all constants, gather const interpolation inputs
        for (unsigned i = 0; i < psIn.size(); i++)
        {
            const IntrinVec& iv = psIn[i];

            if (!iv.empty())
            {
                bool clean = false;

                if (const ConstantFP * cfp = isConstFPReplicated(outVals[i]))
                {
                    // if last geometry stage's output is const, then just propagate it
                    APFloat immf = cfp->getValueAPF();
                    unique_ptr<LTOPSAction> act(new LTOPSConstRepAction(immf));
                    actions[i] = std::move(act);
                    clean = true;
                }
                else
                {
                    // check and handle constant interpolate attrs
                    bool isConstInterpInput = isConstInterpolationInput(iv);

                    // check for GS output can be promoted to const interpolation
                    bool updateInterpMode = false;
                    if (inputShaderType == ShaderType::GEOMETRY_SHADER &&
                        !psCtx->m_DriverInfo.WADisableConstInterpolationPromotion() &&
                        isConstInterpolationOutput(outVals[i]) &&
                        !((nConstInterp & 0x3) == 1 && indexMapper.get() == nConstInterp - 1))
                    {
                        isConstInterpInput = true;
                        updateInterpMode = true;
                    }

                    if (isConstInterpInput)
                    {
                        assert(psIdxMap[i] == -1);
                        indexMapper.assignIndex(psIdxMap, i);
                        unique_ptr<LTOPSAction> act(
                            new LTOPSConstInterpAction(psIdxMap[i], updateInterpMode));
                        actions[i] = std::move(act);
                        clean = true;
                    }
                }

                if (clean)
                {
                    psInClear.insert(i);
                }
            }
            indexMapper.finalizeIndex(psIdxMap, i);
        }
        indexMapper.finalizeRemainder(psIdxMap, psIn);
        indexMapper.align(psIdxMap);

        // gather non-const interpolation inputs
        for (unsigned i = 0; i < psIn.size(); i++)
        {
            const IntrinVec& iv = psIn[i];

            if (!iv.empty() && psInClear.count(i) == 0)
            {
                if (psIdxMap[i] == -1)
                {
                    indexMapper.assignIndex(psIdxMap, i);
                }
                else
                {
                    assert(0);
                }
                assert(!actions[i]);
                unique_ptr<LTOPSAction> act(new LTOPSAdjustIndexAction(psIdxMap[i]));
                actions[i] = std::move(act);
            }
            indexMapper.finalizeIndex(psIdxMap, i);
        }
        indexMapper.finalizeRemainder(psIdxMap, psIn);
    }

    void applyPsLtoActions(
        VecOfIntrinVec& psIn,
        const LTOPSActions& actions)
    {
        for (unsigned i = 0; i < psIn.size(); i++)
        {
            IntrinVec& iv = psIn[i];
            if (!iv.empty() && actions[i])
            {
                for (auto& inst : iv)
                {
                    (*actions[i])(inst);
                }
            }
        }
    }

    // Compact VS/DS/GS output attributes based on PS attr index map, -1 in index
    // map means it's not used by PS.
    // Returns true if any output instruction was removed.
    static bool compactVsDsGsOutput(
        const ShaderType outputShaderType,
        CodeGenContext* vsdsCtx,
        const vector<int>& psIdxMap,
        VecOfIntrinVec& outInsts,
        const VecOfVec<Value*>& outVals)
    {
        unsigned numOut = outInsts.size() * 4;
        VecOfIntrinVec newOut(outInsts.size());
        unsigned nNewOut = 0;
        unsigned int maxIndex = 0;
        bool outputRemoved = false;

        // init all output attrs to undef
        // move output intrinsics to close to each other, for f64, we may see
        // non-contiguous output index
        Value* undef = UndefValue::get(Type::getFloatTy(*vsdsCtx->getLLVMContext()));
        for (unsigned i = 0; i < outInsts.size(); i++)
        {
            if (outInsts[i].size() > 0)
            {
                for (auto intrin : outInsts[i])
                {
                    intrin->setOperand(0, undef);
                    intrin->setOperand(1, undef);
                    intrin->setOperand(2, undef);
                    intrin->setOperand(3, undef);
                    intrin->setOperand(ShaderIOAnalysis::OUTPUT_ATTR_ARG,
                        ConstantInt::get(Type::getInt32Ty(*vsdsCtx->getLLVMContext()), nNewOut));
                    newOut[nNewOut].push_back(intrin);
                }
                nNewOut++;
            }
        }

        // fill-in live attrs to new location based on psIdxMap
        SmallBitVector liveOutInst(nNewOut);
        for (unsigned i = 0; i < numOut; i++)
        {
            if (psIdxMap[i] >= 0)
            {
                unsigned newIdx = psIdxMap[i];
                maxIndex = std::max(newIdx, maxIndex);
                if (newIdx / 4 >= nNewOut)
                {
                    // output attr is promoted to const interpolation, this may
                    // case the increasing of PS input attrs, so we need to create
                    // new output intrinsics in GS
                    assert(outputShaderType == ShaderType::GEOMETRY_SHADER);
                    newOut.resize(newIdx / 4 + 1);
                    liveOutInst.resize(newIdx / 4 + 1);
                    for (auto intrin : newOut[nNewOut - 1])
                    {
                        GenIntrinsicInst* newIntrin = cast<GenIntrinsicInst>(
                            intrin->clone());
                        newIntrin->setOperand(0, undef);
                        newIntrin->setOperand(1, undef);
                        newIntrin->setOperand(2, undef);
                        newIntrin->setOperand(3, undef);
                        newIntrin->setOperand(ShaderIOAnalysis::OUTPUT_ATTR_ARG,
                            ConstantInt::get(Type::getInt32Ty(*vsdsCtx->getLLVMContext()), newIdx / 4));
                        newIntrin->insertAfter(intrin);
                        newOut[newIdx / 4].push_back(newIntrin);
                    }
                    nNewOut = newIdx / 4 + 1;
                }
                assert(newOut[newIdx / 4].size() == outVals[i].size());
                for (unsigned j = 0; j < newOut[newIdx / 4].size(); j++)
                {
                    newOut[newIdx / 4][j]->setOperand(newIdx % 4, outVals[i][j]);
                }
                liveOutInst.set(newIdx / 4);
            }
        }

        // If the size of attribute is aligned on a cache line we force the beginning of the
        // attributes to be aligned on 64B to reduce the number of cachelines accessed by SBE
        if (iSTD::Align(maxIndex + 1, 8) % 16 == 0)
        {
            vsdsCtx->getModuleMetaData()->URBInfo.has64BVertexHeaderOutput = true;
        }

        // cleanup unused output intrinsics
        for (unsigned i = 0; i < nNewOut; i++)
        {
            if (!liveOutInst.test(i))
            {
                for (auto intrin : newOut[i])
                {
                    intrin->eraseFromParent();
                    outputRemoved = true;
                }
            }
        }

        return outputRemoved;
    }


    // Call it for last geometry stage, to link the geometry stage outputs with PS inputs.
    // Input: PS optimized, last geometry stage unified.
    // Output: dead attr removed, last geometry stage optimized.
    // Returns true if last geometry stage output was removed.
    static bool linkOptLastGeomStageToPs(
        LinkOptContext* linkCtx,
        ShaderType outShaderType,
        CodeGenContext* outCtx,
        std::vector<VecOfIntrinVec*>& vecOutInsts,
        LTOPSActions& actions)
    {
        bool preStageOutputRemoved = false;

        const uint numIoType = 1;
        const std::array<VecOfIntrinVec*, 1>  psInVec{ { &linkCtx->ps.inInsts } };

        for (uint ioType = 0; ioType < numIoType; ++ioType)
        {
            VecOfIntrinVec& outInsts = *vecOutInsts[ioType];

            if (outInsts.empty())
            {
                if (ioType == numIoType - 1)
                {
                    return preStageOutputRemoved;
                }
                else
                {
                    continue;
                }
            }

            const unsigned numOut = outInsts.size() * 4;
            VecOfVec<Value*> outVals(numOut);
            vector<int> psIdxMap(numOut, -1);   // -1 marks unused last geometry stage's output

            // gather vs output operands
            for (unsigned i = 0; i < outInsts.size(); i++)
            {
                for (const auto& intrin : outInsts[i])
                {
                    for (unsigned j = 0; j < 4; j++)
                    {
                        Value* val = intrin->getOperand(j);
                        outVals[i * 4 + j].push_back(val);
                    }
                }
            }

            VecOfIntrinVec& psIn = *psInVec[ioType];
            // there are cases where PS has more inputs than last geometry stage's outputs
            // e.g. gl_PointCoord
            if (psIn.size() > numOut)
            {
                psIn.resize(numOut);
            }

            {
                // make sure all PS input will have a last geometry stage's output
                Value* f0 = ConstantFP::get(Type::getFloatTy(*outCtx->getLLVMContext()), 0);
                for (unsigned i = 0; i < psIn.size(); i++)
                {
                    if (!psIn[i].empty())
                    {
                        if (!outVals[i].empty())
                        {
                            // for input with the output is undef, reset the output to 0.0f
                            for (unsigned j = 0; j < outVals[i].size(); j++)
                            {
                                if (isa<UndefValue>(outVals[i][j]))
                                {
                                    outVals[i][j] = f0;
                                }
                            }
                        }
                        else
                        {
                            // also for input with missing output, reset the output to 0.0f
                            outVals[i].resize(psIn[i].size(), f0);
                        }
                    }
                }
            }

            std::unique_ptr<LTOIOIndexMapper> indexMapper(static_cast<LTOIOIndexMapper*>(new LTOPSIndexMapper));

            if (CodeGenContext * psCtx = linkCtx->getPS())
            {
                compactPsInputs(psCtx, psIdxMap, outShaderType, psIn, outVals, actions, *indexMapper);
                applyPsLtoActions(psIn, actions);
            }

            {
                preStageOutputRemoved = compactVsDsGsOutput(outShaderType, outCtx, psIdxMap, outInsts, outVals) ||
                    preStageOutputRemoved;
            }
        }

        // whether we have output values removed, so that we can do DCR again
        // to remove operations producing them
        return preStageOutputRemoved;
    }

    static bool linkOptHsToDs(LinkOptContext* linkCtx)
    {
        bool hsOutRemoved = false;
        vector<vector<list<GenIntrinsicInst*> > >& dsIn = linkCtx->ds.inInsts;
        VecOfIntrinVec& hsOut = linkCtx->hs.outInsts;
        map<unsigned, IntrinVec >& hsOutIn = linkCtx->hs.outIn;

        CodeGenContext* dsCtx = linkCtx->getDS();
        CodeGenContext* hsCtx = linkCtx->getHS();

        // optimize control points
        assert(hsOut.size() >= dsIn.size());
        unsigned nDead = 0;
        for (unsigned i = 0; i < dsIn.size(); i++)
        {
            if (dsIn[i].size() == 0 && hsOutIn.find(i) == hsOutIn.end())
            {
                nDead++;
                for (auto inst : hsOut[i])
                {
                    inst->eraseFromParent();
                    hsOutRemoved = true;
                }
            }
            else
            {
                if (nDead)
                {
                    for (unsigned c = 0; c < dsIn[i].size(); c++)
                    {
                        for (auto inst : dsIn[i][c])
                        {
                            inst->setOperand(ShaderIOAnalysis::DSCTRLPTINPUT_ATTR_ARG,
                                ConstantInt::get(Type::getInt32Ty(*dsCtx->getLLVMContext()), i - nDead));
                        }
                    }
                    for (auto inst : hsOut[i])
                    {
                        inst->setOperand(ShaderIOAnalysis::OUTPUTTESSCTRLPT_ATTR_ARG,
                            ConstantInt::get(Type::getInt32Ty(*hsCtx->getLLVMContext()), i - nDead));
                    }
                    for (auto inst : hsOutIn[i])
                    {
                        inst->setOperand(1,
                            ConstantInt::get(Type::getInt32Ty(*hsCtx->getLLVMContext()), i - nDead));
                    }
                }
            }
        }

        nDead = 0;
        for (unsigned i = dsIn.size(); i < hsOut.size(); i++)
        {
            if (hsOutIn.find(i) == hsOutIn.end())
            {
                nDead++;
                for (auto inst : hsOut[i])
                {
                    inst->eraseFromParent();
                    hsOutRemoved = true;
                }
            }
            else
            {
                if (nDead)
                {
                    for (auto inst : hsOut[i])
                    {
                        inst->setOperand(ShaderIOAnalysis::OUTPUTTESSCTRLPT_ATTR_ARG,
                            ConstantInt::get(Type::getInt32Ty(*hsCtx->getLLVMContext()), i - nDead));
                    }
                    for (auto inst : hsOutIn[i])
                    {
                        inst->setOperand(1,
                            ConstantInt::get(Type::getInt32Ty(*hsCtx->getLLVMContext()), i - nDead));
                    }
                }
            }
        }

        // optimize patch constants
        VecOfIntrinVec& pcOut = linkCtx->hs.pcOut;
        VecOfIntrinVec& hspcIn = linkCtx->hs.pcIn;
        VecOfIntrinVec& dspcIn = linkCtx->ds.pcIn;

        assert(pcOut.size() >= hspcIn.size() && pcOut.size() >= dspcIn.size());
        unsigned sidx = 0;
        for (unsigned i = 0; i < pcOut.size(); i++)
        {
            if ((i < hspcIn.size() && hspcIn[i].size()) ||
                (i < dspcIn.size() && dspcIn[i].size()))
            {
                assert(pcOut[i].size());
                Value* dsv_sidx = ConstantInt::get(
                    Type::getInt32Ty(*dsCtx->getLLVMContext()), sidx);
                Value* hsv_sidx = ConstantInt::get(
                    Type::getInt32Ty(*hsCtx->getLLVMContext()), sidx);

                if (i < hspcIn.size())
                {
                    for (auto inst : hspcIn[i])
                    {
                        inst->setOperand(ShaderIOAnalysis::HSPATCHCONSTINPUT_ATTR_ARG, hsv_sidx);
                    }
                }

                if (i < dspcIn.size())
                {
                    for (auto inst : dspcIn[i])
                    {
                        inst->setOperand(ShaderIOAnalysis::DSPATCHCONSTINPUT_ATTR_ARG, dsv_sidx);
                    }
                }

                unsigned attrIdx;
                if (pcOut[i][0]->getIntrinsicID() == GenISAIntrinsic::GenISA_OUTPUT)
                {
                    attrIdx = ShaderIOAnalysis::OUTPUT_ATTR_ARG;
                }
                else
                {
                    assert(pcOut[i][0]->getIntrinsicID() ==
                        GenISAIntrinsic::GenISA_PatchConstantOutput);
                    attrIdx = ShaderIOAnalysis::PATCHCONSTOUTPUT_ATTR_ARG;
                }

                for (auto inst : pcOut[i])
                {
                    inst->setOperand(attrIdx, hsv_sidx);
                }

                pcOut[i].clear();

                sidx++;
            }
        }

        for (unsigned i = 0; i < pcOut.size(); i++)
        {
            if (pcOut[i].size())
            {
                for (auto inst : pcOut[i])
                {
                    inst->eraseFromParent();
                }
            }
        }

        return hsOutRemoved;
    }

    const static std::multimap<unsigned, unsigned> outputTypeToUsage =
    {
        { SHADER_OUTPUT_TYPE_POSITION, POSITION_X },
        { SHADER_OUTPUT_TYPE_POSITION, POSITION_Y },
        { SHADER_OUTPUT_TYPE_POSITION, POSITION_Z },
        { SHADER_OUTPUT_TYPE_POSITION, POSITION_W },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO, CLIP_DISTANCE_X },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO, CLIP_DISTANCE_Y },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO, CLIP_DISTANCE_Z },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO, CLIP_DISTANCE_W },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI, CLIP_DISTANCE_X },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI, CLIP_DISTANCE_Y },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI, CLIP_DISTANCE_Z },
        { SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI, CLIP_DISTANCE_W },
        { SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX, RENDER_TARGET_ARRAY_INDEX },
        { SHADER_OUTPUT_TYPE_POINTWIDTH, POINT_WIDTH }
    };

    // link opt between VecOfIntrinVec outputs & VecOfIntrinVec inputs
    // VS -> HS or GS
    // DS -> GS
    static void linkOptVovToVov(LinkOptContext* linkCtx,
        CodeGenContext* outCtx,
        VecOfIntrinVec& outInsts,
        VecOfIntrinVec& outNonDefaultInsts,
        CodeGenContext* inCtx,
        VecOfIntrinVec& inInsts,
        VecOfIntrinVec& inNonDefaultInsts)
    {
        unsigned nDead = 0;
        for (unsigned i = 0; i < inInsts.size(); i++)
        {
            if (inInsts[i].size() == 0)
            {
                if (outInsts[i].size() != 0)
                {
                    assert(outInsts[i].size() == 1);
                    nDead++;
                    outInsts[i][0]->eraseFromParent();
                }
            }
            else
            {
                if (nDead)
                {
                    GenIntrinsicInst* inst;
                    for (unsigned j = 0; j < inInsts[i].size(); j++)
                    {
                        inst = inInsts[i][j];
                        inst->setOperand(ShaderIOAnalysis::HSGSINPUT_ATTR_ARG,
                            ConstantInt::get(Type::getInt32Ty(*inCtx->getLLVMContext()), i - nDead));
                    }
                    assert(outInsts[i].size() == 1);
                    inst = outInsts[i][0];
                    inst->setOperand(ShaderIOAnalysis::OUTPUT_ATTR_ARG,
                        ConstantInt::get(Type::getInt32Ty(*outCtx->getLLVMContext()), i - nDead));
                }
            }
        }
        for (unsigned i = inInsts.size(); i < outInsts.size(); i++)
        {
            if (outInsts[i].size() != 0)
            {
                assert(outInsts[i].size() == 1);
                outInsts[i][0]->eraseFromParent();
            }
        }
        unsigned outputClipCullDistances = (SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI < outNonDefaultInsts.size() ? 1 : 0) +
            (SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO < outNonDefaultInsts.size() ? 1 : 0);
        for (unsigned i = 0; i < outNonDefaultInsts.size(); i++)
        {
            if (!outNonDefaultInsts[i].empty())
            {
                auto inputUsages = outputTypeToUsage.equal_range(ShaderOutputType(i));
                if (all_of(inputUsages.first, inputUsages.second,
                    [&inNonDefaultInsts](const std::pair<const unsigned, unsigned>& iter)
                { return iter.second >= inNonDefaultInsts.size() || inNonDefaultInsts[iter.second].empty(); }))
                {
                    assert(outNonDefaultInsts[i].size() == 1);
                    outNonDefaultInsts[i][0]->eraseFromParent();
                    if (ShaderOutputType(i) == SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI ||
                        ShaderOutputType(i) == SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO)
                    {
                        outputClipCullDistances--;
                    }
                }
            }
        }
        GlobalVariable* shaderInputClipCullDistances = inCtx->getModule()->getGlobalVariable("ShaderHasClipCullInput");
        if (outputClipCullDistances == 0 && shaderInputClipCullDistances)
        {
            shaderInputClipCullDistances->eraseFromParent();
        }
    }

    static void ltoPrepare(LinkOptContext* linkContext)
    {
        linkContext->initDebugDump();

        linkContext->debugPrint("Link program:\n");
        for (int i = 0; i < LTO_NUM_SHADER_STAGES; i++)
        {
            CodeGenContext* pctx = linkContext->getContext((ShaderType)i);
            ShaderType stype = (ShaderType)i;
            if (pctx != nullptr)
            {
#if defined(_DEBUG)
                if (!pctx->m_hasLegacyDebugInfo)
                    llvm::verifyModule(*pctx->getModule());
#endif
                linkContext->debugPrint(
                    "  %s %016llx\n",
                    ShaderTypeString[int(stype)],
                    pctx->hash.getAsmHash());
            }
        }
    }

    static void ltoDestroy(LinkOptContext* linkContext)
    {
        linkContext->closeDebugDump();
    }

    static ShaderType ltoToPS(LinkOptContext* ltoCtx, LTOPSActions& actions)
    {
        CodeGenContext* psCtx = ltoCtx->getPS();
        CodeGenContext* gsCtx = ltoCtx->getGS();
        CodeGenContext* dsCtx = ltoCtx->getDS();
        CodeGenContext* vsCtx = ltoCtx->getVS();
        CodeGenContext* prePsCtx;
        std::vector<VecOfIntrinVec*> prePsOuts;
        ShaderType prevType;

        if (gsCtx != nullptr)
        {
            prePsCtx = gsCtx;
            prevType = s_gsType;
            prePsOuts.push_back(&ltoCtx->gs.outInsts);
        }
        else if (dsCtx != nullptr)
        {
            prePsCtx = dsCtx;
            prevType = s_dsType;
            prePsOuts.push_back(&ltoCtx->ds.outInsts);
        }
        else if (vsCtx != nullptr)
        {
            prePsCtx = vsCtx;
            prevType = s_vsType;
            prePsOuts.push_back(&ltoCtx->vs.outInsts);
        }
        else
        {
            return s_vsType;
        }

        ltoCtx->m_abortLTO = false;
        runPasses(prePsCtx,
            new ShaderIOAnalysis(ltoCtx, prevType, s_doOut),
            nullptr);
        if (psCtx)
        {
            runPasses(psCtx,
                new ShaderIOAnalysis(ltoCtx, s_psType, s_doIn),
                nullptr);
        }

        if (!ltoCtx->m_abortLTO)
        {
            if (psCtx)
            {
                LTODumpLLVMIR(psCtx, "beLTOI");
            }
            LTODumpLLVMIR(prePsCtx, "beLTOO");

            if (linkOptLastGeomStageToPs(ltoCtx, prevType, prePsCtx, prePsOuts, actions))
            {
                // Outputs were removed from preStage, so DCR make sense in such case.
                runPasses(prePsCtx, createDeadCodeEliminationPass(), nullptr);
            }
            if (psCtx)
            {
                LTODumpLLVMIR(psCtx, "afLTOI");
            }
            LTODumpLLVMIR(prePsCtx, "afLTOO");
        }

        return prevType;
    }

    static ShaderType ltoToGs(LinkOptContext* ltoCtx)
    {
        CodeGenContext* gsCtx = ltoCtx->getGS();
        CodeGenContext* dsCtx = ltoCtx->getDS();
        CodeGenContext* vsCtx = ltoCtx->getVS();
        CodeGenContext* preGsCtx;
        VecOfIntrinVec* preGsOuts;
        VecOfIntrinVec* preGsNonDefaultOuts;
        ShaderType prevType;

        if (dsCtx != nullptr)
        {
            // ds -> gs
            prevType = s_dsType;
            preGsCtx = dsCtx;
            preGsOuts = &ltoCtx->ds.outInsts;
            preGsNonDefaultOuts = &ltoCtx->ds.outNonDefaultInsts;
        }
        else
        {
            // vs -> gs
            prevType = s_vsType;
            preGsCtx = vsCtx;
            preGsOuts = &ltoCtx->vs.outInsts;
            preGsNonDefaultOuts = &ltoCtx->vs.outNonDefaultInsts;
        }

        if (preGsCtx != nullptr)
        {
            ltoCtx->m_abortLTO = false;
            runPasses(preGsCtx,
                new ShaderIOAnalysis(ltoCtx, prevType, s_doOut),
                nullptr);
            runPasses(gsCtx,
                llvm::createDeadCodeEliminationPass(),
                new ShaderIOAnalysis(ltoCtx, s_gsType, s_doIn),
                nullptr);
        }
        if (!ltoCtx->m_abortLTO)
        {
            LTODumpLLVMIR(gsCtx, "beLTOI");    LTODumpLLVMIR(preGsCtx, "beLTOO");
            linkOptVovToVov(ltoCtx,
                preGsCtx, *preGsOuts, *preGsNonDefaultOuts,
                gsCtx, ltoCtx->gs.inInsts, ltoCtx->gs.inNonDefaultInsts);
            LTODumpLLVMIR(gsCtx, "afLTOI");   LTODumpLLVMIR(preGsCtx, "afLTOO");
        }

        return prevType;
    }

    static ShaderType ltoToDs(LinkOptContext* ltoCtx)
    {
        CodeGenContext* dsCtx = ltoCtx->getDS();
        CodeGenContext* hsCtx = ltoCtx->getHS();
        ShaderType prevType;

        assert(dsCtx != nullptr && hsCtx != nullptr);
        prevType = s_hsType;

        ltoCtx->m_abortLTO = false;

        runPasses(hsCtx,
            new ShaderIOAnalysis(ltoCtx, s_hsType, s_doOut),
            nullptr);
        runPasses(dsCtx,
            llvm::createDeadCodeEliminationPass(),
            new ShaderIOAnalysis(ltoCtx, s_dsType, s_doIn),
            nullptr);

        if (!ltoCtx->m_abortLTO)
        {
            LTODumpLLVMIR(dsCtx, "beLTOI");    LTODumpLLVMIR(hsCtx, "beLTOO");
            linkOptHsToDs(ltoCtx);
            LTODumpLLVMIR(dsCtx, "afLTOI");   LTODumpLLVMIR(hsCtx, "afLTOO");
        }

        return prevType;
    }

    static ShaderType ltoToHs(LinkOptContext* ltoCtx)
    {
        CodeGenContext* hsCtx = ltoCtx->getHS();
        CodeGenContext* vsCtx = ltoCtx->getVS();
        ShaderType prevType;

        assert(vsCtx != nullptr);
        prevType = s_vsType;

        ltoCtx->m_abortLTO = false;
        runPasses(vsCtx,
            new ShaderIOAnalysis(ltoCtx, s_vsType, s_doOut),
            nullptr);
        runPasses(hsCtx,
            llvm::createDeadCodeEliminationPass(),
            new ShaderIOAnalysis(ltoCtx, s_hsType, s_doIn),
            nullptr);

        if (!ltoCtx->m_abortLTO)
        {
            LTODumpLLVMIR(hsCtx, "beLTOI");    LTODumpLLVMIR(vsCtx, "beLTOO");
            linkOptVovToVov(ltoCtx,
                vsCtx, ltoCtx->vs.outInsts, ltoCtx->vs.outNonDefaultInsts,
                hsCtx, ltoCtx->hs.inInsts, ltoCtx->hs.inNonDefaultInsts);
            LTODumpLLVMIR(hsCtx, "afLTOI");   LTODumpLLVMIR(vsCtx, "afLTOO");
        }

        return prevType;
    }

    void LinkOptIRGetPSActions(CodeGenContext* ctxs[], bool usesStreamOutput,
        LTOPSActions& actions)
    {
        if (!IGC_IS_FLAG_ENABLED(EnableLTO))
        {
            return;
        }

        LinkOptContext linkContextObj;
        for (unsigned i = 0; i < LTO_NUM_SHADER_STAGES; i++)
        {
            linkContextObj.setContext((ShaderType)i, ctxs[i]);
        }

        LinkOptContext* ltoCtx = &linkContextObj;
        ltoPrepare(ltoCtx);

        CodeGenContext* psCtx = ltoCtx->getPS();
        CodeGenContext* gsCtx = ltoCtx->getGS();
        CodeGenContext* dsCtx = ltoCtx->getDS();
        CodeGenContext* hsCtx = ltoCtx->getHS();
        CodeGenContext* vsCtx = ltoCtx->getVS();

        if ((hsCtx != nullptr && dsCtx == nullptr) ||
            (hsCtx == nullptr && dsCtx != nullptr))
        {
            // skip some weired cases only having HS or DS
            ltoDestroy(ltoCtx);
            return;
        }

        ShaderType prevType;

        if (usesStreamOutput)
        {
            if (gsCtx != nullptr)
            {
                prevType = s_gsType;
            }
            else if (dsCtx != nullptr)
            {
                prevType = s_dsType;
            }
            else
            {
                prevType = s_vsType;
            }
        }
        else
        {
            prevType = ltoToPS(ltoCtx, actions);
        }

        while (prevType != ShaderType::VERTEX_SHADER)
        {
            switch (prevType)
            {
            case ShaderType::GEOMETRY_SHADER:
                prevType = ltoToGs(ltoCtx);
                break;

            case ShaderType::DOMAIN_SHADER:
                prevType = ltoToDs(ltoCtx);
                prevType = ltoToHs(ltoCtx);
                break;


            default:
                assert(false && "Internal error in link opt!");
            }
        }

        if (psCtx)  DumpLLVMIR(psCtx, "lto");
        if (gsCtx)  DumpLLVMIR(gsCtx, "lto");
        if (dsCtx)  DumpLLVMIR(dsCtx, "lto");
        if (hsCtx)  DumpLLVMIR(hsCtx, "lto");
        if (vsCtx)  DumpLLVMIR(vsCtx, "lto");

        ltoDestroy(ltoCtx);
    }

    void LinkOptReplayPSActions(PixelShaderContext* psCtx,
        const LTOPSActions& psActions)
    {
        LinkOptContext ltoCtx;
        ltoCtx.setContext(ShaderType::PIXEL_SHADER, psCtx);

        runPasses(psCtx,
            new ShaderIOAnalysis(&ltoCtx, s_psType, s_doIn),
            nullptr);
        VecOfIntrinVec& psIn = ltoCtx.ps.inInsts;

        applyPsLtoActions(psIn, psActions);
        DumpLLVMIR(psCtx, "lto");
    }

    void LinkOptIR(CodeGenContext* ctxs[], bool usesStreamOutput)
    {
        LTOPSActions actions;
        LinkOptIRGetPSActions(ctxs, usesStreamOutput, actions);
    }

} // namespace IGC
