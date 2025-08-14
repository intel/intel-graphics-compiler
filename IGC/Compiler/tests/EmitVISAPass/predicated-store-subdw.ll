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

; Verifies predicated stores for sub-DW values with uniform destination address
; and non-uniform stored value

; CHECK: .decl [[OFFSET:.*]] v_type=G type=uw num_elts=2 align=dword alias=<[[OFFSET_ALIAS:.*]], 0>
; CHECK: .decl [[DATA:.*]] v_type=G type=uw num_elts=1 align=word alias=<[[DATA_ALIAS:.*]], 0>
; CHECK: barrier
; CHECK: mov (M1, 32) [[LOCAL_IDX:.*]](0,0)<1> localIdX_0(0,0)<1;1,0>
; CHECK: or (M1, 32) [[LOC_IDYZ:.*]](0,0)<1> localIdY(0,0)<1;1,0> localIdZ(0,0)<1;1,0>
; CHECK: cmp.eq (M1, 32) [[F_LOC_IDYZ:.*]] [[LOC_IDYZ]](0,0)<1;1,0> 0x0:w
; CHECK: cmp.eq (M1, 32) [[F_LOC_IDX_MATCH:.*]] [[LOCAL_IDX]](0,0)<1;1,0> loc_idx(0,0)<0;1,0>
; CHECK: and (M1, 32) [[F_LOC_IDX_MATCH]] [[F_LOC_IDX_MATCH]] [[F_LOC_IDYZ]]
; CHECK: mov (M1_NM, 1) [[FVAR:.*]](0,0)<1> [[F_LOC_IDX_MATCH]]
; CHECK: setp (M1_NM, 32) [[F_EMASK:.*]] 0x0:ud
; CHECK: cmp.eq (M1, 32) [[F_EMASK]] [[DUMMY:.*]](0,0)<0;1,0> [[DUMMY]](0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) [[VAR_EMASK:.*]](0,0)<1> [[F_EMASK]]
; CHECK: and (M1_NM, 1) [[VAR_EMASK]](0,0)<1> [[VAR_EMASK]](0,0)<0;1,0> [[FVAR]](0,0)<0;1,0>
; CHECK: fbl (M1_NM, 1) [[OFFSET_ALIAS]](0,0)<1> [[VAR_EMASK]](0,0)<0;1,0>
; CHECK: shl (M1_NM, 1) [[OFFSET_ALIAS]](0,0)<1> [[OFFSET_ALIAS]](0,0)<0;1,0> 0x1:w
; CHECK: addr_add (M1_NM, 1) [[ADDR:.*]](0)<1> &{{.*}} [[OFFSET]](0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) [[DATA_ALIAS]](0,0)<1> r[[[ADDR]](0),0]<0;1,0>:hf
; CHECK: mov (M1_NM, 1) [[DATA1:.*]](0,0)<1> [[DATA]](0,0)<0;1,0>
; CHECK: ([[F_LOC_IDX_MATCH]].any) lsc_store.slm (M1_NM, 1)  flat[Trunc]:a32  [[DATA1]]:d16u32

define spir_kernel void @test_kernel_work_group_broadcast(i32 addrspace(1)* align 4 %dst, half addrspace(1)* align 2 %bcastval, i32 %final_x_size, i32 %final_y_size, i32 %final_z_size, i32 %loc_idx, i32 %loc_idy, i32 %loc_idz, i64 %start_address, i64 %end_address, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %0 = ptrtoint half addrspace(1)* %bcastval to i64
  %idxprom = zext i16 %localIdX to i64
  %1 = shl nuw nsw i64 %idxprom, 1
  %2 = add i64 %1, %0
  %3 = inttoptr i64 %2 to half addrspace(1)*
  %4 = load half, half addrspace(1)* %3, align 2
  %idxprom7 = zext i32 %loc_idx to i64
  %5 = shl nuw nsw i64 %idxprom7, 1
  %6 = add i64 %5, %0
  %7 = inttoptr i64 %6 to i16 addrspace(1)*
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %8 = zext i16 %localIdX to i32
  %9 = or i16 %localIdY, %localIdZ
  %10 = icmp eq i32 %8, %loc_idx
  %11 = icmp eq i16 %9, 0
  %12 = and i1 %10, %11
  call void @llvm.genx.GenISA.PredicatedStore.p3f16.f16(half addrspace(3)* null, half %4, i64 2, i1 %12)
  ret void
}

declare void @llvm.genx.GenISA.threadgroupbarrier()
declare void @llvm.genx.GenISA.PredicatedStore.p3f16.f16(half addrspace(3)*, half, i64, i1)

!igc.functions = !{!3}
!IGCMetadata = !{!17}

!3 = !{void (i32 addrspace(1)*, half addrspace(1)*, i32, i32, i32, i32, i32, i32, i64, i64, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @test_kernel_work_group_broadcast, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13, !15}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 7}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 10}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{i32 15, !16}
!16 = !{!"explicit_arg_num", i32 1}
!17 = !{!"ModuleMD", !139}
!139 = !{!"FuncMD", !140, !141}
!140 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, half addrspace(1)*, i32, i32, i32, i32, i32, i32, i64, i64, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @test_kernel_work_group_broadcast}
!141 = !{!"FuncMDValue[0]", !177}
!177 = !{!"resAllocMD", !181}
!181 = !{!"argAllocMDList", !182, !186, !187, !188, !189, !190, !191, !192, !193, !194, !195, !196, !197, !198, !199, !200, !201, !202}
!182 = !{!"argAllocMDListVec[0]", !183, !184, !185}
!183 = !{!"type", i32 0}
!184 = !{!"extensionType", i32 -1}
!185 = !{!"indexType", i32 -1}
!186 = !{!"argAllocMDListVec[1]", !183, !184, !185}
!187 = !{!"argAllocMDListVec[2]", !183, !184, !185}
!188 = !{!"argAllocMDListVec[3]", !183, !184, !185}
!189 = !{!"argAllocMDListVec[4]", !183, !184, !185}
!190 = !{!"argAllocMDListVec[5]", !183, !184, !185}
!191 = !{!"argAllocMDListVec[6]", !183, !184, !185}
!192 = !{!"argAllocMDListVec[7]", !183, !184, !185}
!193 = !{!"argAllocMDListVec[8]", !183, !184, !185}
!194 = !{!"argAllocMDListVec[9]", !183, !184, !185}
!195 = !{!"argAllocMDListVec[10]", !183, !184, !185}
!196 = !{!"argAllocMDListVec[11]", !183, !184, !185}
!197 = !{!"argAllocMDListVec[12]", !183, !184, !185}
!198 = !{!"argAllocMDListVec[13]", !183, !184, !185}
!199 = !{!"argAllocMDListVec[14]", !183, !184, !185}
!200 = !{!"argAllocMDListVec[15]", !183, !184, !185}
!201 = !{!"argAllocMDListVec[16]", !183, !184, !185}
!202 = !{!"argAllocMDListVec[17]", !183, !184, !185}
