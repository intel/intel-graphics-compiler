;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-device-enqueue-func-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; DeviceEnqueueFuncsAnalysis
; ------------------------------------------------
; This test checks that DeviceEnqueueFuncsAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; This is an analysis pass, check that IR and debug MD are not modified
; ------------------------------------------------

; Check IR
; CHECK: define spir_kernel void @test_device
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[DDQ_V:%[A-z0-9]*]] = call i8 addrspace(1)*
; CHECK-SAME: !dbg [[DDQ_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i8 addrspace(1)* [[DDQ_V]]
; CHECK-SAME: metadata [[DDQ_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DDQ_LOC]]
;
; CHECK: [[EP_V:%[A-z0-9]*]] = call i8 addrspace(1)*
; CHECK-SAME: !dbg [[EP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i8 addrspace(1)* [[EP_V]]
; CHECK-SAME: metadata [[EP_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[EP_LOC]]
;
; CHECK: [[MWG_V:%[A-z0-9]*]] = call i32
; CHECK-SAME: !dbg [[MWG_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[MWG_V]]
; CHECK-SAME: metadata [[MWG_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[MWG_LOC]]
;
; CHECK: [[PE_V:%[A-z0-9]*]] = call i32
; CHECK-SAME: !dbg [[PE_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[PE_V]]
; CHECK-SAME: metadata [[PE_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[PE_LOC]]
;
; CHECK: [[PWM_V:%[A-z0-9]*]] = call i32
; CHECK-SAME: !dbg [[PWM_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[PWM_V]]
; CHECK-SAME: metadata [[PWM_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[PWM_LOC]]
;
; CHECK: store
; CHECK-SAME: [[MWG_V]]
; CHECK-SAME: !dbg [[SMWG_LOC:![0-9]*]]
;
; CHECK: store
; CHECK-SAME: [[PE_V]]
; CHECK-SAME: !dbg [[SPE_LOC:![0-9]*]]
;
; CHECK: store
; CHECK-SAME: [[PWM_V]]
; CHECK-SAME: !dbg [[SPWM_LOC:![0-9]*]]

define spir_kernel void @test_device() !dbg !10 {
  %1 = call i8 addrspace(1)* @__builtin_IB_get_default_device_queue(), !dbg !22
  call void @llvm.dbg.value(metadata i8 addrspace(1)* %1, metadata !13, metadata !DIExpression()), !dbg !22
  %2 = call i8 addrspace(1)* @__builtin_IB_get_event_pool(), !dbg !23
  call void @llvm.dbg.value(metadata i8 addrspace(1)* %2, metadata !15, metadata !DIExpression()), !dbg !23
  %3 = call i32 @__builtin_IB_get_max_workgroup_size(), !dbg !24
  call void @llvm.dbg.value(metadata i32 %3, metadata !16, metadata !DIExpression()), !dbg !24
  %4 = call i32 @__builtin_IB_get_parent_event(), !dbg !25
  call void @llvm.dbg.value(metadata i32 %4, metadata !18, metadata !DIExpression()), !dbg !25
  %5 = call i32 @__builtin_IB_get_prefered_workgroup_multiple(), !dbg !26
  call void @llvm.dbg.value(metadata i32 %5, metadata !19, metadata !DIExpression()), !dbg !26
  %6 = bitcast i8 addrspace(1)* %1 to i32 addrspace(1)*, !dbg !27
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %6, metadata !20, metadata !DIExpression()), !dbg !27
  %7 = bitcast i8 addrspace(1)* %2 to i32 addrspace(1)*, !dbg !28
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %7, metadata !21, metadata !DIExpression()), !dbg !28
  store i32 %3, i32 addrspace(1)* %6, !dbg !29
  store i32 %4, i32 addrspace(1)* %7, !dbg !30
  store i32 %5, i32 addrspace(1)* %6, !dbg !31
  ret void, !dbg !32
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "DeviceEnqueueFuncsAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_device", linkageName: "test_device", scope: null, file: [[FILE]], line: 1
;
; CHECK-DAG: [[DDQ_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DDQ_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[EP_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[EP_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[MWG_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[MWG_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[PE_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[PE_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[PWM_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[PWM_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[SMWG_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SPE_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SPWM_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

declare i8 addrspace(1)* @__builtin_IB_get_default_device_queue()

declare i8 addrspace(1)* @__builtin_IB_get_event_pool()

declare i32 @__builtin_IB_get_max_workgroup_size()

declare i32 @__builtin_IB_get_parent_event()

declare i32 @__builtin_IB_get_prefered_workgroup_multiple()

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void ()* @test_device, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "DeviceEnqueueFuncsAnalysis.ll", directory: "/")
!6 = !{}
!7 = !{i32 11}
!8 = !{i32 7}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_device", linkageName: "test_device", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !18, !19, !20, !21}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !17)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 5, type: !17)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 6, type: !14)
!21 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 7, type: !14)
!22 = !DILocation(line: 1, column: 1, scope: !10)
!23 = !DILocation(line: 2, column: 1, scope: !10)
!24 = !DILocation(line: 3, column: 1, scope: !10)
!25 = !DILocation(line: 4, column: 1, scope: !10)
!26 = !DILocation(line: 5, column: 1, scope: !10)
!27 = !DILocation(line: 6, column: 1, scope: !10)
!28 = !DILocation(line: 7, column: 1, scope: !10)
!29 = !DILocation(line: 8, column: 1, scope: !10)
!30 = !DILocation(line: 9, column: 1, scope: !10)
!31 = !DILocation(line: 10, column: 1, scope: !10)
!32 = !DILocation(line: 11, column: 1, scope: !10)
