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

; FPEXT

; CHECK-VISA: .kernel "test_bfloat_fpext_float"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT1:.*]](0,0)<1> [[SRCVAR_BFLOAT1:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: lsc_store.ugm (M1_NM, 1) {{.*}} [[RESVAR_FLOAT1]]
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT1]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR_BFLOAT1]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_fpext_float(bfloat %b1, float addrspace(1)* %out) {
entry:
  %ext = fpext bfloat %b1 to float
  store float %ext, float addrspace(1)* %out, align 2
  ret void
}


; CHECK-VISA: .kernel "test_bfloat_fpext_double"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT2:.*]](0,0)<1> [[SRCVAR_BFLOAT2:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_DOUBLE:.*]](0,0)<1> [[RESVAR_FLOAT2]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT2]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR_BFLOAT2]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_DOUBLE]] v_type=G type=df num_elts=1
; CHECK-VISA-DAG: lsc_store.ugm (M1_NM, 1) {{.*}} [[RESVAR_DOUBLE]]
define spir_kernel void @test_bfloat_fpext_double(bfloat %b1, double addrspace(1)* %out) {
entry:
  %ext = fpext bfloat %b1 to double
  store double %ext, double addrspace(1)* %out, align 2
  ret void
}

; FPTRUNC

; CHECK-VISA: .kernel "test_bfloat_fptrunc_float"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_BFLOAT:.*]](0,0)<1> [[SRCVAR_FLOAT:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: lsc_store.ugm (M1_NM, 1) {{.*}} [[RESVAR_BFLOAT]]
; CHECK-VISA-DAG: .decl [[RESVAR_BFLOAT]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR_FLOAT]] v_type=G type=f num_elts=1
define spir_kernel void @test_bfloat_fptrunc_float(float %b1, bfloat addrspace(1)* %out) {
entry:
  %trunc = fptrunc float %b1 to bfloat
  store bfloat %trunc, bfloat addrspace(1)* %out, align 2
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_fptrunc_double"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR_DOUBLE:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_BFLOAT:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR_DOUBLE]] v_type=G type=df num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_BFLOAT]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_fptrunc_double(double %b1, bfloat addrspace(1)* %out) {
entry:
  %trunc = fptrunc double %b1 to bfloat
  store bfloat %trunc, bfloat addrspace(1)* %out, align 2
  ret void
}

; FPTOUI

; CHECK-VISA: .kernel "test_bfloat_fptoui_i64"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I64:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I64]] v_type=G type=uq num_elts=1
define spir_kernel void @test_bfloat_fptoui_i64(bfloat %b1, i64 addrspace(1)* %out) {
entry:
  %fptoui = fptoui bfloat %b1 to i64
  store i64 %fptoui, i64 addrspace(1)* %out, align 4
  ret void
}

; FPTOUI

; CHECK-VISA: .kernel "test_bfloat_fptoui_i32"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I32:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I32]] v_type=G type=ud num_elts=1
define spir_kernel void @test_bfloat_fptoui_i32(bfloat %b1, i32 addrspace(1)* %out) {
entry:
  %fptoui = fptoui bfloat %b1 to i32
  store i32 %fptoui, i32 addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_fptoui_i16"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I16:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I16]] v_type=G type=uw num_elts=1
define spir_kernel void @test_bfloat_fptoui_i16(bfloat %b1, i16 addrspace(1)* %out) {
entry:
  %fptoui = fptoui bfloat %b1 to i16
  store i16 %fptoui, i16 addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_fptoui_i8"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I8:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I8]] v_type=G type=ub num_elts=1
define spir_kernel void @test_bfloat_fptoui_i8(bfloat %b1, i8 addrspace(1)* %out) {
entry:
  %fptoui = fptoui bfloat %b1 to i8
  store i8 %fptoui, i8 addrspace(1)* %out, align 4
  ret void
}

; FPTOSI

; CHECK-VISA: .kernel "test_bfloat_fptosi_i64"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I64:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I64]] v_type=G type=q num_elts=1
define spir_kernel void @test_bfloat_fptosi_i64(bfloat %b1, i64 addrspace(1)* %out) {
entry:
  %fptosi = fptosi bfloat %b1 to i64
  store i64 %fptosi, i64 addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_fptosi_i32"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I32:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I32]] v_type=G type=d num_elts=1
define spir_kernel void @test_bfloat_fptosi_i32(bfloat %b1, i32 addrspace(1)* %out) {
entry:
  %fptosi = fptosi bfloat %b1 to i32
  store i32 %fptosi, i32 addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_fptosi_i16"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I16:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I16]] v_type=G type=w num_elts=1
define spir_kernel void @test_bfloat_fptosi_i16(bfloat %b1, i16 addrspace(1)* %out) {
entry:
  %fptosi = fptosi bfloat %b1 to i16
  store i16 %fptosi, i16 addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_fptosi_i8"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_I8:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=bf num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR_I8]] v_type=G type=b num_elts=1
define spir_kernel void @test_bfloat_fptosi_i8(bfloat %b1, i8 addrspace(1)* %out) {
entry:
  %fptosi = fptosi bfloat %b1 to i8
  store i8 %fptosi, i8 addrspace(1)* %out, align 4
  ret void
}

; UITOFP

; CHECK-VISA: .kernel "test_bfloat_uitofp_i64"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=uq num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_uitofp_i64(i64 %n1, bfloat addrspace(1)* %out) {
entry:
  %uitofp = uitofp i64 %n1 to bfloat
  store bfloat %uitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_uitofp_i32"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=ud num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_uitofp_i32(i32 %n1, bfloat addrspace(1)* %out) {
entry:
  %uitofp = uitofp i32 %n1 to bfloat
  store bfloat %uitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_uitofp_i16"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=uw num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_uitofp_i16(i16 %n1, bfloat addrspace(1)* %out) {
entry:
  %uitofp = uitofp i16 %n1 to bfloat
  store bfloat %uitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_uitofp_i8"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=ub num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_uitofp_i8(i8 %n1, bfloat addrspace(1)* %out) {
entry:
  %uitofp = uitofp i8 %n1 to bfloat
  store bfloat %uitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; SITOFP

; CHECK-VISA: .kernel "test_bfloat_sitofp_i64"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=q num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_sitofp_i64(i64 %n1, bfloat addrspace(1)* %out) {
entry:
  %sitofp = sitofp i64 %n1 to bfloat
  store bfloat %sitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_sitofp_i32"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=d num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_sitofp_i32(i32 %n1, bfloat addrspace(1)* %out) {
entry:
  %sitofp = sitofp i32 %n1 to bfloat
  store bfloat %sitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_sitofp_i16"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=w num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_sitofp_i16(i16 %n1, bfloat addrspace(1)* %out) {
entry:
  %sitofp = sitofp i16 %n1 to bfloat
  store bfloat %sitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_sitofp_i8"
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR_FLOAT:.*]](0,0)<1> [[SRCVAR:.*]](0,0)<0;1,0>
; CHECK-VISA-DAG: mov (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[RESVAR_FLOAT]](0,0)<0;1,0>
; CHECK-VISA-DAG: .decl [[RESVAR_FLOAT]] v_type=G type=f num_elts=1
; CHECK-VISA-DAG: .decl [[SRCVAR]] v_type=G type=b num_elts=1
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_sitofp_i8(i8 %n1, bfloat addrspace(1)* %out) {
entry:
  %sitofp = sitofp i8 %n1 to bfloat
  store bfloat %sitofp, bfloat addrspace(1)* %out, align 4
  ret void
}

; BITCAST

; CHECK-VISA: .kernel "test_bfloat_bitcast_i16"
; CHECK-VISA-DAG: add (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[CASTEDSRC:.*]](0,0)<0;1,0> 0x1:w
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=w num_elts=1
; CHECK-VISA-DAG: .decl [[CASTEDSRC]] v_type=G type=w num_elts=1 {{.*}} alias=<[[BFLOATVAR:.*]], 0>
; CHECK-VISA-DAG: .decl [[BFLOATVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_bitcast_i16(bfloat %b1, bfloat addrspace(1)* %out) {
entry:
  %word = bitcast bfloat %b1 to i16
  %add = add i16 %word, 1
  %res = bitcast i16 %add to bfloat
  store bfloat %res, bfloat addrspace(1)* %out, align 4
  ret void
}

; CHECK-VISA: .kernel "test_bfloat_bitcast_half"
; CHECK-VISA-DAG: add (M1_NM, 1) [[RESVAR:.*]](0,0)<1> [[CASTEDSRC:.*]](0,0)<0;1,0> 0x3c00:hf
; CHECK-VISA-DAG: .decl [[RESVAR]] v_type=G type=hf num_elts=1
; CHECK-VISA-DAG: .decl [[CASTEDSRC]] v_type=G type=hf num_elts=1 {{.*}} alias=<[[BFLOATVAR:.*]], 0>
; CHECK-VISA-DAG: .decl [[BFLOATVAR]] v_type=G type=bf num_elts=1
define spir_kernel void @test_bfloat_bitcast_half(bfloat %b1, bfloat addrspace(1)* %out) {
entry:
  %half = bitcast bfloat %b1 to half
  %add = fadd half %half, 1.0
  %res = bitcast half %add to bfloat
  store bfloat %res, bfloat addrspace(1)* %out, align 4
  ret void
}
