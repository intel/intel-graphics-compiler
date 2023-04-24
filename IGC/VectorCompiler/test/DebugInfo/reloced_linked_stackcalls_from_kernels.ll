;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -vc-experimental-dbg-info-zebin-compatible \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: %igc-lld --relocatable \
; RUN: -o dbginfo_%basename_t_linked.elf \
; RUN: dbginfo_%basename_t_K1_dwarf.elf dbginfo_%basename_t_K2_dwarf.elf

; RUN: oneapi-readelf -a dbginfo_%basename_t_linked.elf | FileCheck %s --check-prefix=CHECK_READELF

; CHECK_READELF: Relocation section '.rela.debug_info' at offset 0x{{[0-9a-f]+}} contains {{[0-9]+}} entries
; CHECK_READELF: Relocation section '.rela.debug_frame' at offset 0x{{[0-9a-f]+}} contains 4 entries
; CHECK_READELF: Relocation section '.rela.debug_line' at offset 0x{{[0-9a-f]+}} contains 2 entries
; CHECK_READELF-NEXT: Offset
; CHECK_READELF-NEXT: .text.K1
; CHECK_READELF-NEXT: .text.K2

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind readnone
define internal spir_func i32 @S1(<8 x i32> %v_out) unnamed_addr #0 !dbg !16 !FuncArgSize !49 !FuncRetSize !50 {
entry:
  call void @llvm.dbg.value(metadata <8 x i32> %v_out, metadata !24, metadata !DIExpression()), !dbg !25
  %sev.cast.1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %v_out, i32 0, i32 1, i32 1, i16 0, i32 undef)
  ret i32 %sev.cast.1.regioncollapsed, !dbg !26
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @K1(i32 %indx) local_unnamed_addr #1 !dbg !27 {
entry:
  call void @llvm.dbg.value(metadata i32 %indx, metadata !32, metadata !DIExpression()), !dbg !34
  call void @llvm.dbg.value(metadata <8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, metadata !33, metadata !DIExpression()), !dbg !34
  %call = tail call spir_func i32 @S1(<8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>) #5, !dbg !35, !FuncArgSize !49, !FuncRetSize !50
  %add = add nsw i32 %call, 1, !dbg !36
  %sev.cast.1 = insertelement <1 x i32> undef, i32 %add, i64 0
  %wrregion = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, <1 x i32> %sev.cast.1, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true), !dbg !36
  call void @llvm.dbg.value(metadata <8 x i32> %wrregion, metadata !33, metadata !DIExpression()), !dbg !34
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 %indx, i32 0, i32 32, i32 0, i32 0, <8 x i32> %wrregion), !dbg !37
  ret void, !dbg !38
}

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) #2

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>) #3

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @K2(i32 %indx) local_unnamed_addr #1 !dbg !39 {
entry:
  call void @llvm.dbg.value(metadata i32 %indx, metadata !41, metadata !DIExpression()), !dbg !43
  call void @llvm.dbg.value(metadata <8 x i32> <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>, metadata !42, metadata !DIExpression()), !dbg !43
  %call = tail call spir_func i32 @S1(<8 x i32> <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>) #5, !dbg !44, !FuncArgSize !49, !FuncRetSize !50
  %add = add nsw i32 %call, 2, !dbg !45
  %sev.cast.1 = insertelement <1 x i32> undef, i32 %add, i64 0
  %wrregion = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>, <1 x i32> %sev.cast.1, i32 0, i32 1, i32 0, i16 4, i32 undef, i1 true), !dbg !45
  call void @llvm.dbg.value(metadata <8 x i32> %wrregion, metadata !42, metadata !DIExpression()), !dbg !43
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 %indx, i32 0, i32 32, i32 0, i32 0, <8 x i32> %wrregion), !dbg !46
  ret void, !dbg !47
}

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !48 i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) #2

attributes #0 = { noinline nounwind readnone "CMStackCall" }
attributes #1 = { noinline nounwind "CMGenxMain" }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }
attributes #4 = { nounwind readnone speculatable willreturn }
attributes #5 = { noinline nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!5}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!5}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!7}
!genx.kernels = !{!8, !13}
!genx.kernel.internal = !{!14, !15}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "2K1S.cpp", directory: "/the_directory")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{i32 1, i32 2}
!7 = !{i16 6, i16 14}
!8 = !{void (i32)* @K1, !"K1", !9, i32 0, !10, !11, !12, i32 0}
!9 = !{i32 2}
!10 = !{i32 32}
!11 = !{i32 0}
!12 = !{!"buffer_t read_write"}
!13 = !{void (i32)* @K2, !"K2", !9, i32 0, !10, !11, !12, i32 0}
!14 = !{void (i32)* @K1, !11, !11, null, null}
!15 = !{void (i32)* @K2, !11, !11, null, null}
!16 = distinct !DISubprogram(name: "S1", scope: null, file: !3, line: 3, type: !17, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, templateParams: !4, retainedNodes: !23)
!17 = !DISubroutineType(types: !18)
!18 = !{!19, !20}
!19 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!20 = !DICompositeType(tag: DW_TAG_array_type, baseType: !19, size: 256, flags: DIFlagVector, elements: !21)
!21 = !{!22}
!22 = !DISubrange(count: 8)
!23 = !{!24}
!24 = !DILocalVariable(name: "v_out", arg: 1, scope: !16, file: !3, line: 3, type: !20)
!25 = !DILocation(line: 0, scope: !16)
!26 = !DILocation(line: 4, column: 3, scope: !16)
!27 = distinct !DISubprogram(name: "K1", linkageName: "_Z2K115cm_surfaceindex", scope: null, file: !3, line: 7, type: !28, scopeLine: 7, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !31)
!28 = !DISubroutineType(types: !29)
!29 = !{null, !30}
!30 = !DIBasicType(name: "SurfaceIndex", size: 32, encoding: DW_ATE_unsigned)
!31 = !{!32, !33}
!32 = !DILocalVariable(name: "indx", arg: 1, scope: !27, file: !3, line: 7, type: !30)
!33 = !DILocalVariable(name: "v", scope: !27, file: !3, line: 8, type: !20)
!34 = !DILocation(line: 0, scope: !27)
!35 = !DILocation(line: 9, column: 11, scope: !27)
!36 = !DILocation(line: 9, column: 8, scope: !27)
!37 = !DILocation(line: 10, column: 3, scope: !27)
!38 = !DILocation(line: 11, column: 1, scope: !27)
!39 = distinct !DISubprogram(name: "K2", linkageName: "_Z2K215cm_surfaceindex", scope: null, file: !3, line: 13, type: !28, scopeLine: 13, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !40)
!40 = !{!41, !42}
!41 = !DILocalVariable(name: "indx", arg: 1, scope: !39, file: !3, line: 13, type: !30)
!42 = !DILocalVariable(name: "v", scope: !39, file: !3, line: 14, type: !20)
!43 = !DILocation(line: 0, scope: !39)
!44 = !DILocation(line: 15, column: 11, scope: !39)
!45 = !DILocation(line: 15, column: 8, scope: !39)
!46 = !DILocation(line: 16, column: 3, scope: !39)
!47 = !DILocation(line: 17, column: 1, scope: !39)
!48 = !{i32 7747}
!49 = !{i32 1}
!50 = !{i32 1}
