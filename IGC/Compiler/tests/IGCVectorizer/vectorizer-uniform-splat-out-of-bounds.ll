;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers -platformbmg -rev-id B -has-emulated-64-bit-insts --igc-vectorizer --igc-vector-coalescer -dce --igc-legalization -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s 2>&1 | FileCheck %s

; CHECK-LABEL: .kernel "splat_coalesced_maxnum"
; CHECK-NOT: {{[[:space:]]s[ab]\(0,[1-9]}}
; CHECK: max (M1_NM, 16) coalesced_intrinsic(0,0)<1>
; CHECK-NOT: {{[[:space:]]s[ab]\(0,[1-9]}}

; CHECK-LABEL: .kernel "splat_fptrunc"
; CHECK-NOT: {{[[:space:]]s\(0,[1-9]}}
; CHECK: mov (M1_NM, 8) vector_splat(0,0)<1> s(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) vectorized_cast(0,7)<1> vector_splat(0,7)<0;1,0>
; CHECK-NOT: {{[[:space:]]s\(0,[1-9]}}
; CHECK: ret (M1, 1)

;==== splat_coalesced_maxnum: igc_opt --igc-vectorizer --igc-vector-coalescer -dce
; define spir_kernel void @splat_coalesced_maxnum(float %a, float %b, i32 %n) #0 {
; entry:
;   %sa = fadd float %a, 1.000000e+00
;   %vector_splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %sa)
;   %sb = fadd float %b, 2.000000e+00
;   %vector_splat40 = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %sb)
;   br label %loop
;
; loop:                                             ; preds = %loop, %entry
;   %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
;   %vectorized_phi = phi <8 x float> [ <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float 6.000000e+00, float 7.000000e+00, float 8.000000e+00>, %entry ], [ %coalesced_output51, %loop ]
;   %vectorized_phi31 = phi <8 x float> [ <float 9.000000e+00, float 1.000000e+01, float 1.100000e+01, float 1.200000e+01, float 1.300000e+01, float 1.400000e+01, float 1.500000e+01, float 1.600000e+01>, %entry ], [ %coalesced_output, %loop ]
;   %coalesced_input = shufflevector <8 x float> %vectorized_phi31, <8 x float> %vectorized_phi, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
;   %coalesced_input50 = shufflevector <8 x float> %vector_splat40, <8 x float> %vector_splat, <16 x i32> <i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
;   %coalesced_intrinsic = call <16 x float> @llvm.maxnum.v16f32(<16 x float> %coalesced_input, <16 x float> %coalesced_input50)
;   %coalesced_output = shufflevector <16 x float> %coalesced_intrinsic, <16 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
;   %coalesced_output51 = shufflevector <16 x float> %coalesced_intrinsic, <16 x float> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
;   %i.next = add i32 %i, 1
;   %exitc = icmp sge i32 %i.next, %n
;   br i1 %exitc, label %exit, label %loop
;
; exit:                                             ; preds = %loop
;   %mbc = bitcast <8 x float> %coalesced_output51 to <8 x i32>
;   call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %mbc)
;   %nbc = bitcast <8 x float> %coalesced_output to <8 x i32>
;   call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %nbc)
;   ret void
; }
;
;==== splat_coalesced_maxnum: full vISA (DumpVISAASMToConsole, BMG SIMD16)
; .version 4.2
;
; .kernel "splat_coalesced_maxnum"
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
; .decl b v_type=G type=f num_elts=1 align=dword
; .decl n v_type=G type=d num_elts=1 align=dword
; .decl vectorized_phi v_type=G type=f num_elts=8 align=dword
; .decl vectorized_phi31 v_type=G type=f num_elts=8 align=dword
; .decl sa v_type=G type=f num_elts=1 align=dword
; .decl vector_splat v_type=G type=f num_elts=8 align=dword
; .decl sb v_type=G type=f num_elts=1 align=dword
; .decl vector_splat40 v_type=G type=f num_elts=8 align=dword
; .decl i v_type=G type=d num_elts=1 align=dword
; .decl V0032 v_type=G type=f num_elts=8 align=wordx32
; .decl V0033 v_type=G type=f num_elts=8 align=wordx32
; .decl V0034 v_type=G type=f num_elts=16 align=dword
; .decl V0035 v_type=G type=f num_elts=16 align=dword
; .decl coalesced_intrinsic v_type=G type=f num_elts=16 align=dword
; .decl vectorized_phi_0v v_type=G type=d num_elts=8 align=dword alias=<vectorized_phi, 0>
; .decl vectorized_phiBroadcast v_type=G type=d num_elts=128 align=wordx32
; .decl vectorized_phi31_0v v_type=G type=d num_elts=8 align=dword alias=<vectorized_phi31, 0>
; .decl vectorized_phi31Broadcast v_type=G type=d num_elts=128 align=wordx32
; .decl P1 v_type=P num_elts=16
; .decl S0 v_type=S num_elts=1 v_name=S000
; .decl T6 v_type=T num_elts=1 v_name=T006
; .input a offset=0 size=4
; .input b offset=4 size=4
; .input n offset=8 size=4
; .kernel_attr Target="3d"
; .kernel_attr SimdSize=16
;
; .function "_main_0"
; _main_0:
;     add (M1_NM, 1) sa(0,0)<1> a(0,0)<0;1,0> 0x3f800000:f                         /// $1
;     mov (M1_NM, 8) vector_splat(0,0)<1> sa(0,0)<0;1,0>                           /// $2
;     add (M1_NM, 1) sb(0,0)<1> b(0,0)<0;1,0> 0x40000000:f                         /// $3
;     mov (M1_NM, 8) vector_splat40(0,0)<1> sb(0,0)<0;1,0>                         /// $4
;     mov (M1_NM, 1) V0032(0,0)<1> 0x3f800000:f                                    /// $5
;     mov (M1_NM, 1) V0032(0,1)<1> 0x40000000:f                                    /// $6
;     mov (M1_NM, 1) V0032(0,2)<1> 0x40400000:f                                    /// $7
;     mov (M1_NM, 1) V0032(0,3)<1> 0x40800000:f                                    /// $8
;     mov (M1_NM, 1) V0032(0,4)<1> 0x40a00000:f                                    /// $9
;     mov (M1_NM, 1) V0032(0,5)<1> 0x40c00000:f                                    /// $10
;     mov (M1_NM, 1) V0032(0,6)<1> 0x40e00000:f                                    /// $11
;     mov (M1_NM, 1) V0032(0,7)<1> 0x41000000:f                                    /// $12
;     mov (M1_NM, 1) V0033(0,0)<1> 0x41100000:f                                    /// $13
;     mov (M1_NM, 1) V0033(0,1)<1> 0x41200000:f                                    /// $14
;     mov (M1_NM, 1) V0033(0,2)<1> 0x41300000:f                                    /// $15
;     mov (M1_NM, 1) V0033(0,3)<1> 0x41400000:f                                    /// $16
;     mov (M1_NM, 1) V0033(0,4)<1> 0x41500000:f                                    /// $17
;     mov (M1_NM, 1) V0033(0,5)<1> 0x41600000:f                                    /// $18
;     mov (M1_NM, 1) V0033(0,6)<1> 0x41700000:f                                    /// $19
;     mov (M1_NM, 1) V0033(0,7)<1> 0x41800000:f                                    /// $20
;     mov (M1_NM, 1) i(0,0)<1> 0x0:d                                               /// $21
;     mov (M1_NM, 8) vectorized_phi(0,0)<1> V0032(0,0)<1;1,0>                      /// $22
;     mov (M1_NM, 8) vectorized_phi31(0,0)<1> V0033(0,0)<1;1,0>                    /// $23
;
; _splat_coalesced_maxnum_001_loop:
;     mov (M1_NM, 1) V0034(0,0)<1> vectorized_phi31(0,0)<0;1,0>                    /// $25
;     mov (M1_NM, 1) V0034(0,1)<1> vectorized_phi31(0,1)<0;1,0>                    /// $26
;     mov (M1_NM, 1) V0034(0,2)<1> vectorized_phi31(0,2)<0;1,0>                    /// $27
;     mov (M1_NM, 1) V0034(0,3)<1> vectorized_phi31(0,3)<0;1,0>                    /// $28
;     mov (M1_NM, 1) V0034(0,4)<1> vectorized_phi31(0,4)<0;1,0>                    /// $29
;     mov (M1_NM, 1) V0034(0,5)<1> vectorized_phi31(0,5)<0;1,0>                    /// $30
;     mov (M1_NM, 1) V0034(0,6)<1> vectorized_phi31(0,6)<0;1,0>                    /// $31
;     mov (M1_NM, 1) V0034(0,7)<1> vectorized_phi31(0,7)<0;1,0>                    /// $32
;     mov (M1_NM, 1) V0034(0,8)<1> vectorized_phi(0,0)<0;1,0>                      /// $33
;     mov (M1_NM, 1) V0034(0,9)<1> vectorized_phi(0,1)<0;1,0>                      /// $34
;     mov (M1_NM, 1) V0034(0,10)<1> vectorized_phi(0,2)<0;1,0>                     /// $35
;     mov (M1_NM, 1) V0034(0,11)<1> vectorized_phi(0,3)<0;1,0>                     /// $36
;     mov (M1_NM, 1) V0034(0,12)<1> vectorized_phi(0,4)<0;1,0>                     /// $37
;     mov (M1_NM, 1) V0034(0,13)<1> vectorized_phi(0,5)<0;1,0>                     /// $38
;     mov (M1_NM, 1) V0034(0,14)<1> vectorized_phi(0,6)<0;1,0>                     /// $39
;     mov (M1_NM, 1) V0034(0,15)<1> vectorized_phi(0,7)<0;1,0>                     /// $40
;     mov (M1_NM, 1) V0035(0,0)<1> vector_splat40(0,0)<0;1,0>                      /// $41
;     mov (M1_NM, 1) V0035(0,1)<1> vector_splat40(0,0)<0;1,0>                      /// $42
;     mov (M1_NM, 1) V0035(0,2)<1> vector_splat40(0,0)<0;1,0>                      /// $43
;     mov (M1_NM, 1) V0035(0,3)<1> vector_splat40(0,0)<0;1,0>                      /// $44
;     mov (M1_NM, 1) V0035(0,4)<1> vector_splat40(0,0)<0;1,0>                      /// $45
;     mov (M1_NM, 1) V0035(0,5)<1> vector_splat40(0,0)<0;1,0>                      /// $46
;     mov (M1_NM, 1) V0035(0,6)<1> vector_splat40(0,0)<0;1,0>                      /// $47
;     mov (M1_NM, 1) V0035(0,7)<1> vector_splat40(0,0)<0;1,0>                      /// $48
;     mov (M1_NM, 1) V0035(0,8)<1> vector_splat(0,0)<0;1,0>                        /// $49
;     mov (M1_NM, 1) V0035(0,9)<1> vector_splat(0,0)<0;1,0>                        /// $50
;     mov (M1_NM, 1) V0035(0,10)<1> vector_splat(0,0)<0;1,0>                       /// $51
;     mov (M1_NM, 1) V0035(0,11)<1> vector_splat(0,0)<0;1,0>                       /// $52
;     mov (M1_NM, 1) V0035(0,12)<1> vector_splat(0,0)<0;1,0>                       /// $53
;     mov (M1_NM, 1) V0035(0,13)<1> vector_splat(0,0)<0;1,0>                       /// $54
;     mov (M1_NM, 1) V0035(0,14)<1> vector_splat(0,0)<0;1,0>                       /// $55
;     mov (M1_NM, 1) V0035(0,15)<1> vector_splat(0,0)<0;1,0>                       /// $56
;     max (M1_NM, 16) coalesced_intrinsic(0,0)<1> V0034(0,0)<1;1,0> V0035(0,0)<1;1,0> /// $57
;     mov (M1_NM, 1) vectorized_phi31(0,0)<1> coalesced_intrinsic(0,0)<0;1,0>      /// $58
;     mov (M1_NM, 1) vectorized_phi31(0,1)<1> coalesced_intrinsic(0,1)<0;1,0>      /// $59
;     mov (M1_NM, 1) vectorized_phi31(0,2)<1> coalesced_intrinsic(0,2)<0;1,0>      /// $60
;     mov (M1_NM, 1) vectorized_phi31(0,3)<1> coalesced_intrinsic(0,3)<0;1,0>      /// $61
;     mov (M1_NM, 1) vectorized_phi31(0,4)<1> coalesced_intrinsic(0,4)<0;1,0>      /// $62
;     mov (M1_NM, 1) vectorized_phi31(0,5)<1> coalesced_intrinsic(0,5)<0;1,0>      /// $63
;     mov (M1_NM, 1) vectorized_phi31(0,6)<1> coalesced_intrinsic(0,6)<0;1,0>      /// $64
;     mov (M1_NM, 1) vectorized_phi31(0,7)<1> coalesced_intrinsic(0,7)<0;1,0>      /// $65
;     mov (M1_NM, 1) vectorized_phi(0,0)<1> coalesced_intrinsic(0,8)<0;1,0>        /// $66
;     mov (M1_NM, 1) vectorized_phi(0,1)<1> coalesced_intrinsic(0,9)<0;1,0>        /// $67
;     mov (M1_NM, 1) vectorized_phi(0,2)<1> coalesced_intrinsic(0,10)<0;1,0>       /// $68
;     mov (M1_NM, 1) vectorized_phi(0,3)<1> coalesced_intrinsic(0,11)<0;1,0>       /// $69
;     mov (M1_NM, 1) vectorized_phi(0,4)<1> coalesced_intrinsic(0,12)<0;1,0>       /// $70
;     mov (M1_NM, 1) vectorized_phi(0,5)<1> coalesced_intrinsic(0,13)<0;1,0>       /// $71
;     mov (M1_NM, 1) vectorized_phi(0,6)<1> coalesced_intrinsic(0,14)<0;1,0>       /// $72
;     mov (M1_NM, 1) vectorized_phi(0,7)<1> coalesced_intrinsic(0,15)<0;1,0>       /// $73
;     add (M1_NM, 1) i(0,0)<1> i(0,0)<0;1,0> 0x1:w                                 /// $74
;     cmp.ge (M1_NM, 16) P1 i(0,0)<0;1,0> n(0,0)<0;1,0>                            /// $75
;     (!P1) goto (M1, 1) _splat_coalesced_maxnum_001_loop                          /// $76
;
; _splat_coalesced_maxnum_002_exit:
;     mov (M1, 16) vectorized_phiBroadcast(0,0)<1> vectorized_phi_0v(0,0)<0;1,0>   /// $78
;     mov (M1, 16) vectorized_phiBroadcast(1,0)<1> vectorized_phi_0v(0,1)<0;1,0>   /// $79
;     mov (M1, 16) vectorized_phiBroadcast(2,0)<1> vectorized_phi_0v(0,2)<0;1,0>   /// $80
;     mov (M1, 16) vectorized_phiBroadcast(3,0)<1> vectorized_phi_0v(0,3)<0;1,0>   /// $81
;     mov (M1, 16) vectorized_phiBroadcast(4,0)<1> vectorized_phi_0v(0,4)<0;1,0>   /// $82
;     mov (M1, 16) vectorized_phiBroadcast(5,0)<1> vectorized_phi_0v(0,5)<0;1,0>   /// $83
;     mov (M1, 16) vectorized_phiBroadcast(6,0)<1> vectorized_phi_0v(0,6)<0;1,0>   /// $84
;     mov (M1, 16) vectorized_phiBroadcast(7,0)<1> vectorized_phi_0v(0,7)<0;1,0>   /// $85
;     lsc_store_block2d.ugm (M1, 1)  flat[0x0,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0x0,0x0]  vectorized_phiBroadcast:d32.16x8nn /// $86
;     mov (M1, 16) vectorized_phi31Broadcast(0,0)<1> vectorized_phi31_0v(0,0)<0;1,0> /// $87
;     mov (M1, 16) vectorized_phi31Broadcast(1,0)<1> vectorized_phi31_0v(0,1)<0;1,0> /// $88
;     mov (M1, 16) vectorized_phi31Broadcast(2,0)<1> vectorized_phi31_0v(0,2)<0;1,0> /// $89
;     mov (M1, 16) vectorized_phi31Broadcast(3,0)<1> vectorized_phi31_0v(0,3)<0;1,0> /// $90
;     mov (M1, 16) vectorized_phi31Broadcast(4,0)<1> vectorized_phi31_0v(0,4)<0;1,0> /// $91
;     mov (M1, 16) vectorized_phi31Broadcast(5,0)<1> vectorized_phi31_0v(0,5)<0;1,0> /// $92
;     mov (M1, 16) vectorized_phi31Broadcast(6,0)<1> vectorized_phi31_0v(0,6)<0;1,0> /// $93
;     mov (M1, 16) vectorized_phi31Broadcast(7,0)<1> vectorized_phi31_0v(0,7)<0;1,0> /// $94
;     lsc_store_block2d.ugm (M1, 1)  flat[0x0,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0x0,0x0]  vectorized_phi31Broadcast:d32.16x8nn /// $95
;     ret (M1, 1)                                                                  /// $96
;
; //Platform: XE2
; //Build option: "-enableUnsafeCP_DF -wideMulMadOpsEn -enableCoalesceScalarMoves -supportLSCImmScale 0 -samplerHeaderWA -enablePreemptionR0Only -noStitchExternFunc -emitCrossThreadOffR0Reloc -abortOnSpill 4 -enableBundleCR 3 -freqBasedSpillCost 8 -freqBasedSpillCostFunc 1 -boundsChecking -nopresched -nodpsendreorder -PVCSendWARWA -dumpcommonisa -isaasmToConsole -printHexFloatInAsm -noverifyCISA -enableHalfLSC -partialInt64 -activeThreadsOnlyBarrier -GRFBumpUpNumber 2 "
;==== end of splat_coalesced_maxnum dumps

define spir_kernel void @splat_coalesced_maxnum(float %a, float %b, i32 %n) #0 {
entry:
  %sa = fadd float %a, 1.000000e+00
  %sb = fadd float %b, 2.000000e+00
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %p0 = phi float [ 1.000000e+00, %entry ], [ %m0, %loop ]
  %p1 = phi float [ 2.000000e+00, %entry ], [ %m1, %loop ]
  %p2 = phi float [ 3.000000e+00, %entry ], [ %m2, %loop ]
  %p3 = phi float [ 4.000000e+00, %entry ], [ %m3, %loop ]
  %p4 = phi float [ 5.000000e+00, %entry ], [ %m4, %loop ]
  %p5 = phi float [ 6.000000e+00, %entry ], [ %m5, %loop ]
  %p6 = phi float [ 7.000000e+00, %entry ], [ %m6, %loop ]
  %p7 = phi float [ 8.000000e+00, %entry ], [ %m7, %loop ]
  %q0 = phi float [ 9.000000e+00, %entry ], [ %n0, %loop ]
  %q1 = phi float [ 1.000000e+01, %entry ], [ %n1, %loop ]
  %q2 = phi float [ 1.100000e+01, %entry ], [ %n2, %loop ]
  %q3 = phi float [ 1.200000e+01, %entry ], [ %n3, %loop ]
  %q4 = phi float [ 1.300000e+01, %entry ], [ %n4, %loop ]
  %q5 = phi float [ 1.400000e+01, %entry ], [ %n5, %loop ]
  %q6 = phi float [ 1.500000e+01, %entry ], [ %n6, %loop ]
  %q7 = phi float [ 1.600000e+01, %entry ], [ %n7, %loop ]
  %m0 = call float @llvm.maxnum.f32(float %p0, float %sa)
  %m1 = call float @llvm.maxnum.f32(float %p1, float %sa)
  %m2 = call float @llvm.maxnum.f32(float %p2, float %sa)
  %m3 = call float @llvm.maxnum.f32(float %p3, float %sa)
  %m4 = call float @llvm.maxnum.f32(float %p4, float %sa)
  %m5 = call float @llvm.maxnum.f32(float %p5, float %sa)
  %m6 = call float @llvm.maxnum.f32(float %p6, float %sa)
  %m7 = call float @llvm.maxnum.f32(float %p7, float %sa)
  %n0 = call float @llvm.maxnum.f32(float %q0, float %sb)
  %n1 = call float @llvm.maxnum.f32(float %q1, float %sb)
  %n2 = call float @llvm.maxnum.f32(float %q2, float %sb)
  %n3 = call float @llvm.maxnum.f32(float %q3, float %sb)
  %n4 = call float @llvm.maxnum.f32(float %q4, float %sb)
  %n5 = call float @llvm.maxnum.f32(float %q5, float %sb)
  %n6 = call float @llvm.maxnum.f32(float %q6, float %sb)
  %n7 = call float @llvm.maxnum.f32(float %q7, float %sb)
  %i.next = add i32 %i, 1
  %exitc = icmp sge i32 %i.next, %n
  br i1 %exitc, label %exit, label %loop
exit:
  %mv0 = insertelement <8 x float> undef, float %m0, i64 0
  %mv1 = insertelement <8 x float> %mv0, float %m1, i64 1
  %mv2 = insertelement <8 x float> %mv1, float %m2, i64 2
  %mv3 = insertelement <8 x float> %mv2, float %m3, i64 3
  %mv4 = insertelement <8 x float> %mv3, float %m4, i64 4
  %mv5 = insertelement <8 x float> %mv4, float %m5, i64 5
  %mv6 = insertelement <8 x float> %mv5, float %m6, i64 6
  %mv7 = insertelement <8 x float> %mv6, float %m7, i64 7
  %mbc = bitcast <8 x float> %mv7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %mbc)
  %nv0 = insertelement <8 x float> undef, float %n0, i64 0
  %nv1 = insertelement <8 x float> %nv0, float %n1, i64 1
  %nv2 = insertelement <8 x float> %nv1, float %n2, i64 2
  %nv3 = insertelement <8 x float> %nv2, float %n3, i64 3
  %nv4 = insertelement <8 x float> %nv3, float %n4, i64 4
  %nv5 = insertelement <8 x float> %nv4, float %n5, i64 5
  %nv6 = insertelement <8 x float> %nv5, float %n6, i64 6
  %nv7 = insertelement <8 x float> %nv6, float %n7, i64 7
  %nbc = bitcast <8 x float> %nv7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %nbc)
  ret void
}

;==== splat_fptrunc: igc_opt --igc-vectorizer --igc-vector-coalescer -dce
; define spir_kernel void @splat_fptrunc(float %a) #0 {
; entry:
;   %s = fadd float %a, 1.000000e+00
;   %vector_splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %s)
;   %vectorized_cast = fptrunc <8 x float> %vector_splat to <8 x half>
;   %bc = bitcast <8 x half> %vectorized_cast to <4 x i32>
;   call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <4 x i32> %bc)
;   ret void
; }
;
;==== splat_fptrunc: full vISA (DumpVISAASMToConsole, BMG SIMD16)
; .version 4.2
;
; .kernel "splat_fptrunc"
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
; .decl vectorized_cast v_type=G type=hf num_elts=8 align=word
; .decl bc v_type=G type=d num_elts=4 align=dword
; .decl bc_0v v_type=G type=hf num_elts=8 align=dword alias=<bc, 0>
; .decl bcBroadcast v_type=G type=d num_elts=64 align=wordx32
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
;     mov (M1_NM, 1) vectorized_cast(0,0)<1> vector_splat(0,0)<0;1,0>              /// $3
;     mov (M1_NM, 1) vectorized_cast(0,1)<1> vector_splat(0,1)<0;1,0>              /// $4
;     mov (M1_NM, 1) vectorized_cast(0,2)<1> vector_splat(0,2)<0;1,0>              /// $5
;     mov (M1_NM, 1) vectorized_cast(0,3)<1> vector_splat(0,3)<0;1,0>              /// $6
;     mov (M1_NM, 1) vectorized_cast(0,4)<1> vector_splat(0,4)<0;1,0>              /// $7
;     mov (M1_NM, 1) vectorized_cast(0,5)<1> vector_splat(0,5)<0;1,0>              /// $8
;     mov (M1_NM, 1) vectorized_cast(0,6)<1> vector_splat(0,6)<0;1,0>              /// $9
;     mov (M1_NM, 1) vectorized_cast(0,7)<1> vector_splat(0,7)<0;1,0>              /// $10
;     mov (M1_NM, 1) bc_0v(0,0)<1> vectorized_cast(0,0)<0;1,0>                     /// $11
;     mov (M1_NM, 1) bc_0v(0,1)<1> vectorized_cast(0,1)<0;1,0>                     /// $12
;     mov (M1_NM, 1) bc_0v(0,2)<1> vectorized_cast(0,2)<0;1,0>                     /// $13
;     mov (M1_NM, 1) bc_0v(0,3)<1> vectorized_cast(0,3)<0;1,0>                     /// $14
;     mov (M1_NM, 1) bc_0v(0,4)<1> vectorized_cast(0,4)<0;1,0>                     /// $15
;     mov (M1_NM, 1) bc_0v(0,5)<1> vectorized_cast(0,5)<0;1,0>                     /// $16
;     mov (M1_NM, 1) bc_0v(0,6)<1> vectorized_cast(0,6)<0;1,0>                     /// $17
;     mov (M1_NM, 1) bc_0v(0,7)<1> vectorized_cast(0,7)<0;1,0>                     /// $18
;     mov (M1, 16) bcBroadcast(0,0)<1> bc(0,0)<0;1,0>                              /// $19
;     mov (M1, 16) bcBroadcast(1,0)<1> bc(0,1)<0;1,0>                              /// $20
;     mov (M1, 16) bcBroadcast(2,0)<1> bc(0,2)<0;1,0>                              /// $21
;     mov (M1, 16) bcBroadcast(3,0)<1> bc(0,3)<0;1,0>                              /// $22
;     lsc_store_block2d.ugm (M1, 1)  flat[0x0,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0xFFFFFFFFFFFFFFFF,0x0,0x0]  bcBroadcast:d16.16x8nn /// $23
;     ret (M1, 1)                                                                  /// $24
;
; //Platform: XE2
; //Build option: "-enableUnsafeCP_DF -wideMulMadOpsEn -enableCoalesceScalarMoves -supportLSCImmScale 0 -samplerHeaderWA -enablePreemptionR0Only -noStitchExternFunc -emitCrossThreadOffR0Reloc -abortOnSpill 4 -enableBundleCR 3 -freqBasedSpillCost 8 -freqBasedSpillCostFunc 1 -boundsChecking -nopresched -nodpsendreorder -PVCSendWARWA -dumpcommonisa -isaasmToConsole -printHexFloatInAsm -noverifyCISA -enableHalfLSC -partialInt64 -activeThreadsOnlyBarrier -GRFBumpUpNumber 2 "
;==== end of splat_fptrunc dumps

define spir_kernel void @splat_fptrunc(float %a) #0 {
entry:
  %s = fadd float %a, 1.000000e+00
  %t0 = fptrunc float %s to half
  %t1 = fptrunc float %s to half
  %t2 = fptrunc float %s to half
  %t3 = fptrunc float %s to half
  %t4 = fptrunc float %s to half
  %t5 = fptrunc float %s to half
  %t6 = fptrunc float %s to half
  %t7 = fptrunc float %s to half
  %v0 = insertelement <8 x half> undef, half %t0, i64 0
  %v1 = insertelement <8 x half> %v0, half %t1, i64 1
  %v2 = insertelement <8 x half> %v1, half %t2, i64 2
  %v3 = insertelement <8 x half> %v2, half %t3, i64 3
  %v4 = insertelement <8 x half> %v3, half %t4, i64 4
  %v5 = insertelement <8 x half> %v4, half %t5, i64 5
  %v6 = insertelement <8 x half> %v5, half %t6, i64 6
  %v7 = insertelement <8 x half> %v6, half %t7, i64 7
  %bc = bitcast <8 x half> %v7 to <4 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <4 x i32> %bc)
  ret void
}
declare float @llvm.maxnum.f32(float, float)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v4i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <4 x i32>)

attributes #0 = { convergent }

!igc.functions = !{!0, !3}

!0 = !{ptr @splat_coalesced_maxnum, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @splat_fptrunc, !1}

!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @splat_coalesced_maxnum}
!7 = !{!"FuncMDValue[1]", !4}
!8 = !{!"FuncMDMap[1]", ptr @splat_fptrunc}
!9 = !{!"FuncMD", !6, !5, !8, !7}
!10 = !{!"ModuleMD", !9}
!IGCMetadata = !{!10}
