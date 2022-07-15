/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Upon a DispatchRays() call from the application, a raygen shader will be
/// invoked to kick off execution.  The raygen shader is executed with a
/// COMPUTE_WALKER.  The contract with the UMD is that the global and local
/// pointers will be provided via inline data as such
/// (see 'RayDispatchInlinedData'):
///
/// +------+------+------+------+
/// | N/A  | N/A  |Global| Desc | r2
/// +------+------+------+------+
///   QW3    QW2    QW1     QW0
///
/// We get the ray dispatch descriptor address (Desc) rather than the local
/// pointer itself.  To get the local pointer, just offset by the shader
/// identifier size.
///
/// All other raytracing shaders spawned via bindless thread dispatch (BTD)
/// will have the r2 part of their payload setup as:
///
/// +------+------+------+------+
/// | N/A  | N/A  |Local |Global| r2
/// +------+------+------+------+
///   QW3    QW2    QW1     QW0
///
/// We have this pass so that we can think in terms of global and local pointers
/// throughout the raytracing pipeline without having to consider the special
/// case of raygen until now.  That's why we execute this pass at the end.

//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"

#include <vector>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvmWrapper/IR/InstrTypes.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

class BindlessInlineDataPass : public ModulePass
{
public:
    BindlessInlineDataPass(): ModulePass(ID)
    {
        initializeBindlessInlineDataPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "BindlessInlineDataPass";
    }

    static char ID;
};

char BindlessInlineDataPass::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "bindless-inline-data-pass"
#define PASS_DESCRIPTION2 "Use inline data in raygen shaders"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(BindlessInlineDataPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(BindlessInlineDataPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

bool BindlessInlineDataPass::runOnModule(Module &M)
{
    auto* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData* modMD = pCtx->getModuleMetaData();

    std::vector<Instruction*> replaced;

    static_assert(sizeof(RayDispatchInlinedData) == 2 * sizeof(uint64_t),
        "inlined data change?");

    RTBuilder builder(M.getContext(), *pCtx);

    //Need to loop to replace GlobalBufferPtr/LocalBufferPtr with
    //InlinedData for raydispatch shaders
    for (auto &curFunc : M)
    {
        if (curFunc.isDeclaration())
            continue;

        auto functionMD = modMD->FuncMD.find(&curFunc);
        if (functionMD == modMD->FuncMD.end())
        {
            IGC_ASSERT_MESSAGE(0, "missing metadata?");
            continue;
        }

        if (functionMD->second.functionType != FunctionTypeMD::KernelFunction)
            continue;

        static_assert(sizeof(RayDispatchInlinedData) == 16,
            "You may need to change the below code!");

        for (auto& intrin : instructions(curFunc))
        {
            if (isa<GlobalBufferPointerIntrinsic>(&intrin))
            {
                builder.SetInsertPoint(&intrin);
                // The argument to the intrinsic is the number of QWORDs
                // to offset from r2.0 to find the data.
                constexpr uint32_t GlobalPtrOffset =
                    offsetof(RayDispatchInlinedData, RayDispatchGlobalDataPtr) / sizeof(uint64_t);

                CallInst* inlinedDataPtr = builder.getInlineData(
                    intrin.getType(),
                    GlobalPtrOffset,
                    RTGlobalsAlign
                );
                // Wrap the returned pointer of the inlinedData intrinsic from the input, in a bitcast.
                auto* inlinedDataBitCast = builder.CreateBitCast(inlinedDataPtr, intrin.getType());

                intrin.replaceAllUsesWith(inlinedDataBitCast);
                replaced.push_back(&intrin);
            }
            else if (auto* local_buffer_ptr = dyn_cast<LocalBufferPointerIntrinsic>(&intrin))
            {
                // For the shader dumps, we are having the code to generate
                // the pointer to the raygen shader record and
                // offset by the shader identifier size which gives us the
                // pointer to the local args.

                uint32_t AddrSpace = intrin.getType()->getPointerAddressSpace();
                builder.SetInsertPoint(&intrin);
                constexpr uint32_t RayDispatchAddrOffset =
                    offsetof(RayDispatchInlinedData, RayDispatchDescriptorAddress) / sizeof(uint64_t);

                // Since the InlinedData intrinsic returns a LLVM pointer to the start of the entire Shader Recrod,
                // The size of the dereferenceable data =
                // the size of the Shader Identifier + the size of the Local arguments
                const uint64_t dereferenceable_size =
                    sizeof(ShaderIdentifier) +
                    IGCLLVM::getRetDereferenceableBytes(local_buffer_ptr);

                // Return a CallInst* representing the InlinedData intrinsic call
                // In the shader dumps,
                // InlinedData intrinsic call returns a pointer to start of the ShaderRecord for the raygen shader.
                Type* RetTy = builder.getInt8PtrTy(AddrSpace);
                CallInst* basePtr = builder.getInlineData(
                    RetTy,
                    RayDispatchAddrOffset,
                    // There is only one raygen shader so its shader record
                    // alignment must be the same as the shader table alignment.
                    RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
                    static_cast<uint32_t>(dereferenceable_size)
                );

                // Create a LLVM BitCast around the return of the call into a pointer to the local args data type.
                auto* basePtrAddr = builder.CreateBitCast(basePtr, RetTy);
                // Create a getelementptr instruction over the return of the InlinedData intrinsic, offsets the
                // sizeof(ShaderIdentifier) the 32 bytes, to get the start of the Local arguments in the Shader Record.
                // The pointer returned by the InlinedData intrinsic is treated as an operand to the getelementptr instruction
                Value* Ptr = builder.CreateGEP(
                    basePtrAddr,
                    builder.getInt32(sizeof(ShaderIdentifier)),
                    VALUE_NAME("&localArgs")
                );

                // Replace all occurences of the LocalBufferPointerIntrinsic in the input
                // with the pointer to the Local arguments within the Shader Record.
                intrin.replaceAllUsesWith(Ptr);
                replaced.push_back(&intrin);
            }
        }
    }

    for (auto it : replaced)
        it->eraseFromParent();

    return !replaced.empty();
}

namespace IGC
{

Pass* CreateBindlessInlineDataPass(void)
{
    return new BindlessInlineDataPass();
}

} // namespace IGC
