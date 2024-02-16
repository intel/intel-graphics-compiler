;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv,regkeys,pvc-supported,llvm-14-plus
; UNSUPPORTED: legacy-translator

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_brain_float16 -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'DumpVISAASMToConsole=1'" -device pvc 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spirv64-unknown-unknown"

; CHECK-VISA: .kernel "test_fadd"
define spir_kernel void @test_fadd(
  bfloat %b1, bfloat %b2, bfloat addrspace(1)* %out1,
  <2 x bfloat>  %b1_2,  <2 x bfloat>  %b2_2,  <2 x bfloat>  addrspace(1)* %out2,
  <3 x bfloat>  %b1_3,  <3 x bfloat>  %b2_3,  <3 x bfloat>  addrspace(1)* %out3,
  <4 x bfloat>  %b1_4,  <4 x bfloat>  %b2_4,  <4 x bfloat>  addrspace(1)* %out4,
  <8 x bfloat>  %b1_8,  <8 x bfloat>  %b2_8,  <8 x bfloat>  addrspace(1)* %out8,
  <16 x bfloat> %b1_16, <16 x bfloat> %b2_16, <16 x bfloat> addrspace(1)* %out16
) {
entry:
  %res = fadd bfloat %b1, %b2
  store bfloat %res, bfloat addrspace(1)* %out1, align 2
; CHECK-VISA-DAG: add {{.*}} [[RES:.*]](0,0)<1> [[SRC0:.*]](0,0)<0;1,0> [[SRC1:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC0]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC1]] {{.*}} type=bf num_elts=1 {{.*}}
  %res2 = fadd <2 x bfloat> %b1_2, %b2_2
  store <2 x bfloat> %res2, <2 x bfloat> addrspace(1)* %out2, align 4
; CHECK-VISA-DAG: add {{.*}} [[RES_2_1:.*]](0,0)<1> [[SRC0_2:.*]](0,0)<0;1,0> [[SRC1_2:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: add {{.*}} [[RES_2_2:.*]](0,0)<1> [[SRC0_2]](0,1)<0;1,0> [[SRC1_2]](0,1)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES_2_1]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[RES_2_2]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC0_2]] {{.*}} type=bf num_elts=2 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC1_2]] {{.*}} type=bf num_elts=2 {{.*}}
  %res4 = fadd <4 x bfloat> %b1_4, %b2_4
  store <4 x bfloat> %res4, <4 x bfloat> addrspace(1)* %out4, align 8
; CHECK-VISA-DAG: add {{.*}} [[RES_4_1:.*]](0,0)<1> [[SRC0_4:.*]](0,0)<0;1,0> [[SRC1_4:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: add {{.*}} [[RES_4_2:.*]](0,0)<1> [[SRC0_4]](0,1)<0;1,0> [[SRC1_4]](0,1)<0;1,0>
; CHECK-VISA-DAG: add {{.*}} [[RES_4_3:.*]](0,0)<1> [[SRC0_4]](0,2)<0;1,0> [[SRC1_4]](0,2)<0;1,0>
; CHECK-VISA-DAG: add {{.*}} [[RES_4_4:.*]](0,0)<1> [[SRC0_4]](0,3)<0;1,0> [[SRC1_4]](0,3)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES_4_1]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[RES_4_2]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[RES_4_3]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[RES_4_4]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC0_4]] {{.*}} type=bf num_elts=4 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC1_4]] {{.*}} type=bf num_elts=4 {{.*}}
  %res3 = fadd <3 x bfloat> %b1_3, %b2_3
  store <3 x bfloat> %res3, <3 x bfloat> addrspace(1)* %out3, align 8
  %res8 = fadd <8 x bfloat> %b1_8, %b2_8
  store <8 x bfloat> %res8, <8 x bfloat> addrspace(1)* %out8, align 16
  %res16 = fadd <16 x bfloat> %b1_16, %b2_16
  store <16 x bfloat> %res16, <16 x bfloat> addrspace(1)* %out16, align 32
  ret void
}

; CHECK-VISA: .kernel "test_fsub"
define spir_kernel void @test_fsub(
  bfloat %b1, bfloat %b2, bfloat addrspace(1)* %out1,
  <2 x bfloat>  %b1_2,  <2 x bfloat>  %b2_2,  <2 x bfloat>  addrspace(1)* %out2,
  <3 x bfloat>  %b1_3,  <3 x bfloat>  %b2_3,  <3 x bfloat>  addrspace(1)* %out3,
  <4 x bfloat>  %b1_4,  <4 x bfloat>  %b2_4,  <4 x bfloat>  addrspace(1)* %out4,
  <8 x bfloat>  %b1_8,  <8 x bfloat>  %b2_8,  <8 x bfloat>  addrspace(1)* %out8,
  <16 x bfloat> %b1_16, <16 x bfloat> %b2_16, <16 x bfloat> addrspace(1)* %out16
) {
entry:
  %res = fsub bfloat %b1, %b2
  store bfloat %res, bfloat addrspace(1)* %out1, align 2
; CHECK-VISA-DAG: add {{.*}} [[RES:.*]](0,0)<1> [[SRC0:.*]](0,0)<0;1,0> (-)[[SRC1:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC0]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC1]] {{.*}} type=bf num_elts=1 {{.*}}
  %res2 = fsub <2 x bfloat> %b1_2, %b2_2
  store <2 x bfloat> %res2, <2 x bfloat> addrspace(1)* %out2, align 4
  %res4 = fsub <4 x bfloat> %b1_4, %b2_4
  store <4 x bfloat> %res4, <4 x bfloat> addrspace(1)* %out4, align 8
  %res3 = fsub <3 x bfloat> %b1_3, %b2_3
  store <3 x bfloat> %res3, <3 x bfloat> addrspace(1)* %out3, align 8
  %res8 = fsub <8 x bfloat> %b1_8, %b2_8
  store <8 x bfloat> %res8, <8 x bfloat> addrspace(1)* %out8, align 16
  %res16 = fsub <16 x bfloat> %b1_16, %b2_16
  store <16 x bfloat> %res16, <16 x bfloat> addrspace(1)* %out16, align 32
  ret void
}

; CHECK-VISA: .kernel "test_fmul"
define spir_kernel void @test_fmul(
  bfloat %b1, bfloat %b2, bfloat addrspace(1)* %out1,
  <2 x bfloat>  %b1_2,  <2 x bfloat>  %b2_2,  <2 x bfloat>  addrspace(1)* %out2,
  <3 x bfloat>  %b1_3,  <3 x bfloat>  %b2_3,  <3 x bfloat>  addrspace(1)* %out3,
  <4 x bfloat>  %b1_4,  <4 x bfloat>  %b2_4,  <4 x bfloat>  addrspace(1)* %out4,
  <8 x bfloat>  %b1_8,  <8 x bfloat>  %b2_8,  <8 x bfloat>  addrspace(1)* %out8,
  <16 x bfloat> %b1_16, <16 x bfloat> %b2_16, <16 x bfloat> addrspace(1)* %out16
) {
entry:
  %res = fmul bfloat %b1, %b2
  store bfloat %res, bfloat addrspace(1)* %out1, align 2
; CHECK-VISA-DAG: mul {{.*}} [[RES:.*]](0,0)<1> [[SRC0:.*]](0,0)<0;1,0> [[SRC1:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC0]] {{.*}} type=bf num_elts=1 {{.*}}
; CHECK-VISA-DAG: .decl [[SRC1]] {{.*}} type=bf num_elts=1 {{.*}}
  %res2 = fmul <2 x bfloat> %b1_2, %b2_2
  store <2 x bfloat> %res2, <2 x bfloat> addrspace(1)* %out2, align 4
  %res4 = fmul <4 x bfloat> %b1_4, %b2_4
  store <4 x bfloat> %res4, <4 x bfloat> addrspace(1)* %out4, align 8
  %res3 = fmul <3 x bfloat> %b1_3, %b2_3
  store <3 x bfloat> %res3, <3 x bfloat> addrspace(1)* %out3, align 8
  %res8 = fmul <8 x bfloat> %b1_8, %b2_8
  store <8 x bfloat> %res8, <8 x bfloat> addrspace(1)* %out8, align 16
  %res16 = fmul <16 x bfloat> %b1_16, %b2_16
  store <16 x bfloat> %res16, <16 x bfloat> addrspace(1)* %out16, align 32
  ret void
}

; CHECK-VISA: .kernel "test_fdiv"
define spir_kernel void @test_fdiv(
  bfloat %b1, bfloat %b2, bfloat addrspace(1)* %out1,
  <2 x bfloat>  %b1_2,  <2 x bfloat>  %b2_2,  <2 x bfloat>  addrspace(1)* %out2,
  <3 x bfloat>  %b1_3,  <3 x bfloat>  %b2_3,  <3 x bfloat>  addrspace(1)* %out3,
  <4 x bfloat>  %b1_4,  <4 x bfloat>  %b2_4,  <4 x bfloat>  addrspace(1)* %out4,
  <8 x bfloat>  %b1_8,  <8 x bfloat>  %b2_8,  <8 x bfloat>  addrspace(1)* %out8,
  <16 x bfloat> %b1_16, <16 x bfloat> %b2_16, <16 x bfloat> addrspace(1)* %out16
) {
entry:
  %res = fdiv bfloat %b1, %b2
  store bfloat %res, bfloat addrspace(1)* %out1, align 2
; fdiv is expanded to float inv+mul
; CHECK-VISA-DAG: inv {{.*}} [[RESINV:.*]](0,0)<1> {{.*}}<0;1,0>
; CHECK-VISA-DAG: mul {{.*}} [[RESMUL:.*]](0,0)<1> [[RESINV]](0,0)<0;1,0> [[SRC1:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RES:.*]](0,0)<1> [[RESMUL]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES]] {{.*}} type=bf {{.*}}
  %res2 = fdiv <2 x bfloat> %b1_2, %b2_2
  store <2 x bfloat> %res2, <2 x bfloat> addrspace(1)* %out2, align 4
  %res4 = fdiv <4 x bfloat> %b1_4, %b2_4
  store <4 x bfloat> %res4, <4 x bfloat> addrspace(1)* %out4, align 8
  %res3 = fdiv <3 x bfloat> %b1_3, %b2_3
  store <3 x bfloat> %res3, <3 x bfloat> addrspace(1)* %out3, align 8
  %res8 = fdiv <8 x bfloat> %b1_8, %b2_8
  store <8 x bfloat> %res8, <8 x bfloat> addrspace(1)* %out8, align 16
  %res16 = fdiv <16 x bfloat> %b1_16, %b2_16
  store <16 x bfloat> %res16, <16 x bfloat> addrspace(1)* %out16, align 32
  ret void
}

