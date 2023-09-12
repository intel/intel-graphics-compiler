;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -simdcf-region -simdcf-skip-search-preds \
; RUN: -simdcf-rm-loop-mask -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------

; Function Attrs: nounwind readnone
declare i1 @llvm.genx.any.v32i1(<32 x i1>) #0

; Function Attrs: nounwind
declare void @llvm.genx.svm.block.st.i64.v32i32(i64, <32 x i32>) #1
; CHECK: vadd

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @vadd(i8 addrspace(1)* nocapture readnone %ibuf0, i8 addrspace(1)* nocapture readnone %ibuf1, i8 addrspace(1)* %obuf, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size, i64 %impl.arg.private.base) local_unnamed_addr #2 {
; CHECK: entry:
entry:
; COM: Initialize execution mask by allone
; CHECK: %[[EM:EM.*]] = alloca
; CHECK-DAG: store <32 x i1> <
; CHECK-SAME: i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true,
; CHECK-SAME: i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true,
; CHECK-SAME: i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true,
; CHECK-SAME: i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>,
; CHECK-SAME: <32 x i1>* %[[EM]]
  br label %while.cond

; CHECK: while.cond:
while.cond:                                       ; preds = %while.body, %entry
  %random.0 = phi <32 x i32> [ <i32 12345, i32 29785766, i32 26004755, i32 22223744, i32 18442733, i32 14661722, i32 10880711, i32 7099700, i32 3318689, i32 33092110, i32 29311099, i32 25530088, i32 21749077, i32 17968066, i32 14187055, i32 10406044, i32 6625033, i32 2844022, i32 32617443, i32 28836432, i32 25055421, i32 21274410, i32 17493399, i32 13712388, i32 9931377, i32 6150366, i32 2369355, i32 32142776, i32 28361765, i32 24580754, i32 20799743, i32 17018732>, %entry ], [ %merge28, %while.body ]
  %mask.0 = phi <32 x i16> [ <i16 0, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, %entry ], [ %zext32, %while.body ]
  %out.0 = phi <32 x i32> [ <i32 0, i32 29785766, i32 26004755, i32 22223744, i32 18442733, i32 14661722, i32 10880711, i32 7099700, i32 3318689, i32 33092110, i32 29311099, i32 25530088, i32 21749077, i32 17968066, i32 14187055, i32 10406044, i32 6625033, i32 2844022, i32 32617443, i32 28836432, i32 25055421, i32 21274410, i32 17493399, i32 13712388, i32 9931377, i32 6150366, i32 2369355, i32 32142776, i32 28361765, i32 24580754, i32 20799743, i32 17018732>, %entry ], [ %merge29, %while.body ]

  %cmp18 = icmp ne <32 x i16> %mask.0, zeroinitializer
; CHECK: %[[PRED_1:.*]] = load <32 x i1>, <32 x i1>* %[[EM]]

  %allany = tail call i1 @llvm.genx.any.v32i1(<32 x i1> %cmp18)
; CHECK: %[[GOTO_PRE:.*]] = tail call {{.*}} @llvm.genx.simdcf.goto
; CHECK: %[[EM_TO_STORE:.*]] = extractvalue { <32 x i1>, <32 x i1>, i1 } %[[GOTO_PRE]], 0
; CHECK: store <32 x i1> %[[EM_TO_STORE]], <32 x i1>* %[[EM]], align 4
  br i1 %allany, label %while.body, label %while.end


; CHECK: while.body:
while.body:                                       ; preds = %while.cond
; COM:        Here apply SIMD CF and change predicate from loop-predicate to execution mask
; CHECK: %[[EM_BODY:.*]] = load <32 x i1>, <32 x i1>* %[[EM]]
  %.not = icmp eq <32 x i16> %mask.0, zeroinitializer
; CHECK-NOT: %{{.*}} = icmp eq <32 x i16>
  %mul21 = mul <32 x i32> %random.0, <i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245, i32 1103515245>
; CHECK: %[[MUL:.*]] = mul <32 x i32>
  %add24 = add <32 x i32> %mul21, <i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345>
; CHECK: %[[ADD:.*]] = add <32 x i32>
  %rem27 = and <32 x i32> %add24, <i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431, i32 33554431>
; CHECK: %[[AND:.*]] = and <32 x i32>

; CHECK: %[[MERGE_1:.*]] = select <32 x i1> %[[EM_BODY]], <32 x i32> %[[PHI_1:.*]], <32 x i32> %[[AND]]
  %merge28 = select <32 x i1> %.not, <32 x i32> %random.0, <32 x i32> %rem27
; CHECK: %[[MERGE_2:.*]] = select <32 x i1> %[[EM_BODY]], <32 x i32> %[[PHI_2:.*]], <32 x i32> %[[AND]]
  %merge29 = select <32 x i1> %.not, <32 x i32> %out.0, <32 x i32> %rem27
  %rem30 = urem <32 x i32> %merge28, <i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345, i32 12345>
  %cmp31.not = icmp ne <32 x i32> %rem30, zeroinitializer
  %zext32 = zext <32 x i1> %cmp31.not to <32 x i16>
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %vecext.i.i32 = extractelement <3 x i32> %impl.arg.llvm.genx.local.size, i32 0
  %call.i.i.i = tail call i32 @llvm.genx.group.id.x() #1
  %mul = mul i32 %call.i.i.i, %vecext.i.i32
  %0 = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i32 0
  %vecext.i.i23 = zext i16 %0 to i32
  %add = add i32 %mul, %vecext.i.i23
  %mul3 = shl i32 %add, 7
  %conv6 = zext i32 %mul3 to i64
  %1 = ptrtoint i8 addrspace(1)* %obuf to i64
  %add34 = add i64 %conv6, %1
  tail call void @llvm.genx.svm.block.st.i64.v32i32(i64 %add34, <32 x i32> %out.0)
  ret void
}

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !17 i32 @llvm.genx.group.id.x() #0

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!llvm.ident = !{!11, !11, !11}
!llvm.module.flags = !{!12}
!genx.kernel.internal = !{!13}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, <3 x i16>, <3 x i32>, i64)* @vadd, !"vadd", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 0, i32 0, i32 0, i32 24, i32 8, i32 96}
!8 = !{i32 88, i32 96, i32 104, i32 32, i32 64, i32 80}
!9 = !{i32 0, i32 0, i32 0}
!10 = !{!"svmptr_t", !"svmptr_t", !"svmptr_t"}
!11 = !{!"clang version 11.1.0"}
!12 = !{i32 1, !"wchar_size", i32 4}
!13 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, <3 x i16>, <3 x i32>, i64)* @vadd, !14, !15, !4, !16}
!14 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!15 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5}
!16 = !{i32 255, i32 255, i32 255, i32 255, i32 255, i32 255}
!17 = !{i32 8171}
