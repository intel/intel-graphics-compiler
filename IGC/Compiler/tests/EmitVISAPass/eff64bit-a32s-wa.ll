;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Stateless loads, stores and atomics share the address calculation.
; FOLD moves a negative descriptor offset into IND0 while retaining A32S.
; FORCE also avoids A32S when the adjusted IND0 may be nonzero.
; A64/A32U retain their descriptor offsets without an extra base calculation.
; Platform-specific companion tests consume BASE/NOWA.
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformCri -simd-mode 32 -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b | FileCheck %s --check-prefixes=CHECK,FOLD
; RUN: igc_opt --opaque-pointers -platformNvl -simd-mode 32 -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b | FileCheck %s --check-prefixes=CHECK,FOLD
; RUN: igc_opt --opaque-pointers -platformNvl -simd-mode 32 -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b -regkey ForceLSCA32SUniformBaseWA | FileCheck %s --check-prefixes=CHECK,FORCE --implicit-check-not=LSCAdjustedBase --implicit-check-not=LSCUniformBase

target datalayout = "e-p:64:64:64-p3:32:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare i16 @llvm.genx.GenISA.simdLaneId() nounwind readnone
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1), ptr addrspace(1), i32, i32)

; CHECK-LABEL: .kernel "negative_offset"
; FOLD-DAG: .decl [[ADJ_LD:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_ST:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_AT:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD: add (M1_NM, 1) [[ADJ_LD]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; FOLD: add (M1_NM, 1) [[ADJ_ST]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_store.ugm (M1, 32)  flat([[ADJ_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s {{.*}}:d32
; FOLD: add (M1_NM, 1) [[ADJ_AT]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
define spir_kernel void @negative_offset(i64 %base, i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_nonnegative"
; FOLD-DAG: .decl [[ADJ_LD:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_ST:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_AT:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD: add (M1_NM, 1) [[ADJ_LD]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; FOLD: add (M1_NM, 1) [[ADJ_ST]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_store.ugm (M1, 32)  flat([[ADJ_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s {{.*}}:d32
; FOLD: add (M1_NM, 1) [[ADJ_AT]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
define spir_kernel void @negative_nonnegative(i64 %base, i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %index = and i32 %varying, 2147483647
  %extended = sext i32 %index to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "positive_offset"
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a64
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a64
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FOLD: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a64 {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a64 {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a64
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a64
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
define spir_kernel void @positive_offset(i64 %base, i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %displaced = add i64 %address, 16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "zero_offset"
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a64
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a64
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FOLD: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a64 {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a64 {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a64
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a64
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
define spir_kernel void @zero_offset(i64 %base, i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %ptr = inttoptr i64 %address to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "positive_nonnegative"
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32u
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32u
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FOLD: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32u {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32u {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32u
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32u
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
define spir_kernel void @positive_nonnegative(i64 %base, i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %index = and i32 %varying, 2147483647
  %extended = sext i32 %index to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %displaced = add i64 %address, 16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_zero_extended"
; CHECK-NOT: LSCAdjustedBase
; CHECK-NOT: LSCUniformBase
; CHECK: lsc_load.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
; CHECK: lsc_store.ugm (M1, 32)  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u {{.*}}:d32
; CHECK: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
define spir_kernel void @negative_zero_extended(i64 %base, i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %index = or i32 %varying, -2147483648
  %extended = zext i32 %index to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_signed_boundary"
; FOLD-DAG: .decl [[ADJ_LD:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_ST:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_AT:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD: add (M1_NM, 1) [[ADJ_LD]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; FOLD: add (M1_NM, 1) [[ADJ_ST]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_store.ugm (M1, 32)  flat([[ADJ_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s {{.*}}:d32
; FOLD: add (M1_NM, 1) [[ADJ_AT]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
define spir_kernel void @negative_signed_boundary(i64 %base) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %index = or i32 %lane, -2147483648
  %extended = sext i32 %index to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_without_scale"
; FOLD-DAG: .decl [[ADJ_LD:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_ST:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_AT:LSCAdjustedBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD: add (M1_NM, 1) [[ADJ_LD]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_LD]])[{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat(base)[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; FOLD: add (M1_NM, 1) [[ADJ_ST]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_store.ugm (M1, 32)  flat([[ADJ_ST]])[{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat(base)[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s {{.*}}:d32
; FOLD: add (M1_NM, 1) [[ADJ_AT]](0,0)<1> base(0,0)<0;1,0> 0xfffffff0:d
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_AT]])[{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat(base)[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
define spir_kernel void @negative_without_scale(i64 %base, i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %address = add i64 %base, %extended
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "positive_zero_base"
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FOLD: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s {{.*}}:d32
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}+0x10]:a32s
define spir_kernel void @positive_zero_base(i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 0, %scaled
  %displaced = add i64 %address, 16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_zero_base"
; FOLD-DAG: .decl [[ADJ_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD: mov (M1_NM, 1) [[ADJ_LD]](0,0)<1> 0xfffffffffffffff0:uq
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; FOLD: mov (M1_NM, 1) [[ADJ_ST]](0,0)<1> 0xfffffffffffffff0:uq
; FOLD: lsc_store.ugm (M1, 32)  flat([[ADJ_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s {{.*}}:d32
; FOLD: mov (M1_NM, 1) [[ADJ_AT]](0,0)<1> 0xfffffffffffffff0:uq
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
define spir_kernel void @negative_zero_base(i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 0, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_cancelled_base"
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; BASE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_load.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
; FOLD: lsc_store.ugm (M1, 32)  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: lsc_store.ugm (M1, 32)  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; BASE: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; NOWA: lsc_store.ugm (M1, 32)  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s {{.*}}:d32
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; BASE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; NOWA: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat{{(\([^)]*\))?}}[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32s
define spir_kernel void @negative_cancelled_base(i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 16, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_i64_offset"
; CHECK-NOT: LSCAdjustedBase
; CHECK-NOT: LSCUniformBase
; CHECK: lsc_load.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; CHECK: lsc_store.ugm (M1, 32)  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; CHECK: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat(base)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
define spir_kernel void @negative_i64_offset(i64 %base, i64 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %lane64 = zext i32 %lane to i64
  %extended = add i64 %seed, %lane64
  %scaled = shl i64 %extended, 2
  %address = add i64 %base, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_without_base"
; CHECK-NOT: LSCAdjustedBase
; CHECK-NOT: LSCUniformBase
; CHECK: lsc_load.ugm (M1, 32) {{.*}}:d32  flat[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; CHECK: lsc_store.ugm (M1, 32)  flat[{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; CHECK: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat[displaced_0v]:a64
define spir_kernel void @negative_without_base(i64 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %lane64 = zext i32 %lane to i64
  %extended = add i64 %seed, %lane64
  %scaled = shl i64 %extended, 2
  %displaced = add i64 %scaled, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12}
!0 = !{ptr @negative_offset, !13}
!1 = !{ptr @negative_nonnegative, !13}
!2 = !{ptr @positive_offset, !13}
!3 = !{ptr @zero_offset, !13}
!4 = !{ptr @positive_nonnegative, !13}
!5 = !{ptr @negative_zero_extended, !13}
!6 = !{ptr @negative_signed_boundary, !13}
!7 = !{ptr @negative_without_scale, !13}
!8 = !{ptr @positive_zero_base, !13}
!9 = !{ptr @negative_zero_base, !13}
!10 = !{ptr @negative_cancelled_base, !13}
!11 = !{ptr @negative_i64_offset, !13}
!12 = !{ptr @negative_without_base, !13}
!13 = !{!14}
!14 = !{!"function_type", i32 0}
