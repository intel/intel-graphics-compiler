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
; A64 retains the original full-width base and negative descriptor offset.
; Large constant bases are tested on the platforms that adjust IND0.
; The encoder accepts only 32-bit immediate IND0; larger values need a register.
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformCri -simd-mode 32 -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b | FileCheck %s --check-prefixes=CHECK,FOLD
; RUN: igc_opt --opaque-pointers -platformNvl -simd-mode 32 -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b | FileCheck %s --check-prefixes=CHECK,FOLD
; RUN: igc_opt --opaque-pointers -platformNvl -simd-mode 32 -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b -regkey ForceLSCA32SUniformBaseWA | FileCheck %s --check-prefixes=CHECK,FORCE --implicit-check-not=LSCAdjustedBase

target datalayout = "e-p:64:64:64-p3:32:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare i16 @llvm.genx.GenISA.simdLaneId() nounwind readnone
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1), ptr addrspace(1), i32, i32)

; CHECK-LABEL: .kernel "negative_constant_base"
; FORCE-DAG: .decl [[BASE_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FORCE-DAG: .decl [[COPY_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_LD]], 0>
; FORCE-DAG: .decl [[BASE_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FORCE-DAG: .decl [[COPY_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_ST]], 0>
; FORCE-DAG: .decl [[BASE_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FORCE-DAG: .decl [[COPY_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_AT]], 0>
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat(0xFFFFFFF8)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: mov (M1_NM, 1) [[COPY_LD]](0,0)<1> 0x100000008:q
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[BASE_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; FOLD: lsc_store.ugm (M1, 32)  flat(0xFFFFFFF8)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: mov (M1_NM, 1) [[COPY_ST]](0,0)<1> 0x100000008:q
; FORCE: lsc_store.ugm (M1, 32)  flat([[BASE_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat(0xFFFFFFF8)[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: mov (M1_NM, 1) [[COPY_AT]](0,0)<1> 0x100000008:q
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[BASE_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
define spir_kernel void @negative_constant_base(i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 4294967304, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_large_constant_base"
; FORCE-DAG: .decl [[BASE_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FORCE-DAG: .decl [[COPY_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_LD]], 0>
; FORCE-DAG: .decl [[BASE_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FORCE-DAG: .decl [[COPY_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_ST]], 0>
; FORCE-DAG: .decl [[BASE_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FORCE-DAG: .decl [[COPY_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_AT]], 0>
; FOLD-DAG: .decl [[ADJ_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD-DAG: .decl [[ADJ_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; FOLD: mov (M1_NM, 1) [[ADJ_LD]](0,0)<1> 0x100000008:uq
; FOLD: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: mov (M1_NM, 1) [[COPY_LD]](0,0)<1> 0x100000018:q
; FORCE: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[BASE_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; FOLD: mov (M1_NM, 1) [[ADJ_ST]](0,0)<1> 0x100000008:uq
; FOLD: lsc_store.ugm (M1, 32)  flat([[ADJ_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s {{.*}}:d32
; FORCE: mov (M1_NM, 1) [[COPY_ST]](0,0)<1> 0x100000018:q
; FORCE: lsc_store.ugm (M1, 32)  flat([[BASE_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; FOLD: mov (M1_NM, 1) [[ADJ_AT]](0,0)<1> 0x100000008:uq
; FOLD: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[ADJ_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}]:a32s
; FORCE: mov (M1_NM, 1) [[COPY_AT]](0,0)<1> 0x100000018:q
; FORCE: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[BASE_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
define spir_kernel void @negative_large_constant_base(i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %extended = sext i32 %varying to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 4294967320, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_large_base_a64"
; CHECK-NOT: LSCAdjustedBase
; CHECK-DAG: .decl [[BASE_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; CHECK-DAG: .decl [[COPY_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_LD]], 0>
; CHECK-DAG: .decl [[BASE_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; CHECK-DAG: .decl [[COPY_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_ST]], 0>
; CHECK-DAG: .decl [[BASE_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; CHECK-DAG: .decl [[COPY_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_AT]], 0>
; CHECK: mov (M1_NM, 1) [[COPY_LD]](0,0)<1> 0x100000018:q
; CHECK: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[BASE_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
; CHECK: mov (M1_NM, 1) [[COPY_ST]](0,0)<1> 0x100000018:q
; CHECK: lsc_store.ugm (M1, 32)  flat([[BASE_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64 {{.*}}:d32
; CHECK: mov (M1_NM, 1) [[COPY_AT]](0,0)<1> 0x100000018:q
; CHECK: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[BASE_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a64
define spir_kernel void @negative_large_base_a64(i64 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %extended = add i64 %seed, %lane
  %scaled = shl i64 %extended, 2
  %address = add i64 4294967320, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

; CHECK-LABEL: .kernel "negative_large_base_a32u"
; CHECK-NOT: LSCAdjustedBase
; CHECK-DAG: .decl [[BASE_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; CHECK-DAG: .decl [[COPY_LD:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_LD]], 0>
; CHECK-DAG: .decl [[BASE_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; CHECK-DAG: .decl [[COPY_ST:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_ST]], 0>
; CHECK-DAG: .decl [[BASE_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=uq num_elts=1
; CHECK-DAG: .decl [[COPY_AT:LSCUniformBase[a-zA-Z0-9_]*]] v_type=G type=q num_elts=1 align=qword alias=<[[BASE_AT]], 0>
; CHECK: mov (M1_NM, 1) [[COPY_LD]](0,0)<1> 0x100000018:q
; CHECK: lsc_load.ugm (M1, 32) {{.*}}:d32  flat([[BASE_LD]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
; CHECK: mov (M1_NM, 1) [[COPY_ST]](0,0)<1> 0x100000018:q
; CHECK: lsc_store.ugm (M1, 32)  flat([[BASE_ST]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u {{.*}}:d32
; CHECK: mov (M1_NM, 1) [[COPY_AT]](0,0)<1> 0x100000018:q
; CHECK: lsc_atomic_iadd.ugm (M1, 32) {{.*}}:d32  flat([[BASE_AT]])[0x4*{{[a-zA-Z_][a-zA-Z0-9_]*}}-0x10]:a32u
define spir_kernel void @negative_large_base_a32u(i32 %seed) {
entry:
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i32
  %varying = add i32 %seed, %lane
  %index = or i32 %varying, -2147483648
  %extended = zext i32 %index to i64
  %scaled = shl i64 %extended, 2
  %address = add i64 4294967320, %scaled
  %displaced = add i64 %address, -16
  %ptr = inttoptr i64 %displaced to ptr addrspace(1)
  %data = load i32, ptr addrspace(1) %ptr, align 4
  store i32 %data, ptr addrspace(1) %ptr, align 4
  %old = call i32 @llvm.genx.GenISA.intatomicrawA64.i32.p1.p1(ptr addrspace(1) %ptr, ptr addrspace(1) %ptr, i32 1, i32 0)
  ret void
}

!igc.functions = !{!0, !1, !4, !5}
!0 = !{ptr @negative_constant_base, !2}
!1 = !{ptr @negative_large_constant_base, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!4 = !{ptr @negative_large_base_a64, !2}
!5 = !{ptr @negative_large_base_a32u, !2}
