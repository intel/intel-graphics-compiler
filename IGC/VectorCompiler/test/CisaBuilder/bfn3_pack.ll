;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer -o /dev/null \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa -isaasmToConsole" -mcpu=XeHPC %s \
; RUN: | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer -o /dev/null \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa -isaasmToConsole" -mcpu=XeHPC %s \
; RUN: | FileCheck %s

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

declare i32 @llvm.genx.bfn.i32(i32, i32, i32, i8)
declare <1 x i32> @llvm.genx.oword.ld.v1i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v1i32(i32, i32, <1 x i32>) #0
; CHECK-LABEL: .kernel "test_bfn_1i32"
define dllexport spir_kernel void @test_bfn_1i32(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec1i16 = tail call <1 x i32> @llvm.genx.oword.ld.v1i32(i32 0, i32 %0, i32 0) #2
  %in_scal = bitcast <1 x i32> %vec1i16 to i32
; Constants propagate succsess
; CHECK-1I32: bfn.x0 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0> 0x12345:ud
; CHECK-1I32: bfn.x1 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> 0x12345:ud {{[V0-9]*}}(0,0)<0;1,0>
; CHECK-1I32: bfn.x2 (M1, 1) {{[V0-9]*}}(0,0)<1> 0x12345:ud {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0>
  %res1i32a = call i32 @llvm.genx.bfn.i32(i32 %in_scal, i32 %in_scal, i32 74565, i8 0) #2
  %res1i32b = call i32 @llvm.genx.bfn.i32(i32 %in_scal, i32 74565, i32 %in_scal, i8 1) #2
  %res1i32c = call i32 @llvm.genx.bfn.i32(i32 74565, i32 %in_scal, i32 %in_scal, i8 2) #2

  %out_casta = bitcast i32  %res1i32a to <1 x i32>
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %out_casta) #2
  %out_castb = bitcast i32  %res1i32b to <1 x i32>
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %out_castb) #2
  %out_castc = bitcast i32  %res1i32c to <1 x i32>
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %out_castc) #2
  ret void
}

declare <1 x i16> @llvm.genx.bfn.v1i16(<1 x i16>, <1 x i16>, <1 x i16>, i8)
declare <1 x i16> @llvm.genx.oword.ld.v1i16(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v1i16(i32, i32, <1 x i16>) #0
; CHECK-LABEL: .kernel "test_bfn_v1i16"
define dllexport spir_kernel void @test_bfn_v1i16(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec1i16 = tail call <1 x i16> @llvm.genx.oword.ld.v1i16(i32 0, i32 %0, i32 0) #2
; Constants propagate succsess
; CHECK-V1I16: bfn.x3 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0> 0x1234:uw
; CHECK-V1I16: bfn.x4 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> 0x1234:uw {{[V0-9]*}}(0,0)<0;1,0>
; CHECK-V1I16: bfn.x5 (M1, 1) {{[V0-9]*}}(0,0)<1> 0x1234:uw {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0>
  %res1i16a = call <1 x i16> @llvm.genx.bfn.v1i16(<1 x i16> %vec1i16, <1 x i16> %vec1i16, <1 x i16> <i16 4660>, i8 3) #2
  %res1i16b = call <1 x i16> @llvm.genx.bfn.v1i16(<1 x i16> %vec1i16, <1 x i16> <i16 4660>, <1 x i16> %vec1i16, i8 4) #2
  %res1i16c = call <1 x i16> @llvm.genx.bfn.v1i16(<1 x i16> <i16 4660>, <1 x i16> %vec1i16, <1 x i16> %vec1i16, i8 5) #2

  tail call void @llvm.genx.oword.st.v1i16(i32 %1, i32 0, <1 x i16> %res1i16a) #2
  tail call void @llvm.genx.oword.st.v1i16(i32 %1, i32 0, <1 x i16> %res1i16b) #2
  tail call void @llvm.genx.oword.st.v1i16(i32 %1, i32 0, <1 x i16> %res1i16c) #2
  ret void
}

declare <4 x i16> @llvm.genx.oword.ld.v4i16(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v4i16(i32, i32, <4 x i16>) #0
declare <4 x i16> @llvm.genx.bfn.v4i16(<4 x i16>, <4 x i16>, <4 x i16>, i8)
; CHECK-LABEL: .kernel "test_bfn_v4i16"
define dllexport spir_kernel void @test_bfn_v4i16(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec4i16 = tail call <4 x i16> @llvm.genx.oword.ld.v4i16(i32 0, i32 %0, i32 0) #2
; CHECK-V4I16: mov (M1, 4) {{[V0-9]*}}(0,0)<1> 0x10011001:v                                         /// $2
; CHECK-V4I16: bfn.x6 (M1, 4) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V4I16: bfn.x7 (M1, 4) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V4I16: bfn.x8 (M1, 4) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
  %resi4v16a = call <4 x i16> @llvm.genx.bfn.v4i16(<4 x i16> %vec4i16, <4 x i16> %vec4i16, <4 x i16> <i16 1, i16 0, i16 0, i16 1>, i8 6) #2
  %resi4v16b = call <4 x i16> @llvm.genx.bfn.v4i16(<4 x i16> %vec4i16, <4 x i16> <i16 1, i16 0, i16 0, i16 1>, <4 x i16> %vec4i16, i8 7) #2
  %resi4v16c = call <4 x i16> @llvm.genx.bfn.v4i16(<4 x i16> <i16 1, i16 0, i16 0, i16 1>, <4 x i16> %vec4i16, <4 x i16> %vec4i16, i8 8) #2

  tail call void @llvm.genx.oword.st.v4i16(i32 %1, i32 0, <4 x i16> %resi4v16a) #2
  tail call void @llvm.genx.oword.st.v4i16(i32 %1, i32 0, <4 x i16> %resi4v16b) #2
  tail call void @llvm.genx.oword.st.v4i16(i32 %1, i32 0, <4 x i16> %resi4v16c) #2
  ret void
}

declare <16 x i32> @llvm.genx.oword.ld.v16i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v16i32(i32, i32, <16 x i32>) #0
declare <16 x i32> @llvm.genx.bfn.v16i32(<16 x i32>, <16 x i32>, <16 x i32>, i8)
; CHECK-LABEL: .kernel "test_bfn_v16i32"
define dllexport spir_kernel void @test_bfn_v16i32(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec4i32 = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 %0, i32 0) #2
; CHECK-V16I32: mov (M1, 8) {{[V0-9]*}}(0,0)<1> 0x10001100:v
; CHECK-V16I32: mov (M1, 8) {{[V0-9]*}}(0,8)<1> 0x10011001:v
; CHECK-V16I32: bfn.x9 (M1, 16) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V16I32: bfn.xa (M1, 16) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V16I32: bfn.xb (M1, 16) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
  %resi4v16a = call <16 x i32> @llvm.genx.bfn.v16i32(<16 x i32> %vec4i32, <16 x i32> %vec4i32, <16 x i32> <i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 1>, i8 9) #2
  %resi4v16b = call <16 x i32> @llvm.genx.bfn.v16i32(<16 x i32> %vec4i32, <16 x i32> <i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 1>, <16 x i32> %vec4i32, i8 10) #2
  %resi4v16c = call <16 x i32> @llvm.genx.bfn.v16i32(<16 x i32> <i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 1, i32 1, i32 0, i32 0, i32 1>, <16 x i32> %vec4i32, <16 x i32> %vec4i32, i8 11) #2

  tail call void @llvm.genx.oword.st.v16i32(i32 %1, i32 0, <16 x i32> %resi4v16a) #2
  tail call void @llvm.genx.oword.st.v16i32(i32 %1, i32 0, <16 x i32> %resi4v16b) #2
  tail call void @llvm.genx.oword.st.v16i32(i32 %1, i32 0, <16 x i32> %resi4v16c) #2
  ret void
}

attributes #0 = { "target-cpu"="XeHPC" }
attributes #1 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeHPC" }
attributes #2 = { nounwind }
!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!7, !9, !11, !13}
!genx.kernel.internal = !{!8, !10, !12, !14}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{i32 2, i32 2}
!5 = !{i32 64, i32 68}
!6 = !{!"buffer_t", !"buffer_t"}
!7 = !{void (i32, i32)* @test_bfn_1i32, !"test_bfn_1i32", !4, i32 0, !5, !0, !6, i32 0}
!8 = !{void (i32, i32)* @test_bfn_1i32, null, null, null, null}
!9 = !{void (i32, i32)* @test_bfn_v1i16, !"test_bfn_v1i16", !4, i32 0, !5, !0, !6, i32 0}
!10 = !{void (i32, i32)* @test_bfn_v1i16, null, null, null, null}
!11 = !{void (i32, i32)* @test_bfn_v4i16, !"test_bfn_v4i16", !4, i32 0, !5, !0, !6, i32 0}
!12 = !{void (i32, i32)* @test_bfn_v4i16, null, null, null, null}
!13 = !{void (i32, i32)* @test_bfn_v16i32, !"test_bfn_v16i32", !4, i32 0, !5, !0, !6, i32 0}
!14 = !{void (i32, i32)* @test_bfn_v16i32, null, null, null, null}
