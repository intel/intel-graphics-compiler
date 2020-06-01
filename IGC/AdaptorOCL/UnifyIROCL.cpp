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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "llvm/ADT/PostOrderIterator.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Transforms/InstCombine/InstCombineWorklist.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>

#include <llvmWrapper/Transforms/Utils.h>

#include "common/LLVMWarningsPop.hpp"

#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "AdaptorCommon/ProcessFuncAttributes.h"
#include "common/LLVMUtils.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CISACodeGen/FixAddrSpaceCast.h"
#include "Compiler/CISACodeGen/ResolveGAS.h"
#include "Compiler/CISACodeGen/ResolvePredefinedConstant.h"
#include "Compiler/CISACodeGen/SimplifyConstant.h"
#include "Compiler/CISACodeGen/FoldKnownWorkGroupSizes.h"

#include "Compiler/HandleFRemInstructions.hpp"
#include "Compiler/Optimizer/BuiltInFuncImport.h"
#include "Compiler/Optimizer/CodeAssumption.hpp"
#include "Compiler/Optimizer/Scalarizer.h"
#include "Compiler/Optimizer/OpenCLPasses/DebuggerSupport/ImplicitGIDPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtenstionFuncs/ExtensionArgAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtenstionFuncs/ExtensionFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtenstionFuncs/ExtensionFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ImageFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ImageFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ResourceAllocator/ResourceAllocator.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LocalBuffers/InlineLocalsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ReplaceUnsupportedIntrinsics/ReplaceUnsupportedIntrinsics.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Atomics/ResolveOCLAtomics.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WGFuncs/WGFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AlignmentAnalysis/AlignmentAnalysis.hpp"
#include "Compiler/Optimizer/PreCompiledFuncImport.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AggregateArguments/AggregateArguments.hpp"
#include "Compiler/Optimizer/OpenCLPasses/UnreachableHandling/UnreachableHandling.hpp"
#include "Compiler/Optimizer/OCLBIConverter.h"
#include "Compiler/Optimizer/OpenCLPasses/SetFastMathFlags/SetFastMathFlags.hpp"
#include "Compiler/Optimizer/OpenCLPasses/CorrectlyRoundedDivSqrt/CorrectlyRoundedDivSqrt.hpp"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressDynamicResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AddressSpaceAliasAnalysis/AddressSpaceAliasAnalysis.h"
#include "Compiler/Optimizer/OpenCLPasses/DeviceEnqueueFuncs/DeviceEnqueue.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DeviceEnqueueFuncs/TransformBlocks.hpp"
#include "Compiler/Optimizer/OpenCLPasses/UndefinedReferences/UndefinedReferencesPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SubGroupFuncs/SubGroupFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BIFTransforms/BIFTransforms.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakdownIntrinsic.h"
#include "Compiler/Optimizer/OpenCLPasses/StatelessToStatefull/StatelessToStatefull.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelFunctionCloning.h"
#include "Compiler/Legalizer/TypeLegalizerPass.h"
#include "Compiler/Optimizer/OpenCLPasses/ClampLoopUnroll/ClampLoopUnroll.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Image3dToImage2darray/Image3dToImage2darray.hpp"
#include "Compiler/Optimizer/OpenCLPasses/RewriteLocalSize/RewriteLocalSize.hpp"
#include "Compiler/MetaDataApi/PurgeMetaDataUtils.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/SPIRMetaDataTranslation.h"
#include "Compiler/Optimizer/OpenCLPasses/ErrorCheckPass.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/FixResourcePtr.hpp"
#include "Compiler/InitializePasses.h"
#include "Compiler/MetaDataApi/SpirMetaDataApi.h"
#include "Compiler/Optimizer/FixFastMathFlags.hpp"
#include "MoveStaticAllocas.h"
#include "Compiler/Optimizer/IGCInstCombiner/IGCInstructionCombining.hpp"

#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/debug/Dump.hpp"
#include "common/MemStats.h"

#include <iStdLib/utility.h>

#include "Compiler/DebugInfo/VISADebugEmitter.hpp"
#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"

#include <string>
#include <algorithm>



using namespace llvm;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

namespace IGC
{
    int getOCLMajorVersion(const SPIRMD::SpirMetaDataUtils &spirMDUtils)
    {
        int oclMajor = 0, oclMinor = 0;
        if (spirMDUtils.isOpenCLVersionsHasValue())
        {
            SPIRMD::VersionMetaDataHandle oclVersion = spirMDUtils.getOpenCLVersionsItem(0);
            oclMajor = oclVersion->getMajor();
            oclMinor = oclVersion->getMinor();
        }
        else
        {
            if (!spirMDUtils.empty_CompilerOptions())
            {
                // check compiler options
                for (auto i = spirMDUtils.getCompilerOptionsItem(0)->begin(), e = spirMDUtils.getCompilerOptionsItem(0)->end(); i != e; ++i)
                {
                    if (StringRef(*i).startswith("-cl-std=CL") && i->length() >= 13)
                    {
                        oclMajor = i->at(10) - '0';
                        oclMinor = i->at(12) - '0';
                        break;
                    }
                }
            }
            // default is 1.2
            if (!isLegalOCLVersion(oclMajor, oclMinor))
            {
                oclMajor = 1;
                oclMinor = 2;
            }
        }
        return oclMajor;
    }


static void CommonOCLBasedPasses(
    OpenCLProgramContext* pContext,
    std::unique_ptr<llvm::Module> BuiltinGenericModule,
    std::unique_ptr<llvm::Module> BuiltinSizeModule)
{
    IGCPassManager mpm(pContext, "Unify");

#if defined( _DEBUG )
    llvm::verifyModule(*pContext->getModule());
#endif

    COMPILER_TIME_START(pContext, TIME_UnificationPasses);

    unify_opt_PreProcess(pContext);

    DumpLLVMIR(pContext, "beforeUnification");

    // override the data layout to match Gen HW
    int pointerSize = getPointerSize(*pContext->getModule());
    std::string layoutstr;
    if (pointerSize == 4)
    {
        layoutstr = "e-p:32:32:32";
    }
    else {
        layoutstr = "e-p:64:64:64";
    }
    layoutstr += "-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64"\
        "-f32:32:32-f64:64:64-v16:16:16-v24:32:32"\
        "-v32:32:32-v48:64:64-v64:64:64-v96:128:128"\
        "-v128:128:128-v192:256:256-v256:256:256"\
        "-v512:512:512-v1024:1024:1024-n8:16:32";

    StringRef dataLayout = layoutstr;
    pContext->getModule()->setDataLayout(dataLayout);
    BuiltinGenericModule->setDataLayout(dataLayout);
    if( BuiltinSizeModule )
    {
        BuiltinSizeModule->setDataLayout(dataLayout);
    }

    MetaDataUtils *pMdUtils = pContext->getMetaDataUtils();

    //extracting OCL version major before SPIRMetadataTranslation pass deletes its metadata node
    const SPIRMD::SpirMetaDataUtils spirMDUtils(&(*pContext->getModule()));
    int OCLMajor = getOCLMajorVersion(spirMDUtils);

    CompOptions &CompilerOpts = pContext->getModuleMetaData()->compOpt;

    // check OpenCL build options
    bool shouldForceCR = pContext->m_Options.CorrectlyRoundedSqrt;

    CompilerOpts.replaceGlobalOffsetsByZero =
        pContext->m_InternalOptions.replaceGlobalOffsetsByZero;

    CompilerOpts.SubgroupIndependentForwardProgressRequired =
        (pContext->m_Options.NoSubgroupIFP == false);

    if (OCLMajor >= 2)
    {
        CompilerOpts.UniformWGS =
            pContext->m_Options.UniformWGS;
    }

    CompilerOpts.GreaterThan2GBBufferRequired =
        !pContext->m_InternalOptions.Use32BitPtrArith;

    CompilerOpts.GreaterThan4GBBufferRequired =
        pContext->m_InternalOptions.IntelGreaterThan4GBBufferRequired;

    CompilerOpts.DisableA64WA =
        pContext->m_InternalOptions.IntelDisableA64WA;

    CompilerOpts.ForceEnableA64WA =
        pContext->m_InternalOptions.IntelForceEnableA64WA;

    CompilerOpts.HasBufferOffsetArg =
        pContext->m_InternalOptions.IntelHasBufferOffsetArg;

    CompilerOpts.PreferBindlessImages =
        pContext->m_InternalOptions.PreferBindlessImages;

    if (CompilerOpts.PreferBindlessImages) {
        pContext->getModuleMetaData()->UseBindlessImage = true;
    }

    CompilerOpts.EnableTakeGlobalAddress =
        pContext->m_Options.EnableTakeGlobalAddress;

    // right now we don't support any standard function in the code gen
    // maybe we want to support some at some point to take advantage of LLVM optimizations
    TargetLibraryInfoImpl TLI;
    TLI.disableAllFunctions();

    mpm.add( new llvm::TargetLibraryInfoWrapperPass(TLI));

    // This should be removed, once FE will be updated to use LLVM IR that supports
    // AllowContract and ApproxFunc FastMathFlags.
    mpm.add(new FixFastMathFlags());

    IGCPassManager mpmSPIR(pContext, "Unify");
    mpmSPIR.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
    mpmSPIR.add(new CodeGenContextWrapper(pContext));
    mpmSPIR.add(new SPIRMetaDataTranslation());
    mpmSPIR.run(*pContext->getModule());

    mpm.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
    mpm.add(new CodeGenContextWrapper(pContext));

    mpm.add(new ClampLoopUnroll(256));

    mpm.add(new MoveStaticAllocas());

    mpm.add(new UnreachableHandling());

    // Skip this pass if OCL version < 2.0
    if (!(OCLMajor < 2))
    {
        mpm.add(createTransformBlocksPass());
    }

    // Clone kernel function being used as user function.
    mpm.add(createKernelFunctionCloningPass());

    mpm.add(new CorrectlyRoundedDivSqrt(shouldForceCR, false));
    if(IGC_IS_FLAG_ENABLED(EnableIntelFast))
    {
        mpm.add(createBIFTransformsPass());
    }

    if(pContext->m_InternalOptions.KernelDebugEnable)
    {
        IF_DEBUG_INFO(mpm.add(new ImplicitGlobalId());)
    }

    if (IGC_IS_FLAG_ENABLED(EnableCodeAssumption))
    {
        mpm.add(new CodeAssumption());
    }

    if (pContext->m_instrTypes.hasFRem)
    {
        mpm.add(new HandleFRemInstructions());
    }

    mpm.add(new PreBIImportAnalysis());
    mpm.add(createTimeStatsCounterPass(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_START));
    mpm.add(createBuiltInImportPass(std::move(BuiltinGenericModule), std::move(BuiltinSizeModule)));
    mpm.add(createTimeStatsCounterPass(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_END));
    mpm.add(new UndefinedReferencesPass());

    // Estimate maximal function size in the module and disable subroutine if not profitable.
    mpm.add(createEstimateFunctionSizePass());

    if (IGC_GET_FLAG_VALUE(AllowMem2Reg))
    {
        mpm.add(createPromoteMemoryToRegisterPass());
    }

    mpm.add(new CatchAllLineNumber());

    // OCL has built-ins so it always need to run inlining
    {
        mpm.add(createProcessFuncAttributesPass());
        if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
        {
            int Threshold = IGC_GET_FLAG_VALUE(OCLInlineThreshold);
            mpm.add(createFunctionInliningPass(Threshold));
        }
        else
        {
            mpm.add(createAlwaysInlinerLegacyPass());
        }
        // The inliner sometimes fails to delete unused functions, this cleans up the remaining mess.
        mpm.add(createGlobalDCEPass());

        // Check after GlobalDCE in case of doubles in dead functions
        mpm.add(new ErrorCheck());

        if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
        {
            mpm.add(createProcessBuiltinMetaDataPass());
        }
        mpm.add(new PurgeMetaDataUtils());
    }

    // OpenCL WI + image function resolution

    // OCLTODO : do another DCE that will get rid of unused WI func calls before this?
    // We can save passing of unused implicit args from the runtime

    // Adding Mem2Reg pass in order to help ImageFuncsAnalysis to identify the image arguments
    // that the image functions operate on
    // Clang output is: alloca --> store image func arg into allocated address -->
    //                  load image arg from stored address --> call func on loaded image
    // After Mem2Reg: call func on image func arg

    mpm.add(createSROAPass());

    mpm.add(new BreakConstantExpr());

    if (IGC_IS_FLAG_ENABLED(EnableGASResolver))
    {
        // Add fix up of illegal `addrspacecast` in respect to OCL 2.0 spec.
        mpm.add(createFixAddrSpaceCastPass());
        mpm.add(createResolveGASPass());
        // Run another round of constant breaking as GAS resolving may generate constants (constant address)
        mpm.add(new BreakConstantExpr());
    }

    if (CompilerOpts.UniformWGS)
        mpm.add(new RewriteLocalSize());

    mpm.add(createSROAPass());
    mpm.add(new BreakConstantExpr());

    mpm.add(CreateFoldKnownWorkGroupSizes());

    // 64-bit atomics have to be resolved before AddImplicitArgs pass as it uses
    // local ids for spin lock initialization
    mpm.add(new ResolveOCLAtomics());

    // Run the AlignmentAnalysis pass before the passes which add implicit arguments, to ensure we do not lose load/store alignment information.
    // For example, ProgramScopeConstantResolution will relocate the buffer's base to an i8* typed pointer.
    mpm.add(new AlignmentAnalysis());

    // Analysis passes
    mpm.add(new WIFuncsAnalysis());
    mpm.add(new ImageFuncsAnalysis());
    mpm.add(new OpenCLPrintfAnalysis());
    mpm.add(createDeadCodeEliminationPass());
    mpm.add(new ProgramScopeConstantAnalysis());
    mpm.add(new PrivateMemoryUsageAnalysis());
    mpm.add(new AggregateArgumentsAnalysis());
    mpm.add(new ExtensionFuncsAnalysis());
    mpm.add(new ExtensionArgAnalysis());
    mpm.add(new DeviceEnqueueFuncsAnalysis());
    mpm.add(createGenericAddressAnalysisPass());
    if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
    {
        mpm.add(new BuiltinCallGraphAnalysis());
    }
    // Adding implicit args based on Analysis passes
    mpm.add(new AddImplicitArgs());

    // Resolution passes
    mpm.add(new WIFuncResolution());
    mpm.add(new OpenCLPrintfResolution());
    mpm.add(new ResolveOCLAtomics());
    mpm.add(new ResourceAllocator());
    mpm.add(new SubGroupFuncsResolution());

    // Run InlineLocals and GenericAddressDynamic together
    mpm.add(new InlineLocalsResolution());

    mpm.add(new WGFuncResolution());
    mpm.add(new ResolveAggregateArguments());
    mpm.add(new ExtensionFuncsResolution());
    mpm.add(new DeviceEnqueueFuncsResolution());

    mpm.add(createDeadCodeEliminationPass());

    mpm.add(createBuiltinsConverterPass());

    // check for unsupported intrinsics
    mpm.add(new ErrorCheck());

    mpm.add(new ImageFuncResolution());
    mpm.add(new Image3dToImage2darray());

    // Break down the intrinsics into smaller operations (eg. fmuladd to fmul add)
    mpm.add(new BreakdownIntrinsicPass());

    {
        if(IGC_IS_FLAG_ENABLED(EnableConstantPromotion))
        {
            mpm.add(createSimplifyConstantPass());
            mpm.add(createPromoteConstantPass());
        }
        mpm.add(createIGCInstructionCombiningPass());

        // Instcombine can create constant expressions, which are not handled by the program scope constant resolution pass
        mpm.add(new BreakConstantExpr());

        // Run constant lowering conservatively for tests where constant
        // objects are over-written after casting pointers in constant address
        // space into ones in private address.
        //
        // NOTE: Per OpenCL C standard (both 1.2 and 2.0), that's illegal.
        //
        // This has to be run after instcombine to allow memcpy from GlobalVariable arrays private
        // allocs to be optimized away.
        mpm.add(new ProgramScopeConstantResolution(true));
    }

    // TODO: Run CheckInstrTypes after builtin import to determine if builtins have allocas.
    mpm.add(createSROAPass());
    mpm.add(createIGCInstructionCombiningPass());

    // "false" to createScalarizerPass() means that vector load/stores are NOT scalarized
    mpm.add(createScalarizerPass(false));

    // Add SetFastMathFalgs pass at the end so that we don't need to worry about previous passes that
    // forget setting math flags. We expect the generic llvm optimizations after the unification
    // (optimizeIR()) will fully take advantage of the flags.
    mpm.add(new SetFastMathFlags());
    mpm.add(new FixResourcePtr());

    bool isOptDisabled = CompilerOpts.OptDisable;
    if(isOptDisabled)
    {
        // Run additional predefined constant resolving when optimization is
        // disabled. It's definitely a workaround so far.
        mpm.add(createResolvePredefinedConstantPass());
    }

    mpm.add(createLowerSwitchPass());
    mpm.add(createTypeLegalizerPass());

    mpm.run(*pContext->getModule());

    // Following functions checks whether -g option is specified.
    // The flag is set only in SPIRMetadataTranslationPass which
    // is run in above mpm.run statement. The downside of calling
    // this function here, as opposed to beginning of this function,
    // is that unreferenced constants will be eliminated. So
    // debugger will not be able to query those variables.
    IF_DEBUG_INFO(insertOCLMissingDebugConstMetadata(pContext);)

    COMPILER_TIME_END(pContext, TIME_UnificationPasses);

    DumpLLVMIR(pContext, "afterUnification");

    MEM_SNAPSHOT(IGC::SMS_AFTER_UNIFICATION);
}

void UnifyIROCL(
    OpenCLProgramContext* pContext,
    std::unique_ptr<llvm::Module> BuiltinGenericModule,
    std::unique_ptr<llvm::Module> BuiltinSizeModule)
{
    CommonOCLBasedPasses(pContext, std::move(BuiltinGenericModule), std::move(BuiltinSizeModule));
}

void UnifyIRSPIR(
    OpenCLProgramContext* pContext,
    std::unique_ptr<llvm::Module> BuiltinGenericModule,
    std::unique_ptr<llvm::Module> BuiltinSizeModule)
{
    CommonOCLBasedPasses(pContext, std::move(BuiltinGenericModule), std::move(BuiltinSizeModule));
}
}
