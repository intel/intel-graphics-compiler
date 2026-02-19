;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -loop-gating -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK
; ------------------------------------------------
; GatingSimilarSamples
; ------------------------------------------------

; Test checks gating of similar sample instructions(to skip redundant) :
;   New if-then block with gating condition is created
;   Corresponding phi nodes(x,y,z) have fast math flag set

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(float %src1, float %src2, float %src3, float* %dst) {
; CHECK-LABEL: @test(
; CHECK:    [[MUL_1:%[A-z0-9]*]] = fmul float [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    [[A:%[A-z0-9]*]] = fsub float [[SRC1]], [[MUL_1]]
; CHECK:    [[B:%[A-z0-9]*]] = fadd float [[SRC1]], [[MUL_1]]
; CHECK:    [[TMP1:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float [[SRC1]], float [[SRC2]], float 0.000000e+00, float 1.000000e+00, float [[SRC2]], float [[SRC3:%[A-z0-9]*]], i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
; CHECK:    [[TMP2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float [[B]], float [[A]], float [[SRC3]], float 1.000000e+00, float [[SRC2]], float [[SRC3]], i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
; CHECK:    [[TMP3:%[A-z0-9]*]] = extractelement <4 x float> [[TMP2]], i32 0
; CHECK:    [[TMP4:%[A-z0-9]*]] = extractelement <4 x float> [[TMP2]], i32 1
; CHECK:    [[TMP5:%[A-z0-9]*]] = extractelement <4 x float> [[TMP2]], i32 2
; CHECK:    [[TMP6:%[A-z0-9]*]] = fcmp fast one float [[MUL_1]], 0.000000e+00
; CHECK:    [[TMP7:%[A-z0-9]*]] = fcmp fast one float [[MUL_1]], 0.000000e+00
; CHECK:    [[TMP8:%[A-z0-9]*]] = or i1 [[TMP6]], [[TMP7]]
; CHECK:    br i1 [[TMP8]], label %[[TMP9:[A-z0-9]*]], label %[[TMP20:[A-z0-9]*]]
; CHECK:  [[TMP9]]:
; CHECK:    [[TMP10:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float [[A]], float [[B]], float [[SRC3]], float 1.000000e+00, float [[SRC2]], float [[SRC3]], i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
; CHECK:    [[TMP11:%[A-z0-9]*]] = extractelement <4 x float> [[TMP10]], i32 0
; CHECK:    [[TMP12:%[A-z0-9]*]] = extractelement <4 x float> [[TMP10]], i32 1
; CHECK:    [[TMP13:%[A-z0-9]*]] = extractelement <4 x float> [[TMP10]], i32 2
; CHECK:    [[TMP14:%[A-z0-9]*]] = fadd float [[TMP3]], [[TMP11]]
; CHECK:    [[TMP15:%[A-z0-9]*]] = fmul float [[TMP14]], 5.000000e-01
; CHECK:    [[TMP16:%[A-z0-9]*]] = fadd float [[TMP4]], [[TMP12]]
; CHECK:    [[TMP17:%[A-z0-9]*]] = fmul float [[TMP16]], 5.000000e-01
; CHECK:    [[TMP18:%[A-z0-9]*]] = fadd float [[TMP5]], [[TMP13]]
; CHECK:    [[TMP19:%[A-z0-9]*]] = fmul float [[TMP18]], 5.000000e-01
; CHECK:    br label %[[TMP20]]
; CHECK:  [[TMP20]]:

; Fast flags are not supported on phi for llvm9
; CHECK-PRE-LLVM-14:    [[TMP21:%[A-z0-9]*]] = phi{{.*}} float [ [[TMP15]], %[[TMP9]] ], [ [[TMP3]], [[TMP0:%[A-z0-9]*]] ]
; CHECK-PRE-LLVM-14:    [[TMP22:%[A-z0-9]*]] = phi{{.*}} float [ [[TMP17]], %[[TMP9]] ], [ [[TMP4]], [[TMP0]] ]
; CHECK-PRE-LLVM-14:    [[TMP23:%[A-z0-9]*]] = phi{{.*}} float [ [[TMP19]], %[[TMP9]] ], [ [[TMP5]], [[TMP0]] ]
; CHECK:    [[TMP21:%[A-z0-9]*]] = phi fast float [ [[TMP15]], %[[TMP9]] ], [ [[TMP3]], [[TMP0:%[A-z0-9]*]] ]
; CHECK:    [[TMP22:%[A-z0-9]*]] = phi fast float [ [[TMP17]], %[[TMP9]] ], [ [[TMP4]], [[TMP0]] ]
; CHECK:    [[TMP23:%[A-z0-9]*]] = phi fast float [ [[TMP19]], %[[TMP9]] ], [ [[TMP5]], [[TMP0]] ]
; CHECK:    call void @llvm.genx.GenISA.OUTPUT.f32(float [[TMP21]], float [[TMP22]], float [[TMP23]], float 0.000000e+00, i32 1, i32 1, i32 15)
; CHECK:    ret void
;
  %mul = fmul float %src1, %src2
  %a = fsub float %src1, %mul
  %b = fadd float %src1, %mul
  %1 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %src1, float %src2, float 0.000000e+00, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
  %2 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %b, float %a, float %src3, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
  %3 = extractelement <4 x float> %2, i32 0
  %4 = extractelement <4 x float> %2, i32 1
  %5 = extractelement <4 x float> %2, i32 2
  %6 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %a, float %b, float %src3, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0)
  %7 = extractelement <4 x float> %6, i32 0
  %8 = extractelement <4 x float> %6, i32 1
  %9 = extractelement <4 x float> %6, i32 2
  %10 = fadd float %3, %7
  %11 = fmul float %10, 5.000000e-01
  %12 = fadd float %4, %8
  %13 = fmul float %12, 5.000000e-01
  %14 = fadd float %5, %9
  %15 = fmul float %14, 5.000000e-01
  call void @llvm.genx.GenISA.OUTPUT.f32(float %11, float %13, float %15, float 0.000000e+00, i32 1, i32 1, i32 15)
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float, float, float, float, float, float, i8 addrspace(196609)*, i8 addrspace(524293)*, i32, i32, i32)
declare void @llvm.genx.GenISA.OUTPUT.f32(float, float, float, float, i32, i32, i32)

