;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Verifies that predicated loads are emitted correctly for uniform loads

define spir_kernel void @test_uniform_dst(ptr addrspace(1) align 8 %in0, ptr addrspace(1) align 4 %in1, ptr addrspace(1) align 2 %in2, i32 %predicate) {
entry:
  %p = icmp slt i32 0, %predicate

; Check that alias of merge val is created for this case: dSize == 4 && (vSize > 64 || vSize == 6) && Align >= 8
; CHECK: .decl [[VAR0:.*]] v_type=G type=d num_elts=6 align=wordx32
; CHECK: .decl [[ALS0:.*]] v_type=G type=uq num_elts=3 align=wordx32 alias=<[[VAR0]], 0>
; CHECK: .decl [[RES01:.*]] v_type=G type=uq num_elts=3 align=wordx32

; CHECK: .decl [[RES11:.*]] v_type=G type=ud num_elts=2 align=wordx32

; Check that alias of merge val is created for this case: dSize == 8 && vSize < 64 && Align == 4
; CHECK: .decl [[VAR1:.*]] v_type=G type=q num_elts=2 align=wordx32
; CHECK: .decl [[ALS1:.*]] v_type=G type=ud num_elts=4 align=wordx32 alias=<[[VAR1]], 0>
; CHECK: .decl [[RES21:.*]] v_type=G type=ud num_elts=4 align=wordx32

; CHECK: .decl [[VAR2:.*]] v_type=G type=d num_elts=6 align=dword
; CHECK: .decl [[ALS2:.*]] v_type=G type=uq num_elts=3 align=dword alias=<[[VAR2]], 0>
; CHECK: .decl [[VAR3:.*]] v_type=G type=uq num_elts=3 align=wordx32

; CHECK: .decl [[VAR4:.*]] v_type=G type=d num_elts=1 align=wordx32

; CHECK: .decl [[VAR5:.*]] v_type=G type=d num_elts=1 align=dword
; CHECK: .decl [[VAR6:.*]] v_type=G type=d num_elts=1 align=wordx32

; check emitVectorCopy for merge val for SIMT1 transposed load
; no need to predicate copy of temp value, since we copied merge value to the temp before load
; CHECK: mov (M1_NM, 4) [[VAR0]](0,0)<1> 0x0:d
; CHECK: mov (M1_NM, 2) [[VAR0]](0,4)<1> 0x0:d
; CHECK: mov (M1_NM, 2) [[RES01]](0,0)<1> [[ALS0]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 1) [[RES01]](0,2)<1> [[ALS0]](0,2)<0;1,0>
; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[RES01]]:d64x3t  flat[{{.*}}]:a64
; CHECK: mov (M1_NM, 2) {{.*}}(0,0)<1> [[RES01]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 1) {{.*}}(0,2)<1> [[RES01]](0,2)<0;1,0>
  %res0 = call <6 x i32> @llvm.genx.GenISA.PredicatedLoad.v6i32.p1.v6i32(ptr addrspace(1) %in0, i64 8, i1 %p, <6 x i32> zeroinitializer)

; case of immediate merge value when dSize == 8 && vSize < 64 && Align == 4. Creating immediate of different type
; CHECK: mov (M1_NM, 2) [[RES11]](0,0)<1> 0x0:ud
; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[RES11]]:d32x2t  flat[{{.*}}]:a64
; CHECK: mov (M1_NM, 2) {{.*}}(0,0)<1> [[RES11]](0,0)<1;1,0>
  %res1 = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1.i64(ptr addrspace(1) %in1, i64 4, i1 %p, i64 0)

; case of non-immediate merge value when dSize == 8 && vSize < 64 && Align == 4.
; CHECK: mov (M1_NM, 2) [[VAR1]](0,0)<1> 0x0:q
; CHECK: mov (M1_NM, 4) [[RES21]](0,0)<1> [[ALS1]](0,0)<1;1,0>
; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[RES21]]:d32x4t  flat[{{.*}}]:a64
; CHECK: mov (M1_NM, 4) {{.*}}(0,0)<1> [[RES21]](0,0)<1;1,0>
  %res2 = call <2 x i64> @llvm.genx.GenISA.PredicatedLoad.v2i64.p1.v2i64(ptr addrspace(1) %in1, i64 4, i1 %p, <2 x i64> zeroinitializer)

; case when merge value is used as destination, and we need temp variable, so emit predicated copy after predicated load
; CHECK: mov (M1_NM, 1) [[VAR2]](0,0)<1> predicate(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) [[VAR2]](0,1)<1> predicate(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) [[VAR2]](0,2)<1> predicate(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) [[VAR2]](0,3)<1> predicate(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) [[VAR2]](0,4)<1> predicate(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) [[VAR2]](0,5)<1> predicate(0,0)<0;1,0>
; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[VAR3]]:d64x3t  flat[{{.*}}]:a64
; CHECK: (P1.any) mov (M1_NM, 2) [[ALS2]](0,0)<1> [[VAR3]](0,0)<1;1,0>
; CHECK: (P1.any) mov (M1_NM, 1) [[ALS2]](0,2)<1> [[VAR3]](0,2)<0;1,0>
  %mergeV = insertelement <6 x i32> undef, i32 %predicate, i32 0
  %mergeV1 = insertelement <6 x i32> %mergeV, i32 %predicate, i32 1
  %mergeV2 = insertelement <6 x i32> %mergeV1, i32 %predicate, i32 2
  %mergeV3 = insertelement <6 x i32> %mergeV2, i32 %predicate, i32 3
  %mergeV4 = insertelement <6 x i32> %mergeV3, i32 %predicate, i32 4
  %mergeV5 = insertelement <6 x i32> %mergeV4, i32 %predicate, i32 5
  %res3 = call <6 x i32> @llvm.genx.GenISA.PredicatedLoad.v6i32.p1.v6i32(ptr addrspace(1) %in0, i64 8, i1 %p, <6 x i32> %mergeV5)

; Sub-DW aligned
; CHECK: mov (M1_NM, 1) [[VAR4]](0,0)<1> 0x0:d
; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[VAR4]]:d32  flat[{{.*}}]:a64
; CHECK: mov (M1_NM, 1) {{.*}}(0,0)<1> [[VAR4]](0,0)<0;1,0>
  %res4 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %in2, i64 2, i1 %p, i32 0)

; Sub-DW aligned with predicated copy after load
; CHECK: add (M1_NM, 1) [[VAR5]](0,0)<1> predicate(0,0)<0;1,0> 0x5:w
; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[VAR6]]:d32  flat[{{.*}}]:a64
; CHECK: (P1.any) mov (M1_NM, 1) [[VAR5]](0,0)<1> [[VAR6]](0,0)<0;1,0>
  %mergeV6 = add i32 %predicate, 5
  %res5 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %in2, i64 2, i1 %p, i32 %mergeV6)

  ret void
}

define spir_kernel void @test_non_uniform_dst(ptr addrspace(1) align 8 %in0, ptr addrspace(1) align 4 %in1, ptr addrspace(1) align 2 %in2, ptr addrspace(1) align 8 %out, i16 %localIdX) {
entry:
  %idx = zext i16 %localIdX to i32
  %p = icmp slt i32 0, %idx
  %idx64 = zext i32 %idx to i64

  ; Check that alias of merge val is created for this case: dSize == 4 && (vSize > 64 || vSize == 6) && Align >= 8
  ; CHECK: .decl [[VAR0:.*]] v_type=G type=d num_elts=6 align=wordx32
  ; CHECK: .decl [[ALS0:.*]] v_type=G type=uq num_elts=3 align=wordx32 alias=<[[VAR0]], 0>
  ; CHECK: .decl [[RES01:.*]] v_type=G type=uq num_elts=3 align=wordx32

  ; CHECK: .decl [[RES11:.*]] v_type=G type=ud num_elts=2 align=wordx32

  ; Check that alias of merge val is created for this case: dSize == 8 && vSize < 64 && Align == 4
  ; CHECK: .decl [[VAR1:.*]] v_type=G type=q num_elts=2 align=wordx32
  ; CHECK: .decl [[ALS1:.*]] v_type=G type=ud num_elts=4 align=wordx32 alias=<[[VAR1]], 0>
  ; CHECK: .decl [[RES21:.*]] v_type=G type=ud num_elts=4 align=wordx32

  ; CHECK: .decl [[VAR2:.*]] v_type=G type=d num_elts=192 align=wordx32
  ; CHECK: .decl [[ALS2:.*]] v_type=G type=uq num_elts=96 align=wordx32 alias=<[[VAR2]], 0>
  ; CHECK: .decl [[VAR3:.*]] v_type=G type=uq num_elts=3 align=wordx32

  ; CHECK: .decl [[VAR4:.*]] v_type=G type=d num_elts=1 align=wordx32

  ; CHECK: .decl [[VAR5:.*]] v_type=G type=d num_elts=32 align=wordx32
  ; CHECK: .decl [[VAR6:.*]] v_type=G type=d num_elts=1 align=wordx32

  ; check emitVectorCopy for merge val for SIMT1 transposed load
  ; load destination is non-uniform, so even though source pointer is uniform, some lanes may get loaded value, some - merge value
  ; therefore we need to store merge value into final destination and then copy loaded value there for lanes that are enabled by predicate
  ; CHECK: mov (M1_NM, 4) [[VAR0]](0,0)<1> 0x0:d
  ; CHECK: mov (M1_NM, 2) [[VAR0]](0,4)<1> 0x0:d
  ; CHECK: mov (M1, 32) [[DST:.*]](0,0)<1> [[ALS0]](0,0)<0;1,0>
  ; CHECK: mov (M1, 32) [[DST]](4,0)<1> [[ALS0]](0,1)<0;1,0>
  ; CHECK: mov (M1, 32) [[DST]](8,0)<1> [[ALS0]](0,2)<0;1,0>
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[RES01]]:d64x3t  flat[{{.*}}]:a64
  ; CHECK: (P1) mov (M1, 32) [[DST]](0,0)<1> [[RES01]](0,0)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[DST]](4,0)<1> [[RES01]](0,1)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[DST]](8,0)<1> [[RES01]](0,2)<0;1,0>
  %res0 = call <6 x i32> @llvm.genx.GenISA.PredicatedLoad.v6i32.p1.v6i32(ptr addrspace(1) %in0, i64 8, i1 %p, <6 x i32> zeroinitializer)
  %out0 = getelementptr <6 x i32>, ptr addrspace(1) %out, i64 %idx64
  store <6 x i32> %res0, ptr addrspace(1) %out0, align 8

  ; case of immediate merge value when dSize == 8 && vSize < 64 && Align == 4. Creating immediate of different type
  ; CHECK: mov (M1, 32) [[DST1:.*]](0,0)<1> 0x0:ud
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[RES11]]:d32x2t  flat[{{.*}}]:a64
  ; CHECK: (P1) mov (M1, 32) [[DST1]](0,0)<1> [[RES11]](0,0)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[DST1]](2,0)<1> [[RES11]](0,1)<0;1,0>
  %idx1 = add i64 %idx64, 1
  %res1 = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1.i64(ptr addrspace(1) %in1, i64 4, i1 %p, i64 0)
  %out1 = getelementptr i64, ptr addrspace(1) %out, i64 %idx1
  store i64 %res1, ptr addrspace(1) %out1, align 8

  ; case of non-immediate merge value when dSize == 8 && vSize < 64 && Align == 4.
  ; CHECK: mov (M1_NM, 2) [[VAR1]](0,0)<1> 0x0:q
  ; CHECK: mov (M1, 32) [[DST2:.*]](0,0)<1> [[ALS1]](0,0)<0;1,0>
  ; CHECK: mov (M1, 32) [[DST2]](2,0)<1> [[ALS1]](0,1)<0;1,0>
  ; CHECK: mov (M1, 32) [[DST2]](4,0)<1> [[ALS1]](0,2)<0;1,0>
  ; CHECK: mov (M1, 32) [[DST2]](6,0)<1> [[ALS1]](0,3)<0;1,0>
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[RES21]]:d32x4t  flat[{{.*}}]:a64
  ; CHECK: (P1) mov (M1, 32) [[DST2]](0,0)<1> [[RES21]](0,0)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[DST2]](2,0)<1> [[RES21]](0,1)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[DST2]](4,0)<1> [[RES21]](0,2)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[DST2]](6,0)<1> [[RES21]](0,3)<0;1,0>
  %idx2 = add i64 %idx64, 2
  %res2 = call <2 x i64> @llvm.genx.GenISA.PredicatedLoad.v2i64.p1.v2i64(ptr addrspace(1) %in1, i64 4, i1 %p, <2 x i64> zeroinitializer)
  %out2 = getelementptr <2 x i64>, ptr addrspace(1) %out, i64 %idx2
  store <2 x i64> %res2, ptr addrspace(1) %out2, align 8

  ; case when merge value is used as destination, and we need temp variable, so emit predicated copy after predicated load
  ; CHECK: mov (M1, 32) [[VAR2]](0,0)<1> idx(0,0)<1;1,0>
  ; CHECK: mov (M1, 32) [[VAR2]](2,0)<1> idx(0,0)<1;1,0>
  ; CHECK: mov (M1, 32) [[VAR2]](4,0)<1> idx(0,0)<1;1,0>
  ; CHECK: mov (M1, 32) [[VAR2]](6,0)<1> idx(0,0)<1;1,0>
  ; CHECK: mov (M1, 32) [[VAR2]](8,0)<1> idx(0,0)<1;1,0>
  ; CHECK: mov (M1, 32) [[VAR2]](10,0)<1> idx(0,0)<1;1,0>
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[VAR3]]:d64x3t  flat[{{.*}}]:a64
  ; CHECK: (P1) mov (M1, 32) [[ALS2]](0,0)<1> [[VAR3]](0,0)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[ALS2]](4,0)<1> [[VAR3]](0,1)<0;1,0>
  ; CHECK: (P1) mov (M1, 32) [[ALS2]](8,0)<1> [[VAR3]](0,2)<0;1,0>
  %idx3 = add i64 %idx64, 3
  %mergeV = insertelement <6 x i32> undef, i32 %idx, i32 0
  %mergeV1 = insertelement <6 x i32> %mergeV, i32 %idx, i32 1
  %mergeV2 = insertelement <6 x i32> %mergeV1, i32 %idx, i32 2
  %mergeV3 = insertelement <6 x i32> %mergeV2, i32 %idx, i32 3
  %mergeV4 = insertelement <6 x i32> %mergeV3, i32 %idx, i32 4
  %mergeV5 = insertelement <6 x i32> %mergeV4, i32 %idx, i32 5
  %res3 = call <6 x i32> @llvm.genx.GenISA.PredicatedLoad.v6i32.p1.v6i32(ptr addrspace(1) %in0, i64 8, i1 %p, <6 x i32> %mergeV5)
  %out3 = getelementptr <6 x i32>, ptr addrspace(1) %out, i64 %idx3
  store <6 x i32> %res3, ptr addrspace(1) %out3, align 8

  ; Sub-DW aligned
  ; CHECK: mov (M1, 32) [[DST4:.*]](0,0)<1> 0x0:d
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[VAR4]]:d32  flat[{{.*}}]:a64
  ; CHECK: (P1) mov (M1, 32) [[DST4]](0,0)<1> [[VAR4]](0,0)<0;1,0>
  %idx4 = add i64 %idx64, 4
  %res4 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %in2, i64 2, i1 %p, i32 0)
  %out4 = getelementptr i32, ptr addrspace(1) %out, i64 %idx4
  store i32 %res4, ptr addrspace(1) %out4, align 4

  ; Sub-DW aligned with predicated copy after load
  ; CHECK: add (M1, 32) [[VAR5]](0,0)<1> idx(0,0)<1;1,0> 0x5:w
  ; CHECK: (P1.any) lsc_load.ugm (M1_NM, 1)  [[VAR6]]:d32  flat[{{.*}}]:a64
  ; CHECK: (P1) mov (M1, 32) [[VAR5]](0,0)<1> [[VAR6]](0,0)<0;1,0>
  %idx5 = add i64 %idx64, 5
  %mergeV6 = add i32 %idx, 5
  %res5 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %in2, i64 2, i1 %p, i32 %mergeV6)
  %out5 = getelementptr i32, ptr addrspace(1) %out, i64 %idx5
  store i32 %res5, ptr addrspace(1) %out5, align 4

  ret void
}

declare <6 x i32> @llvm.genx.GenISA.PredicatedLoad.v6i32.p1.v6i32(ptr addrspace(1), i64, i1, <6 x i32>)
declare i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1.i64(ptr addrspace(1), i64, i1, i64)
declare <2 x i64> @llvm.genx.GenISA.PredicatedLoad.v2i64.p1.v2i64(ptr addrspace(1), i64, i1, <2 x i64>)
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!1, !100}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_uniform_dst, !3}
!3 = !{!4}
!4 = !{!"function_type", i32 0}
!2 = !{!"FuncMD", !5, !6, !50, !51}
!5 = !{!"FuncMDMap[0]", ptr @test_uniform_dst}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !170, !174, !175, !176}
!170 = !{!"argAllocMDListVec[0]", !171, !172, !173}
!171 = !{!"type", i32 0}
!172 = !{!"extensionType", i32 -1}
!173 = !{!"indexType", i32 -1}
!174 = !{!"argAllocMDListVec[1]", !171, !172, !173}
!175 = !{!"argAllocMDListVec[2]", !171, !172, !173}
!176 = !{!"argAllocMDListVec[3]", !171, !172, !173}
!50 = !{!"FuncMDMap[1]", ptr @test_non_uniform_dst}
!51 = !{!"FuncMDValue[1]", !52}
!52 = !{!"resAllocMD", !53}
!53 = !{!"argAllocMDList", !54, !55, !56, !57, !58}
!54 = !{!"argAllocMDListVec[0]", !171, !172, !173}
!55 = !{!"argAllocMDListVec[1]", !171, !172, !173}
!56 = !{!"argAllocMDListVec[2]", !171, !172, !173}
!57 = !{!"argAllocMDListVec[3]", !171, !172, !173}
!58 = !{!"argAllocMDListVec[4]", !171, !172, !173}
!100 = !{ptr @test_non_uniform_dst, !101}
!101 = !{!4, !102}
!102 = !{!"implicit_arg_desc", !103}
!103 = !{i32 8}
