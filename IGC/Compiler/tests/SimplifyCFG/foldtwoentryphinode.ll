;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --simplifycfg -speculate-one-expensive-inst=false -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Checks if GenIntrinsicsTTIImpl returns proper intrinsic cost to stop
; SimplifyCFG pass from folding two entry phi node into select instruction.
;
; CHECK: entry:
; CHECK-NOT: select
; CHECK: br i1 %cmp.i, label %if.then.i, label %func.exit

define spir_kernel void @foo(double addrspace(1)* %x, double addrspace(1)* %y, double addrspace(1)* %z, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2) #0 {
entry:
  %0 = load double, double addrspace(1)* %x, align 8
  %cmp.i = fcmp ogt double %0, 1.000000e+00
  br i1 %cmp.i, label %if.then.i, label %if.end.i

if.then.i:                                        ; preds = %entry
  %call.i.i.i3 = call double @llvm.sqrt.f64(double %0)
  br label %func.exit

if.end.i:                                         ; preds = %entry
  br label %func.exit

func.exit:                                        ; preds = %if.then.i, %if.end.i
  %retval.0.i = phi double [ %call.i.i.i3, %if.then.i ], [ %0, %if.end.i ]
  store double %retval.0.i, double addrspace(1)* %z, align 8
  ret void
}

declare double @llvm.sqrt.f64(double)
