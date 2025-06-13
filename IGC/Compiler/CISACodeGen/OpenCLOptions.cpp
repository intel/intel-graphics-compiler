/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/CommandLine.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/OpenCLOptions.hpp"
#include "common/igc_regkeys.hpp"
#include "igc/Options/Options.h"

namespace IGC {
void InternalOptions::parseOptions(const char* internalOpts)
{
    using namespace options::internal;
    if (internalOpts == nullptr)
        return;

    llvm::BumpPtrAllocator alloc;
    llvm::StringSaver saver{ alloc };
    llvm::SmallVector<const char*, 8> argv;
    llvm::cl::TokenizeGNUCommandLine(internalOpts, saver, argv);

    const llvm::opt::OptTable& internalOptionsTable = getInternalOptTable();
    unsigned missingArgIndex = 0;
    unsigned missingArgCount = 0;
    llvm::opt::InputArgList internalOptions =
        internalOptionsTable.ParseArgs(argv, missingArgIndex, missingArgCount, options::Flags::IGCInternalOption);

    if (internalOptions.hasArg(OPT_replace_global_offsets_by_zero_common))
    {
        replaceGlobalOffsetsByZero = true;
    }

    if (internalOptions.hasArg(OPT_kernel_debug_enable_common))
    {
        KernelDebugEnable = true;
    }

    if (internalOptions.hasArg(OPT_include_sip_csr_common))
    {
        IncludeSIPCSR = true;
    }

    if (internalOptions.hasArg(OPT_include_sip_kernel_debug_common))
    {
        IncludeSIPKernelDebug = true;
    }

    if (internalOptions.hasArg(OPT_include_sip_kernel_local_debug_common))
    {
        IncludeSIPKernelDebugWithLocalMemory = true;
    }

    if (internalOptions.hasArg(OPT_use_32_bit_ptr_arith_common))
    {
        Use32BitPtrArith = true;
    }

    if (internalOptions.hasArg(OPT_greater_than_4GB_buffer_required_common))
    {
        IntelGreaterThan4GBBufferRequired = true;
    }

    if (internalOptions.hasArg(OPT_has_buffer_offset_arg_common))
    {
        IntelHasBufferOffsetArg = true;
    }

    if (internalOptions.hasArg(OPT_buffer_offset_arg_required_common))
    {
        IntelBufferOffsetArgOptional = false;
    }

    if (internalOptions.hasArg(OPT_has_positive_pointer_offset_common))
    {
        IntelHasPositivePointerOffset = true;
    }

    if (internalOptions.hasArg(OPT_disable_a64wa_common))
    {
        IntelDisableA64WA = true;
    }

    if (internalOptions.hasArg(OPT_force_enable_a64wa_common))
    {
        IntelForceEnableA64WA = true;
    }

    if (internalOptions.hasArg(OPT_no_prera_scheduling_common))
    {
        IntelEnablePreRAScheduling = false;
    }

    if (internalOptions.hasArg(OPT_force_global_mem_allocation_common))
    {
        ForceGlobalMemoryAllocation = true;
    }

    if (internalOptions.hasArg(OPT_128_grf_per_thread_common))
    {
        Intel128GRFPerThread = true;
    }

    if (internalOptions.hasArg(OPT_256_grf_per_thread_common))
    {
        Intel256GRFPerThread = true;
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_num_thread_per_eu_common))
    {
        IntelNumThreadPerEU = true;
        llvm::StringRef valStr = arg->getValue();
        valStr.getAsInteger(10, numThreadsPerEU);
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_exp_register_file_size_common))
    {
        IntelExpGRFSize = true;
        llvm::StringRef valStr = arg->getValue();
        valStr.getAsInteger(10, expGRFSize);
    }

    if (internalOptions.hasArg(OPT_enable_auto_large_GRF_mode_common))
    {
        IntelEnableAutoLargeGRF = true;
    }
    if (internalOptions.hasArg(OPT_disable_recompilation_common))
    {
        IGC_SET_FLAG_VALUE(DisableRecompilation, true);
    }

    if (internalOptions.hasArg(OPT_force_emu_int32divrem_common))
    {
        IntelForceInt32DivRemEmu = true;
    }

    if (internalOptions.hasArg(OPT_force_emu_sp_int32divrem_common))
    {
        IntelForceInt32DivRemEmuSP = true;
    }

    if (internalOptions.hasArg(OPT_force_disable_4GB_buffer_common))
    {
        IntelForceDisable4GBBuffer = true;
    }

    if (internalOptions.hasArg(OPT_use_bindless_buffers_common))
    {
        PromoteStatelessToBindless = true;
    }

    if (internalOptions.hasArg(OPT_use_bindless_images_common))
    {
        PreferBindlessImages = true;
    }

    if (internalOptions.hasArg(OPT_use_bindless_mode_common))
    {
        // This is a new option that combines bindless generation for buffers
        // and images. Keep the old internal options to have compatibility
        // for existing tests. Those (old) options could be removed in future.
        UseBindlessMode = true;
        PreferBindlessImages = true;
        PromoteStatelessToBindless = true;
    }

    if (internalOptions.hasArg(OPT_use_bindless_printf_common))
    {
        UseBindlessPrintf = true;
    }

    if (internalOptions.hasArg(OPT_use_bindless_legacy_mode_common))
    {
        UseBindlessLegacyMode = true;
    }

    if (internalOptions.hasArg(OPT_use_bindless_advanced_mode_common))
    {
        UseBindlessLegacyMode = false;
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_vector_coalescing_common))
    {
        // -cl-intel-vector-coalescing=<0-5>
        llvm::StringRef valStr = arg->getValue();
        int16_t val = 0;
        if (valStr.getAsInteger(10, val) || val < 0 || val > 5)
        {
            IGC_ASSERT_MESSAGE(false, "-cl-intel-vector-coalescing: invalid value, ignored!");
        }
        else
        {
            VectorCoalescingControl = val;
        }
    }

    if (internalOptions.hasArg(OPT_exclude_ir_from_zebin_common))
    {
        ExcludeIRFromZEBinary = true;
    }

    if (internalOptions.hasArg(OPT_emit_zebin_visa_sections_common))
    {
        EmitZeBinVISASections = true;
    }

    if (internalOptions.hasArg(OPT_no_spill_common))
    {
        // This is an option to avoid spill/fill instructions in scheduler kernel.
        // OpenCL Runtime triggers scheduler kernel offline compilation while driver building,
        // since scratch space is not supported in this specific case, we cannot end up with
        // spilling kernel. If this option is set, then IGC will recompile the kernel with
        // some some optimizations disabled to avoid spill/fill instructions.
        NoSpill = true;
    }

    if (internalOptions.hasArg(OPT_disable_noMaskWA_common))
    {
        DisableNoMaskWA = true;
    }

    if (internalOptions.hasArg(OPT_ignoreBFRounding_common))
    {
        IgnoreBFRounding = true;
    }

    if (internalOptions.hasArg(OPT_compile_one_at_time_common))
    {
        CompileOneKernelAtTime = true;
    }

    if (internalOptions.hasArg(OPT_skip_reloc_add_common))
    {
        AllowRelocAdd = false;
    }

    if (internalOptions.hasArg(OPT_disableEUFusion_common))
    {
        DisableEUFusion = true;
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_functonControl_common))
    {
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || val < 0)
        {
            IGC_ASSERT_MESSAGE(false, "-cl-intel-functonControl: invalid value, ignored!");
        }
        else
        {
            FunctionControl = val;
        }
    }

    if (internalOptions.hasArg(OPT_fail_on_spill_common))
    {
        FailOnSpill = true;
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_load_cache_default_common))
    {
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || val < 0)
        {
            IGC_ASSERT_MESSAGE(false, "-cl-load-cache-default: invalid value, ignored!");
        }
        else
        {
            LoadCacheDefault = val;
        }
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_store_cache_default_common))
    {
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || val < 0)
        {
            IGC_ASSERT_MESSAGE(false, "-cl-store-cache-default: invalid value, ignored!");
        }
        else
        {
            StoreCacheDefault = val;
        }
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_ldstcombine_common))
    {
        // Valid value: 0|1|2
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || (val !=0 && val != 1 && val != 2))
        {
            IGC_ASSERT_MESSAGE(false, "-ldstcombine: invalid and ignored!");
        }
        else
        {
            LdStCombine = val;
        }
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_ldstcombine_max_storebytes_common))
    {
        // Valid value: 4|8|16|32
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || !(llvm::isPowerOf2_32(val) && val >= 4 && val <= 32))
        {
            IGC_ASSERT_MESSAGE(false, "-ldstcombine_max_storebytes: invalid and ignored!");
        }
        else
        {
            MaxStoreBytes = val;
        }
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_ldstcombine_max_loadbytes_common))
    {
        // Valid value: 4|8|16|32
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || !(llvm::isPowerOf2_32(val) && val >= 4 && val <= 32))
        {
            IGC_ASSERT_MESSAGE(false, "-ldstcombine_max_loadbytes: invalid and ignored!");
        }
        else
        {
            MaxLoadBytes = val;
        }
    }

    if (internalOptions.hasArg(OPT_fp64_gen_emu_common))
    {
        // This option enables FP64 emulation for platforms that
        // cannot HW support for double operations
        EnableFP64GenEmu = true;
    }

    // *-private-memory-minimal-size-per-thread <SIZE>
    // SIZE >= 0
    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_private_memory_minimal_size_per_thread_common))
    {
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || val < 0)
        {
            IGC_ASSERT_MESSAGE(false, "-cl-private-memory-minimal-size-per-thread: invalid value, ignored!");
        }
        else
        {
            IntelPrivateMemoryMinimalSizePerThread = val;
        }
    }

    // *-scratch-space-private-memory-minimal-size-per-thread <SIZE>
    // SIZE >= 0
    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_scratch_space_private_memory_minimal_size_per_thread_common))
    {
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(10, val) || val < 0)
        {
            IGC_ASSERT_MESSAGE(false, "-cl-scratch-space-private-memory-minimal-size-per-thread: invalid value, ignored!");
        }
        else
        {
            IntelScratchSpacePrivateMemoryMinimalSizePerThread = val;
        }
    }

    if (internalOptions.hasArg(OPT_enable_divergent_barrier_handling_common))
    {
        EnableDivergentBarrierHandling = true;
    }

    if (internalOptions.hasArg(OPT_high_accuracy_nolut_math_common))
    {
        UseHighAccuracyMathFuncs = true;
    }

    if (internalOptions.hasArg(OPT_emit_visa_only))
    {
        EmitVisaOnly = true;
    }

    if (IntelForceDisable4GBBuffer)
    {
        IntelGreaterThan4GBBufferRequired = false;
    }

    if (internalOptions.hasArg(OPT_buffer_bounds_checking_common))
    {
        EnableBufferBoundsChecking = true;
    }

    if (const llvm::opt::Arg* arg = internalOptions.getLastArg(OPT_minimum_valid_address_checking_common))
    {
        llvm::StringRef valStr = arg->getValue();
        int val = 0;
        if (valStr.getAsInteger(16, val) || val < 0)
        {
            IGC_ASSERT_MESSAGE(false, "-cl-minimum-valid-address-checking: invalid value, ignored!");
        }
        else
        {
            MinimumValidAddress = val;
        }
    }
}

void Options::parseOptions(const char* opts)
{
    using namespace options::api;
    if (opts == nullptr)
        return;

    llvm::BumpPtrAllocator alloc;
    llvm::StringSaver saver{ alloc };
    llvm::SmallVector<const char*, 8> argv;
    llvm::cl::TokenizeGNUCommandLine(opts, saver, argv);

    const llvm::opt::OptTable& apiOptionsTable = getApiOptTable();
    unsigned missingArgIndex = 0;
    unsigned missingArgCount = 0;
    llvm::opt::InputArgList apiOptions =
        apiOptionsTable.ParseArgs(argv, missingArgIndex, missingArgCount, options::Flags::IGCApiOption);

    if (apiOptions.hasArg(OPT_fp32_correctly_rounded_divide_sqrt_common))
    {
        CorrectlyRoundedSqrt = true;
    }

    if (apiOptions.hasArg(OPT_no_subgroup_ifp_common))
    {
        NoSubgroupIFP = true;
    }

    if (apiOptions.hasArg(OPT_uniform_work_group_size_common))
    {
        // Note that this is only available for -cl-std >= 2.0.
        // This will be checked before we place this into the
        // the module metadata.
        UniformWGS = true;
    }

    if (apiOptions.hasArg(OPT_take_global_address_common))
    {
        EnableTakeGlobalAddress = true;
    }

    if (apiOptions.hasArg(OPT_library_compilation_common))
    {
        IsLibraryCompilation = true;
    }

    if (const llvm::opt::Arg* arg = apiOptions.getLastArg(OPT_library_compile_simd_common))
    {
        llvm::StringRef valStr = arg->getValue();
        unsigned simdsz = 0;
        if (!valStr.getAsInteger(10, simdsz) && (simdsz == 8 || simdsz == 16 || simdsz == 32))
            LibraryCompileSIMDSize = simdsz;
        else
            IGC_ASSERT_MESSAGE(false, "Library selected with invalid SIMD size");
    }

    if (apiOptions.hasArg(OPT_emit_lib_compile_errors_common))
    {
        EmitErrorsForLibCompilation = true;
    }

    if (apiOptions.hasArg(OPT_gtpin_rera_common))
    {
        GTPinReRA = true;
    }

    if (apiOptions.hasArg(OPT_gtpin_grf_info_common))
    {
        GTPinGRFInfo = true;
    }

    if (const llvm::opt::Arg* arg = apiOptions.getLastArg(OPT_gtpin_scratch_area_size_common))
    {
        if (arg->getNumValues() > 0)
        {
            GTPinScratchAreaSize = true;
            llvm::StringRef valStr = arg->getValue();
            valStr.getAsInteger(10, GTPinScratchAreaSizeValue);
        }
    }

    if (apiOptions.hasArg(OPT_gtpin_indir_ref_common))
    {
        GTPinIndirRef = true;
    }

    if (apiOptions.hasArg(OPT_skip_fde_common))
    {
        SkipFDE = true;
    }

    if (apiOptions.hasArg(OPT_no_fusedCallWA_common)) {
      NoFusedCallWA = true;
    }

    if (apiOptions.hasArg(OPT_disable_compaction_common)) {
      DisableCompaction = true;
    }

    if (const llvm::opt::Arg* arg = apiOptions.getLastArg(OPT_required_thread_count_common))
    {
        if (arg->getNumValues() > 0)
        {
            IntelRequiredEUThreadCount = true;
            llvm::StringRef valStr = arg->getValue();
            valStr.getAsInteger(10, requiredEUThreadCount);
        }
    }

    for (const auto& arg : apiOptions.getAllArgValues(OPT_large_grf_kernel_common))
    {
        LargeGRFKernels.push_back(arg);
    }

    for (const auto& arg : apiOptions.getAllArgValues(OPT_regular_grf_kernel_common))
    {
        RegularGRFKernels.push_back(arg);
    }

    if (apiOptions.hasArg(OPT_enable_auto_large_GRF_mode_common))
    {
        IntelEnableAutoLargeGRF = true;
    }

    if (const llvm::opt::Arg* arg = apiOptions.getLastArg(OPT_exp_register_file_size_common))
    {
        IntelExpGRFSize = true;
        llvm::StringRef valStr = arg->getValue();
        valStr.getAsInteger(10, expGRFSize);

        IntelLargeRegisterFile = expGRFSize == 256;
    }

    if (apiOptions.hasArg(OPT_no_local_to_generic_common))
    {
        NoLocalToGeneric = true;
    }

    if (apiOptions.hasArg(OPT_greater_than_4GB_buffer_required_common))
    {
        IntelGreaterThan4GBBufferRequired = true;
    }

    if (apiOptions.hasArg(OPT_128_grf_per_thread_common))
    {
        Intel128GRFPerThread = true;
    }

    if (apiOptions.hasArg(OPT_256_grf_per_thread_common))
    {
        Intel256GRFPerThread = true;
    }

    if (apiOptions.hasArg(OPT_poison_unsupported_fp64_kernels_common))
    {
        // This option forces IGC to poison kernels using fp64
        // operations on platforms without HW support for fp64.
        EnableUnsupportedFP64Poisoning = true;
    }

    if (apiOptions.hasArg(OPT_fp64_gen_emu_common))
    {
        // This option enables FP64 emulation for platforms that
        // cannot HW support for double operations
        EnableFP64GenEmu = true;
    }

    if (apiOptions.hasArg(OPT_fp64_gen_conv_emu_common))
    {
        // This option enables FP64 emulation for conversions
        // This applies to platforms that cannot HW support for double operations
        EnableFP64GenConvEmu = true;
    }

    if (const llvm::opt::Arg* arg = apiOptions.getLastArg(OPT_Xfinalizer))
    {
        Xfinalizer = true;
        XfinalizerOption = arg->getValue();
    }

    if (apiOptions.hasArg(OPT_static_profile_guided_trimming_common)) {
        StaticProfileGuidedTrimming = true;
    }

    if (apiOptions.hasArg(OPT_enable_ieee_float_exception_trap_common)) {
        EnableIEEEFloatExceptionTrap = true;
    }
}
} // namespace IGC
