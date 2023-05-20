/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

///
/// Generic form:
///    DEF_ATTR(Enum, Name, Kind, Default, Desc)
///
/// Specific form:
/// DEF_ATTR_BOOL(Enum, Name, Kind, Default, Desc)
/// DEF_ATTR_INT32(Enum, Name, Kind, Default, Desc)
/// DEF_ATTR_INT64(Enum, Name, Kind, Default, Desc)
/// DEF_ATTR_CSTR(Enum, Name, Kind, Default, Desc)
///

#ifdef DEF_ATTR
#define DEF_ATTR_BOOL(E, N, K, Default, D) DEF_ATTR(E, N, K, Default, D)
#define DEF_ATTR_INT32(E, N, K, Default, D) DEF_ATTR(E, N, K, Default, D)
#define DEF_ATTR_INT64(E, N, K, Default, D) DEF_ATTR(E, N, K, Default, D)
#define DEF_ATTR_CSTR(E, N, K, Default, D) DEF_ATTR(E, N, K, Default, D)
#endif

// bool attributes
DEF_ATTR_BOOL(ATTR_Extern, "Extern", AK_KERNEL, false,
              "True for extern function")
DEF_ATTR_BOOL(ATTR_NoBarrier, "NoBarrier", AK_KERNEL, false,
              "True if kernel has no barrier")
DEF_ATTR_BOOL(ATTR_Callable, "Callable", AK_KERNEL, false, "")
DEF_ATTR_BOOL(ATTR_Caller, "Caller", AK_KERNEL, false, "")
DEF_ATTR_BOOL(ATTR_Composable, "Composable", AK_KERNEL, false, "")
DEF_ATTR_BOOL(ATTR_Entry, "Entry", AK_KERNEL, false, "")
DEF_ATTR_BOOL(ATTR_Input, "Input", AK_VAR, false, "True if var is input var")
DEF_ATTR_BOOL(ATTR_Output, "Output", AK_VAR, false, "True if var is output var")
DEF_ATTR_BOOL(ATTR_PayloadLiveOut, "PayloadLiveOut", AK_VAR, false,
              "True if var is payload liveout var")
DEF_ATTR_BOOL(ATTR_Input_Output, "Input_Output", AK_VAR, false,
              "True if var is both input and output var")
DEF_ATTR_BOOL(ATTR_NoWidening, "NoWidening", AK_VAR, false, "")
DEF_ATTR_BOOL(ATTR_DoNotSpill, "DoNotSpill", AK_VAR, false,
              "True if var should not be spilled")
DEF_ATTR_BOOL(ATTR_ForceSpill, "ForceSpill", AK_VAR, false,
              "True if var is forced to be spilled")
DEF_ATTR_BOOL(ATTR_SepSpillPvtSS, "SepSpillPvtSS", AK_KERNEL, false, "")
DEF_ATTR_BOOL(ATTR_LTOInvokeOptTarget, "LTO_InvokeOptTarget", AK_KERNEL, 0, "")
DEF_ATTR_BOOL(ATTR_AllLaneActive, "AllLaneActive", AK_KERNEL, false,
              "True if all lanes are active at the function entry")

// int32 attributes
DEF_ATTR_INT32(ATTR_Target, "Target", AK_KERNEL, VISA_CM,
               "Software Platform that generates this kernel (CM, 3D, CS)")
DEF_ATTR_INT32(ATTR_SLMSize, "SLMSize", AK_KERNEL, 0, "SLM size")
DEF_ATTR_INT32(ATTR_SpillMemOffset, "SpillMemOffset", AK_KERNEL, 0,
               "Offset at which location of GRF spill/fill memory starts")
DEF_ATTR_INT32(ATTR_ArgSize, "ArgSize", AK_KERNEL, 0,
               "Argument's size in bytes")
DEF_ATTR_INT32(ATTR_RetValSize, "RetValSize", AK_KERNEL, 0,
               "Return value's size in bytes")
DEF_ATTR_INT32(ATTR_PerThreadInputSize, "PerThreadInputSize", AK_KERNEL, 0,
               "per-thread payload size in bytes")
DEF_ATTR_INT32(ATTR_SimdSize, "SimdSize", AK_KERNEL, 0,
               "Dispatch simd size. 0 if not provided.")
DEF_ATTR_INT32(ATTR_Scope, "Scope", AK_VAR, 0, "")
DEF_ATTR_INT32(ATTR_SurfaceUsage, "SurfaceUsage", AK_VAR, 0, "")
DEF_ATTR_INT32(ATTR_CrossThreadInputSize, "CrossThreadInputSize", AK_KERNEL, -1,
               "cross-thread payload size in bytes")
DEF_ATTR_INT32(ATTR_NBarrierCnt, "NBarrierCnt", AK_KERNEL, 0, "")

// C String Attributes
DEF_ATTR_CSTR(ATTR_OutputAsmPath, "OutputAsmPath", AK_KERNEL, "",
              "Directory name under which output files go")

#undef DEF_ATTR
#undef DEF_ATTR_BOOL
#undef DEF_ATTR_INT32
#undef DEF_ATTR_INT64
#undef DEF_ATTR_CSTR
