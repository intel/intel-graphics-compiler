;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; On XE2 a kernel whose SIMD32 pressure is over the 128 GRF budget is dropped to SIMD16. The 72 live accumulators
; keep the pressure over the budget no matter how the two addresses are computed, so the first-compile remat is
; rolled back. The same remat is kept when it is not a first-compile run.

; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=1 -remat-widest-simd --dce | FileCheck %s --check-prefix=KEEP
; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=1 -remat-widest-simd -remat-first-compile --dce | FileCheck %s --check-prefix=ROLLBACK

; KEEP: !remat
; ROLLBACK-NOT: !remat
define spir_kernel void @main(ptr addrspace(3) %slm, ptr addrspace(1) %out, i32 %n, i32 %tid) {
entry:
  br label %loop

loop:
  %idx = phi i32 [ %tid, %entry ], [ %next, %loop ]
  %acc0 = phi float [ 0.0, %entry ], [ %nacc0, %loop ]
  %acc1 = phi float [ 0.0, %entry ], [ %nacc1, %loop ]
  %acc2 = phi float [ 0.0, %entry ], [ %nacc2, %loop ]
  %acc3 = phi float [ 0.0, %entry ], [ %nacc3, %loop ]
  %acc4 = phi float [ 0.0, %entry ], [ %nacc4, %loop ]
  %acc5 = phi float [ 0.0, %entry ], [ %nacc5, %loop ]
  %acc6 = phi float [ 0.0, %entry ], [ %nacc6, %loop ]
  %acc7 = phi float [ 0.0, %entry ], [ %nacc7, %loop ]
  %acc8 = phi float [ 0.0, %entry ], [ %nacc8, %loop ]
  %acc9 = phi float [ 0.0, %entry ], [ %nacc9, %loop ]
  %acc10 = phi float [ 0.0, %entry ], [ %nacc10, %loop ]
  %acc11 = phi float [ 0.0, %entry ], [ %nacc11, %loop ]
  %acc12 = phi float [ 0.0, %entry ], [ %nacc12, %loop ]
  %acc13 = phi float [ 0.0, %entry ], [ %nacc13, %loop ]
  %acc14 = phi float [ 0.0, %entry ], [ %nacc14, %loop ]
  %acc15 = phi float [ 0.0, %entry ], [ %nacc15, %loop ]
  %acc16 = phi float [ 0.0, %entry ], [ %nacc16, %loop ]
  %acc17 = phi float [ 0.0, %entry ], [ %nacc17, %loop ]
  %acc18 = phi float [ 0.0, %entry ], [ %nacc18, %loop ]
  %acc19 = phi float [ 0.0, %entry ], [ %nacc19, %loop ]
  %acc20 = phi float [ 0.0, %entry ], [ %nacc20, %loop ]
  %acc21 = phi float [ 0.0, %entry ], [ %nacc21, %loop ]
  %acc22 = phi float [ 0.0, %entry ], [ %nacc22, %loop ]
  %acc23 = phi float [ 0.0, %entry ], [ %nacc23, %loop ]
  %acc24 = phi float [ 0.0, %entry ], [ %nacc24, %loop ]
  %acc25 = phi float [ 0.0, %entry ], [ %nacc25, %loop ]
  %acc26 = phi float [ 0.0, %entry ], [ %nacc26, %loop ]
  %acc27 = phi float [ 0.0, %entry ], [ %nacc27, %loop ]
  %acc28 = phi float [ 0.0, %entry ], [ %nacc28, %loop ]
  %acc29 = phi float [ 0.0, %entry ], [ %nacc29, %loop ]
  %acc30 = phi float [ 0.0, %entry ], [ %nacc30, %loop ]
  %acc31 = phi float [ 0.0, %entry ], [ %nacc31, %loop ]
  %acc32 = phi float [ 0.0, %entry ], [ %nacc32, %loop ]
  %acc33 = phi float [ 0.0, %entry ], [ %nacc33, %loop ]
  %acc34 = phi float [ 0.0, %entry ], [ %nacc34, %loop ]
  %acc35 = phi float [ 0.0, %entry ], [ %nacc35, %loop ]
  %acc36 = phi float [ 0.0, %entry ], [ %nacc36, %loop ]
  %acc37 = phi float [ 0.0, %entry ], [ %nacc37, %loop ]
  %acc38 = phi float [ 0.0, %entry ], [ %nacc38, %loop ]
  %acc39 = phi float [ 0.0, %entry ], [ %nacc39, %loop ]
  %acc40 = phi float [ 0.0, %entry ], [ %nacc40, %loop ]
  %acc41 = phi float [ 0.0, %entry ], [ %nacc41, %loop ]
  %acc42 = phi float [ 0.0, %entry ], [ %nacc42, %loop ]
  %acc43 = phi float [ 0.0, %entry ], [ %nacc43, %loop ]
  %acc44 = phi float [ 0.0, %entry ], [ %nacc44, %loop ]
  %acc45 = phi float [ 0.0, %entry ], [ %nacc45, %loop ]
  %acc46 = phi float [ 0.0, %entry ], [ %nacc46, %loop ]
  %acc47 = phi float [ 0.0, %entry ], [ %nacc47, %loop ]
  %acc48 = phi float [ 0.0, %entry ], [ %nacc48, %loop ]
  %acc49 = phi float [ 0.0, %entry ], [ %nacc49, %loop ]
  %acc50 = phi float [ 0.0, %entry ], [ %nacc50, %loop ]
  %acc51 = phi float [ 0.0, %entry ], [ %nacc51, %loop ]
  %acc52 = phi float [ 0.0, %entry ], [ %nacc52, %loop ]
  %acc53 = phi float [ 0.0, %entry ], [ %nacc53, %loop ]
  %acc54 = phi float [ 0.0, %entry ], [ %nacc54, %loop ]
  %acc55 = phi float [ 0.0, %entry ], [ %nacc55, %loop ]
  %acc56 = phi float [ 0.0, %entry ], [ %nacc56, %loop ]
  %acc57 = phi float [ 0.0, %entry ], [ %nacc57, %loop ]
  %acc58 = phi float [ 0.0, %entry ], [ %nacc58, %loop ]
  %acc59 = phi float [ 0.0, %entry ], [ %nacc59, %loop ]
  %acc60 = phi float [ 0.0, %entry ], [ %nacc60, %loop ]
  %acc61 = phi float [ 0.0, %entry ], [ %nacc61, %loop ]
  %acc62 = phi float [ 0.0, %entry ], [ %nacc62, %loop ]
  %acc63 = phi float [ 0.0, %entry ], [ %nacc63, %loop ]
  %acc64 = phi float [ 0.0, %entry ], [ %nacc64, %loop ]
  %acc65 = phi float [ 0.0, %entry ], [ %nacc65, %loop ]
  %acc66 = phi float [ 0.0, %entry ], [ %nacc66, %loop ]
  %acc67 = phi float [ 0.0, %entry ], [ %nacc67, %loop ]
  %acc68 = phi float [ 0.0, %entry ], [ %nacc68, %loop ]
  %acc69 = phi float [ 0.0, %entry ], [ %nacc69, %loop ]
  %acc70 = phi float [ 0.0, %entry ], [ %nacc70, %loop ]
  %acc71 = phi float [ 0.0, %entry ], [ %nacc71, %loop ]
  %s = shl i32 %idx, 4
  %p0 = add i32 %s, 256
  %q0 = inttoptr i32 %p0 to ptr addrspace(3)
  %v0 = load float, ptr addrspace(3) %q0, align 4
  %p1 = add i32 %s, 512
  %q1 = inttoptr i32 %p1 to ptr addrspace(3)
  %v1 = load float, ptr addrspace(3) %q1, align 4
  %sum = fadd float %v0, %v1
  %nacc0 = fadd float %acc0, %sum
  %nacc1 = fadd float %acc1, %sum
  %nacc2 = fadd float %acc2, %sum
  %nacc3 = fadd float %acc3, %sum
  %nacc4 = fadd float %acc4, %sum
  %nacc5 = fadd float %acc5, %sum
  %nacc6 = fadd float %acc6, %sum
  %nacc7 = fadd float %acc7, %sum
  %nacc8 = fadd float %acc8, %sum
  %nacc9 = fadd float %acc9, %sum
  %nacc10 = fadd float %acc10, %sum
  %nacc11 = fadd float %acc11, %sum
  %nacc12 = fadd float %acc12, %sum
  %nacc13 = fadd float %acc13, %sum
  %nacc14 = fadd float %acc14, %sum
  %nacc15 = fadd float %acc15, %sum
  %nacc16 = fadd float %acc16, %sum
  %nacc17 = fadd float %acc17, %sum
  %nacc18 = fadd float %acc18, %sum
  %nacc19 = fadd float %acc19, %sum
  %nacc20 = fadd float %acc20, %sum
  %nacc21 = fadd float %acc21, %sum
  %nacc22 = fadd float %acc22, %sum
  %nacc23 = fadd float %acc23, %sum
  %nacc24 = fadd float %acc24, %sum
  %nacc25 = fadd float %acc25, %sum
  %nacc26 = fadd float %acc26, %sum
  %nacc27 = fadd float %acc27, %sum
  %nacc28 = fadd float %acc28, %sum
  %nacc29 = fadd float %acc29, %sum
  %nacc30 = fadd float %acc30, %sum
  %nacc31 = fadd float %acc31, %sum
  %nacc32 = fadd float %acc32, %sum
  %nacc33 = fadd float %acc33, %sum
  %nacc34 = fadd float %acc34, %sum
  %nacc35 = fadd float %acc35, %sum
  %nacc36 = fadd float %acc36, %sum
  %nacc37 = fadd float %acc37, %sum
  %nacc38 = fadd float %acc38, %sum
  %nacc39 = fadd float %acc39, %sum
  %nacc40 = fadd float %acc40, %sum
  %nacc41 = fadd float %acc41, %sum
  %nacc42 = fadd float %acc42, %sum
  %nacc43 = fadd float %acc43, %sum
  %nacc44 = fadd float %acc44, %sum
  %nacc45 = fadd float %acc45, %sum
  %nacc46 = fadd float %acc46, %sum
  %nacc47 = fadd float %acc47, %sum
  %nacc48 = fadd float %acc48, %sum
  %nacc49 = fadd float %acc49, %sum
  %nacc50 = fadd float %acc50, %sum
  %nacc51 = fadd float %acc51, %sum
  %nacc52 = fadd float %acc52, %sum
  %nacc53 = fadd float %acc53, %sum
  %nacc54 = fadd float %acc54, %sum
  %nacc55 = fadd float %acc55, %sum
  %nacc56 = fadd float %acc56, %sum
  %nacc57 = fadd float %acc57, %sum
  %nacc58 = fadd float %acc58, %sum
  %nacc59 = fadd float %acc59, %sum
  %nacc60 = fadd float %acc60, %sum
  %nacc61 = fadd float %acc61, %sum
  %nacc62 = fadd float %acc62, %sum
  %nacc63 = fadd float %acc63, %sum
  %nacc64 = fadd float %acc64, %sum
  %nacc65 = fadd float %acc65, %sum
  %nacc66 = fadd float %acc66, %sum
  %nacc67 = fadd float %acc67, %sum
  %nacc68 = fadd float %acc68, %sum
  %nacc69 = fadd float %acc69, %sum
  %nacc70 = fadd float %acc70, %sum
  %nacc71 = fadd float %acc71, %sum
  %next = add i32 %idx, 1
  %cmp = icmp ult i32 %next, %n
  br i1 %cmp, label %loop, label %exit

exit:
  %e0 = fadd float %nacc0, %nacc1
  %e1 = fadd float %e0, %nacc2
  %e2 = fadd float %e1, %nacc3
  %e3 = fadd float %e2, %nacc4
  %e4 = fadd float %e3, %nacc5
  %e5 = fadd float %e4, %nacc6
  %e6 = fadd float %e5, %nacc7
  %e7 = fadd float %e6, %nacc8
  %e8 = fadd float %e7, %nacc9
  %e9 = fadd float %e8, %nacc10
  %e10 = fadd float %e9, %nacc11
  %e11 = fadd float %e10, %nacc12
  %e12 = fadd float %e11, %nacc13
  %e13 = fadd float %e12, %nacc14
  %e14 = fadd float %e13, %nacc15
  %e15 = fadd float %e14, %nacc16
  %e16 = fadd float %e15, %nacc17
  %e17 = fadd float %e16, %nacc18
  %e18 = fadd float %e17, %nacc19
  %e19 = fadd float %e18, %nacc20
  %e20 = fadd float %e19, %nacc21
  %e21 = fadd float %e20, %nacc22
  %e22 = fadd float %e21, %nacc23
  %e23 = fadd float %e22, %nacc24
  %e24 = fadd float %e23, %nacc25
  %e25 = fadd float %e24, %nacc26
  %e26 = fadd float %e25, %nacc27
  %e27 = fadd float %e26, %nacc28
  %e28 = fadd float %e27, %nacc29
  %e29 = fadd float %e28, %nacc30
  %e30 = fadd float %e29, %nacc31
  %e31 = fadd float %e30, %nacc32
  %e32 = fadd float %e31, %nacc33
  %e33 = fadd float %e32, %nacc34
  %e34 = fadd float %e33, %nacc35
  %e35 = fadd float %e34, %nacc36
  %e36 = fadd float %e35, %nacc37
  %e37 = fadd float %e36, %nacc38
  %e38 = fadd float %e37, %nacc39
  %e39 = fadd float %e38, %nacc40
  %e40 = fadd float %e39, %nacc41
  %e41 = fadd float %e40, %nacc42
  %e42 = fadd float %e41, %nacc43
  %e43 = fadd float %e42, %nacc44
  %e44 = fadd float %e43, %nacc45
  %e45 = fadd float %e44, %nacc46
  %e46 = fadd float %e45, %nacc47
  %e47 = fadd float %e46, %nacc48
  %e48 = fadd float %e47, %nacc49
  %e49 = fadd float %e48, %nacc50
  %e50 = fadd float %e49, %nacc51
  %e51 = fadd float %e50, %nacc52
  %e52 = fadd float %e51, %nacc53
  %e53 = fadd float %e52, %nacc54
  %e54 = fadd float %e53, %nacc55
  %e55 = fadd float %e54, %nacc56
  %e56 = fadd float %e55, %nacc57
  %e57 = fadd float %e56, %nacc58
  %e58 = fadd float %e57, %nacc59
  %e59 = fadd float %e58, %nacc60
  %e60 = fadd float %e59, %nacc61
  %e61 = fadd float %e60, %nacc62
  %e62 = fadd float %e61, %nacc63
  %e63 = fadd float %e62, %nacc64
  %e64 = fadd float %e63, %nacc65
  %e65 = fadd float %e64, %nacc66
  %e66 = fadd float %e65, %nacc67
  %e67 = fadd float %e66, %nacc68
  %e68 = fadd float %e67, %nacc69
  %e69 = fadd float %e68, %nacc70
  %e70 = fadd float %e69, %nacc71
  store float %e70, ptr addrspace(1) %out, align 4
  ret void
}
