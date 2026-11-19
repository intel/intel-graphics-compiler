;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers -platformbmg -rev-id B -has-emulated-64-bit-insts --igc-vectorizer -dce < %s 2>&1 | FileCheck %s --check-prefix=IR
; RUN: igc_opt -S --opaque-pointers -platformbmg -rev-id B -has-emulated-64-bit-insts --igc-vectorizer -dce -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s 2>&1 | FileCheck %s

; IR-LABEL: @bc_payload
; IR: [[SPLAT:%.*]] = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %s)
; IR: [[CAST:%.*]] = bitcast <8 x float> [[SPLAT]] to <8 x i32>
; IR: call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32({{.*}}, <8 x i32> [[CAST]])

; IR-LABEL: @bc_extract
; IR: [[SPLAT:%.*]] = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %s)
; IR: [[CAST:%.*]] = bitcast <8 x float> [[SPLAT]] to <8 x i32>
; IR: extractelement <8 x i32> [[CAST]], i32 5
; IR: call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32({{.*}}, <8 x i32> [[CAST]])

; CHECK-LABEL: .kernel "bc_payload"
; CHECK: mov (M1, 16) [[PAYLOAD:[A-Za-z0-9_]+]](7,0)<1>
; CHECK: lsc_store_block2d.ugm {{.*}} [[PAYLOAD]]:d32.16x8nn

; CHECK-LABEL: .kernel "bc_extract"
; CHECK-NOT: {{[[:space:]]s(_0v)?\(0,[1-9]}}
; CHECK: mov (M1, 16) [[PAYLOAD:[A-Za-z0-9_]+]](7,0)<1>
; CHECK-NOT: {{[[:space:]]s(_0v)?\(0,[1-9]}}
; CHECK: lsc_store_block2d.ugm {{.*}} [[PAYLOAD]]:d32.16x8nn
; CHECK-NOT: {{[[:space:]]s(_0v)?\(0,[1-9]}}
; CHECK: ret (M1, 1)

;==== bc_payload: igc_opt --igc-vectorizer -dce
; define spir_kernel void @bc_payload(float %a) #0 {
; entry:
;   %s = fadd float %a, 1.000000e+00
;   %vector_splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %s)
;   %vectorized_cast = bitcast <8 x float> %vector_splat to <8 x i32>
;   call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %vectorized_cast)
;   ret void
; }
;
;==== bc_payload: full vISA (DumpVISAASMToConsole, BMG SIMD16)
; .version 4.2
;
; .kernel "bc_payload"
;
; /// VISA Predefined Variables
; // .decl V0 v_type=G v_name=%null
; // .decl V1 v_type=G v_name=%thread_x
; // .decl V2 v_type=G v_name=%thread_y
; // .decl V3 v_type=G v_name=%group_id_x
; // .decl V4 v_type=G v_name=%group_id_y
; // .decl V5 v_type=G v_name=%group_id_z
; // .decl V6 v_type=G v_name=%tsc
; // .decl V7 v_type=G v_name=%r0
; // .decl V8 v_type=G v_name=%arg
; // .decl V9 v_type=G v_name=%retval
; // .decl V10 v_type=G v_name=%sp
; // .decl V11 v_type=G v_name=%fp
; // .decl V12 v_type=G v_name=%hw_id
; // .decl V13 v_type=G v_name=%sr0
; // .decl V14 v_type=G v_name=%cr0
; // .decl V15 v_type=G v_name=%ce0
; // .decl V16 v_type=G v_name=%dbg0
; // .decl V17 v_type=G v_name=%color
; // .decl V18 v_type=G v_name=%impl_arg_buf_ptr
; // .decl V19 v_type=G v_name=%local_id_buf_ptr
; // .decl V20 v_type=G v_name=%msg0
; // .decl V21 v_type=G v_name=%scratchloc
; // .decl V22 v_type=G v_name=%global_timer
; // .decl T0 v_type=T v_name=%slm
; // .decl T1 v_type=T v_name=T1
; // .decl T2 v_type=T v_name=T2
; // .decl T3 v_type=T v_name=TSS
; // .decl T4 v_type=T v_name=%bss
; // .decl T5 v_type=T v_name=%scratch
;
; .decl R0 v_type=G type=d num_elts=16 align=wordx32
; .decl R0_0v v_type=G type=d num_elts=16 align=wordx32 alias=<%r0, 0>
; .decl a v_type=G type=f num_elts=1 align=dword
; .decl s v_type=G type=f num_elts=1 align=dword
; .decl vector_splat v_type=G type=f num_elts=8 align=dword
; .decl vector_splat_0v v_type=G type=d num_elts=8 align=dword alias=<vector_splat, 0>
; .decl vector_splatBroadcast v_type=G type=d num_elts=128 align=wordx32
; .decl S0 v_type=S num_elts=1 v_name=S000
; .decl T6 v_type=T num_elts=1 v_name=T006
; .input a offset=0 size=4
; .kernel_attr Target="3d"
; .kernel_attr SimdSize=16
;
; .function "_main_0"
; _main_0:
;     add (M1_NM, 1) s(0,0)<1> a(0,0)<0;1,0> 0x3f800000:f                          /// $1
;     mov (M1_NM, 8) vector_splat(0,0)<1> s(0,0)<0;1,0>                            /// $2
;     mov (M1, 16) vector_splatBroadcast(0,0)<1> vector_splat_0v(0,0)<0;1,0>       /// $3
;     mov (M1, 16) vector_splatBroadcast(1,0)<1> vector_splat_0v(0,1)<0;1,0>       /// $4
;     mov (M1, 16) vector_splatBroadcast(2,0)<1> vector_splat_0v(0,2)<0;1,0>       /// $5
;     mov (M1, 16) vector_splatBroadcast(3,0)<1> vector_splat_0v(0,3)<0;1,0>       /// $6
;     mov (M1, 16) vector_splatBroadcast(4,0)<1> vector_splat_0v(0,4)<0;1,0>       /// $7
;     mov (M1, 16) vector_splatBroadcast(5,0)<1> vector_splat_0v(0,5)<0;1,0>       /// $8
;     mov (M1, 16) vector_splatBroadcast(6,0)<1> vector_splat_0v(0,6)<0;1,0>       /// $9
;     mov (M1, 16) vector_splatBroadcast(7,0)<1> vector_splat_0v(0,7)<0;1,0>       /// $10
;     lsc_store_block2d.ugm (M1, 1)  flat[0x0,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0x0,0x0]  vector_splatBroadcast:d32.16x8nn /// $11
;     ret (M1, 1)                                                                  /// $12
;
; //Platform: XE2
; //Build option: "-enableUnsafeCP_DF -wideMulMadOpsEn -enableCoalesceScalarMoves -supportLSCImmScale 0 -samplerHeaderWA -enablePreemptionR0Only -noStitchExternFunc -emitCrossThreadOffR0Reloc -abortOnSpill 4 -enableBundleCR 3 -freqBasedSpillCost 8 -freqBasedSpillCostFunc 1 -boundsChecking -nopresched -nodpsendreorder -PVCSendWARWA -dumpcommonisa -isaasmToConsole -printHexFloatInAsm -noverifyCISA -enableHalfLSC -partialInt64 -activeThreadsOnlyBarrier -GRFBumpUpNumber 2 "
;==== end of bc_payload dumps

define spir_kernel void @bc_payload(float %a) #0 {
entry:
  %s = fadd float %a, 1.000000e+00
  %c0 = bitcast float %s to i32
  %c1 = bitcast float %s to i32
  %c2 = bitcast float %s to i32
  %c3 = bitcast float %s to i32
  %c4 = bitcast float %s to i32
  %c5 = bitcast float %s to i32
  %c6 = bitcast float %s to i32
  %c7 = bitcast float %s to i32
  %v0 = insertelement <8 x i32> undef, i32 %c0, i64 0
  %v1 = insertelement <8 x i32> %v0, i32 %c1, i64 1
  %v2 = insertelement <8 x i32> %v1, i32 %c2, i64 2
  %v3 = insertelement <8 x i32> %v2, i32 %c3, i64 3
  %v4 = insertelement <8 x i32> %v3, i32 %c4, i64 4
  %v5 = insertelement <8 x i32> %v4, i32 %c5, i64 5
  %v6 = insertelement <8 x i32> %v5, i32 %c6, i64 6
  %v7 = insertelement <8 x i32> %v6, i32 %c7, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %v7)
  ret void
}

;==== bc_extract: igc_opt --igc-vectorizer -dce
; define spir_kernel void @bc_extract(float %a, ptr addrspace(1) %out) #0 {
; entry:
;   %s = fadd float %a, 1.000000e+00
;   %vector_splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %s)
;   %vectorized_cast = bitcast <8 x float> %vector_splat to <8 x i32>
;   %vector_extract5 = extractelement <8 x i32> %vectorized_cast, i32 5
;   call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %vectorized_cast)
;   %g = getelementptr i32, ptr addrspace(1) %out, i32 5
;   store i32 %vector_extract5, ptr addrspace(1) %g, align 4
;   ret void
; }
;
;==== bc_extract: full vISA (DumpVISAASMToConsole, BMG SIMD16)
; .version 4.2
;
; .kernel "bc_extract"
;
; /// VISA Predefined Variables
; // .decl V0 v_type=G v_name=%null
; // .decl V1 v_type=G v_name=%thread_x
; // .decl V2 v_type=G v_name=%thread_y
; // .decl V3 v_type=G v_name=%group_id_x
; // .decl V4 v_type=G v_name=%group_id_y
; // .decl V5 v_type=G v_name=%group_id_z
; // .decl V6 v_type=G v_name=%tsc
; // .decl V7 v_type=G v_name=%r0
; // .decl V8 v_type=G v_name=%arg
; // .decl V9 v_type=G v_name=%retval
; // .decl V10 v_type=G v_name=%sp
; // .decl V11 v_type=G v_name=%fp
; // .decl V12 v_type=G v_name=%hw_id
; // .decl V13 v_type=G v_name=%sr0
; // .decl V14 v_type=G v_name=%cr0
; // .decl V15 v_type=G v_name=%ce0
; // .decl V16 v_type=G v_name=%dbg0
; // .decl V17 v_type=G v_name=%color
; // .decl V18 v_type=G v_name=%impl_arg_buf_ptr
; // .decl V19 v_type=G v_name=%local_id_buf_ptr
; // .decl V20 v_type=G v_name=%msg0
; // .decl V21 v_type=G v_name=%scratchloc
; // .decl V22 v_type=G v_name=%global_timer
; // .decl T0 v_type=T v_name=%slm
; // .decl T1 v_type=T v_name=T1
; // .decl T2 v_type=T v_name=T2
; // .decl T3 v_type=T v_name=TSS
; // .decl T4 v_type=T v_name=%bss
; // .decl T5 v_type=T v_name=%scratch
;
; .decl R0 v_type=G type=d num_elts=16 align=wordx32
; .decl R0_0v v_type=G type=d num_elts=16 align=wordx32 alias=<%r0, 0>
; .decl a v_type=G type=f num_elts=1 align=dword
; .decl out v_type=G type=uq num_elts=1 align=qword
; .decl s v_type=G type=f num_elts=1 align=dword
; .decl vector_splat v_type=G type=f num_elts=8 align=dword
; .decl vector_splat_0v v_type=G type=d num_elts=8 align=dword alias=<vector_splat, 0>
; .decl vector_extract5 v_type=G type=d num_elts=1 align=wordx64
; .decl vector_splatBroadcast v_type=G type=d num_elts=128 align=wordx32
; .decl g v_type=G type=uq num_elts=1 align=wordx64
; .decl V0032 v_type=G type=uq num_elts=16 align=wordx32
; .decl V0033 v_type=G type=uq num_elts=16 align=wordx32
; .decl S0 v_type=S num_elts=1 v_name=S000
; .decl T6 v_type=T num_elts=1 v_name=T006
; .input out offset=0 size=8
; .input a offset=8 size=4
; .kernel_attr Target="3d"
; .kernel_attr SimdSize=16
;
; .function "_main_0"
; _main_0:
;     add (M1_NM, 1) s(0,0)<1> a(0,0)<0;1,0> 0x3f800000:f                          /// $1
;     mov (M1_NM, 8) vector_splat(0,0)<1> s(0,0)<0;1,0>                            /// $2
;     mov (M1_NM, 1) vector_extract5(0,0)<1> vector_splat_0v(0,5)<0;1,0>           /// $3
;     mov (M1, 16) vector_splatBroadcast(0,0)<1> vector_splat_0v(0,0)<0;1,0>       /// $4
;     mov (M1, 16) vector_splatBroadcast(1,0)<1> vector_splat_0v(0,1)<0;1,0>       /// $5
;     mov (M1, 16) vector_splatBroadcast(2,0)<1> vector_splat_0v(0,2)<0;1,0>       /// $6
;     mov (M1, 16) vector_splatBroadcast(3,0)<1> vector_splat_0v(0,3)<0;1,0>       /// $7
;     mov (M1, 16) vector_splatBroadcast(4,0)<1> vector_splat_0v(0,4)<0;1,0>       /// $8
;     mov (M1, 16) vector_splatBroadcast(5,0)<1> vector_splat_0v(0,5)<0;1,0>       /// $9
;     mov (M1, 16) vector_splatBroadcast(6,0)<1> vector_splat_0v(0,6)<0;1,0>       /// $10
;     mov (M1, 16) vector_splatBroadcast(7,0)<1> vector_splat_0v(0,7)<0;1,0>       /// $11
;     lsc_store_block2d.ugm (M1, 1)  flat[0x0,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0x0,0x0]  vector_splatBroadcast:d32.16x8nn /// $12
;     add (M1_NM, 1) V0033(0,0)<1> out(0,0)<0;1,0> 0x14:uq                         /// $13
;     mov (M1_NM, 1) g(0,0)<1> V0033(0,0)<0;1,0>                                   /// $14
;     lsc_store.ugm (M1_NM, 1)  flat[g]:a64  vector_extract5:d32t                  /// $15
;     ret (M1, 1)                                                                  /// $16
;
; //Platform: XE2
; //Build option: "-enableUnsafeCP_DF -wideMulMadOpsEn -enableCoalesceScalarMoves -supportLSCImmScale 0 -samplerHeaderWA -enablePreemptionR0Only -noStitchExternFunc -emitCrossThreadOffR0Reloc -abortOnSpill 4 -enableBundleCR 3 -freqBasedSpillCost 8 -freqBasedSpillCostFunc 1 -boundsChecking -nopresched -nodpsendreorder -PVCSendWARWA -dumpcommonisa -isaasmToConsole -printHexFloatInAsm -noverifyCISA -enableHalfLSC -partialInt64 -activeThreadsOnlyBarrier -GRFBumpUpNumber 2 "
;==== end of bc_extract dumps

define spir_kernel void @bc_extract(float %a, ptr addrspace(1) %out) #0 {
entry:
  %s = fadd float %a, 1.000000e+00
  %c0 = bitcast float %s to i32
  %c1 = bitcast float %s to i32
  %c2 = bitcast float %s to i32
  %c3 = bitcast float %s to i32
  %c4 = bitcast float %s to i32
  %c5 = bitcast float %s to i32
  %c6 = bitcast float %s to i32
  %c7 = bitcast float %s to i32
  %v0 = insertelement <8 x i32> undef, i32 %c0, i64 0
  %v1 = insertelement <8 x i32> %v0, i32 %c1, i64 1
  %v2 = insertelement <8 x i32> %v1, i32 %c2, i64 2
  %v3 = insertelement <8 x i32> %v2, i32 %c3, i64 3
  %v4 = insertelement <8 x i32> %v3, i32 %c4, i64 4
  %v5 = insertelement <8 x i32> %v4, i32 %c5, i64 5
  %v6 = insertelement <8 x i32> %v5, i32 %c6, i64 6
  %v7 = insertelement <8 x i32> %v6, i32 %c7, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %v7)
  %g = getelementptr i32, ptr addrspace(1) %out, i32 5
  store i32 %c5, ptr addrspace(1) %g
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

attributes #0 = { convergent }

!igc.functions = !{!0, !3}

!0 = !{ptr @bc_payload, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @bc_extract, !1}

!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @bc_payload}
!7 = !{!"FuncMDValue[1]", !4, !11}
!8 = !{!"FuncMDMap[1]", ptr @bc_extract}
!9 = !{!"FuncMD", !6, !5, !8, !7}
!10 = !{!"ModuleMD", !9}
!IGCMetadata = !{!10}
!11 = !{!"resAllocMD", !12}
!12 = !{!"argAllocMDList", !16, !17}
!13 = !{!"type", i32 0}
!14 = !{!"extensionType", i32 -1}
!15 = !{!"indexType", i32 -1}
!16 = !{!"argAllocMDListVec[0]", !13, !14, !15}
!17 = !{!"argAllocMDListVec[1]", !13, !14, !15}
