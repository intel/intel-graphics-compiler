;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -S --basic-aa -platformpvc -igc-memopt --regkey=MemOptGEPCanon=1 %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_func i8 @sobel_grayscale(i8 addrspace(1)* %src, i32 %arg0, i32 %arg1, i32 %width) {
entry:
  %sub20 = add nsw i32 %arg0, -1
  %sub21 = add nsw i32 %arg1, -1
  %mul22 = mul nsw i32 %sub21, %width
  %add23 = add nsw i32 %sub20, %mul22
  %idxprom24 = sext i32 %add23 to i64
  %arrayidx25 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 %idxprom24
  %v0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %arrayidx25, i64 1, i1 true, i8 1)
  %add28 = add nsw i32 %arg0, %mul22
  %idxprom29 = sext i32 %add28 to i64
  %arrayidx30 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 %idxprom29
  %v1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %arrayidx30, i64 1, i1 true, i8 2)
  %sum = add i8 %v0, %v1
  ret i8 %sum
}

; Only one load after merging two i8 load!

; CHECK-LABEL: define spir_func i8 @sobel_grayscale
; CHECK:       <2 x i8> @llvm.genx.GenISA.PredicatedLoad.{{.*}}(ptr addrspace(1) {{.*}}, i64 1, i1 true, <2 x i8> <i8 1, i8 2>)
; CHECK-NOT:   PredicatedLoad
; CHECK:       ret

define spir_func i32 @luxmark31_noise([512 x i32] addrspace(2)* %src, float %arg0, i32 %arg1) {
entry:
  %conv = fptosi float %arg0 to i32
  %and = and i32 %conv, 255
  %add.i1 = add nsw i32 %arg1, %and
  %idxprom.i1 = zext i32 %add.i1 to i64
  %arrayidx.i1 = getelementptr inbounds [512 x i32], [512 x i32] addrspace(2)* %src, i64 0, i64 %idxprom.i1
  %val0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p2i32.i32(i32 addrspace(2)* %arrayidx.i1, i64 4, i1 true, i32 42)
  %add.i2.1 = add nuw nsw i32 %and, 1
  %add.i2 = add nuw nsw i32 %arg1, %add.i2.1
  %idxprom.i2 = zext i32 %add.i2 to i64
  %arrayidx.i2 = getelementptr inbounds [512 x i32], [512 x i32] addrspace(2)* %src, i64 0, i64 %idxprom.i2
  %val1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p2i32.i32(i32 addrspace(2)* %arrayidx.i2, i64 4, i1 true, i32 43)
  %total = add i32 %val0, %val1
  ret i32 %total
}

; CHECK-LABEL: define spir_func i32 @luxmark31_noise
; CHECK:       <2 x i32> @llvm.genx.GenISA.PredicatedLoad.{{.*}}(ptr addrspace(2) {{.*}}, i64 4, i1 true, <2 x i32> <i32 42, i32 43>)
; CHECK-NOT:   PredicatedLoad
; CHECK:       ret

; Test no merges, if could not get offset due to differnt load types.
define spir_func i8 @diff_types(i8 addrspace(1)* %src, i32 %arg0, i32 %arg1, i32 %width) {
entry:
  %sub20 = add nsw i32 %arg0, -1
  %sub21 = add nsw i32 %arg1, -1
  %mul22 = mul nsw i32 %sub21, %width
  %add23 = add nsw i32 %sub20, %mul22
  %idxprom24 = sext i32 %add23 to i64
  %arrayidx25 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 %idxprom24
  %v0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %arrayidx25, i64 1, i1 true, i8 1)
  %add28 = add nsw i32 %arg0, %mul22
  %idxprom29 = sext i32 %add28 to i64
  %arrayidx30 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 %idxprom29
  %arrayidx30.1 = bitcast i8 addrspace(1)* %arrayidx30 to <2 x i8> addrspace(1)*
  %v1 = call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)* %arrayidx30.1, i64 1, i1 true, <2 x i8> <i8 2, i8 3>)
  %v1.1 = extractelement <2 x i8> %v1, i32 0
  %v1.2 = extractelement <2 x i8> %v1, i32 1
  %sum = add i8 %v0, %v1.1
  %sum.1 = add i8 %sum, %v1.2
  ret i8 %sum.1
}

; CHECK-LABEL: define spir_func i8 @diff_types
; CHECK:       i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(ptr addrspace(1) %arrayidx25, i64 1, i1 true, i8 1)
; CHECK:       <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(ptr addrspace(1) %arrayidx30.1, i64 1, i1 true, <2 x i8> <i8 2, i8 3>)
; CHECK-NOT:   PredicatedLoad
; CHECK:       ret

; Function Attrs: nounwind readonly
declare spir_func i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)*, i64, i1, i8) #0
declare spir_func i32 @llvm.genx.GenISA.PredicatedLoad.i32.p2i32.i32(i32 addrspace(2)*, i64, i1, i32) #0
declare spir_func <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)*, i64, i1, <2 x i8>) #0

attributes #0 = { nounwind readonly }

!igc.functions = !{!0, !1, !2}

!0 = !{i8 (i8 addrspace(1)*, i32, i32, i32)* @sobel_grayscale, !10}
!1 = !{i32 ([512 x i32] addrspace(2)*, float, i32)* @luxmark31_noise, !10}
!2 = !{i8 (i8 addrspace(1)*, i32, i32, i32)* @diff_types, !10}

!10 = !{!20}
!20 = !{!"function_type", i32 0}
