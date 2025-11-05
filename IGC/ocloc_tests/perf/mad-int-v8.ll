;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported, llvm-14-plus
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -file %t.bc -llvm_input -options "-igc_opts 'EnableOpaquePointersBackend=1,VISAOptions=-asmToConsole'" -device pvc | FileCheck %s --check-prefix=CHECK

; simple kernel with a large number of multiply-add
; This test checks that there are less than 200 BankConflicts on the kernel

; CHECK: .BankConflicts: {{([0-9]|[1-9][0-9]|1[0-9][0-9])$}}


; Function Attrs: nounwind
define spir_kernel void @foo(i32 addrspace(1)* %input_value, i32 addrspace(1)* %output) #0  {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %vecinit = insertelement <16 x i32> undef, i32 %0, i32 0
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %add = add i32 %1, 1
  %vecinit2 = insertelement <16 x i32> %vecinit, i32 %add, i32 1
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %2 = load i32, i32 addrspace(1)* %arrayidx3, align 4
  %add4 = add i32 %2, 2
  %vecinit5 = insertelement <16 x i32> %vecinit2, i32 %add4, i32 2
  %arrayidx6 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %3 = load i32, i32 addrspace(1)* %arrayidx6, align 4
  %add7 = add i32 %3, 3
  %vecinit8 = insertelement <16 x i32> %vecinit5, i32 %add7, i32 3
  %arrayidx9 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %4 = load i32, i32 addrspace(1)* %arrayidx9, align 4
  %add10 = add i32 %4, 4
  %vecinit11 = insertelement <16 x i32> %vecinit8, i32 %add10, i32 4
  %arrayidx12 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %5 = load i32, i32 addrspace(1)* %arrayidx12, align 4
  %add13 = add i32 %5, 5
  %vecinit14 = insertelement <16 x i32> %vecinit11, i32 %add13, i32 5
  %arrayidx15 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %6 = load i32, i32 addrspace(1)* %arrayidx15, align 4
  %add16 = add i32 %6, 6
  %vecinit17 = insertelement <16 x i32> %vecinit14, i32 %add16, i32 6
  %arrayidx18 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %7 = load i32, i32 addrspace(1)* %arrayidx18, align 4
  %add19 = add i32 %7, 7
  %vecinit20 = insertelement <16 x i32> %vecinit17, i32 %add19, i32 7
  %arrayidx21 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %8 = load i32, i32 addrspace(1)* %arrayidx21, align 4
  %add22 = add i32 %8, 8
  %vecinit23 = insertelement <16 x i32> %vecinit20, i32 %add22, i32 8
  %arrayidx24 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %9 = load i32, i32 addrspace(1)* %arrayidx24, align 4
  %add25 = add i32 %9, 9
  %vecinit26 = insertelement <16 x i32> %vecinit23, i32 %add25, i32 9
  %arrayidx27 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %10 = load i32, i32 addrspace(1)* %arrayidx27, align 4
  %add28 = add i32 %10, 10
  %vecinit29 = insertelement <16 x i32> %vecinit26, i32 %add28, i32 10
  %arrayidx30 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %11 = load i32, i32 addrspace(1)* %arrayidx30, align 4
  %add31 = add i32 %11, 11
  %vecinit32 = insertelement <16 x i32> %vecinit29, i32 %add31, i32 11
  %arrayidx33 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %12 = load i32, i32 addrspace(1)* %arrayidx33, align 4
  %add34 = add i32 %12, 12
  %vecinit35 = insertelement <16 x i32> %vecinit32, i32 %add34, i32 12
  %arrayidx36 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %13 = load i32, i32 addrspace(1)* %arrayidx36, align 4
  %add37 = add i32 %13, 13
  %vecinit38 = insertelement <16 x i32> %vecinit35, i32 %add37, i32 13
  %arrayidx39 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %14 = load i32, i32 addrspace(1)* %arrayidx39, align 4
  %add40 = add i32 %14, 14
  %vecinit41 = insertelement <16 x i32> %vecinit38, i32 %add40, i32 14
  %arrayidx42 = getelementptr inbounds i32, i32 addrspace(1)* %input_value, i64 0
  %15 = load i32, i32 addrspace(1)* %arrayidx42, align 4
  %add43 = add i32 %15, 15
  %vecinit44 = insertelement <16 x i32> %vecinit41, i32 %add43, i32 15
  %16 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0) #1
  %17 = insertelement <3 x i64> undef, i64 %16, i32 0
  %18 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 1) #1
  %19 = insertelement <3 x i64> %17, i64 %18, i32 1
  %20 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 2) #1
  %21 = insertelement <3 x i64> %19, i64 %20, i32 2
  %call = extractelement <3 x i64> %21, i32 0
  %conv = trunc i64 %call to i32
  %splat.splatinsert = insertelement <16 x i32> undef, i32 %conv, i32 0
  %splat.splat = shufflevector <16 x i32> %splat.splatinsert, <16 x i32> undef, <16 x i32> zeroinitializer
  %mul = mul <16 x i32> %splat.splat, %vecinit44
  %add45 = add <16 x i32> %mul, %splat.splat
  %mul46 = mul <16 x i32> %add45, %splat.splat
  %add47 = add <16 x i32> %mul46, %add45
  %mul48 = mul <16 x i32> %add47, %add45
  %add49 = add <16 x i32> %mul48, %add47
  %mul50 = mul <16 x i32> %add49, %add47
  %add51 = add <16 x i32> %mul50, %add49
  %mul52 = mul <16 x i32> %add51, %add49
  %add53 = add <16 x i32> %mul52, %add51
  %mul54 = mul <16 x i32> %add53, %add51
  %add55 = add <16 x i32> %mul54, %add53
  %mul56 = mul <16 x i32> %add55, %add53
  %add57 = add <16 x i32> %mul56, %add55
  %mul58 = mul <16 x i32> %add57, %add55
  %add59 = add <16 x i32> %mul58, %add57
  %mul60 = mul <16 x i32> %add59, %add57
  %add61 = add <16 x i32> %mul60, %add59
  %mul62 = mul <16 x i32> %add61, %add59
  %add63 = add <16 x i32> %mul62, %add61
  %mul64 = mul <16 x i32> %add63, %add61
  %add65 = add <16 x i32> %mul64, %add63
  %mul66 = mul <16 x i32> %add65, %add63
  %add67 = add <16 x i32> %mul66, %add65
  %mul68 = mul <16 x i32> %add67, %add65
  %add69 = add <16 x i32> %mul68, %add67
  %mul70 = mul <16 x i32> %add69, %add67
  %add71 = add <16 x i32> %mul70, %add69
  %mul72 = mul <16 x i32> %add71, %add69
  %add73 = add <16 x i32> %mul72, %add71
  %mul74 = mul <16 x i32> %add73, %add71
  %add75 = add <16 x i32> %mul74, %add73
  %mul76 = mul <16 x i32> %add75, %add73
  %add77 = add <16 x i32> %mul76, %add75
  %mul78 = mul <16 x i32> %add77, %add75
  %add79 = add <16 x i32> %mul78, %add77
  %mul80 = mul <16 x i32> %add79, %add77
  %add81 = add <16 x i32> %mul80, %add79
  %mul82 = mul <16 x i32> %add81, %add79
  %add83 = add <16 x i32> %mul82, %add81
  %mul84 = mul <16 x i32> %add83, %add81
  %add85 = add <16 x i32> %mul84, %add83
  %mul86 = mul <16 x i32> %add85, %add83
  %add87 = add <16 x i32> %mul86, %add85
  %mul88 = mul <16 x i32> %add87, %add85
  %add89 = add <16 x i32> %mul88, %add87
  %mul90 = mul <16 x i32> %add89, %add87
  %add91 = add <16 x i32> %mul90, %add89
  %mul92 = mul <16 x i32> %add91, %add89
  %add93 = add <16 x i32> %mul92, %add91
  %mul94 = mul <16 x i32> %add93, %add91
  %add95 = add <16 x i32> %mul94, %add93
  %mul96 = mul <16 x i32> %add95, %add93
  %add97 = add <16 x i32> %mul96, %add95
  %mul98 = mul <16 x i32> %add97, %add95
  %add99 = add <16 x i32> %mul98, %add97
  %mul100 = mul <16 x i32> %add99, %add97
  %add101 = add <16 x i32> %mul100, %add99
  %mul102 = mul <16 x i32> %add101, %add99
  %add103 = add <16 x i32> %mul102, %add101
  %mul104 = mul <16 x i32> %add103, %add101
  %add105 = add <16 x i32> %mul104, %add103
  %mul106 = mul <16 x i32> %add105, %add103
  %add107 = add <16 x i32> %mul106, %add105
  %mul108 = mul <16 x i32> %add107, %add105
  %add109 = add <16 x i32> %mul108, %add107
  %mul110 = mul <16 x i32> %add109, %add107
  %add111 = add <16 x i32> %mul110, %add109
  %mul112 = mul <16 x i32> %add111, %add109
  %add113 = add <16 x i32> %mul112, %add111
  %mul114 = mul <16 x i32> %add113, %add111
  %add115 = add <16 x i32> %mul114, %add113
  %mul116 = mul <16 x i32> %add115, %add113
  %add117 = add <16 x i32> %mul116, %add115
  %mul118 = mul <16 x i32> %add117, %add115
  %add119 = add <16 x i32> %mul118, %add117
  %mul120 = mul <16 x i32> %add119, %add117
  %add121 = add <16 x i32> %mul120, %add119
  %mul122 = mul <16 x i32> %add121, %add119
  %add123 = add <16 x i32> %mul122, %add121
  %mul124 = mul <16 x i32> %add123, %add121
  %add125 = add <16 x i32> %mul124, %add123
  %mul126 = mul <16 x i32> %add125, %add123
  %add127 = add <16 x i32> %mul126, %add125
  %mul128 = mul <16 x i32> %add127, %add125
  %add129 = add <16 x i32> %mul128, %add127
  %mul130 = mul <16 x i32> %add129, %add127
  %add131 = add <16 x i32> %mul130, %add129
  %mul132 = mul <16 x i32> %add131, %add129
  %add133 = add <16 x i32> %mul132, %add131
  %mul134 = mul <16 x i32> %add133, %add131
  %add135 = add <16 x i32> %mul134, %add133
  %mul136 = mul <16 x i32> %add135, %add133
  %add137 = add <16 x i32> %mul136, %add135
  %mul138 = mul <16 x i32> %add137, %add135
  %add139 = add <16 x i32> %mul138, %add137
  %mul140 = mul <16 x i32> %add139, %add137
  %add141 = add <16 x i32> %mul140, %add139
  %mul142 = mul <16 x i32> %add141, %add139
  %add143 = add <16 x i32> %mul142, %add141
  %mul144 = mul <16 x i32> %add143, %add141
  %add145 = add <16 x i32> %mul144, %add143
  %mul146 = mul <16 x i32> %add145, %add143
  %add147 = add <16 x i32> %mul146, %add145
  %mul148 = mul <16 x i32> %add147, %add145
  %add149 = add <16 x i32> %mul148, %add147
  %mul150 = mul <16 x i32> %add149, %add147
  %add151 = add <16 x i32> %mul150, %add149
  %mul152 = mul <16 x i32> %add151, %add149
  %add153 = add <16 x i32> %mul152, %add151
  %mul154 = mul <16 x i32> %add153, %add151
  %add155 = add <16 x i32> %mul154, %add153
  %mul156 = mul <16 x i32> %add155, %add153
  %add157 = add <16 x i32> %mul156, %add155
  %mul158 = mul <16 x i32> %add157, %add155
  %add159 = add <16 x i32> %mul158, %add157
  %mul160 = mul <16 x i32> %add159, %add157
  %add161 = add <16 x i32> %mul160, %add159
  %mul162 = mul <16 x i32> %add161, %add159
  %add163 = add <16 x i32> %mul162, %add161
  %mul164 = mul <16 x i32> %add163, %add161
  %add165 = add <16 x i32> %mul164, %add163
  %mul166 = mul <16 x i32> %add165, %add163
  %add167 = add <16 x i32> %mul166, %add165
  %mul168 = mul <16 x i32> %add167, %add165
  %add169 = add <16 x i32> %mul168, %add167
  %mul170 = mul <16 x i32> %add169, %add167
  %add171 = add <16 x i32> %mul170, %add169
  %22 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 0, i32 1>
  %23 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 2, i32 3>
  %add172 = add <2 x i32> %22, %23
  %24 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 4, i32 5>
  %add173 = add <2 x i32> %add172, %24
  %25 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 6, i32 7>
  %add174 = add <2 x i32> %add173, %25
  %26 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 8, i32 9>
  %add175 = add <2 x i32> %add174, %26
  %27 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 10, i32 11>
  %add176 = add <2 x i32> %add175, %27
  %28 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 12, i32 13>
  %add177 = add <2 x i32> %add176, %28
  %29 = shufflevector <16 x i32> %add171, <16 x i32> undef, <2 x i32> <i32 14, i32 15>
  %add178 = add <2 x i32> %add177, %29
  %30 = extractelement <2 x i32> %add178, i32 0
  %31 = extractelement <2 x i32> %add178, i32 1
  %add179 = add i32 %30, %31
  %32 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0) #1
  %33 = insertelement <3 x i64> undef, i64 %32, i32 0
  %34 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1) #1
  %35 = insertelement <3 x i64> %33, i64 %34, i32 1
  %36 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2) #1
  %37 = insertelement <3 x i64> %35, i64 %36, i32 2
  %call180 = extractelement <3 x i64> %37, i32 0
  %arrayidx181 = getelementptr inbounds i32, i32 addrspace(1)* %output, i64 %call180
  store i32 %add179, i32 addrspace(1)* %arrayidx181, align 4
  ret void
}

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32) #1

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32) #1

attributes #0 = { nounwind }
