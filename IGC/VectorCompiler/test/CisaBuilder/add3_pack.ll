;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer -o /dev/null \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa" -mcpu=XeHPC %s
; RUN: cat < test_add3_v1i16.visaasm | FileCheck --check-prefix=CHECK-V1I16 %s
; RUN: cat < test_add3_v2i16.visaasm | FileCheck --check-prefix=CHECK-V2I16 %s
; RUN: cat < test_add3_v8i16.visaasm | FileCheck --check-prefix=CHECK-V8I16 %s
; RUN: cat < test_add3_v1i32.visaasm | FileCheck --check-prefix=CHECK-V1I32 %s
; RUN: cat < test_add3_v2i32.visaasm | FileCheck --check-prefix=CHECK-V2I32 %s
; RUN: cat < test_add3_v8i32.visaasm | FileCheck --check-prefix=CHECK-V8I32 %s
; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

declare <1 x i16> @llvm.genx.oword.ld.v1i16(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v1i16(i32, i32, <1 x i16>) #0
declare <1 x i16> @llvm.genx.add3.v1i16.v1i16(<1 x i16>, <1 x i16>, <1 x i16>)

; CHECK-V1I16-LABEL: test_add3_v1i16_BB_0
define dllexport spir_kernel void @test_add3_v1i16(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec1i16 = tail call <1 x i16> @llvm.genx.oword.ld.v1i16(i32 0, i32 %0, i32 0) #2
; Constants propagate succsess
; CHECK-V1I16:  add3 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0> 0x4d2:w
; CHECK-V1I16:  add3 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> 0x4d2:w {{[V0-9]*}}(0,0)<0;1,0>
; CHECK-V1I16:  add3 (M1, 1) {{[V0-9]*}}(0,0)<1> 0x4d2:w {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0>
  %res1i16a = call <1 x i16> @llvm.genx.add3.v1i16.v1i16(<1 x i16> %vec1i16, <1 x i16> %vec1i16, <1 x i16> <i16 1234>) #2
  tail call void @llvm.genx.oword.st.v1i16(i32 %1, i32 0, <1 x i16> %res1i16a) #2
  %res1i16b = call <1 x i16> @llvm.genx.add3.v1i16.v1i16(<1 x i16> %vec1i16, <1 x i16> <i16 1234>, <1 x i16> %vec1i16) #2
  tail call void @llvm.genx.oword.st.v1i16(i32 %1, i32 0, <1 x i16> %res1i16b) #2
  %res1i16c = call <1 x i16> @llvm.genx.add3.v1i16.v1i16(<1 x i16> <i16 1234>, <1 x i16> %vec1i16, <1 x i16> %vec1i16) #2
  tail call void @llvm.genx.oword.st.v1i16(i32 %1, i32 0, <1 x i16> %res1i16c) #2
  ret void
}

declare <2 x i16> @llvm.genx.oword.ld.v2i16(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v2i16(i32, i32, <2 x i16>) #0
declare <2 x i16> @llvm.genx.add3.v2i16.v2i16(<2 x i16>, <2 x i16>, <2 x i16>)

; CHECK-V2I16-LABEL: test_add3_v2i16_BB_0
define dllexport spir_kernel void @test_add3_v2i16(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec2i16 = tail call <2 x i16> @llvm.genx.oword.ld.v2i16(i32 0, i32 %0, i32 0) #2
; CHECK-V2I16:   mov (M1, 2) {{[V0-9]*}}(0,0)<1> 0x10101010:v
; CHECK-V2I16:   add3 (M1, 2) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V2I16:   add3 (M1, 2) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V2I16:   add3 (M1, 2) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
  %res2i16a = call <2 x i16> @llvm.genx.add3.v2i16.v2i16(<2 x i16> %vec2i16, <2 x i16> %vec2i16, <2 x i16> <i16 0, i16 1>) #2
  tail call void @llvm.genx.oword.st.v2i16(i32 %1, i32 0, <2 x i16> %res2i16a) #2
  %res2i16b = call <2 x i16> @llvm.genx.add3.v2i16.v2i16(<2 x i16> %vec2i16, <2 x i16> <i16 0, i16 1>, <2 x i16> %vec2i16) #2
  tail call void @llvm.genx.oword.st.v2i16(i32 %1, i32 0, <2 x i16> %res2i16b) #2
  %res2i16c = call <2 x i16> @llvm.genx.add3.v2i16.v2i16(<2 x i16> <i16 0, i16 1>, <2 x i16> %vec2i16, <2 x i16> %vec2i16) #2
  tail call void @llvm.genx.oword.st.v2i16(i32 %1, i32 0, <2 x i16> %res2i16c) #2
  ret void
}

declare <8 x i16> @llvm.genx.oword.ld.v8i16(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v8i16(i32, i32, <8 x i16>) #0
declare <8 x i16> @llvm.genx.add3.v8i16.v8i16(<8 x i16>, <8 x i16>, <8 x i16>)

; CHECK-V8I16-LABEL: test_add3_v8i16_BB_0
define dllexport spir_kernel void @test_add3_v8i16(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec8i16 = tail call <8 x i16> @llvm.genx.oword.ld.v8i16(i32 0, i32 %0, i32 0) #2
; CHECK-V8I16:   mov (M1, 8) {{[V0-9]*}}(0,0)<1> 0x10:v
; CHECK-V8I16:   add3 (M1, 8) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V8I16:   add3 (M1, 8) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V8I16:   add3 (M1, 8) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
  %res8i16a = call <8 x i16> @llvm.genx.add3.v8i16.v8i16(<8 x i16> %vec8i16, <8 x i16> %vec8i16, <8 x i16> <i16 0, i16 1, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0>) #2
  tail call void @llvm.genx.oword.st.v8i16(i32 %1, i32 0, <8 x i16> %res8i16a) #2
  %res8i16b = call <8 x i16> @llvm.genx.add3.v8i16.v8i16(<8 x i16> %vec8i16, <8 x i16> <i16 0, i16 1, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0>, <8 x i16> %vec8i16) #2
  tail call void @llvm.genx.oword.st.v8i16(i32 %1, i32 0, <8 x i16> %res8i16b) #2
  %res8i16c = call <8 x i16> @llvm.genx.add3.v8i16.v8i16(<8 x i16> <i16 0, i16 1, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0>, <8 x i16> %vec8i16, <8 x i16> %vec8i16) #2
  tail call void @llvm.genx.oword.st.v8i16(i32 %1, i32 0, <8 x i16> %res8i16c) #2
  ret void
}

declare <1 x i32> @llvm.genx.oword.ld.v1i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v1i32(i32, i32, <1 x i32>) #0
declare <1 x i32> @llvm.genx.add3.v1i32.v1i32(<1 x i32>, <1 x i32>, <1 x i32>)

; CHECK-V1I32-LABEL: test_add3_v1i32_BB_0
define dllexport spir_kernel void @test_add3_v1i32(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec1i32 = tail call <1 x i32> @llvm.genx.oword.ld.v1i32(i32 0, i32 %0, i32 0) #2

; Constants propagate succsess
; CHECK-V1I32:    add3 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0> 0x4d2:d
; CHECK-V1I32:    add3 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> 0x4d2:d {{[V0-9]*}}(0,0)<0;1,0>
; CHECK-V1I32:    add3 (M1, 1) {{[V0-9]*}}(0,0)<1> 0x4d2:d {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0>
  %res1i32a = call <1 x i32> @llvm.genx.add3.v1i32.v1i32(<1 x i32> %vec1i32, <1 x i32> %vec1i32, <1 x i32> <i32 1234>) #2
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %res1i32a) #2
  %res1i32b = call <1 x i32> @llvm.genx.add3.v1i32.v1i32(<1 x i32> %vec1i32, <1 x i32> <i32 1234>, <1 x i32> %vec1i32) #2
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %res1i32b) #2
  %res1i32c = call <1 x i32> @llvm.genx.add3.v1i32.v1i32(<1 x i32> <i32 1234>, <1 x i32> %vec1i32, <1 x i32> %vec1i32) #2
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %res1i32c) #2
  ret void
}

declare <2 x i32> @llvm.genx.oword.ld.v2i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v2i32(i32, i32, <2 x i32>) #0
declare <2 x i32> @llvm.genx.add3.v2i32.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)

; CHECK-V2I32-LABEL: test_add3_v2i32_BB_0
define dllexport spir_kernel void @test_add3_v2i32(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec2i32 = tail call <2 x i32> @llvm.genx.oword.ld.v2i32(i32 0, i32 %0, i32 0) #2
; CHECK-V2I32:    mov (M1, 2) {{[V0-9]*}}(0,0)<1> 0x10101010:v
; CHECK-V2I32:    add3 (M1, 2) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V2I32:    add3 (M1, 2) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V2I32:    add3 (M1, 2) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
  %res2i32a = call <2 x i32> @llvm.genx.add3.v2i32.v2i32(<2 x i32> %vec2i32, <2 x i32> %vec2i32, <2 x i32> <i32 0, i32 1>) #2
  tail call void @llvm.genx.oword.st.v2i32(i32 %1, i32 0, <2 x i32> %res2i32a) #2
  %res2i32b = call <2 x i32> @llvm.genx.add3.v2i32.v2i32(<2 x i32> %vec2i32, <2 x i32> <i32 0, i32 1>, <2 x i32> %vec2i32) #2
  tail call void @llvm.genx.oword.st.v2i32(i32 %1, i32 0, <2 x i32> %res2i32b) #2
  %res2i32c = call <2 x i32> @llvm.genx.add3.v2i32.v2i32(<2 x i32> <i32 0, i32 1>, <2 x i32> %vec2i32, <2 x i32> %vec2i32) #2
  tail call void @llvm.genx.oword.st.v2i32(i32 %1, i32 0, <2 x i32> %res2i32c) #2
  ret void
}

declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>) #0
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)

; CHECK-V8I32-LABEL: test_add3_v8i32_BB_0
define dllexport spir_kernel void @test_add3_v8i32(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec8i32 = tail call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 %0, i32 0) #2
; CHECK-V8I32:    mov (M1, 8) {{[V0-9]*}}(0,0)<1> 0x10:v
; CHECK-V8I32:    add3 (M1, 8) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V8I32:    add3 (M1, 8) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
; CHECK-V8I32:    add3 (M1, 8) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0> {{[V0-9]*}}(0,0)<1;1,0>
  %res8i32a = call <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32> %vec8i32, <8 x i32> %vec8i32, <8 x i32> <i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>) #2
  tail call void @llvm.genx.oword.st.v8i32(i32 %1, i32 0, <8 x i32> %res8i32a) #2
  %res8i32b = call <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32> %vec8i32, <8 x i32> <i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>, <8 x i32> %vec8i32) #2
  tail call void @llvm.genx.oword.st.v8i32(i32 %1, i32 0, <8 x i32> %res8i32b) #2
  %res8i32c = call <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32> <i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>, <8 x i32> %vec8i32, <8 x i32> %vec8i32) #2
  tail call void @llvm.genx.oword.st.v8i32(i32 %1, i32 0, <8 x i32> %res8i32c) #2
  ret void
}

; CHECK-V1I32-2-LABEL: test_add3_v1i32_BB_0
define dllexport spir_kernel void @test_add3_v1i32_2(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec1i32 = tail call <1 x i32> @llvm.genx.oword.ld.v1i32(i32 0, i32 %0, i32 0) #2
; Visa accepts 32-bit imm for 32-bit instructions (will be split up later)
; CHECK-V1I32-2:    add3 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0> 0x1110fffc:d
; CHECK-V1I32-2:    add3 (M1, 1) {{[V0-9]*}}(0,0)<1> {{[V0-9]*}}(0,0)<0;1,0> 0x1110fffc:d {{[V0-9]*}}(0,0)<0;1,0>
; CHECK-V1I32-2:    add3 (M1, 1) {{[V0-9]*}}(0,0)<1> 0x1110fffc:d {{[V0-9]*}}(0,0)<0;1,0> {{[V0-9]*}}(0,0)<0;1,0>
  %res1i32a = call <1 x i32> @llvm.genx.add3.v1i32.v1i32(<1 x i32> %vec1i32, <1 x i32> %vec1i32, <1 x i32> <i32 286326780>) #2
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %res1i32a) #2
  %res1i32b = call <1 x i32> @llvm.genx.add3.v1i32.v1i32(<1 x i32> %vec1i32, <1 x i32> <i32 286326780>, <1 x i32> %vec1i32) #2
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %res1i32b) #2
  %res1i32c = call <1 x i32> @llvm.genx.add3.v1i32.v1i32(<1 x i32> <i32 286326780>, <1 x i32> %vec1i32, <1 x i32> %vec1i32) #2
  tail call void @llvm.genx.oword.st.v1i32(i32 %1, i32 0, <1 x i32> %res1i32c) #2
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
!genx.kernels = !{!7, !9, !11, !13, !15, !17, !19}
!genx.kernel.internal = !{!8, !10, !12, !14, !16, !18, !20}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{i32 2, i32 2}
!5 = !{i32 64, i32 68}
!6 = !{!"buffer_t", !"buffer_t"}
!7 = !{void (i32, i32)* @test_add3_v1i32, !"test_add3_v1i32", !4, i32 0, !5, !0, !6, i32 0}
!8 = !{void (i32, i32)* @test_add3_v1i32, null, null, null, null}
!9 = !{void (i32, i32)* @test_add3_v2i32, !"test_add3_v2i32", !4, i32 0, !5, !0, !6, i32 0}
!10 = !{void (i32, i32)* @test_add3_v2i32, null, null, null, null}
!11 = !{void (i32, i32)* @test_add3_v8i32, !"test_add3_v8i32", !4, i32 0, !5, !0, !6, i32 0}
!12 = !{void (i32, i32)* @test_add3_v8i32, null, null, null, null}
!13 = !{void (i32, i32)* @test_add3_v1i16, !"test_add3_v1i16", !4, i32 0, !5, !0, !6, i32 0}
!14 = !{void (i32, i32)* @test_add3_v1i16, null, null, null, null}
!15 = !{void (i32, i32)* @test_add3_v2i16, !"test_add3_v2i16", !4, i32 0, !5, !0, !6, i32 0}
!16 = !{void (i32, i32)* @test_add3_v2i16, null, null, null, null}
!17 = !{void (i32, i32)* @test_add3_v8i16, !"test_add3_v8i16", !4, i32 0, !5, !0, !6, i32 0}
!18 = !{void (i32, i32)* @test_add3_v8i16, null, null, null, null}
!19 = !{void (i32, i32)* @test_add3_v1i32_2, !"test_add3_v1i32_2", !4, i32 0, !5, !0, !6, i32 0}
!20 = !{void (i32, i32)* @test_add3_v1i32_2, null, null, null, null}
