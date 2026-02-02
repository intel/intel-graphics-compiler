;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole=1,DebugSoftwareNeedsA0Reset=1 | FileCheck %s
; RUN: igc_opt -platformNvl -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole=1,DebugSoftwareNeedsA0Reset=1 | FileCheck %s
; ------------------------------------------------
; EmitVISAPass - Testing addr_add instruction generation
; ------------------------------------------------
target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"

%dx.types.i8x8 = type { [8 x i8] }
%dx.types.u32 = type { i32 }
%dx.types.i8x16 = type { [16 x i8] }

@TGSM0 = internal addrspace(3) global [1024 x i8] undef, align 4
@TGSM1 = internal addrspace(3) global [40 x i8] undef, align 4
@llvm.used = appending global [2 x i8*] [i8* addrspacecast (i8 addrspace(3)* getelementptr inbounds ([1024 x i8], [1024 x i8] addrspace(3)* @TGSM0, i32 0, i32 0) to i8*), i8* addrspacecast (i8 addrspace(3)* getelementptr inbounds ([40 x i8], [40 x i8] addrspace(3)* @TGSM1, i32 0, i32 0) to i8*)], section "llvm.metadata"
@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; CHECK: mov {{.*}} A0
; CHECK: addr_add {{.*}} A0
; CHECK: mov {{.*}} A1
; CHECK: addr_add {{.*}} A1

define void @main(<8 x i32> %r0, i8* %privateBase, <4 x float> addrspace(2621440)* %0, i32 %1, i32 %2) #0 {
entry:
  %3 = call <2 x i32> @llvm.genx.GenISA.vectorUniform.v2i32()
  %4 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %4 to i32
  %5 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
  %GroupID_Y = bitcast float %5 to i32
  %6 = add i32 %1, 64
  %7 = inttoptr i32 %6 to <4 x float> addrspace(2490369)*
  %8 = add i32 %2, 832
  %9 = inttoptr i32 %8 to <4 x float> addrspace(2555917)*
  %b09 = inttoptr i32 %2 to <4 x float> addrspace(2555904)*
  %10 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 19)
  %11 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 18)
  %12 = zext i16 %11 to i32
  %13 = zext i16 %10 to i32
  %14 = add i32 %12, %13
  %15 = shl i32 %14, 6
  %16 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %17 = zext i16 %16 to i32
  %18 = add i32 %17, %15
  %19 = icmp ult i32 %18, 10
  br i1 %19, label %if0.then, label %entry.if0.end_crit_edge

entry.if0.end_crit_edge:                          ; preds = %entry
  br label %if0.end

if0.then:                                         ; preds = %entry
  %20 = shl nuw nsw i32 %18, 2
  %21 = add i32 %20, 1024
  %22 = inttoptr i32 %21 to float addrspace(3)*
  store float 0.000000e+00, float addrspace(3)* %22, align 4
  br label %if0.end

if0.end:                                          ; preds = %entry.if0.end_crit_edge, %if0.then
  call void @llvm.genx.GenISA.LSCFence(i32 3, i32 0, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %23 = shl i32 %GroupID_Y, 7
  %24 = add i32 %23, %GroupID_X
  %25 = call <1 x float> @llvm.genx.GenISA.ldrawvector.indexed.v1f32.p2555904v4f32(<4 x float> addrspace(2555904)* %b09, i32 0, i32 -2147483648, i1 false)
  %26 = extractelement <1 x float> %25, i32 0
  %27 = bitcast float %26 to i32
  %.not2 = icmp ult i32 %18, %27
  br i1 %.not2, label %loop0.breakc0.preheader, label %if0.end.loop0.end_crit_edge

if0.end.loop0.end_crit_edge:                      ; preds = %if0.end
  br label %loop0.end

loop0.breakc0.preheader:                          ; preds = %if0.end
  br label %loop0.breakc0

loop0.breakc0:                                    ; preds = %loop0.backedge.loop0.breakc0_crit_edge, %loop0.breakc0.preheader
  %.012 = phi <2 x i32> [ %.1, %loop0.backedge.loop0.breakc0_crit_edge ], [ zeroinitializer, %loop0.breakc0.preheader ]
  %.0 = phi <2 x i32> [ %.1, %loop0.backedge.loop0.breakc0_crit_edge ], [ <i32 -1, i32 -1>, %loop0.breakc0.preheader ]
  %dx.v32.r3.03 = phi i32 [ %dx.v32.r3.0.be, %loop0.backedge.loop0.breakc0_crit_edge ], [ %18, %loop0.breakc0.preheader ]
  %28 = shl i32 %dx.v32.r3.03, 3
  %29 = call <2 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v2i32.p2621440v4f32(<4 x float> addrspace(2621440)* %0, i32 %28, i32 8, i1 false)
  %30 = extractelement <2 x i32> %29, i32 1
  %31 = lshr i32 %30, 16
  %32 = and i32 %30, 65535
  %33 = icmp ult i32 %32, %24
  %34 = icmp ult i32 %24, %31
  %35 = or i1 %33, %34
  br i1 %35, label %loop0.breakc0.loop0.backedge_crit_edge, label %if1.end

loop0.breakc0.loop0.backedge_crit_edge:           ; preds = %loop0.breakc0
  br label %loop0.backedge

if1.end:                                          ; preds = %loop0.breakc0
  %36 = extractelement <2 x i32> %29, i32 0
  %37 = and i32 %36, 65535
  %38 = lshr i32 %36, 16
  %39 = extractelement <2 x i32> %.0, i32 0
  %40 = icmp ult i32 %36, 65536
  %41 = select i1 %40, i32 %39, i32 undef
  %42 = extractelement <2 x i32> %.0, i32 1
  %43 = icmp eq i32 %38, 1
  %44 = select i1 %43, i32 %42, i32 %41
  %.not46 = icmp ult i32 %37, %44
  %45 = select i1 %.not46, i32 %37, i32 %44
  %46 = insertelement <2 x i32> %.0, i32 %45, i32 %38
  %47 = extractelement <2 x i32> %.012, i32 0
  %48 = select i1 %40, i32 %47, i32 undef
  %49 = extractelement <2 x i32> %.012, i32 1
  %50 = select i1 %43, i32 %49, i32 %48
  %.not47 = icmp ult i32 %37, %50
  %51 = select i1 %.not47, i32 %50, i32 %37
  %52 = insertelement <2 x i32> %.012, i32 %51, i32 %38
  %53 = call i32 @llvm.genx.GenISA.ubfe(i32 11, i32 5, i32 %36)
  br label %loop0.backedge

loop0.backedge:                                   ; preds = %loop0.breakc0.loop0.backedge_crit_edge, %if1.end
  %.1 = phi <2 x i32> [ %.0, %loop0.breakc0.loop0.backedge_crit_edge ], [ %46, %if1.end ]
  %dx.v32.r3.0.be = add i32 %dx.v32.r3.03, 64
  %.not = icmp ult i32 %12, %13
  br i1 %.not, label %loop0.backedge.loop0.breakc0_crit_edge, label %loop0.end

loop0.backedge.loop0.breakc0_crit_edge:           ; preds = %loop0.backedge
  br label %loop0.breakc0

loop0.end:                                        ; preds = %if0.end.loop0.end_crit_edge, %loop0.backedge
  ret void
}

declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #1
declare void @llvm.genx.GenISA.LSCFence(i32, i32, i32) #2
declare void @llvm.genx.GenISA.threadgroupbarrier() #2
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904v4f32(<4 x float> addrspace(2555904)*, i32, i32, i1) #3
declare i32 @llvm.genx.GenISA.bfi(i32, i32, i32, i32) #1
declare i32 @llvm.genx.GenISA.intatomicrawA64.i32.p3i32.p3i32(i32 addrspace(3)*, i32 addrspace(3)*, i32, i32) #4
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555917v4f32(<4 x float> addrspace(2555917)*, i32, i32, i1) #3
declare i32 @llvm.genx.GenISA.ubfe(i32, i32, i32) #1
declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32) #1
declare void @llvm.genx.GenISA.storeraw.indexed.p2490369v4f32.i32(<4 x float> addrspace(2490369)*, i32, i32, i32, i1) #5
declare <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #3
declare <2 x i32> @llvm.genx.GenISA.vectorUniform.v2i32() #6
declare <2 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v2i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #3
declare <1 x float> @llvm.genx.GenISA.ldrawvector.indexed.v1f32.p2555904v4f32(<4 x float> addrspace(2555904)*, i32, i32, i1) #3
declare <3 x float> @llvm.genx.GenISA.ldrawvector.indexed.v3f32.p2555917v4f32(<4 x float> addrspace(2555917)*, i32, i32, i1) #3
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nounwind readnone willreturn }
attributes #2 = { convergent nounwind }
attributes #3 = { argmemonly nounwind readonly willreturn }
attributes #4 = { argmemonly nounwind }
attributes #5 = { argmemonly nounwind writeonly }
attributes #6 = { nounwind }

!dx.version = !{!0}
!dx.valver = !{!0}
!dx.shaderModel = !{!1}
!dx.resources = !{!2}
!dx.entryPoints = !{!11}
!llvm.ident = !{!14}
!igc.functions = !{!15}
!IGCMetadata = !{!21}

!0 = !{i32 1, i32 0}
!1 = !{!"cs", i32 6, i32 0}
!2 = !{!3, !6, !8, null}
!3 = !{!4}
!4 = !{i32 0, %dx.types.i8x8 addrspace(1)* undef, !"T0", i32 0, i32 0, i32 1, i32 12, i32 0, !5}
!5 = !{i32 1, i32 8}
!6 = !{!7}
!7 = !{i32 0, %dx.types.u32 addrspace(1)* undef, !"U0", i32 0, i32 1, i32 1, i32 11, i1 false, i1 false, i1 false, null}
!8 = !{!9, !10}
!9 = !{i32 0, %dx.types.i8x16 addrspace(2)* undef, !"CB0", i32 0, i32 13, i32 1, i32 16, null}
!10 = !{i32 1, %dx.types.i8x16 addrspace(2)* undef, !"CB1", i32 0, i32 0, i32 1, i32 16, null}
!11 = distinct !{null, !"main", null, !2, !12}
!12 = !{i32 0, i64 256, i32 4, !13}
!13 = !{i32 64, i32 1, i32 1}
!14 = !{!"dxbc2dxil 1.2"}
!15 = !{void (<8 x i32>, i8*, <4 x float> addrspace(2621440)*, i32, i32)* @main, !16}
!16 = !{!17, !18}
!17 = !{!"function_type", i32 0}
!18 = !{!"implicit_arg_desc", !19, !20}
!19 = !{i32 0}
!20 = !{i32 13}
!21 = !{!"ModuleMD", !22 }
!22 = !{!"isPrecise", i1 false}
