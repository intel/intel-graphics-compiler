;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -simdcf-region -enable-simdcf-transform -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; This test checks that GenXSimdCFRegion generate
; correct double if-then simd regions llvm-ir (based on Bif)

; Function Attrs: noinline nounwind
; CHECK: double_if_then_test
define spir_func <2 x double> @double_if_then_test(<2 x double> %a, <2 x double> %b) local_unnamed_addr #0 {
; 1-st Region
entry:
  %_0 = bitcast <2 x double> %b to <4 x i32>
  %_1 = bitcast <2 x double> %a to <4 x i32>
  %_35 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_1, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_36 = lshr <2 x i32> %_35, <i32 20, i32 20>
  %_37 = trunc <2 x i32> %_36 to <2 x i16>
  %_38 = and <2 x i16> %_37, <i16 2047, i16 2047>
  %_39 = zext <2 x i16> %_38 to <2 x i32>
  %_40 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_0, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_41 = lshr <2 x i32> %_40, <i32 20, i32 20>
  %_42 = trunc <2 x i32> %_41 to <2 x i16>
  %_43 = and <2 x i16> %_42, <i16 2047, i16 2047>
  %_44 = zext <2 x i16> %_43 to <2 x i32>
  %_45 = add nsw <2 x i32> %_39, <i32 -127, i32 -127>
  %_46 = icmp ugt <2 x i32> %_45, <i32 1791, i32 1791>
  %_47 = add nsw <2 x i32> %_44, <i32 -897, i32 -897>
  %_48 = icmp ugt <2 x i32> %_47, <i32 251, i32 251>
  %_49 = or <2 x i1> %_48, %_46
  %_50 = sext <2 x i1> %_49 to <2 x i8>
  %_51 = tail call i1 @llvm.genx.any.v2i8(<2 x i8> %_50) #3
  br i1 %_51, label %if.then.i, label %if.end163.i

; 1-st Region
if.then.i:                                        ; preds = %entry
  %_52 = icmp eq <2 x i32> %_39, zeroinitializer
  %_53 = sext <2 x i1> %_52 to <2 x i8>
  %_54 = tail call i1 @llvm.genx.any.v2i8(<2 x i8> %_53) #3
  br i1 %_54, label %if.then25.i, label %if.end.i

; 1-st Region
if.then25.i:                                      ; preds = %if.then.i
  %_55 = fmul <2 x double> %a, <double 0x43F0000000000000, double 0x43F0000000000000>
  %_56 = select <2 x i1> %_52, <2 x double> %_55, <2 x double> %a
  %_2 = bitcast <2 x double> %_56 to <4 x i32>
  %_57 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_2, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_58 = lshr <2 x i32> %_57, <i32 20, i32 20>
  %_59 = trunc <2 x i32> %_58 to <2 x i16>
  %_60 = and <2 x i16> %_59, <i16 2047, i16 2047>
  %_61 = zext <2 x i16> %_60 to <2 x i32>
  %_62 = icmp ne <2 x i32> %_61, zeroinitializer
  %_63 = and <2 x i1> %_62, %_52
  %_64 = select <2 x i1> %_63, <2 x double> <double 0x3BE0000000000000, double 0x3BE0000000000000>, <2 x double> <double 5.000000e-01, double 5.000000e-01>
  %_3 = bitcast <2 x double> %_64 to <4 x i32>
  br label %if.end.i

; 1-st Region
if.end.i:                                         ; preds = %if.then25.i, %if.then.i
  %_4 = phi <2 x double> [ %_56, %if.then25.i ], [ %a, %if.then.i ]
  %_5 = phi <4 x i32> [ %_2, %if.then25.i ], [ %_1, %if.then.i ]
  %_65 = phi <4 x i32> [ %_3, %if.then25.i ], [ <i32 0, i32 1071644672, i32 0, i32 1071644672>, %if.then.i ]
  %_6 = phi <2 x i32> [ %_61, %if.then25.i ], [ %_39, %if.then.i ]
  %_66 = icmp eq <2 x i32> %_44, zeroinitializer
  %_67 = sext <2 x i1> %_66 to <2 x i8>
  %_68 = tail call i1 @llvm.genx.any.v2i8(<2 x i8> %_67) #3
  br i1 %_68, label %if.then52.i, label %if.end78.i

; 1-st Region
if.then52.i:                                      ; preds = %if.end.i
  %_69 = fmul <2 x double> %b, <double 0x43F0000000000000, double 0x43F0000000000000>
  %_70 = select <2 x i1> %_66, <2 x double> %_69, <2 x double> %b
  %_7 = bitcast <2 x double> %_70 to <4 x i32>
  %_71 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_7, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_72 = lshr <2 x i32> %_71, <i32 20, i32 20>
  %_73 = trunc <2 x i32> %_72 to <2 x i16>
  %_74 = and <2 x i16> %_73, <i16 2047, i16 2047>
  %_75 = zext <2 x i16> %_74 to <2 x i32>
  %_8 = bitcast <4 x i32> %_65 to <2 x double>
  %_76 = fmul <2 x double> %_8, <double 0x43F0000000000000, double 0x43F0000000000000>
  %_77 = icmp ne <2 x i32> %_75, zeroinitializer
  %_78 = and <2 x i1> %_77, %_66
  %_79 = select <2 x i1> %_78, <2 x double> %_76, <2 x double> %_8
  %_9 = bitcast <2 x double> %_79 to <4 x i32>
  br label %if.end78.i

; 1-st Region
if.end78.i:                                       ; preds = %if.then52.i, %if.end.i
  %_80.phi = phi <2 x i32> [ %_71, %if.then52.i ], [ %_40, %if.end.i ]
  %_10 = phi <2 x double> [ %_70, %if.then52.i ], [ %b, %if.end.i ]
  %_11 = phi <4 x i32> [ %_7, %if.then52.i ], [ %_0, %if.end.i ]
  %_81 = phi <4 x i32> [ %_9, %if.then52.i ], [ %_65, %if.end.i ]
  %_12 = phi <2 x i32> [ %_75, %if.then52.i ], [ %_44, %if.end.i ]
  %_82 = sub nsw <2 x i32> %_6, %_12
  %_83 = add nsw <2 x i32> %_82, <i32 2047, i32 2047>
  %_84 = ashr <2 x i32> %_83, <i32 1, i32 1>
  %_85 = sub nsw <2 x i32> %_83, %_84
  %_86 = shl <2 x i32> %_84, <i32 20, i32 20>
  %_87 = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> zeroinitializer, <2 x i32> %_86, i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true) #3
  %_88 = shl <2 x i32> %_85, <i32 20, i32 20>
  %_89 = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> zeroinitializer, <2 x i32> %_88, i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true) #3
  %_90 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_5, i32 0, i32 2, i32 2, i16 0, i32 undef) #3
  %_91 = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> undef, <2 x i32> %_90, i32 0, i32 2, i32 2, i16 0, i32 undef, i1 true) #3
  %_92 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_81, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_93 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_5, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_94 = and <2 x i32> %_93, <i32 -2146435073, i32 -2146435073>
  %_95 = or <2 x i32> %_94, %_92
  %_96 = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> %_91, <2 x i32> %_95, i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true) #3
  %_97 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_11, i32 0, i32 2, i32 2, i16 0, i32 undef) #3
  %_98 = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> undef, <2 x i32> %_97, i32 0, i32 2, i32 2, i16 0, i32 undef, i1 true) #3
  %_99 = and <2 x i32> %_80.phi, <i32 -2146435073, i32 -2146435073>
  %_100 = or <2 x i32> %_99, <i32 1072693248, i32 1072693248>
  %_101 = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> %_98, <2 x i32> %_100, i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true) #3
  %_13 = bitcast <4 x i32> %_96 to <2 x double>
  %_102 = select <2 x i1> %_49, <2 x double> %_13, <2 x double> %_4
  %_14 = bitcast <4 x i32> %_101 to <2 x double>
  %_103 = select <2 x i1> %_49, <2 x double> %_14, <2 x double> %_10
  %_104 = tail call <2 x double> @llvm.fabs.v2f64(<2 x double> %_102) #3
  %_15 = bitcast <2 x double> %_104 to <2 x i64>
  %_105 = tail call <2 x double> @llvm.fabs.v2f64(<2 x double> %_103) #3
  %_16 = bitcast <2 x double> %_105 to <2 x i64>
  %_106 = sub <2 x i64> %_15, %_16
  %_17 = bitcast <2 x i64> %_106 to <4 x i32>
  %_107 = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_17, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_108 = ashr <2 x i32> %_107, <i32 20, i32 20>
  %_109 = add nsw <2 x i32> %_108, %_83
  %_110 = select <2 x i1> %_49, <2 x i32> %_109, <2 x i32> zeroinitializer
  %_111 = bitcast <4 x i32> %_87 to <2 x double>
  %_112 = bitcast <4 x i32> %_89 to <2 x double>
  br label %if.end163.i

; 1-st Region
; <<< if region end
if.end163.i:                                      ; preds = %if.end78.i, %entry
  ; phi for fixing
  %_18 = phi <2 x double> [ %_103, %if.end78.i ], [ %b, %entry ]
  ; !!! Also must be phi for fixing
  %_19 = phi <2 x double> [ %_102, %if.end78.i ], [ %a, %entry ]
  %_113.phi = phi <2 x double> [ %_112, %if.end78.i ], [ zeroinitializer, %entry ]
  %_114.phi = phi <2 x double> [ %_111, %if.end78.i ], [ zeroinitializer, %entry ]
  %_20 = phi <2 x i32> [ %_110, %if.end78.i ], [ zeroinitializer, %entry ]
  %_21 = phi <2 x i32> [ %_12, %if.end78.i ], [ %_44, %entry ]
  %_22 = phi <2 x i32> [ %_6, %if.end78.i ], [ %_39, %entry ]
  %_115 = tail call <2 x double> @llvm.fma.v2f64(<2 x double> %_19, <2 x double> %_18, <2 x double> %_19) #3
  br i1 %_51, label %if.then180.i, label %_branch_.exit

; 2-nd Region
if.then180.i:                                     ; preds = %if.end163.i
  %cmp.i.i.i249.i = icmp slt <2 x i32> %_20, <i32 1024, i32 1024>
  %cmp.i.i.i244.i = icmp sgt <2 x i32> %_20, <i32 967, i32 967>
  %and.i15.i2401277.i = and <2 x i1> %cmp.i.i.i244.i, %cmp.i.i.i249.i
  %conv.i.i.i8.i.i.i232.i = sext <2 x i1> %and.i15.i2401277.i to <2 x i8>
  %call2.i233.i = tail call i1 @llvm.genx.any.v2i8(<2 x i8> %conv.i.i.i8.i.i.i232.i) #3
  br i1 %call2.i233.i, label %if.then212.i, label %if.end202.i

; 2-nd Region
if.end202.i:                                      ; preds = %if.then180.i
  %mul.i.i214.i = fmul <2 x double> %_114.phi, %_115
  %_23 = bitcast <2 x double> %mul.i.i214.i to <4 x i32>
  br label %if.end279.i

; 2-nd Region
if.then212.i:                                     ; preds = %if.then180.i
  %call.i.i222.i = select <2 x i1> %and.i15.i2401277.i, <2 x double> %_115, <2 x double> %_115
  %mul.i.i2141275.i = fmul <2 x double> %_114.phi, %call.i.i222.i
  %_24 = bitcast <2 x double> %mul.i.i2141275.i to <4 x i32>
  %cmp.i.i.i200.i = fcmp une <2 x double> %_115, zeroinitializer
  %cmp.i.i.i196.i = icmp eq <2 x i32> %_20, <i32 1023, i32 1023>
  %and.i15.i1921281.i = xor <2 x i1> %and.i15.i2401277.i, %cmp.i.i.i196.i
  %and.i.i1851282.i = and <2 x i1> %cmp.i.i.i200.i, %and.i15.i1921281.i
  %call.i.i167.i = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_24, i32 0, i32 2, i32 2, i16 0, i32 undef) #3
  %or.i.i.i163.i = zext <2 x i1> %and.i.i1851282.i to <2 x i32>
  %call.i.i8.i160.i = or <2 x i32> %call.i.i167.i, %or.i.i.i163.i
  %call.i.i4.i161.i = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> %_24, <2 x i32> %call.i.i8.i160.i, i32 0, i32 2, i32 2, i16 0, i32 undef, i1 true) #3
  %and.i.i1283.i = and <2 x i1> %cmp.i.i.i200.i, %cmp.i.i.i196.i
  %conv.i.i.i8.i.i.i.i = sext <2 x i1> %and.i.i1283.i to <2 x i8>
  %call2.i144.i = tail call i1 @llvm.genx.any.v2i8(<2 x i8> %conv.i.i.i8.i.i.i.i) #3
  br i1 %call2.i144.i, label %if.then244.i, label %if.end279.i

; 2-nd Region
if.then244.i:                                     ; preds = %if.then212.i
  %call.i.i111.i = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %call.i.i4.i161.i, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %and.i.i.i107.i = and <2 x i32> %call.i.i111.i, <i32 -1048576, i32 -1048576>
  %call.i.i105.i = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> zeroinitializer, <2 x i32> %and.i.i.i107.i, i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true) #3
  %_25 = bitcast <4 x i32> %call.i.i4.i161.i to <2 x double>
  %_26 = bitcast <4 x i32> %call.i.i105.i to <2 x double>
  %sub.i.i.i = fsub <2 x double> %_25, %_26
  %_27 = bitcast <2 x double> %sub.i.i.i to <4 x i32>
  %call.i.i94.i = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_27, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %and.i.i.i90.i = and <2 x i32> %call.i.i94.i, <i32 2147483647, i32 2147483647>
  %call.i.i87.i = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %call.i.i105.i, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %and.i.i.i.i = and <2 x i32> %call.i.i87.i, <i32 -2147483648, i32 -2147483648>
  %or.i.i82.i = or <2 x i32> %and.i.i.i.i, %and.i.i.i90.i
  %call.i.i79.i = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> %_27, <2 x i32> %or.i.i82.i, i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true) #3
  %call.i.i73.i = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %call.i.i79.i, i32 0, i32 2, i32 2, i16 0, i32 undef) #3
  %or.i.i.i.i = or <2 x i32> %call.i.i73.i, <i32 1, i32 1>
  %call.i.i68.i = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> %call.i.i79.i, <2 x i32> %or.i.i.i.i, i32 0, i32 2, i32 2, i16 0, i32 undef, i1 true) #3
  %_28 = bitcast <4 x i32> %call.i.i68.i to <2 x double>
  %call.i.i63.i = select <2 x i1> %and.i.i1283.i, <2 x double> %_28, <2 x double> %_25
  %_29 = bitcast <2 x double> %call.i.i63.i to <4 x i32>
  br label %if.end279.i

; 2-nd Region
if.end279.i:                                      ; preds = %if.then244.i, %if.then212.i, %if.end202.i
  %_30 = phi <2 x double> [ %_115, %if.end202.i ], [ %call.i.i222.i, %if.then244.i ], [ %call.i.i222.i, %if.then212.i ]
  %qscaled.sroa.0.1.i = phi <4 x i32> [ %_23, %if.end202.i ], [ %_29, %if.then244.i ], [ %call.i.i4.i161.i, %if.then212.i ]
  %_31 = phi <2 x i1> [ zeroinitializer, %if.end202.i ], [ %and.i.i1283.i, %if.then244.i ], [ %and.i.i1283.i, %if.then212.i ]
  %_32 = bitcast <4 x i32> %qscaled.sroa.0.1.i to <2 x double>
  %mul.i.i = fmul <2 x double> %_113.phi, %_32
  %_33 = bitcast <2 x double> %mul.i.i to <4 x i32>
  %call.i.i44.i = tail call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> %_33, i32 0, i32 2, i32 2, i16 4, i32 undef) #3
  %_109.i = add <2 x i32> %call.i.i44.i, <i32 524288, i32 524288>
  %call.i.i8.i.i = select <2 x i1> %_31, <2 x i32> %_109.i, <2 x i32> %call.i.i44.i
  %call.i.i4.i.i = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32> %_33, <2 x i32> %call.i.i8.i.i, i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true) #3
  %_34 = bitcast <4 x i32> %call.i.i4.i.i to <2 x double>
  %call.i.i39.i = select <2 x i1> %_49, <2 x double> %_34, <2 x double> %_30
  %sub.i.i.i26.i = add nsw <2 x i32> %_22, <i32 -1, i32 -1>
  %cmp.i.i.i22.i = icmp ugt <2 x i32> %sub.i.i.i26.i, <i32 2045, i32 2045>
  %sub.i.i.i.i = add nsw <2 x i32> %_21, <i32 -1, i32 -1>
  %cmp.i.i.i.i = icmp ugt <2 x i32> %sub.i.i.i.i, <i32 2045, i32 2045>
  %or.i15.i1280.i = or <2 x i1> %cmp.i.i.i22.i, %cmp.i.i.i.i
  %conv.i.i.i.i.i.i.i = sext <2 x i1> %or.i15.i1280.i to <2 x i8>
  %call2.i.i = tail call i1 @llvm.genx.any.v2i8(<2 x i8> %conv.i.i.i.i.i.i.i) #3
  %call.i.i.i = select <2 x i1> %or.i15.i1280.i, <2 x double> zeroinitializer, <2 x double> %call.i.i39.i
  %spec.select.i = select i1 %call2.i.i, <2 x double> %call.i.i.i, <2 x double> %call.i.i39.i
  br label %_branch_.exit

; 2-nd Region
; <<< if region end
_branch_.exit:                                    ; preds = %if.end279.i, %if.end163.i
  %r.sroa.0.0 = phi <2 x double> [ %spec.select.i, %if.end279.i ], [ %_115, %if.end163.i ]
  ret <2 x double> %r.sroa.0.0
}

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !3 <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32>, <2 x i32>, i32, i32, i32, i16, i32, i1) #1

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !4 <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare <2 x double> @llvm.fabs.v2f64(<2 x double>) #2

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !5 i1 @llvm.genx.any.v2i8(<2 x i8>) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare <2 x double> @llvm.fma.v2f64(<2 x double>, <2 x double>, <2 x double>) #2

attributes #0 = { noinline nounwind "VC.Emulation.Routine" "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="384" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind readnone speculatable willreturn }
attributes #3 = { nounwind }

!opencl.ocl.version = !{!0, !0, !0, !0}
!opencl.spir.version = !{!0, !0, !0, !0}
!llvm.ident = !{!1, !1, !1, !1}
!llvm.module.flags = !{!2}

!0 = !{i32 2, i32 0}
!1 = !{!"clang version 10.0.0 (c850858c57d75a2670ceceea8f7bb1ba8e3803ce)"}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{i32 7959}
!4 = !{i32 7753}
!5 = !{i32 7560}
