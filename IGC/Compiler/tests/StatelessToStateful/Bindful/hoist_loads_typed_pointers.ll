;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;

; RUN: igc_opt %s -S -o - -igc-stateless-to-stateful-resolution | FileCheck %s

define spir_kernel void @func_with_phinode_1(i1 %n, i32 addrspace(1)* %r, <4 x i32> addrspace(1)* %otp, i64 %idx1, i64 %idx2, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i8 addrspace(1)* %s2, i8 addrspace(1)* %s3, i32 %s4, i32 %s5, i32 %bufferOffset) #0 {
bb1:
  %add.ptr1 = getelementptr inbounds i32, i32 addrspace(1)* %r, i64 16
  %add.ptr2 = getelementptr inbounds i32, i32 addrspace(1)* %r, i64 128
  ; CHECK: [[TMP0:%.*]] = load <4 x i32>, <4 x i32> addrspace(131072)*
  br i1 %n, label %bb3, label %bb2
bb2:
  ; CHECK: [[TMP1:%.*]] = load <4 x i32>, <4 x i32> addrspace(131072)*
  br label %bb3
bb3:
  ; CHECK: phi <4 x i32> [ [[TMP1]], %bb2 ], [ [[TMP0]], %bb1 ]
  %phinode = phi i32 addrspace(1)* [ %add.ptr2 , %bb2 ], [ %add.ptr1, %bb1 ]
  %cast = bitcast i32 addrspace(1)* %phinode to <4 x i32> addrspace(1)*
  %ld = load <4 x i32>, <4 x i32> addrspace(1)* %cast, align 4
  store <4 x i32> %ld, <4 x i32> addrspace(1)* %otp, align 4
  ret void
}


define spir_kernel void @func_with_phinode_2(i1 %n, i32 addrspace(1)* %r, <4 x i32> addrspace(1)* %otp, i64 %idx1, i64 %idx2, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i8 addrspace(1)* %s2, i8 addrspace(1)* %s3, i32 %s4, i32 %s5, i32 %bufferOffset) #0 {
bb1:
  %add.ptr1 = getelementptr inbounds i32, i32 addrspace(1)* %r, i64 16
  %add.ptr2 = getelementptr inbounds i32, i32 addrspace(1)* %r, i64 128
  ; CHECK-NOT: load <4 x i32>, <4 x i32> addrspace(131072)*
  br i1 %n, label %bb3, label %bb2
bb2:
  ; CHECK-NOT: load <4 x i32>, <4 x i32> addrspace(131072)*
  store i8 100, i8 addrspace(1)* %s2, align 2
  br label %bb3
bb3:
  ; CHECK: %phinode = phi i32 addrspace(1)*
  %phinode = phi i32 addrspace(1)* [ %add.ptr2 , %bb2 ], [ %add.ptr1, %bb1 ]
  %cast = bitcast i32 addrspace(1)* %phinode to <4 x i32> addrspace(1)*
  %ld = load <4 x i32>, <4 x i32> addrspace(1)* %cast, align 4
  store <4 x i32> %ld, <4 x i32> addrspace(1)* %otp, align 4
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!IGCMetadata = !{!0}
!igc.functions = !{!78, !93}

!0 = !{!"ModuleMD", !1, !2, !6, !69, !65}
!1 = !{!"isPrecise", i1 false}
!2 = !{!"compOpt", !3, !4, !5}
!3 = !{!"DenormsAreZero", i1 false}
!4 = !{!"CorrectlyRoundedDivSqrt", i1 false}
!5 = !{!"OptDisable", i1 false}
!6 = !{!"FuncMD", !7, !8, !91, !92}
!7 = !{!"FuncMDMap[0]", void (i1, i32 addrspace(1)*, <4 x i32> addrspace(1)*, i64, i64, <8 x i32>, <8 x i32>, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @func_with_phinode_1}
!8 = !{!"FuncMDValue[0]", !9, !10, !14, !15, !16, !17, !32, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68}
!91 = !{!"FuncMDMap[1]", void (i1, i32 addrspace(1)*, <4 x i32> addrspace(1)*, i64, i64, <8 x i32>, <8 x i32>, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @func_with_phinode_2}
!92 = !{!"FuncMDValue[1]", !9, !10, !14, !15, !16, !17, !32, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68}
!9 = !{!"localOffsets"}
!10 = !{!"workGroupWalkOrder", !11, !12, !13}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 0}
!13 = !{!"dim2", i32 0}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
!16 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!17 = !{!"rtInfo", !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28}
!18 = !{!"hasTraceRayPayload", i1 false}
!19 = !{!"hasHitAttributes", i1 false}
!20 = !{!"hasCallableData", i1 false}
!21 = !{!"ShaderStackSize", i32 0}
!22 = !{!"IsCallStackHandler", i1 false}
!23 = !{!"ShaderName", !""}
!24 = !{!"NOSSize", i32 0}
!25 = !{!"Entries"}
!26 = !{!"SpillUnions"}
!27 = !{!"CustomHitAttrSizeInBytes", i32 0}
!28 = !{!"Types", !29, !30, !31}
!29 = !{!"FrameStartTys"}
!30 = !{!"ArgumentTys"}
!31 = !{!"FullFrameTys"}
!32 = !{!"resAllocMD", !33, !34, !35, !36, !55}
!33 = !{!"uavsNumType", i32 0}
!34 = !{!"srvsNumType", i32 0}
!35 = !{!"samplersNumType", i32 0}
!36 = !{!"argAllocMDList", !37, !41, !44, !45, !46, !48, !50, !52, !53, !54}
!37 = !{!"argAllocMDListVec[0]", !38, !39, !40}
!38 = !{!"type", i32 0}
!39 = !{!"extensionType", i32 -1}
!40 = !{!"indexType", i32 -1}
!41 = !{!"argAllocMDListVec[1]", !42, !39, !43}
!42 = !{!"type", i32 1}
!43 = !{!"indexType", i32 0}
!44 = !{!"argAllocMDListVec[2]", !38, !39, !40}
!45 = !{!"argAllocMDListVec[3]", !38, !39, !40}
!46 = !{!"argAllocMDListVec[4]", !42, !39, !47}
!47 = !{!"indexType", i32 1}
!48 = !{!"argAllocMDListVec[5]", !42, !39, !49}
!49 = !{!"indexType", i32 2}
!50 = !{!"argAllocMDListVec[6]", !42, !39, !51}
!51 = !{!"indexType", i32 3}
!52 = !{!"argAllocMDListVec[7]", !38, !39, !40}
!53 = !{!"argAllocMDListVec[8]", !38, !39, !40}
!54 = !{!"argAllocMDListVec[9]", !38, !39, !40}
!55 = !{!"inlineSamplersMD"}
!56 = !{!"maxByteOffsets"}
!57 = !{!"IsInitializer", i1 false}
!58 = !{!"IsFinalizer", i1 false}
!59 = !{!"CompiledSubGroupsNumber", i32 0}
!60 = !{!"isCloned", i1 false}
!61 = !{!"hasInlineVmeSamplers", i1 false}
!62 = !{!"localSize", i32 0}
!63 = !{!"localIDPresent", i1 false}
!64 = !{!"groupIDPresent", i1 false}
!65 = !{!"privateMemoryPerWI", i32 0}
!66 = !{!"globalIDPresent", i1 false}
!67 = !{!"isUniqueEntry", i1 false}
!68 = !{!"UserAnnotations"}
!69 = !{!"pushInfo", !70, !71, !74, !75, !76, !77}
!70 = !{!"pushableAddresses"}
!71 = !{!"dynamicBufferInfo", !72, !73}
!72 = !{!"firstIndex", i32 0}
!73 = !{!"numOffsets", i32 0}
!74 = !{!"MaxNumberOfPushedBuffers", i32 0}
!75 = !{!"inlineConstantBufferSlot", i32 -1}
!76 = !{!"inlineConstantBufferOffset", i32 -1}
!77 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!78 = !{void (i1, i32 addrspace(1)*, <4 x i32> addrspace(1)*, i64, i64, <8 x i32>, <8 x i32>, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @func_with_phinode_1, !79}
!93 = !{void (i1, i32 addrspace(1)*, <4 x i32> addrspace(1)*, i64, i64, <8 x i32>, <8 x i32>, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @func_with_phinode_2, !79}
!79 = !{!80, !81}
!80 = !{!"function_type", i32 0}
!81 = !{!"implicit_arg_desc", !82, !83, !84, !85, !86, !87, !88, !89}
!82 = !{i32 0}
!83 = !{i32 1}
!84 = !{i32 12}
!85 = !{i32 40}
!86 = !{i32 41}
!87 = !{i32 42}
!88 = !{i32 43}
!89 = !{i32 14, !90}
!90 = !{!"explicit_arg_num", i32 1}