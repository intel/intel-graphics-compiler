;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

@__imparg_llvm.genx.local.id16 = internal global <3 x i16> zeroinitializer, !dbg !0

; CHECK-LABEL: @K1
; CHECK-SAME: (i32 %0, <3 x i16> %__arg_llvm.genx.local.id16, i64 %privBase)
; CHECK: [[K1_ALLOCA:%[^ ]+]] = alloca <3 x i16>
; CHECK: call void @llvm.dbg.declare(metadata <3 x i16>* [[K1_ALLOCA]], metadata ![[#K1VAR:]], metadata !DIExpression()), !dbg ![[#K1LOC:]]
; CHECK-DAG: ![[#K1_SP:]] = distinct !DISubprogram(name: "K1",
; CHECK-DAG: ![[#K1VAR]] = !DILocalVariable(name: "__llvm_genx_local_id16", scope: ![[#K1_SP]], file: ![[#]], type: ![[#K1VAR_TYPE:]], flags: DIFlagArtificial)
; CHECK-DAG: ![[#K1VAR_TYPE]] = !DICompositeType(tag: DW_TAG_array_type, baseType: ![[#K1VAR_BASE_TYPE:]], size: 48, flags: DIFlagVector, elements: ![[#K1VAR_ELEMENTS:]])
; CHECK-DAG: ![[#K1VAR_BASE_TYPE]] = !DIBasicType(name: "i16", size: 16, encoding: DW_ATE_unsigned)
; CHECK-DAG: ![[#K1VAR_ELEMENTS]] = !{![[#EL:]]}
; CHECK-DAG: ![[#EL]] = !DISubrange(count: 3)

define dllexport spir_kernel void @K1(i32 %0, <3 x i16> %__arg_llvm.genx.local.id16, i64 %privBase) #0 !dbg !20 {
  store <3 x i16> %__arg_llvm.genx.local.id16, <3 x i16>* @__imparg_llvm.genx.local.id16
  %__imparg_llvm.genx.local.id16.val = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
  %2 = zext <3 x i16> %__imparg_llvm.genx.local.id16.val to <3 x i32>
  %3 = shufflevector <3 x i32> %2, <3 x i32> undef, <8 x i32> zeroinitializer
  %4 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v8i32.i16.i1(<8 x i32> undef, <8 x i32> %3, i32 8, i32 8, i32 1, i16 0, i32 8, i1 true)
  call void @llvm.genx.media.st.v8i32(i32 0, i32 %0, i32 0, i32 32, i32 0, i32 0, <8 x i32> %4)
  ret void
}

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v8i32.i16.i1(<8 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>) #2

attributes #0 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!10, !11}
!llvm.dbg.cu = !{!2}
!genx.kernels = !{!15}
!genx.kernel.internal = !{!19}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "__llvm_genx_local_id16", linkageName: "__llvm_genx_local_id16", scope: !2, file: !3, type: !6, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "1K1S.cpp", directory: "/the_directory")
!4 = !{}
!5 = !{!0}
!6 = !DICompositeType(tag: DW_TAG_array_type, baseType: !7, size: 48, flags: DIFlagVector, elements: !8)
!7 = !DIBasicType(name: "i16", size: 16, encoding: DW_ATE_unsigned)
!8 = !{!9}
!9 = !DISubrange(count: 3)
!10 = !{i32 2, !"Dwarf Version", i32 4}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = !{i32 0, i32 0}
!13 = !{i32 1, i32 2}
!14 = !{i16 6, i16 14}
!15 = !{void (i32, <3 x i16>, i64)* @K1, !"K1", !16, i32 0, i32 0, !17, !18, i32 0}
!16 = !{i32 2, i32 24, i32 96}
!17 = !{i32 0}
!18 = !{!"buffer_t read_write"}
!19 = !{void (i32, <3 x i16>, i64)* @K1, null, null, !4, null}
!20 = distinct !DISubprogram(name: "K1", linkageName: "K1", scope: null, file: !3, line: 15, type: !21, scopeLine: 15, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !24)
!21 = !DISubroutineType(types: !22)
!22 = !{null, !23}
!23 = !DIBasicType(name: "SurfaceIndex", size: 32, encoding: DW_ATE_unsigned)
!24 = !{}
!25 = !{i32 1}
!26 = !{i32 7680}
