;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri -options " -igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,ForceOCLSIMDWidth=32'" | FileCheck %s

; LLVM with typed pointers:
; RUN: llvm-as %TYPED_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri -options " -igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,ForceOCLSIMDWidth=32'" | FileCheck %s


target triple = "spir64-unknown-unknown"
declare i64 @_Z13get_global_idj(i32)

; CHECK-LABEL:      .kernel "test_div_fp16_rne"
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            mov (M1_NM, 1) {{.*}}(0,0)<1> [[div]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp16_rne(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = fdiv half %a, %b, !spirv.Decorations !1
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtz"
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1_NM, 1) [[ftof:.*]](0,0)<1> [[div]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp16_rtz(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = fdiv half %a, %b, !spirv.Decorations !2
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtp"
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            mov (M1_NM, 1) [[ftof:.*]](0,0)<1> [[div]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp16_rtp(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = fdiv half %a, %b, !spirv.Decorations !3
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtn"
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            mov (M1_NM, 1) [[ftof:.*]](0,0)<1> [[div]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp16_rtn(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = fdiv half %a, %b, !spirv.Decorations !4
  store half %div, half addrspace(1)* %c, align 2
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtz_vec2_uniform"
; CHECK:            divm (M1_NM, 1) [[div1:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            divm (M1_NM, 1) [[div2:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1_NM, 1) [[ftof1:.*]](0,0)<1> [[div1]](0,0)<0;1,0>
; CHECK:            mov (M1_NM, 1) [[ftof2:.*]](0,0)<1> [[div2]](0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1_NM, 1) [[vec:.*]](0,0)<1> [[ftof1]](0,0)<0;1,0>
; CHECK:            mov (M1_NM, 1) [[vec]](0,1)<1> [[ftof2]](0,0)<0;1,0>
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div1]] v_type=G type=f num_elts=1
; CHECK:            .decl [[div2]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp16_rtz_vec2_uniform(<2 x half> %a, <2 x half> %b, <2 x half> addrspace(1)* align 4 %c) {
entry:
  %div = fdiv <2 x half> %a, %b, !spirv.Decorations !2
  store <2 x half> %div, <2 x half> addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rtz_vec2"
; CHECK:            divm (M1, 32) [[div1:.*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK:            divm (M1, 32) [[div2:.*]](0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1, 32) [[ftof1:.*]](0,0)<1> [[div1]](0,0)<1;1,0>
; CHECK:            mov (M1, 32) [[ftof2:.*]](0,0)<1> [[div2]](0,0)<1;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            mov (M1, 32) [[vec:.*]](0,0)<1> [[ftof1]](0,0)<1;1,0>
; CHECK:            mov (M1, 32) [[vec]](1,0)<1> [[ftof2]](0,0)<1;1,0>
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div1]] v_type=G type=f num_elts=32
; CHECK:            .decl [[div2]] v_type=G type=f num_elts=32
define spir_kernel void @test_div_fp16_rtz_vec2(<2 x half> addrspace(1)* align 4 %a, <2 x half> addrspace(1)* align 4 %b, <2 x half> addrspace(1)* align 4 %c) {
entry:
  %gid = call i64 @_Z13get_global_idj(i32 0)
  %a_ptr  = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %a, i64 %gid
  %a_val  = load <2 x half>, <2 x half> addrspace(1)* %a_ptr, align 4
  %b_ptr  = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %b, i64 %gid
  %b_val  = load <2 x half>, <2 x half> addrspace(1)* %b_ptr, align 4
  %div    = fdiv <2 x half> %a_val, %b_val, !spirv.Decorations !2
  %c_ptr  = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %c, i64 %gid
  store <2 x half> %div, <2 x half> addrspace(1)* %c_ptr, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rne(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = fdiv float %a, %b, !spirv.Decorations !1
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rtz(float %a, float %b, float addrspace(1)* align 4 %c)  {
entry:
  %div = fdiv float %a, %b, !spirv.Decorations !2
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rtp(float %a, float %b, float addrspace(1)* align 4 %c)  {
entry:
  %div = fdiv float %a, %b, !spirv.Decorations !3
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rtn(float %a, float %b, float addrspace(1)* align 4 %c)  {
entry:
  %div = fdiv float %a, %b, !spirv.Decorations !4
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rne(double %a, double %b, double addrspace(1)* align 8 %c)  {
entry:
  %div = fdiv double %a, %b, !spirv.Decorations !1
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rtz(double %a, double %b, double addrspace(1)* align 8 %c)  {
entry:
  %div = fdiv double %a, %b, !spirv.Decorations !2
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rtp(double %a, double %b, double addrspace(1)* align 8 %c)  {
entry:
  %div = fdiv double %a, %b, !spirv.Decorations !3
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rtn(double %a, double %b, double addrspace(1)* align 8 %c)  {
entry:
  %div = fdiv double %a, %b, !spirv.Decorations !4
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

!1 = !{!5}
!2 = !{!6}
!3 = !{!7}
!4 = !{!8}
!5 = !{i32 39, i32 0} ; RNE
!6 = !{i32 39, i32 1} ; RTZ
!7 = !{i32 39, i32 2} ; RTP
!8 = !{i32 39, i32 3} ; RTN
