;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 -dce < %s 2>&1 | FileCheck %s

; CHECK-NOT: shufflevector

; Function Attrs: convergent nounwind
define spir_kernel void @foo(<8 x double> %in1, <8 x double> %in2, <8 x double> %in3, <8 x double> %in4) #0 {
cond-add:
  br label %._crit_edge4000

._crit_edge4000:                                  ; preds = %._crit_edge4000, %cond-add
  %vectorized_intrinsic0 = call <8 x double> @llvm.exp2.v8f32(<8 x double> %in1)
  %vectorized_intrinsic1 = call <8 x double> @llvm.exp2.v8f32(<8 x double> %in2)

  %vectorized_intrinsic2 = call <8 x double> @llvm.exp2.v8f32(<8 x double> %in3)
  %vectorized_intrinsic3 = call <8 x double> @llvm.exp2.v8f32(<8 x double> %in3)
  br label %end

end:                                      ; No predecessors!
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x double> %vectorized_intrinsic0)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x double> %vectorized_intrinsic1)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x double> %vectorized_intrinsic2)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x double> %vectorized_intrinsic3)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x double>)

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <8 x double> @llvm.exp2.v8f32(<8 x double>) #1

attributes #0 = { convergent nounwind }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2, !29}
!2 = !{!"function_type", i32 0}
!29 = !{!"sub_group_size", i32 16}
