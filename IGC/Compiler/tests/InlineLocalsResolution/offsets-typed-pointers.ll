;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-resolve-inline-locals -igc-serialize-metadata -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; InlineLocalsResolution
; ------------------------------------------------

@test.Q = internal addrspace(3) global [16384 x half] undef, align 2, !spirv.Decorations !0
@test.S = internal addrspace(3) global [32768 x half] undef, align 2, !spirv.Decorations !0
@test.C = internal addrspace(3) global [4096 x float] undef, align 4, !spirv.Decorations !2

define spir_kernel void @test(i64 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) #0 {
entry:
  %arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %out, i64 0
  %0 = ptrtoint [16384 x half] addrspace(3)* @test.Q to i64
  store i64 %0, i64 addrspace(1)* %arrayidx, align 8
  %arrayidx1 = getelementptr inbounds i64, i64 addrspace(1)* %out, i64 0
  %1 = ptrtoint [32768 x half] addrspace(3)* @test.S to i64
  store i64 %1, i64 addrspace(1)* %arrayidx1, align 8
  %arrayidx2 = getelementptr inbounds i64, i64 addrspace(1)* %out, i64 0
  %2 = ptrtoint [4096 x float] addrspace(3)* @test.C to i64
  store i64 %2, i64 addrspace(1)* %arrayidx2, align 8
  ret void
}

!igc.functions = !{!4}

!0 = !{!1}
!1 = !{i32 44, i32 2}
!2 = !{!3}
!3 = !{i32 44, i32 4}
!4 = !{void (i64 addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @test, !5}
!5 = !{!6, !7}
!6 = !{!"function_type", i32 0}
!7 = !{!"implicit_arg_desc", !8, !9}
!8 = !{i32 0}
!9 = !{i32 1}

; CHECK: ![[IDX1:[0-9]+]] = !{!"localOffsets", ![[IDX2:[0-9]+]], ![[IDX3:[0-9]+]], ![[IDX4:[0-9]+]]}
; CHECK: ![[IDX2]] = !{!"localOffsetsVec[0]", ![[IDX5:[0-9]+]], ![[IDX6:[0-9]+]]}
; CHECK: ![[IDX5]] = !{!"m_Offset", i32 268435456}
; CHECK: ![[IDX6]] = !{!"m_Var", [16384 x half] addrspace(3)* @test.Q}
; CHECK: ![[IDX3]] = !{!"localOffsetsVec[1]", ![[IDX7:[0-9]+]], ![[IDX8:[0-9]+]]}
; CHECK: ![[IDX7]] = !{!"m_Offset", i32 268468224}
; CHECK: ![[IDX8]] = !{!"m_Var", [32768 x half] addrspace(3)* @test.S}
; CHECK: ![[IDX4]] = !{!"localOffsetsVec[2]", ![[IDX9:[0-9]+]], ![[IDX10:[0-9]+]]}
; CHECK: ![[IDX9]] = !{!"m_Offset", i32 268533760}
; CHECK: ![[IDX10]] = !{!"m_Var", [4096 x float] addrspace(3)* @test.C}
