;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 --enable-debugify --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_fptoui_f16(half %a) {
; CHECK-LABEL: @test_fptoui_f16
; CHECK:    [[TMP1:%[A-z0-9]*]] = fptoui half [[A:%[A-z0-9]*]] to i32
; CHECK:    [[TMP2:%[A-z0-9]*]] = insertelement <2 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9]*]] = insertelement <2 x i32> [[TMP2]], i32 0, i32 1
; CHECK:    [[TMP4:%[A-z0-9]*]] = bitcast <2 x i32> [[TMP3]] to i64
; CHECK:    call void @use.i64(i64 [[TMP4]])
; CHECK:    ret void
;
  %1 = fptoui half %a to i64
  call void @use.i64(i64 %1)
  ret void
}

define void @test_fptoui_f32(float %a) {
; CHECK-LABEL: @test_fptoui_f32
; CHECK:    [[TMP1:%[A-z0-9]*]] = fmul float [[A:%[A-z0-9]*]], 0x3DF0000000000000
; CHECK:    [[TMP2:%[A-z0-9]*]] = call float @llvm.trunc.f32(float [[TMP1]])
; CHECK:    [[TMP3:%[A-z0-9]*]] = fptoui float [[TMP2]] to i32
; CHECK:    [[TMP4:%[A-z0-9]*]] = call float @llvm.fma.f32(float [[TMP2]], float 0xC1F0000000000000, float [[A]])
; CHECK:    [[TMP5:%[A-z0-9]*]] = fptoui float [[TMP4]] to i32
; CHECK:    [[TMP6:%[A-z0-9]*]] = insertelement <2 x i32> undef, i32 [[TMP5]], i32 0
; CHECK:    [[TMP7:%[A-z0-9]*]] = insertelement <2 x i32> [[TMP6]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP8:%[A-z0-9]*]] = bitcast <2 x i32> [[TMP7]] to i64
; CHECK:    call void @use.i64(i64 [[TMP8]])
; CHECK:    ret void
;
  %1 = fptoui float %a to i64
  call void @use.i64(i64 %1)
  ret void
}

define void @test_fptosi_f16(half %a) {
; CHECK-LABEL: @test_fptosi_f16
; CHECK:    [[TMP1:%[A-z0-9]*]] = fptosi half [[A:%[A-z0-9]*]] to i32
; CHECK:    [[TMP2:%[A-z0-9]*]] = ashr i32 [[TMP1]], 31
; CHECK:    [[TMP3:%[A-z0-9]*]] = insertelement <2 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP4:%[A-z0-9]*]] = insertelement <2 x i32> [[TMP3]], i32 [[TMP2]], i32 1
; CHECK:    [[TMP5:%[A-z0-9]*]] = bitcast <2 x i32> [[TMP4]] to i64
; CHECK:    call void @use.i64(i64 [[TMP5]])
; CHECK:    ret void
;
  %1 = fptosi half %a to i64
  call void @use.i64(i64 %1)
  ret void
}

define void @test_fptosi_f64(double %a) {
; CHECK-LABEL: @test_fptosi_f64
; CHECK:    [[TMP1:%[A-z0-9]*]] = bitcast double [[A:%[A-z0-9]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP3:%[A-z0-9]*]] = ashr i32 [[TMP2]], 31
; CHECK:    [[TMP4:%[A-z0-9]*]] = call double @llvm.fabs.f64(double [[A]])
; CHECK:    [[TMP5:%[A-z0-9]*]] = fmul double [[TMP4]], 0x3DF0000000000000
; CHECK:    [[TMP6:%[A-z0-9]*]] = fptoui double [[TMP5]] to i32
; CHECK:    [[TMP7:%[A-z0-9]*]] = uitofp i32 [[TMP6]] to double
; CHECK:    [[TMP8:%[A-z0-9]*]] = call double @llvm.fma.f64(double [[TMP7]], double 0xC1F0000000000000, double [[TMP4]])
; CHECK:    [[TMP9:%[A-z0-9]*]] = fptoui double [[TMP8]] to i32
; CHECK:    [[TMP10:%[A-z0-9]*]] = xor i32 [[TMP9]], [[TMP3]]
; CHECK:    [[TMP11:%[A-z0-9]*]] = xor i32 [[TMP6]], [[TMP3]]
; CHECK:    [[TMP12:%[A-z0-9]*]] = call { i32, i32 } @llvm.genx.GenISA.sub.pair(i32 [[TMP10]], i32 [[TMP11]], i32 [[TMP3]], i32 [[TMP3]])
; CHECK:    [[TMP13:%[A-z0-9]*]] = extractvalue { i32, i32 } [[TMP12]], 0
; CHECK:    [[TMP14:%[A-z0-9]*]] = extractvalue { i32, i32 } [[TMP12]], 1
; CHECK:    [[TMP15:%[A-z0-9]*]] = insertelement <2 x i32> undef, i32 [[TMP13]], i32 0
; CHECK:    [[TMP16:%[A-z0-9]*]] = insertelement <2 x i32> [[TMP15]], i32 [[TMP14]], i32 1
; CHECK:    [[TMP17:%[A-z0-9]*]] = bitcast <2 x i32> [[TMP16]] to i64
; CHECK:    call void @use.i64(i64 [[TMP17]])
; CHECK:    ret void
;
  %1 = fptosi double %a to i64
  call void @use.i64(i64 %1)
  ret void
}

define void @test_uitofp(i64 %a) {
; CHECK-LABEL: @test_uitofp(
; CHECK:    [[TMP1:%[A-z0-9]*]] = bitcast i64 [[A:%[A-z0-9]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9]*]] = call i32 @llvm.ctlz.i32(i32 [[TMP3]], i1 false)
; CHECK:    [[TMP5:%[A-z0-9]*]] = icmp ne i32 [[TMP4]], 32
; CHECK:    br i1 [[TMP5]], label [[DOTU2F_OUTER_TRUE_BRANCH:%[A-z0-9.]*]], label [[TMP16:%[A-z0-9]*]]
; CHECK: .u2f.outer.true.branch:
; CHECK:    [[TMP6:%[A-z0-9]*]] = icmp ne i32 [[TMP4]], 0
; CHECK:    br i1 [[TMP6]], label [[DOTU2F_INNER_TRUE_BRANCH:%[A-z0-9.]*]], label [[TMP12:%[A-z0-9]*]]
; CHECK: .u2f.inner.true.branch:
; CHECK:    [[TMP7:%[A-z0-9]*]] = shl i32 [[TMP2]], [[TMP4]]
; CHECK:    [[TMP8:%[A-z0-9]*]] = shl i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP9:%[A-z0-9]*]] = sub i32 0, [[TMP4]]
; CHECK:    [[TMP10:%[A-z0-9]*]] = lshr i32 [[TMP2]], [[TMP9]]
; CHECK:    [[TMP11:%[A-z0-9]*]] = or i32 [[TMP8]], [[TMP10]]
; CHECK:    br label [[TMP12]]
; CHECK: 12:
; CHECK:    [[DOTU2F_INNER_MERGE_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP3]], [[DOTU2F_OUTER_TRUE_BRANCH]] ], [ [[TMP11]], [[DOTU2F_INNER_TRUE_BRANCH]] ]
; CHECK:    [[DOTU2F_INNER_MERGE_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP2]], [[DOTU2F_OUTER_TRUE_BRANCH]] ], [ [[TMP7]], [[DOTU2F_INNER_TRUE_BRANCH]] ]
; CHECK:    [[TMP13:%[A-z0-9]*]] = icmp ne i32 [[DOTU2F_INNER_MERGE_LO]], 0
; CHECK:    br i1 [[TMP13]], label [[DOTU2F_ROUNDING_BRANCH:%[A-z0-9.]*]], label [[TMP15:%[A-z0-9]*]]
; CHECK: .u2f.rounding.branch:
; CHECK:    [[TMP14:%[A-z0-9]*]] = or i32 [[DOTU2F_INNER_MERGE_HI]], 1
; CHECK:    br label [[TMP15]]
; CHECK: 15:
; CHECK:    [[DOTU2F_ROUNDING_MERGE_HI:%[A-z0-9.]*]] = phi i32 [ [[DOTU2F_INNER_MERGE_HI]], [[TMP12]] ], [ [[TMP14]], [[DOTU2F_ROUNDING_BRANCH]] ]
; CHECK:    br label [[TMP16]]
; CHECK: 16:
; CHECK:    [[DOTU2F_OUTER_MERGE:%[A-z0-9.]*]] = phi i32 [ [[TMP2]], [[TMP0:%[A-z0-9]*]] ], [ [[DOTU2F_ROUNDING_MERGE_HI]], [[TMP15]] ]
; CHECK:    [[TMP17:%[A-z0-9]*]] = uitofp i32 [[DOTU2F_OUTER_MERGE]] to float
; CHECK:    [[TMP18:%[A-z0-9]*]] = sub i32 159, [[TMP4]]
; CHECK:    [[TMP19:%[A-z0-9]*]] = shl i32 [[TMP18]], 23
; CHECK:    [[TMP20:%[A-z0-9]*]] = bitcast i32 [[TMP19]] to float
; CHECK:    [[TMP21:%[A-z0-9]*]] = fmul float [[TMP17]], [[TMP20]]
; CHECK:    call void @use.f32(float [[TMP21]])
; CHECK:    ret void
;
  %1 = uitofp i64 %a to float
  call void @use.f32(float %1)
  ret void
}

define void @test_sitofp(i64 %a) {
; CHECK-LABEL: @test_sitofp(
; CHECK:    [[TMP1:%[A-z0-9]*]] = bitcast i64 [[A:%[A-z0-9]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9]*]] = ashr i32 [[TMP3]], 31
; CHECK:    [[TMP5:%[A-z0-9]*]] = xor i32 [[TMP2]], [[TMP4]]
; CHECK:    [[TMP6:%[A-z0-9]*]] = xor i32 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP7:%[A-z0-9]*]] = call { i32, i32 } @llvm.genx.GenISA.sub.pair(i32 [[TMP5]], i32 [[TMP6]], i32 [[TMP4]], i32 [[TMP4]])
; CHECK:    [[TMP8:%[A-z0-9]*]] = extractvalue { i32, i32 } [[TMP7]], 0
; CHECK:    [[TMP9:%[A-z0-9]*]] = extractvalue { i32, i32 } [[TMP7]], 1
; CHECK:    [[TMP10:%[A-z0-9]*]] = call i32 @llvm.ctlz.i32(i32 [[TMP9]], i1 false)
; CHECK:    [[TMP11:%[A-z0-9]*]] = icmp ne i32 [[TMP10]], 32
; CHECK:    br i1 [[TMP11]], label [[DOTU2F_OUTER_TRUE_BRANCH:%[A-z0-9.]*]], label [[TMP22:%[A-z0-9]*]]
; CHECK: .u2f.outer.true.branch:
; CHECK:    [[TMP12:%[A-z0-9]*]] = icmp ne i32 [[TMP10]], 0
; CHECK:    br i1 [[TMP12]], label [[DOTU2F_INNER_TRUE_BRANCH:%[A-z0-9.]*]], label [[TMP18:%[A-z0-9]*]]
; CHECK: .u2f.inner.true.branch:
; CHECK:    [[TMP13:%[A-z0-9]*]] = shl i32 [[TMP8]], [[TMP10]]
; CHECK:    [[TMP14:%[A-z0-9]*]] = shl i32 [[TMP9]], [[TMP10]]
; CHECK:    [[TMP15:%[A-z0-9]*]] = sub i32 0, [[TMP10]]
; CHECK:    [[TMP16:%[A-z0-9]*]] = lshr i32 [[TMP8]], [[TMP15]]
; CHECK:    [[TMP17:%[A-z0-9]*]] = or i32 [[TMP14]], [[TMP16]]
; CHECK:    br label [[TMP18]]
; CHECK: 18:
; CHECK:    [[DOTU2F_INNER_MERGE_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP9]], [[DOTU2F_OUTER_TRUE_BRANCH]] ], [ [[TMP17]], [[DOTU2F_INNER_TRUE_BRANCH]] ]
; CHECK:    [[DOTU2F_INNER_MERGE_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP8]], [[DOTU2F_OUTER_TRUE_BRANCH]] ], [ [[TMP13]], [[DOTU2F_INNER_TRUE_BRANCH]] ]
; CHECK:    [[TMP19:%[A-z0-9]*]] = icmp ne i32 [[DOTU2F_INNER_MERGE_LO]], 0
; CHECK:    br i1 [[TMP19]], label [[DOTU2F_ROUNDING_BRANCH:%[A-z0-9.]*]], label [[TMP21:%[A-z0-9]*]]
; CHECK: .u2f.rounding.branch:
; CHECK:    [[TMP20:%[A-z0-9]*]] = or i32 [[DOTU2F_INNER_MERGE_HI]], 1
; CHECK:    br label [[TMP21]]
; CHECK: 21:
; CHECK:    [[DOTU2F_ROUNDING_MERGE_HI:%[A-z0-9.]*]] = phi i32 [ [[DOTU2F_INNER_MERGE_HI]], [[TMP18]] ], [ [[TMP20]], [[DOTU2F_ROUNDING_BRANCH]] ]
; CHECK:    br label [[TMP22]]
; CHECK: 22:
; CHECK:    [[DOTU2F_OUTER_MERGE:%[A-z0-9.]*]] = phi i32 [ [[TMP8]], [[TMP0:%[A-z0-9]*]] ], [ [[DOTU2F_ROUNDING_MERGE_HI]], [[TMP21]] ]
; CHECK:    [[TMP23:%[A-z0-9]*]] = uitofp i32 [[DOTU2F_OUTER_MERGE]] to float
; CHECK:    [[TMP24:%[A-z0-9]*]] = sub i32 159, [[TMP10]]
; CHECK:    [[TMP25:%[A-z0-9]*]] = shl i32 [[TMP24]], 23
; CHECK:    [[TMP26:%[A-z0-9]*]] = bitcast i32 [[TMP25]] to float
; CHECK:    [[TMP27:%[A-z0-9]*]] = fmul float [[TMP23]], [[TMP26]]
; CHECK:    [[TMP28:%[A-z0-9]*]] = bitcast float [[TMP27]] to i32
; CHECK:    [[TMP29:%[A-z0-9]*]] = and i32 [[TMP4]], -2147483648
; CHECK:    [[TMP30:%[A-z0-9]*]] = or i32 [[TMP28]], [[TMP29]]
; CHECK:    [[TMP31:%[A-z0-9]*]] = bitcast i32 [[TMP30]] to float
; CHECK:    call void @use.f32(float [[TMP31]])
; CHECK:    ret void
;
  %1 = sitofp i64 %a to float
  call void @use.f32(float %1)
  ret void
}

declare void @use.i64(i64)
declare void @use.f32(float)


!igc.functions = !{!0, !3, !4, !5, !6, !7}

!0 = !{void (half)* @test_fptoui_f16, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (float)* @test_fptoui_f32, !1}
!4 = !{void (half)* @test_fptosi_f16, !1}
!5 = !{void (double)* @test_fptosi_f64, !1}
!6 = !{void (i64)* @test_uitofp, !1}
!7 = !{void (i64)* @test_sitofp, !1}
