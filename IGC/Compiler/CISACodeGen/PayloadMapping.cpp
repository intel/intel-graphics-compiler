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

//===----------------------------------------------------------------------===//
//
// A helper (static) class that returns a mapping from message generating
// intrinsic (e.g. sample, load, urb_write) arguments to their respective
// positions in the payload message.
//
//===----------------------------------------------------------------------===//

#include "Compiler/CISACodeGen/PayloadMapping.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "Compiler/CISACodeGen/helper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/BasicBlock.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

///
const Instruction* PayloadMapping::GetSupremumOfNonHomogeneousPart(
    const Instruction* inst1,
    const Instruction* inst2)
{
    const GenIntrinsicInst* intrinsicInst1 = dyn_cast<GenIntrinsicInst>(inst1);
    const GenIntrinsicInst* intrinsicInst2 = dyn_cast<GenIntrinsicInst>(inst2);

    assert(intrinsicInst1);
    assert(intrinsicInst2);
    assert(intrinsicInst1->getIntrinsicID() == GenISAIntrinsic::GenISA_RTWrite);
    assert(intrinsicInst2->getIntrinsicID() == GenISAIntrinsic::GenISA_RTWrite);

    switch (intrinsicInst1->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_RTWrite:
        return GetSupremumOfNonHomogeneousPart_RTWrite(inst1, inst2);
    default:
        assert(0); //should not be called at all for other intrinsics
        return nullptr;
    }
}


/// Helper method.
/// compare rtwi1 and rtwi2 w.r.t. oMask
/// biased towards rtwi1:
/// X X -> return rtwi1
/// X 2 -> return rtwi2
/// 1 X -> return rtwi1
/// 1 2 -> return rtwi1
static const Instruction* CompareOnMask(
    const RTWritIntrinsic* rtwi1,
    const RTWritIntrinsic* rtwi2)
{
    if (rtwi1->hasMask() && rtwi2->hasMask())
    {
        return rtwi1;
    }
    /// rtwi1 S0 < rtwi2 S0
    else if (!rtwi1->hasMask() && rtwi2->hasMask())
    {
        return rtwi2;
    }
    /// rtwi1 S0 > rtwi2 S0
    else if (rtwi1->hasMask() && !rtwi2->hasMask())
    {
        return rtwi1;
    }

    return rtwi1;
}

///
const Instruction* PayloadMapping::GetSupremumOfNonHomogeneousPart_RTWrite(
    const Instruction* inst1,
    const Instruction* inst2)
{
    assert(llvm::isa<llvm::RTWritIntrinsic>(inst1));
    assert(llvm::isa<llvm::RTWritIntrinsic>(inst2));
    ModuleMetaData* modMD = m_CodeGenContext->getModuleMetaData();
    const RTWritIntrinsic* rtwi1 = cast<RTWritIntrinsic>(inst1);
    const RTWritIntrinsic* rtwi2 = cast<RTWritIntrinsic>(inst2);

    /// rtwi1 S0 == rtwi2 S0
    if (RTWriteHasSource0Alpha(rtwi1, modMD) && RTWriteHasSource0Alpha(rtwi2, modMD))
    {
        return CompareOnMask(rtwi1, rtwi2);
    }
    /// rtwi1 X < rtwi2 S0
    else if (!RTWriteHasSource0Alpha(rtwi1, modMD) && RTWriteHasSource0Alpha(rtwi2, modMD))
    {
        return CompareOnMask(rtwi2, rtwi1) == rtwi2 ? rtwi2 : nullptr;
    }
    /// rtwi1 S0 > rtwi2 S0
    else if (RTWriteHasSource0Alpha(rtwi1, m_CodeGenContext->getModuleMetaData()) && !RTWriteHasSource0Alpha(rtwi2, modMD))
    {
        return CompareOnMask(rtwi1, rtwi2) == rtwi1 ? rtwi1 : nullptr;
    }

    return nullptr;
}


/// Return the 'reserved' left offset of modeled payload (in bytes).
/// By default, homogeneous payload messages returns 0, but if one
/// wants to model the 'reserved' part of the payload (which cannot
/// be coalesced due to non-homogeneous) this method should handle
/// that intrinsic and return the offset (in units of bytes).
int PayloadMapping::GetLeftReservedOffset(const Instruction* inst, SIMDMode simdMode)
{
    const GenIntrinsicInst* intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
    assert(intrinsicInst);

    switch (intrinsicInst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_RTWrite:
        return GetLeftReservedOffset_RTWrite(intrinsicInst, simdMode);
    default:
        return 0;
    }
}

///
int PayloadMapping::GetLeftReservedOffset_RTWrite(const Instruction* inst, SIMDMode simdMode)
{
    assert(llvm::isa<llvm::RTWritIntrinsic>(inst));
    const RTWritIntrinsic* rtwi = cast<RTWritIntrinsic>(inst);

    int offset = 0;

    if (rtwi->hasMask())
    {
        //Output mask is always fixed size (32 bytes, UW)
        offset += 32; //256bits=1grf
    }

    if (RTWriteHasSource0Alpha(rtwi, m_CodeGenContext->getModuleMetaData()))
    {
        assert(simdMode == SIMDMode::SIMD8 || simdMode == SIMDMode::SIMD16);
        int multiplier = rtwi->getSource0Alpha()->getType()->isHalfTy() ? 1 : 2;
        if (simdMode == SIMDMode::SIMD8)
        {
            offset += 32; //always 1GRF, regardless of HF
        }
        else if (simdMode == SIMDMode::SIMD16)
        {
            offset += 32 * multiplier; // 2GRFs normal precision, 1GRF HF precision
        }
    }

    return offset;
}

/// 
int PayloadMapping::GetRightReservedOffset(const Instruction* inst, SIMDMode simdMode)
{
    const GenIntrinsicInst* intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
    assert(intrinsicInst);

    switch (intrinsicInst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_RTWrite:
        return GetRightReservedOffset_RTWrite(intrinsicInst, simdMode);
    default:
        return 0;
    }
}

int PayloadMapping::GetRightReservedOffset_RTWrite(const Instruction* inst, SIMDMode simdMode)
{
    assert(llvm::isa<llvm::RTWritIntrinsic>(inst));
    const RTWritIntrinsic* rtwi = cast<RTWritIntrinsic>(inst);

    int offset = 0;

    if (rtwi->hasStencil())
    {
        //Must not be set in simd16 mode.
        assert(simdMode == SIMDMode::SIMD8);
        offset += 32; //256bits=1grf
    }

    if (rtwi->hasDepth())
    {
        assert(simdMode == SIMDMode::SIMD8 || simdMode == SIMDMode::SIMD16);

        //Depth always has normal precision, regardless of the rest of the fields.
        if (simdMode == SIMDMode::SIMD8)
        {
            offset += 32; //always 1GRF, 256 bytes
        }
        else if (simdMode == SIMDMode::SIMD16)
        {
            offset += 64; // 2GRFs normal precision
        }
    }

    return offset;
}

/// Returns true if a given intrinsic (and its specific configuration) will produce
/// non-homogeneous payload (the one in which some elements are not 'expanded' with simd mode)
/// e.g. RT writes with oMask
bool PayloadMapping::HasNonHomogeneousPayloadElements(const Instruction* inst)
{
    const GenIntrinsicInst* intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
    assert(intrinsicInst);

    switch (intrinsicInst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_RTWrite:
        return HasNonHomogeneousPayloadElements_RTWrite(intrinsicInst);
    default:
        return false;
    }

}

///
bool PayloadMapping::HasNonHomogeneousPayloadElements_RTWrite(const Instruction* inst)
{
    assert(llvm::isa<llvm::RTWritIntrinsic>(inst));
    const RTWritIntrinsic* rtwi = cast<RTWritIntrinsic>(inst);

    if (rtwi->hasMask())
    {
        return true;
    }
    if (RTWriteHasSource0Alpha(rtwi, m_CodeGenContext->getModuleMetaData()))
    {
        return true;
    }
    if (rtwi->hasDepth())
    {
        return true;
    }
    if (rtwi->hasStencil())
    {
        return true;
    }

    return false;
}

/// \brief
bool PayloadMapping::IsUndefOrZeroImmediate(const Value* value)
{
    if (llvm::isa<llvm::UndefValue>(value))
    {
        return true;
    }

    if (const llvm::ConstantInt * CInt = llvm::dyn_cast<llvm::ConstantInt>(value)) {
        if (CInt->getZExtValue() == 0) {
            return true;
        }
    }

    if (const llvm::ConstantFP * CFP = llvm::dyn_cast<llvm::ConstantFP>(value))
    {
        APInt api = CFP->getValueAPF().bitcastToAPInt();
        if (api.getZExtValue() == 0)
        {
            return true;
        }
    }
    return false;
}

/// Return the number of payload elements that this instruction will
/// generate.
uint PayloadMapping::GetNumPayloadElements(const Instruction* inst)
{
    const GenIntrinsicInst* intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
    assert(intrinsicInst);
    if (const SampleIntrinsic * sampleInst = dyn_cast<SampleIntrinsic>(inst))
    {
        return GetNumPayloadElements_Sample(sampleInst);
    }
    else if (const SamplerLoadIntrinsic * sampleInst = dyn_cast<SamplerLoadIntrinsic>(inst))
    {
        return GetNumPayloadElements_LDMS(sampleInst);
    }

    switch (intrinsicInst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_URBWrite:
        return GetNumPayloadElements_URBWrite(intrinsicInst);
    case GenISAIntrinsic::GenISA_RTWrite:
        return GetNumPayloadElements_RTWrite(intrinsicInst);
    default:
        break;
    }

    assert(0);
    return 0;
}

/// \brief
uint PayloadMapping::GetNumPayloadElements_LDMS(const GenIntrinsicInst* inst)
{
    const uint numOperands = inst->getNumOperands();

    //Subtract the offsets, and texture resource, lod to get 
    //the number of texture coordinates and index to texture source
    uint numSources = numOperands - 5;

    for (uint i = numSources - 1; i > 0; i--)
    {
        if (IsUndefOrZeroImmediate(inst->getOperand(i)))
        {
            numSources--;
        }
        else
        {
            break;
        }
    }

    return numSources;
}

/// \brief Adjusts the number of sources for a sampler, based on a sampler type.
void PayloadMapping::ValidateNumberofSources(EOPCODE opCode, uint& numberofSrcs)
{
    switch (opCode)
    {
    case llvm_sample_dptr:
    {
        switch (numberofSrcs)
        {
        case 1:
            numberofSrcs++;
        case 2:
            numberofSrcs++;
            break;
        case 5:
            numberofSrcs++;
            break;
        case 8:
            numberofSrcs++;
            break;
        }
    }
    break;
    case llvm_sample_dcptr:
    {
        switch (numberofSrcs)
        {
        case 2:
            numberofSrcs++;
        case 3:
            numberofSrcs++;
            break;
        case 5:
            numberofSrcs++;
        case 6:
            numberofSrcs++;
            break;
        case 8:
            numberofSrcs++;
        case 9:
            numberofSrcs++;
            break;
        }
    }
    break;
    case llvm_sample_bptr:
    case llvm_sample_cptr:
    case llvm_sample_lptr:
    {
        switch (numberofSrcs)
        {
        case 1:
            numberofSrcs++;
            break;
        }
    }
    break;
    case llvm_sample_lcptr:
    case llvm_sample_bcptr:
    {
        switch (numberofSrcs)
        {
        case 1:
            numberofSrcs++;
        case 2:
            numberofSrcs++;
            break;
        }
    }
    break;
    default:
        break;
    }

}

/// \brief Provides the total number of payload elements for a sample.
///
/// Does not consider 'peeling' of the first element for split-send.
/// The peeling itself is supposed to be done by a wrapper method.
uint PayloadMapping::GetNonAdjustedNumPayloadElements_Sample(const SampleIntrinsic* inst)
{
    const unsigned int  numOperands = inst->getNumOperands();
    unsigned int numSources = numOperands - 6;

    if (inst->IsLODInst())
    {
        //Subtract resource and sampler sources to get 
        //the number of texture coordinates and index to texture source
        numSources = numOperands - 3;
    }

    //Check for valid number of sources from the end of the list
    for (uint i = (numSources - 1); i >= 1; i--)
    {
        if (IsUndefOrZeroImmediate(inst->getOperand(i)))
        {
            numSources--;
        }
        else
        {
            break;
        }
    }

    //temp solution to send valid sources but having 0 as their values.
    EOPCODE opCode = GetOpCode(inst);
    ValidateNumberofSources(opCode, numSources);

    if (IsZeroLOD(inst))
    {
        numSources--;
    }

    return numSources;
}

/// \brief Gets the adjusted number of payload elements for sampler.
///
/// Whether to peel is determined by the target GEN architecture.
uint PayloadMapping::GetNumPayloadElements_Sample(const SampleIntrinsic* inst)
{
    unsigned int numSources = GetNonAdjustedNumPayloadElements_Sample(inst);
    return numSources;
}

/// \brief Determines whether sample instruction has LOD of zero
bool PayloadMapping::IsZeroLOD(const SampleIntrinsic* inst)
{
    const CPlatform& platform = m_CodeGenContext->platform;

    if (platform.supportSampleAndLd_lz())
    {
        if (const SampleIntrinsic * sampleInst = dyn_cast<SampleIntrinsic>(inst))
        {
            return sampleInst->ZeroLOD();
        }
    }
    return false;
}

///
uint PayloadMapping::GetNumPayloadElements_URBWrite(const GenIntrinsicInst* inst)
{

    //FIXME: this was copied from EmitPass::emitURBWrite.
    //find a way to unify this, so not to cause sudden troubles if it is
    //modified there

    uint size = 0;
    if (llvm::isa<llvm::ConstantInt>(inst->getOperand(1)))
    {
        uint mask = (uint)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
        size = iSTD::bsr(mask) + 1;
    }
    else
    {
        // All 4 elements will be send - we don't know which are masked out. 
        size = 4;
    }

    return size;
}

/// \brief Returns the number of homogeneous slots in payload.
///
/// RT write complete format: s0Alpha oM [R G B A] sZ oS
/// Only [R G B A] part forms the homogeneous sequence. (Though one can
/// also include s0Alpha in the homogeneous part if oM is not present, but that
/// seemed not to give good results in terms of coalescing efficiency).
uint PayloadMapping::GetNumPayloadElements_RTWrite(const GenIntrinsicInst* inst)
{
    const int numElements = 4; //4 colors, always form homogeneous 'part'.
    return numElements;
}

///
Value* PayloadMapping::GetPayloadElementToValueMapping_URBWrite(const GenIntrinsicInst* inst, uint index)
{
    //First 3 operands are immediates with specific meaning (not part of payload)
    return inst->getOperand(index + 2);
}

/// \brief Gets non-adjusted mapping from the payload element index to intrinsic value(argument) index.
///
/// Peeling is not applied here.
uint PayloadMapping::GetNonAdjustedPayloadElementIndexToValueIndexMapping_sample(
    const SampleIntrinsic* inst, uint index)
{
    const bool zeroLOD = IsZeroLOD(inst);
    uint startIndex = zeroLOD ? 1 : 0;

    GenISAIntrinsic::ID IID = inst->getIntrinsicID();
    //Here we want to get 'C', but to skip 'L'.
    // C L ...
    if (!(zeroLOD &&
        index == 0 &&
        IID == GenISAIntrinsic::GenISA_sampleLCptr))
    {
        index = index + startIndex;
    }

    return index;
}

/// \brief Gets payload element index to value mapping, adjusted with splitting decision(peeling).
Value* PayloadMapping::GetPayloadElementToValueMapping_sample(const SampleIntrinsic* inst, const uint index)
{
    uint valueIndex = GetNonAdjustedPayloadElementIndexToValueIndexMapping_sample(inst, index);
    return inst->getOperand(valueIndex);
}

/// \brief Gets payload element index to value mapping for RT writes.
Value* PayloadMapping::GetPayloadElementToValueMapping_RTWrite(const GenIntrinsicInst* inst, const uint index)
{
    //s0Alpha oM[R G B A] sZ oS
    assert(index < GetNumPayloadElements(inst));
    assert(llvm::isa<llvm::RTWritIntrinsic>(inst));
    const RTWritIntrinsic* rtwi = cast<RTWritIntrinsic>(inst);

    if (index < 4)
    {
        switch (index)
        {
        case 0: return rtwi->getRed();
        case 1: return rtwi->getGreen();
        case 2: return rtwi->getBlue();
        case 3: return rtwi->getAlpha();
        }
    }

    return nullptr;
}

Value* PayloadMapping::GetPayloadElementToValueMapping_LDMS(const SamplerLoadIntrinsic* inst, const uint index)
{
    uint valueIndex = index;

    return inst->getOperand(valueIndex);
}


Value* PayloadMapping::GetPayloadElementToValueMapping(const Instruction* inst, uint index)
{
    assert(index < GetNumPayloadElements(inst));

    const llvm::GenIntrinsicInst* intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
    assert(intrinsicInst);

    std::pair<const llvm::Instruction*, uint> instIndexPair(inst, index);

    PayloadMappingCache::iterator cachedValue = m_PayloadMappingCache.find(instIndexPair);
    if (cachedValue != m_PayloadMappingCache.end())
    {
        return cachedValue->second;
    }

    Value* payloadValue = nullptr;

    if (const SampleIntrinsic * sampleInst = dyn_cast<SampleIntrinsic>(inst))
    {
        payloadValue = GetPayloadElementToValueMapping_sample(sampleInst, index);
        assert(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }
    else if (const SamplerLoadIntrinsic * sampleInst = dyn_cast<SamplerLoadIntrinsic>(inst))
    {
        payloadValue = GetPayloadElementToValueMapping_LDMS(sampleInst, index);
        assert(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }


    switch (intrinsicInst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_URBWrite:
        payloadValue = GetPayloadElementToValueMapping_URBWrite(intrinsicInst, index);
        assert(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    case GenISAIntrinsic::GenISA_RTWrite:
        payloadValue = GetPayloadElementToValueMapping_RTWrite(intrinsicInst, index);
        assert(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    default:
        break;
    }

    assert(0);
    return NULL;
}

/// \brief Determines whether the payload is being split by peeling the first element.
bool PayloadMapping::DoPeelFirstElement(const Instruction* inst)
{
    if (dyn_cast<SamplerLoadIntrinsic>(inst))
    {
        return true;
    }

    return false;
}
