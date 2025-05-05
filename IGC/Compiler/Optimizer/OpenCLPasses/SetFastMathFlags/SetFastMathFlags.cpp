/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/SetFastMathFlags/SetFastMathFlags.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-set-fast-math-flags"
#define PASS_DESCRIPTION "Set llvm fast math flags according to compiler options"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SetFastMathFlags, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SetFastMathFlags, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char SetFastMathFlags::ID = 0;

SetFastMathFlags::SetFastMathFlags() : ModulePass(ID)
{
    m_Mask.setFast();
    initializeSetFastMathFlagsPass(*PassRegistry::getPassRegistry());
}

SetFastMathFlags::SetFastMathFlags(FastMathFlags Mask) : ModulePass(ID)
{
    m_Mask = Mask;
    initializeSetFastMathFlagsPass(*PassRegistry::getPassRegistry());
}

SetFastMathFlags::SetFastMathFlags(FastMathFlags Mask, bool skipUnsafeFpMathAttr) : ModulePass(ID)
{
    m_Mask = Mask;
    m_skipUnsafeFpMathAttr = skipUnsafeFpMathAttr;
    initializeSetFastMathFlagsPass(*PassRegistry::getPassRegistry());
}

bool SetFastMathFlags::runOnModule(Module& M)
{
    auto hasFnAttributeSet = [](Function& F, StringRef Attr)
    {
        return F.hasFnAttribute(Attr) && F.getFnAttribute(Attr).getValueAsString() == "true";
    };

    const ModuleMetaData& modMD = *(getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
    bool changed = false;
    for (Function& F : M) {
        FastMathFlags fmfs;
        // We determine if we have to use the unsafe-fp-math attr or we can omit it
        bool unsafeFpMathAttrCheck = m_skipUnsafeFpMathAttr || hasFnAttributeSet(F, "unsafe-fp-math");
        // Fast relaxed math implies all other flags.
        if (modMD.compOpt.FastRelaxedMath && unsafeFpMathAttrCheck) {
            fmfs.setFast();
            fmfs &= m_Mask;
            changed |= setFlags(F, fmfs);
            continue;
        }
        // Unsafe math implies no signed zeros.
        if ((modMD.compOpt.NoSignedZeros || modMD.compOpt.UnsafeMathOptimizations) && (hasFnAttributeSet(F, "no-signed-zeros-fp-math"))) {
            fmfs.setNoSignedZeros();
        }
        // Finite math implies no infs and nans.
        if (modMD.compOpt.FiniteMathOnly) {
            if (hasFnAttributeSet(F, "no-infs-fp-math")) {
                fmfs.setNoInfs();
            }
            if (hasFnAttributeSet(F, "no-nans-fp-math")) {
                fmfs.setNoNaNs();
            }
        }
        fmfs &= m_Mask;
        changed |= setFlags(F, fmfs);
    }
    return changed;
}

static bool setMathFlags(IntrinsicInst* II) {
    switch (II->getIntrinsicID()) {
    case Intrinsic::pow:
    case Intrinsic::exp2:
    case Intrinsic::log:
    case Intrinsic::sqrt:
        II->setFast(true);
        return true;
    default:
        break;
    }
    return false;
}

bool SetFastMathFlags::setFlags(Function& F, FastMathFlags fmfs)
{
    if (!fmfs.any())
    {
        return false;
    }

    StringRef fName = F.getName();
    if (fName.equals("__ocl_svml_cos") ||
        fName.equals("__ocl_svml_sin"))
    {
        return false;
    }

    bool changed = false;
    for (inst_iterator i = inst_begin(&F), e = inst_end(&F); i != e; ++i)
    {
        unsigned int op = i->getOpcode();
        switch (op) {
        case Instruction::FNeg:
        case Instruction::FAdd:
        case Instruction::FSub:
        case Instruction::FMul:
        case Instruction::FDiv:
        case Instruction::FRem:
            i->setFastMathFlags(fmfs); // this actually does an OR between flags.
            changed = true;
            break;
        case Instruction::Call:
            if (auto II = dyn_cast<IntrinsicInst>(&*i)) {
                if (fmfs.isFast())
                    changed |= setMathFlags(II);
            }
            else if (isa<FPMathOperator>(&*i)) {
                i->setFastMathFlags(fmfs);
                changed = true;
            }
            break;
        default:
            break;
        }
    }
    return changed;
}
