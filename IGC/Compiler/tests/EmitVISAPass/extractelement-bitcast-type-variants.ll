;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt -S %s --opaque-pointers -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; Verify that CanTreatAsAlias works correctly for extracts whose downstream
; bitcasts produce types wider, narrower, or differently interpreted than i32.
;
; test_extract_bitcast_to_vec_type:
;   Each extract is bitcast to <2 x i16>. This is a scalar→vector cast of
;   equal bit-width (32→32), handled by DeSSA's canAliasVecCast path.
;   CanTreatAsAlias allows aliasing because the only coalescing on the extract
;   is the noop-alias created by canAliasVecCast (isExtractCoalescedOnlyByNoopAlias).
;   IGC creates a word-typed (type=w, 8-element) alias of vec for the
;   <2 x i16> view, so consumers read vec's i16 sub-elements directly.
;
; test_extract_mixed_bitcast_consumers:
;   Four extracts from the same <4 x float> select, each with a different
;   downstream consumer type to exercise all alias paths in one kernel:
;     e0 → bitcast float → i32         (DeSSA isNoOpInst noop-alias)
;     e1 → bitcast float → <2 x i16>   (DeSSA canAliasVecCast noop-alias)
;     e2 → fptrunc float → half        (NOT a noop-alias; IsCoalesced=false,
;                                        CanTreatAsAlias holds unconditionally)
;     e3 → fptrunc float → bfloat      (same as e2)
;   In all four cases no standalone float variable is extracted from vec:
;   e0/e1 read vec sub-registers via typed DWORD/WORD aliases; e2/e3 are
;   type-converted directly from vec sub-registers by the fptrunc MOV.

; DPAS forces SIMD16 execution: all variables expand to 16-lane width.
; vec is 4 rows × 16 lanes (rows 0-3 map to vec elements 0-3).
; vec_0v (word alias) has 8 rows; stride-2 region reads select lo or hi i16.
; In test 3, bc_v lo-halves are in column 0-15, hi-halves in column 16-31.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x i32>   @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32>,  <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

; -- test 1: bitcast to vector type <2 x i16> --------------------------------
; CHECK-LABEL: .kernel "test_extract_bitcast_to_vec_type"
; <2 x i16> consumers use stride-2 regions on vec_0v (word alias of vec).
; No extraction MOV copies a float row of vec to a standalone variable.
; CHECK-NOT: mov (M1, 16) {{.*}}(0,0)<1> vec(0,
; CHECK: add (M1, 16) p0(0,0)<1> vec_0v(0,0)<2;1,0> vec_0v(0,1)<2;1,0>
; CHECK: add (M1, 16) p1(0,0)<1> vec_0v(1,0)<2;1,0> vec_0v(1,1)<2;1,0>
; CHECK: add (M1, 16) p2(0,0)<1> vec_0v(2,0)<2;1,0> vec_0v(2,1)<2;1,0>
; CHECK: add (M1, 16) p3(0,0)<1> vec_0v(3,0)<2;1,0> vec_0v(3,1)<2;1,0>
; CHECK-NOT: mov (M1, 16) {{.*}}(0,0)<1> vec(0,
; CHECK: mad (M1, 16) {{.*}} vec(0,0)<1;1,0> vec(0,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(1,0)<1;1,0> vec(1,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(2,0)<1;1,0> vec(2,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(3,0)<1;1,0> vec(3,0)<1;1,0>
define spir_kernel void @test_extract_bitcast_to_vec_type(float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  ; DPAS produces a non-uniform <8 x float>; split into lower/upper <4 x float>
  ; for the select operands.  Using %acc in the accumulator prevents DCE.
  %dpas_acc = insertelement <8 x float> zeroinitializer, float %acc, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %d0 = extractelement <8 x float> %dpas, i32 0
  %d1 = extractelement <8 x float> %dpas, i32 1
  %d2 = extractelement <8 x float> %dpas, i32 2
  %d3 = extractelement <8 x float> %dpas, i32 3
  %d4 = extractelement <8 x float> %dpas, i32 4
  %d5 = extractelement <8 x float> %dpas, i32 5
  %d6 = extractelement <8 x float> %dpas, i32 6
  %d7 = extractelement <8 x float> %dpas, i32 7
  %tv0 = insertelement <4 x float> undef, float %d0, i32 0
  %tv1 = insertelement <4 x float> %tv0, float %d1, i32 1
  %tv2 = insertelement <4 x float> %tv1, float %d2, i32 2
  %true_val_v  = insertelement <4 x float> %tv2, float %d3, i32 3
  %fv0 = insertelement <4 x float> undef, float %d4, i32 0
  %fv1 = insertelement <4 x float> %fv0, float %d5, i32 1
  %fv2 = insertelement <4 x float> %fv1, float %d6, i32 2
  %false_val_v = insertelement <4 x float> %fv2, float %d7, i32 3
  %vec = select i1 %cmp, <4 x float> %true_val_v, <4 x float> %false_val_v

  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  ; Float consumers (keep extracts alive via fmul)
  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3

  ; Bitcast each float extract to a 2-element i16 vector: canAliasVecCast noop-alias
  %bc0_v = bitcast float %e0 to <2 x i16>
  %bc1_v = bitcast float %e1 to <2 x i16>
  %bc2_v = bitcast float %e2 to <2 x i16>
  %bc3_v = bitcast float %e3 to <2 x i16>

  ; Extract the two i16 halves of each float's bit pattern
  %lo0 = extractelement <2 x i16> %bc0_v, i32 0
  %hi0 = extractelement <2 x i16> %bc0_v, i32 1
  %lo1 = extractelement <2 x i16> %bc1_v, i32 0
  %hi1 = extractelement <2 x i16> %bc1_v, i32 1
  %lo2 = extractelement <2 x i16> %bc2_v, i32 0
  %hi2 = extractelement <2 x i16> %bc2_v, i32 1
  %lo3 = extractelement <2 x i16> %bc3_v, i32 0
  %hi3 = extractelement <2 x i16> %bc3_v, i32 1

  ; Add the two halves of each float's bit pattern
  %p0 = add i16 %lo0, %hi0
  %p1 = add i16 %lo1, %hi1
  %p2 = add i16 %lo2, %hi2
  %p3 = add i16 %lo3, %hi3

  ; Convert back to float to feed the loop accumulator
  %q0 = zext i16 %p0 to i32
  %q1 = zext i16 %p1 to i32
  %q2 = zext i16 %p2 to i32
  %q3 = zext i16 %p3 to i32
  %r0 = sitofp i32 %q0 to float
  %r1 = sitofp i32 %q1 to float
  %r2 = sitofp i32 %q2 to float
  %r3 = sitofp i32 %q3 to float

  %s0 = fadd float %f0, %r0
  %s1 = fadd float %f1, %r1
  %s2 = fadd float %f2, %r2
  %s3 = fadd float %f3, %r3
  %t0 = fadd float %s0, %s1
  %t1 = fadd float %s2, %s3
  %sum = fadd float %t0, %t1

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; -- test 2: each extract with a different downstream consumer type -----------
; CHECK-LABEL: .kernel "test_extract_mixed_bitcast_consumers"
; No standalone float rows of vec copied to standalone variables.
; CHECK-NOT: mov (M1, 16) e0(0,0)<1> vec(0,0)
; CHECK-NOT: mov (M1, 16) e1(0,0)<1> vec(1,0)
; CHECK-NOT: mov (M1, 16) e2(0,0)<1> vec(2,0)
; CHECK-NOT: mov (M1, 16) e3(0,0)<1> vec(3,0)
; e0 (→ i32): dword alias at row 0, stride-1.
; CHECK: add (M1, 16) i0(0,0)<1> vec_0v(0,0)<1;1,0>
; e1 (→ <2 x i16>): word alias at row 1, stride-2 for lo/hi halves.
; CHECK: add (M1, 16) p1(0,0)<1> vec_1v(1,0)<2;1,0> vec_1v(1,1)<2;1,0>
; e2 (→ half): fptrunc reads vec row 2 directly (no float copy to e2 first).
; CHECK: mov (M1, 16) hf2(0,0)<1> vec(2,0)<1;1,0>
; e3 (→ bfloat): same as e2 for row 3.
; CHECK: mov (M1, 16) bf3(0,0)<1> vec(3,0)<1;1,0>
define spir_kernel void @test_extract_mixed_bitcast_consumers(float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %dpas_acc = insertelement <8 x float> zeroinitializer, float %acc, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %d0 = extractelement <8 x float> %dpas, i32 0
  %d1 = extractelement <8 x float> %dpas, i32 1
  %d2 = extractelement <8 x float> %dpas, i32 2
  %d3 = extractelement <8 x float> %dpas, i32 3
  %d4 = extractelement <8 x float> %dpas, i32 4
  %d5 = extractelement <8 x float> %dpas, i32 5
  %d6 = extractelement <8 x float> %dpas, i32 6
  %d7 = extractelement <8 x float> %dpas, i32 7
  %tv0 = insertelement <4 x float> undef, float %d0, i32 0
  %tv1 = insertelement <4 x float> %tv0, float %d1, i32 1
  %tv2 = insertelement <4 x float> %tv1, float %d2, i32 2
  %true_val_v  = insertelement <4 x float> %tv2, float %d3, i32 3
  %fv0 = insertelement <4 x float> undef, float %d4, i32 0
  %fv1 = insertelement <4 x float> %fv0, float %d5, i32 1
  %fv2 = insertelement <4 x float> %fv1, float %d6, i32 2
  %false_val_v = insertelement <4 x float> %fv2, float %d7, i32 3
  %vec = select i1 %cmp, <4 x float> %true_val_v, <4 x float> %false_val_v

  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  ; e0: bitcast float -> i32 (DeSSA isNoOpInst noop-alias)
  %bc0 = bitcast float %e0 to i32
  %i0 = add i32 %bc0, 1

  ; e1: bitcast float -> <2 x i16> (DeSSA canAliasVecCast noop-alias)
  %bc1_v = bitcast float %e1 to <2 x i16>
  %lo1 = extractelement <2 x i16> %bc1_v, i32 0
  %hi1 = extractelement <2 x i16> %bc1_v, i32 1
  %p1 = add i16 %lo1, %hi1

  ; e2: fptrunc float -> half (NOT a noop-alias; IsCoalesced(e2)=false so
  ;     CanTreatAsAlias works naturally — the fptrunc MOV reads vec directly)
  %hf2 = fptrunc float %e2 to half
  %hf2_f = fpext half %hf2 to float

  ; e3: fptrunc float -> bfloat (same reasoning as e2)
  %bf3 = fptrunc float %e3 to bfloat
  %bf3_f = fpext bfloat %bf3 to float

  ; Combine results into loop accumulator
  %ib0 = bitcast i32 %i0 to float
  %p1_i32 = sext i16 %p1 to i32
  %p1_f = sitofp i32 %p1_i32 to float
  %s0 = fadd float %ib0, %p1_f
  %s1 = fadd float %hf2_f, %bf3_f
  %sum = fadd float %s0, %s1

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; -- test 3: bitcast float -> <2 x i16> then insertelement into the result ----
; CHECK-LABEL: .kernel "test_extract_bitcast_vec_insertelement"
; bc_v variables are standalone (mutable) and initialised from vec_0v using
; stride-2 regions.  Lo halves go to column 0, hi halves to column 16
; (the SIMD16 second half of the GRF).  No float row of vec is copied.
; CHECK-NOT: mov (M1, 16) {{.*}}(0,0)<1> vec(0,
; bc0_v–bc3_v lo/hi initialisation from vec_0v rows 0-3
; CHECK: mov (M1, 16) bc0_v(0,0)<1> vec_0v(0,0)<2;1,0>
; CHECK: mov (M1, 16) bc0_v(0,16)<1> vec_0v(0,1)<2;1,0>
; CHECK: mov (M1, 16) bc1_v(0,0)<1> vec_0v(1,0)<2;1,0>
; CHECK: mov (M1, 16) bc1_v(0,16)<1> vec_0v(1,1)<2;1,0>
; CHECK: mov (M1, 16) bc2_v(0,0)<1> vec_0v(2,0)<2;1,0>
; CHECK: mov (M1, 16) bc2_v(0,16)<1> vec_0v(2,1)<2;1,0>
; CHECK: mov (M1, 16) bc3_v(0,0)<1> vec_0v(3,0)<2;1,0>
; CHECK: mov (M1, 16) bc3_v(0,16)<1> vec_0v(3,1)<2;1,0>
; insertelement: patch is broadcast uniformly into lo half of each bc_v
; CHECK: mov (M1, 16) bc0_v(0,0)<1> patch(0,0)<0;1,0>
; CHECK: mov (M1, 16) bc1_v(0,0)<1> patch(0,0)<0;1,0>
; CHECK: mov (M1, 16) bc2_v(0,0)<1> patch(0,0)<0;1,0>
; CHECK: mov (M1, 16) bc3_v(0,0)<1> patch(0,0)<0;1,0>
; float fmul consumers read vec rows directly
; CHECK-NOT: mov (M1, 16) {{.*}}(0,0)<1> vec(0,
; CHECK: mad (M1, 16) {{.*}} vec(0,0)<1;1,0> vec(0,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(1,0)<1;1,0> vec(1,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(2,0)<1;1,0> vec(2,0)<1;1,0>
; CHECK: mad (M1, 16) {{.*}} vec(3,0)<1;1,0> vec(3,0)<1;1,0>
define spir_kernel void @test_extract_bitcast_vec_insertelement(float %cond_val, i16 %patch) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %dpas_acc = insertelement <8 x float> zeroinitializer, float %acc, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %d0 = extractelement <8 x float> %dpas, i32 0
  %d1 = extractelement <8 x float> %dpas, i32 1
  %d2 = extractelement <8 x float> %dpas, i32 2
  %d3 = extractelement <8 x float> %dpas, i32 3
  %d4 = extractelement <8 x float> %dpas, i32 4
  %d5 = extractelement <8 x float> %dpas, i32 5
  %d6 = extractelement <8 x float> %dpas, i32 6
  %d7 = extractelement <8 x float> %dpas, i32 7
  %tv0 = insertelement <4 x float> undef, float %d0, i32 0
  %tv1 = insertelement <4 x float> %tv0, float %d1, i32 1
  %tv2 = insertelement <4 x float> %tv1, float %d2, i32 2
  %true_val_v  = insertelement <4 x float> %tv2, float %d3, i32 3
  %fv0 = insertelement <4 x float> undef, float %d4, i32 0
  %fv1 = insertelement <4 x float> %fv0, float %d5, i32 1
  %fv2 = insertelement <4 x float> %fv1, float %d6, i32 2
  %false_val_v = insertelement <4 x float> %fv2, float %d7, i32 3
  %vec = select i1 %cmp, <4 x float> %true_val_v, <4 x float> %false_val_v

  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  ; Float consumers (keep float alias alive via fmul)
  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3

  ; Bitcast float extracts to <2 x i16>
  %bc0_v = bitcast float %e0 to <2 x i16>
  %bc1_v = bitcast float %e1 to <2 x i16>
  %bc2_v = bitcast float %e2 to <2 x i16>
  %bc3_v = bitcast float %e3 to <2 x i16>

  ; Insert %patch into element 0 of each bitcasted vector
  %ins0 = insertelement <2 x i16> %bc0_v, i16 %patch, i32 0
  %ins1 = insertelement <2 x i16> %bc1_v, i16 %patch, i32 0
  %ins2 = insertelement <2 x i16> %bc2_v, i16 %patch, i32 0
  %ins3 = insertelement <2 x i16> %bc3_v, i16 %patch, i32 0

  ; Extract the inserted (lo) and unmodified (hi) halves
  %lo0 = extractelement <2 x i16> %ins0, i32 0
  %hi0 = extractelement <2 x i16> %ins0, i32 1
  %lo1 = extractelement <2 x i16> %ins1, i32 0
  %hi1 = extractelement <2 x i16> %ins1, i32 1
  %lo2 = extractelement <2 x i16> %ins2, i32 0
  %hi2 = extractelement <2 x i16> %ins2, i32 1
  %lo3 = extractelement <2 x i16> %ins3, i32 0
  %hi3 = extractelement <2 x i16> %ins3, i32 1

  %p0 = add i16 %lo0, %hi0
  %p1 = add i16 %lo1, %hi1
  %p2 = add i16 %lo2, %hi2
  %p3 = add i16 %lo3, %hi3

  ; Combine into loop accumulator
  %q0 = zext i16 %p0 to i32
  %q1 = zext i16 %p1 to i32
  %q2 = zext i16 %p2 to i32
  %q3 = zext i16 %p3 to i32
  %r0 = sitofp i32 %q0 to float
  %r1 = sitofp i32 %q1 to float
  %r2 = sitofp i32 %q2 to float
  %r3 = sitofp i32 %q3 to float

  %s0 = fadd float %f0, %r0
  %s1 = fadd float %f1, %r1
  %s2 = fadd float %f2, %r2
  %s3 = fadd float %f3, %r3
  %t0 = fadd float %s0, %s1
  %t1 = fadd float %s2, %s3
  %sum = fadd float %t0, %t1

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; -- test 4: extract pairs from <8 x i32> and bitcast to i64 ----------------
; CHECK-LABEL: .kernel "test_extract_pair_bitcast_to_i64"
; Two i32 elements are paired into a <2 x i32> and bitcast to i64 (wider type).
; Contiguous pair [e1, e2]: elements at adjacent ivec rows 1 and 2.
; Non-contiguous pair [e3, e0]: elements at ivec rows 3 and 0 — not adjacent,
; stressing the alignment path (assembly reads rows out of natural order).
; No standalone extraction MOV is emitted for any extract.
; CHECK-NOT: mov (M1, 16) e1(0,0)<1> ivec(1,
; CHECK-NOT: mov (M1, 16) e2(0,0)<1> ivec(2,
; CHECK-NOT: mov (M1, 16) e3(0,0)<1> ivec(3,
; CHECK-NOT: mov (M1, 16) e0(0,0)<1> ivec(0,
; Contiguous [1,2]: pack12_0 rows 0 and 1 read directly from ivec rows 1 and 2.
; CHECK: mov (M1, 16) pack12_0(0,0)<1> ivec(1,0)<1;1,0>
; CHECK: mov (M1, 16) pack12_0(1,0)<1> ivec(2,0)<1;1,0>
; i64 assembly: stride-2 writes interleave the two i32 rows into pair12.
; CHECK: mov (M1, 16) pair12_0v(0,0)<2> pack12_0(0,0)<1;1,0>
; CHECK: mov (M1, 16) pair12_0v(0,1)<2> pack12_0(1,0)<1;1,0>
; Non-contiguous [3,0]: pack30_0 row 0 = ivec row 3, row 1 = ivec row 0.
; CHECK: mov (M1, 16) pack30_0(0,0)<1> ivec(3,0)<1;1,0>
; CHECK: mov (M1, 16) pack30_0(1,0)<1> ivec(0,0)<1;1,0>
; CHECK: mov (M1, 16) pair30_0v(0,0)<2> pack30_0(0,0)<1;1,0>
; CHECK: mov (M1, 16) pair30_0v(0,1)<2> pack30_0(1,0)<1;1,0>
; i64 addition operates on the fully assembled pair variables.
; CHECK: add (M1, 16) result64(0,0)<1> pair12(0,0)<1;1,0> pair30(0,0)<1;1,0>
define spir_kernel void @test_extract_pair_bitcast_to_i64(float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  ; Integer DPAS produces a non-uniform <8 x i32>.
  ; bitcast of %acc prevents constant folding.
  %acc_i    = bitcast float %acc to i32
  %dpas_acc = insertelement <8 x i32> zeroinitializer, i32 %acc_i, i32 0
  %dpas_i32 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 4, i32 4, i32 8, i32 8, i1 false)
  %ivec = select i1 %cmp, <8 x i32> %dpas_i32, <8 x i32> zeroinitializer

  ; Contiguous pair: elements 1 and 2
  %e1 = extractelement <8 x i32> %ivec, i32 1
  %e2 = extractelement <8 x i32> %ivec, i32 2
  %pack12_0 = insertelement <2 x i32> undef, i32 %e1, i32 0
  %pack12   = insertelement <2 x i32> %pack12_0, i32 %e2, i32 1
  %pair12   = bitcast <2 x i32> %pack12 to i64

  ; Non-contiguous pair: elements 3 and 0 (stress-tests alignment — rows 3 and
  ; 0 are not adjacent in the <8 x i32> register layout)
  %e3 = extractelement <8 x i32> %ivec, i32 3
  %e0 = extractelement <8 x i32> %ivec, i32 0
  %pack30_0 = insertelement <2 x i32> undef, i32 %e3, i32 0
  %pack30   = insertelement <2 x i32> %pack30_0, i32 %e0, i32 1
  %pair30   = bitcast <2 x i32> %pack30 to i64

  %result64 = add i64 %pair12, %pair30
  %result_f = uitofp i64 %result64 to float
  %sum = fadd float %result_f, %acc

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; ── Uniform (kernel parameter) variants ─────────────────────────────────────

; CHECK-LABEL: .kernel "test_extract_bitcast_to_vec_type_uniform"
; <2 x i16> consumers use broadcast regions on vec_0v (word alias of vec).
; CHECK-NOT: mov (M1_NM, 1) {{.*}}(0,0)<1> vec(0,
; CHECK: add (M1_NM, 1) p0(0,0)<1> vec_0v(0,0)<0;1,0> vec_0v(0,1)<0;1,0>
; CHECK: add (M1_NM, 1) p1(0,0)<1> vec_0v(0,2)<0;1,0> vec_0v(0,3)<0;1,0>
; CHECK: add (M1_NM, 1) p2(0,0)<1> vec_0v(0,4)<0;1,0> vec_0v(0,5)<0;1,0>
; CHECK: add (M1_NM, 1) p3(0,0)<1> vec_0v(0,6)<0;1,0> vec_0v(0,7)<0;1,0>
; CHECK-NOT: mov (M1_NM, 1) {{.*}}(0,0)<1> vec(0,
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,0)<0;1,0> vec(0,0)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,1)<0;1,0> vec(0,1)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,2)<0;1,0> vec(0,2)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,3)<0;1,0> vec(0,3)<0;1,0>
define spir_kernel void @test_extract_bitcast_to_vec_type_uniform(
    <4 x float> %true_val, <4 x float> %false_val, float %cond_val) {
entry:
  br label %loop
loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x float> %true_val, <4 x float> %false_val
  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3
  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3
  %bc0_v = bitcast float %e0 to <2 x i16>
  %bc1_v = bitcast float %e1 to <2 x i16>
  %bc2_v = bitcast float %e2 to <2 x i16>
  %bc3_v = bitcast float %e3 to <2 x i16>
  %lo0 = extractelement <2 x i16> %bc0_v, i32 0
  %hi0 = extractelement <2 x i16> %bc0_v, i32 1
  %lo1 = extractelement <2 x i16> %bc1_v, i32 0
  %hi1 = extractelement <2 x i16> %bc1_v, i32 1
  %lo2 = extractelement <2 x i16> %bc2_v, i32 0
  %hi2 = extractelement <2 x i16> %bc2_v, i32 1
  %lo3 = extractelement <2 x i16> %bc3_v, i32 0
  %hi3 = extractelement <2 x i16> %bc3_v, i32 1
  %p0 = add i16 %lo0, %hi0
  %p1 = add i16 %lo1, %hi1
  %p2 = add i16 %lo2, %hi2
  %p3 = add i16 %lo3, %hi3
  %q0 = zext i16 %p0 to i32
  %q1 = zext i16 %p1 to i32
  %q2 = zext i16 %p2 to i32
  %q3 = zext i16 %p3 to i32
  %r0 = sitofp i32 %q0 to float
  %r1 = sitofp i32 %q1 to float
  %r2 = sitofp i32 %q2 to float
  %r3 = sitofp i32 %q3 to float
  %s0 = fadd float %f0, %r0
  %s1 = fadd float %f1, %r1
  %s2 = fadd float %f2, %r2
  %s3 = fadd float %f3, %r3
  %t0 = fadd float %s0, %s1
  %t1 = fadd float %s2, %s3
  %sum = fadd float %t0, %t1
  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit
exit:
  ret void
}

; CHECK-LABEL: .kernel "test_extract_mixed_bitcast_consumers_uniform"
; CHECK-NOT: mov (M1_NM, 1) e0(0,0)<1> vec(0,0)
; CHECK-NOT: mov (M1_NM, 1) e1(0,0)<1> vec(0,1)
; CHECK-NOT: mov (M1_NM, 1) e2(0,0)<1> vec(0,2)
; CHECK-NOT: mov (M1_NM, 1) e3(0,0)<1> vec(0,3)
; CHECK: add (M1_NM, 1) i0(0,0)<1> vec_0v(0,0)<0;1,0>
; CHECK: add (M1_NM, 1) p1(0,0)<1> vec_1v(0,2)<0;1,0> vec_1v(0,3)<0;1,0>
; CHECK: mov (M1_NM, 1) hf2(0,0)<1> vec(0,2)<0;1,0>
; CHECK: mov (M1_NM, 1) bf3(0,0)<1> vec(0,3)<0;1,0>
define spir_kernel void @test_extract_mixed_bitcast_consumers_uniform(
    <4 x float> %true_val, <4 x float> %false_val, float %cond_val) {
entry:
  br label %loop
loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x float> %true_val, <4 x float> %false_val
  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3
  %bc0 = bitcast float %e0 to i32
  %i0 = add i32 %bc0, 1
  %bc1_v = bitcast float %e1 to <2 x i16>
  %lo1 = extractelement <2 x i16> %bc1_v, i32 0
  %hi1 = extractelement <2 x i16> %bc1_v, i32 1
  %p1 = add i16 %lo1, %hi1
  %hf2 = fptrunc float %e2 to half
  %hf2_f = fpext half %hf2 to float
  %bf3 = fptrunc float %e3 to bfloat
  %bf3_f = fpext bfloat %bf3 to float
  %ib0 = bitcast i32 %i0 to float
  %p1_i32 = sext i16 %p1 to i32
  %p1_f = sitofp i32 %p1_i32 to float
  %s0 = fadd float %ib0, %p1_f
  %s1 = fadd float %hf2_f, %bf3_f
  %sum = fadd float %s0, %s1
  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit
exit:
  ret void
}

; CHECK-LABEL: .kernel "test_extract_bitcast_vec_insertelement_uniform"
; CHECK-NOT: mov (M1_NM, 1) {{.*}}(0,0)<1> vec(0,
; CHECK: mov (M1_NM, 1) bc0_v(0,0)<1> vec_0v(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) bc0_v(0,1)<1> vec_0v(0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) bc1_v(0,0)<1> vec_0v(0,2)<0;1,0>
; CHECK: mov (M1_NM, 1) bc1_v(0,1)<1> vec_0v(0,3)<0;1,0>
; CHECK: mov (M1_NM, 1) bc2_v(0,0)<1> vec_0v(0,4)<0;1,0>
; CHECK: mov (M1_NM, 1) bc2_v(0,1)<1> vec_0v(0,5)<0;1,0>
; CHECK: mov (M1_NM, 1) bc3_v(0,0)<1> vec_0v(0,6)<0;1,0>
; CHECK: mov (M1_NM, 1) bc3_v(0,1)<1> vec_0v(0,7)<0;1,0>
; CHECK: mov (M1_NM, 1) bc0_v(0,0)<1> patch(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) bc1_v(0,0)<1> patch(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) bc2_v(0,0)<1> patch(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) bc3_v(0,0)<1> patch(0,0)<0;1,0>
; CHECK-NOT: mov (M1_NM, 1) {{.*}}(0,0)<1> vec(0,
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,0)<0;1,0> vec(0,0)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,1)<0;1,0> vec(0,1)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,2)<0;1,0> vec(0,2)<0;1,0>
; CHECK: mad (M1_NM, 1) {{.*}} vec(0,3)<0;1,0> vec(0,3)<0;1,0>
define spir_kernel void @test_extract_bitcast_vec_insertelement_uniform(
    <4 x float> %true_val, <4 x float> %false_val, float %cond_val, i16 %patch) {
entry:
  br label %loop
loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x float> %true_val, <4 x float> %false_val
  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3
  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3
  %bc0_v = bitcast float %e0 to <2 x i16>
  %bc1_v = bitcast float %e1 to <2 x i16>
  %bc2_v = bitcast float %e2 to <2 x i16>
  %bc3_v = bitcast float %e3 to <2 x i16>
  %ins0 = insertelement <2 x i16> %bc0_v, i16 %patch, i32 0
  %ins1 = insertelement <2 x i16> %bc1_v, i16 %patch, i32 0
  %ins2 = insertelement <2 x i16> %bc2_v, i16 %patch, i32 0
  %ins3 = insertelement <2 x i16> %bc3_v, i16 %patch, i32 0
  %lo0 = extractelement <2 x i16> %ins0, i32 0
  %hi0 = extractelement <2 x i16> %ins0, i32 1
  %lo1 = extractelement <2 x i16> %ins1, i32 0
  %hi1 = extractelement <2 x i16> %ins1, i32 1
  %lo2 = extractelement <2 x i16> %ins2, i32 0
  %hi2 = extractelement <2 x i16> %ins2, i32 1
  %lo3 = extractelement <2 x i16> %ins3, i32 0
  %hi3 = extractelement <2 x i16> %ins3, i32 1
  %p0 = add i16 %lo0, %hi0
  %p1 = add i16 %lo1, %hi1
  %p2 = add i16 %lo2, %hi2
  %p3 = add i16 %lo3, %hi3
  %q0 = zext i16 %p0 to i32
  %q1 = zext i16 %p1 to i32
  %q2 = zext i16 %p2 to i32
  %q3 = zext i16 %p3 to i32
  %r0 = sitofp i32 %q0 to float
  %r1 = sitofp i32 %q1 to float
  %r2 = sitofp i32 %q2 to float
  %r3 = sitofp i32 %q3 to float
  %s0 = fadd float %f0, %r0
  %s1 = fadd float %f1, %r1
  %s2 = fadd float %f2, %r2
  %s3 = fadd float %f3, %r3
  %t0 = fadd float %s0, %s1
  %t1 = fadd float %s2, %s3
  %sum = fadd float %t0, %t1
  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit
exit:
  ret void
}

; CHECK-LABEL: .kernel "test_extract_pair_bitcast_to_i64_uniform"
; Uniform <8 x i32> from kernel parameters; all ops use M1_NM SIMD1 mode.
; ivec elements are accessed by column index within one row.
; CHECK-NOT: mov (M1_NM, 1) e1(0,0)<1> ivec(0,1)
; CHECK-NOT: mov (M1_NM, 1) e2(0,0)<1> ivec(0,2)
; CHECK-NOT: mov (M1_NM, 1) e3(0,0)<1> ivec(0,3)
; CHECK-NOT: mov (M1_NM, 1) e0(0,0)<1> ivec(0,0)
; Contiguous [1,2]: pack12_0 reads ivec columns 1 and 2 directly.
; CHECK: mov (M1_NM, 1) pack12_0(0,0)<1> ivec(0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) pack12_0(0,1)<1> ivec(0,2)<0;1,0>
; CHECK: mov (M1_NM, 1) pair12_0v(0,0)<1> pack12_0(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) pair12_0v(0,1)<1> pack12_0(0,1)<0;1,0>
; Non-contiguous [3,0]: pack30_0 reads ivec column 3 then column 0.
; CHECK: mov (M1_NM, 1) pack30_0(0,0)<1> ivec(0,3)<0;1,0>
; CHECK: mov (M1_NM, 1) pack30_0(0,1)<1> ivec(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) pair30_0v(0,0)<1> pack30_0(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) pair30_0v(0,1)<1> pack30_0(0,1)<0;1,0>
; CHECK: add (M1_NM, 1) result64(0,0)<1> pair12(0,0)<0;1,0> pair30(0,0)<0;1,0>
define spir_kernel void @test_extract_pair_bitcast_to_i64_uniform(
    <8 x i32> %true_val_i32, <8 x i32> %false_val_i32, float %cond_val) {
entry:
  br label %loop
loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %ivec = select i1 %cmp, <8 x i32> %true_val_i32, <8 x i32> %false_val_i32
  %e1 = extractelement <8 x i32> %ivec, i32 1
  %e2 = extractelement <8 x i32> %ivec, i32 2
  %pack12_0 = insertelement <2 x i32> undef, i32 %e1, i32 0
  %pack12   = insertelement <2 x i32> %pack12_0, i32 %e2, i32 1
  %pair12   = bitcast <2 x i32> %pack12 to i64
  %e3 = extractelement <8 x i32> %ivec, i32 3
  %e0 = extractelement <8 x i32> %ivec, i32 0
  %pack30_0 = insertelement <2 x i32> undef, i32 %e3, i32 0
  %pack30   = insertelement <2 x i32> %pack30_0, i32 %e0, i32 1
  %pair30   = bitcast <2 x i32> %pack30 to i64
  %result64 = add i64 %pair12, %pair30
  %result_f = uitofp i64 %result64 to float
  %sum = fadd float %result_f, %acc
  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit
exit:
  ret void
}

!igc.functions = !{!0, !10, !30, !40, !50, !60, !70, !80}
!IGCMetadata = !{!4}

!0 = !{ptr @test_extract_bitcast_to_vec_type, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7, !11, !12, !21, !22, !34, !35, !51, !52, !61, !62, !71, !72, !81, !82}
!6 = !{!"FuncMDMap[0]", ptr @test_extract_bitcast_to_vec_type}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !20}
!20 = !{!"argAllocMDListVec[0]"}
!10 = !{ptr @test_extract_mixed_bitcast_consumers, !13}
!11 = !{!"FuncMDMap[1]", ptr @test_extract_mixed_bitcast_consumers}
!12 = !{!"FuncMDValue[1]", !14}
!13 = !{!17, !18}
!14 = !{!"resAllocMD", !15}
!15 = !{!"argAllocMDList", !16}
!16 = !{!"argAllocMDListVec[0]"}
!17 = !{!"function_type", i32 0}
!18 = !{!"sub_group_size", i32 16}
!21 = !{!"FuncMDMap[2]", ptr @test_extract_bitcast_vec_insertelement}
!22 = !{!"FuncMDValue[2]", !23}
!23 = !{!"resAllocMD", !24}
!24 = !{!"argAllocMDList", !25}
!25 = !{!"argAllocMDListVec[0]"}

!30 = !{ptr @test_extract_bitcast_vec_insertelement, !31}
!31 = !{!32, !33}
!32 = !{!"function_type", i32 0}
!33 = !{!"sub_group_size", i32 16}
!34 = !{!"FuncMDMap[3]", ptr @test_extract_pair_bitcast_to_i64}
!35 = !{!"FuncMDValue[3]", !36}
!36 = !{!"resAllocMD", !37}
!37 = !{!"argAllocMDList", !38}
!38 = !{!"argAllocMDListVec[0]"}

!40 = !{ptr @test_extract_pair_bitcast_to_i64, !41}
!41 = !{!42, !43}
!42 = !{!"function_type", i32 0}
!43 = !{!"sub_group_size", i32 16}
!50 = !{ptr @test_extract_bitcast_to_vec_type_uniform, !31}
!51 = !{!"FuncMDMap[4]", ptr @test_extract_bitcast_to_vec_type_uniform}
!52 = !{!"FuncMDValue[4]", !53}
!53 = !{!"resAllocMD", !54}
!54 = !{!"argAllocMDList", !55}
!55 = !{!"argAllocMDListVec[0]"}
!60 = !{ptr @test_extract_mixed_bitcast_consumers_uniform, !31}
!61 = !{!"FuncMDMap[5]", ptr @test_extract_mixed_bitcast_consumers_uniform}
!62 = !{!"FuncMDValue[5]", !63}
!63 = !{!"resAllocMD", !64}
!64 = !{!"argAllocMDList", !65}
!65 = !{!"argAllocMDListVec[0]"}
!70 = !{ptr @test_extract_bitcast_vec_insertelement_uniform, !31}
!71 = !{!"FuncMDMap[6]", ptr @test_extract_bitcast_vec_insertelement_uniform}
!72 = !{!"FuncMDValue[6]", !73}
!73 = !{!"resAllocMD", !74}
!74 = !{!"argAllocMDList", !75}
!75 = !{!"argAllocMDListVec[0]"}
!80 = !{ptr @test_extract_pair_bitcast_to_i64_uniform, !31}
!81 = !{!"FuncMDMap[7]", ptr @test_extract_pair_bitcast_to_i64_uniform}
!82 = !{!"FuncMDValue[7]", !83}
!83 = !{!"resAllocMD", !84}
!84 = !{!"argAllocMDList", !85}
!85 = !{!"argAllocMDListVec[0]"}
