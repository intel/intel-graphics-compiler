;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv,regkeys,pvc-supported,spirv-promote

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16 -o %t.spv
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
; CHECK-VISA-DAG: and {{.*}} [[YExp:.*]](0,0)<1> [[Y_asInt:.*]](0,0)<0;1,0> 0x7f800000:d
; CHECK-VISA-DAG: cmp.eq {{.*}} [[P1:.*]] [[YExp]](0,0)<0;1,0> 0x0:d
; CHECK-VISA-DAG: ([[P1]]) sel {{.*}} [[ScaleUp:.*]](0,0)<1> 0x4f800000:f 0x3f800000:f
; CHECK-VISA-DAG: cmp.ge {{.*}} [[P2:.*]] {{.*}} 0x64000000:ud
; CHECK-VISA-DAG: ([[P2]]) sel {{.*}} [[Scale:.*]](0,0)<1> 0x2f800000:f [[ScaleUp]](0,0)<0;1,0>
; CHECK-VISA-DAG: mul {{.*}} [[ScaledY:.*]](0,0)<1> [[Y:.*]](0,0)<0;1,0> [[Scale]](0,0)<0;1,0>
; CHECK-VISA-DAG: inv {{.*}} [[ResInv:.*]](0,0)<1> [[ScaledY]](0,0)<0;1,0>
; CHECK-VISA-DAG: mul {{.*}} [[TMP:.*]](0,0)<1> [[ResInv]](0,0)<0;1,0> [[X:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mul {{.*}} [[ResMul:.*]](0,0)<1> [[TMP]](0,0)<0;1,0> [[Scale]](0,0)<0;1,0>
; CHECK-VISA-DAG: and {{.*}} [[YMantisa:.*]](0,0)<1> [[Y_asInt]](0,0)<0;1,0> 0x7fffff:d
; CHECK-VISA-DAG: cmp.eq {{.*}} [[P3:.*]] [[YMantisa]](0,0)<0;1,0> 0x0:d
; CHECK-VISA-DAG: cmp.eq {{.*}} [[P4:.*]] [[YExp]](0,0)<0;1,0> 0x0:d
; CHECK-VISA-DAG: or {{.*}} [[IsZeroOrSubnormal:.*]] [[P4]] [[P3]]
; CHECK-VISA-DAG: not {{.*}} [[IsNotZeroOrSubnormal:.*]] [[IsZeroOrSubnormal]]
; CHECK-VISA-DAG: cmp.eq {{.*}} [[IsEqual:.*]] [[X]](0,0)<0;1,0> [[Y]](0,0)<0;1,0>
; CHECK-VISA-DAG: and {{.*}} [[IsNotProperValue:.*]] [[IsEqual]] [[IsNotZeroOrSubnormal]]
; CHECK-VISA-DAG: ([[IsNotProperValue]]) sel {{.*}} [[Res:.*]](0,0)<1> 0x3f800000:f  [[ResMul]]
; CHECK-VISA-DAG: mov (M1_NM, 1) [[StoreRes:.*]](0,0)<1> [[Res]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[StoreRes]] {{.*}} type=bf {{.*}}
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

; CHECK-VISA: .kernel "test_bfloat_select"
; CHECK-VISA-DAG: {{.*}} sel (M1_NM, 1) [[RES:.*]](0,0)<1> [[BFLOAT1:.*]](0,0)<0;1,0> [[BFLOAT2:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RES]] {{.*}} type=bf
; CHECK-VISA-DAG: .decl [[BFLOAT1]] {{.*}} type=bf
; CHECK-VISA-DAG: .decl [[BFLOAT2]] {{.*}} type=bf
define spir_kernel void @test_bfloat_select(bfloat %b1, bfloat %b2, i1 %cond, bfloat addrspace(1)* %out) {
entry:
  %res = select i1 %cond, bfloat %b1, bfloat %b2
  store bfloat %res, bfloat addrspace(1)* %out, align 4
  ret void
}

;
define spir_kernel void @test(bfloat %b1, bfloat %b2, bfloat %b3, bfloat %b4, bfloat addrspace(1)* %out, half %h1, half %h2, half addrspace(1)* %hout) {
entry:
  %add = fadd bfloat %b1, %b2
  store bfloat %add, bfloat addrspace(1)* %out, align 2
  %mulptr1 = ptrtoint bfloat addrspace(1)* %out to i32
  %mulptr2 = add i32 %mulptr1, 4
  %outmul = inttoptr i32 %mulptr2 to bfloat addrspace(1)*
  %mul = fmul bfloat %b1, %b2
  store bfloat %mul, bfloat addrspace(1)* %outmul, align 2
;  %sub = fsub bfloat %b3, %b2
;  %mul = fmul bfloat %add, %sub
;  %div = fdiv bfloat %mul, %b4
;  %rem = frem bfloat %div, %b3
;  %neg = fneg bfloat %rem
;  %addimm = fadd bfloat %neg, 1.0
;  ;%div = frem half %h1, %h2
;  store bfloat %addimm, bfloat addrspace(1)* %out, align 2
;  ;store half %div, half addrspace(1)* %hout, align 2
  ret void
}
