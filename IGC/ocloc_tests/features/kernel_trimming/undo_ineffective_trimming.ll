;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; Kernel trimming that provably cannot reach its size target must be rolled back
; (intel/intel-graphics-compiler#420).
;
; checkSubroutine() calls reduceKernelSize() once a unit's fully inlined size
; exceeds SubroutineThreshold. reduceKernelSize() then marks candidate functions
; noinline until the unit drops below KernelTotalSizeThreshold. When every
; candidate has been consumed and the unit is still above that threshold, the
; trimming missed the objective it exists to serve while still turning on
; subroutine mode for the whole module, deferring inlining to SubroutineInliner
; and paying the compile-time and register-pressure cost for nothing.
;
; The unit below is built so that trimming cannot help: three helpers, each
; called five times, and the sum of the three helper bodies is on its own above
; the trimming target. Sizes are reached through the loop trip count rather than
; through a huge .ll: with LoopCountAwareTrimming a basic block counts
; blockSize * tripCount, so each helper is worth about 11200 and the fully
; inlined unit about 168000, which is above the *default* SubroutineThreshold of
; 110000. Every regkey used here is declared releaseMode=true, so the test is
; meaningful in a Release build.
;
; Three configurations are checked, all on the same module:
;   1. rollback disabled                     -> trimming is kept (old behaviour)
;   2. rollback enabled                      -> trimming is rolled back
;   3. rollback enabled, but the unit is above the large-kernel threshold
;      (KernelTotalSizeThreshold * LargeKernelThresholdMultiplier, the
;      multiplier being 12 by default)       -> trimming is kept
;
; REQUIRES: regkeys, dg2-supported, llvm-16-plus
;
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
;
; RUN: ocloc compile -llvm_input -file %t.bc -device dg2 \
; RUN:   -options "-igc_opts 'PrintControlKernelTotalSize=2,LoopCountAwareTrimming=1,MaxUnrollCountForFunctionSizeAnalysis=200,KernelTotalSizeThreshold=20000,UndoIneffectiveKernelTrimming=0'" \
; RUN:   2>&1 | FileCheck %s --check-prefix=CHECK-KEEP
;
; RUN: ocloc compile -llvm_input -file %t.bc -device dg2 \
; RUN:   -options "-igc_opts 'PrintControlKernelTotalSize=2,LoopCountAwareTrimming=1,MaxUnrollCountForFunctionSizeAnalysis=200,KernelTotalSizeThreshold=20000,UndoIneffectiveKernelTrimming=1'" \
; RUN:   2>&1 | FileCheck %s --check-prefix=CHECK-UNDO
;
; RUN: ocloc compile -llvm_input -file %t.bc -device dg2 \
; RUN:   -options "-igc_opts 'PrintControlKernelTotalSize=2,LoopCountAwareTrimming=1,MaxUnrollCountForFunctionSizeAnalysis=200,KernelTotalSizeThreshold=10000,UndoIneffectiveKernelTrimming=1'" \
; RUN:   2>&1 | FileCheck %s --check-prefix=CHECK-KEEP

; COM: old behaviour, and the large-kernel exclusion: the decisions stand even
; COM: though the threshold was never reached.
; CHECK-KEEP: The size is still above threshold even though all candidates are trimmed
; CHECK-KEEP-NOT: Rolling back

; COM: new behaviour: every decision is restored and the unit stays inlined.
; CHECK-UNDO: trimming cannot reach the threshold (20000)
; CHECK-UNDO-SAME: Rolling back 3 trimming decision(s) and keeping the unit inlined.
; CHECK-UNDO-NOT: The size is still above threshold even though all candidates are trimmed



target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"



define internal spir_func void @helper_alpha(ptr addrspace(1) %out) #1 {
entry:
  %seed = load i32, ptr addrspace(1) %out, align 4
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inext, %loop ]
  %acc = phi i32 [ %seed, %entry ], [ %v50, %loop ]
  %v1 = add i32 %acc, 1
  %v2 = xor i32 %v1, 7
  %v3 = mul i32 %v2, 3
  %v4 = sub i32 %v3, 5
  %v5 = shl i32 %v4, 1
  %v6 = or i32 %v5, 9
  %v7 = ashr i32 %v6, 1
  %v8 = and i32 %v7, 65535
  %v9 = add i32 %v8, 1
  %v10 = xor i32 %v9, 7
  %v11 = mul i32 %v10, 3
  %v12 = sub i32 %v11, 5
  %v13 = shl i32 %v12, 1
  %v14 = or i32 %v13, 9
  %v15 = ashr i32 %v14, 1
  %v16 = and i32 %v15, 65535
  %v17 = add i32 %v16, 1
  %v18 = xor i32 %v17, 7
  %v19 = mul i32 %v18, 3
  %v20 = sub i32 %v19, 5
  %v21 = shl i32 %v20, 1
  %v22 = or i32 %v21, 9
  %v23 = ashr i32 %v22, 1
  %v24 = and i32 %v23, 65535
  %v25 = add i32 %v24, 1
  %v26 = xor i32 %v25, 7
  %v27 = mul i32 %v26, 3
  %v28 = sub i32 %v27, 5
  %v29 = shl i32 %v28, 1
  %v30 = or i32 %v29, 9
  %v31 = ashr i32 %v30, 1
  %v32 = and i32 %v31, 65535
  %v33 = add i32 %v32, 1
  %v34 = xor i32 %v33, 7
  %v35 = mul i32 %v34, 3
  %v36 = sub i32 %v35, 5
  %v37 = shl i32 %v36, 1
  %v38 = or i32 %v37, 9
  %v39 = ashr i32 %v38, 1
  %v40 = and i32 %v39, 65535
  %v41 = add i32 %v40, 1
  %v42 = xor i32 %v41, 7
  %v43 = mul i32 %v42, 3
  %v44 = sub i32 %v43, 5
  %v45 = shl i32 %v44, 1
  %v46 = or i32 %v45, 9
  %v47 = ashr i32 %v46, 1
  %v48 = and i32 %v47, 65535
  %v49 = add i32 %v48, 1
  %v50 = xor i32 %v49, 7
  store i32 %v50, ptr addrspace(1) %out, align 4
  %inext = add i32 %i, 1
  %cmp = icmp slt i32 %inext, 200
  br i1 %cmp, label %loop, label %exit

exit:
  ret void
}

define internal spir_func void @helper_beta(ptr addrspace(1) %out) #1 {
entry:
  %seed = load i32, ptr addrspace(1) %out, align 4
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inext, %loop ]
  %acc = phi i32 [ %seed, %entry ], [ %v50, %loop ]
  %v1 = add i32 %acc, 1
  %v2 = xor i32 %v1, 7
  %v3 = mul i32 %v2, 3
  %v4 = sub i32 %v3, 5
  %v5 = shl i32 %v4, 1
  %v6 = or i32 %v5, 9
  %v7 = ashr i32 %v6, 1
  %v8 = and i32 %v7, 65535
  %v9 = add i32 %v8, 1
  %v10 = xor i32 %v9, 7
  %v11 = mul i32 %v10, 3
  %v12 = sub i32 %v11, 5
  %v13 = shl i32 %v12, 1
  %v14 = or i32 %v13, 9
  %v15 = ashr i32 %v14, 1
  %v16 = and i32 %v15, 65535
  %v17 = add i32 %v16, 1
  %v18 = xor i32 %v17, 7
  %v19 = mul i32 %v18, 3
  %v20 = sub i32 %v19, 5
  %v21 = shl i32 %v20, 1
  %v22 = or i32 %v21, 9
  %v23 = ashr i32 %v22, 1
  %v24 = and i32 %v23, 65535
  %v25 = add i32 %v24, 1
  %v26 = xor i32 %v25, 7
  %v27 = mul i32 %v26, 3
  %v28 = sub i32 %v27, 5
  %v29 = shl i32 %v28, 1
  %v30 = or i32 %v29, 9
  %v31 = ashr i32 %v30, 1
  %v32 = and i32 %v31, 65535
  %v33 = add i32 %v32, 1
  %v34 = xor i32 %v33, 7
  %v35 = mul i32 %v34, 3
  %v36 = sub i32 %v35, 5
  %v37 = shl i32 %v36, 1
  %v38 = or i32 %v37, 9
  %v39 = ashr i32 %v38, 1
  %v40 = and i32 %v39, 65535
  %v41 = add i32 %v40, 1
  %v42 = xor i32 %v41, 7
  %v43 = mul i32 %v42, 3
  %v44 = sub i32 %v43, 5
  %v45 = shl i32 %v44, 1
  %v46 = or i32 %v45, 9
  %v47 = ashr i32 %v46, 1
  %v48 = and i32 %v47, 65535
  %v49 = add i32 %v48, 1
  %v50 = xor i32 %v49, 7
  store i32 %v50, ptr addrspace(1) %out, align 4
  %inext = add i32 %i, 1
  %cmp = icmp slt i32 %inext, 200
  br i1 %cmp, label %loop, label %exit

exit:
  ret void
}

define internal spir_func void @helper_gamma(ptr addrspace(1) %out) #1 {
entry:
  %seed = load i32, ptr addrspace(1) %out, align 4
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inext, %loop ]
  %acc = phi i32 [ %seed, %entry ], [ %v50, %loop ]
  %v1 = add i32 %acc, 1
  %v2 = xor i32 %v1, 7
  %v3 = mul i32 %v2, 3
  %v4 = sub i32 %v3, 5
  %v5 = shl i32 %v4, 1
  %v6 = or i32 %v5, 9
  %v7 = ashr i32 %v6, 1
  %v8 = and i32 %v7, 65535
  %v9 = add i32 %v8, 1
  %v10 = xor i32 %v9, 7
  %v11 = mul i32 %v10, 3
  %v12 = sub i32 %v11, 5
  %v13 = shl i32 %v12, 1
  %v14 = or i32 %v13, 9
  %v15 = ashr i32 %v14, 1
  %v16 = and i32 %v15, 65535
  %v17 = add i32 %v16, 1
  %v18 = xor i32 %v17, 7
  %v19 = mul i32 %v18, 3
  %v20 = sub i32 %v19, 5
  %v21 = shl i32 %v20, 1
  %v22 = or i32 %v21, 9
  %v23 = ashr i32 %v22, 1
  %v24 = and i32 %v23, 65535
  %v25 = add i32 %v24, 1
  %v26 = xor i32 %v25, 7
  %v27 = mul i32 %v26, 3
  %v28 = sub i32 %v27, 5
  %v29 = shl i32 %v28, 1
  %v30 = or i32 %v29, 9
  %v31 = ashr i32 %v30, 1
  %v32 = and i32 %v31, 65535
  %v33 = add i32 %v32, 1
  %v34 = xor i32 %v33, 7
  %v35 = mul i32 %v34, 3
  %v36 = sub i32 %v35, 5
  %v37 = shl i32 %v36, 1
  %v38 = or i32 %v37, 9
  %v39 = ashr i32 %v38, 1
  %v40 = and i32 %v39, 65535
  %v41 = add i32 %v40, 1
  %v42 = xor i32 %v41, 7
  %v43 = mul i32 %v42, 3
  %v44 = sub i32 %v43, 5
  %v45 = shl i32 %v44, 1
  %v46 = or i32 %v45, 9
  %v47 = ashr i32 %v46, 1
  %v48 = and i32 %v47, 65535
  %v49 = add i32 %v48, 1
  %v50 = xor i32 %v49, 7
  store i32 %v50, ptr addrspace(1) %out, align 4
  %inext = add i32 %i, 1
  %cmp = icmp slt i32 %inext, 200
  br i1 %cmp, label %loop, label %exit

exit:
  ret void
}

define spir_kernel void @trim_target(ptr addrspace(1) %out) #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !1 !kernel_arg_type !2 !kernel_arg_base_type !2 !kernel_arg_type_qual !3 {
entry:
  call spir_func void @helper_alpha(ptr addrspace(1) %out)
  call spir_func void @helper_beta(ptr addrspace(1) %out)
  call spir_func void @helper_gamma(ptr addrspace(1) %out)
  call spir_func void @helper_alpha(ptr addrspace(1) %out)
  call spir_func void @helper_beta(ptr addrspace(1) %out)
  call spir_func void @helper_gamma(ptr addrspace(1) %out)
  call spir_func void @helper_alpha(ptr addrspace(1) %out)
  call spir_func void @helper_beta(ptr addrspace(1) %out)
  call spir_func void @helper_gamma(ptr addrspace(1) %out)
  call spir_func void @helper_alpha(ptr addrspace(1) %out)
  call spir_func void @helper_beta(ptr addrspace(1) %out)
  call spir_func void @helper_gamma(ptr addrspace(1) %out)
  call spir_func void @helper_alpha(ptr addrspace(1) %out)
  call spir_func void @helper_beta(ptr addrspace(1) %out)
  call spir_func void @helper_gamma(ptr addrspace(1) %out)
  ret void
}

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind }

!0 = !{i32 1}
!1 = !{!"none"}
!2 = !{!"int*"}
!3 = !{!""}
