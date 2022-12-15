;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey EnableZEBinary=0 -enable-debugify --igc-programscope-constant-resolve -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,%LLVM_DEPENDENT_CHECK_PREFIX%
; ------------------------------------------------
; ProgramScopeConstantResolution
; ------------------------------------------------

; Debug-info related checks
;
; For llvm 14 check-debugify treats missing debug location on globalbase getter
; at the begining of BB as a warning, while on earlier llvm versions its treated as an error.
;
; CHECK-LLVM-14-PLUS: CheckModuleDebugify: PASS

@a = internal addrspace(2) constant [2 x i32] [i32 0, i32 1], align 4
@d = internal addrspace(1) global i32 addrspace(2)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(2)* @a, i32 0, i32 0), align 8
@c = internal addrspace(1) global i32 0, align 4
@b = common addrspace(1) global i32 0, align 4
@llvm.used = appending global [3 x i8*] [i8* addrspacecast (i8 addrspace(2)* bitcast ([2 x i32] addrspace(2)* @a to i8 addrspace(2)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(1)* @c to i8 addrspace(1)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(2)* addrspace(1)* @d to i8 addrspace(1)*) to i8*)], section "llvm.metadata"

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_program(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i8 addrspace(1)* %globalBase, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_program(
; CHECK:  entry:
; CHECK:    [[OFFC:%.*]] = getelementptr i8, i8 addrspace(1)* %globalBase, i32 8
; CHECK:    [[CASTC:%.*]] = bitcast i8 addrspace(1)* [[OFFC]] to i32 addrspace(1)*
; CHECK:    [[OFFD:%.*]] = getelementptr i8, i8 addrspace(1)* %globalBase, i32 0
; CHECK:    [[CASTD:%.*]] = bitcast i8 addrspace(1)* [[OFFD]] to i32 addrspace(2)* addrspace(1)*
; CHECK:    [[OFFA:%.*]] = getelementptr i8, i8 addrspace(2)* %constBase, i32 0
; CHECK:    [[CASTA:%.*]] = bitcast i8 addrspace(2)* [[OFFA]] to [2 x i32] addrspace(2)*
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[AA:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = getelementptr inbounds [2 x i32], [2 x i32] addrspace(2)* [[CASTA]], i64 0, i64 1
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(2)* [[TMP0]], align 4
; CHECK:    store i32 [[TMP1]], i32* [[AA]], align 4
; CHECK:    [[TMP2:%.*]] = load i32 addrspace(2)*, i32 addrspace(2)* addrspace(1)* [[CASTD]], align 8
; CHECK:    [[TMP3:%.*]] = load i32, i32 addrspace(2)* [[TMP2]], align 4
; CHECK:    store i32 [[TMP3]], i32 addrspace(1)* [[CASTC]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %aa = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  %0 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(2)* @a, i64 0, i64 1
  %1 = load i32, i32 addrspace(2)* %0, align 4
  store i32 %1, i32* %aa, align 4
  %2 = load i32 addrspace(2)*, i32 addrspace(2)* addrspace(1)* @d, align 8
  %3 = load i32, i32 addrspace(2)* %2, align 4
  store i32 %3, i32 addrspace(1)* @c, align 4
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }

!IGCMetadata = !{!3}
!igc.functions = !{!51}

!3 = !{!"ModuleMD", !4, !6, !44}
!4 = !{!"compOpt", !5}
!5 = !{!"OptDisable", i1 true}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8 addrspace(1)*, i8*, i32)* @test_program}
!8 = !{!"FuncMDValue[0]", !9, !10, !14, !32, !34, !36, !38, !40, !42}
!9 = !{!"localOffsets"}
!10 = !{!"workGroupWalkOrder", !11, !12, !13}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 0}
!13 = !{!"dim2", i32 0}
!14 = !{!"resAllocMD", !15, !16}
!15 = !{!"samplersNumType", i32 0}
!16 = !{!"argAllocMDList", !17, !21, !24, !25, !27, !29, !31}
!17 = !{!"argAllocMDListVec[0]", !18, !19, !20}
!18 = !{!"type", i32 1}
!19 = !{!"extensionType", i32 -1}
!20 = !{!"indexType", i32 0}
!21 = !{!"argAllocMDListVec[1]", !22, !19, !23}
!22 = !{!"type", i32 0}
!23 = !{!"indexType", i32 -1}
!24 = !{!"argAllocMDListVec[2]", !22, !19, !23}
!25 = !{!"argAllocMDListVec[3]", !18, !19, !26}
!26 = !{!"indexType", i32 1}
!27 = !{!"argAllocMDListVec[4]", !18, !19, !28}
!28 = !{!"indexType", i32 2}
!29 = !{!"argAllocMDListVec[5]", !18, !19, !30}
!30 = !{!"indexType", i32 3}
!31 = !{!"argAllocMDListVec[6]", !22, !19, !23}
!32 = !{!"m_OpenCLArgAddressSpaces", !33}
!33 = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
!34 = !{!"m_OpenCLArgAccessQualifiers", !35}
!35 = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
!36 = !{!"m_OpenCLArgTypes", !37}
!37 = !{!"m_OpenCLArgTypesVec[0]", !"int*"}
!38 = !{!"m_OpenCLArgBaseTypes", !39}
!39 = !{!"m_OpenCLArgBaseTypesVec[0]", !"int*"}
!40 = !{!"m_OpenCLArgTypeQualifiers", !41}
!41 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!42 = !{!"m_OpenCLArgNames", !43}
!43 = !{!"m_OpenCLArgNamesVec[0]", !"dst"}
!44 = !{!"inlineProgramScopeOffsets", !45, !46, !47, !48, !49, !50}
!45 = !{!"inlineProgramScopeOffsetsMap[0]", [2 x i32] addrspace(2)* @a}
!46 = !{!"inlineProgramScopeOffsetsValue[0]", i32 0}
!47 = !{!"inlineProgramScopeOffsetsMap[1]", i32 addrspace(1)* @c}
!48 = !{!"inlineProgramScopeOffsetsValue[1]", i32 8}
!49 = !{!"inlineProgramScopeOffsetsMap[2]", i32 addrspace(2)* addrspace(1)* @d}
!50 = !{!"inlineProgramScopeOffsetsValue[2]", i32 0}
!51 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8 addrspace(1)*, i8*, i32)* @test_program, !52}
!52 = !{!53, !54}
!53 = !{!"function_type", i32 0}
!54 = !{!"implicit_arg_desc", !55, !56, !57, !58, !59, !60}
!55 = !{i32 0}
!56 = !{i32 1}
!57 = !{i32 10}
!58 = !{i32 11}
!59 = !{i32 12}
!60 = !{i32 14, !61}
!61 = !{!"explicit_arg_num", i32 0}
