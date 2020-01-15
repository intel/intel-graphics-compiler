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

#include "Compiler/Optimizer/OpenCLPasses/AddressSpaceAliasAnalysis/AddressSpaceAliasAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

namespace {

    class AddressSpaceAAResult : public AAResultBase<AddressSpaceAAResult> {
        friend AAResultBase<AddressSpaceAAResult>;
        const TargetLibraryInfo& TLI;
        const CodeGenContext& CGC;
    public:
        explicit AddressSpaceAAResult(
            const TargetLibraryInfo& TLI,
            const CodeGenContext& ctx)
            : AAResultBase(), TLI(TLI), CGC(ctx) {}
        AddressSpaceAAResult(AddressSpaceAAResult&& Arg)
            : AAResultBase(std::move(Arg)), TLI(Arg.TLI), CGC(Arg.CGC) {}

        AliasResult alias(const MemoryLocation& LocA, const MemoryLocation& LocB
#if LLVM_VERSION_MAJOR >= 9
            , AAQueryInfo & AAQI
#endif
        ) {
            // DO NOT strip any casting as the address space is encoded in pointer
            // type. For `addrspacecast`, the current implementation in LLVM is too
            // aggressive to strip them. With address space resolution, we should not
            // have trivial cases where a non-generic pointer is cased into a generic
            // one.

            PointerType* PtrTy1 = dyn_cast<PointerType>(LocA.Ptr->getType());
            PointerType* PtrTy2 = dyn_cast<PointerType>(LocB.Ptr->getType());

            if (!PtrTy1 || !PtrTy2)
                return NoAlias;

            unsigned AS1 = PtrTy1->getAddressSpace();
            unsigned AS2 = PtrTy2->getAddressSpace();

            // If LocA and LocB are pointers to different non-generic address spaces
            // then they do not alias.  This is not true in general, however, in OpenCL
            // all address spaces except generic are disjoint.
            //
            // Address spaces greater than ADDRESS_SPACE_NUM_ADDRESSES are used for
            // statefull accesses and may alias.
            //
            if (AS1 < ADDRESS_SPACE_NUM_ADDRESSES &&
                AS2 < ADDRESS_SPACE_NUM_ADDRESSES &&
                AS1 != ADDRESS_SPACE_GENERIC &&
                AS2 != ADDRESS_SPACE_GENERIC &&
                AS1 != AS2)
                return NoAlias;


            // Shared local memory doesn't alias any statefull memory.
            if ((AS1 == ADDRESS_SPACE_LOCAL && AS2 > ADDRESS_SPACE_NUM_ADDRESSES) ||
                (AS1 > ADDRESS_SPACE_NUM_ADDRESSES && AS2 == ADDRESS_SPACE_LOCAL))
            {
                return NoAlias;
            }

            // Private memory doesn't alias any stateful memory
            if ((AS1 == ADDRESS_SPACE_PRIVATE && AS2 > ADDRESS_SPACE_NUM_ADDRESSES) ||
                (AS1 > ADDRESS_SPACE_NUM_ADDRESSES && AS2 == ADDRESS_SPACE_PRIVATE))
            {
                return NoAlias;
            }


            /// For some client APIs (e.g. vulkan) compiler is free to assume that
            /// resources bound to two different bindings points never alias unless a
            /// resource is explicitly marked as being aliased.
            assert(CGC.getModuleMetaData());
            if (AS1 > ADDRESS_SPACE_NUM_ADDRESSES &&
                AS2 > ADDRESS_SPACE_NUM_ADDRESSES &&
                CGC.getModuleMetaData()->statefullResourcesNotAliased)
            {
                bool isDirectAccess[2] = { false, false };
                unsigned resourceIndex[2] = { 0, 0 };
                BufferType resourceType[2] = { BUFFER_TYPE_UNKNOWN, BUFFER_TYPE_UNKNOWN };

                resourceType[0] = DecodeAS4GFXResource(AS1, isDirectAccess[0], resourceIndex[0]);
                resourceType[1] = DecodeAS4GFXResource(AS2, isDirectAccess[1], resourceIndex[1]);

                /// Returns true if resource type is a texture, constant buffer or a storage buffer.
                auto IsBufferOrTexture = [](
                    BufferType type)->bool
                {
                    return (type == BufferType::CONSTANT_BUFFER || type == BufferType::UAV || type == BufferType::RESOURCE);
                };

                if (IsBufferOrTexture(resourceType[0]) &&
                    IsBufferOrTexture(resourceType[1]))
                {
                    if ((resourceType[0] != resourceType[1]) || // different resource types
                        (isDirectAccess[0] && isDirectAccess[1] && resourceIndex[0] != resourceIndex[1])) // direct access to different BTIs
                    {
                        return NoAlias;
                    }
                }
            }


            return AAResultBase::alias(LocA, LocB
#if LLVM_VERSION_MAJOR >= 9
                , AAQI
#endif
            );
        }

        bool pointsToConstantMemory(const llvm::MemoryLocation& Loc,
#if LLVM_VERSION_MAJOR >= 9
            AAQueryInfo & AAQI,
#endif
            bool OrLocal) {
            // Pointers to constant address space memory, well, point to constant memory
            PointerType* ptrType = dyn_cast<PointerType>(Loc.Ptr->getType());
            if (ptrType && ptrType->getAddressSpace() == ADDRESS_SPACE_CONSTANT)
                return true;

            bool DirectIdx;
            unsigned BufId;
            BufferType BufTy = DecodeAS4GFXResource(ptrType->getAddressSpace(),
                DirectIdx, BufId);
            if (BufTy == CONSTANT_BUFFER)
                return true;

            return AAResultBase::pointsToConstantMemory(Loc,
#if LLVM_VERSION_MAJOR >= 9
                AAQI,
#endif
                OrLocal);
        }
    };

    class AddressSpaceAAWrapperPass : public ImmutablePass {
        std::unique_ptr<AddressSpaceAAResult> Result;

    public:
        static char ID;

        AddressSpaceAAWrapperPass();

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesAll();
            AU.addRequired<TargetLibraryInfoWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        bool doInitialization(Module& M) override {
            Result.reset(new AddressSpaceAAResult(
            getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(
#if LLVM_VERSION_MAJOR >= 10
                // LLVM_UPGRADE_TODO - check how to feed this place with correct Function ref
                *M.begin()
#endif
                ),
                *getAnalysis<CodeGenContextWrapper>().getCodeGenContext()));
            return false;
        }

        bool doFinalization(Module& M) override {
            Result.reset();
            return false;
        }

        AddressSpaceAAResult& getResult() { return *Result; }
        const AddressSpaceAAResult& getResult() const { return *Result; }
    };

} // End anonymous namespace

#define PASS_FLAG     "igc-address-space-alias-analysis"
#define PASS_DESC     "Address space alias analysis"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(AddressSpaceAAWrapperPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(AddressSpaceAAWrapperPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char AddressSpaceAAWrapperPass::ID = 0;

AddressSpaceAAWrapperPass::AddressSpaceAAWrapperPass() : ImmutablePass(ID) {
    initializeAddressSpaceAAWrapperPassPass(*PassRegistry::getPassRegistry());
}

ImmutablePass* IGC::createAddressSpaceAAWrapperPass() {
    return new AddressSpaceAAWrapperPass();
}

void IGC::addAddressSpaceAAResult(Pass& P, Function&, AAResults& AAR) {
    if (auto * WrapperPass = P.getAnalysisIfAvailable<AddressSpaceAAWrapperPass>())
        AAR.addAAResult(WrapperPass->getResult());
}
