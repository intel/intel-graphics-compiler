;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-spir-metadata-translation -S < %s | FileCheck %s
; ------------------------------------------------
; SPIRMetaDataTranslation
; ------------------------------------------------
; This test checks that SPIRMetaDataTranslation pass follows
; 'How to Update Debug Info' llvm guideline.
;
; ------------------------------------------------

; CHECK: @test_spir{{.*}} !dbg [[SCOPE:![0-9]*]]

; CHECK: @llvm.dbg.declare(metadata i64 addrspace(1)** {{.*}}, metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
; CHECK: @llvm.dbg.declare(metadata i32* {{.*}}, metadata [[GID_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC:![0-9]*]]
; CHECK: [[EXTR_V:%[A-z0-9]*]] = extractelement {{.*}} !dbg [[EXTR_LOC:![0-9]*]]
; CHECK: [[CALLB1_V:%[A-z0-9]*]] = call spir_func {{.*}} !dbg [[EXTR_LOC]]
; CHECK: store {{.*}} !dbg [[GID_LOC]]
; CHECK: @llvm.dbg.declare(metadata i32* {{.*}}, metadata [[TID_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TID_LOC:![0-9]*]]
; CHECK: store {{.*}} !dbg [[TID_LOC]]
; CHECK: @llvm.dbg.declare(metadata i32* {{.*}}, metadata [[DIM_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DIM_LOC:![0-9]*]]
; CHECK: store {{.*}} !dbg [[DIM_LOC]]
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load {{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load {{.*}} !dbg [[LOAD2_LOC:![0-9]*]]

define spir_kernel void @test_spir(i64 addrspace(1)* %dst) #0 !dbg !70 {
entry:
  %dst.addr = alloca i64 addrspace(1)*, align 8
  %gid = alloca i32, align 4
  %tid = alloca i32, align 4
  %dim = alloca i32, align 4
  %c = alloca i64, align 8
  store i64 addrspace(1)* %dst, i64 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i64 addrspace(1)** %dst.addr, metadata !78, metadata !DIExpression()), !dbg !79
  call void @llvm.dbg.declare(metadata i32* %gid, metadata !80, metadata !DIExpression()), !dbg !82
  %0 = call spir_func <3 x i64> @__builtin_spirv_BuiltInWorkgroupId() #3
  %call = extractelement <3 x i64> %0, i32 0, !dbg !83
  %conv = call spir_func i32 @__builtin_spirv_OpUConvert_i32_i64(i64 %call) #2, !dbg !83
  store i32 %conv, i32* %gid, align 4, !dbg !82
  call void @llvm.dbg.declare(metadata i32* %tid, metadata !84, metadata !DIExpression()), !dbg !85
  %call1 = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #3
  store i32 %call1, i32* %tid, align 4, !dbg !85
  call void @llvm.dbg.declare(metadata i32* %dim, metadata !86, metadata !DIExpression()), !dbg !87
  %call2 = call spir_func i32 @__builtin_spirv_BuiltInWorkDim() #3
  store i32 %call2, i32* %dim, align 4, !dbg !87
  %1 = load i32, i32* %gid, align 4, !dbg !88
  %2 = load i32, i32* %tid, align 4, !dbg !89
  %add = add nsw i32 %1, %2, !dbg !90
  %conv3 = call spir_func i64 @__builtin_spirv_OpSConvert_i64_i32(i32 %add) #2, !dbg !88
  %3 = load i64 addrspace(1)*, i64 addrspace(1)** %dst.addr, align 8, !dbg !91
  %arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %3, i64 0, !dbg !91
  store volatile i64 %conv3, i64 addrspace(1)* %arrayidx, align 8, !dbg !92
  call void @llvm.dbg.declare(metadata i64* %c, metadata !93, metadata !DIExpression()), !dbg !94
  %4 = load i64 addrspace(1)*, i64 addrspace(1)** %dst.addr, align 8, !dbg !95
  %5 = load i32, i32* %dim, align 4, !dbg !96
  %conv4 = call spir_func i64 @__builtin_spirv_OpSConvert_i64_i32(i32 %5) #2, !dbg !96
  %call5 = call spir_func i64 @__builtin_spirv_OpAtomicOr_p1i64_i32_i32_i64(i64 addrspace(1)* %4, i32 1, i32 16, i64 %conv4) #2, !dbg !97
  store i64 %call5, i64* %c, align 8, !dbg !94
  %6 = load i64, i64* %c, align 8, !dbg !98
  %7 = load i64 addrspace(1)*, i64 addrspace(1)** %dst.addr, align 8, !dbg !99
  %arrayidx6 = getelementptr inbounds i64, i64 addrspace(1)* %7, i64 1, !dbg !99
  store volatile i64 %6, i64 addrspace(1)* %arrayidx6, align 8, !dbg !100
  ret void, !dbg !101
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "SPIRMetaDataTranslation.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_spir", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 5, column: 40, scope: [[SCOPE]])
; CHECK-DAG: [[GID_MD]] = !DILocalVariable(name: "gid", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[GID_LOC]] = !DILocation(line: 7, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR_LOC]] = !DILocation(line: 7, column: 13, scope: [[SCOPE]])
; CHECK-DAG: [[TID_MD]] = !DILocalVariable(name: "tid", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[TID_LOC]] = !DILocation(line: 8, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[DIM_MD]] = !DILocalVariable(name: "dim", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[DIM_LOC]] = !DILocation(line: 9, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 10, column: 12, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 10, column: 18, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind
declare spir_func i32 @__builtin_spirv_OpUConvert_i32_i64(i64) #2

; Function Attrs: nounwind
declare spir_func i64 @__builtin_spirv_OpSConvert_i64_i32(i32) #2

; Function Attrs: nounwind
declare spir_func i64 @__builtin_spirv_OpAtomicOr_p1i64_i32_i32_i64(i64 addrspace(1)*, i32, i32, i64) #2

; Function Attrs: nounwind readnone
declare spir_func <3 x i64> @__builtin_spirv_BuiltInWorkgroupId() #3

; Function Attrs: nounwind readnone
declare spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #3

; Function Attrs: nounwind readnone
declare spir_func i32 @__builtin_spirv_BuiltInWorkDim() #3

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind }
attributes #3 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}
!opencl.kernels = !{!5}
!IGCMetadata = !{!16}
!opencl.enable.FP_CONTRACT = !{}
!opencl.spir.version = !{!67}
!opencl.ocl.version = !{!67}
!opencl.used.extensions = !{!68}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!69}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{void (i64 addrspace(1)*)* @test_spir, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15}
!6 = !{!"kernel_arg_addr_space", i32 1}
!7 = !{!"kernel_arg_access_qual", !"none"}
!8 = !{!"kernel_arg_type", !"long*"}
!9 = !{!"kernel_arg_type_qual", !"volatile"}
!10 = !{!"kernel_arg_base_type", !"long*"}
!11 = !{!"kernel_arg_name", !"dst"}
!12 = !{!"reqd_work_group_size", i32 1, i32 1, i32 16}
!13 = !{!"work_group_size_hint", i32 1, i32 1, i32 4}
!14 = !{!"vec_type_hint", <4 x float> undef, i32 0}
!15 = !{!"intel_reqd_sub_group_size", i32 16}
!16 = !{!"ModuleMD", !17, !18, !59}
!17 = !{!"isPrecise", i1 false}
!18 = !{!"compOpt", !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57}
!19 = !{!"DenormsAreZero", i1 false}
!20 = !{!"CorrectlyRoundedDivSqrt", i1 false}
!21 = !{!"OptDisable", i1 false}
!22 = !{!"MadEnable", i1 false}
!23 = !{!"NoSignedZeros", i1 false}
!24 = !{!"NoNaNs", i1 false}
!25 = !{!"FloatRoundingMode", i32 0}
!26 = !{!"FloatCvtIntRoundingMode", i32 3}
!27 = !{!"UnsafeMathOptimizations", i1 false}
!28 = !{!"FiniteMathOnly", i1 false}
!29 = !{!"FastRelaxedMath", i1 false}
!30 = !{!"DashGSpecified", i1 false}
!31 = !{!"FastCompilation", i1 false}
!32 = !{!"UseScratchSpacePrivateMemory", i1 true}
!33 = !{!"RelaxedBuiltins", i1 false}
!34 = !{!"SubgroupIndependentForwardProgressRequired", i1 true}
!35 = !{!"GreaterThan2GBBufferRequired", i1 true}
!36 = !{!"GreaterThan4GBBufferRequired", i1 true}
!37 = !{!"DisableA64WA", i1 false}
!38 = !{!"ForceEnableA64WA", i1 false}
!39 = !{!"PushConstantsEnable", i1 true}
!40 = !{!"HasBufferOffsetArg", i1 false}
!41 = !{!"replaceGlobalOffsetsByZero", i1 false}
!42 = !{!"forcePixelShaderSIMDMode", i32 0}
!43 = !{!"pixelShaderDoNotAbortOnSpill", i1 false}
!44 = !{!"UniformWGS", i1 false}
!45 = !{!"disableVertexComponentPacking", i1 false}
!46 = !{!"disablePartialVertexComponentPacking", i1 false}
!47 = !{!"PreferBindlessImages", i1 false}
!48 = !{!"disableMathRefactoring", i1 false}
!49 = !{!"WaveIntrinsicUsed", i1 false}
!50 = !{!"WaveIntrinsicWaterFallLoopOptimization", i1 false}
!51 = !{!"ForceInt32DivRemEmu", i1 false}
!52 = !{!"ForceInt32DivRemEmuSP", i1 false}
!53 = !{!"EnableTakeGlobalAddress", i1 false}
!54 = !{!"IsLibraryCompilation", i1 false}
!55 = !{!"FastVISACompile", i1 false}
!56 = !{!"MatchSinCosPi", i1 false}
!57 = !{!"CaptureCompilerStats", i1 false}
!59 = !{!"FuncMD", !60, !61}
!60 = !{!"FuncMDMap[0]", void (i64 addrspace(1)*)* @test_spir}
!61 = !{!"FuncMDValue[0]", !62, !63}
!62 = !{!"localOffsets"}
!63 = !{!"workGroupWalkOrder", !64, !65, !66}
!64 = !{!"dim0", i32 0}
!65 = !{!"dim1", i32 0}
!66 = !{!"dim2", i32 0}
!67 = !{i32 2, i32 0}
!68 = !{!"cl_khr_int64_extended_atomics"}
!69 = !{!"-cl-std=CL2.0", !"-cl-opt-disable", !"-g"}
!70 = distinct !DISubprogram(name: "test_spir", scope: null, file: !71, line: 5, type: !72, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!71 = !DIFile(filename: "SPIRMetaDataTranslation.ll", directory: "/")
!72 = !DISubroutineType(types: !73)
!73 = !{!74, !75}
!74 = !DIBasicType(name: "int", size: 4)
!75 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !76, size: 64)
!76 = !DIDerivedType(tag: DW_TAG_volatile_type, baseType: !77)
!77 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!78 = !DILocalVariable(name: "dst", arg: 1, scope: !70, file: !71, line: 5, type: !75)
!79 = !DILocation(line: 5, column: 40, scope: !70)
!80 = !DILocalVariable(name: "gid", scope: !70, file: !71, line: 7, type: !81)
!81 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!82 = !DILocation(line: 7, column: 7, scope: !70)
!83 = !DILocation(line: 7, column: 13, scope: !70)
!84 = !DILocalVariable(name: "tid", scope: !70, file: !71, line: 8, type: !81)
!85 = !DILocation(line: 8, column: 7, scope: !70)
!86 = !DILocalVariable(name: "dim", scope: !70, file: !71, line: 9, type: !81)
!87 = !DILocation(line: 9, column: 7, scope: !70)
!88 = !DILocation(line: 10, column: 12, scope: !70)
!89 = !DILocation(line: 10, column: 18, scope: !70)
!90 = !DILocation(line: 10, column: 16, scope: !70)
!91 = !DILocation(line: 10, column: 3, scope: !70)
!92 = !DILocation(line: 10, column: 10, scope: !70)
!93 = !DILocalVariable(name: "c", scope: !70, file: !71, line: 11, type: !77)
!94 = !DILocation(line: 11, column: 8, scope: !70)
!95 = !DILocation(line: 11, column: 21, scope: !70)
!96 = !DILocation(line: 11, column: 27, scope: !70)
!97 = !DILocation(line: 11, column: 12, scope: !70)
!98 = !DILocation(line: 12, column: 12, scope: !70)
!99 = !DILocation(line: 12, column: 3, scope: !70)
!100 = !DILocation(line: 12, column: 10, scope: !70)
!101 = !DILocation(line: 13, column: 1, scope: !70)
