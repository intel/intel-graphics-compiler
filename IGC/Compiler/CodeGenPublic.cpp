/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Option/ArgList.h"
#include "igc/Options/Options.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;

OpenCLProgramContext::InternalOptions &OpenCLProgramContext::InternalOptions::createInternalOptions(const TC::STB_TranslateInputArgs *pInputArgs)
{
    InternalOptions *obj = new InternalOptions();

    if (pInputArgs == nullptr)
        return *obj;

    if (pInputArgs->pInternalOptions == nullptr)
        return *obj;

    llvm::BumpPtrAllocator Alloc;
    llvm::StringSaver Saver{Alloc};
    llvm::SmallVector<const char *, 8> Argv;
    const char *options = pInputArgs->pInternalOptions;
    llvm::cl::TokenizeGNUCommandLine(options, Saver, Argv);

    const llvm::opt::OptTable &AvailableOptions = IGC::getInternalOptTable();
    unsigned MissingArgIndex = 0;
    unsigned MissingArgCount = 0;
    const unsigned FlagsToInclude = IGC::options::IGCInternalOption;

    llvm::opt::InputArgList InputArgs =
        AvailableOptions.ParseArgs(Argv, MissingArgIndex, MissingArgCount, FlagsToInclude);

    using namespace IGC::options::internal;

    if (InputArgs.hasArg(OPT_replace_global_offsets_by_zero_ze))
    {
        obj->replaceGlobalOffsetsByZero = true;
    }
    if (InputArgs.hasArg(OPT_kernel_debug_enable_ze))
    {
        obj->KernelDebugEnable = true;
    }

    if (InputArgs.hasArg(OPT_include_sip_csr_ze))
    {
        obj->IncludeSIPCSR = true;
    }

    if (InputArgs.hasArg(OPT_include_sip_kernel_debug_ze))
    {
        obj->IncludeSIPKernelDebug = true;
    }
    else if (InputArgs.hasArg(OPT_include_sip_kernel_local_debug_ze))
    {
        obj->IncludeSIPKernelDebugWithLocalMemory = true;
    }

    if (InputArgs.hasArg(OPT_use_32_bit_ptr_arith_ze))
    {
        obj->Use32BitPtrArith = true;
    }

    // -cl-intel-has-buffer-offset-arg, -ze-opt-has-buffer-offset-arg
    if (InputArgs.hasArg(OPT_has_buffer_offset_arg))
    {
        obj->IntelHasBufferOffsetArg = true;
    }

    // -cl-intel-greater-than-4GB-buffer-required, -ze-opt-greater-than-4GB-buffer-required
    if (InputArgs.hasArg(OPT_greater_than_4GB_buffer_required))
    {
        obj->IntelGreaterThan4GBBufferRequired = true;
    }

    // -cl-intel-buffer-offset-arg-required, -ze-opt-buffer-offset-arg-required
    if (InputArgs.hasArg(OPT_buffer_offset_arg_required))
    {
        obj->IntelBufferOffsetArgOptional = false;
    }

    // -cl-intel-has-positive-pointer-offset, -ze-opt-has-positive-pointer-offset
    if (InputArgs.hasArg(OPT_has_positive_pointer_offset))
    {
        obj->IntelHasPositivePointerOffset = true;
    }

    // -cl-intel-has-subDW-aligned-ptr-arg, -ze-opt-has-subDW-aligned-ptr-arg
    if (InputArgs.hasArg(OPT_has_subdw_aligned_ptr_arg))
    {
        obj->IntelHasSubDWAlignedPtrArg = true;
    }

    if (InputArgs.hasArg(OPT_intel_disable_a64wa_ze))
    {
        obj->IntelDisableA64WA = true;
    }

    if (InputArgs.hasArg(OPT_intel_force_enable_a64wa_ze))
    {
        obj->IntelForceEnableA64WA = true;
    }

    if (InputArgs.hasArg(OPT_intel_enable_prera_scheduling_ze))
    {
        obj->IntelEnablePreRAScheduling = true;
    }
    if (InputArgs.hasArg(OPT_intel_use_bindless_buffers_ze))
    {
        obj->PromoteStatelessToBindless = true;
    }
    if (InputArgs.hasArg(OPT_intel_use_bindless_images_ze))
    {
        obj->PreferBindlessImages = true;
    }
    if (InputArgs.hasArg(OPT_intel_use_bindless_mode_ze))
    {
        // This is a new option that combines bindless generation for buffers
        // and images. Keep the old internal options to have compatibility
        obj->UseBindlessMode = true;
        obj->PreferBindlessImages = true;
        obj->PromoteStatelessToBindless = true;
    }
    if (InputArgs.hasArg(OPT_intel_use_bindless_printf_ze))
    {
        obj->UseBindlessPrintf = true;
    }

    if (llvm::opt::Arg *A = InputArgs.getLastArg(OPT_intel_vector_coalescing_ze))
    {
        llvm::StringRef Val = A->getValue();
        unsigned Result;
        Val.getAsInteger(0, Result);
        obj->VectorCoalescingControl = Result;
    }
    if (InputArgs.hasArg(OPT_allow_zebin_ze))
    {
        obj->EnableZEBinary = true;
    }
    if (InputArgs.hasArg(OPT_intel_no_spill_ze))
    {
        // This is an option to avoid spill/fill instructions in scheduler kernel.
        // OpenCL Runtime triggers scheduler kernel offline compilation while driver building,
        // since scratch space is not supported in this specific case, we cannot end up with
        // spilling kernel. If this option is set, then IGC will recompile the kernel with
        // some some optimizations disabled to avoid spill/fill instructions.
        obj->NoSpill = true;
    }

    return *obj;
}

OpenCLProgramContext::Options &OpenCLProgramContext::Options::createOptions(const TC::STB_TranslateInputArgs *pInputArgs)
{
    Options *obj = new Options();

    if (pInputArgs == nullptr)
        return *obj;

    if (pInputArgs->pOptions == nullptr)
        return *obj;

    llvm::BumpPtrAllocator Alloc;
    llvm::StringSaver Saver{Alloc};
    llvm::SmallVector<const char *, 8> Argv;
    const char *options = pInputArgs->pInternalOptions;
    llvm::cl::TokenizeGNUCommandLine(options, Saver, Argv);

    const llvm::opt::OptTable &AvailableOptions = IGC::getApiOptTable();
    unsigned MissingArgIndex = 0;
    unsigned MissingArgCount = 0;
    const unsigned FlagsToInclude = IGC::options::IGCApiOption;

    llvm::opt::InputArgList InputArgs =
        AvailableOptions.ParseArgs(Argv, MissingArgIndex, MissingArgCount, FlagsToInclude);

    using namespace IGC::options::api;

    if (InputArgs.hasArg(OPT_fp32_correctly_rounded_divide_sqrt_ze))
    {
        obj->CorrectlyRoundedSqrt = true;
    }
    if (InputArgs.hasArg(OPT_no_subgroup_ifp_ze))
    {
        obj->NoSubgroupIFP = true;
    }
    if (InputArgs.hasArg(OPT_uniform_work_group_size_ze))
    {
        // Note that this is only available for -cl-std >= 2.0.
        // This will be checked before we place this into the
        // the module metadata.
        obj->UniformWGS = true;
    }
    if (InputArgs.hasArg(OPT_take_global_address_ze))
    {
        obj->EnableTakeGlobalAddress = true;
    }
    if (InputArgs.hasArg(OPT_library_compilation_ze))
    {
        obj->IsLibraryCompilation = true;
    }


    return *obj;
}