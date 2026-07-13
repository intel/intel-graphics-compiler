;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; An OpenCL kernel whose SIMD32 register pressure sits right at the 128-GRF
; boundary: its first RA pass at 128 GRF spills a tiny amount. The vISA
; GRF-selection spill threshold (-spillAllowed, default 256B, enabled on PVC+)
; must tolerate that spill and keep the kernel at 128 GRF (8 threads/EU) instead
; of bumping to 256 GRF (4 threads/EU) and halving occupancy.

; REQUIRES: regkeys, pvc-supported, opaque-pointers

; Fix: default spill threshold keeps the kernel at 128 GRF.
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -internal_options "-cl-intel-enable-auto-large-GRF-mode" -options "-igc_opts 'DumpASMToConsole=1'" 2>&1 | FileCheck %s

; Negative control: with the threshold disabled (VISASpillAllowed=1) the same kernel bumps to 256 GRF.
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -internal_options "-cl-intel-enable-auto-large-GRF-mode" -options "-igc_opts 'DumpASMToConsole=1,VISASpillAllowed=1'" 2>&1 | FileCheck %s --check-prefix=NOFIX

; CHECK: numGRF=128
; NOFIX: numGRF=256

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

%struct.0 = type { ptr addrspace(4), i64, i64, i64, i64, i64, [3 x %struct.2] }
%struct.2 = type { i64, i64, i64 }
%struct.1 = type { ptr addrspace(4), i64, i64, i64, i64, i64, [1 x %struct.2] }

; Function Attrs: nounwind
declare spir_func i64 @_Z13get_global_idj(i32) #0

; Function Attrs: alwaysinline nounwind
define spir_func void @__itt_offload_wi_start_wrapper() #1 {
  %1 = alloca [3 x i64], align 8
  br i1 true, label %29, label %2

2:                                                ; preds = %0
  %3 = getelementptr inbounds [3 x i64], ptr %1, i64 0, i64 0
  %4 = addrspacecast ptr %3 to ptr addrspace(4)
  %5 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 0) #4
  %6 = insertelement <3 x i64> undef, i64 %5, i32 0
  %7 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 1) #4
  %8 = insertelement <3 x i64> %6, i64 %7, i32 1
  %9 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 2) #4
  %10 = insertelement <3 x i64> %8, i64 %9, i32 2
  %11 = extractelement <3 x i64> %10, i32 0
  store i64 %11, ptr %3, align 8
  %12 = getelementptr inbounds [3 x i64], ptr %1, i64 0, i64 1
  %13 = extractelement <3 x i64> %10, i32 1
  store i64 %13, ptr %12, align 8
  %14 = getelementptr inbounds [3 x i64], ptr %1, i64 0, i64 2
  %15 = extractelement <3 x i64> %10, i32 2
  store i64 %15, ptr %14, align 8
  %16 = call spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv() #4
  %17 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 0) #4
  %18 = insertelement <3 x i64> undef, i64 %17, i32 0
  %19 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 1) #4
  %20 = insertelement <3 x i64> %18, i64 %19, i32 1
  %21 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 2) #4
  %22 = insertelement <3 x i64> %20, i64 %21, i32 2
  %23 = extractelement <3 x i64> %22, i32 0
  %24 = extractelement <3 x i64> %22, i32 1
  %25 = mul i64 %23, %24
  %26 = extractelement <3 x i64> %22, i32 2
  %27 = mul i64 %25, %26
  %28 = trunc i64 %27 to i32
  call spir_func void @__itt_offload_wi_start_stub(ptr addrspace(4) %4, i64 %16, i32 %28) #2
  br label %29

29:                                               ; preds = %2, %0
  ret void
}

; Function Attrs: noinline nounwind optnone
declare spir_func void @__itt_offload_wi_start_stub(ptr addrspace(4), i64, i32) #2

; Function Attrs: alwaysinline nounwind
define spir_func void @__itt_offload_wi_finish_wrapper() #1 {
  %1 = alloca [3 x i64], align 8
  br i1 true, label %17, label %2

2:                                                ; preds = %0
  %3 = getelementptr inbounds [3 x i64], ptr %1, i64 0, i64 0
  %4 = addrspacecast ptr %3 to ptr addrspace(4)
  %5 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 0) #4
  %6 = insertelement <3 x i64> undef, i64 %5, i32 0
  %7 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 1) #4
  %8 = insertelement <3 x i64> %6, i64 %7, i32 1
  %9 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 2) #4
  %10 = insertelement <3 x i64> %8, i64 %9, i32 2
  %11 = extractelement <3 x i64> %10, i32 0
  store i64 %11, ptr %3, align 8
  %12 = getelementptr inbounds [3 x i64], ptr %1, i64 0, i64 1
  %13 = extractelement <3 x i64> %10, i32 1
  store i64 %13, ptr %12, align 8
  %14 = getelementptr inbounds [3 x i64], ptr %1, i64 0, i64 2
  %15 = extractelement <3 x i64> %10, i32 2
  store i64 %15, ptr %14, align 8
  %16 = call spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv() #4
  call spir_func void @__itt_offload_wi_finish_stub(ptr addrspace(4) %4, i64 %16) #2
  br label %17

17:                                               ; preds = %2, %0
  ret void
}

; Function Attrs: noinline nounwind optnone
declare spir_func void @__itt_offload_wi_finish_stub(ptr addrspace(4), i64) #2

; Function Attrs: noinline nounwind
define spir_kernel void @spill_threshold_kernel(ptr addrspace(1) %0, ptr addrspace(1) %1, ptr addrspace(1) %2, ptr addrspace(1) %3, ptr addrspace(1) %4, ptr addrspace(1) %5, ptr addrspace(1) %6, ptr addrspace(1) %7, ptr addrspace(1) %8, ptr addrspace(1) %9, ptr addrspace(1) %10, ptr addrspace(1) %11, ptr addrspace(1) %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24) #3 {
  call spir_func void @__itt_offload_wi_start_wrapper() #1
  %26 = trunc i64 %18 to i32
  %27 = trunc i64 %17 to i32
  %28 = trunc i64 %16 to i32
  %29 = trunc i64 %15 to i32
  %30 = trunc i64 %14 to i32
  %31 = trunc i64 %13 to i32
  %32 = icmp sgt i32 %26, %27
  br i1 %32, label %33, label %34

33:                                               ; preds = %50, %40, %34, %25
  call spir_func void @__itt_offload_wi_finish_wrapper() #1
  ret void

34:                                               ; preds = %25
  %35 = call spir_func i64 @_Z13get_global_idj(i32 0) #0
  %36 = call spir_func i64 @_Z13get_global_idj(i32 1) #0
  %37 = call spir_func i64 @_Z13get_global_idj(i32 2) #0
  %38 = trunc i64 %37 to i32
  %39 = icmp slt i32 %27, %38
  br i1 %39, label %33, label %40

40:                                               ; preds = %34
  %41 = icmp sgt i32 %28, %29
  %42 = trunc i64 %36 to i32
  %43 = icmp sgt i32 %30, %31
  %44 = trunc i64 %35 to i32
  %45 = icmp slt i32 %31, %44
  %46 = icmp slt i32 %29, %42
  %47 = select i1 %41, i1 true, i1 %46
  %48 = select i1 %47, i1 true, i1 %43
  %49 = select i1 %48, i1 true, i1 %45
  br i1 %49, label %33, label %50

50:                                               ; preds = %40
  %51 = getelementptr inbounds %struct.0, ptr addrspace(1) %12, i64 0, i32 6, i64 2, i32 2
  %52 = getelementptr inbounds %struct.0, ptr addrspace(1) %12, i64 0, i32 6, i64 2, i32 1
  %53 = getelementptr inbounds %struct.0, ptr addrspace(1) %12, i64 0, i32 6, i64 1, i32 2
  %54 = getelementptr inbounds %struct.0, ptr addrspace(1) %12, i64 0, i32 6, i64 1, i32 1
  %55 = getelementptr inbounds %struct.0, ptr addrspace(1) %12, i64 0, i32 6, i64 0, i32 2
  %56 = getelementptr inbounds %struct.1, ptr addrspace(1) %11, i64 0, i32 6, i64 0, i32 2
  %57 = getelementptr inbounds %struct.0, ptr addrspace(1) %10, i64 0, i32 6, i64 2, i32 2
  %58 = getelementptr inbounds %struct.0, ptr addrspace(1) %10, i64 0, i32 6, i64 2, i32 1
  %59 = getelementptr inbounds %struct.0, ptr addrspace(1) %10, i64 0, i32 6, i64 1, i32 2
  %60 = getelementptr inbounds %struct.0, ptr addrspace(1) %10, i64 0, i32 6, i64 1, i32 1
  %61 = getelementptr inbounds %struct.0, ptr addrspace(1) %10, i64 0, i32 6, i64 0, i32 2
  %62 = getelementptr inbounds %struct.1, ptr addrspace(1) %9, i64 0, i32 6, i64 0, i32 2
  %63 = getelementptr inbounds %struct.0, ptr addrspace(1) %8, i64 0, i32 6, i64 2, i32 2
  %64 = getelementptr inbounds %struct.0, ptr addrspace(1) %8, i64 0, i32 6, i64 2, i32 1
  %65 = getelementptr inbounds %struct.0, ptr addrspace(1) %8, i64 0, i32 6, i64 1, i32 2
  %66 = getelementptr inbounds %struct.0, ptr addrspace(1) %8, i64 0, i32 6, i64 1, i32 1
  %67 = getelementptr inbounds %struct.0, ptr addrspace(1) %8, i64 0, i32 6, i64 0, i32 2
  %68 = getelementptr inbounds %struct.1, ptr addrspace(1) %7, i64 0, i32 6, i64 0, i32 2
  %69 = getelementptr inbounds %struct.0, ptr addrspace(1) %6, i64 0, i32 6, i64 2, i32 2
  %70 = getelementptr inbounds %struct.0, ptr addrspace(1) %6, i64 0, i32 6, i64 2, i32 1
  %71 = getelementptr inbounds %struct.0, ptr addrspace(1) %6, i64 0, i32 6, i64 1, i32 2
  %72 = getelementptr inbounds %struct.0, ptr addrspace(1) %6, i64 0, i32 6, i64 1, i32 1
  %73 = getelementptr inbounds %struct.0, ptr addrspace(1) %6, i64 0, i32 6, i64 0, i32 2
  %74 = getelementptr inbounds %struct.0, ptr addrspace(1) %5, i64 0, i32 6, i64 2, i32 2
  %75 = getelementptr inbounds %struct.0, ptr addrspace(1) %5, i64 0, i32 6, i64 2, i32 1
  %76 = getelementptr inbounds %struct.0, ptr addrspace(1) %5, i64 0, i32 6, i64 1, i32 2
  %77 = getelementptr inbounds %struct.0, ptr addrspace(1) %5, i64 0, i32 6, i64 1, i32 1
  %78 = getelementptr inbounds %struct.0, ptr addrspace(1) %5, i64 0, i32 6, i64 0, i32 2
  %79 = getelementptr inbounds %struct.0, ptr addrspace(1) %4, i64 0, i32 6, i64 2, i32 2
  %80 = getelementptr inbounds %struct.0, ptr addrspace(1) %4, i64 0, i32 6, i64 2, i32 1
  %81 = getelementptr inbounds %struct.0, ptr addrspace(1) %4, i64 0, i32 6, i64 1, i32 2
  %82 = getelementptr inbounds %struct.0, ptr addrspace(1) %4, i64 0, i32 6, i64 1, i32 1
  %83 = getelementptr inbounds %struct.0, ptr addrspace(1) %4, i64 0, i32 6, i64 0, i32 2
  %84 = getelementptr inbounds %struct.0, ptr addrspace(1) %3, i64 0, i32 6, i64 2, i32 2
  %85 = getelementptr inbounds %struct.0, ptr addrspace(1) %3, i64 0, i32 6, i64 2, i32 1
  %86 = getelementptr inbounds %struct.0, ptr addrspace(1) %3, i64 0, i32 6, i64 1, i32 2
  %87 = getelementptr inbounds %struct.0, ptr addrspace(1) %3, i64 0, i32 6, i64 1, i32 1
  %88 = getelementptr inbounds %struct.0, ptr addrspace(1) %3, i64 0, i32 6, i64 0, i32 2
  %89 = getelementptr inbounds %struct.0, ptr addrspace(1) %2, i64 0, i32 6, i64 2, i32 2
  %90 = getelementptr inbounds %struct.0, ptr addrspace(1) %2, i64 0, i32 6, i64 2, i32 1
  %91 = getelementptr inbounds %struct.0, ptr addrspace(1) %2, i64 0, i32 6, i64 1, i32 2
  %92 = getelementptr inbounds %struct.0, ptr addrspace(1) %2, i64 0, i32 6, i64 1, i32 1
  %93 = getelementptr inbounds %struct.0, ptr addrspace(1) %2, i64 0, i32 6, i64 0, i32 2
  %94 = getelementptr inbounds %struct.0, ptr addrspace(1) %1, i64 0, i32 6, i64 2, i32 2
  %95 = getelementptr inbounds %struct.0, ptr addrspace(1) %1, i64 0, i32 6, i64 2, i32 1
  %96 = getelementptr inbounds %struct.0, ptr addrspace(1) %1, i64 0, i32 6, i64 1, i32 2
  %97 = getelementptr inbounds %struct.0, ptr addrspace(1) %1, i64 0, i32 6, i64 1, i32 1
  %98 = getelementptr inbounds %struct.0, ptr addrspace(1) %1, i64 0, i32 6, i64 0, i32 2
  %99 = getelementptr inbounds %struct.0, ptr addrspace(1) %0, i64 0, i32 6, i64 2, i32 2
  %100 = getelementptr inbounds %struct.0, ptr addrspace(1) %0, i64 0, i32 6, i64 2, i32 1
  %101 = getelementptr inbounds %struct.0, ptr addrspace(1) %0, i64 0, i32 6, i64 1, i32 2
  %102 = getelementptr inbounds %struct.0, ptr addrspace(1) %0, i64 0, i32 6, i64 1, i32 1
  %103 = getelementptr inbounds %struct.0, ptr addrspace(1) %0, i64 0, i32 6, i64 0, i32 2
  %104 = bitcast ptr addrspace(1) %0 to ptr addrspace(1)
  %105 = load ptr addrspace(4), ptr addrspace(1) %104, align 8
  %106 = load i64, ptr addrspace(1) %103, align 1
  %107 = load i64, ptr addrspace(1) %102, align 1
  %108 = load i64, ptr addrspace(1) %101, align 1
  %109 = load i64, ptr addrspace(1) %100, align 1
  %110 = load i64, ptr addrspace(1) %99, align 1
  %111 = bitcast ptr addrspace(1) %1 to ptr addrspace(1)
  %112 = load ptr addrspace(4), ptr addrspace(1) %111, align 8
  %113 = load i64, ptr addrspace(1) %98, align 1
  %114 = load i64, ptr addrspace(1) %97, align 1
  %115 = load i64, ptr addrspace(1) %96, align 1
  %116 = load i64, ptr addrspace(1) %95, align 1
  %117 = load i64, ptr addrspace(1) %94, align 1
  %118 = bitcast ptr addrspace(1) %2 to ptr addrspace(1)
  %119 = load ptr addrspace(4), ptr addrspace(1) %118, align 8
  %120 = load i64, ptr addrspace(1) %93, align 1
  %121 = load i64, ptr addrspace(1) %92, align 1
  %122 = load i64, ptr addrspace(1) %91, align 1
  %123 = load i64, ptr addrspace(1) %90, align 1
  %124 = load i64, ptr addrspace(1) %89, align 1
  %125 = bitcast ptr addrspace(1) %3 to ptr addrspace(1)
  %126 = load ptr addrspace(4), ptr addrspace(1) %125, align 8
  %127 = load i64, ptr addrspace(1) %88, align 1
  %128 = load i64, ptr addrspace(1) %87, align 1
  %129 = load i64, ptr addrspace(1) %86, align 1
  %130 = load i64, ptr addrspace(1) %85, align 1
  %131 = load i64, ptr addrspace(1) %84, align 1
  %132 = bitcast ptr addrspace(1) %4 to ptr addrspace(1)
  %133 = load ptr addrspace(4), ptr addrspace(1) %132, align 8
  %134 = load i64, ptr addrspace(1) %83, align 1
  %135 = load i64, ptr addrspace(1) %82, align 1
  %136 = load i64, ptr addrspace(1) %81, align 1
  %137 = load i64, ptr addrspace(1) %80, align 1
  %138 = load i64, ptr addrspace(1) %79, align 1
  %139 = bitcast ptr addrspace(1) %5 to ptr addrspace(1)
  %140 = load ptr addrspace(4), ptr addrspace(1) %139, align 8
  %141 = load i64, ptr addrspace(1) %78, align 1
  %142 = load i64, ptr addrspace(1) %77, align 1
  %143 = load i64, ptr addrspace(1) %76, align 1
  %144 = load i64, ptr addrspace(1) %75, align 1
  %145 = load i64, ptr addrspace(1) %74, align 1
  %146 = bitcast ptr addrspace(1) %6 to ptr addrspace(1)
  %147 = load ptr addrspace(4), ptr addrspace(1) %146, align 8
  %148 = load i64, ptr addrspace(1) %73, align 1
  %149 = load i64, ptr addrspace(1) %72, align 1
  %150 = load i64, ptr addrspace(1) %71, align 1
  %151 = load i64, ptr addrspace(1) %70, align 1
  %152 = load i64, ptr addrspace(1) %69, align 1
  %153 = bitcast ptr addrspace(1) %7 to ptr addrspace(1)
  %154 = load ptr addrspace(4), ptr addrspace(1) %153, align 8
  %155 = load i64, ptr addrspace(1) %68, align 1
  %156 = bitcast ptr addrspace(1) %8 to ptr addrspace(1)
  %157 = load ptr addrspace(4), ptr addrspace(1) %156, align 8
  %158 = load i64, ptr addrspace(1) %67, align 1
  %159 = load i64, ptr addrspace(1) %66, align 1
  %160 = load i64, ptr addrspace(1) %65, align 1
  %161 = load i64, ptr addrspace(1) %64, align 1
  %162 = load i64, ptr addrspace(1) %63, align 1
  %163 = bitcast ptr addrspace(1) %9 to ptr addrspace(1)
  %164 = load ptr addrspace(4), ptr addrspace(1) %163, align 8
  %165 = load i64, ptr addrspace(1) %62, align 1
  %166 = bitcast ptr addrspace(1) %10 to ptr addrspace(1)
  %167 = load ptr addrspace(4), ptr addrspace(1) %166, align 8
  %168 = load i64, ptr addrspace(1) %61, align 1
  %169 = load i64, ptr addrspace(1) %60, align 1
  %170 = load i64, ptr addrspace(1) %59, align 1
  %171 = load i64, ptr addrspace(1) %58, align 1
  %172 = load i64, ptr addrspace(1) %57, align 1
  %173 = bitcast ptr addrspace(1) %11 to ptr addrspace(1)
  %174 = load ptr addrspace(4), ptr addrspace(1) %173, align 8
  %175 = load i64, ptr addrspace(1) %56, align 1
  %176 = bitcast ptr addrspace(1) %12 to ptr addrspace(1)
  %177 = load ptr addrspace(4), ptr addrspace(1) %176, align 8
  %178 = load i64, ptr addrspace(1) %55, align 1
  %179 = load i64, ptr addrspace(1) %54, align 1
  %180 = load i64, ptr addrspace(1) %53, align 1
  %181 = load i64, ptr addrspace(1) %52, align 1
  %182 = load i64, ptr addrspace(1) %51, align 1
  %183 = add nsw i32 %38, 1
  %184 = sext i32 %183 to i64
  %185 = add nsw i32 %38, 2
  %186 = sext i32 %185 to i64
  %187 = sub nsw i64 %184, %110
  %188 = mul nsw i64 %109, %187
  %189 = getelementptr inbounds i8, ptr addrspace(4) %105, i64 %188
  %190 = sub nsw i64 %184, %117
  %191 = mul nsw i64 %116, %190
  %192 = getelementptr inbounds i8, ptr addrspace(4) %112, i64 %191
  %193 = sub nsw i64 %184, %124
  %194 = mul nsw i64 %123, %193
  %195 = getelementptr inbounds i8, ptr addrspace(4) %119, i64 %194
  %196 = sub nsw i64 %184, %131
  %197 = mul nsw i64 %130, %196
  %198 = getelementptr inbounds i8, ptr addrspace(4) %126, i64 %197
  %199 = sub nsw i64 %186, %138
  %200 = mul nsw i64 %137, %199
  %201 = getelementptr inbounds i8, ptr addrspace(4) %133, i64 %200
  %202 = sub nsw i64 %184, %138
  %203 = mul nsw i64 %137, %202
  %204 = getelementptr inbounds i8, ptr addrspace(4) %133, i64 %203
  %205 = sub nsw i64 %186, %145
  %206 = mul nsw i64 %144, %205
  %207 = getelementptr inbounds i8, ptr addrspace(4) %140, i64 %206
  %208 = sub nsw i64 %184, %145
  %209 = mul nsw i64 %144, %208
  %210 = getelementptr inbounds i8, ptr addrspace(4) %140, i64 %209
  %211 = sub nsw i64 %186, %152
  %212 = mul nsw i64 %151, %211
  %213 = getelementptr inbounds i8, ptr addrspace(4) %147, i64 %212
  %214 = sub nsw i64 %184, %152
  %215 = mul nsw i64 %151, %214
  %216 = getelementptr inbounds i8, ptr addrspace(4) %147, i64 %215
  %217 = sub nsw i64 %184, %162
  %218 = mul nsw i64 %161, %217
  %219 = getelementptr inbounds i8, ptr addrspace(4) %157, i64 %218
  %220 = sub nsw i64 %184, %172
  %221 = mul nsw i64 %171, %220
  %222 = getelementptr inbounds i8, ptr addrspace(4) %167, i64 %221
  %223 = sub nsw i64 %184, %175
  %224 = getelementptr inbounds double, ptr addrspace(4) %174, i64 %223
  %225 = load double, ptr addrspace(4) %224, align 8
  %226 = sub nsw i64 %184, %182
  %227 = mul nsw i64 %181, %226
  %228 = getelementptr inbounds i8, ptr addrspace(4) %177, i64 %227
  %229 = fsub double 6.250000e+02, %225
  %230 = fadd double %225, 6.250000e+02
  %231 = add nsw i32 %42, 2
  %232 = sext i32 %231 to i64
  %233 = sub nsw i64 %232, %108
  %234 = mul nsw i64 %107, %233
  %235 = getelementptr inbounds i8, ptr addrspace(4) %189, i64 %234
  %236 = sub nsw i64 %232, %115
  %237 = mul nsw i64 %114, %236
  %238 = getelementptr inbounds i8, ptr addrspace(4) %192, i64 %237
  %239 = sub nsw i64 %232, %122
  %240 = mul nsw i64 %121, %239
  %241 = getelementptr inbounds i8, ptr addrspace(4) %195, i64 %240
  %242 = add nsw i32 %42, 1
  %243 = sext i32 %242 to i64
  %244 = sub nsw i64 %243, %122
  %245 = mul nsw i64 %121, %244
  %246 = getelementptr inbounds i8, ptr addrspace(4) %195, i64 %245
  %247 = sub nsw i64 %232, %129
  %248 = mul nsw i64 %128, %247
  %249 = getelementptr inbounds i8, ptr addrspace(4) %198, i64 %248
  %250 = sub nsw i64 %243, %129
  %251 = mul nsw i64 %128, %250
  %252 = getelementptr inbounds i8, ptr addrspace(4) %198, i64 %251
  %253 = sub nsw i64 %232, %136
  %254 = mul nsw i64 %135, %253
  %255 = getelementptr inbounds i8, ptr addrspace(4) %201, i64 %254
  %256 = getelementptr inbounds i8, ptr addrspace(4) %204, i64 %254
  %257 = sub nsw i64 %232, %143
  %258 = mul nsw i64 %142, %257
  %259 = getelementptr inbounds i8, ptr addrspace(4) %207, i64 %258
  %260 = getelementptr inbounds i8, ptr addrspace(4) %210, i64 %258
  %261 = sub nsw i64 %232, %150
  %262 = mul nsw i64 %149, %261
  %263 = getelementptr inbounds i8, ptr addrspace(4) %213, i64 %262
  %264 = getelementptr inbounds i8, ptr addrspace(4) %216, i64 %262
  %265 = sub nsw i64 %232, %160
  %266 = mul nsw i64 %159, %265
  %267 = getelementptr inbounds i8, ptr addrspace(4) %219, i64 %266
  %268 = sub nsw i64 %232, %165
  %269 = getelementptr inbounds double, ptr addrspace(4) %164, i64 %268
  %270 = load double, ptr addrspace(4) %269, align 8
  %271 = sub nsw i64 %232, %170
  %272 = mul nsw i64 %169, %271
  %273 = getelementptr inbounds i8, ptr addrspace(4) %222, i64 %272
  %274 = fsub double 6.250000e+02, %270
  %275 = fadd double %270, 6.250000e+02
  %276 = sub nsw i64 %232, %180
  %277 = mul nsw i64 %179, %276
  %278 = getelementptr inbounds i8, ptr addrspace(4) %228, i64 %277
  %279 = add nsw i32 %44, 1
  %280 = add nsw i32 %44, 2
  %281 = sext i32 %280 to i64
  %282 = sub nsw i64 %281, %106
  %283 = bitcast ptr addrspace(4) %235 to ptr addrspace(4)
  %284 = getelementptr inbounds double, ptr addrspace(4) %283, i64 %282
  %285 = load double, ptr addrspace(4) %284, align 8
  %286 = sext i32 %279 to i64
  %287 = sub nsw i64 %286, %106
  %288 = bitcast ptr addrspace(4) %235 to ptr addrspace(4)
  %289 = getelementptr inbounds double, ptr addrspace(4) %288, i64 %287
  %290 = load double, ptr addrspace(4) %289, align 8
  %291 = fsub double %285, %290
  %292 = fdiv double %291, 1.000000e+01
  %293 = sub nsw i64 %281, %113
  %294 = bitcast ptr addrspace(4) %238 to ptr addrspace(4)
  %295 = getelementptr inbounds double, ptr addrspace(4) %294, i64 %293
  %296 = load double, ptr addrspace(4) %295, align 8
  %297 = sub nsw i64 %286, %113
  %298 = bitcast ptr addrspace(4) %238 to ptr addrspace(4)
  %299 = getelementptr inbounds double, ptr addrspace(4) %298, i64 %297
  %300 = load double, ptr addrspace(4) %299, align 8
  %301 = fsub double %296, %300
  %302 = fdiv double %301, 1.000000e+01
  %303 = fadd double %292, %302
  %304 = sub nsw i64 %286, %120
  %305 = bitcast ptr addrspace(4) %241 to ptr addrspace(4)
  %306 = getelementptr inbounds double, ptr addrspace(4) %305, i64 %304
  %307 = load double, ptr addrspace(4) %306, align 8
  %308 = bitcast ptr addrspace(4) %246 to ptr addrspace(4)
  %309 = getelementptr inbounds double, ptr addrspace(4) %308, i64 %304
  %310 = load double, ptr addrspace(4) %309, align 8
  %311 = fsub double %307, %310
  %312 = fdiv double %311, 1.000000e+01
  %313 = sub nsw i64 %286, %127
  %314 = bitcast ptr addrspace(4) %249 to ptr addrspace(4)
  %315 = getelementptr inbounds double, ptr addrspace(4) %314, i64 %313
  %316 = load double, ptr addrspace(4) %315, align 8
  %317 = bitcast ptr addrspace(4) %252 to ptr addrspace(4)
  %318 = getelementptr inbounds double, ptr addrspace(4) %317, i64 %313
  %319 = load double, ptr addrspace(4) %318, align 8
  %320 = fsub double %316, %319
  %321 = fdiv double %320, 1.000000e+01
  %322 = fadd double %312, %321
  %323 = sub nsw i64 %286, %134
  %324 = bitcast ptr addrspace(4) %255 to ptr addrspace(4)
  %325 = getelementptr inbounds double, ptr addrspace(4) %324, i64 %323
  %326 = load double, ptr addrspace(4) %325, align 8
  %327 = bitcast ptr addrspace(4) %256 to ptr addrspace(4)
  %328 = getelementptr inbounds double, ptr addrspace(4) %327, i64 %323
  %329 = load double, ptr addrspace(4) %328, align 8
  %330 = fsub double %326, %329
  %331 = fdiv double %330, 1.000000e+01
  %332 = sub nsw i64 %286, %141
  %333 = bitcast ptr addrspace(4) %259 to ptr addrspace(4)
  %334 = getelementptr inbounds double, ptr addrspace(4) %333, i64 %332
  %335 = load double, ptr addrspace(4) %334, align 8
  %336 = bitcast ptr addrspace(4) %260 to ptr addrspace(4)
  %337 = getelementptr inbounds double, ptr addrspace(4) %336, i64 %332
  %338 = load double, ptr addrspace(4) %337, align 8
  %339 = fsub double %335, %338
  %340 = fdiv double %339, 1.000000e+01
  %341 = fadd double %331, %340
  %342 = sub nsw i64 %286, %148
  %343 = bitcast ptr addrspace(4) %263 to ptr addrspace(4)
  %344 = getelementptr inbounds double, ptr addrspace(4) %343, i64 %342
  %345 = load double, ptr addrspace(4) %344, align 8
  %346 = bitcast ptr addrspace(4) %264 to ptr addrspace(4)
  %347 = getelementptr inbounds double, ptr addrspace(4) %346, i64 %342
  %348 = load double, ptr addrspace(4) %347, align 8
  %349 = fsub double %345, %348
  %350 = fdiv double %349, 1.000000e+01
  %351 = fadd double %341, %350
  %352 = sub nsw i64 %286, %155
  %353 = getelementptr inbounds double, ptr addrspace(4) %154, i64 %352
  %354 = load double, ptr addrspace(4) %353, align 8
  %355 = sub nsw i64 %286, %158
  %356 = bitcast ptr addrspace(4) %267 to ptr addrspace(4)
  %357 = getelementptr inbounds double, ptr addrspace(4) %356, i64 %355
  %358 = load double, ptr addrspace(4) %357, align 8
  %359 = fsub double 6.250000e+02, %354
  %360 = fmul double %358, %359
  %361 = fdiv double %303, 2.800000e+03
  %362 = fadd double %360, %361
  %363 = fadd double %354, 6.250000e+02
  %364 = fdiv double %362, %363
  store double %364, ptr addrspace(4) %357, align 8
  %365 = sub nsw i64 %286, %168
  %366 = bitcast ptr addrspace(4) %273 to ptr addrspace(4)
  %367 = getelementptr inbounds double, ptr addrspace(4) %366, i64 %365
  %368 = load double, ptr addrspace(4) %367, align 8
  %369 = fmul double %368, %274
  %370 = fdiv double %322, 2.800000e+03
  %371 = fadd double %369, %370
  %372 = fdiv double %371, %275
  store double %372, ptr addrspace(4) %367, align 8
  %373 = sub nsw i64 %286, %178
  %374 = bitcast ptr addrspace(4) %278 to ptr addrspace(4)
  %375 = getelementptr inbounds double, ptr addrspace(4) %374, i64 %373
  %376 = load double, ptr addrspace(4) %375, align 8
  %377 = fmul double %376, %229
  %378 = fdiv double %351, 2.800000e+03
  %379 = fadd double %377, %378
  %380 = fdiv double %379, %230
  store double %380, ptr addrspace(4) %375, align 8
  br label %33
}

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv() #4

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32) #4

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32) #4

attributes #0 = { nounwind }
attributes #1 = { alwaysinline nounwind }
attributes #2 = { noinline nounwind optnone }
attributes #3 = { noinline nounwind }
attributes #4 = { nounwind willreturn memory(none) }

!opencl.spir.version = !{!0}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!1}
!opencl.used.optional.core.features = !{!2}
!igc.functions = !{}

!0 = !{i32 2, i32 0}
!1 = !{!"cl_khr_int64_extended_atomics"}
!2 = !{!"cl_doubles"}
