;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-insert-dummy-kernel-for-symbol-table -S < %s | FileCheck %s
; ------------------------------------------------
; InsertDummyKernelForSymbolTable
; ------------------------------------------------

@b = common addrspace(1) global i32 0, align 4

define spir_kernel void @test(i32 %a) #0 {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[A_ADDR:%.*]] = alloca i32, align 4
; CHECK:    [[SUBID:%.*]] = alloca i32, align 4
; CHECK:    [[SUM:%.*]] = alloca i32, align 4
; CHECK:    store i32 [[A:%.*]], i32* [[A_ADDR]], align 4
; CHECK:    [[CALL:%.*]] = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #1
; CHECK:    store i32 [[CALL]], i32* [[SUBID]], align 4
; CHECK:    [[TMP0:%.*]] = load i32, i32* [[SUBID]], align 4
; CHECK:    [[TMP1:%.*]] = load i32, i32* [[A_ADDR]], align 4
; CHECK:    [[ADD:%.*]] = add nsw i32 [[TMP0]], [[TMP1]]
; CHECK:    store i32 [[ADD]], i32* [[SUM]], align 4
; CHECK:    ret void
;
entry:
  %a.addr = alloca i32, align 4
  %subid = alloca i32, align 4
  %sum = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  %call = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #2
  store i32 %call, i32* %subid, align 4
  %0 = load i32, i32* %subid, align 4
  %1 = load i32, i32* %a.addr, align 4
  %add = add nsw i32 %0, %1
  store i32 %add, i32* %sum, align 4
  ret void
}

; CHECK-LABEL: @Intel_Symbol_Table_Void_Program(
; CHECK:  entry:
; CHECK:    ret void

; Function Attrs: nounwind readnone
declare spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #2

attributes #0 = { noinline nounwind }
attributes #2 = { nounwind readnone }

!IGCMetadata = !{!2}
!igc.functions = !{!16}

!2 = !{!"ModuleMD", !3, !13}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (i32)* @test}
!5 = !{!"FuncMDValue[0]", !6, !7, !11, !12}
!6 = !{!"localOffsets"}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"funcArgs"}
!12 = !{!"functionType", !"KernelFunction"}
!13 = !{!"inlineProgramScopeOffsets", !14, !15}
!14 = !{!"inlineProgramScopeOffsetsMap[0]", i32 addrspace(1)* @b}
!15 = !{!"inlineProgramScopeOffsetsValue[0]", i32 0}
!16 = !{void (i32)* @test, !17}
!17 = !{!18}
!18 = !{!"function_type", i32 0}
