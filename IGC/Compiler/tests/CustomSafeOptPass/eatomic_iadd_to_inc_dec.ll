;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --platformdg2 -igc-custom-safe-opt -S < %s --dce | FileCheck %s

target datalayout = "e-p:64:64:64-p3:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

%__Buffer_Typed_DIM_Resource = type opaque
%"class.RWBuffer<unsigned int>" = type { i32 }
%"class.RWStructuredBuffer<xs>" = type { %struct.xs }
%struct.xs = type { i32 }

@"\01?outputg@@3PAIA" = external addrspace(3) global [32 x i32], align 4
@ThreadGroupSize_X = constant i32 32
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; -------------------------------------------------------------
; CustomSafeOptPass: EATOMIC_IADD to EATOMIC_INC or EATOMIC_DEC
; -------------------------------------------------------------
define void @main(<8 x i32> %r0) {
; CHECK-LABEL: @main(
; CHECK-NEXT:  [[TMP1:%.*]] = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
; CHECK-NEXT:  [[GROUPX:%.*]] = bitcast float [[TMP1]] to i32
; CHECK-NEXT:  [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 0)
; CHECK-NEXT:  [[TMP3:%.*]] = add i32 [[TMP2]], 64
; CHECK-NEXT:  [[TMP4:%.*]] = zext i32 [[TMP3]] to i64
; CHECK-NEXT:  [[U0:%.*]] = inttoptr i64 [[TMP4]] to ptr addrspace(2490369)
; CHECK-NEXT:  [[TMP5:%.*]] = zext i32 [[TMP2]] to i64
; CHECK-NEXT:  [[U01:%.*]] = inttoptr i64 [[TMP5]] to ptr addrspace(2490368)
; CHECK-NEXT:  [[TMP6:%.*]] = shl i32 [[GROUPX]], 5
; CHECK-NEXT:  [[LOCALIDX:%.*]] = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
; CHECK-NEXT:  [[THREADIDX:%.*]] = add i32 [[TMP6]], [[LOCALIDX]]
; CHECK-NEXT:  [[TMP7:%.*]] = call i32 @llvm.genx.GenISA.intatomictyped.i32.p2490368__Buffer_Typed_DIM_Resource(ptr addrspace(2490368) [[U01]], i32 [[THREADIDX]], i32 undef, i32 undef, i32 poison, i32 2)
; CHECK-NEXT:  [[TMP8:%.*]] = call i32 @llvm.genx.GenISA.intatomictyped.i32.p2490368__Buffer_Typed_DIM_Resource(ptr addrspace(2490368) [[U01]], i32 [[THREADIDX]], i32 undef, i32 undef, i32 poison, i32 3)
; CHECK-NEXT:  [[TMP9:%.*]] = shl i32 [[THREADIDX]], 2
; CHECK-NEXT:  [[TMP10:%.*]] = call i32 @llvm.genx.GenISA.intatomicraw.i32.p2490369v4f32(ptr addrspace(2490369) [[U0]], i32 [[TMP9]], i32 poison, i32 2)
; CHECK-NEXT:  [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.intatomicraw.i32.p2490369v4f32(ptr addrspace(2490369) [[U0]], i32 [[TMP9]], i32 poison, i32 3)
; CHECK-NEXT:  [[TMP12:%.*]] = getelementptr [32 x i32], ptr addrspace(3) null, i32 0, i32 [[THREADIDX]]
; CHECK-NEXT:  [[TMP13:%.*]] = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(ptr addrspace(3) [[TMP12]], ptr addrspace(3) [[TMP12]], i32 1, i32 0)
; CHECK-NEXT:  [[TMP14:%.*]] = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(ptr addrspace(3) [[TMP12]], ptr addrspace(3) [[TMP12]], i32 -1, i32 0)
; CHECK-NEXT:  [[TMP15:%.*]] = load i32, ptr addrspace(3) [[TMP12]], align 4, !tbaa !18
; CHECK-NEXT:  [[TMP16:%.*]] = insertelement <1 x i32> undef, i32 [[TMP15]], i64 0
; CHECK-NEXT:  call void @llvm.genx.GenISA.storerawvector.indexed.p2490369v4f32.v1i32(ptr addrspace(2490369) [[U0]], i32 0, <1 x i32> [[TMP16]], i32 4, i1 false)
; CHECK-NEXT:  ret void
;
  %1 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %1 to i32
  %2 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 0)
  %3 = add i32 %2, 64
  %4 = zext i32 %3 to i64
  %u0 = inttoptr i64 %4 to <4 x float> addrspace(2490369)*
  %5 = zext i32 %2 to i64
  %u01 = inttoptr i64 %5 to %__Buffer_Typed_DIM_Resource addrspace(2490368)*
  %6 = shl i32 %GroupID_X, 5
  %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ThreadID_X = add i32 %6, %LocalID_X
  %7 = call i32 @llvm.genx.GenISA.intatomictyped.i32.p2490368__Buffer_Typed_DIM_Resource(%__Buffer_Typed_DIM_Resource addrspace(2490368)* %u01, i32 %ThreadID_X, i32 undef, i32 undef, i32 1, i32 0)
  %8 = call i32 @llvm.genx.GenISA.intatomictyped.i32.p2490368__Buffer_Typed_DIM_Resource(%__Buffer_Typed_DIM_Resource addrspace(2490368)* %u01, i32 %ThreadID_X, i32 undef, i32 undef, i32 -1, i32 0)
  %9 = shl i32 %ThreadID_X, 2
  %10 = call i32 @llvm.genx.GenISA.intatomicraw.i32.p2490369v4f32(<4 x float> addrspace(2490369)* %u0, i32 %9, i32 1, i32 0)
  %11 = call i32 @llvm.genx.GenISA.intatomicraw.i32.p2490369v4f32(<4 x float> addrspace(2490369)* %u0, i32 %9, i32 -1, i32 0)
  %12 = getelementptr [32 x i32], [32 x i32] addrspace(3)* null, i32 0, i32 %ThreadID_X
  %13 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)* %12, i32 addrspace(3)* %12, i32 1, i32 0)
  %14 = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)* %12, i32 addrspace(3)* %12, i32 -1, i32 0)
  %15 = load i32, i32 addrspace(3)* %12, align 4, !tbaa !18
  %16 = insertelement <1 x i32> undef, i32 %15, i64 0
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490369v4f32.v1i32(<4 x float> addrspace(2490369)* %u0, i32 0, <1 x i32> %16, i32 4, i1 false)
  ret void
}

declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32)
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)*, i32 addrspace(3)*, i32, i32)
declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32)
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32)
declare i32 @llvm.genx.GenISA.intatomictyped.i32.p2490368__Buffer_Typed_DIM_Resource(%__Buffer_Typed_DIM_Resource addrspace(2490368)*, i32, i32, i32, i32, i32)
declare i32 @llvm.genx.GenISA.intatomicraw.i32.p2490369v4f32(<4 x float> addrspace(2490369)*, i32, i32, i32)
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490369v4f32.v1i32(<4 x float> addrspace(2490369)*, i32, <1 x i32>, i32, i1)

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!2}
!dx.shaderModel = !{!3}
!dx.resources = !{!4}
!dx.entryPoints = !{!10}
!igc.functions = !{!13}

!0 = !{!"dxcoob 1.8.2502.11 (239921522)"}
!1 = !{i32 1, i32 6}
!2 = !{i32 1, i32 8}
!3 = !{!"cs", i32 6, i32 6}
!4 = !{null, !5, null, null}
!5 = !{!6, !8}
!6 = !{i32 0, %"class.RWBuffer<unsigned int>"* undef, !"", i32 0, i32 0, i32 1, i32 10, i1 false, i1 false, i1 false, !7}
!7 = !{i32 0, i32 5}
!8 = !{i32 1, %"class.RWStructuredBuffer<xs>"* undef, !"", i32 0, i32 1, i32 1, i32 12, i1 false, i1 false, i1 false, !9}
!9 = !{i32 1, i32 4}
!10 = distinct !{null, !"main", null, !4, !11}
!11 = !{i32 0, i64 8388624, i32 4, !12}
!12 = !{i32 32, i32 1, i32 1}
!13 = !{void (<8 x i32>)* @main, !14}
!14 = !{!15, !16}
!15 = !{!"function_type", i32 0}
!16 = !{!"implicit_arg_desc", !17}
!17 = !{i32 0}
!18 = !{!19, !19, i64 0}
!19 = !{!"int", !20, i64 0}
!20 = !{!"omnipotent char", !21, i64 0}
!21 = !{!"Simple C/C++ TBAA"}