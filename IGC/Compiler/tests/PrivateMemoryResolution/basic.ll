;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey EnablePrivMemNewSOATranspose=0 --igc-private-mem-resolution -S %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that allocas to private memory are resolved
; with OptDisable set

define spir_kernel void @test_pmem(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @test_pmem(
; CHECK:  entry:
; CHECK:    [[SIMDLANEID16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:    [[SIMDLANEID:%.*]] = zext i16 [[SIMDLANEID16]] to i32
; CHECK:    [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:    [[TMP0:%.*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; CHECK:    [[TOTALPRIVATEMEMPERTHREAD:%.*]] = mul i32 [[SIMDSIZE]], 24
; CHECK:    [[PERTHREADOFFSET:%.*]] = mul i32 [[TMP0]], [[TOTALPRIVATEMEMPERTHREAD]]
;;
;; end of entryBuilder
;;
;; Next: per each alloca use
;;
; CHECK:    [[DST_ADDR_SECTIONBUFFEROFFSET:%.*]] = mul i32 [[SIMDSIZE]], 8
; CHECK:    [[DST_ADDR_BUFFEROFFSET:%.*]] = add i32 0, [[DST_ADDR_SECTIONBUFFEROFFSET]]
; CHECK:    [[PERLANEOFFSET2:%.*]] = mul i32 [[SIMDLANEID]], 8
; CHECK:    [[DST_ADDR_SIMDBUFOFF:%.*]] = add i32 [[DST_ADDR_BUFFEROFFSET]], [[PERLANEOFFSET2]]
; CHECK:    [[DST_ADDR_TOTALOFFSET:%.*]] = add {{.*}} [[PERTHREADOFFSET]], [[DST_ADDR_SIMDBUFOFF]]
; CHECK:    [[ZXT0:%.*]] = zext i32 [[DST_ADDR_TOTALOFFSET]] to i64
; CHECK:    [[DST_ADDR_PRIVATEBUFFERGEP:%.*]] = getelementptr i8, i8* %privateBase, i64 [[ZXT0]]
; CHECK:    [[DST_ADDR_PRIVATEBUFFER:%.*]] = bitcast i8* [[DST_ADDR_PRIVATEBUFFERGEP]] to i32 addrspace(1)**
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR_PRIVATEBUFFER]], align 8
;
; CHECK:    [[SRC_ADDR_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 16
; CHECK:    [[SRC_ADDR_BUFFEROFFSET:%.*]] = add i32 0, [[SRC_ADDR_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET3:%.*]] = mul i32 [[SIMDLANEID]], 8
; CHECK:    [[SRC_ADDR_SIMDBUFOFF:%.*]] = add i32 [[SRC_ADDR_BUFFEROFFSET]], [[PERLANEOFFSET3]]
; CHECK:    [[SRC_ADDR_TOTALOFFSET:%.*]] = add {{.*}} [[PERTHREADOFFSET]], [[SRC_ADDR_SIMDBUFOFF]]
; CHECK:    [[ZXT1:%.*]] = zext i32 [[SRC_ADDR_TOTALOFFSET]] to i64
; CHECK:    [[SRC_ADDR_PRIVATEBUFFERGEP:%.*]] = getelementptr i8, i8* %privateBase, i64 [[ZXT1]]
; CHECK:    [[SRC_ADDR_PRIVATEBUFFER:%.*]] = bitcast i8* [[SRC_ADDR_PRIVATEBUFFERGEP]] to i32 addrspace(1)**
; CHECK:    store i32 addrspace(1)* [[SRC:%.*]], i32 addrspace(1)** [[SRC_ADDR_PRIVATEBUFFER]], align 8
;
; CHECK:    [[TMP1:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[SRC_ADDR_PRIVATEBUFFER]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP1]], i64 0
; CHECK:    [[TMP2:%.*]] = load i32, i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    [[AA_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 0
; CHECK:    [[AA_BUFFEROFFSET:%.*]] = add i32 0, [[AA_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET:%.*]] = mul i32 [[SIMDLANEID]], 4
; CHECK:    [[AA_SIMDBUFOFF:%.*]] = add i32 [[AA_BUFFEROFFSET]], [[PERLANEOFFSET]]
; CHECK:    [[AA_TOTALOFFSET:%.*]] = add {{.*}} [[PERTHREADOFFSET]], [[AA_SIMDBUFOFF]]
; CHECK:    [[ZXT2:%.*]] = zext i32 [[AA_TOTALOFFSET]] to i64
; CHECK:    [[AA_PRIVATEBUFFERGEP:%.*]] = getelementptr i8, i8* %privateBase, i64 [[ZXT2]]
; CHECK:    [[AA_PRIVATEBUFFER:%.*]] = bitcast i8* [[AA_PRIVATEBUFFERGEP]] to i32*
; CHECK:    store i32 [[TMP2]], i32* [[AA_PRIVATEBUFFER]], align 4
;
; CHECK:    [[TMP3:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[SRC_ADDR_PRIVATEBUFFER]], align 8
; CHECK:    [[ARRAYIDX1:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP3]], i64 1
; CHECK:    [[TMP4:%.*]] = load i32, i32 addrspace(1)* [[ARRAYIDX1]], align 4
; CHECK:    [[BB_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 4
; CHECK:    [[BB_BUFFEROFFSET:%.*]] = add i32 0, [[BB_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET1:%.*]] = mul i32 [[SIMDLANEID]], 4
; CHECK:    [[BB_SIMDBUFOFF:%.*]] = add i32 [[BB_BUFFEROFFSET]], [[PERLANEOFFSET1]]
; CHECK:    [[BB_TOTALOFFSET:%.*]] = add {{.*}} [[PERTHREADOFFSET]], [[BB_SIMDBUFOFF]]
; CHECK:    [[ZXT3:%.*]] = zext i32 [[BB_TOTALOFFSET]] to i64
; CHECK:    [[BB_PRIVATEBUFFERGEP:%.*]] = getelementptr i8, i8* %privateBase, i64 [[ZXT3]]
; CHECK:    [[BB_PRIVATEBUFFER:%.*]] = bitcast i8* [[BB_PRIVATEBUFFERGEP]] to i32*
; CHECK:    store i32 [[TMP4]], i32* [[BB_PRIVATEBUFFER]], align 4
; CHECK:    [[TMP5:%.*]] = load i32, i32* [[AA_PRIVATEBUFFER]], align 4
; CHECK:    [[TMP6:%.*]] = load i32, i32* [[BB_PRIVATEBUFFER]], align 4
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
!5 = !{!"OptDisable", i1 true}
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
