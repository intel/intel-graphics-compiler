;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --simplifycfg -hoist-common-insts=true -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Checks that SimplyCFG pass hoists common instructions
;
; REQUIRES: llvm-14-plus
;
; CHECK: entry:
; CHECK-NOT: br
; CHECK: [[LD:%[a-zA-Z0-9_.]*]] = load <2 x float>
; CHECK: [[EXT1:%[a-zA-Z0-9_.]*]] = extractelement {{.*}}[[LD]]
; CHECK: [[EXT2:%[a-zA-Z0-9_.]*]] = extractelement {{.*}}[[LD]]
; CHECK: [[SEL:%[a-zA-Z0-9_.]*]] = select {{.*}}[[EXT1]]{{.*}}[[EXT2]]
; CHECK: fadd float {{.*}}[[SEL]]{{$}}

define spir_kernel void @detect(<2 x float> addrspace(1)* %classifier_weights, i1 %cmp79) {
entry:
  br i1 %cmp79, label %if.then81, label %if.else

if.then81:                                        ; preds = %for.end
  %idxprom82 = zext i32 0 to i64
  %arrayidx83 = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %classifier_weights, i64 %idxprom82
  %0 = load <2 x float>, <2 x float> addrspace(1)* %classifier_weights, align 8
  %scalar47 = extractelement <2 x float> %0, i32 0
  br label %if.end88

if.else:                                          ; preds = %for.end
  %idxprom85 = zext i32 0 to i64
  %arrayidx86 = getelementptr inbounds <2 x float>, <2 x float> addrspace(1)* %classifier_weights, i64 %idxprom85
  %1 = load <2 x float>, <2 x float> addrspace(1)* %classifier_weights, align 8
  %scalar50 = extractelement <2 x float> %1, i32 1
  br label %if.end88

if.end88:                                         ; preds = %if.else, %if.then81
  %.pn = phi float [ %scalar47, %if.then81 ], [ %scalar50, %if.else ]
  %stage_value.1 = fadd float 0.000000e+00, %.pn
  ret void
}

