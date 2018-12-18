#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

class UniformAtomic : public FunctionPass, public InstVisitor<UniformAtomic> 
{
public:
  static char ID;

  UniformAtomic() : FunctionPass(ID) {
    initializeUniformAtomicPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &) override;
  void visitCallInst(llvm::CallInst &C);
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreservedID(WIAnalysis::ID);
    AU.addRequired<WIAnalysis>();
    AU.addRequired<CodeGenContextWrapper>();
  }
private:
    struct AtomicInfo
    {
        Value* resourcePtr;
        Value* offset;
        Value* src;
        Value * atomicOp;
        WaveOps op;
    };
    void GetScalarAtomicInfo(CallInst* atomic, AtomicInfo& info);
    void Get(CallInst* pInst);
    bool IsUniformAtomic(CallInst* pInst);
    bool m_changed = false;
    WIAnalysis* m_WIAnalysis = nullptr;
};

} // End anonymous namespace

char UniformAtomic::ID = 0;

#define PASS_FLAG     "igc-uniform-atomic"
#define PASS_DESC     "optimized atomic to uniform address"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(UniformAtomic, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(UniformAtomic, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{
    FunctionPass *createUniformAtomicPass() {
        return new UniformAtomic();
    }
}

bool UniformAtomic::runOnFunction(Function &F) 
{
    if(IGC_IS_FLAG_ENABLED(DisableScalarAtomics) ||
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->m_DriverInfo.WASLMPointersDwordUnit())
    {
        return false;
    }
    m_WIAnalysis = &getAnalysis<WIAnalysis>();
    visit(F);
    return m_changed;
}

bool UniformAtomic::IsUniformAtomic(llvm::CallInst* pInst)
{
    if(llvm::GenIntrinsicInst* pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(pInst))
    {
        GenISAIntrinsic::ID id = pIntrinsic->getIntrinsicID();
        // Dst address in bytes.
        if(id == GenISAIntrinsic::GenISA_intatomicraw ||
            id == GenISAIntrinsic::GenISA_intatomicrawA64)
        {
            // TODO: add support for 64bits type
            if(pInst->getType()->getScalarSizeInBits() == 64)
            {
                return false;
            }
            llvm::Value* pllDstAddr = pInst->getOperand(1);
            bool uniformAddress = m_WIAnalysis->whichDepend(pllDstAddr) == WIAnalysis::UNIFORM;
            if(uniformAddress)
            {
                AtomicOp atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInst->getOperand(3))->getZExtValue());

                bool isAddAtomic = atomic_op == EATOMIC_IADD ||
                    atomic_op == EATOMIC_INC ||
                    atomic_op == EATOMIC_SUB;
                bool isMinMaxAtomic =
                    atomic_op == EATOMIC_UMAX ||
                    atomic_op == EATOMIC_UMIN ||
                    atomic_op == EATOMIC_IMIN ||
                    atomic_op == EATOMIC_IMAX;

                if(isAddAtomic || (isMinMaxAtomic && pInst->use_empty()))
                    return true;
            }
        }
    }
    return false;
}

void UniformAtomic::GetScalarAtomicInfo(CallInst* pInst, AtomicInfo& info)
{
    IRBuilder<> builder(pInst);
    GenISAIntrinsic::ID id = cast<GenIntrinsicInst>(pInst)->getIntrinsicID();
    info.resourcePtr = pInst->getOperand(0);
    info.offset = pInst->getOperand(1);
    info.src = pInst->getOperand(2);
    info.atomicOp = pInst->getOperand(3);
    if(id != GenISAIntrinsic::GenISA_intatomicraw)
    {
        info.offset = UndefValue::get(builder.getInt32Ty());
    }
    AtomicOp atomic_op = static_cast<AtomicOp>(cast<ConstantInt>(info.atomicOp)->getZExtValue());
    switch(atomic_op)
    {
    case EATOMIC_IADD:
    case EATOMIC_SUB:
    case EATOMIC_INC:
    case EATOMIC_DEC:
        info.op = WaveOps::SUM;
        info.atomicOp = builder.getInt32(EATOMIC_IADD);
        break;
    case EATOMIC_UMAX:
        info.op = WaveOps::UMAX;
        break;
    case EATOMIC_IMAX:
        info.op = WaveOps::IMAX;
        break;
    case EATOMIC_UMIN:
        info.op = WaveOps::UMIN;
        break;
    case EATOMIC_IMIN:
        info.op = WaveOps::IMIN;
        break;
    default:
        assert(0 && "unsupported scalar atomic type");
        break;
    }
    if(atomic_op == EATOMIC_DEC || atomic_op == EATOMIC_INC)
    {
        info.src = builder.getIntN(pInst->getType()->getScalarSizeInBits(), atomic_op == EATOMIC_DEC ? -1 : 1);
    }
    if(atomic_op == EATOMIC_SUB)
    {
        info.src = builder.CreateNeg(info.src);
    }
}

void UniformAtomic::visitCallInst(llvm::CallInst &C)
{
    Module* module = C.getParent()->getParent()->getParent();
    if(IsUniformAtomic(&C))
    {
        AtomicInfo info;
        GetScalarAtomicInfo(&C, info);
        Function* AtomicIntr = GenISAIntrinsic::getDeclaration(
            module,
            GenISAIntrinsic::GenISA_WaveUniformAtomic,
            { C.getType(), C.getOperand(0)->getType() });
        IRBuilder<> builder(&C);
        if(C.user_empty())
        {
            Function* intr =
                GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_WaveAll, C.getType());
            Value* reducedSrc = builder.CreateCall(
                intr, { info.src, builder.getInt8(static_cast<uint32_t>(info.op))});
            m_WIAnalysis->incUpdateDepend(reducedSrc, WIAnalysis::UNIFORM);
            Value* atomicInst = builder.CreateCall(
                AtomicIntr, { info.resourcePtr, info.offset, reducedSrc, info.atomicOp });
            m_WIAnalysis->incUpdateDepend(atomicInst, WIAnalysis::UNIFORM);
        }
        else
        {
            Function* intr =
                GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_WavePrefix, C.getType());
            Value* scanSrc = builder.CreateCall(
                intr, 
                { info.src, builder.getInt8(static_cast<uint32_t>(info.op)), builder.getInt1(true) });
            m_WIAnalysis->incUpdateDepend(scanSrc, WIAnalysis::RANDOM);
            Function* shuffleIntr =
                GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_WaveShuffleIndex, C.getType());
            Function* simdSizeIntr =
                GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_simdSize);
            Value* simdSize = builder.CreateCall(simdSizeIntr);
            m_WIAnalysis->incUpdateDepend(simdSize, WIAnalysis::UNIFORM);
            Value* lastChannel = builder.CreateSub(simdSize, builder.getInt32(1));
            m_WIAnalysis->incUpdateDepend(lastChannel, WIAnalysis::UNIFORM);
            Value* sumall = builder.CreateCall(shuffleIntr, { scanSrc, lastChannel });
            m_WIAnalysis->incUpdateDepend(sumall, WIAnalysis::UNIFORM);
            Value* atomicInst = builder.CreateCall(
                AtomicIntr, { info.resourcePtr, info.offset, sumall, info.atomicOp });
            m_WIAnalysis->incUpdateDepend(atomicInst, WIAnalysis::UNIFORM);
            Value* returnVal = builder.CreateAdd(atomicInst, scanSrc);
            m_WIAnalysis->incUpdateDepend(returnVal, WIAnalysis::RANDOM);
            returnVal = builder.CreateSub(returnVal, info.src);
            m_WIAnalysis->incUpdateDepend(returnVal, WIAnalysis::RANDOM);
            C.replaceAllUsesWith(returnVal);
        }
        C.eraseFromParent();
    }
}
