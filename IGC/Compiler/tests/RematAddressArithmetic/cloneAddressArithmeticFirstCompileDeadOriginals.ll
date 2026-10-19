;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; The address operands come from the entry block, so the replaced originals keep them live until dead code is
; removed. The pressure check must see the IR without them: remat brings SIMD32 pressure from above the 128 GRF
; budget to below it and is kept.

; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o /dev/null -igc-clone-address-arithmetic --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=RematRPELimit=93 -remat-widest-simd -remat-first-compile --regkey=RematLog=1 --regkey=RematLogToErr=1 2>&1 | FileCheck %s

; CHECK: Pressure budget: 128 MaxPressure: {{[0-9]+}} -> {{1[0-2][0-9]|[0-9]?[0-9]}}
; CHECK-NOT: rolling back
declare i16 @llvm.genx.GenISA.simdLaneId()
define spir_kernel void @main(ptr addrspace(1) %out, i64 %base, i32 %n) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %tid = zext i16 %lane to i32
  %init = uitofp i32 %tid to float
  %t64 = zext i32 %tid to i64
  %entryoffset = shl i64 %t64, 4
  %entrybase = add i64 %base, %entryoffset
  %p0 = add i64 %entrybase, 256
  %p1 = add i64 %entrybase, 512
  %p2 = add i64 %entrybase, 768
  %p3 = add i64 %entrybase, 1024
  %p4 = add i64 %entrybase, 1280
  %p5 = add i64 %entrybase, 1536
  %p6 = add i64 %entrybase, 1792
  %p7 = add i64 %entrybase, 2048
  %p8 = add i64 %entrybase, 2304
  %p9 = add i64 %entrybase, 2560
  br label %loop
loop:
  %idx = phi i32 [ %tid, %entry ], [ %next, %loop ]
  %acc0 = phi float [ %init, %entry ], [ %nacc0, %loop ]
  %acc1 = phi float [ %init, %entry ], [ %nacc1, %loop ]
  %acc2 = phi float [ %init, %entry ], [ %nacc2, %loop ]
  %acc3 = phi float [ %init, %entry ], [ %nacc3, %loop ]
  %acc4 = phi float [ %init, %entry ], [ %nacc4, %loop ]
  %acc5 = phi float [ %init, %entry ], [ %nacc5, %loop ]
  %acc6 = phi float [ %init, %entry ], [ %nacc6, %loop ]
  %acc7 = phi float [ %init, %entry ], [ %nacc7, %loop ]
  %acc8 = phi float [ %init, %entry ], [ %nacc8, %loop ]
  %acc9 = phi float [ %init, %entry ], [ %nacc9, %loop ]
  %acc10 = phi float [ %init, %entry ], [ %nacc10, %loop ]
  %acc11 = phi float [ %init, %entry ], [ %nacc11, %loop ]
  %acc12 = phi float [ %init, %entry ], [ %nacc12, %loop ]
  %acc13 = phi float [ %init, %entry ], [ %nacc13, %loop ]
  %acc14 = phi float [ %init, %entry ], [ %nacc14, %loop ]
  %acc15 = phi float [ %init, %entry ], [ %nacc15, %loop ]
  %acc16 = phi float [ %init, %entry ], [ %nacc16, %loop ]
  %acc17 = phi float [ %init, %entry ], [ %nacc17, %loop ]
  %acc18 = phi float [ %init, %entry ], [ %nacc18, %loop ]
  %acc19 = phi float [ %init, %entry ], [ %nacc19, %loop ]
  %acc20 = phi float [ %init, %entry ], [ %nacc20, %loop ]
  %acc21 = phi float [ %init, %entry ], [ %nacc21, %loop ]
  %acc22 = phi float [ %init, %entry ], [ %nacc22, %loop ]
  %acc23 = phi float [ %init, %entry ], [ %nacc23, %loop ]
  %acc24 = phi float [ %init, %entry ], [ %nacc24, %loop ]
  %acc25 = phi float [ %init, %entry ], [ %nacc25, %loop ]
  %acc26 = phi float [ %init, %entry ], [ %nacc26, %loop ]
  %acc27 = phi float [ %init, %entry ], [ %nacc27, %loop ]
  %acc28 = phi float [ %init, %entry ], [ %nacc28, %loop ]
  %acc29 = phi float [ %init, %entry ], [ %nacc29, %loop ]
  %acc30 = phi float [ %init, %entry ], [ %nacc30, %loop ]
  %acc31 = phi float [ %init, %entry ], [ %nacc31, %loop ]
  %acc32 = phi float [ %init, %entry ], [ %nacc32, %loop ]
  %acc33 = phi float [ %init, %entry ], [ %nacc33, %loop ]
  %acc34 = phi float [ %init, %entry ], [ %nacc34, %loop ]
  %acc35 = phi float [ %init, %entry ], [ %nacc35, %loop ]
  %acc36 = phi float [ %init, %entry ], [ %nacc36, %loop ]
  %acc37 = phi float [ %init, %entry ], [ %nacc37, %loop ]
  %acc38 = phi float [ %init, %entry ], [ %nacc38, %loop ]
  %acc39 = phi float [ %init, %entry ], [ %nacc39, %loop ]
  %acc40 = phi float [ %init, %entry ], [ %nacc40, %loop ]
  %acc41 = phi float [ %init, %entry ], [ %nacc41, %loop ]
  %acc42 = phi float [ %init, %entry ], [ %nacc42, %loop ]
  %acc43 = phi float [ %init, %entry ], [ %nacc43, %loop ]
  %acc44 = phi float [ %init, %entry ], [ %nacc44, %loop ]
  %acc45 = phi float [ %init, %entry ], [ %nacc45, %loop ]
  %acc46 = phi float [ %init, %entry ], [ %nacc46, %loop ]
  %acc47 = phi float [ %init, %entry ], [ %nacc47, %loop ]
  %acc48 = phi float [ %init, %entry ], [ %nacc48, %loop ]
  %acc49 = phi float [ %init, %entry ], [ %nacc49, %loop ]
  %acc50 = phi float [ %init, %entry ], [ %nacc50, %loop ]
  %acc51 = phi float [ %init, %entry ], [ %nacc51, %loop ]
  %acc52 = phi float [ %init, %entry ], [ %nacc52, %loop ]
  %acc53 = phi float [ %init, %entry ], [ %nacc53, %loop ]
  %acc54 = phi float [ %init, %entry ], [ %nacc54, %loop ]
  %idx64 = zext i32 %idx to i64
  %offset = shl i64 %idx64, 4
  %b = add i64 %base, %offset
  %q0 = inttoptr i64 %p0 to ptr addrspace(1)
  %q1 = inttoptr i64 %p1 to ptr addrspace(1)
  %q2 = inttoptr i64 %p2 to ptr addrspace(1)
  %q3 = inttoptr i64 %p3 to ptr addrspace(1)
  %q4 = inttoptr i64 %p4 to ptr addrspace(1)
  %q5 = inttoptr i64 %p5 to ptr addrspace(1)
  %q6 = inttoptr i64 %p6 to ptr addrspace(1)
  %q7 = inttoptr i64 %p7 to ptr addrspace(1)
  %q8 = inttoptr i64 %p8 to ptr addrspace(1)
  %q9 = inttoptr i64 %p9 to ptr addrspace(1)
  %v0 = load float, ptr addrspace(1) %q0, align 4
  %v1 = load float, ptr addrspace(1) %q1, align 4
  %sum1 = fadd float %v0, %v1
  %v2 = load float, ptr addrspace(1) %q2, align 4
  %sum2 = fadd float %sum1, %v2
  %v3 = load float, ptr addrspace(1) %q3, align 4
  %sum3 = fadd float %sum2, %v3
  %v4 = load float, ptr addrspace(1) %q4, align 4
  %sum4 = fadd float %sum3, %v4
  %v5 = load float, ptr addrspace(1) %q5, align 4
  %sum5 = fadd float %sum4, %v5
  %v6 = load float, ptr addrspace(1) %q6, align 4
  %sum6 = fadd float %sum5, %v6
  %v7 = load float, ptr addrspace(1) %q7, align 4
  %sum7 = fadd float %sum6, %v7
  %v8 = load float, ptr addrspace(1) %q8, align 4
  %sum8 = fadd float %sum7, %v8
  %v9 = load float, ptr addrspace(1) %q9, align 4
  %sum9 = fadd float %sum8, %v9
  %nacc0 = fadd float %acc0, %sum9
  %nacc1 = fadd float %acc1, %sum9
  %nacc2 = fadd float %acc2, %sum9
  %nacc3 = fadd float %acc3, %sum9
  %nacc4 = fadd float %acc4, %sum9
  %nacc5 = fadd float %acc5, %sum9
  %nacc6 = fadd float %acc6, %sum9
  %nacc7 = fadd float %acc7, %sum9
  %nacc8 = fadd float %acc8, %sum9
  %nacc9 = fadd float %acc9, %sum9
  %nacc10 = fadd float %acc10, %sum9
  %nacc11 = fadd float %acc11, %sum9
  %nacc12 = fadd float %acc12, %sum9
  %nacc13 = fadd float %acc13, %sum9
  %nacc14 = fadd float %acc14, %sum9
  %nacc15 = fadd float %acc15, %sum9
  %nacc16 = fadd float %acc16, %sum9
  %nacc17 = fadd float %acc17, %sum9
  %nacc18 = fadd float %acc18, %sum9
  %nacc19 = fadd float %acc19, %sum9
  %nacc20 = fadd float %acc20, %sum9
  %nacc21 = fadd float %acc21, %sum9
  %nacc22 = fadd float %acc22, %sum9
  %nacc23 = fadd float %acc23, %sum9
  %nacc24 = fadd float %acc24, %sum9
  %nacc25 = fadd float %acc25, %sum9
  %nacc26 = fadd float %acc26, %sum9
  %nacc27 = fadd float %acc27, %sum9
  %nacc28 = fadd float %acc28, %sum9
  %nacc29 = fadd float %acc29, %sum9
  %nacc30 = fadd float %acc30, %sum9
  %nacc31 = fadd float %acc31, %sum9
  %nacc32 = fadd float %acc32, %sum9
  %nacc33 = fadd float %acc33, %sum9
  %nacc34 = fadd float %acc34, %sum9
  %nacc35 = fadd float %acc35, %sum9
  %nacc36 = fadd float %acc36, %sum9
  %nacc37 = fadd float %acc37, %sum9
  %nacc38 = fadd float %acc38, %sum9
  %nacc39 = fadd float %acc39, %sum9
  %nacc40 = fadd float %acc40, %sum9
  %nacc41 = fadd float %acc41, %sum9
  %nacc42 = fadd float %acc42, %sum9
  %nacc43 = fadd float %acc43, %sum9
  %nacc44 = fadd float %acc44, %sum9
  %nacc45 = fadd float %acc45, %sum9
  %nacc46 = fadd float %acc46, %sum9
  %nacc47 = fadd float %acc47, %sum9
  %nacc48 = fadd float %acc48, %sum9
  %nacc49 = fadd float %acc49, %sum9
  %nacc50 = fadd float %acc50, %sum9
  %nacc51 = fadd float %acc51, %sum9
  %nacc52 = fadd float %acc52, %sum9
  %nacc53 = fadd float %acc53, %sum9
  %nacc54 = fadd float %acc54, %sum9
  %next = add i32 %idx, 1
  %cmp = icmp ult i32 %next, %n
  br i1 %cmp, label %loop, label %exit
exit:
  %e1 = fadd float %nacc0, %nacc1
  %e2 = fadd float %e1, %nacc2
  %e3 = fadd float %e2, %nacc3
  %e4 = fadd float %e3, %nacc4
  %e5 = fadd float %e4, %nacc5
  %e6 = fadd float %e5, %nacc6
  %e7 = fadd float %e6, %nacc7
  %e8 = fadd float %e7, %nacc8
  %e9 = fadd float %e8, %nacc9
  %e10 = fadd float %e9, %nacc10
  %e11 = fadd float %e10, %nacc11
  %e12 = fadd float %e11, %nacc12
  %e13 = fadd float %e12, %nacc13
  %e14 = fadd float %e13, %nacc14
  %e15 = fadd float %e14, %nacc15
  %e16 = fadd float %e15, %nacc16
  %e17 = fadd float %e16, %nacc17
  %e18 = fadd float %e17, %nacc18
  %e19 = fadd float %e18, %nacc19
  %e20 = fadd float %e19, %nacc20
  %e21 = fadd float %e20, %nacc21
  %e22 = fadd float %e21, %nacc22
  %e23 = fadd float %e22, %nacc23
  %e24 = fadd float %e23, %nacc24
  %e25 = fadd float %e24, %nacc25
  %e26 = fadd float %e25, %nacc26
  %e27 = fadd float %e26, %nacc27
  %e28 = fadd float %e27, %nacc28
  %e29 = fadd float %e28, %nacc29
  %e30 = fadd float %e29, %nacc30
  %e31 = fadd float %e30, %nacc31
  %e32 = fadd float %e31, %nacc32
  %e33 = fadd float %e32, %nacc33
  %e34 = fadd float %e33, %nacc34
  %e35 = fadd float %e34, %nacc35
  %e36 = fadd float %e35, %nacc36
  %e37 = fadd float %e36, %nacc37
  %e38 = fadd float %e37, %nacc38
  %e39 = fadd float %e38, %nacc39
  %e40 = fadd float %e39, %nacc40
  %e41 = fadd float %e40, %nacc41
  %e42 = fadd float %e41, %nacc42
  %e43 = fadd float %e42, %nacc43
  %e44 = fadd float %e43, %nacc44
  %e45 = fadd float %e44, %nacc45
  %e46 = fadd float %e45, %nacc46
  %e47 = fadd float %e46, %nacc47
  %e48 = fadd float %e47, %nacc48
  %e49 = fadd float %e48, %nacc49
  %e50 = fadd float %e49, %nacc50
  %e51 = fadd float %e50, %nacc51
  %e52 = fadd float %e51, %nacc52
  %e53 = fadd float %e52, %nacc53
  %e54 = fadd float %e53, %nacc54
  store float %e54, ptr addrspace(1) %out, align 4
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @main, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
