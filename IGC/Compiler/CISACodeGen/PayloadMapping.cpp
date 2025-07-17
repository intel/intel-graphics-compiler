/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

///
const Instruction* PayloadMapping::GetSupremumOfNonHomogeneousPart(
    const Instruction* inst1,
    const Instruction* inst2)
{
    const RTWriteIntrinsic* rtwi1 = cast_or_null<RTWriteIntrinsic>(inst1);
    const RTWriteIntrinsic* rtwi2 = cast_or_null<RTWriteIntrinsic>(inst2);

    if (rtwi1 && rtwi2)
    {
        return GetSupremumOfNonHomogeneousPart_RTWrite(rtwi1, rtwi2);
    }

    IGC_ASSERT(0);
    return nullptr;
}


/// Helper method.
/// compare rtwi1 and rtwi2 w.r.t. oMask
/// biased towards rtwi1:
/// X X -> return rtwi1
/// X 2 -> return rtwi2
/// 1 X -> return rtwi1
/// 1 2 -> return rtwi1
static const Instruction* CompareOnMask(
    const RTWriteIntrinsic* rtwi1,
    const RTWriteIntrinsic* rtwi2)
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
    const RTWriteIntrinsic* rtwi1,
    const RTWriteIntrinsic* rtwi2)
{
    ModuleMetaData* modMD = m_CodeGenContext->getModuleMetaData();

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
    IGC_ASSERT(intrinsicInst);

    if (llvm::isa<llvm::RTWriteIntrinsic>(intrinsicInst))
    {
        return GetLeftReservedOffset_RTWrite(cast<RTWriteIntrinsic>(inst), simdMode);
    }
    else if (llvm::isa<llvm::RTDualBlendSourceIntrinsic>(intrinsicInst))
    {
        return GetLeftReservedOffset_RTWrite(cast<RTDualBlendSourceIntrinsic>(inst), simdMode);
    }

    return 0;
}

///
template <typename T>
int PayloadMapping::GetLeftReservedOffset_RTWrite(const T* inst, SIMDMode simdMode)
{
    int offset = 0;

    if (inst->hasMask())
    {
        //Output mask is always fixed size regardless of SIMD mode.
        offset += m_CodeGenContext->platform.getGRFSize();
    }

    if (RTWriteHasSource0Alpha(inst, m_CodeGenContext->getModuleMetaData()))
    {
        if (m_CodeGenContext->platform.getMinDispatchMode() == SIMDMode::SIMD16)
        {
            int multiplier = simdMode == SIMDMode::SIMD16 ? 1 : 2;
            offset += m_CodeGenContext->platform.getGRFSize() * multiplier;
            return offset;
        }

        IGC_ASSERT(simdMode == SIMDMode::SIMD8 || simdMode == SIMDMode::SIMD16);
        int multiplier = inst->getSource0Alpha()->getType()->isHalfTy() ? 1 : 2;
        if (simdMode == SIMDMode::SIMD8)
        {
            offset += m_CodeGenContext->platform.getGRFSize(); //always 1GRF, regardless of HF
        }
        else if (simdMode == SIMDMode::SIMD16)
        {
            offset += m_CodeGenContext->platform.getGRFSize() * multiplier; // 2GRFs normal precision, 1GRF HF precision
        }
    }

    return offset;
}

///
int PayloadMapping::GetRightReservedOffset(const Instruction* inst, SIMDMode simdMode)
{
    const GenIntrinsicInst* intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
    IGC_ASSERT(intrinsicInst);

    if (llvm::isa<llvm::RTWriteIntrinsic>(intrinsicInst))
    {
        return GetRightReservedOffset_RTWrite(cast<RTWriteIntrinsic>(inst), simdMode);
    }
    else if (llvm::isa<llvm::RTDualBlendSourceIntrinsic>(intrinsicInst))
    {
        return GetRightReservedOffset_RTWrite(cast<RTDualBlendSourceIntrinsic>(inst), simdMode);
    }

    return 0;
}

template <typename T>
int PayloadMapping::GetRightReservedOffset_RTWrite(const T* inst, SIMDMode simdMode)
{
    int offset = 0;

    if (inst->hasStencil())
    {
        IGC_ASSERT(m_CodeGenContext->platform.supportsStencil(simdMode));
        offset += m_CodeGenContext->platform.getGRFSize();
    }

    if (inst->hasDepth())
    {
        if (m_CodeGenContext->platform.getMinDispatchMode() == SIMDMode::SIMD16)
        {
            int multiplier = simdMode == SIMDMode::SIMD16 ? 1 : 2;
            offset += m_CodeGenContext->platform.getGRFSize() * multiplier;
            return offset;
        }

        IGC_ASSERT(simdMode == SIMDMode::SIMD8 || simdMode == SIMDMode::SIMD16);

        //Depth always has normal precision, regardless of the rest of the fields.
        if (simdMode == SIMDMode::SIMD8)
        {
            offset += m_CodeGenContext->platform.getGRFSize(); //always 1GRF, 32 bytes
        }
        else if (simdMode == SIMDMode::SIMD16)
        {
            offset += m_CodeGenContext->platform.getGRFSize() * 2; // 2GRFs normal precision, 64 bytes
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
    IGC_ASSERT(intrinsicInst);

    if (llvm::isa<llvm::RTWriteIntrinsic>(intrinsicInst))
    {
        return HasNonHomogeneousPayloadElements_RTWrite(cast<RTWriteIntrinsic>(inst));
    }
    else if (llvm::isa<llvm::RTDualBlendSourceIntrinsic>(intrinsicInst))
    {
        return HasNonHomogeneousPayloadElements_RTWrite(cast<RTDualBlendSourceIntrinsic>(inst));
    }

    return false;
}

///
template <typename T>
bool PayloadMapping::HasNonHomogeneousPayloadElements_RTWrite(const T* inst)
{
    if (inst->hasMask())
    {
        return true;
    }
    if (RTWriteHasSource0Alpha(inst, m_CodeGenContext->getModuleMetaData()))
    {
        return true;
    }
    if (inst->hasDepth())
    {
        return true;
    }
    if (inst->hasStencil())
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
    IGC_ASSERT(intrinsicInst);
    if (const SampleIntrinsic * sampleInst = dyn_cast<SampleIntrinsic>(inst))
    {
        return GetNumPayloadElements_Sample(sampleInst);
    }
    else if (const SamplerLoadIntrinsic * sampleInst = dyn_cast<SamplerLoadIntrinsic>(inst))
    {
        return GetNumPayloadElements_LDMS(sampleInst);
    }

    if (llvm::isa<llvm::AtomicTypedIntrinsic>(intrinsicInst))
    {
        return GetNumPayloadElements_AtomicTyped(intrinsicInst);
    }

    if (isURBWriteIntrinsic(intrinsicInst))
    {
        return GetNumPayloadElements_URBWrite(intrinsicInst);
    }
    else if (llvm::isa<llvm::RTWriteIntrinsic>(intrinsicInst))
    {
        return GetNumPayloadElements_RTWrite(intrinsicInst);
    }
    else if (llvm::isa<llvm::RTDualBlendSourceIntrinsic>(intrinsicInst))
    {
        return GetNumPayloadElements_DSRTWrite(intrinsicInst);
    }

    IGC_ASSERT(0);
    return 0;
}

/// \brief
uint PayloadMapping::GetNumPayloadElements_LDMS(const GenIntrinsicInst* inst)
{
    const uint numOperands = inst->getNumOperands();

    //Subtract the offsets, and texture resource, lod to get
    //the number of texture coordinates and index to texture source
    uint numSources = numOperands - 6;

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
void PayloadMapping::ValidateNumberofSources(EOPCODE opCode, bool isCube, uint& numberofSrcs)
{
    switch (opCode)
    {
    case llvm_sampleptr:
    case llvm_sample_killpix:
    case llvm_lodptr:
    {
        if (m_CodeGenContext->platform.getWATable().Wa_14012688258 && isCube)
        {
            numberofSrcs = numberofSrcs >= 3 ? numberofSrcs : 3;
        }
    }
    break;

    case llvm_sample_bptr:
    case llvm_sample_cptr:
    case llvm_sample_lptr:
    {
        if (m_CodeGenContext->platform.getWATable().Wa_14012688258 && isCube)
        {
            numberofSrcs = numberofSrcs >= 4 ? numberofSrcs : 4;
        }

        switch (numberofSrcs)
        {
        case 1:
            numberofSrcs++;
            break;
        }
    }
    break;

    case llvm_sample_dptr:
    {
        if (m_CodeGenContext->platform.getWATable().Wa_14012688258 && isCube)
        {
            numberofSrcs = numberofSrcs >= 7 ? numberofSrcs : 7;
        }

        switch (numberofSrcs)
        {
        case 1:
            numberofSrcs++;
            [[fallthrough]];
        case 2:
            numberofSrcs++;
            break;
        case 5:
            numberofSrcs++;
            break;
        case 8:
            if (!m_CodeGenContext->platform.supports3DAndCubeSampleD())
            {
                break;
            }
            numberofSrcs++;
            break;
        }
    }
    break;
    case llvm_sample_dcptr:
    {
        if (m_CodeGenContext->platform.getWATable().Wa_14012688258 && isCube)
        {
            numberofSrcs = numberofSrcs >= 8 ? numberofSrcs : 8;
        }

        switch (numberofSrcs)
        {
        case 2:
            numberofSrcs++;
            [[fallthrough]];
        case 3:
            numberofSrcs++;
            break;
        case 5:
            numberofSrcs++;
            [[fallthrough]];
        case 6:
            numberofSrcs++;
            break;
        case 8:
            numberofSrcs++;
            [[fallthrough]];
        case 9:
            numberofSrcs++;
            break;
        }
    }
    break;

    case llvm_sample_lcptr:
    case llvm_sample_bcptr:
    {
        if (m_CodeGenContext->platform.getWATable().Wa_14012688258 && isCube)
        {
            numberofSrcs = numberofSrcs >= 5 ? numberofSrcs : 5;
        }

        switch (numberofSrcs)
        {
        case 1:
            numberofSrcs++;
            [[fallthrough]];
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
    unsigned int numSources = numOperands - 7;

    if (inst->IsLODInst())
    {
        //Subtract resource and sampler sources to get
        //the number of texture coordinates and index to texture source
        numSources = numOperands - 4;
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
    llvm::Type* cubeTextureType = GetResourceDimensionType(*inst->getModule(), RESOURCE_DIMENSION_TYPE::DIM_CUBE_TYPE);
    llvm::Type* cubeArrayTextureType = GetResourceDimensionType(*inst->getModule(), RESOURCE_DIMENSION_TYPE::DIM_CUBE_ARRAY_TYPE);
    llvm::Type* textureType = inst->getTexturePtrEltTy();
    bool isCube = (textureType == cubeTextureType || textureType == cubeArrayTextureType);
    ValidateNumberofSources(opCode, isCube, numSources);

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

    if (platform.supportSampleAndLd_lz() && !platform.WaDisableSampleLz())
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

    auto GetSize = [](auto inst) -> uint
    {
        uint size = 0u;
        llvm::Value* maskVal = inst->getMask();
        if (llvm::isa<llvm::ConstantInt>(maskVal))
        {
            uint mask = static_cast<uint>(llvm::cast<llvm::ConstantInt>(maskVal)->getZExtValue());
            size = iSTD::bsr(mask) + 1u;
        }
        else
        {
            // All defined elements will be send - we don't know which are masked out.
            constexpr uint numVals = std::remove_pointer_t<decltype(inst)>::scNumVals;
            for (uint i = 0; i < numVals; i++)
            {
                if (llvm::isa<llvm::UndefValue>(inst->getValue(i)))
                {
                    continue;
                }
                size = i + 1u;
            }
        }
        return size;
    };

    uint size = llvm::isa<UrbWriteIntrinsic>(inst) ? GetSize(llvm::cast<UrbWriteIntrinsic>(inst)) :
        0u;

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

uint PayloadMapping::GetNumPayloadElements_AtomicTyped(const GenIntrinsicInst* inst)
{
    const int numElements = 3; //xyz coordinate.
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

/// \brief Gets payload element index to value mapping, adjusted with splitting decision(peeling).
Value* PayloadMapping::GetPayloadElementToValueMapping_AtomicTyped(const AtomicTypedIntrinsic* inst, const uint index)
{
    return inst->getOperand(index+1);
}

/// \brief Gets payload element index to value mapping for RT writes.
Value* PayloadMapping::GetPayloadElementToValueMapping_RTWrite(const GenIntrinsicInst* inst, const uint index)
{
    //s0Alpha oM[R G B A] sZ oS
    IGC_ASSERT(index < GetNumPayloadElements(inst));
    const RTWriteIntrinsic* rtwi = cast_or_null<RTWriteIntrinsic>(inst);
    IGC_ASSERT(rtwi);
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
    IGC_ASSERT(index < GetNumPayloadElements(inst));

    const llvm::GenIntrinsicInst* intrinsicInst = dyn_cast<GenIntrinsicInst>(inst);
    IGC_ASSERT(intrinsicInst);

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
        IGC_ASSERT(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }
    else if (const SamplerLoadIntrinsic * sampleInst = dyn_cast<SamplerLoadIntrinsic>(inst))
    {
        payloadValue = GetPayloadElementToValueMapping_LDMS(sampleInst, index);
        IGC_ASSERT(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }

    if (const AtomicTypedIntrinsic* atomicInst = dyn_cast<AtomicTypedIntrinsic>(inst))
    {
        payloadValue = GetPayloadElementToValueMapping_AtomicTyped(atomicInst, index);
        IGC_ASSERT(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }

    if (isURBWriteIntrinsic(intrinsicInst))
    {
        payloadValue = GetPayloadElementToValueMapping_URBWrite(intrinsicInst, index);
        IGC_ASSERT(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }
    else if (llvm::isa<llvm::RTWriteIntrinsic>(intrinsicInst))
    {
        payloadValue = GetPayloadElementToValueMapping_RTWrite(intrinsicInst, index);
        IGC_ASSERT(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }
    else if (llvm::isa<llvm::RTDualBlendSourceIntrinsic>(intrinsicInst))
    {
        payloadValue = GetPayloadElementToValueMapping_DSRTWrite(intrinsicInst, index);
        IGC_ASSERT(payloadValue != nullptr);
        m_PayloadMappingCache.insert(std::pair<std::pair<const llvm::Instruction*, uint>, Value*>(instIndexPair, payloadValue));
        return payloadValue;
    }

    IGC_ASSERT(0);
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

/// \brief Returns the number of homogeneous slots in dual-source payload.
///
/// dual-source RT write complete format: oM [R0 G0 B0 A0 R1 G1 B1 A1] sZ oS
/// Only [R0 G0 B0 A0 R1 G1 B1 A1] part forms the homogeneous sequence.
uint PayloadMapping::GetNumPayloadElements_DSRTWrite(const GenIntrinsicInst* inst)
{
    IGC_ASSERT(llvm::isa<llvm::RTDualBlendSourceIntrinsic>(inst));
    return 8; //8 colors, always form homogeneous 'part'.
}

/// \brief Gets payload element index to value mapping for dual-source RT writes.
Value* PayloadMapping::GetPayloadElementToValueMapping_DSRTWrite(const GenIntrinsicInst* inst, const uint index)
{
    //oM [R0 G0 B0 A0 R1 G1 B1 A1] sZ oS
    IGC_ASSERT(index < GetNumPayloadElements(inst));
    const RTDualBlendSourceIntrinsic* dsrtwi = cast_or_null<RTDualBlendSourceIntrinsic>(inst);
    IGC_ASSERT(dsrtwi);

    if (index < 8)
    {
        switch (index)
        {
        case 0: return dsrtwi->getRed0();
        case 1: return dsrtwi->getGreen0();
        case 2: return dsrtwi->getBlue0();
        case 3: return dsrtwi->getAlpha0();
        case 4: return dsrtwi->getRed1();
        case 5: return dsrtwi->getGreen1();
        case 6: return dsrtwi->getBlue1();
        case 7: return dsrtwi->getAlpha1();
        }
    }

    return nullptr;
}
