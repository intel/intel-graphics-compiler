;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey TotalGRFNum=256 --regkey DumpLoopSink=1 --regkey PrintToConsole=1 --regkey LoopSinkThresholdDelta=30 --regkey DisableCodeSinking=0 --regkey DisableLoopSink=0 --regkey EnableLoadChainLoopSink=1 -platformdg2 -simd-mode 16 --igc-wi-analysis %enable-basic-aa% --scev-aa --igc-code-sinking -S %s 2>&1 | FileCheck %s

; The own regpressure of the functions is not enough to sink
; Checking that we use interprocedural analysis to find out that combined regpressure is high enough
; And trying to sink some instructions into the loop

; Checking the Loop Sink dumps

; CHECK: Function bar
; CHECK: Checking loop with preheader bb45:
; CHECK-NEXT: Threshold to sink = 286
; CHECK-NEXT: MaxLoopPressure = [[PRESSURE:[0-9]+]]
; CHECK-NEXT: MaxLoopPressure + FunctionExternalPressure = [[SUM_PRESSURE:[0-9]+]]
; CHECK-NEXT: >> Sinking in the loop with preheader bb45

%struct.ham = type { %struct.spam addrspace(4)*, %struct.spam addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, %struct.hoge.4 addrspace(4)*, %struct.snork addrspace(4)*, %struct.bar addrspace(4)*, %struct.wobble addrspace(4)*, i32 addrspace(4)*, float addrspace(4)*, i32 addrspace(4)*, %struct.wobble addrspace(4)*, i32 addrspace(4)*, %struct.hoge addrspace(4)*, %struct.ham.0 addrspace(4)*, i32 addrspace(4)*, %struct.hoge.4 addrspace(4)*, %struct.hoge addrspace(4)*, %struct.hoge.8 addrspace(4)*, %struct.spam addrspace(4)*, %struct.wombat addrspace(4)*, i32 addrspace(4)*, %struct.spam addrspace(4)*, i32 addrspace(4)*, %struct.ham.1 addrspace(4)*, float addrspace(4)*, %struct.hoge.4 addrspace(4)*, %struct.hoge addrspace(4)*, %struct.spam addrspace(4)*, %struct.bar.2 addrspace(4)*, %struct.wibble addrspace(4)*, %struct.baz addrspace(4)*, %struct.hoge.4 addrspace(4)*, %struct.hoge.4 addrspace(4)*, %struct.snork.5 addrspace(4)*, %struct.barney addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, %struct.wibble.7 addrspace(4)*, %struct.hoge.8 addrspace(4)*, %struct.widget.9 addrspace(4)*, float addrspace(4)*, float addrspace(4)*, %struct.spam.10 addrspace(4)*, float addrspace(4)*, %struct.zot addrspace(4)*, %struct.hoge.20 addrspace(4)*, %struct.wobble.29 }
%struct.snork = type { %struct.bar, %struct.bar, float, float, float, [3 x float], float, i32, [3 x float], [2 x float], i32, i32, i32, i32, i32, i32, float, float, float, float, float, i32, i32, i32, float, i64, i32, i64, i32 }
%struct.bar = type { %struct.spam, %struct.spam, %struct.spam }
%struct.spam = type { float, float, float, float }
%struct.wobble = type { %struct.spam, %struct.spam, %struct.spam, %struct.spam }
%struct.ham.0 = type { i32, i32, i32 }
%struct.wombat = type { i32, i32 }
%struct.ham.1 = type { i64, i32, i16, i8, i8 }
%struct.hoge = type { float, float, float }
%struct.bar.2 = type { i8, i8, i8, i8 }
%struct.wibble = type { float, i32, %struct.wombat }
%struct.baz = type { i32, %struct.hoge, i32, float, float, [3 x float], i32, i32, %struct.bar, %struct.bar, %struct.widget, i64, i64 }
%struct.widget = type { %struct.spam.3 }
%struct.spam.3 = type { %struct.hoge, float, %struct.hoge, float, %struct.hoge, float, float, float, float, float }
%struct.hoge.4 = type { float, float }
%struct.snork.5 = type { %struct.blam, %struct.wobble.6, float, i8, i32, %struct.pluto, i32, i8, [11 x i8] }
%struct.blam = type { %struct.hoge, %struct.hoge }
%struct.wobble.6 = type { %struct.hoge, float, float }
%struct.pluto = type { %struct.wombat }
%struct.barney = type { float, float, float, %struct.pluto, %struct.wombat, i32 }
%struct.wibble.7 = type { i32, float, float, float, %struct.spam, %struct.spam, %struct.spam, %struct.spam }
%struct.hoge.8 = type { i32, i32, i32, i32 }
%struct.widget.9 = type { [3 x float], float, i32, i32, i32, i32 }
%struct.spam.10 = type { i64, i32, i32, i32, i32, i32, i32, i32, [12 x i8], %struct.bar }
%struct.zot = type { %struct.wombat.11, %struct.wibble.12, %struct.pluto.13, %struct.widget.14, [32 x %struct.wibble.18], %struct.wobble.15, %struct.wobble.16, %struct.quux, %struct.pluto.17, [4 x %struct.pluto.13], [32 x %struct.wibble.18], %struct.pluto.19 addrspace(4)*, [16 x i32 addrspace(4)*], i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 }
%struct.wombat.11 = type { i32 addrspace(4)*, i32 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i32 addrspace(4)*, i16 addrspace(4)*, i32 addrspace(4)*, i8 addrspace(4)*, float addrspace(4)*, i32 addrspace(4)*, %struct.hoge addrspace(4)*, float addrspace(4)*, float addrspace(4)*, %struct.hoge addrspace(4)*, float addrspace(4)*, %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, i32 addrspace(4)* }
%struct.wibble.12 = type { %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, float addrspace(4)*, float addrspace(4)*, float addrspace(4)*, float addrspace(4)*, float addrspace(4)* }
%struct.pluto.13 = type { float addrspace(4)*, float addrspace(4)*, float addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)* }
%struct.widget.14 = type { %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, float addrspace(4)*, %struct.hoge addrspace(4)* }
%struct.wobble.15 = type { i64 addrspace(4)*, i8 addrspace(4)*, float addrspace(4)*, float addrspace(4)*, float addrspace(4)*, i8 addrspace(4)*, float addrspace(4)*, float addrspace(4)* }
%struct.wobble.16 = type { float addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)* }
%struct.quux = type { i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i16 addrspace(4)*, i32 addrspace(4)*, %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, i16 addrspace(4)*, i8 addrspace(4)*, %struct.hoge addrspace(4)*, i64 addrspace(4)*, float addrspace(4)* }
%struct.pluto.17 = type { %struct.hoge addrspace(4)*, %struct.hoge addrspace(4)*, float addrspace(4)*, float addrspace(4)*, float addrspace(4)*, float addrspace(4)*, i32 addrspace(4)* }
%struct.wibble.18 = type { i32 addrspace(4)*, i32 addrspace(4)* }
%struct.pluto.19 = type { [16 x i32] }
%struct.hoge.20 = type { i32, i32, i32, i32, %struct.wibble.21, %struct.hoge.8, %struct.baz.22, [64 x %struct.zot.23], %struct.quux.24, %struct.wibble.25, %struct.wombat.26, %struct.baz.27, %struct.blam.28, i32, i32, i32, i32 }
%struct.wibble.21 = type { i32, i32, float, float, float, float, float, i32, i32, i32, i32, i32, i32, float, float, float, %struct.spam, %struct.spam, float, float, float, float, %struct.bar, %struct.wobble, %struct.spam, %struct.spam, float, float, float, float, float, float, float, i32, %struct.wobble, %struct.wobble, %struct.wobble, %struct.wobble, %struct.wobble, %struct.wobble, %struct.bar, %struct.wobble, %struct.wobble, %struct.bar, %struct.bar, i32, i32, float, i32 }
%struct.baz.22 = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32 }
%struct.zot.23 = type { i32 }
%struct.quux.24 = type { %struct.spam, i32, i32, i32, float, i32, float, float, float, float, i32, i32, i32, i32, i32, i32, i32, i32, [12 x i8] }
%struct.wibble.25 = type { i32, i32, i32, i32, i32, i32, i32, i32 }
%struct.wombat.26 = type { %struct.spam, %struct.spam, %struct.spam, %struct.spam, %struct.spam, %struct.spam, %struct.spam, i32, float, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, float, i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, [8 x i8] }
%struct.baz.27 = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, float, float, float, i32, i32, i32, i32, i32, i32, i32, float, float, float, i32, i32, i32, i32, i32, float, i32, float, float, i32, i32, float, i32, i32, i32, i32, float, i32, i32, i32, float, float, i32, i32, float, i32, i32, i32, i32, i32, i32, i32, i32, i32 }
%struct.blam.28 = type { i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32 }
%struct.wobble.29 = type { i8 addrspace(4)* }
%struct.spam.30 = type { %struct.ham }
%struct.quux.31 = type { %struct.hoge, %struct.hoge, %struct.hoge, %struct.hoge, i32, i32, i32, i32, i32, float, float, i32, i32, float, float, float, float, %struct.hoge.4, %struct.hoge.4, %struct.hoge, %struct.hoge, [4 x i8], %struct.bar, %struct.bar, %struct.hoge, float, i32, i32, i32, %struct.hoge, %struct.hoge, [12 x i8], [64 x %struct.widget.32] }
%struct.widget.32 = type { %struct.hoge, i32, float, %struct.hoge, [40 x i8], [8 x i8] }

define spir_kernel void @zot(i32 %arg, i32 addrspace(1)* %arg1, %struct.ham addrspace(1)* %arg2, float addrspace(1)* %arg3, <8 x i32> %arg4, <8 x i32> %arg5, <3 x i32> %arg6, i16 %arg7, i16 %arg8, i16 %arg9, i8 addrspace(2)* %arg10, i8 addrspace(1)* %arg11, i8* %arg12, i32 %arg13, i32 %arg14, i32 %arg15) {
bb:
  %tmp = zext i16 0 to i32
  %tmp16 = call i32 @llvm.genx.GenISA.simdSize()
  %tmp17 = mul i32 0, 0
  %tmp18 = add i32 0, 0
  br label %bb19

bb19:                                             ; preds = %bb
  br label %bb20

bb20:                                             ; preds = %bb19
  br label %bb21

bb21:                                             ; preds = %bb20
  %tmp22 = bitcast %struct.ham addrspace(1)* null to %struct.spam.30 addrspace(1)*
  %tmp23 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp24 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp25 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp26 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp27 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  %tmp28 = call %struct.quux.31* @llvm.genx.GenISA.pair.to.ptr.p0struct.ShaderData(i32 0, i32 0)
  %tmp29 = add i32 0, 0
  %tmp30 = inttoptr i32 0 to %struct.zot addrspace(4)* addrspace(131073)*
  %tmp31 = shl i32 0, 0
  %tmp32 = ashr i32 0, 0
  %tmp33 = or i32 0, 0
  %tmp34 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp35 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp36 = call i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32 0, i32 0)
  %tmp37 = call i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32 0, i32 0)
  %tmp38 = call i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32 0, i32 0)
  %tmp39 = call i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32 0, i32 0)
  %tmp40 = call i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32 0, i32 0)
  %tmp41 = call float* @llvm.genx.GenISA.pair.to.ptr.p0f32(i32 0, i32 0)
  br label %bb42

bb42:                                             ; preds = %bb21
  br label %bb43

bb43:                                             ; preds = %bb42
  %tmp44 = call <3 x float>* @llvm.genx.GenISA.pair.to.ptr.p0v3f32(i32 0, i32 0)
  br label %bb45

bb45:                                             ; preds = %bb43
  br label %bb46

bb46:                                             ; preds = %bb45
  br label %bb47

bb47:                                             ; preds = %bb46
  br label %bb48

bb48:                                             ; preds = %bb47
  br label %bb49

bb49:                                             ; preds = %bb48
  br label %bb50

bb50:                                             ; preds = %bb49
  br label %bb51

bb51:                                             ; preds = %bb50
  br label %bb52

bb52:                                             ; preds = %bb51
  br label %bb53

bb53:                                             ; preds = %bb52
  br label %bb54

bb54:                                             ; preds = %bb53
  %tmp55 = call i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32 0, i32 0)
  %tmp56 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp57 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp58 = call float* @llvm.genx.GenISA.pair.to.ptr.p0f32(i32 0, i32 0)
  %tmp59 = call float* @llvm.genx.GenISA.pair.to.ptr.p0f32(i32 0, i32 0)
  %tmp60 = call float* @llvm.genx.GenISA.pair.to.ptr.p0f32(i32 0, i32 0)
  br label %bb61

bb61:                                             ; preds = %bb54
  br label %bb62

bb62:                                             ; preds = %bb61
  %tmp63 = call float* @llvm.genx.GenISA.pair.to.ptr.p0f32(i32 0, i32 0)
  br label %bb64

bb64:                                             ; preds = %bb62
  br label %bb65

bb65:                                             ; preds = %bb64
  br label %bb66

bb66:                                             ; preds = %bb65
  %tmp67 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp68 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp69 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  %tmp70 = mul nuw nsw i32 0, 0
  %tmp71 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp72 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp73 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  br label %bb74

bb74:                                             ; preds = %bb66
  br label %bb75

bb75:                                             ; preds = %bb74
  br label %bb76

bb76:                                             ; preds = %bb75
  br label %bb77

bb77:                                             ; preds = %bb76
  br label %bb78

bb78:                                             ; preds = %bb77
  br label %bb79

bb79:                                             ; preds = %bb78
  br label %bb80

bb80:                                             ; preds = %bb79
  %tmp81 = load i32, i32 addrspace(1)* null, align 4
  %tmp82 = zext i16 0 to i32
  %tmp83 = load i32, i32 addrspace(1)* null, align 4
  %tmp84 = add i32 0, 0
  %tmp85 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp86 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp87 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  br label %bb88

bb88:                                             ; preds = %bb80
  br label %bb89

bb89:                                             ; preds = %bb88
  %tmp90 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp91 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp92 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  br label %bb93

bb93:                                             ; preds = %bb89
  br label %bb94

bb94:                                             ; preds = %bb93
  br label %bb95

bb95:                                             ; preds = %bb94
  br label %bb96

bb96:                                             ; preds = %bb95
  br label %bb97

bb97:                                             ; preds = %bb96
  br label %bb98

bb98:                                             ; preds = %bb97
  br label %bb99

bb99:                                             ; preds = %bb98
  br label %bb100

bb100:                                            ; preds = %bb99
  br label %bb101

bb101:                                            ; preds = %bb100
  br label %bb102

bb102:                                            ; preds = %bb101
  %tmp103 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp104 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp105 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  %tmp106 = call <3 x float>* @llvm.genx.GenISA.pair.to.ptr.p0v3f32(i32 0, i32 0)
  %tmp107 = fadd fast float 0.000000e+00, 0.000000e+00
  %tmp108 = fcmp olt float %tmp107, 0.000000e+00
  %tmp109 = call %struct.quux.31* @llvm.genx.GenISA.pair.to.ptr.p0struct.ShaderData(i32 0, i32 0)
  %tmp110 = call i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32 0, i32 0)
  br label %bb111

bb111:                                            ; preds = %bb102
  %tmp112 = load i32, i32* null, align 4
  %tmp113 = icmp eq i32 %tmp112, 0
  ret void

bb114:                                            ; No predecessors!
  %tmp115 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  %tmp116 = call %struct.hoge* @llvm.genx.GenISA.pair.to.ptr.p0struct.packed_float3(i32 0, i32 0)
  %tmp117 = call float* @llvm.genx.GenISA.pair.to.ptr.p0f32(i32 0, i32 0)
  %tmp118 = inttoptr i32 0 to %struct.hoge.8 addrspace(4)* addrspace(131073)*
  %tmp119 = call <3 x float>* @llvm.genx.GenISA.pair.to.ptr.p0v3f32(i32 0, i32 0)
  %tmp120 = call <3 x i32>* @llvm.genx.GenISA.pair.to.ptr.p0v3i32(i32 0, i32 0)
  br label %bb121

bb121:                                            ; preds = %bb151, %bb114
  %tmp122 = bitcast %struct.hoge.8 addrspace(4)* addrspace(131073)* %tmp118 to i64 addrspace(131073)*
  br i1 false, label %bb135, label %bb123

bb123:                                            ; preds = %bb121
  br label %bb124

bb124:                                            ; preds = %bb123
  br i1 false, label %bb131, label %bb125

bb125:                                            ; preds = %bb124
  br label %bb126

bb126:                                            ; preds = %bb125
  br label %bb127

bb127:                                            ; preds = %bb126
  br label %bb128

bb128:                                            ; preds = %bb127
  br label %bb129

bb129:                                            ; preds = %bb128
  br label %bb130

bb130:                                            ; preds = %bb129
  br label %bb153

bb131:                                            ; preds = %bb124
  br label %bb132

bb132:                                            ; preds = %bb131
  br i1 false, label %bb134, label %bb133

bb133:                                            ; preds = %bb132
  br label %bb150

bb134:                                            ; preds = %bb132
  br label %bb149

bb135:                                            ; preds = %bb121
  br label %bb136

bb136:                                            ; preds = %bb135
  br label %bb137

bb137:                                            ; preds = %bb136
  br i1 false, label %bb140, label %bb138

bb138:                                            ; preds = %bb137
  br label %bb139

bb139:                                            ; preds = %bb138
  br i1 false, label %bb146, label %bb144

bb140:                                            ; preds = %bb137
  br label %bb141

bb141:                                            ; preds = %bb140
  br label %bb142

bb142:                                            ; preds = %bb141
  br label %bb143

bb143:                                            ; preds = %bb142
  store <3 x i32> zeroinitializer, <3 x i32>* %tmp120, align 4
  br label %bb151

bb144:                                            ; preds = %bb139
  %tmp145 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %tmp67, i32 %tmp68, i32 0, i32 0)
  store <3 x float> zeroinitializer, <3 x float>* %tmp119, align 4
  br label %bb151

bb146:                                            ; preds = %bb139
  call spir_func void @snork(%struct.spam.30 addrspace(4)* null, %struct.ham addrspace(4)* null, %struct.quux.31 addrspace(4)* null, float* %tmp117, %struct.hoge* %tmp116, %struct.hoge.8 zeroinitializer, i8 addrspace(2)* null, i8 addrspace(1)* null)
  br label %bb151

bb147:                                            ; No predecessors!
  %tmp148 = call spir_func i32 @bar(%struct.spam.30 addrspace(1)* null, %struct.ham addrspace(4)* null, %struct.quux.31 addrspace(4)* null, float* null, i32 0, i32 0, i32 0, i32 0, i8 addrspace(2)* null, i8 addrspace(1)* null)
  br label %bb151

bb149:                                            ; preds = %bb134
  call spir_func void @ham(%struct.spam.30 addrspace(1)* %tmp22, %struct.ham addrspace(4)* null, %struct.quux.31* %tmp109, float* null, %struct.hoge.8 zeroinitializer, i8 addrspace(2)* null, i8 addrspace(1)* null)
  br label %bb151

bb150:                                            ; preds = %bb133
  call spir_func void @wibble(%struct.spam.30 addrspace(1)* null, %struct.ham addrspace(4)* null, %struct.quux.31 addrspace(4)* null, float* null, i32 0, i32 0, i8 addrspace(2)* null, i8 addrspace(1)* %arg11)
  br label %bb151

bb151:                                            ; preds = %bb150, %bb149, %bb147, %bb146, %bb144, %bb143
  %tmp152 = phi i32 [ 0, %bb150 ], [ 0, %bb149 ], [ %tmp148, %bb147 ], [ 0, %bb146 ], [ 0, %bb144 ], [ 0, %bb143 ]
  br label %bb121

bb153:                                            ; preds = %bb130
  br label %bb154

bb154:                                            ; preds = %bb153
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %tmp115)
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %tmp69)
  %tmp155 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %tmp103, i32 %tmp104, i32 0, i32 0)
  br i1 %tmp113, label %bb156, label %bb158

bb156:                                            ; preds = %bb154
  br label %bb157

bb157:                                            ; preds = %bb156
  br label %bb159

bb158:                                            ; preds = %bb154
  ret void

bb159:                                            ; preds = %bb157
  br label %bb160

bb160:                                            ; preds = %bb159
  br label %bb161

bb161:                                            ; preds = %bb160
  br i1 false, label %bb162, label %bb163

bb162:                                            ; preds = %bb161
  br label %bb230

bb163:                                            ; preds = %bb161
  %tmp164 = load i32, i32* %tmp110, align 4
  %tmp165 = load <3 x float>, <3 x float>* %tmp106, align 4
  %tmp166 = mul i32 %tmp16, 0
  %tmp167 = add i32 %tmp17, 0
  %tmp168 = shl nuw nsw i32 %tmp, 0
  br i1 false, label %bb170, label %bb169

bb169:                                            ; preds = %bb163
  br label %bb177

bb170:                                            ; preds = %bb163
  br label %bb171

bb171:                                            ; preds = %bb170
  br label %bb172

bb172:                                            ; preds = %bb171
  br label %bb173

bb173:                                            ; preds = %bb172
  br label %bb174

bb174:                                            ; preds = %bb173
  br label %bb175

bb175:                                            ; preds = %bb174
  %tmp176 = call spir_func %struct.hoge @pluto(%struct.spam.30 addrspace(1)* null, %struct.ham addrspace(4)* null, %struct.quux.31* %tmp28, %struct.widget.32* null, %struct.hoge zeroinitializer, float* null, i8 addrspace(2)* null, i8 addrspace(1)* null)
  ret void

bb177:                                            ; preds = %bb169
  br label %bb178

bb178:                                            ; preds = %bb177
  br label %bb179

bb179:                                            ; preds = %bb178
  br label %bb180

bb180:                                            ; preds = %bb179
  br label %bb181

bb181:                                            ; preds = %bb180
  br label %bb182

bb182:                                            ; preds = %bb181
  br label %bb183

bb183:                                            ; preds = %bb182
  br label %bb184

bb184:                                            ; preds = %bb183
  %tmp185 = call <3 x float>* @llvm.genx.GenISA.pair.to.ptr.p0v3f32(i32 %tmp25, i32 %tmp26)
  %tmp186 = load i32, i32* %tmp36, align 16
  %tmp187 = load i32, i32* %tmp55, align 16
  br i1 false, label %bb188, label %bb189

bb188:                                            ; preds = %bb184
  br label %bb202

bb189:                                            ; preds = %bb184
  %tmp190 = add i32 %arg14, 0
  br label %bb191

bb191:                                            ; preds = %bb189
  br label %bb192

bb192:                                            ; preds = %bb191
  br label %bb193

bb193:                                            ; preds = %bb192
  br label %bb194

bb194:                                            ; preds = %bb193
  br label %bb195

bb195:                                            ; preds = %bb194
  br label %bb196

bb196:                                            ; preds = %bb195
  call void @llvm.lifetime.start.p0i8(i64 0, i8* %tmp73)
  call void @llvm.lifetime.start.p0i8(i64 0, i8* %tmp87)
  br label %bb197

bb197:                                            ; preds = %bb196
  %tmp198 = load i32, i32* %tmp39, align 4
  %tmp199 = load float, float* %tmp41, align 4
  %tmp200 = call %struct.hoge* @llvm.genx.GenISA.pair.to.ptr.p0struct.packed_float3(i32 %tmp71, i32 %tmp72)
  %tmp201 = call %struct.hoge* @llvm.genx.GenISA.pair.to.ptr.p0struct.packed_float3(i32 %tmp85, i32 %tmp86)
  ret void

bb202:                                            ; preds = %bb188
  br label %bb203

bb203:                                            ; preds = %bb202
  br label %bb204

bb204:                                            ; preds = %bb203
  %tmp205 = load i32, i32* %tmp37, align 4
  %tmp206 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %tmp90, i32 %tmp91, i32 0, i32 0)
  br i1 false, label %bb207, label %bb208

bb207:                                            ; preds = %bb204
  br label %bb218

bb208:                                            ; preds = %bb204
  br label %bb209

bb209:                                            ; preds = %bb208
  %tmp210 = add i32 0, %tmp70
  %tmp211 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %tmp23, i32 %tmp24, i32 0, i32 0)
  br label %bb212

bb212:                                            ; preds = %bb209
  br label %bb213

bb213:                                            ; preds = %bb212
  %tmp214 = load i32, i32* %tmp38, align 8
  br label %bb215

bb215:                                            ; preds = %bb213
  br label %bb216

bb216:                                            ; preds = %bb215
  br label %bb217

bb217:                                            ; preds = %bb216
  ret void

bb218:                                            ; preds = %bb207
  br label %bb219

bb219:                                            ; preds = %bb218
  br label %bb220

bb220:                                            ; preds = %bb219
  br label %bb221

bb221:                                            ; preds = %bb220
  br label %bb222

bb222:                                            ; preds = %bb221
  br label %bb223

bb223:                                            ; preds = %bb222
  br label %bb224

bb224:                                            ; preds = %bb223
  br i1 %tmp108, label %bb226, label %bb225

bb225:                                            ; preds = %bb224
  br label %bb228

bb226:                                            ; preds = %bb224
  %tmp227 = load i32, i32* %tmp40, align 4
  ret void

bb228:                                            ; preds = %bb225
  %tmp229 = inttoptr i32 %tmp29 to <4 x i32> addrspace(131073)*
  ret void

bb230:                                            ; preds = %bb162
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %tmp105)
  br label %bb231

bb231:                                            ; preds = %bb230
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %tmp92)
  %tmp232 = inttoptr i32 %tmp84 to %struct.hoge.20 addrspace(4)* addrspace(131073)*
  br label %bb233

bb233:                                            ; preds = %bb231
  br label %bb234

bb234:                                            ; preds = %bb233
  br label %bb235

bb235:                                            ; preds = %bb234
  br i1 false, label %bb236, label %bb237

bb236:                                            ; preds = %bb235
  br label %bb266

bb237:                                            ; preds = %bb235
  %tmp238 = add nuw nsw i32 %tmp82, 0
  br label %bb239

bb239:                                            ; preds = %bb237
  br label %bb240

bb240:                                            ; preds = %bb239
  br label %bb241

bb241:                                            ; preds = %bb240
  %tmp242 = xor i32 0, %tmp81
  %tmp243 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %tmp83)
  br label %bb244

bb244:                                            ; preds = %bb241
  br label %bb245

bb245:                                            ; preds = %bb244
  br label %bb246

bb246:                                            ; preds = %bb245
  br label %bb247

bb247:                                            ; preds = %bb246
  br label %bb248

bb248:                                            ; preds = %bb247
  br label %bb249

bb249:                                            ; preds = %bb248
  br i1 false, label %bb250, label %bb257

bb250:                                            ; preds = %bb249
  %tmp251 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %tmp31, i32 %tmp33, i32 0, i32 0)
  %tmp252 = load float, float* %tmp63, align 4
  %tmp253 = load float, float* %tmp58, align 8
  %tmp254 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %tmp34, i32 %tmp35, i32 0, i32 0)
  %tmp255 = load float, float* %tmp59, align 4
  %tmp256 = load float, float* %tmp60, align 16
  ret void

bb257:                                            ; preds = %bb249
  br label %bb258

bb258:                                            ; preds = %bb257
  br label %bb259

bb259:                                            ; preds = %bb258
  br label %bb260

bb260:                                            ; preds = %bb259
  br label %bb261

bb261:                                            ; preds = %bb260
  br label %bb262

bb262:                                            ; preds = %bb261
  %tmp263 = select i1 false, i32 %tmp56, i32 0
  %tmp264 = select i1 false, i32 %arg15, i32 0
  %tmp265 = load <3 x float>, <3 x float>* %tmp44, align 4
  ret void

bb266:                                            ; preds = %bb236
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %arg12)
  br label %bb267

bb267:                                            ; preds = %bb266
  %tmp268 = bitcast %struct.zot addrspace(4)* addrspace(131073)* %tmp30 to i64 addrspace(131073)*
  %tmp269 = shl i32 %arg13, 0
  %tmp270 = shl i32 %arg, 0
  ret void
}

declare spir_func %struct.hoge @pluto(%struct.spam.30 addrspace(1)*, %struct.ham addrspace(4)*, %struct.quux.31*, %struct.widget.32*, %struct.hoge, float*, i8 addrspace(2)*, i8 addrspace(1)*)

define internal spir_func i32 @bar(%struct.spam.30 addrspace(1)* %arg, %struct.ham addrspace(4)* %arg1, %struct.quux.31 addrspace(4)* %arg2, float* %arg3, i32 %arg4, i32 %arg5, i32 %arg6, i32 %arg7, i8 addrspace(2)* %arg8, i8 addrspace(1)* %arg9) {
bb:
  %tmp = extractelement <4 x i32> zeroinitializer, i32 0
  %tmp10 = extractelement <4 x i32> zeroinitializer, i32 0
  %tmp11 = extractvalue { i32, i32 } zeroinitializer, 0
  %tmp12 = extractvalue { i32, i32 } zeroinitializer, 1
  %tmp13 = call i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32 0, i32 0)
  br label %bb14

bb14:                                             ; preds = %bb
  br label %bb15

bb15:                                             ; preds = %bb14
  br label %bb16

bb16:                                             ; preds = %bb15
  br label %bb17

bb17:                                             ; preds = %bb16
  br label %bb18

bb18:                                             ; preds = %bb17
  br label %bb19

bb19:                                             ; preds = %bb18
  br label %bb20

bb20:                                             ; preds = %bb19
  %tmp21 = load float, float* null, align 4
  br label %bb22

bb22:                                             ; preds = %bb20
  br label %bb23

bb23:                                             ; preds = %bb22
  br label %bb24

bb24:                                             ; preds = %bb23
  br label %bb25

bb25:                                             ; preds = %bb24
  br label %bb26

bb26:                                             ; preds = %bb25
  %tmp27 = call float @llvm.minnum.f32(float 0.000000e+00, float 0.000000e+00)
  %tmp28 = call float @llvm.genx.GenISA.fsat.f32(float 0.000000e+00)
  %tmp29 = call float @llvm.genx.GenISA.fsat.f32(float 0.000000e+00)
  %tmp30 = fmul fast float 0.000000e+00, 0.000000e+00
  %tmp31 = fmul fast float 0.000000e+00, 0.000000e+00
  %tmp32 = fmul fast float 0.000000e+00, 0.000000e+00
  %tmp33 = fmul fast float 0.000000e+00, 0.000000e+00
  br label %bb34

bb34:                                             ; preds = %bb26
  br label %bb35

bb35:                                             ; preds = %bb34
  %tmp36 = fadd fast float 0.000000e+00, 0.000000e+00
  br label %bb37

bb37:                                             ; preds = %bb35
  br label %bb38

bb38:                                             ; preds = %bb37
  br label %bb39

bb39:                                             ; preds = %bb38
  br label %bb40

bb40:                                             ; preds = %bb39
  %tmp41 = or i1 false, false
  %tmp42 = call float @llvm.ceil.f32(float 0.000000e+00)
  br label %bb43

bb43:                                             ; preds = %bb40
  %tmp44 = fsub fast float 0.000000e+00, 0.000000e+00
  br label %bb45

bb45:                                             ; preds = %bb215, %bb43
  %tmp46 = phi float [ 0.000000e+00, %bb43 ], [ 0.000000e+00, %bb215 ]
  %tmp47 = phi float [ 0.000000e+00, %bb43 ], [ 0.000000e+00, %bb215 ]
  %tmp48 = phi float [ 0.000000e+00, %bb43 ], [ 0.000000e+00, %bb215 ]
  %tmp49 = phi float [ 0.000000e+00, %bb43 ], [ 0.000000e+00, %bb215 ]
  %tmp50 = phi float [ 0.000000e+00, %bb43 ], [ 0.000000e+00, %bb215 ]
  %tmp51 = phi i32 [ 0, %bb43 ], [ 0, %bb215 ]
  %tmp52 = fdiv fast float 0.000000e+00, 0.000000e+00
  %tmp53 = fmul fast float %tmp30, 0.000000e+00
  %tmp54 = fmul fast float %tmp31, 0.000000e+00
  %tmp55 = fmul fast float %tmp32, 0.000000e+00
  %tmp56 = fmul fast float %tmp33, 0.000000e+00
  %tmp57 = call float @llvm.floor.f32(float 0.000000e+00)
  %tmp58 = call float @llvm.floor.f32(float 0.000000e+00)
  %tmp59 = fsub fast float 0.000000e+00, 0.000000e+00
  %tmp60 = fsub fast float 0.000000e+00, 0.000000e+00
  %tmp61 = fsub fast float 0.000000e+00, 0.000000e+00
  %tmp62 = fsub fast float 0.000000e+00, 0.000000e+00
  %tmp63 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp64 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp65 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp66 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp67 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp68 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp69 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp70 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp71 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp72 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp73 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp74 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp75 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp76 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp77 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp78 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp79 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 1
  %tmp80 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp81 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp82 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp83 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp84 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp85 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp86 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp87 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp88 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp89 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp90 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp91 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp92 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp93 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp94 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp95 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp96 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp97 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  %tmp98 = insertvalue %struct.spam zeroinitializer, float 0.000000e+00, 2
  br label %bb99

bb99:                                             ; preds = %bb166, %bb45
  %tmp100 = phi float [ 0.000000e+00, %bb45 ], [ 0.000000e+00, %bb166 ]
  %tmp101 = phi i32 [ 0, %bb45 ], [ 0, %bb166 ]
  %tmp102 = phi float [ 0.000000e+00, %bb45 ], [ 0.000000e+00, %bb166 ]
  %tmp103 = phi float [ 0.000000e+00, %bb45 ], [ 0.000000e+00, %bb166 ]
  %tmp104 = phi float [ 0.000000e+00, %bb45 ], [ 0.000000e+00, %bb166 ]
  %tmp105 = phi float [ 0.000000e+00, %bb45 ], [ 0.000000e+00, %bb166 ]
  %tmp106 = sitofp i32 0 to float
  %tmp107 = fadd fast float %tmp58, 0.000000e+00
  %tmp108 = insertvalue %struct.spam %tmp64, float 0.000000e+00, 3
  %tmp109 = fmul fast float 0.000000e+00, %tmp29
  %tmp110 = fsub fast float 0.000000e+00, %tmp59
  %tmp111 = fsub fast float 0.000000e+00, %tmp60
  %tmp112 = fsub fast float 0.000000e+00, %tmp61
  %tmp113 = fsub fast float 0.000000e+00, %tmp62
  %tmp114 = fmul fast float 0.000000e+00, 0.000000e+00
  %tmp115 = fmul fast float 0.000000e+00, 0.000000e+00
  %tmp116 = fmul fast float 0.000000e+00, 0.000000e+00
  %tmp117 = fmul fast float 0.000000e+00, 0.000000e+00
  %tmp118 = fadd fast float %tmp116, %tmp117
  %tmp119 = fadd fast float 0.000000e+00, %tmp114
  %tmp120 = fadd fast float 0.000000e+00, %tmp115
  %tmp121 = fcmp olt float 0.000000e+00, %tmp100
  %tmp122 = select i1 false, float %tmp113, float %tmp105
  %tmp123 = select i1 false, float %tmp112, float %tmp104
  %tmp124 = select i1 false, float %tmp111, float %tmp103
  %tmp125 = select i1 false, float %tmp110, float %tmp102
  %tmp126 = insertvalue %struct.spam %tmp66, float 0.000000e+00, 3
  %tmp127 = insertvalue %struct.spam %tmp68, float 0.000000e+00, 3
  %tmp128 = fadd fast float 0.000000e+00, %tmp106
  %tmp129 = insertvalue %struct.spam %tmp70, float 0.000000e+00, 3
  br label %bb130

bb130:                                            ; preds = %bb99
  br label %bb131

bb131:                                            ; preds = %bb130
  %tmp132 = insertvalue %struct.spam %tmp72, float %tmp107, 3
  %tmp133 = insertvalue %struct.spam %tmp74, float 0.000000e+00, 3
  %tmp134 = insertvalue %struct.spam %tmp76, float 0.000000e+00, 3
  %tmp135 = insertvalue %struct.spam %tmp78, float 0.000000e+00, 3
  br label %bb136

bb136:                                            ; preds = %bb131
  br label %bb137

bb137:                                            ; preds = %bb136
  %tmp138 = insertvalue %struct.spam %tmp80, float 0.000000e+00, 3
  %tmp139 = insertvalue %struct.spam %tmp81, float 0.000000e+00, 3
  %tmp140 = insertvalue %struct.spam %tmp82, float 0.000000e+00, 3
  %tmp141 = insertvalue %struct.spam %tmp83, float 0.000000e+00, 3
  br label %bb142

bb142:                                            ; preds = %bb137
  br label %bb143

bb143:                                            ; preds = %bb142
  %tmp144 = insertvalue %struct.spam %tmp84, float 0.000000e+00, 3
  %tmp145 = insertvalue %struct.spam %tmp85, float 0.000000e+00, 3
  %tmp146 = insertvalue %struct.spam %tmp86, float 0.000000e+00, 3
  %tmp147 = insertvalue %struct.spam %tmp87, float 0.000000e+00, 3
  br label %bb148

bb148:                                            ; preds = %bb143
  br label %bb149

bb149:                                            ; preds = %bb148
  %tmp150 = insertvalue %struct.spam %tmp88, float 0.000000e+00, 3
  %tmp151 = insertvalue %struct.spam %tmp89, float 0.000000e+00, 3
  %tmp152 = insertvalue %struct.spam %tmp90, float 0.000000e+00, 3
  %tmp153 = insertvalue %struct.spam %tmp91, float 0.000000e+00, 3
  br label %bb154

bb154:                                            ; preds = %bb149
  br label %bb155

bb155:                                            ; preds = %bb154
  %tmp156 = insertvalue %struct.spam %tmp92, float 0.000000e+00, 3
  %tmp157 = insertvalue %struct.spam %tmp93, float 0.000000e+00, 3
  %tmp158 = insertvalue %struct.spam %tmp94, float 0.000000e+00, 3
  %tmp159 = insertvalue %struct.spam %tmp95, float 0.000000e+00, 3
  br label %bb160

bb160:                                            ; preds = %bb155
  br label %bb161

bb161:                                            ; preds = %bb160
  %tmp162 = insertvalue %struct.spam %tmp96, float 0.000000e+00, 3
  %tmp163 = insertvalue %struct.spam %tmp97, float 0.000000e+00, 3
  %tmp164 = insertvalue %struct.spam %tmp98, float 0.000000e+00, 3
  %tmp165 = icmp slt i32 %tmp101, 0
  br i1 false, label %bb166, label %bb167

bb166:                                            ; preds = %bb161
  br label %bb99

bb167:                                            ; preds = %bb161
  br label %bb168

bb168:                                            ; preds = %bb167
  br label %bb169

bb169:                                            ; preds = %bb168
  %tmp170 = fadd fast float %tmp57, 0.000000e+00
  %tmp171 = insertvalue %struct.spam %tmp63, float 0.000000e+00, 2
  br label %bb173

bb172:                                            ; preds = %bb198
  br label %bb199

bb173:                                            ; preds = %bb169
  br label %bb174

bb174:                                            ; preds = %bb173
  %tmp175 = insertvalue %struct.spam %tmp65, float 0.000000e+00, 2
  br label %bb176

bb176:                                            ; preds = %bb174
  br label %bb177

bb177:                                            ; preds = %bb176
  %tmp178 = insertvalue %struct.spam %tmp67, float 0.000000e+00, 2
  br label %bb179

bb179:                                            ; preds = %bb177
  br label %bb180

bb180:                                            ; preds = %bb179
  %tmp181 = insertvalue %struct.spam %tmp69, float 0.000000e+00, 2
  br label %bb182

bb182:                                            ; preds = %bb180
  br label %bb183

bb183:                                            ; preds = %bb182
  %tmp184 = insertvalue %struct.spam %tmp71, float 0.000000e+00, 2
  br label %bb185

bb185:                                            ; preds = %bb183
  br label %bb186

bb186:                                            ; preds = %bb185
  %tmp187 = insertvalue %struct.spam %tmp73, float 0.000000e+00, 2
  br label %bb188

bb188:                                            ; preds = %bb186
  br label %bb189

bb189:                                            ; preds = %bb188
  %tmp190 = insertvalue %struct.spam %tmp75, float 0.000000e+00, 2
  br label %bb191

bb191:                                            ; preds = %bb189
  br label %bb192

bb192:                                            ; preds = %bb191
  %tmp193 = insertvalue %struct.spam %tmp77, float 0.000000e+00, 2
  br label %bb194

bb194:                                            ; preds = %bb192
  br label %bb195

bb195:                                            ; preds = %bb194
  %tmp196 = insertvalue %struct.spam %tmp79, float 0.000000e+00, 2
  br label %bb197

bb197:                                            ; preds = %bb195
  br label %bb198

bb198:                                            ; preds = %bb197
  br label %bb172

bb199:                                            ; preds = %bb172
  br i1 %tmp41, label %bb200, label %bb201

bb200:                                            ; preds = %bb199
  br label %bb216

bb201:                                            ; preds = %bb199
  %tmp202 = fcmp oge float %tmp27, %tmp46
  br i1 false, label %bb203, label %bb207

bb203:                                            ; preds = %bb201
  %tmp204 = fadd fast float 0.000000e+00, %tmp50
  %tmp205 = fmul fast float %tmp49, %tmp21
  %tmp206 = fmul fast float %tmp47, %tmp28
  br label %bb212

bb207:                                            ; preds = %bb201
  br label %bb208

bb208:                                            ; preds = %bb207
  %tmp209 = fmul fast float %tmp36, %tmp52
  %tmp210 = fmul fast float 0.000000e+00, %tmp44
  %tmp211 = fadd fast float 0.000000e+00, %tmp48
  br label %bb212

bb212:                                            ; preds = %bb208, %bb203
  %tmp213 = add nuw nsw i32 %tmp51, 0
  %tmp214 = fcmp oge float %tmp42, 0.000000e+00
  br label %bb215

bb215:                                            ; preds = %bb212
  br label %bb45

bb216:                                            ; preds = %bb200
  br label %bb217

bb217:                                            ; preds = %bb216
  br label %bb218

bb218:                                            ; preds = %bb217
  %tmp219 = lshr i32 %arg6, 0
  %tmp220 = and i32 %tmp10, 0
  br label %bb221

bb221:                                            ; preds = %bb218
  br label %bb222

bb222:                                            ; preds = %bb221
  br label %bb223

bb223:                                            ; preds = %bb222
  br label %bb224

bb224:                                            ; preds = %bb223
  br label %bb225

bb225:                                            ; preds = %bb224
  %tmp226 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %arg7, i32 %arg5, i32 0, i32 0)
  br label %bb227

bb227:                                            ; preds = %bb225
  br label %bb228

bb228:                                            ; preds = %bb227
  br label %bb229

bb229:                                            ; preds = %bb228
  br label %bb230

bb230:                                            ; preds = %bb229
  br label %bb231

bb231:                                            ; preds = %bb230
  %tmp232 = add nsw i32 %arg4, 0
  call void @llvm.lifetime.end.p0i8(i64 0, i8* %tmp13)
  ret i32 0
}

declare spir_func void @wibble(%struct.spam.30 addrspace(1)*, %struct.ham addrspace(4)*, %struct.quux.31 addrspace(4)*, float*, i32, i32, i8 addrspace(2)*, i8 addrspace(1)*)

declare spir_func void @ham(%struct.spam.30 addrspace(1)*, %struct.ham addrspace(4)*, %struct.quux.31*, float*, %struct.hoge.8, i8 addrspace(2)*, i8 addrspace(1)*)

declare spir_func void @snork(%struct.spam.30 addrspace(4)*, %struct.ham addrspace(4)*, %struct.quux.31 addrspace(4)*, float*, %struct.hoge*, %struct.hoge.8, i8 addrspace(2)*, i8 addrspace(1)*)

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.minnum.f32(float, float) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.floor.f32(float) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.ceil.f32(float) #1

declare i32 @llvm.genx.GenISA.simdSize()

declare float @llvm.genx.GenISA.fsat.f32(float)

declare i32 @llvm.genx.GenISA.bfrev.i32(i32)

declare { i32, i32 } @llvm.genx.GenISA.add.pair(i32, i32, i32, i32)

declare i8* @llvm.genx.GenISA.pair.to.ptr.p0i8(i32, i32)

declare %struct.quux.31* @llvm.genx.GenISA.pair.to.ptr.p0struct.ShaderData(i32, i32)

declare float* @llvm.genx.GenISA.pair.to.ptr.p0f32(i32, i32)

declare i32* @llvm.genx.GenISA.pair.to.ptr.p0i32(i32, i32)

declare <3 x i32>* @llvm.genx.GenISA.pair.to.ptr.p0v3i32(i32, i32)

declare <3 x float>* @llvm.genx.GenISA.pair.to.ptr.p0v3f32(i32, i32)

declare %struct.hoge* @llvm.genx.GenISA.pair.to.ptr.p0struct.packed_float3(i32, i32)

attributes #0 = { argmemonly nofree nosync nounwind willreturn }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
