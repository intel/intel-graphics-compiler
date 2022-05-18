/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/Support/KnownBits.h>
#include <llvm/ADT/Optional.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/CodeGenPublic.h"
#include "Interval.h"

using namespace llvm;
using namespace Intervals;

namespace IGC {

class UnifiedBits
{
public:
    UnifiedBits(unsigned NumBits);
    // Merge new value into the what we already know about the value.
    UnifiedBits& operator+=(llvm::Value* V);
    llvm::Optional<bool> operator[](unsigned BitPosition) const;
    const llvm::Optional<llvm::APInt> getConstant() const;
private:
    llvm::KnownBits KB;
    bool Unset = true;
};

template <typename Fn>
void visitGenIntrinsic(llvm::Module& M, llvm::GenISAIntrinsic::ID ID, Fn Visit)
{
    using namespace llvm;

    for (auto& F : M)
    {
        if (!F.isIntrinsic() || !GenISAIntrinsic::isIntrinsic(&F))
            continue;

        if (GenISAIntrinsic::getIntrinsicID(&F) == ID)
        {
            for (auto* U : F.users())
            {
                IGC_ASSERT_MESSAGE(isa<GenIntrinsicInst>(U), "not a call?");
                if (auto* GII = dyn_cast<GenIntrinsicInst>(U))
                    Visit(GII);
            }
        }
    }
}

UnifiedBits examineRayFlags(const RayDispatchShaderContext& Ctx);

struct PayloadUse
{
    PayloadUse(Instruction* I, const DataLayout& DL, uint64_t Offset) : I(I)
    {
        Type* Ty = nullptr;
        if (auto* LI = dyn_cast<LoadInst>(I))
            Ty = LI->getType();
        else if (auto* SI = dyn_cast<StoreInst>(I))
            Ty = SI->getValueOperand()->getType();

        IGC_ASSERT_MESSAGE(Ty, "unhandled inst!");

        uint64_t Size = DL.getTypeAllocSize(Ty);
        MemInterval = { Offset, Offset + Size - 1 };
    }

    Instruction* I = nullptr;
    Interval MemInterval;
};

// Walks all the uses of the payload pointer and will return true if we know
// how to handle them. `Uses` will be populated with the loads and stores
// along with their region of memory they access as an offset from the base
// of the payload pointer.
// instTypeMask: 1: Load; 2: Store; 3: Load&Store
enum PL_Inst_Type : uint8_t
{
    Invalid = 0,
    Load = 1,
    Store = 2
};

bool collectAnalyzablePayloadUses(
    Value* I,
    const DataLayout& DL,
    SmallVector<PayloadUse, 4>& Uses,
    uint64_t Offset,
    uint32_t instTypeMask = (PL_Inst_Type::Load|PL_Inst_Type::Store));

unsigned gcd(unsigned Dividend, unsigned Divisor);

} // namespace IGC
