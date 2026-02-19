;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; COM: Depends on other commit with m_instTypes initialization, enable when fixed
; UNSUPPORTED: system-windows, llvm-17-plus
;
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --regkey EnablePrivMemNewSOATranspose=0 --igc-private-mem-resolution -S %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that allocas to private memory are resolved
; with OptDisable not set (optimization enabled)

define spir_kernel void @test_pmem(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @test_pmem(
; CHECK:  entry:
; CHECK:    [[SIMDLANEID16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:    [[SIMDLANEID:%.*]] = zext i16 [[SIMDLANEID16]] to i32
; CHECK:    [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:    [[R0_5:%.*]] = extractelement <8 x i32> [[R0:%.*]], i32 5
; CHECK:    [[PRIVATEBASE1:%.*]] = and i32 [[R0_5]], -1024
; CHECK:    [[DST_ADDR_SECTIONBUFFEROFFSET:%.*]] = mul i32 [[SIMDSIZE]], 8
; CHECK:    [[DST_ADDR_SIMDBUFFEROFFSET:%.*]] = add i32 0, [[DST_ADDR_SECTIONBUFFEROFFSET]]
; CHECK:    [[PERLANEOFFSET3:%.*]] = mul i32 [[SIMDLANEID]], 8
; CHECK:    [[DST_ADDR_TOTALOFFSET:%.*]] = add i32 [[DST_ADDR_SIMDBUFFEROFFSET]], [[PERLANEOFFSET3]]
; CHECK:    [[DST_ADDR_THREADOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[DST_ADDR_TOTALOFFSET]]
; CHECK:    [[DST_ADDR_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[DST_ADDR_THREADOFFSET]] to i32 addrspace(1)**
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[SRC_ADDR_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 16
; CHECK:    [[SRC_ADDR_SIMDBUFFEROFFSET:%.*]] = add i32 0, [[SRC_ADDR_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET4:%.*]] = mul i32 [[SIMDLANEID]], 8
; CHECK:    [[SRC_ADDR_TOTALOFFSET:%.*]] = add i32 [[SRC_ADDR_SIMDBUFFEROFFSET]], [[PERLANEOFFSET4]]
; CHECK:    [[SRC_ADDR_THREADOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[SRC_ADDR_TOTALOFFSET]]
; CHECK:    [[SRC_ADDR_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[SRC_ADDR_THREADOFFSET]] to i32 addrspace(1)**
; CHECK:    store i32 addrspace(1)* [[SRC:%.*]], i32 addrspace(1)** [[SRC_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[TMP0:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[SRC_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP0]], i64 0
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    [[AA_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 0
; CHECK:    [[AA_SIMDBUFFEROFFSET:%.*]] = add i32 0, [[AA_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET:%.*]] = mul i32 [[SIMDLANEID]], 4
; CHECK:    [[AA_TOTALOFFSET:%.*]] = add i32 [[AA_SIMDBUFFEROFFSET]], [[PERLANEOFFSET]]
; CHECK:    [[AA_THREADOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[AA_TOTALOFFSET]]
; CHECK:    [[AA_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[AA_THREADOFFSET]] to i32*
; CHECK:    store i32 [[TMP1]], i32* [[AA_PRIVATEBUFFERPTR]], align 4
; CHECK:    [[TMP2:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[SRC_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[ARRAYIDX1:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP2]], i64 1
; CHECK:    [[TMP3:%.*]] = load i32, i32 addrspace(1)* [[ARRAYIDX1]], align 4
; CHECK:    [[BB_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 4
; CHECK:    [[BB_SIMDBUFFEROFFSET:%.*]] = add i32 0, [[BB_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET2:%.*]] = mul i32 [[SIMDLANEID]], 4
; CHECK:    [[BB_TOTALOFFSET:%.*]] = add i32 [[BB_SIMDBUFFEROFFSET]], [[PERLANEOFFSET2]]
; CHECK:    [[BB_THREADOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[BB_TOTALOFFSET]]
; CHECK:    [[BB_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[BB_THREADOFFSET]] to i32*
; CHECK:    store i32 [[TMP3]], i32* [[BB_PRIVATEBUFFERPTR]], align 4
; CHECK:    [[TMP4:%.*]] = load i32, i32* [[AA_PRIVATEBUFFERPTR]], align 4
; CHECK:    [[TMP5:%.*]] = load i32, i32* [[BB_PRIVATEBUFFERPTR]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32 addrspace(1)*, align 8
  %aa = alloca i32, align 4
  %bb = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store i32 addrspace(1)* %src, i32 addrspace(1)** %src.addr, align 8
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %src.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4
  store i32 %1, i32* %aa, align 4
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %src.addr, align 8
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 1
  %3 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  store i32 %3, i32* %bb, align 4
  %4 = load i32, i32* %aa, align 4
  %5 = load i32, i32* %bb, align 4
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 false}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem, !18}
!18 = !{!19, !20}
!19 = !{!"function_type", i32 0}
!20 = !{!"implicit_arg_desc", !21, !22, !23, !24, !26}
!21 = !{i32 0}
!22 = !{i32 1}
!23 = !{i32 13}
!24 = !{i32 15, !25}
!25 = !{!"explicit_arg_num", i32 0}
!26 = !{i32 15, !27}
!27 = !{!"explicit_arg_num", i32 1}
