;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -enable-debugify --igc-programscope-constant-resolve -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK
; ------------------------------------------------
; ProgramScopeConstantResolution
; ------------------------------------------------

; Debug-info related checks
;
; For llvm 14 check-debugify treats missing debug location on globalbase getter
; at the begining of BB as a warning, while on earlier llvm versions its treated as an error.
;
; CHECK: CheckModuleDebugify: PASS

@a = internal addrspace(2) constant [2 x i32] [i32 0, i32 1], align 4
@d = internal addrspace(1) global i32 addrspace(2)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(2)* @a, i32 0, i32 0), align 8
@c = internal addrspace(1) global i32 0, align 4
@b = common addrspace(1) global i32 0, align 4
@llvm.used = appending global [3 x i8*] [i8* addrspacecast (i8 addrspace(2)* bitcast ([2 x i32] addrspace(2)* @a to i8 addrspace(2)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(1)* @c to i8 addrspace(1)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(2)* addrspace(1)* @d to i8 addrspace(1)*) to i8*)], section "llvm.metadata"

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_program(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i8 addrspace(1)* %globalBase, i8* %privateBase, i32 %bufferOffset) {
; CHECK-LABEL: @test_program(
; CHECK:  entry:
; CHECK:    [[OFFC:%.*]] = getelementptr i8, ptr addrspace(1) %globalBase, i64 8
; CHECK:    [[OFFD:%.*]] = getelementptr i8, ptr addrspace(1) %globalBase, i64 0
; CHECK:    [[OFFA:%.*]] = getelementptr i8, ptr addrspace(2) %constBase, i64 0
; CHECK:    [[DST_ADDR:%.*]] = alloca ptr addrspace(1), align 8
; CHECK:    [[AA:%.*]] = alloca i32, align 4
; CHECK:    store ptr addrspace(1) [[DST:%.*]], ptr [[DST_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = getelementptr inbounds [2 x i32], ptr addrspace(2) [[OFFA]], i64 0, i64 1
; CHECK:    [[TMP1:%.*]] = load i32, ptr addrspace(2) [[TMP0]], align 4
; CHECK:    store i32 [[TMP1]], ptr [[AA]], align 4
; CHECK:    [[TMP2:%.*]] = load ptr addrspace(2), ptr addrspace(1) [[OFFD]], align 8
; CHECK:    [[TMP3:%.*]] = load i32, ptr addrspace(2) [[TMP2]], align 4
; CHECK:    store i32 [[TMP3]], ptr addrspace(1) [[OFFC]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %aa = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  %0 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(2)* @a, i64 0, i64 1
  %1 = load i32, i32 addrspace(2)* %0, align 4
  store i32 %1, i32* %aa, align 4
  %2 = load i32 addrspace(2)*, i32 addrspace(2)* addrspace(1)* @d, align 8
  %3 = load i32, i32 addrspace(2)* %2, align 4
  store i32 %3, i32 addrspace(1)* @c, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineProgramScopeOffsets", !2, !3, !4, !5, !6, !7}
!2 = !{!"inlineProgramScopeOffsetsMap[0]", [2 x i32] addrspace(2)* @a}
!3 = !{!"inlineProgramScopeOffsetsValue[0]", i64 0}
!4 = !{!"inlineProgramScopeOffsetsMap[1]", i32 addrspace(1)* @c}
!5 = !{!"inlineProgramScopeOffsetsValue[1]", i64 8}
!6 = !{!"inlineProgramScopeOffsetsMap[2]", i32 addrspace(2)* addrspace(1)* @d}
!7 = !{!"inlineProgramScopeOffsetsValue[2]", i64 0}

!20 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8 addrspace(1)*, i8*, i32)* @test_program, !21}
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
