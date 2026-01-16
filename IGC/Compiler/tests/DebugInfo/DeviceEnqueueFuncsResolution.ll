;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-device-enqueue-func-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; DeviceEnqueueFuncsResolution
; ------------------------------------------------
; This test checks that DeviceEnqueueFuncsResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; Check IR
; CHECK: define spir_kernel void @test_device
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata ptr addrspace(1) [[DDQ_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[DDQ_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DDQ_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata ptr addrspace(1) [[EP_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[EP_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[EP_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[MWG_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[MWG_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[MWG_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[PE_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[PE_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[PE_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[PWM_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[PWM_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[PWM_LOC:![0-9]*]]
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


define spir_kernel void @test_device(ptr addrspace(1) %deviceEnqueueDefaultDeviceQueue, ptr addrspace(1) %deviceEnqueueEventPool, i32 %deviceEnqueueMaxWorkgroupSize, i32 %deviceEnqueueParentEvent, i32 %deviceEnqueuePreferedWorkgroupMultiple) !dbg !15 {
  %1 = call ptr addrspace(1) @__builtin_IB_get_default_device_queue(), !dbg !27
  call void @llvm.dbg.value(metadata ptr addrspace(1) %1, metadata !18, metadata !DIExpression()), !dbg !27
  %2 = call ptr addrspace(1) @__builtin_IB_get_event_pool(), !dbg !28
  call void @llvm.dbg.value(metadata ptr addrspace(1) %2, metadata !20, metadata !DIExpression()), !dbg !28
  %3 = call i32 @__builtin_IB_get_max_workgroup_size(), !dbg !29
  call void @llvm.dbg.value(metadata i32 %3, metadata !21, metadata !DIExpression()), !dbg !29
  %4 = call i32 @__builtin_IB_get_parent_event(), !dbg !30
  call void @llvm.dbg.value(metadata i32 %4, metadata !23, metadata !DIExpression()), !dbg !30
  %5 = call i32 @__builtin_IB_get_prefered_workgroup_multiple(), !dbg !31
  call void @llvm.dbg.value(metadata i32 %5, metadata !24, metadata !DIExpression()), !dbg !31

  call void @llvm.dbg.value(metadata ptr addrspace(1) %1, metadata !25, metadata !DIExpression()), !dbg !32

  call void @llvm.dbg.value(metadata ptr addrspace(1) %2, metadata !26, metadata !DIExpression()), !dbg !33
  store i32 %3, ptr addrspace(1) %1, !dbg !34
  store i32 %4, ptr addrspace(1) %2, !dbg !35
  store i32 %5, ptr addrspace(1) %1, !dbg !36
  ret void, !dbg !37
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "DeviceEnqueueFuncsResolution.ll", directory: "/")
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


declare ptr addrspace(1) @__builtin_IB_get_default_device_queue()

declare ptr addrspace(1) @__builtin_IB_get_event_pool()

declare i32 @__builtin_IB_get_max_workgroup_size()

declare i32 @__builtin_IB_get_parent_event()

declare i32 @__builtin_IB_get_prefered_workgroup_multiple()

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!9}
!llvm.debugify = !{!12, !13}
!llvm.module.flags = !{!14}

!0 = !{ptr @test_device, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8}
!4 = !{i32 38}
!5 = !{i32 39}
!6 = !{i32 40}
!7 = !{i32 41}
!8 = !{i32 42}
!9 = distinct !DICompileUnit(language: DW_LANG_C, file: !10, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !11)
!10 = !DIFile(filename: "DeviceEnqueueFuncsResolution.ll", directory: "/")
!11 = !{}
!12 = !{i32 11}
!13 = !{i32 7}
!14 = !{i32 2, !"Debug Info Version", i32 3}
!15 = distinct !DISubprogram(name: "test_device", linkageName: "test_device", scope: null, file: !10, line: 1, type: !16, scopeLine: 1, unit: !9, retainedNodes: !17)
!16 = !DISubroutineType(types: !11)
!17 = !{!18, !20, !21, !23, !24, !25, !26}
!18 = !DILocalVariable(name: "1", scope: !15, file: !10, line: 1, type: !19)
!19 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "2", scope: !15, file: !10, line: 2, type: !19)
!21 = !DILocalVariable(name: "3", scope: !15, file: !10, line: 3, type: !22)
!22 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!23 = !DILocalVariable(name: "4", scope: !15, file: !10, line: 4, type: !22)
!24 = !DILocalVariable(name: "5", scope: !15, file: !10, line: 5, type: !22)
!25 = !DILocalVariable(name: "6", scope: !15, file: !10, line: 6, type: !19)
!26 = !DILocalVariable(name: "7", scope: !15, file: !10, line: 7, type: !19)
!27 = !DILocation(line: 1, column: 1, scope: !15)
!28 = !DILocation(line: 2, column: 1, scope: !15)
!29 = !DILocation(line: 3, column: 1, scope: !15)
!30 = !DILocation(line: 4, column: 1, scope: !15)
!31 = !DILocation(line: 5, column: 1, scope: !15)
!32 = !DILocation(line: 6, column: 1, scope: !15)
!33 = !DILocation(line: 7, column: 1, scope: !15)
!34 = !DILocation(line: 8, column: 1, scope: !15)
!35 = !DILocation(line: 9, column: 1, scope: !15)
!36 = !DILocation(line: 10, column: 1, scope: !15)
!37 = !DILocation(line: 11, column: 1, scope: !15)
