;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ResourceLoopAnalysis fuses consecutive ldraw_indexed calls
; that use the same non-uniform resource into a single resource loop.
;
; REQUIRES: llvm-14-plus, regkeys
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformPtl -simd-mode 16 -inputcs -igc-emit-visa -regkey DumpVISAASMToConsole,FuseResourceLoop=0 -S %s 2>&1 | FileCheck %s --check-prefix=CHECK-FRL0
; RUN: igc_opt --typed-pointers -platformPtl -simd-mode 16 -inputcs -igc-emit-visa -regkey DumpVISAASMToConsole,FuseResourceLoop=1 -S %s 2>&1 | FileCheck %s --check-prefix=CHECK-FRL1

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

define spir_kernel void @test_ldraw_fusion(<8 x i32> %r0, i32 %runtime_value_0, i32 %runtime_value_1, <4 x float> addrspace(2)* %runtime_value_2, <4 x float> addrspace(2)* %runtime_value_3, i32 %runtime_value_4, i32 %runtime_value_5) #0 {
entry:
; COM: check fuse resource loop disabled case
; CHECK-FRL0:  _main_0:
; CHECK-FRL0:      (!P1) goto (M1, 16) _test_ldraw_fusion_003
; CHECK-FRL0:      _test_ldraw_fusion_002:
; CHECK-FRL0:      _test_ldraw_fusion_004__opt_resource_loop:
; CHECK-FRL0:      _test_ldraw_fusion_005__opt_resource_loop:
; CHECK-FRL0:      _test_ldraw_fusion_006__opt_resource_loop:
; CHECK-FRL0:      (P6) lsc_load.ugm (M1, 16)  V0083:d32  bss(V0088)[V0080]:a32
; CHECK-FRL0-NEXT: (!P6) goto (M1, 16) _test_ldraw_fusion_006__opt_resource_loop
; CHECK-FRL0:      _test_ldraw_fusion_007__opt_resource_loop:
; CHECK-FRL0:      _test_ldraw_fusion_008__opt_resource_loop:
; CHECK-FRL0:      _test_ldraw_fusion_009__opt_resource_loop:
; CHECK-FRL0:      _test_ldraw_fusion_003:
; CHECK-FRL0-NEXT: ret (M1, 1)

; COM: check fuse resource loop enabled case
; CHECK-FRL1:  _main_0:
; CHECK-FRL1:      (!P1) goto (M1, 16) _test_ldraw_fusion_003
; CHECK-FRL1:      _test_ldraw_fusion_002:
; CHECK-FRL1:      _test_ldraw_fusion_004__opt_resource_loop:
; CHECK-FRL1:      _test_ldraw_fusion_005__opt_resource_loop:
; CHECK-FRL1:      lifetime.start V0083
; CHECK-FRL1-NEXT: lifetime.start V0084
; CHECK-FRL1:      _test_ldraw_fusion_006__opt_resource_loop:
; CHECK-FRL1:      (P6) lsc_load.ugm (M1, 16)  V0083:d32  bss(V0089)[V0080]:a32                 /// $72
; CHECK-FRL1-NEXT: or (M1, 16) V0090(0,0)<1> V0080(0,0)<1;1,0> 0x4:d                            /// $73
; CHECK-FRL1-NEXT: (P6) lsc_load.ugm (M1, 16)  V0084:d32  bss(V0089)[V0090]:a32                 /// $74
; CHECK-FRL1-NEXT: (!P6) goto (M1, 16) _test_ldraw_fusion_006__opt_resource_loop                /// $75
; CHECK-FRL1:      _test_ldraw_fusion_007__opt_resource_loop:
; CHECK-FRL1:      _test_ldraw_fusion_008__opt_resource_loop:
; CHECK-FRL1-NOT:  _test_ldraw_fusion_009__opt_resource_loop:
; CHECK-FRL1:      _test_ldraw_fusion_003:
; CHECK-FRL1-NEXT: ret (M1, 1)

  %0 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 12)
  %1 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %1 to i32
  %t06 = inttoptr i32 %runtime_value_1 to i8 addrspace(11010048)*
  %2 = shl i32 %GroupID_X, 6
  %3 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %4 = zext i16 %3 to i32
  %ThreadID_X = add i32 %2, %4
  %5 = ptrtoint <4 x float> addrspace(2)* %runtime_value_3 to i64
  %6 = add i64 %5, 272
  %7 = inttoptr i64 %6 to <4 x i32> addrspace(2)*
  %8 = load <4 x i32>, <4 x i32> addrspace(2)* %7, align 4, !invariant.load !114
  %9 = extractelement <4 x i32> %8, i32 3
  %10 = shl i32 %9, 2
  %11 = call fast <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p11010048__Buffer_Typed_DIM_Resource.p11010048__Buffer_Typed_DIM_Resource(i32 %10, i32 0, i32 0, i32 0, i8 addrspace(11010048)* undef, i8 addrspace(11010048)* %t06, i32 0, i32 0, i32 0)
  %12 = extractelement <4 x float> %11, i32 0
  %13 = bitcast float %12 to i32
  %14 = icmp ult i32 %ThreadID_X, %13
  br i1 %14, label %15, label %._crit_edge13, !dx.controlflow.hints !110, !stats.blockFrequency.digits !111, !stats.blockFrequency.scale !112

._crit_edge13:                                    ; preds = %0
  br label %62, !stats.blockFrequency.digits !111, !stats.blockFrequency.scale !113

15:                                               ; preds = %0
  %16 = add i32 %runtime_value_0, 64
  %17 = inttoptr i32 %16 to i8 addrspace(15073281)*
  %18 = add i32 %runtime_value_1, 64
  %19 = inttoptr i32 %18 to i8 addrspace(11010049)*
  %20 = call fast <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p11010049__Buffer_Typed_DIM_Resource.p11010049__Buffer_Typed_DIM_Resource(i32 %ThreadID_X, i32 0, i32 0, i32 0, i8 addrspace(11010049)* undef, i8 addrspace(11010049)* %19, i32 0, i32 0, i32 0)
  %21 = extractelement <4 x float> %20, i32 0
  %22 = bitcast float %21 to i32
  %23 = lshr i32 %22, 24
  %24 = ptrtoint <4 x float> addrspace(2)* %runtime_value_2 to i64
  %25 = shl nuw nsw i32 %23, 4
  %26 = or i32 %25, 16384
  %27 = zext i32 %26 to i64
  %28 = add i64 %24, %27
  %29 = inttoptr i64 %28 to <3 x float> addrspace(2)*
  %30 = load <3 x float>, <3 x float> addrspace(2)* %29, align 4
  %31 = extractelement <3 x float> %30, i32 2
  %32 = extractelement <3 x float> %30, i32 1
  %33 = extractelement <3 x float> %30, i32 0
  %34 = shl nuw nsw i32 %23, 6
  %35 = add i32 %runtime_value_4, %34
  %"t0,2" = inttoptr i32 %35 to <4 x float> addrspace(2621440)*
  %36 = shl i32 %22, 5
  %37 = and i32 %36, 536870880
  %38 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %"t0,2", i32 %37, i32 32, i1 false)
  %39 = or i32 %37, 16
  %40 = or i32 %37, 4
  %41 = call fast <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2621440v4f32(<4 x float> addrspace(2621440)* %"t0,2", i32 %39, i32 16, i1 false)
  %42 = extractelement <4 x float> %41, i32 0
  %43 = extractelement <4 x float> %41, i32 1
  %44 = extractelement <4 x float> %41, i32 2
  %45 = extractelement <4 x float> %41, i32 3
  %46 = and i32 %38, 1073741823
  %47 = add i32 %runtime_value_5, %34
  %"t0,3" = inttoptr i32 %47 to <4 x float> addrspace(2621440)*
  %48 = mul i32 %46, 144
  %49 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %"t0,3", i32 %48, i32 16, i1 false)
  %50 = or i32 %48, 4
  %51 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %"t0,3", i32 %50, i32 4, i1 false)
  %52 = or i32 %48, 12
  %53 = call <3 x float> @llvm.genx.GenISA.ldrawvector.indexed.v3f32.p2621440v4f32(<4 x float> addrspace(2621440)* %"t0,2", i32 %40, i32 4, i1 false)
  %54 = extractelement <3 x float> %53, i32 2
  %55 = extractelement <3 x float> %53, i32 1
  %56 = extractelement <3 x float> %53, i32 0
  %57 = fadd fast float %56, %33
  %58 = fadd fast float %55, %32
  %59 = fadd fast float %54, %31
  %60 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %"t0,3", i32 %52, i32 4, i1 false)
  %61 = and i32 %60, 2

  %sum0 = add i32 %60, %61
  %bc42 = bitcast float %42 to i32
  %bc57 = bitcast float %57 to i32
  %chain0 = add i32 %sum0, %49
  %chain1 = add i32 %chain0, %51
  %chain2 = add i32 %chain1, %bc42
  %chain3 = add i32 %chain2, %bc57
  %sink_ptr = inttoptr i32 %chain3 to i32*
  store i32 %chain3, i32* %sink_ptr
  br label %62

62:
  ret void
}

declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32) #1
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #2
declare <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p11010048__Buffer_Typed_DIM_Resource.p11010048__Buffer_Typed_DIM_Resource(i32, i32, i32, i32, i8 addrspace(11010048)*, i8 addrspace(11010048)*, i32, i32, i32) #3
declare <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p11010049__Buffer_Typed_DIM_Resource.p11010049__Buffer_Typed_DIM_Resource(i32, i32, i32, i32, i8 addrspace(11010049)*, i8 addrspace(11010049)*, i32, i32, i32) #4

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #5
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #6
declare <3 x float> @llvm.genx.GenISA.ldrawvector.indexed.v3f32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #7

attributes #0 = { convergent nounwind "null-pointer-is-valid"="true" }
attributes #1 = { argmemonly nounwind readonly willreturn }

!IGCMetadata = !{!0}
!igc.functions = !{!100}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (<8 x i32>, i32, i32, <4 x float> addrspace(2)*, <4 x float> addrspace(2)*, i32, i32)* @test_ldraw_fusion}
!3 = !{!"FuncMDValue[0]", !4, !5}
!4 = !{!"functionType", !"KernelFunction"}
!5 = !{!"resAllocMD", !6, !7, !8, !9, !20}
!6 = !{!"uavsNumType", i32 0}
!7 = !{!"srvsNumType", i32 0}
!8 = !{!"samplersNumType", i32 0}
!9 = !{!"argAllocMDList", !10, !12, !13, !14, !15, !16, !17}
!10 = !{!"argAllocMDListVec[0]", !30, !31, !32}
!12 = !{!"argAllocMDListVec[1]", !30, !31, !32}
!13 = !{!"argAllocMDListVec[2]", !30, !31, !32}
!14 = !{!"argAllocMDListVec[3]", !30, !31, !32}
!15 = !{!"argAllocMDListVec[4]", !30, !31, !32}
!16 = !{!"argAllocMDListVec[5]", !30, !31, !32}
!17 = !{!"argAllocMDListVec[6]", !30, !31, !32}
!20 = !{!"inlineSamplersMD"}
!30 = !{!"type", i32 0}
!31 = !{!"extensionType", i32 -1}
!32 = !{!"indexType", i32 -1}

!100 = !{void (<8 x i32>, i32, i32, <4 x float> addrspace(2)*, <4 x float> addrspace(2)*, i32, i32)* @test_ldraw_fusion, !101}
!101 = !{!102, !103}
!102 = !{!"function_type", i32 0}
!103 = !{!"implicit_arg_desc", !104, !105, !106, !107}
!104 = !{i32 0}
!105 = !{i32 1}
!106 = !{i32 8}
!107 = !{i32 9}

!110 = distinct !{!110, !"dx.controlflow.hints", i32 1}
!111 = !{!"1441151880758558720"}
!112 = !{!"-57"}
!113 = !{!"-58"}
!114 = !{null}
