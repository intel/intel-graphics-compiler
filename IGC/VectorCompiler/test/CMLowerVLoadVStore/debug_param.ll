;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -CMLowerVLoadVStore -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; CMLowerVLoadVStore
; ------------------------------------------------
; Test checks, what pass is applied even with gdb-info
;
; CHECK: void @test_vloadvstore{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata <4 x i32>* %a, metadata {{.*}}, metadata !DIExpression()), !dbg {{.*}}
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = alloca {{.*}}
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = load {{.*}}
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}@llvm.genx.rdregioni{{.*}}
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = load {{.*}}
; CHECK: ret void

@global_vec = internal global <4 x i32> undef, align 8 #0

define void @test_vloadvstore(<4 x i32>* %a, <4 x i32> addrspace(4)* %b) !dbg !6 {
entry:
  call void @llvm.dbg.value(metadata <4 x i32>* %a, metadata !11, metadata !DIExpression()), !dbg !23
  %0 = alloca <4 x i32>, align 8
  call void @llvm.dbg.value(metadata <4 x i32>* %a, metadata !11, metadata !DIExpression()), !dbg !23
  %1 = load <4 x i32>, <4 x i32>* %a
  call void @llvm.dbg.value(metadata <4 x i32>* %a, metadata !11, metadata !DIExpression()), !dbg !23
  %2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %1, i32 1, i32 1, i32 0, i16 0, i32 0)
  call void @llvm.dbg.value(metadata <4 x i32>* %a, metadata !11, metadata !DIExpression()), !dbg !23
  %3 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* %0)
  call void @llvm.dbg.value(metadata <4 x i32>* %a, metadata !11, metadata !DIExpression()), !dbg !23
  ret void
}

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>*)
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "CMLowerVLoadVStore_debug.ll", directory: "/")
!2 = !{}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_vloadvstore", linkageName: "test_vloadvstore", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!11}
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!23 = !DILocation(line: 4, column: 1, scope: !6)



