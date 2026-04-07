;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --verify --opaque-pointers --igc-break-const-expr --igc-programscope-constant-resolve -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantResolution: GEP address space mismatch
; ------------------------------------------------
;
; @gVar is in addrspace(0) and SROA loads use constant GEP expressions
; (ptr getelementptr(@gVar, offset)). Check that the new GEP has the correct result type
;
; CHECK-LABEL: @test_kernel
; CHECK: %offgVar = getelementptr i8, ptr addrspace(2) %constBase, i64 0
;
; All loads must use ptr addrspace(2), not bare ptr:
; CHECK: load <8 x float>, ptr addrspace(2) %offgVar
; CHECK: load <8 x float>, ptr addrspace(2)
; CHECK: load <8 x float>, ptr addrspace(2)
; CHECK-NOT: load <8 x float>, ptr %

@gVar = internal global [128 x i8] zeroinitializer, align 32, !spirv.Decorations !0

define spir_kernel void @test_kernel(
    ptr addrspace(1) %dst,
    <8 x i32> %r0,
    <8 x i32> %payloadHeader,
    ptr addrspace(2) %constBase,
    ptr addrspace(1) %globalBase,
    ptr %privateBase,
    i32 %bufferOffset) {
entry:
  %.load0 = load <8 x float>, ptr @gVar, align 32

  %.load1 = load <8 x float>, ptr getelementptr inbounds ([128 x i8], ptr @gVar, i64 0, i64 32), align 32
  %.load2 = load <8 x float>, ptr getelementptr inbounds ([128 x i8], ptr @gVar, i64 0, i64 64), align 32

  %sum01 = fadd <8 x float> %.load0, %.load1
  %sum012 = fadd <8 x float> %sum01, %.load2
  store <8 x float> %sum012, ptr addrspace(1) %dst, align 32
  ret void
}

!IGCMetadata = !{!10}
!igc.functions = !{!20}

!0 = !{!1}
!1 = !{i32 44, i32 16}

!10 = !{!"ModuleMD", !11}
!11 = !{!"inlineProgramScopeOffsets", !12, !13}
!12 = !{!"inlineProgramScopeOffsetsMap[0]", ptr @gVar}
!13 = !{!"inlineProgramScopeOffsetsValue[0]", i64 0}

!20 = !{ptr @test_kernel, !21}
!21 = !{!22, !23}
!22 = !{!"function_type", i32 0}
!23 = !{!"implicit_arg_desc", !24, !25, !26, !27, !28, !29}
!24 = !{i32 0}
!25 = !{i32 1}
!26 = !{i32 11}
!27 = !{i32 12}
!28 = !{i32 13}
!29 = !{i32 15, !30}
!30 = !{!"explicit_arg_num", i32 0}
