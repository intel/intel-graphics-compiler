;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-madloopslice -S < %s | FileCheck %s
; ------------------------------------------------
; MadLoopSlice
; ------------------------------------------------

define spir_kernel void @test() {
entry:
  br label %for.body

; CHECK: %c0 = call double @llvm.fma.f64(double %a0, double %b0, double 1.000000e+00)
; CHECK-NEXT: %d0 = call double @llvm.fma.f64(double %b0, double %c0, double 1.000000e+00)
; CHECK-NEXT: %c1 = call double @llvm.fma.f64(double %a1, double %b1, double 1.000000e+00)
; CHECK-NEXT: %d1 = call double @llvm.fma.f64(double %b1, double %c1, double 1.000000e+00)

for.body:                                         ; preds = %for.body, %entry
  %i = phi i32 [ 1, %entry ], [ %iter, %for.body ]
  %a0 = phi double [ 1.000000e+00, %entry ], [ %c0, %for.body ]
  %b0 = phi double [ 0.000000e+00, %entry ], [ %d0, %for.body ]
  %a1 = phi double [ 1.000000e+00, %entry ], [ %c1, %for.body ]
  %b1 = phi double [ 0.000000e+00, %entry ], [ %d1, %for.body ]
  call void @llvm.dbg.value(metadata double %b1, metadata !11, metadata !DIExpression()), !dbg !18
  %c0 = call double @llvm.fma.f64(double %a0, double %b0, double 1.000000e+00)
  %c1 = call double @llvm.fma.f64(double %a1, double %b1, double 1.000000e+00)
  %d0 = call double @llvm.fma.f64(double %b0, double %c0, double 1.000000e+00)
  %d1 = call double @llvm.fma.f64(double %b1, double %c1, double 1.000000e+00)
  %iter = add nuw nsw i32 %i, 1
  %cmp = icmp ult i32 %iter, 100
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare double @llvm.fma.f64(double, double, double) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!opencl.ocl.version = !{!6}
!llvm.ident = !{!7}
!igc.functions = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 9.0.0 (tags/RELEASE_900/final 372344)", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, nameTableKind: None)
!1 = !DIFile(filename: "mad.ll", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{i32 2, i32 0}
!7 = !{!"clang version 9.0.0 (tags/RELEASE_900/final 372344)"}
!8 = !{void ()* @test, !9}
!9 = !{!10}
!10 = !{!"function_type", i32 0}
!11 = !DILocalVariable(name: "b1", scope: !12, file: !13, line: 3, type: !17)
!12 = distinct !DISubprogram(name: "test", scope: !13, file: !13, line: 1, type: !14, scopeLine: 2, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !16)
!13 = !DIFile(filename: "example.cpp", directory: "/app")
!14 = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel, types: !15)
!15 = !{null}
!16 = !{!11}
!17 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!18 = !DILocation(line: 0, scope: !12)
