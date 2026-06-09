;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Check that we reduce alloca sinking threshold for functions with optnone attribute.
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey AllocaRAPressureThreshold=22 --regkey AllocaSinkingOptNoneAllowance=20 --igc-private-mem-resolution -S %s | FileCheck %s

; CHECK: for.cond:
; CHECK: for.body:
; CHECK: call{{.*}}llvm.genx.GenISA.StackAlloca
; CHECK: br label %for.cond


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @call_func_in_loop() #0 {
entry:
  %out.addr = alloca ptr addrspace(1), align 8
  %gid = alloca i32, align 4
  %j = alloca i32, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  br label %for.body

for.body:                                         ; preds = %for.cond
  %0 = load i32, ptr %j, align 4
  br label %for.cond
}

declare spir_func void @add_one()

attributes #0 = { "visaStackCall" optnone noinline}

!igc.functions = !{!0, !4}

!0 = !{ptr @add_one, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 2}
!4 = !{ptr @call_func_in_loop, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}
!18 = !{!"argId", i32 0}
!19 = !{!"implicitArgInfoListVec[0]", !18}
!20 = !{!"argId", i32 2}
!21 = !{!"implicitArgInfoListVec[1]", !20}
!22 = !{!"argId", i32 7}
!23 = !{!"implicitArgInfoListVec[2]", !22}
!24 = !{!"argId", i32 8}
!25 = !{!"implicitArgInfoListVec[3]", !24}
!26 = !{!"argId", i32 9}
!27 = !{!"implicitArgInfoListVec[4]", !26}
!28 = !{!"argId", i32 10}
!29 = !{!"implicitArgInfoListVec[5]", !28}
!30 = !{!"argId", i32 13}
!31 = !{!"implicitArgInfoListVec[6]", !30}
!32 = !{!"argId", i32 15}
!33 = !{!"explicitArgNum", i32 0}
!34 = !{!"implicitArgInfoListVec[7]", !32, !33}
!35 = !{!"argId", i32 59}
!36 = !{!"explicitArgNum", i32 0}
!37 = !{!"implicitArgInfoListVec[8]", !35, !36}
!38 = !{!"implicitArgInfoList", !19, !21, !23, !25, !27, !29, !31, !34, !37}
!39 = !{!"FuncMDMap[0]", ptr @call_func_in_loop}
!40 = !{!"FuncMDValue[0]", !38}
!41 = !{!"FuncMD", !39, !40}
!42 = !{!"ModuleMD", !41}
!IGCMetadata = !{!42}
