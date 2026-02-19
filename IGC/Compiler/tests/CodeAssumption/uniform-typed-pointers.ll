;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-codeassumption -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CodeAssumption : Uniform part
; ------------------------------------------------
; Was reduced from ocl test kernel:
;
; __kernel void bar(int a)
; {
;     int subid = get_sub_group_id();
;     int sum = subid + a;
; }

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Function Attrs: noinline nounwind
define spir_kernel void @bar(i32 %a) #0 {
; CHECK-LABEL: @bar(
; CHECK:  entry:
; CHECK:    %subid = alloca i32, align 4
; CHECK:    [[CALL:%.*]] = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId()
; CHECK:    [[SGID:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 [[CALL]], i32 0, i32 0)
; CHECK:    store i32 [[SGID]], i32* %subid, align 4
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

; Function Attrs: nounwind readnone
declare spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #2

attributes #0 = { noinline nounwind }
attributes #2 = { nounwind readnone }

!IGCMetadata = !{!2}
!igc.functions = !{!13}

!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (i32)* @bar}
!5 = !{!"FuncMDValue[0]", !6, !7, !11, !12}
!6 = !{!"localOffsets"}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"funcArgs"}
!12 = !{!"functionType", !"KernelFunction"}
!13 = !{void (i32)* @bar, !14}
!14 = !{!15}
!15 = !{!"function_type", i32 0}
