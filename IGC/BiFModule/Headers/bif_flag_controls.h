/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#ifndef __BIF_FLAG_CONTROL_H__
#define __BIF_FLAG_CONTROL_H__

BIF_FLAG_CONTROL(PRODUCT_FAMILY, PlatformType)
BIF_FLAG_CONTROL(GFXCORE_FAMILY, RenderFamily)
BIF_FLAG_CONTROL(bool, FlushDenormals)
BIF_FLAG_CONTROL(bool, FastRelaxedMath)
BIF_FLAG_CONTROL(bool, DashGSpecified)
BIF_FLAG_CONTROL(bool, OptDisable)
BIF_FLAG_CONTROL(bool, MadEnable)
BIF_FLAG_CONTROL(bool, UseMathWithLUT)
BIF_FLAG_CONTROL(bool, UseNativeFP32GlobalAtomicAdd)
BIF_FLAG_CONTROL(bool, UseNativeFP16AtomicMinMax)
BIF_FLAG_CONTROL(bool, HasInt64SLMAtomicCAS)
BIF_FLAG_CONTROL(bool, UseNativeFP64GlobalAtomicAdd)
BIF_FLAG_CONTROL(bool, UseNative64BitIntBuiltin)
BIF_FLAG_CONTROL(bool, UseBfn)
BIF_FLAG_CONTROL(bool, HasThreadPauseSupport)
BIF_FLAG_CONTROL(bool, UseNative64BitFloatBuiltin)
BIF_FLAG_CONTROL(bool, hasHWLocalThreadID)
BIF_FLAG_CONTROL(bool, CRMacros)
BIF_FLAG_CONTROL(bool, APIRS)
BIF_FLAG_CONTROL(bool, IsSPIRV)
BIF_FLAG_CONTROL(float, ProfilingTimerResolution)
BIF_FLAG_CONTROL(bool, UseLSC)
BIF_FLAG_CONTROL(bool, ForceL1Prefetch)
BIF_FLAG_CONTROL(bool, UseHighAccuracyMath)
BIF_FLAG_CONTROL(bool, EnableSWSrgbWrites)
BIF_FLAG_CONTROL(int, MaxHWThreadIDPerSubDevice)
BIF_FLAG_CONTROL(bool, UseAssumeInGetGlobalId)
BIF_FLAG_CONTROL(int, JointMatrixLoadStoreOpt)
BIF_FLAG_CONTROL(bool, UseOOBChecks)
BIF_FLAG_CONTROL(bool, UseBindlessImage)
#endif // __BIF_FLAG_CONTROL_H__
