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

; Negative tests: CanTreatAsAlias must reject extracts when the source vector
; has any user that decomposes the vector or creates coalescing constraints.
;
; Each function tests one blocking type. Expected: extraction MOVs from vec
; are emitted in all cases.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

; -- BitCastInst: vector-level bitcast creates a DeSSA noop-alias on the whole
; vector, conflicting with per-element sub-register aliasing.
; CHECK-LABEL: .kernel "test_vec_bitcast_blocks"
; CHECK: sel (M1_NM, 4) vec
; CHECK: mov (M1_NM, 1) e0(0,0)<1> vec(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) e1(0,0)<1> vec(0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) e2(0,0)<1> vec(0,2)<0;1,0>
; CHECK: mov (M1_NM, 1) e3(0,0)<1> vec(0,3)<0;1,0>
define spir_kernel void @test_vec_bitcast_blocks(
    <4 x float> %true_val, <4 x float> %false_val, float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x float> %true_val, <4 x float> %false_val

  ; Vector-level bitcast: must block CanTreatAsAlias for all extracts
  %ivec = bitcast <4 x float> %vec to <4 x i32>
  %ie0 = extractelement <4 x i32> %ivec, i32 0

  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3
  %iefloat = bitcast i32 %ie0 to float
  %s0 = fadd float %f0, %f1
  %s1 = fadd float %f2, %f3
  %s2 = fadd float %s0, %s1
  %sum = fadd float %s2, %iefloat

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; -- InsertElementInst: uses the vector as a base, putting it in DeSSA InsEltMap
; and creating composite coalescing constraints.
; CHECK-LABEL: .kernel "test_insertelement_blocks"
; CHECK: sel (M1_NM, 4) vec
; CHECK: mov (M1_NM, 1) e0(0,0)<1> vec(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) e1(0,0)<1> vec(0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) e2(0,0)<1> vec(0,2)<0;1,0>
; CHECK: mov (M1_NM, 1) e3(0,0)<1> vec(0,3)<0;1,0>
define spir_kernel void @test_insertelement_blocks(
    <4 x float> %true_val, <4 x float> %false_val,
    float %cond_val, float %extra) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x float> %true_val, <4 x float> %false_val

  ; InsertElement using %vec as base: must block CanTreatAsAlias for extracts
  %modified = insertelement <4 x float> %vec, float %extra, i32 0
  %me0 = extractelement <4 x float> %modified, i32 0

  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3
  %s0 = fadd float %f0, %f1
  %s1 = fadd float %f2, %f3
  %s2 = fadd float %s0, %s1
  %sum = fadd float %s2, %me0

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; -- PtrToIntInst: in the reject list as a defensive measure;
; the extraction MOVs confirm aliasing is blocked.
; CHECK-LABEL: .kernel "test_ptrtoint_blocks"
; CHECK: sel (M1_NM, 4) vec
; CHECK: mov (M1_NM, 1) vec_0v(0,0)<1> vec(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) vec_1v(0,0)<1> vec(0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) vec_2v(0,0)<1> vec(0,2)<0;1,0>
; CHECK: mov (M1_NM, 1) vec_3v(0,0)<1> vec(0,3)<0;1,0>
define spir_kernel void @test_ptrtoint_blocks(
    <4 x ptr addrspace(1)> %true_val, <4 x ptr addrspace(1)> %false_val,
    float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x ptr addrspace(1)> %true_val, <4 x ptr addrspace(1)> %false_val

  ; Vector-level ptrtoint: must block CanTreatAsAlias for all extracts
  %addrs = ptrtoint <4 x ptr addrspace(1)> %vec to <4 x i64>
  %a0 = extractelement <4 x i64> %addrs, i32 0

  %e0 = extractelement <4 x ptr addrspace(1)> %vec, i32 0
  %e1 = extractelement <4 x ptr addrspace(1)> %vec, i32 1
  %e2 = extractelement <4 x ptr addrspace(1)> %vec, i32 2
  %e3 = extractelement <4 x ptr addrspace(1)> %vec, i32 3

  %l0 = load float, ptr addrspace(1) %e0
  %l1 = load float, ptr addrspace(1) %e1
  %l2 = load float, ptr addrspace(1) %e2
  %l3 = load float, ptr addrspace(1) %e3

  %f_a = uitofp i64 %a0 to float
  %s0 = fadd float %l0, %l1
  %s1 = fadd float %l2, %l3
  %s2 = fadd float %s0, %s1
  %sum = fadd float %s2, %f_a

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; -- PHINode: may pull the vector into a DeSSA congruent class,
; co-determining its register assignment.
; CHECK-LABEL: .kernel "test_phi_blocks"
; CHECK: sel (M1_NM, 4) vec
; CHECK: mov (M1_NM, 1) e0(0,0)<1> vec(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) e1(0,0)<1> vec(0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) e2(0,0)<1> vec(0,2)<0;1,0>
; CHECK: mov (M1_NM, 1) e3(0,0)<1> vec(0,3)<0;1,0>
define spir_kernel void @test_phi_blocks(
    <4 x float> %true_val, <4 x float> %false_val, float %cond_val) {
entry:
  br label %loop

loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  ; PHI with %vec as incoming value: must block CanTreatAsAlias for extracts
  %vec_phi = phi <4 x float> [ zeroinitializer, %entry ], [ %vec, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %vec = select i1 %cmp, <4 x float> %true_val, <4 x float> %false_val

  %e0 = extractelement <4 x float> %vec, i32 0
  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2
  %e3 = extractelement <4 x float> %vec, i32 3

  %ep0 = extractelement <4 x float> %vec_phi, i32 0

  %f0 = fmul float %e0, %e0
  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2
  %f3 = fmul float %e3, %e3
  %s0 = fadd float %f0, %f1
  %s1 = fadd float %f2, %f3
  %s2 = fadd float %s0, %s1
  %sum = fadd float %s2, %ep0

  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit

exit:
  ret void
}

; ── DPAS variants of blocked tests (non-uniform select operands) ─────────────
; Note: ptrtoint blocked test has no DPAS variant — the source type <4 x ptr>
; cannot be produced directly from float DPAS without coalescing the source vec.

; CHECK-LABEL: .kernel "test_vec_bitcast_blocks_dpas"
; CHECK: sel (M1, 16) vec
; CHECK: mov (M1, 16) e0(0,0)<1> vec(0,0)<1;1,0>
; CHECK: mov (M1, 16) e1(0,0)<1> vec(1,0)<1;1,0>
; CHECK: mov (M1, 16) e2(0,0)<1> vec(2,0)<1;1,0>
; CHECK: mov (M1, 16) e3(0,0)<1> vec(3,0)<1;1,0>
define spir_kernel void @test_vec_bitcast_blocks_dpas(float %cond_val) {
entry:
  br label %loop
loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %dpas_acc = insertelement <8 x float> zeroinitializer, float %acc, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %d0 = extractelement <8 x float> %dpas, i32 0  %d1 = extractelement <8 x float> %dpas, i32 1
  %d2 = extractelement <8 x float> %dpas, i32 2  %d3 = extractelement <8 x float> %dpas, i32 3
  %d4 = extractelement <8 x float> %dpas, i32 4  %d5 = extractelement <8 x float> %dpas, i32 5
  %d6 = extractelement <8 x float> %dpas, i32 6  %d7 = extractelement <8 x float> %dpas, i32 7
  %tv0 = insertelement <4 x float> undef, float %d0, i32 0
  %tv1 = insertelement <4 x float> %tv0, float %d1, i32 1
  %tv2 = insertelement <4 x float> %tv1, float %d2, i32 2
  %true_val_v  = insertelement <4 x float> %tv2, float %d3, i32 3
  %fv0 = insertelement <4 x float> undef, float %d4, i32 0
  %fv1 = insertelement <4 x float> %fv0, float %d5, i32 1
  %fv2 = insertelement <4 x float> %fv1, float %d6, i32 2
  %false_val_v = insertelement <4 x float> %fv2, float %d7, i32 3
  %vec = select i1 %cmp, <4 x float> %true_val_v, <4 x float> %false_val_v
  %ivec = bitcast <4 x float> %vec to <4 x i32>
  %ie0 = extractelement <4 x i32> %ivec, i32 0
  %e0 = extractelement <4 x float> %vec, i32 0  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2  %e3 = extractelement <4 x float> %vec, i32 3
  %f0 = fmul float %e0, %e0  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2  %f3 = fmul float %e3, %e3
  %iefloat = bitcast i32 %ie0 to float
  %s0 = fadd float %f0, %f1  %s1 = fadd float %f2, %f3
  %s2 = fadd float %s0, %s1  %sum = fadd float %s2, %iefloat
  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit
exit:
  ret void
}

; CHECK-LABEL: .kernel "test_insertelement_blocks_dpas"
; CHECK: sel (M1, 16) vec
; CHECK: mov (M1, 16) e0(0,0)<1> vec(0,0)<1;1,0>
; CHECK: mov (M1, 16) e1(0,0)<1> vec(1,0)<1;1,0>
; CHECK: mov (M1, 16) e2(0,0)<1> vec(2,0)<1;1,0>
; CHECK: mov (M1, 16) e3(0,0)<1> vec(3,0)<1;1,0>
define spir_kernel void @test_insertelement_blocks_dpas(float %cond_val, float %extra) {
entry:
  br label %loop
loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %dpas_acc = insertelement <8 x float> zeroinitializer, float %acc, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %d0 = extractelement <8 x float> %dpas, i32 0  %d1 = extractelement <8 x float> %dpas, i32 1
  %d2 = extractelement <8 x float> %dpas, i32 2  %d3 = extractelement <8 x float> %dpas, i32 3
  %d4 = extractelement <8 x float> %dpas, i32 4  %d5 = extractelement <8 x float> %dpas, i32 5
  %d6 = extractelement <8 x float> %dpas, i32 6  %d7 = extractelement <8 x float> %dpas, i32 7
  %tv0 = insertelement <4 x float> undef, float %d0, i32 0
  %tv1 = insertelement <4 x float> %tv0, float %d1, i32 1
  %tv2 = insertelement <4 x float> %tv1, float %d2, i32 2
  %true_val_v  = insertelement <4 x float> %tv2, float %d3, i32 3
  %fv0 = insertelement <4 x float> undef, float %d4, i32 0
  %fv1 = insertelement <4 x float> %fv0, float %d5, i32 1
  %fv2 = insertelement <4 x float> %fv1, float %d6, i32 2
  %false_val_v = insertelement <4 x float> %fv2, float %d7, i32 3
  %vec = select i1 %cmp, <4 x float> %true_val_v, <4 x float> %false_val_v
  %modified = insertelement <4 x float> %vec, float %extra, i32 0
  %me0 = extractelement <4 x float> %modified, i32 0
  %e0 = extractelement <4 x float> %vec, i32 0  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2  %e3 = extractelement <4 x float> %vec, i32 3
  %f0 = fmul float %e0, %e0  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2  %f3 = fmul float %e3, %e3
  %s0 = fadd float %f0, %f1  %s1 = fadd float %f2, %f3
  %s2 = fadd float %s0, %s1  %sum = fadd float %s2, %me0
  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit
exit:
  ret void
}

; CHECK-LABEL: .kernel "test_phi_blocks_dpas"
; CHECK: sel (M1, 16) vec
; CHECK: mov (M1, 16) e0(0,0)<1> vec(0,0)<1;1,0>
; CHECK: mov (M1, 16) e1(0,0)<1> vec(1,0)<1;1,0>
; CHECK: mov (M1, 16) e2(0,0)<1> vec(2,0)<1;1,0>
; CHECK: mov (M1, 16) e3(0,0)<1> vec(3,0)<1;1,0>
define spir_kernel void @test_phi_blocks_dpas(float %cond_val) {
entry:
  br label %loop
loop:
  %acc = phi float [ 0.000000e+00, %entry ], [ %sum, %loop ]
  %vec_phi = phi <4 x float> [ zeroinitializer, %entry ], [ %vec, %loop ]
  %cmp = fcmp ogt float %cond_val, %acc
  %dpas_acc = insertelement <8 x float> zeroinitializer, float %acc, i32 0
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_acc, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 12, i32 12, i32 8, i32 8, i1 false)
  %d0 = extractelement <8 x float> %dpas, i32 0  %d1 = extractelement <8 x float> %dpas, i32 1
  %d2 = extractelement <8 x float> %dpas, i32 2  %d3 = extractelement <8 x float> %dpas, i32 3
  %d4 = extractelement <8 x float> %dpas, i32 4  %d5 = extractelement <8 x float> %dpas, i32 5
  %d6 = extractelement <8 x float> %dpas, i32 6  %d7 = extractelement <8 x float> %dpas, i32 7
  %tv0 = insertelement <4 x float> undef, float %d0, i32 0
  %tv1 = insertelement <4 x float> %tv0, float %d1, i32 1
  %tv2 = insertelement <4 x float> %tv1, float %d2, i32 2
  %true_val_v  = insertelement <4 x float> %tv2, float %d3, i32 3
  %fv0 = insertelement <4 x float> undef, float %d4, i32 0
  %fv1 = insertelement <4 x float> %fv0, float %d5, i32 1
  %fv2 = insertelement <4 x float> %fv1, float %d6, i32 2
  %false_val_v = insertelement <4 x float> %fv2, float %d7, i32 3
  %vec = select i1 %cmp, <4 x float> %true_val_v, <4 x float> %false_val_v
  %e0 = extractelement <4 x float> %vec, i32 0  %e1 = extractelement <4 x float> %vec, i32 1
  %e2 = extractelement <4 x float> %vec, i32 2  %e3 = extractelement <4 x float> %vec, i32 3
  %ep0 = extractelement <4 x float> %vec_phi, i32 0
  %f0 = fmul float %e0, %e0  %f1 = fmul float %e1, %e1
  %f2 = fmul float %e2, %e2  %f3 = fmul float %e3, %e3
  %s0 = fadd float %f0, %f1  %s1 = fadd float %f2, %f3
  %s2 = fadd float %s0, %s1  %sum = fadd float %s2, %ep0
  %loop_cond = fcmp olt float %sum, 1.000000e+10
  br i1 %loop_cond, label %loop, label %exit
exit:
  ret void
}

!igc.functions = !{!0, !10, !20, !50, !60, !70, !80}
!IGCMetadata = !{!30}

!0  = !{ptr @test_vec_bitcast_blocks, !1}
!1  = !{!2, !3}
!2  = !{!"function_type", i32 0}
!3  = !{!"sub_group_size", i32 16}

!10 = !{ptr @test_insertelement_blocks, !11}
!11 = !{!12, !13}
!12 = !{!"function_type", i32 0}
!13 = !{!"sub_group_size", i32 16}

!20 = !{ptr @test_ptrtoint_blocks, !21}
!21 = !{!22, !23}
!22 = !{!"function_type", i32 0}
!23 = !{!"sub_group_size", i32 16}

!30 = !{!"ModuleMD", !31}
!31 = !{!"FuncMD", !32, !33, !37, !38, !42, !43, !47, !48, !61, !62, !71, !72, !81, !82}
!32 = !{!"FuncMDMap[0]", ptr @test_vec_bitcast_blocks}
!33 = !{!"FuncMDValue[0]", !34}
!34 = !{!"resAllocMD", !35}
!35 = !{!"argAllocMDList", !36}
!36 = !{!"argAllocMDListVec[0]"}
!37 = !{!"FuncMDMap[1]", ptr @test_insertelement_blocks}
!38 = !{!"FuncMDValue[1]", !39}
!39 = !{!"resAllocMD", !40}
!40 = !{!"argAllocMDList", !41}
!41 = !{!"argAllocMDListVec[0]"}
!42 = !{!"FuncMDMap[2]", ptr @test_ptrtoint_blocks}
!43 = !{!"FuncMDValue[2]", !44}
!44 = !{!"resAllocMD", !45}
!45 = !{!"argAllocMDList", !46}
!46 = !{!"argAllocMDListVec[0]"}

!50 = !{ptr @test_phi_blocks, !51}
!51 = !{!52, !53}
!52 = !{!"function_type", i32 0}
!53 = !{!"sub_group_size", i32 16}

!47 = !{!"FuncMDMap[3]", ptr @test_phi_blocks}
!48 = !{!"FuncMDValue[3]", !49}
!49 = !{!"resAllocMD", !54}
!54 = !{!"argAllocMDList", !55}
!55 = !{!"argAllocMDListVec[0]"}

!60 = !{ptr @test_vec_bitcast_blocks_dpas, !1}
!61 = !{!"FuncMDMap[4]", ptr @test_vec_bitcast_blocks_dpas}
!62 = !{!"FuncMDValue[4]", !63}
!63 = !{!"resAllocMD", !64}
!64 = !{!"argAllocMDList", !65}
!65 = !{!"argAllocMDListVec[0]"}

!70 = !{ptr @test_insertelement_blocks_dpas, !1}
!71 = !{!"FuncMDMap[5]", ptr @test_insertelement_blocks_dpas}
!72 = !{!"FuncMDValue[5]", !73}
!73 = !{!"resAllocMD", !74}
!74 = !{!"argAllocMDList", !75}
!75 = !{!"argAllocMDListVec[0]"}

!80 = !{ptr @test_phi_blocks_dpas, !1}
!81 = !{!"FuncMDMap[6]", ptr @test_phi_blocks_dpas}
!82 = !{!"FuncMDValue[6]", !83}
!83 = !{!"resAllocMD", !84}
!84 = !{!"argAllocMDList", !85}
!85 = !{!"argAllocMDListVec[0]"}
