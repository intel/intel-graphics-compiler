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
declare float @llvm.experimental.constrained.fdiv.f32(float, float, metadata, metadata)
declare double @llvm.experimental.constrained.fdiv.f64(double, double, metadata, metadata)
declare half @llvm.experimental.constrained.fdiv.f16(half, half, metadata, metadata)
declare <2 x half> @llvm.experimental.constrained.fdiv.v2f16(<2 x half>, <2 x half>, metadata, metadata)

; CHECK-LABEL:      .kernel "test_div_fp32_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rne(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rtz(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rtp(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.upward", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp32_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp32_rtn(float %a, float %b, float addrspace(1)* align 4 %c) {
entry:
  %div = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.downward", metadata !"fpexcept.ignore")
  store float %div, float addrspace(1)* %c, align 4
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rne"
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rne(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.tonearest", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtz"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rtz(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtp"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rtp(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.upward", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp64_rtn"
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK:            xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=df num_elts=1
define spir_kernel void @test_div_fp64_rtn(double %a, double %b, double addrspace(1)* align 8 %c) {
entry:
  %div = call double @llvm.experimental.constrained.fdiv.f64(double %a, double %b, metadata !"round.downward", metadata !"fpexcept.ignore")
  store double %div, double addrspace(1)* %c, align 8
  ret void
}

; CHECK-LABEL:      .kernel "test_div_fp16_rne"
; CHECK:            divm (M1_NM, 1) [[div:.*]](0,0)<1> {{.*}}(0,0)<0;1,0> {{.*}}(0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            mov (M1_NM, 1) {{.*}}(0,0)<1> [[div]](0,0)<0;1,0>
; CHECK-NOT:        xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
; CHECK:            ret {{.*}}
; CHECK:            .decl [[div]] v_type=G type=f num_elts=1
define spir_kernel void @test_div_fp16_rne(half %a, half %b, half addrspace(1)* align 2 %c) {
entry:
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.tonearest", metadata !"fpexcept.ignore")
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
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
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
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.upward", metadata !"fpexcept.ignore")
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
  %div = call half @llvm.experimental.constrained.fdiv.f16(half %a, half %b, metadata !"round.downward", metadata !"fpexcept.ignore")
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
  %div = call <2 x half> @llvm.experimental.constrained.fdiv.v2f16(<2 x half> %a, <2 x half> %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
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
  %a_ptr = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %a, i64 %gid
  %a_val = load <2 x half>, <2 x half> addrspace(1)* %a_ptr, align 4
  %b_ptr = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %b, i64 %gid
  %b_val = load <2 x half>, <2 x half> addrspace(1)* %b_ptr, align 4
  %div = call <2 x half> @llvm.experimental.constrained.fdiv.v2f16(<2 x half> %a_val, <2 x half> %b_val, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  %c_ptr = getelementptr inbounds <2 x half>, <2 x half> addrspace(1)* %c, i64 %gid
  store <2 x half> %div, <2 x half> addrspace(1)* %c_ptr, align 4
  ret void
}
