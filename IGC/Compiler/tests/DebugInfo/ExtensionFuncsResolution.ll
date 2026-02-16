;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-extension-funcs-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; ExtensionFuncsResolution
; ------------------------------------------------
; This test checks that ExtensionFuncsResolution  pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void test_vme(sampler_t s)
; {
;     int a = intel_get_accelerator_mb_block_type(s);
;     int b = intel_get_accelerator_mb_sub_pixel_mode(s);
;     int c = intel_get_accelerator_mb_sad_sdjust_mode(s);
;     int d = intel_get_accelerator_mb_search_path_type(s);
;     intel_sub_group_avc_mce_payload_t mce_payload;
;     intel_sub_group_avc_mce_payload_t e = intel_sub_group_avc_mce_set_ac_only_haar(mce_payload);
; }
;
; ------------------------------------------------

%struct.mce_payload_t = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_vme(i64 %s, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %vmeMbBlockType, i32 %vmeSubpixelMode, i32 %vmeSadAdjustMode, i32 %vmeSearchPathType) #0 !dbg !18 {
entry:
  %s.addr = alloca i64, align 8
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %mce_payload = alloca %struct.mce_payload_t*, align 8
  %e = alloca %struct.mce_payload_t*, align 8
  store i64 %s, i64* %s.addr, align 8
;
; Testcase 1:
;
; Checks that dbg info info is not lost on affected IR
; VME Calls that are substituted by arg values
;
; CHECK: @llvm.dbg.declare(metadata i32* %a, metadata [[A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[A_LOC:![0-9]*]]
; CHECK: store i32 %vmeMbBlockType, i32* %a, align 4, !dbg [[A_LOC]]
;
; CHECK: @llvm.dbg.declare(metadata i32* %b, metadata [[B_MD:![0-9]*]], metadata !DIExpression()), !dbg [[B_LOC:![0-9]*]]
; CHECK: store i32 %vmeSubpixelMode, i32* %b, align 4, !dbg [[B_LOC]]
;
; CHECK: @llvm.dbg.declare(metadata i32* %c, metadata [[C_MD:![0-9]*]], metadata !DIExpression()), !dbg [[C_LOC:![0-9]*]]
; CHECK: store i32 %vmeSadAdjustMode, i32* %c, align 4, !dbg [[C_LOC]]
;
; CHECK: @llvm.dbg.declare(metadata i32* %d, metadata [[D_MD:![0-9]*]], metadata !DIExpression()), !dbg [[D_LOC:![0-9]*]]
; CHECK: store i32 %vmeSearchPathType, i32* %d, align 4, !dbg [[D_LOC]]

  call void @llvm.dbg.declare(metadata i64* %s.addr, metadata !26, metadata !DIExpression()), !dbg !27
  call void @llvm.dbg.declare(metadata i32* %a, metadata !28, metadata !DIExpression()), !dbg !30
  %0 = load i64, i64* %s.addr, align 8, !dbg !31
  %call.i = call spir_func i32 @__builtin_IB_vme_mb_block_type() #2, !dbg !32
  store i32 %call.i, i32* %a, align 4, !dbg !30
  call void @llvm.dbg.declare(metadata i32* %b, metadata !33, metadata !DIExpression()), !dbg !34
  %1 = load i64, i64* %s.addr, align 8, !dbg !35
  %call.i1 = call spir_func i32 @__builtin_IB_vme_subpixel_mode() #2, !dbg !36
  store i32 %call.i1, i32* %b, align 4, !dbg !34
  call void @llvm.dbg.declare(metadata i32* %c, metadata !37, metadata !DIExpression()), !dbg !38
  %2 = load i64, i64* %s.addr, align 8, !dbg !39
  %call.i2 = call spir_func i32 @__builtin_IB_vme_sad_adjust_mode() #2, !dbg !40
  store i32 %call.i2, i32* %c, align 4, !dbg !38
  call void @llvm.dbg.declare(metadata i32* %d, metadata !41, metadata !DIExpression()), !dbg !42
  %3 = load i64, i64* %s.addr, align 8, !dbg !43
  %call.i3 = call spir_func i32 @__builtin_IB_vme_search_path_type() #2, !dbg !44
  store i32 %call.i3, i32* %d, align 4, !dbg !42
;
; Testcase 2:
;
; Checks that payload builtins are properly substituted and dbg info is preserved
;
; CHECK: @llvm.dbg.declare(metadata %struct.mce_payload_t** %e, metadata [[E_MD:![0-9]*]], metadata !DIExpression()), !dbg [[E_LOC:![0-9]*]]
; CHECK-DAG: store %struct.mce_payload_t* [[CALL_V:%[0-9]*]], %struct.mce_payload_t** %e, align 8, !dbg [[E_LOC]]
; CHECK-DAG: [[CALL_V]] = {{.*}}, !dbg [[CALL_LOC:![0-9]*]]
  call void @llvm.dbg.declare(metadata %struct.mce_payload_t** %mce_payload, metadata !45, metadata !DIExpression()), !dbg !49
  call void @llvm.dbg.declare(metadata %struct.mce_payload_t** %e, metadata !50, metadata !DIExpression()), !dbg !51
  %4 = load %struct.mce_payload_t*, %struct.mce_payload_t** %mce_payload, align 8, !dbg !52
  %call.i4 = call spir_func <4 x i32> @__builtin_IB_vme_helper_get_handle_avc_mce_payload_t(%struct.mce_payload_t* %4) #4, !dbg !53
  %call3.i = call spir_func %struct.mce_payload_t* @__builtin_IB_vme_helper_get_as_avc_mce_payload_t(<4 x i32> %call.i4) #4, !dbg !53
  store %struct.mce_payload_t* %call3.i, %struct.mce_payload_t** %e, align 8, !dbg !51
  ret void, !dbg !54
}

; Testcase 1 MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ExtensionFuncsResolution.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_vme", scope: null, file: [[FILE]], line: 3
; CHECK-DAG: [[A_LOC]] = !DILocation(line: 5, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[A_MD]] = !DILocalVariable(name: "a", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[B_LOC]] = !DILocation(line: 6, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[B_MD]] = !DILocalVariable(name: "b", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[C_LOC]] = !DILocation(line: 7, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[C_MD]] = !DILocalVariable(name: "c", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[D_LOC]] = !DILocation(line: 8, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[D_MD]] = !DILocalVariable(name: "d", scope: [[SCOPE]], file: [[FILE]], line: 8

; Testcase 2 MD:
;
; CHECK-DAG: [[E_LOC]] = !DILocation(line: 10, column: 39, scope: [[SCOPE]])
; CHECK-DAG: [[E_MD]] = !DILocalVariable(name: "e", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 10, column: 43, scope: [[SCOPE]])
;

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_vme_mb_block_type() #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_vme_subpixel_mode() #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_vme_sad_adjust_mode() #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_vme_search_path_type() #2

; Function Attrs: convergent
declare spir_func <4 x i32> @__builtin_IB_vme_helper_get_handle_avc_mce_payload_t(%struct.mce_payload_t*) #3

; Function Attrs: convergent
declare spir_func %struct.mce_payload_t* @__builtin_IB_vme_helper_get_as_avc_mce_payload_t(<4 x i32>) #3

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" "no-infs-fp-math"="true" "no-nans-fp-math"="true" "unsafe-fp-math"="true" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent nounwind readnone }
attributes #3 = { convergent }
attributes #4 = { convergent nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{!6}
!llvm.ident = !{!17, !17, !17, !17, !17}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{void (i64, <8 x i32>, <8 x i32>, i8*, i32, i32, i32, i32)* @test_vme, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12, !13, !14, !15, !16}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 13}
!13 = !{i32 32}
!14 = !{i32 33}
!15 = !{i32 34}
!16 = !{i32 35}
!17 = !{!"clang version 10.0.0"}
!18 = distinct !DISubprogram(name: "test_vme", scope: null, file: !19, line: 3, type: !20, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!19 = !DIFile(filename: "ExtensionFuncsResolution.ll", directory: "/")
!20 = !DISubroutineType(types: !21)
!21 = !{!22, !23}
!22 = !DIBasicType(name: "int", size: 4)
!23 = !DIDerivedType(tag: DW_TAG_typedef, name: "sampler_t", file: !1, baseType: !24)
!24 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !25, size: 64)
!25 = !DICompositeType(tag: DW_TAG_structure_type, name: "opencl_sampler_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!26 = !DILocalVariable(name: "s", arg: 1, scope: !18, file: !19, line: 3, type: !23)
!27 = !DILocation(line: 3, column: 34, scope: !18)
!28 = !DILocalVariable(name: "a", scope: !18, file: !19, line: 5, type: !29)
!29 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!30 = !DILocation(line: 5, column: 9, scope: !18)
!31 = !DILocation(line: 5, column: 49, scope: !18)
!32 = !DILocation(line: 5, column: 13, scope: !18)
!33 = !DILocalVariable(name: "b", scope: !18, file: !19, line: 6, type: !29)
!34 = !DILocation(line: 6, column: 9, scope: !18)
!35 = !DILocation(line: 6, column: 53, scope: !18)
!36 = !DILocation(line: 6, column: 13, scope: !18)
!37 = !DILocalVariable(name: "c", scope: !18, file: !19, line: 7, type: !29)
!38 = !DILocation(line: 7, column: 9, scope: !18)
!39 = !DILocation(line: 7, column: 54, scope: !18)
!40 = !DILocation(line: 7, column: 13, scope: !18)
!41 = !DILocalVariable(name: "d", scope: !18, file: !19, line: 8, type: !29)
!42 = !DILocation(line: 8, column: 9, scope: !18)
!43 = !DILocation(line: 8, column: 55, scope: !18)
!44 = !DILocation(line: 8, column: 13, scope: !18)
!45 = !DILocalVariable(name: "mce_payload", scope: !18, file: !19, line: 9, type: !46)
!46 = !DIDerivedType(tag: DW_TAG_typedef, name: "mce_payload_t", file: !1, baseType: !47)
!47 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !48, size: 64)
!48 = !DICompositeType(tag: DW_TAG_structure_type, name: "str_mce_payload_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!49 = !DILocation(line: 9, column: 39, scope: !18)
!50 = !DILocalVariable(name: "e", scope: !18, file: !19, line: 10, type: !46)
!51 = !DILocation(line: 10, column: 39, scope: !18)
!52 = !DILocation(line: 10, column: 84, scope: !18)
!53 = !DILocation(line: 10, column: 43, scope: !18)
!54 = !DILocation(line: 11, column: 1, scope: !18)
