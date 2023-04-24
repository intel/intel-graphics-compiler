;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromotePredicate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Gen9 -logical-ops-threshold=2 -S < %s | FileCheck %s

; COM: verify update of debug metadata

; CHECK: void @test_debug{{.*}}
; CHECK: [[AND:%.*]] = and <32 x i[[AND_SIZE:[0-9]*]]> {{.*}} !dbg [[AND_LOC:![0-9]*]]
; CHECK: [[TRUNC:%.*]] = icmp ne <32 x i[[AND_SIZE]]> [[AND]], zeroinitializer, !dbg [[AND_LOC]]
; CHECK: void @llvm.dbg.value(metadata <32 x i1> [[TRUNC]], metadata [[TRUNC_MD:![0-9]*]], {{.*}}, !dbg [[AND_LOC]]
;

define void @test_debug(<32 x float> %src1, <32 x i32> %src2, <32 x i32>* %dst) {
entry:
  %0 = bitcast <32 x i32> %src2 to <32 x float>
  %1 = fcmp ogt <32 x float> %src1, %0
  %2 = xor <32 x i1> %1, <i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true>
  %3 = and <32 x i1> %2, <i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true>
  %4 = bitcast <32 x float> %src1 to <32 x i32>
  %5 = icmp ult <32 x i32> %src2, %4
  %6 = or <32 x i1> %5, %3
  %7 = and <32 x i1> %6, <i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true, i1 true, i1 false, i1 false, i1 true>, !dbg !27
  call void @llvm.dbg.value(metadata <32 x i1> %7, metadata !18, metadata !DIExpression()), !dbg !27
  %8 = select <32 x i1> %7, <32 x i32> %src2, <32 x i32> %4
  store <32 x i32> %8, <32 x i32>* %dst
  ret void
}

; CHECK-DAG: [[TRUNC_MD]] = !DILocalVariable({{.*}}, line: 8
; CHECK-DAG: [[AND_LOC]] = !DILocation(line: 8{{.*}}

declare void @llvm.dbg.value(metadata, metadata, metadata) #0

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "debug.ll", directory: "/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_debug", linkageName: "test_debug", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !{})
!7 = !DISubroutineType(types: !{})
!12 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
!27 = !DILocation(line: 8, column: 1, scope: !6)
