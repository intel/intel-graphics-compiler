;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --regkey EnablePrivMemNewSOATranspose=0 --igc-private-mem-resolution -S %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that allocas to private memory are resolved
; with OptDisable not set (optimization enabled)
; Transpose version
; Also checks:
; * Load/store to private memory handling
; * Corresponding lifetime marks handling

define spir_kernel void @test_pmem(<8 x i32> addrspace(1)* %dst, <8 x i32> addrspace(1)* %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
;
; CHECK-LABEL: @test_pmem(
; CHECK:  entry:
; CHECK:    [[SIMDLANEID16:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:    [[SIMDLANEID:%.*]] = zext i16 [[SIMDLANEID16]] to i32
; CHECK:    [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:    [[R0_5:%.*]] = extractelement <8 x i32> [[R0:%.*]], i32 5
; CHECK:    [[PRIVATEBASE1:%.*]] = and i32 [[R0_5]], -1024
; CHECK:    [[DST_ADDR_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 64
; CHECK:    [[DST_ADDR_BUFFEROFFSET:%.*]] = add i32 0, [[DST_ADDR_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET3:%.*]] = mul i32 [[SIMDLANEID]], 8
; CHECK:    [[DST_ADDR_SIMDBUFOFF:%.*]] = add i32 [[DST_ADDR_BUFFEROFFSET]], [[PERLANEOFFSET3]]
; CHECK:    [[DST_ADDR_BASEOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[DST_ADDR_SIMDBUFOFF]]
; CHECK:    [[DST_ADDR_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[DST_ADDR_BASEOFFSET]] to <8 x i32> addrspace(1)**
; CHECK:    call void @llvm.lifetime.start.p0p1v8i32(i64 4, <8 x i32> addrspace(1)** [[DST_ADDR_PRIVATEBUFFERPTR]])
; CHECK:    [[AA_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 0
; CHECK:    [[AA_BUFFEROFFSET:%.*]] = add i32 0, [[AA_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET:%.*]] = mul i32 [[SIMDLANEID]], 32
; CHECK:    [[AA_SIMDBUFOFF:%.*]] = add i32 [[AA_BUFFEROFFSET]], [[PERLANEOFFSET]]
; CHECK:    [[AA_THREADOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[AA_SIMDBUFOFF]]
; CHECK:    [[AA_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[AA_THREADOFFSET]] to <8 x i32>*
; CHECK:    [[TMP0:%.*]] = mul i32 [[SIMDSIZE]], 32
; CHECK:    [[TMP1:%.*]] = mul i32 0, [[TMP0]]
; CHECK:    [[TMP2:%.*]] = add i32 [[AA_THREADOFFSET]], [[TMP1]]
; CHECK:    [[TMP3:%.*]] = mul i32 [[SIMDSIZE]], 32
; CHECK:    [[TMP4:%.*]] = mul i32 0, [[TMP3]]
; CHECK:    [[TMP5:%.*]] = add i32 [[AA_THREADOFFSET]], [[TMP4]]
; CHECK:    store <8 x i32> addrspace(1)* [[DST:%.*]], <8 x i32> addrspace(1)** [[DST_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[SRC_ADDR_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 72
; CHECK:    [[SRC_ADDR_BUFFEROFFSET:%.*]] = add i32 0, [[SRC_ADDR_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET4:%.*]] = mul i32 [[SIMDLANEID]], 8
; CHECK:    [[SRC_ADDR_SIMDBUFOFF:%.*]] = add i32 [[SRC_ADDR_BUFFEROFFSET]], [[PERLANEOFFSET4]]
; CHECK:    [[SRC_ADDR_THREADOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[SRC_ADDR_SIMDBUFOFF]]
; CHECK:    [[SRC_ADDR_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[SRC_ADDR_THREADOFFSET]] to <8 x i32> addrspace(1)**
; CHECK:    store <8 x i32> addrspace(1)* [[SRC:%.*]], <8 x i32> addrspace(1)** [[SRC_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[TMP6:%.*]] = load <8 x i32> addrspace(1)*, <8 x i32> addrspace(1)** [[SRC_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds <8 x i32>, <8 x i32> addrspace(1)* [[TMP6]], i64 0
; CHECK:    [[TMP7:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    [[TMP8:%.*]] = inttoptr i32 [[TMP5]] to <8 x i32>*
; CHECK:    store <8 x i32> [[TMP7]], <8 x i32>* [[TMP8]], align 4
; CHECK:    [[TMP9:%.*]] = load <8 x i32> addrspace(1)*, <8 x i32> addrspace(1)** [[SRC_ADDR_PRIVATEBUFFERPTR]], align 8
; CHECK:    [[ARRAYIDX1:%.*]] = getelementptr inbounds <8 x i32>, <8 x i32> addrspace(1)* [[TMP9]], i64 1
; CHECK:    [[TMP10:%.*]] = load <8 x i32>, <8 x i32> addrspace(1)* [[ARRAYIDX1]], align 4
; CHECK:    [[BB_SECTIONOFFSET:%.*]] = mul i32 [[SIMDSIZE]], 32
; CHECK:    [[BB_BUFFEROFFSET:%.*]] = add i32 0, [[BB_SECTIONOFFSET]]
; CHECK:    [[PERLANEOFFSET2:%.*]] = mul i32 [[SIMDLANEID]], 32
; CHECK:    [[BB_SIMDBUFOFF:%.*]] = add i32 [[BB_BUFFEROFFSET]], [[PERLANEOFFSET2]]
; CHECK:    [[BB_THREADOFFSET:%.*]] = add {{.*}} i32 [[PRIVATEBASE1]], [[BB_SIMDBUFOFF]]
; CHECK:    [[BB_PRIVATEBUFFERPTR:%.*]] = inttoptr i32 [[BB_THREADOFFSET]] to <8 x i32>*
; CHECK:    [[TMP11:%.*]] = mul i32 [[SIMDSIZE]], 32
; CHECK:    [[TMP12:%.*]] = mul i32 0, [[TMP11]]
; CHECK:    [[TMP13:%.*]] = add i32 [[BB_THREADOFFSET]], [[TMP12]]
; CHECK:    [[TMP14:%.*]] = mul i32 [[SIMDSIZE]], 32
; CHECK:    [[TMP15:%.*]] = mul i32 0, [[TMP14]]
; CHECK:    [[TMP16:%.*]] = add i32 [[BB_THREADOFFSET]], [[TMP15]]
; CHECK:    [[TMP17:%.*]] = inttoptr i32 [[TMP16]] to <8 x i32>*
; CHECK:    store <8 x i32> [[TMP10]], <8 x i32>* [[TMP17]], align 4
; CHECK:    call void @llvm.lifetime.end.p0p1v8i32(i64 4, <8 x i32> addrspace(1)** [[DST_ADDR_PRIVATEBUFFERPTR]])
; CHECK:    [[TMP18:%.*]] = inttoptr i32 [[TMP2]] to <8 x i32>*
; CHECK:    [[TMP19:%.*]] = load <8 x i32>, <8 x i32>* [[TMP18]], align 4
; CHECK:    [[TMP20:%.*]] = inttoptr i32 [[TMP13]] to <8 x i32>*
; CHECK:    [[TMP21:%.*]] = load <8 x i32>, <8 x i32>* [[TMP20]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca <8 x i32> addrspace(1)*, align 8
  %src.addr = alloca <8 x i32> addrspace(1)*, align 8
  call void @llvm.lifetime.start.p0p1v8i32(i64 4, <8 x i32> addrspace(1)** %dst.addr)
  %aa = alloca <8 x i32>, align 4
  %bb = alloca <8 x i32>, align 4
  call void @llvm.lifetime.start.p0v8i32(i64 32, <8 x i32>* %aa)
  store <8 x i32> addrspace(1)* %dst, <8 x i32> addrspace(1)** %dst.addr, align 8
  store <8 x i32> addrspace(1)* %src, <8 x i32> addrspace(1)** %src.addr, align 8
  %0 = load <8 x i32> addrspace(1)*, <8 x i32> addrspace(1)** %src.addr, align 8
  %arrayidx = getelementptr inbounds <8 x i32>, <8 x i32> addrspace(1)* %0, i64 0
  %1 = load <8 x i32>, <8 x i32> addrspace(1)* %arrayidx, align 4
  store <8 x i32> %1, <8 x i32>* %aa, align 4
  %2 = load <8 x i32> addrspace(1)*, <8 x i32> addrspace(1)** %src.addr, align 8
  %arrayidx1 = getelementptr inbounds <8 x i32>, <8 x i32> addrspace(1)* %2, i64 1
  %3 = load <8 x i32>, <8 x i32> addrspace(1)* %arrayidx1, align 4
  store <8 x i32> %3, <8 x i32>* %bb, align 4
  call void @llvm.lifetime.end.p0p1v8i32(i64 4, <8 x i32> addrspace(1)** %dst.addr)
  %4 = load <8 x i32>, <8 x i32>* %aa, align 4
  %5 = load <8 x i32>, <8 x i32>* %bb, align 4
  call void @llvm.lifetime.end.p0v8i32(i64 32, <8 x i32>* %aa)
  ret void
}

declare void @llvm.lifetime.start.p0p1v8i32(i64, <8 x i32> addrspace(1)**)
declare void @llvm.lifetime.end.p0p1v8i32(i64, <8 x i32> addrspace(1)**)
declare void @llvm.lifetime.start.p0v8i32(i64, <8 x i32>*)
declare void @llvm.lifetime.end.p0v8i32(i64, <8 x i32>*)

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 false}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", void (<8 x i32> addrspace(1)*, <8 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (<8 x i32> addrspace(1)*, <8 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem, !18}
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
