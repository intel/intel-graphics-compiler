; UNSUPPORTED: system-windows
; REQUIRES: regkeys

; RUN: igc_opt -S -dce -platformpvc -rev-id B -has-emulated-64-bit-insts -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl tmp15 v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl tmp16 v_type=G type=f num_elts=8 align=dword

; CHECK: add (M1, 16) tmp15(0,0)<1> tmp16(0,0)<0;1,0> tmp15(0,0)<1;1,0>
; CHECK: add (M1, 16) tmp15(1,0)<1> tmp16(0,1)<0;1,0> tmp15(1,0)<1;1,0>
; CHECK: add (M1, 16) tmp15(2,0)<1> tmp16(0,2)<0;1,0> tmp15(2,0)<1;1,0>
; CHECK: add (M1, 16) tmp15(3,0)<1> tmp16(0,3)<0;1,0> tmp15(3,0)<1;1,0>
; CHECK: add (M1, 16) tmp15(4,0)<1> tmp16(0,4)<0;1,0> tmp15(4,0)<1;1,0>
; CHECK: add (M1, 16) tmp15(5,0)<1> tmp16(0,5)<0;1,0> tmp15(5,0)<1;1,0>
; CHECK: add (M1, 16) tmp15(6,0)<1> tmp16(0,6)<0;1,0> tmp15(6,0)<1;1,0>
; CHECK: add (M1, 16) tmp15(7,0)<1> tmp16(0,7)<0;1,0> tmp15(7,0)<1;1,0>

define spir_kernel void @blam(half addrspace(1)* %arg, half addrspace(1)* %arg1, half addrspace(1)* %arg2, float %arg3, i8 addrspace(1)* %arg4, float addrspace(1)* %arg5, <8 x i32> %arg6, <8 x i32> %arg7, i8* %arg8, i32 %arg9, i32 %arg10, i32 %arg11, i32 %arg12, i32 %arg13) {
bb:
  br label %bb14

bb14:                                             ; preds = %bb19, %bb
  %tmp = phi float [ 0.000000e+00, %bb ], [ %tmp, %bb19 ]
  %tmp15 = phi <8 x float> [ zeroinitializer, %bb ], [ %tmp18, %bb19 ]
  %tmp16 = insertelement <8 x float> zeroinitializer, float 0.000000e+00, i64 0
  %tmp17 = fadd <8 x float> %tmp16, %tmp15
  %tmp18 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %tmp17, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %bb19

bb19:                                             ; preds = %bb14
  br label %bb14
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = distinct !{void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32, i32, i32, i32)* @blam, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
!4 = distinct !{!"ModuleMD", !5}
!5 = distinct !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float, i8 addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32, i32, i32, i32)* @blam}
!7 = distinct !{!"FuncMDValue[0]", !8}
!8 = distinct !{!"resAllocMD", !9}
!9 = distinct !{!"argAllocMDList", !10}
!10 = distinct !{!"argAllocMDListVec[0]"}