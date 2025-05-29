;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test CreateNewMergeValue in case we merge vector to a bigger vector
; RUN: igc_opt --typed-pointers %s -S -o - --basic-aa -igc-memopt | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @f0(i32* %src) {
entry:
  %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %arrayidx1.p0v2i32 = bitcast i32* %arrayidx1 to <2 x i32>*
  %1 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p0v2i32.v2i32(<2 x i32>* %arrayidx1.p0v2i32, i64 4, i1 true, <2 x i32> <i32 43, i32 44>)
  ret void
}

 ; CHECK-LABEL: define void @f0
 ; CHECK: %0 = bitcast i32* %src to <3 x i32>*
 ; CHECK: %1 = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p0v3i32.v3i32(<3 x i32>* %0, i64 4, i1 true, <3 x i32> <i32 42, i32 43, i32 44>)
 ; CHECK: ret void

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32*, i64, i1, i32) #0
declare <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p0v2i32.v2i32(<2 x i32>*, i64, i1, <2 x i32>) #0

attributes #0 = { nounwind readonly }

!igc.functions = !{!0}

!0 = !{void (i32*)* @f0, !10}

!10 = !{!20}
!20 = !{!"function_type", i32 0}
