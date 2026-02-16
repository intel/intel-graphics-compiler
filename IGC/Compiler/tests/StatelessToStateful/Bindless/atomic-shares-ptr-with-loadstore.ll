;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution --target-addressing-mode bindless | FileCheck %s
; ------------------------------------------------
; PromoteStatelessToBindless : Test promotion of regular loads and stores when a source pointer is shared with atomic
; ------------------------------------------------

; CHECK: target datalayout = "p2490368:32:32:32"

; CHECK-LABEL: @test_promote
; CHECK: [[OFFSET0:%.*]] = add i32 0, 80
; CHECK: [[A64PTR:%.*]] = getelementptr inbounds i64, i64 addrspace(1)* %src, i64 10
; CHECK: [[BASEPTR0:%.*]] = inttoptr i32 %bindlessOffset to i64 addrspace(2490368)*
; CHECK: [[LDRESULT:%.*]] = call i64 @llvm.genx.GenISA.ldraw.indexed.i64.p2490368i64(i64 addrspace(2490368)* [[BASEPTR0]], i32 [[OFFSET0]], i32 4, i1 false)
; CHECK: call i64 @llvm.genx.GenISA.intatomicrawA64.i64.p1i64.p1i64(i64 addrspace(1)* [[A64PTR]], i64 addrspace(1)* [[A64PTR]], i64 25, i32 0)
; CHECK: [[OFFSET1:%.*]] = add i32 0, 160
; CHECK: [[BASEPTR1:%.*]] = inttoptr i32 %bindlessOffset1 to i64 addrspace(2490368)*
; CHECK: call void @llvm.genx.GenISA.storeraw.indexed.p2490368i64.i64(i64 addrspace(2490368)* [[BASEPTR1]], i32 [[OFFSET1]], i64 [[LDRESULT]], i32 4, i1 false)

define spir_kernel void @test_promote(i64 addrspace(1)* %src, i64 addrspace(1)* %dst, i32 %bindlessOffset, i32 %bindlessOffset1) {
  %1 = getelementptr inbounds i64, i64 addrspace(1)* %src, i64 10
  %2 = load i64, i64 addrspace(1)* %1
  %3 = call i64 @llvm.genx.GenISA.intatomicrawA64.i64.p1i64.p1i64(i64 addrspace(1)* %1, i64 addrspace(1)* %1, i64 25, i32 0)
  %4 = getelementptr inbounds i64, i64 addrspace(1)* %dst, i64 20
  store i64 %2, i64 addrspace(1)* %4
  ret void
}

declare i64 @llvm.genx.GenISA.intatomicrawA64.i64.p1i64.p1i64(i64 addrspace(1)*, i64 addrspace(1)*, i64, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (i64 addrspace(1)*, i64 addrspace(1)*, i32, i32)* @test_promote, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !25, !27, !29, !30}
!4 = !{!"ModuleMD", !5, !23}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (i64 addrspace(1)*, i64 addrspace(1)*, i32, i32)* @test_promote}
!7 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = !{!"resAllocMD", !11, !12, !13, !14, !22}
!11 = !{!"uavsNumType", i32 4}
!12 = !{!"srvsNumType", i32 0}
!13 = !{!"samplersNumType", i32 0}
!14 = !{!"argAllocMDList", !15, !19}
!15 = !{!"argAllocMDListVec[0]", !16, !17, !18}
!16 = !{!"type", i32 0}
!17 = !{!"extensionType", i32 -1}
!18 = !{!"indexType", i32 -1}
!19 = !{!"argAllocMDListVec[1]", !20, !17, !21}
!20 = !{!"type", i32 1}
!21 = !{!"indexType", i32 0}
!22 = !{!"inlineSamplersMD"}
!23 = !{!"compOpt", !24}
!24 = !{!"UseLegacyBindlessMode", i1 false}
!25 = !{i32 15, !26}
!26 = !{!"explicit_arg_num", i32 0}
!27 = !{i32 15, !28}
!28 = !{!"explicit_arg_num", i32 1}
!29 = !{i32 53, !26}
!30 = !{i32 53, !28}
