; REQUIRES: regkeys, llvm-16-plus
; RUN: llvm-as --opaque-pointers < %s -o %t.bc

; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options " -igc_opts 'RematSingleFlowRematEnabled=0, DumpASMToConsole=1, EnableOpaquePointersBackend=1, PrintToConsole=1, ' -cl-intel-256-GRF-per-thread" &> %t_output.ll
; RUN: echo "NEXT STAGE!!!!!" >> %t_output.ll
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options " -igc_opts 'RematSingleFlowRematEnabled=1, DumpASMToConsole=1, EnableOpaquePointersBackend=1, PrintToConsole=1, ' -cl-intel-256-GRF-per-thread" &>> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK: //.kernel foo
; CHECK: //.platform XE2
; CHECK: //.thread_config numGRF=256, numAcc=8, numSWSB=32
; CHECK: //.spill size
; CHECK: //.spill GRF est. ref count

; CHECK-LABEL: Build succeeded.
; CHECK-LABEL: NEXT STAGE!!!!!

; CHECK: //.kernel foo
; CHECK: //.platform XE2
; CHECK: //.thread_config numGRF=256, numAcc=8, numSWSB=32
; CHECK-NOT: //.spill size
; CHECK: //.instCount

; CHECK-LABEL: Build succeeded.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @foo(ptr addrspace(1) nocapture readonly %0, ptr addrspace(1) %1, ptr addrspace(1) nocapture %2, i32 %3, i32 %4, ptr addrspace(1) nocapture readnone %5, ptr addrspace(1) nocapture readnone %6, ptr addrspace(3) %7) #0 !kernel_arg_addr_space !387 !kernel_arg_access_qual !388 !kernel_arg_type !389 !kernel_arg_type_qual !390 !kernel_arg_base_type !389 !kernel_arg_name !390 !spirv.ParameterDecorations !391 !reqd_work_group_size !398 !intel_reqd_sub_group_size !399 {
  %9 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 0) #2
  %10 = insertelement <3 x i64> undef, i64 %9, i32 0
  %11 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 1) #2
  %12 = insertelement <3 x i64> %10, i64 %11, i32 1
  %13 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 2) #2
  %14 = insertelement <3 x i64> %12, i64 %13, i32 2
  %15 = extractelement <3 x i64> %14, i32 1
  %16 = select i1 true, i64 %15, i64 0
  %17 = trunc i64 %16 to i32
  %18 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 0) #2
  %19 = insertelement <3 x i64> undef, i64 %18, i32 0
  %20 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 1) #2
  %21 = insertelement <3 x i64> %19, i64 %20, i32 1
  %22 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 2) #2
  %23 = insertelement <3 x i64> %21, i64 %22, i32 2
  %24 = extractelement <3 x i64> %23, i32 2
  %25 = select i1 true, i64 %24, i64 0
  %26 = trunc i64 %25 to i32
  %27 = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 0) #2
  %28 = insertelement <3 x i64> undef, i64 %27, i32 0
  %29 = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 1) #2
  %30 = insertelement <3 x i64> %28, i64 %29, i32 1
  %31 = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 2) #2
  %32 = insertelement <3 x i64> %30, i64 %31, i32 2
  %33 = extractelement <3 x i64> %32, i32 1
  %34 = select i1 true, i64 %33, i64 1
  %35 = trunc i64 %34 to i32
  %36 = mul i32 %35, %26
  %37 = add i32 %36, %17
  %38 = shl i32 %37, 9
  %39 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0) #2
  %40 = insertelement <3 x i64> undef, i64 %39, i32 0
  %41 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 1) #2
  %42 = insertelement <3 x i64> %40, i64 %41, i32 1
  %43 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 2) #2
  %44 = insertelement <3 x i64> %42, i64 %43, i32 2
  %45 = extractelement <3 x i64> %44, i32 0
  %46 = select i1 true, i64 %45, i64 0
  %47 = trunc i64 %46 to i32
  %48 = and i32 %47, 248
  %49 = lshr i32 %48, 3
  %50 = shl i32 %47, 1
  %51 = and i32 %50, 510
  %52 = or i32 %38, %49
  %53 = or i32 %38, %51
  %54 = icmp slt i32 %53, 12582912
  %55 = icmp slt i32 %52, 12582912
  %56 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 0) #2
  %57 = insertelement <3 x i64> undef, i64 %56, i32 0
  %58 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 1) #2
  %59 = insertelement <3 x i64> %57, i64 %58, i32 1
  %60 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 2) #2
  %61 = insertelement <3 x i64> %59, i64 %60, i32 2
  %62 = extractelement <3 x i64> %61, i32 0
  %63 = select i1 true, i64 %62, i64 0
  %64 = trunc i64 %63 to i32
  %65 = shl i32 %64, 6
  %66 = and i32 %47, 7
  %67 = shl nuw nsw i32 %66, 3, !spirv.Decorations !400
  %68 = or i32 %65, 1
  %69 = or i32 %65, 2
  %70 = or i32 %65, 3
  %71 = or i32 %65, 4
  %72 = or i32 %65, 5
  %73 = or i32 %65, 6
  %74 = or i32 %65, 7
  %75 = or i32 %65, 8
  %76 = or i32 %65, 9
  %77 = or i32 %65, 10
  %78 = or i32 %65, 11
  %79 = or i32 %65, 12
  %80 = or i32 %65, 13
  %81 = or i32 %65, 14
  %82 = or i32 %65, 15
  %83 = or i32 %65, 16
  %84 = or i32 %65, 17
  %85 = or i32 %65, 18
  %86 = or i32 %65, 19
  %87 = or i32 %65, 20
  %88 = or i32 %65, 21
  %89 = or i32 %65, 22
  %90 = or i32 %65, 23
  %91 = or i32 %65, 24
  %92 = or i32 %65, 25
  %93 = or i32 %65, 26
  %94 = or i32 %65, 27
  %95 = or i32 %65, 28
  %96 = or i32 %65, 29
  %97 = or i32 %65, 30
  %98 = or i32 %65, 31
  %99 = or i32 %65, 32
  %100 = or i32 %65, 33
  %101 = or i32 %65, 34
  %102 = or i32 %65, 35
  %103 = or i32 %65, 36
  %104 = or i32 %65, 37
  %105 = or i32 %65, 38
  %106 = or i32 %65, 39
  %107 = or i32 %65, 40
  %108 = or i32 %65, 41
  %109 = or i32 %65, 42
  %110 = or i32 %65, 43
  %111 = or i32 %65, 44
  %112 = or i32 %65, 45
  %113 = or i32 %65, 46
  %114 = or i32 %65, 47
  %115 = or i32 %65, 48
  %116 = or i32 %65, 49
  %117 = or i32 %65, 50
  %118 = or i32 %65, 51
  %119 = or i32 %65, 52
  %120 = or i32 %65, 53
  %121 = or i32 %65, 54
  %122 = or i32 %65, 55
  %123 = or i32 %65, 56
  %124 = or i32 %65, 57
  %125 = or i32 %65, 58
  %126 = or i32 %65, 59
  %127 = or i32 %65, 60
  %128 = or i32 %65, 61
  %129 = or i32 %65, 62
  %130 = or i32 %65, 63
  %131 = or i32 %65, %67
  %132 = icmp slt i32 %65, 64
  %133 = icmp slt i32 %131, 64
  %134 = and i32 %50, 126
  %135 = lshr i32 %38, 7
  %136 = lshr i32 %53, 7
  %137 = and i32 %135, 60
  %138 = and i32 %136, 63
  %139 = lshr i32 %38, 13
  %140 = urem i32 %139, 12
  %141 = udiv i32 %38, 98304
  %142 = icmp ult i32 %134, 64
  %143 = icmp eq i32 %137, 0
  %144 = icmp eq i32 %138, 0
  %145 = and i1 %142, %144
  %146 = shl nuw nsw i32 %139, 12, !spirv.Decorations !400
  %147 = and i1 %132, %145
  %148 = and i1 %133, %143
  %149 = and i1 %54, %147
  %150 = and i1 %55, %148
  br i1 %149, label %151, label %159

151:                                              ; preds = %8
  %152 = or i32 %146, %134
  %153 = zext i32 %152 to i64
  %154 = getelementptr i64, ptr addrspace(1) %0, i64 %153
  %155 = bitcast ptr addrspace(1) %154 to ptr addrspace(1)
  %156 = getelementptr i8, ptr addrspace(1) %155, i64 32256
  %157 = bitcast ptr addrspace(1) %156 to ptr addrspace(1)
  %158 = load <2 x i64>, ptr addrspace(1) %157, align 16
  br label %159

159:                                              ; preds = %151, %8
  %160 = phi <2 x i64> [ %158, %151 ], [ zeroinitializer, %8 ]
  %161 = extractelement <2 x i64> %160, i32 0
  %162 = extractelement <2 x i64> %160, i32 1
  %163 = and i64 %161, 4095
  %164 = and i64 %162, 4095
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %165 = shl nuw nsw i32 %140, 6, !spirv.Decorations !400
  %166 = add i32 %165, %65
  %167 = add i32 %68, %165
  %168 = add i32 %69, %165
  %169 = add i32 %70, %165
  %170 = add i32 %71, %165
  %171 = add i32 %72, %165
  %172 = add i32 %73, %165
  %173 = add i32 %74, %165
  %174 = add i32 %75, %165
  %175 = add i32 %76, %165
  %176 = add i32 %77, %165
  %177 = add i32 %78, %165
  %178 = add i32 %79, %165
  %179 = add i32 %80, %165
  %180 = add i32 %81, %165
  %181 = add i32 %82, %165
  %182 = add i32 %83, %165
  %183 = add i32 %84, %165
  %184 = add i32 %85, %165
  %185 = add i32 %86, %165
  %186 = add i32 %87, %165
  %187 = add i32 %88, %165
  %188 = add i32 %89, %165
  %189 = add i32 %90, %165
  %190 = add i32 %91, %165
  %191 = add i32 %92, %165
  %192 = add i32 %93, %165
  %193 = add i32 %94, %165
  %194 = add i32 %95, %165
  %195 = add i32 %96, %165
  %196 = add i32 %97, %165
  %197 = add i32 %98, %165
  %198 = add i32 %99, %165
  %199 = add i32 %100, %165
  %200 = add i32 %101, %165
  %201 = add i32 %102, %165
  %202 = add i32 %103, %165
  %203 = add i32 %104, %165
  %204 = add i32 %105, %165
  %205 = add i32 %106, %165
  %206 = add i32 %107, %165
  %207 = add i32 %108, %165
  %208 = add i32 %109, %165
  %209 = add i32 %110, %165
  %210 = add i32 %111, %165
  %211 = add i32 %112, %165
  %212 = add i32 %113, %165
  %213 = add i32 %114, %165
  %214 = add i32 %115, %165
  %215 = add i32 %116, %165
  %216 = add i32 %117, %165
  %217 = add i32 %118, %165
  %218 = add i32 %119, %165
  %219 = add i32 %120, %165
  %220 = add i32 %121, %165
  %221 = add i32 %122, %165
  %222 = add i32 %123, %165
  %223 = add i32 %124, %165
  %224 = add i32 %125, %165
  %225 = add i32 %126, %165
  %226 = add i32 %127, %165
  %227 = add i32 %128, %165
  %228 = add i32 %129, %165
  %229 = add i32 %130, %165
  %230 = sext i32 %166 to i64
  %231 = sext i32 %167 to i64
  %232 = sext i32 %168 to i64
  %233 = sext i32 %169 to i64
  %234 = sext i32 %170 to i64
  %235 = sext i32 %171 to i64
  %236 = sext i32 %172 to i64
  %237 = sext i32 %173 to i64
  %238 = sext i32 %174 to i64
  %239 = sext i32 %175 to i64
  %240 = sext i32 %176 to i64
  %241 = sext i32 %177 to i64
  %242 = sext i32 %178 to i64
  %243 = sext i32 %179 to i64
  %244 = sext i32 %180 to i64
  %245 = sext i32 %181 to i64
  %246 = sext i32 %182 to i64
  %247 = sext i32 %183 to i64
  %248 = sext i32 %184 to i64
  %249 = sext i32 %185 to i64
  %250 = sext i32 %186 to i64
  %251 = sext i32 %187 to i64
  %252 = sext i32 %188 to i64
  %253 = sext i32 %189 to i64
  %254 = sext i32 %190 to i64
  %255 = sext i32 %191 to i64
  %256 = sext i32 %192 to i64
  %257 = sext i32 %193 to i64
  %258 = sext i32 %194 to i64
  %259 = sext i32 %195 to i64
  %260 = sext i32 %196 to i64
  %261 = sext i32 %197 to i64
  %262 = sext i32 %198 to i64
  %263 = sext i32 %199 to i64
  %264 = sext i32 %200 to i64
  %265 = sext i32 %201 to i64
  %266 = sext i32 %202 to i64
  %267 = sext i32 %203 to i64
  %268 = sext i32 %204 to i64
  %269 = sext i32 %205 to i64
  %270 = sext i32 %206 to i64
  %271 = sext i32 %207 to i64
  %272 = sext i32 %208 to i64
  %273 = sext i32 %209 to i64
  %274 = sext i32 %210 to i64
  %275 = sext i32 %211 to i64
  %276 = sext i32 %212 to i64
  %277 = sext i32 %213 to i64
  %278 = sext i32 %214 to i64
  %279 = sext i32 %215 to i64
  %280 = sext i32 %216 to i64
  %281 = sext i32 %217 to i64
  %282 = sext i32 %218 to i64
  %283 = sext i32 %219 to i64
  %284 = sext i32 %220 to i64
  %285 = sext i32 %221 to i64
  %286 = sext i32 %222 to i64
  %287 = sext i32 %223 to i64
  %288 = sext i32 %224 to i64
  %289 = sext i32 %225 to i64
  %290 = sext i32 %226 to i64
  %291 = sext i32 %227 to i64
  %292 = sext i32 %228 to i64
  %293 = sext i32 %229 to i64
  %294 = mul i32 %141, 3145728
  %295 = sext i32 %294 to i64
  %.idx = mul nuw nsw i64 %163, 1536, !spirv.Decorations !400
  %296 = getelementptr i8, ptr addrspace(1) %1, i64 %.idx
  %297 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %298 = getelementptr half, ptr addrspace(1) %297, i64 %230
  %299 = getelementptr half, ptr addrspace(1) %298, i64 %295
  %.idx41 = mul nuw nsw i64 %164, 1536, !spirv.Decorations !400
  %300 = getelementptr i8, ptr addrspace(1) %1, i64 %.idx41
  %301 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %302 = getelementptr half, ptr addrspace(1) %301, i64 %230
  %303 = getelementptr half, ptr addrspace(1) %302, i64 %295
  %304 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %305 = getelementptr half, ptr addrspace(1) %304, i64 %231
  %306 = getelementptr half, ptr addrspace(1) %305, i64 %295
  %307 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %308 = getelementptr half, ptr addrspace(1) %307, i64 %231
  %309 = getelementptr half, ptr addrspace(1) %308, i64 %295
  %310 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %311 = getelementptr half, ptr addrspace(1) %310, i64 %232
  %312 = getelementptr half, ptr addrspace(1) %311, i64 %295
  %313 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %314 = getelementptr half, ptr addrspace(1) %313, i64 %232
  %315 = getelementptr half, ptr addrspace(1) %314, i64 %295
  %316 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %317 = getelementptr half, ptr addrspace(1) %316, i64 %233
  %318 = getelementptr half, ptr addrspace(1) %317, i64 %295
  %319 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %320 = getelementptr half, ptr addrspace(1) %319, i64 %233
  %321 = getelementptr half, ptr addrspace(1) %320, i64 %295
  %322 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %323 = getelementptr half, ptr addrspace(1) %322, i64 %234
  %324 = getelementptr half, ptr addrspace(1) %323, i64 %295
  %325 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %326 = getelementptr half, ptr addrspace(1) %325, i64 %234
  %327 = getelementptr half, ptr addrspace(1) %326, i64 %295
  %328 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %329 = getelementptr half, ptr addrspace(1) %328, i64 %235
  %330 = getelementptr half, ptr addrspace(1) %329, i64 %295
  %331 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %332 = getelementptr half, ptr addrspace(1) %331, i64 %235
  %333 = getelementptr half, ptr addrspace(1) %332, i64 %295
  %334 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %335 = getelementptr half, ptr addrspace(1) %334, i64 %236
  %336 = getelementptr half, ptr addrspace(1) %335, i64 %295
  %337 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %338 = getelementptr half, ptr addrspace(1) %337, i64 %236
  %339 = getelementptr half, ptr addrspace(1) %338, i64 %295
  %340 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %341 = getelementptr half, ptr addrspace(1) %340, i64 %237
  %342 = getelementptr half, ptr addrspace(1) %341, i64 %295
  %343 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %344 = getelementptr half, ptr addrspace(1) %343, i64 %237
  %345 = getelementptr half, ptr addrspace(1) %344, i64 %295
  %346 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %347 = getelementptr half, ptr addrspace(1) %346, i64 %238
  %348 = getelementptr half, ptr addrspace(1) %347, i64 %295
  %349 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %350 = getelementptr half, ptr addrspace(1) %349, i64 %238
  %351 = getelementptr half, ptr addrspace(1) %350, i64 %295
  %352 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %353 = getelementptr half, ptr addrspace(1) %352, i64 %239
  %354 = getelementptr half, ptr addrspace(1) %353, i64 %295
  %355 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %356 = getelementptr half, ptr addrspace(1) %355, i64 %239
  %357 = getelementptr half, ptr addrspace(1) %356, i64 %295
  %358 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %359 = getelementptr half, ptr addrspace(1) %358, i64 %240
  %360 = getelementptr half, ptr addrspace(1) %359, i64 %295
  %361 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %362 = getelementptr half, ptr addrspace(1) %361, i64 %240
  %363 = getelementptr half, ptr addrspace(1) %362, i64 %295
  %364 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %365 = getelementptr half, ptr addrspace(1) %364, i64 %241
  %366 = getelementptr half, ptr addrspace(1) %365, i64 %295
  %367 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %368 = getelementptr half, ptr addrspace(1) %367, i64 %241
  %369 = getelementptr half, ptr addrspace(1) %368, i64 %295
  %370 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %371 = getelementptr half, ptr addrspace(1) %370, i64 %242
  %372 = getelementptr half, ptr addrspace(1) %371, i64 %295
  %373 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %374 = getelementptr half, ptr addrspace(1) %373, i64 %242
  %375 = getelementptr half, ptr addrspace(1) %374, i64 %295
  %376 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %377 = getelementptr half, ptr addrspace(1) %376, i64 %243
  %378 = getelementptr half, ptr addrspace(1) %377, i64 %295
  %379 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %380 = getelementptr half, ptr addrspace(1) %379, i64 %243
  %381 = getelementptr half, ptr addrspace(1) %380, i64 %295
  %382 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %383 = getelementptr half, ptr addrspace(1) %382, i64 %244
  %384 = getelementptr half, ptr addrspace(1) %383, i64 %295
  %385 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %386 = getelementptr half, ptr addrspace(1) %385, i64 %244
  %387 = getelementptr half, ptr addrspace(1) %386, i64 %295
  %388 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %389 = getelementptr half, ptr addrspace(1) %388, i64 %245
  %390 = getelementptr half, ptr addrspace(1) %389, i64 %295
  %391 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %392 = getelementptr half, ptr addrspace(1) %391, i64 %245
  %393 = getelementptr half, ptr addrspace(1) %392, i64 %295
  %394 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %395 = getelementptr half, ptr addrspace(1) %394, i64 %246
  %396 = getelementptr half, ptr addrspace(1) %395, i64 %295
  %397 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %398 = getelementptr half, ptr addrspace(1) %397, i64 %246
  %399 = getelementptr half, ptr addrspace(1) %398, i64 %295
  %400 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %401 = getelementptr half, ptr addrspace(1) %400, i64 %247
  %402 = getelementptr half, ptr addrspace(1) %401, i64 %295
  %403 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %404 = getelementptr half, ptr addrspace(1) %403, i64 %247
  %405 = getelementptr half, ptr addrspace(1) %404, i64 %295
  %406 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %407 = getelementptr half, ptr addrspace(1) %406, i64 %248
  %408 = getelementptr half, ptr addrspace(1) %407, i64 %295
  %409 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %410 = getelementptr half, ptr addrspace(1) %409, i64 %248
  %411 = getelementptr half, ptr addrspace(1) %410, i64 %295
  %412 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %413 = getelementptr half, ptr addrspace(1) %412, i64 %249
  %414 = getelementptr half, ptr addrspace(1) %413, i64 %295
  %415 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %416 = getelementptr half, ptr addrspace(1) %415, i64 %249
  %417 = getelementptr half, ptr addrspace(1) %416, i64 %295
  %418 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %419 = getelementptr half, ptr addrspace(1) %418, i64 %250
  %420 = getelementptr half, ptr addrspace(1) %419, i64 %295
  %421 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %422 = getelementptr half, ptr addrspace(1) %421, i64 %250
  %423 = getelementptr half, ptr addrspace(1) %422, i64 %295
  %424 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %425 = getelementptr half, ptr addrspace(1) %424, i64 %251
  %426 = getelementptr half, ptr addrspace(1) %425, i64 %295
  %427 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %428 = getelementptr half, ptr addrspace(1) %427, i64 %251
  %429 = getelementptr half, ptr addrspace(1) %428, i64 %295
  %430 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %431 = getelementptr half, ptr addrspace(1) %430, i64 %252
  %432 = getelementptr half, ptr addrspace(1) %431, i64 %295
  %433 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %434 = getelementptr half, ptr addrspace(1) %433, i64 %252
  %435 = getelementptr half, ptr addrspace(1) %434, i64 %295
  %436 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %437 = getelementptr half, ptr addrspace(1) %436, i64 %253
  %438 = getelementptr half, ptr addrspace(1) %437, i64 %295
  %439 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %440 = getelementptr half, ptr addrspace(1) %439, i64 %253
  %441 = getelementptr half, ptr addrspace(1) %440, i64 %295
  %442 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %443 = getelementptr half, ptr addrspace(1) %442, i64 %254
  %444 = getelementptr half, ptr addrspace(1) %443, i64 %295
  %445 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %446 = getelementptr half, ptr addrspace(1) %445, i64 %254
  %447 = getelementptr half, ptr addrspace(1) %446, i64 %295
  %448 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %449 = getelementptr half, ptr addrspace(1) %448, i64 %255
  %450 = getelementptr half, ptr addrspace(1) %449, i64 %295
  %451 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %452 = getelementptr half, ptr addrspace(1) %451, i64 %255
  %453 = getelementptr half, ptr addrspace(1) %452, i64 %295
  %454 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %455 = getelementptr half, ptr addrspace(1) %454, i64 %256
  %456 = getelementptr half, ptr addrspace(1) %455, i64 %295
  %457 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %458 = getelementptr half, ptr addrspace(1) %457, i64 %256
  %459 = getelementptr half, ptr addrspace(1) %458, i64 %295
  %460 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %461 = getelementptr half, ptr addrspace(1) %460, i64 %257
  %462 = getelementptr half, ptr addrspace(1) %461, i64 %295
  %463 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %464 = getelementptr half, ptr addrspace(1) %463, i64 %257
  %465 = getelementptr half, ptr addrspace(1) %464, i64 %295
  %466 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %467 = getelementptr half, ptr addrspace(1) %466, i64 %258
  %468 = getelementptr half, ptr addrspace(1) %467, i64 %295
  %469 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %470 = getelementptr half, ptr addrspace(1) %469, i64 %258
  %471 = getelementptr half, ptr addrspace(1) %470, i64 %295
  %472 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %473 = getelementptr half, ptr addrspace(1) %472, i64 %259
  %474 = getelementptr half, ptr addrspace(1) %473, i64 %295
  %475 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %476 = getelementptr half, ptr addrspace(1) %475, i64 %259
  %477 = getelementptr half, ptr addrspace(1) %476, i64 %295
  %478 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %479 = getelementptr half, ptr addrspace(1) %478, i64 %260
  %480 = getelementptr half, ptr addrspace(1) %479, i64 %295
  %481 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %482 = getelementptr half, ptr addrspace(1) %481, i64 %260
  %483 = getelementptr half, ptr addrspace(1) %482, i64 %295
  %484 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %485 = getelementptr half, ptr addrspace(1) %484, i64 %261
  %486 = getelementptr half, ptr addrspace(1) %485, i64 %295
  %487 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %488 = getelementptr half, ptr addrspace(1) %487, i64 %261
  %489 = getelementptr half, ptr addrspace(1) %488, i64 %295
  %490 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %491 = getelementptr half, ptr addrspace(1) %490, i64 %262
  %492 = getelementptr half, ptr addrspace(1) %491, i64 %295
  %493 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %494 = getelementptr half, ptr addrspace(1) %493, i64 %262
  %495 = getelementptr half, ptr addrspace(1) %494, i64 %295
  %496 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %497 = getelementptr half, ptr addrspace(1) %496, i64 %263
  %498 = getelementptr half, ptr addrspace(1) %497, i64 %295
  %499 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %500 = getelementptr half, ptr addrspace(1) %499, i64 %263
  %501 = getelementptr half, ptr addrspace(1) %500, i64 %295
  %502 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %503 = getelementptr half, ptr addrspace(1) %502, i64 %264
  %504 = getelementptr half, ptr addrspace(1) %503, i64 %295
  %505 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %506 = getelementptr half, ptr addrspace(1) %505, i64 %264
  %507 = getelementptr half, ptr addrspace(1) %506, i64 %295
  %508 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %509 = getelementptr half, ptr addrspace(1) %508, i64 %265
  %510 = getelementptr half, ptr addrspace(1) %509, i64 %295
  %511 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %512 = getelementptr half, ptr addrspace(1) %511, i64 %265
  %513 = getelementptr half, ptr addrspace(1) %512, i64 %295
  %514 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %515 = getelementptr half, ptr addrspace(1) %514, i64 %266
  %516 = getelementptr half, ptr addrspace(1) %515, i64 %295
  %517 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %518 = getelementptr half, ptr addrspace(1) %517, i64 %266
  %519 = getelementptr half, ptr addrspace(1) %518, i64 %295
  %520 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %521 = getelementptr half, ptr addrspace(1) %520, i64 %267
  %522 = getelementptr half, ptr addrspace(1) %521, i64 %295
  %523 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %524 = getelementptr half, ptr addrspace(1) %523, i64 %267
  %525 = getelementptr half, ptr addrspace(1) %524, i64 %295
  %526 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %527 = getelementptr half, ptr addrspace(1) %526, i64 %268
  %528 = getelementptr half, ptr addrspace(1) %527, i64 %295
  %529 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %530 = getelementptr half, ptr addrspace(1) %529, i64 %268
  %531 = getelementptr half, ptr addrspace(1) %530, i64 %295
  %532 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %533 = getelementptr half, ptr addrspace(1) %532, i64 %269
  %534 = getelementptr half, ptr addrspace(1) %533, i64 %295
  %535 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %536 = getelementptr half, ptr addrspace(1) %535, i64 %269
  %537 = getelementptr half, ptr addrspace(1) %536, i64 %295
  %538 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %539 = getelementptr half, ptr addrspace(1) %538, i64 %270
  %540 = getelementptr half, ptr addrspace(1) %539, i64 %295
  %541 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %542 = getelementptr half, ptr addrspace(1) %541, i64 %270
  %543 = getelementptr half, ptr addrspace(1) %542, i64 %295
  %544 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %545 = getelementptr half, ptr addrspace(1) %544, i64 %271
  %546 = getelementptr half, ptr addrspace(1) %545, i64 %295
  %547 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %548 = getelementptr half, ptr addrspace(1) %547, i64 %271
  %549 = getelementptr half, ptr addrspace(1) %548, i64 %295
  %550 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %551 = getelementptr half, ptr addrspace(1) %550, i64 %272
  %552 = getelementptr half, ptr addrspace(1) %551, i64 %295
  %553 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %554 = getelementptr half, ptr addrspace(1) %553, i64 %272
  %555 = getelementptr half, ptr addrspace(1) %554, i64 %295
  %556 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %557 = getelementptr half, ptr addrspace(1) %556, i64 %273
  %558 = getelementptr half, ptr addrspace(1) %557, i64 %295
  %559 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %560 = getelementptr half, ptr addrspace(1) %559, i64 %273
  %561 = getelementptr half, ptr addrspace(1) %560, i64 %295
  %562 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %563 = getelementptr half, ptr addrspace(1) %562, i64 %274
  %564 = getelementptr half, ptr addrspace(1) %563, i64 %295
  %565 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %566 = getelementptr half, ptr addrspace(1) %565, i64 %274
  %567 = getelementptr half, ptr addrspace(1) %566, i64 %295
  %568 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %569 = getelementptr half, ptr addrspace(1) %568, i64 %275
  %570 = getelementptr half, ptr addrspace(1) %569, i64 %295
  %571 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %572 = getelementptr half, ptr addrspace(1) %571, i64 %275
  %573 = getelementptr half, ptr addrspace(1) %572, i64 %295
  %574 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %575 = getelementptr half, ptr addrspace(1) %574, i64 %276
  %576 = getelementptr half, ptr addrspace(1) %575, i64 %295
  %577 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %578 = getelementptr half, ptr addrspace(1) %577, i64 %276
  %579 = getelementptr half, ptr addrspace(1) %578, i64 %295
  %580 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %581 = getelementptr half, ptr addrspace(1) %580, i64 %277
  %582 = getelementptr half, ptr addrspace(1) %581, i64 %295
  %583 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %584 = getelementptr half, ptr addrspace(1) %583, i64 %277
  %585 = getelementptr half, ptr addrspace(1) %584, i64 %295
  %586 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %587 = getelementptr half, ptr addrspace(1) %586, i64 %278
  %588 = getelementptr half, ptr addrspace(1) %587, i64 %295
  %589 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %590 = getelementptr half, ptr addrspace(1) %589, i64 %278
  %591 = getelementptr half, ptr addrspace(1) %590, i64 %295
  %592 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %593 = getelementptr half, ptr addrspace(1) %592, i64 %279
  %594 = getelementptr half, ptr addrspace(1) %593, i64 %295
  %595 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %596 = getelementptr half, ptr addrspace(1) %595, i64 %279
  %597 = getelementptr half, ptr addrspace(1) %596, i64 %295
  %598 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %599 = getelementptr half, ptr addrspace(1) %598, i64 %280
  %600 = getelementptr half, ptr addrspace(1) %599, i64 %295
  %601 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %602 = getelementptr half, ptr addrspace(1) %601, i64 %280
  %603 = getelementptr half, ptr addrspace(1) %602, i64 %295
  %604 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %605 = getelementptr half, ptr addrspace(1) %604, i64 %281
  %606 = getelementptr half, ptr addrspace(1) %605, i64 %295
  %607 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %608 = getelementptr half, ptr addrspace(1) %607, i64 %281
  %609 = getelementptr half, ptr addrspace(1) %608, i64 %295
  %610 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %611 = getelementptr half, ptr addrspace(1) %610, i64 %282
  %612 = getelementptr half, ptr addrspace(1) %611, i64 %295
  %613 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %614 = getelementptr half, ptr addrspace(1) %613, i64 %282
  %615 = getelementptr half, ptr addrspace(1) %614, i64 %295
  %616 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %617 = getelementptr half, ptr addrspace(1) %616, i64 %283
  %618 = getelementptr half, ptr addrspace(1) %617, i64 %295
  %619 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %620 = getelementptr half, ptr addrspace(1) %619, i64 %283
  %621 = getelementptr half, ptr addrspace(1) %620, i64 %295
  %622 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %623 = getelementptr half, ptr addrspace(1) %622, i64 %284
  %624 = getelementptr half, ptr addrspace(1) %623, i64 %295
  %625 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %626 = getelementptr half, ptr addrspace(1) %625, i64 %284
  %627 = getelementptr half, ptr addrspace(1) %626, i64 %295
  %628 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %629 = getelementptr half, ptr addrspace(1) %628, i64 %285
  %630 = getelementptr half, ptr addrspace(1) %629, i64 %295
  %631 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %632 = getelementptr half, ptr addrspace(1) %631, i64 %285
  %633 = getelementptr half, ptr addrspace(1) %632, i64 %295
  %634 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %635 = getelementptr half, ptr addrspace(1) %634, i64 %286
  %636 = getelementptr half, ptr addrspace(1) %635, i64 %295
  %637 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %638 = getelementptr half, ptr addrspace(1) %637, i64 %286
  %639 = getelementptr half, ptr addrspace(1) %638, i64 %295
  %640 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %641 = getelementptr half, ptr addrspace(1) %640, i64 %287
  %642 = getelementptr half, ptr addrspace(1) %641, i64 %295
  %643 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %644 = getelementptr half, ptr addrspace(1) %643, i64 %287
  %645 = getelementptr half, ptr addrspace(1) %644, i64 %295
  %646 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %647 = getelementptr half, ptr addrspace(1) %646, i64 %288
  %648 = getelementptr half, ptr addrspace(1) %647, i64 %295
  %649 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %650 = getelementptr half, ptr addrspace(1) %649, i64 %288
  %651 = getelementptr half, ptr addrspace(1) %650, i64 %295
  %652 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %653 = getelementptr half, ptr addrspace(1) %652, i64 %289
  %654 = getelementptr half, ptr addrspace(1) %653, i64 %295
  %655 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %656 = getelementptr half, ptr addrspace(1) %655, i64 %289
  %657 = getelementptr half, ptr addrspace(1) %656, i64 %295
  %658 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %659 = getelementptr half, ptr addrspace(1) %658, i64 %290
  %660 = getelementptr half, ptr addrspace(1) %659, i64 %295
  %661 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %662 = getelementptr half, ptr addrspace(1) %661, i64 %290
  %663 = getelementptr half, ptr addrspace(1) %662, i64 %295
  %664 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %665 = getelementptr half, ptr addrspace(1) %664, i64 %291
  %666 = getelementptr half, ptr addrspace(1) %665, i64 %295
  %667 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %668 = getelementptr half, ptr addrspace(1) %667, i64 %291
  %669 = getelementptr half, ptr addrspace(1) %668, i64 %295
  %670 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %671 = getelementptr half, ptr addrspace(1) %670, i64 %292
  %672 = getelementptr half, ptr addrspace(1) %671, i64 %295
  %673 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %674 = getelementptr half, ptr addrspace(1) %673, i64 %292
  %675 = getelementptr half, ptr addrspace(1) %674, i64 %295
  %676 = bitcast ptr addrspace(1) %296 to ptr addrspace(1)
  %677 = getelementptr half, ptr addrspace(1) %676, i64 %293
  %678 = getelementptr half, ptr addrspace(1) %677, i64 %295
  %679 = bitcast ptr addrspace(1) %300 to ptr addrspace(1)
  %680 = getelementptr half, ptr addrspace(1) %679, i64 %293
  %681 = getelementptr half, ptr addrspace(1) %680, i64 %295
  %682 = ptrtoint ptr addrspace(1) %299 to i64
  %683 = ptrtoint ptr addrspace(1) %303 to i64
  %684 = ptrtoint ptr addrspace(1) %306 to i64
  %685 = ptrtoint ptr addrspace(1) %309 to i64
  %686 = ptrtoint ptr addrspace(1) %312 to i64
  %687 = ptrtoint ptr addrspace(1) %315 to i64
  %688 = ptrtoint ptr addrspace(1) %318 to i64
  %689 = ptrtoint ptr addrspace(1) %321 to i64
  %690 = ptrtoint ptr addrspace(1) %324 to i64
  %691 = ptrtoint ptr addrspace(1) %327 to i64
  %692 = ptrtoint ptr addrspace(1) %330 to i64
  %693 = ptrtoint ptr addrspace(1) %333 to i64
  %694 = ptrtoint ptr addrspace(1) %336 to i64
  %695 = ptrtoint ptr addrspace(1) %339 to i64
  %696 = ptrtoint ptr addrspace(1) %342 to i64
  %697 = ptrtoint ptr addrspace(1) %345 to i64
  %698 = ptrtoint ptr addrspace(1) %348 to i64
  %699 = ptrtoint ptr addrspace(1) %351 to i64
  %700 = ptrtoint ptr addrspace(1) %354 to i64
  %701 = ptrtoint ptr addrspace(1) %357 to i64
  %702 = ptrtoint ptr addrspace(1) %360 to i64
  %703 = ptrtoint ptr addrspace(1) %363 to i64
  %704 = ptrtoint ptr addrspace(1) %366 to i64
  %705 = ptrtoint ptr addrspace(1) %369 to i64
  %706 = ptrtoint ptr addrspace(1) %372 to i64
  %707 = ptrtoint ptr addrspace(1) %375 to i64
  %708 = ptrtoint ptr addrspace(1) %378 to i64
  %709 = ptrtoint ptr addrspace(1) %381 to i64
  %710 = ptrtoint ptr addrspace(1) %384 to i64
  %711 = ptrtoint ptr addrspace(1) %387 to i64
  %712 = ptrtoint ptr addrspace(1) %390 to i64
  %713 = ptrtoint ptr addrspace(1) %393 to i64
  %714 = ptrtoint ptr addrspace(1) %396 to i64
  %715 = ptrtoint ptr addrspace(1) %399 to i64
  %716 = ptrtoint ptr addrspace(1) %402 to i64
  %717 = ptrtoint ptr addrspace(1) %405 to i64
  %718 = ptrtoint ptr addrspace(1) %408 to i64
  %719 = ptrtoint ptr addrspace(1) %411 to i64
  %720 = ptrtoint ptr addrspace(1) %414 to i64
  %721 = ptrtoint ptr addrspace(1) %417 to i64
  %722 = ptrtoint ptr addrspace(1) %420 to i64
  %723 = ptrtoint ptr addrspace(1) %423 to i64
  %724 = ptrtoint ptr addrspace(1) %426 to i64
  %725 = ptrtoint ptr addrspace(1) %429 to i64
  %726 = ptrtoint ptr addrspace(1) %432 to i64
  %727 = ptrtoint ptr addrspace(1) %435 to i64
  %728 = ptrtoint ptr addrspace(1) %438 to i64
  %729 = ptrtoint ptr addrspace(1) %441 to i64
  %730 = ptrtoint ptr addrspace(1) %444 to i64
  %731 = ptrtoint ptr addrspace(1) %447 to i64
  %732 = ptrtoint ptr addrspace(1) %450 to i64
  %733 = ptrtoint ptr addrspace(1) %453 to i64
  %734 = ptrtoint ptr addrspace(1) %456 to i64
  %735 = ptrtoint ptr addrspace(1) %459 to i64
  %736 = ptrtoint ptr addrspace(1) %462 to i64
  %737 = ptrtoint ptr addrspace(1) %465 to i64
  %738 = ptrtoint ptr addrspace(1) %468 to i64
  %739 = ptrtoint ptr addrspace(1) %471 to i64
  %740 = ptrtoint ptr addrspace(1) %474 to i64
  %741 = ptrtoint ptr addrspace(1) %477 to i64
  %742 = ptrtoint ptr addrspace(1) %480 to i64
  %743 = ptrtoint ptr addrspace(1) %483 to i64
  %744 = ptrtoint ptr addrspace(1) %486 to i64
  %745 = ptrtoint ptr addrspace(1) %489 to i64
  %746 = ptrtoint ptr addrspace(1) %492 to i64
  %747 = ptrtoint ptr addrspace(1) %495 to i64
  %748 = ptrtoint ptr addrspace(1) %498 to i64
  %749 = ptrtoint ptr addrspace(1) %501 to i64
  %750 = ptrtoint ptr addrspace(1) %504 to i64
  %751 = ptrtoint ptr addrspace(1) %507 to i64
  %752 = ptrtoint ptr addrspace(1) %510 to i64
  %753 = ptrtoint ptr addrspace(1) %513 to i64
  %754 = ptrtoint ptr addrspace(1) %516 to i64
  %755 = ptrtoint ptr addrspace(1) %519 to i64
  %756 = ptrtoint ptr addrspace(1) %522 to i64
  %757 = ptrtoint ptr addrspace(1) %525 to i64
  %758 = ptrtoint ptr addrspace(1) %528 to i64
  %759 = ptrtoint ptr addrspace(1) %531 to i64
  %760 = ptrtoint ptr addrspace(1) %534 to i64
  %761 = ptrtoint ptr addrspace(1) %537 to i64
  %762 = ptrtoint ptr addrspace(1) %540 to i64
  %763 = ptrtoint ptr addrspace(1) %543 to i64
  %764 = ptrtoint ptr addrspace(1) %546 to i64
  %765 = ptrtoint ptr addrspace(1) %549 to i64
  %766 = ptrtoint ptr addrspace(1) %552 to i64
  %767 = ptrtoint ptr addrspace(1) %555 to i64
  %768 = ptrtoint ptr addrspace(1) %558 to i64
  %769 = ptrtoint ptr addrspace(1) %561 to i64
  %770 = ptrtoint ptr addrspace(1) %564 to i64
  %771 = ptrtoint ptr addrspace(1) %567 to i64
  %772 = ptrtoint ptr addrspace(1) %570 to i64
  %773 = ptrtoint ptr addrspace(1) %573 to i64
  %774 = ptrtoint ptr addrspace(1) %576 to i64
  %775 = ptrtoint ptr addrspace(1) %579 to i64
  %776 = ptrtoint ptr addrspace(1) %582 to i64
  %777 = ptrtoint ptr addrspace(1) %585 to i64
  %778 = ptrtoint ptr addrspace(1) %588 to i64
  %779 = ptrtoint ptr addrspace(1) %591 to i64
  %780 = ptrtoint ptr addrspace(1) %594 to i64
  %781 = ptrtoint ptr addrspace(1) %597 to i64
  %782 = ptrtoint ptr addrspace(1) %600 to i64
  %783 = ptrtoint ptr addrspace(1) %603 to i64
  %784 = ptrtoint ptr addrspace(1) %606 to i64
  %785 = ptrtoint ptr addrspace(1) %609 to i64
  %786 = ptrtoint ptr addrspace(1) %612 to i64
  %787 = ptrtoint ptr addrspace(1) %615 to i64
  %788 = ptrtoint ptr addrspace(1) %618 to i64
  %789 = ptrtoint ptr addrspace(1) %621 to i64
  %790 = ptrtoint ptr addrspace(1) %624 to i64
  %791 = ptrtoint ptr addrspace(1) %627 to i64
  %792 = ptrtoint ptr addrspace(1) %630 to i64
  %793 = ptrtoint ptr addrspace(1) %633 to i64
  %794 = ptrtoint ptr addrspace(1) %636 to i64
  %795 = ptrtoint ptr addrspace(1) %639 to i64
  %796 = ptrtoint ptr addrspace(1) %642 to i64
  %797 = ptrtoint ptr addrspace(1) %645 to i64
  %798 = ptrtoint ptr addrspace(1) %648 to i64
  %799 = ptrtoint ptr addrspace(1) %651 to i64
  %800 = ptrtoint ptr addrspace(1) %654 to i64
  %801 = ptrtoint ptr addrspace(1) %657 to i64
  %802 = ptrtoint ptr addrspace(1) %660 to i64
  %803 = ptrtoint ptr addrspace(1) %663 to i64
  %804 = ptrtoint ptr addrspace(1) %666 to i64
  %805 = ptrtoint ptr addrspace(1) %669 to i64
  %806 = ptrtoint ptr addrspace(1) %672 to i64
  %807 = ptrtoint ptr addrspace(1) %675 to i64
  %808 = ptrtoint ptr addrspace(1) %678 to i64
  %809 = ptrtoint ptr addrspace(1) %681 to i64
  %810 = shl nuw nsw i32 %66, 13, !spirv.Decorations !400
  %811 = shl nuw nsw i32 %48, 5, !spirv.Decorations !400
  %812 = shl nuw nsw i32 %66, 4, !spirv.Decorations !400
  %813 = or i32 %811, %812
  %814 = or i32 %813, %810
  %815 = zext i32 %814 to i64
  %816 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %815
  %817 = insertelement <2 x i64> undef, i64 %682, i32 0
  %818 = insertelement <2 x i64> %817, i64 %684, i32 1
  %819 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %818, ptr addrspace(3) %819, align 16
  %820 = getelementptr inbounds i8, ptr addrspace(3) %816, i64 128
  %821 = insertelement <2 x i64> undef, i64 %683, i32 0
  %822 = insertelement <2 x i64> %821, i64 %685, i32 1
  %823 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %822, ptr addrspace(3) %823, align 16
  %824 = xor i32 %814, 16
  %825 = zext i32 %824 to i64
  %826 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %825
  %827 = insertelement <2 x i64> undef, i64 %698, i32 0
  %828 = insertelement <2 x i64> %827, i64 %700, i32 1
  %829 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %828, ptr addrspace(3) %829, align 16
  %830 = getelementptr inbounds i8, ptr addrspace(3) %826, i64 128
  %831 = insertelement <2 x i64> undef, i64 %699, i32 0
  %832 = insertelement <2 x i64> %831, i64 %701, i32 1
  %833 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %832, ptr addrspace(3) %833, align 16
  %834 = xor i32 %814, 32
  %835 = zext i32 %834 to i64
  %836 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %835
  %837 = insertelement <2 x i64> undef, i64 %714, i32 0
  %838 = insertelement <2 x i64> %837, i64 %716, i32 1
  %839 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %838, ptr addrspace(3) %839, align 16
  %840 = getelementptr inbounds i8, ptr addrspace(3) %836, i64 128
  %841 = insertelement <2 x i64> undef, i64 %715, i32 0
  %842 = insertelement <2 x i64> %841, i64 %717, i32 1
  %843 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %842, ptr addrspace(3) %843, align 16
  %844 = xor i32 %814, 48
  %845 = zext i32 %844 to i64
  %846 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %845
  %847 = insertelement <2 x i64> undef, i64 %730, i32 0
  %848 = insertelement <2 x i64> %847, i64 %732, i32 1
  %849 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %848, ptr addrspace(3) %849, align 16
  %850 = getelementptr inbounds i8, ptr addrspace(3) %846, i64 128
  %851 = insertelement <2 x i64> undef, i64 %731, i32 0
  %852 = insertelement <2 x i64> %851, i64 %733, i32 1
  %853 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %852, ptr addrspace(3) %853, align 16
  %854 = xor i32 %814, 64
  %855 = zext i32 %854 to i64
  %856 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %855
  %857 = insertelement <2 x i64> undef, i64 %746, i32 0
  %858 = insertelement <2 x i64> %857, i64 %748, i32 1
  %859 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %858, ptr addrspace(3) %859, align 16
  %860 = getelementptr inbounds i8, ptr addrspace(3) %856, i64 128
  %861 = insertelement <2 x i64> undef, i64 %747, i32 0
  %862 = insertelement <2 x i64> %861, i64 %749, i32 1
  %863 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %862, ptr addrspace(3) %863, align 16
  %864 = xor i32 %814, 80
  %865 = zext i32 %864 to i64
  %866 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %865
  %867 = insertelement <2 x i64> undef, i64 %762, i32 0
  %868 = insertelement <2 x i64> %867, i64 %764, i32 1
  %869 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %868, ptr addrspace(3) %869, align 16
  %870 = getelementptr inbounds i8, ptr addrspace(3) %866, i64 128
  %871 = insertelement <2 x i64> undef, i64 %763, i32 0
  %872 = insertelement <2 x i64> %871, i64 %765, i32 1
  %873 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %872, ptr addrspace(3) %873, align 16
  %874 = xor i32 %814, 96
  %875 = zext i32 %874 to i64
  %876 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %875
  %877 = insertelement <2 x i64> undef, i64 %778, i32 0
  %878 = insertelement <2 x i64> %877, i64 %780, i32 1
  %879 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %878, ptr addrspace(3) %879, align 16
  %880 = getelementptr inbounds i8, ptr addrspace(3) %876, i64 128
  %881 = insertelement <2 x i64> undef, i64 %779, i32 0
  %882 = insertelement <2 x i64> %881, i64 %781, i32 1
  %883 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %882, ptr addrspace(3) %883, align 16
  %884 = xor i32 %814, 112
  %885 = zext i32 %884 to i64
  %886 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %885
  %887 = insertelement <2 x i64> undef, i64 %794, i32 0
  %888 = insertelement <2 x i64> %887, i64 %796, i32 1
  %889 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %888, ptr addrspace(3) %889, align 16
  %890 = getelementptr inbounds i8, ptr addrspace(3) %886, i64 128
  %891 = insertelement <2 x i64> undef, i64 %795, i32 0
  %892 = insertelement <2 x i64> %891, i64 %797, i32 1
  %893 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %892, ptr addrspace(3) %893, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %894 = and i32 %47, 112
  %895 = shl nuw nsw i32 %894, 9, !spirv.Decorations !400
  %896 = shl i32 %47, 4
  %897 = and i32 %896, 240
  %898 = and i32 %50, 256
  %899 = or i32 %895, %897
  %900 = xor i32 %899, %894
  %901 = or i32 %900, %898
  %902 = zext i32 %901 to i64
  %903 = getelementptr inbounds i8, ptr addrspace(3) %7, i64 %902
  %904 = bitcast ptr addrspace(3) %903 to ptr addrspace(3)
  %905 = load <2 x i64>, ptr addrspace(3) %904, align 16
  %906 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 512
  %907 = bitcast ptr addrspace(3) %906 to ptr addrspace(3)
  %908 = load i64, ptr addrspace(3) %907, align 16
  %909 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 1024
  %910 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 1536
  %911 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 2048
  %912 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 2560
  %913 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 3072
  %914 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 3584
  %915 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 4096
  %916 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 4608
  %917 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 5120
  %918 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 5632
  %919 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 6144
  %920 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 6656
  %921 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 7168
  %922 = getelementptr inbounds i8, ptr addrspace(3) %903, i64 7680
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %923 = insertelement <2 x i64> undef, i64 %686, i32 0
  %924 = insertelement <2 x i64> %923, i64 %688, i32 1
  %925 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %924, ptr addrspace(3) %925, align 16
  %926 = insertelement <2 x i64> undef, i64 %687, i32 0
  %927 = insertelement <2 x i64> %926, i64 %689, i32 1
  %928 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %927, ptr addrspace(3) %928, align 16
  %929 = insertelement <2 x i64> undef, i64 %702, i32 0
  %930 = insertelement <2 x i64> %929, i64 %704, i32 1
  %931 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %930, ptr addrspace(3) %931, align 16
  %932 = insertelement <2 x i64> undef, i64 %703, i32 0
  %933 = insertelement <2 x i64> %932, i64 %705, i32 1
  %934 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %933, ptr addrspace(3) %934, align 16
  %935 = insertelement <2 x i64> undef, i64 %718, i32 0
  %936 = insertelement <2 x i64> %935, i64 %720, i32 1
  %937 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %936, ptr addrspace(3) %937, align 16
  %938 = insertelement <2 x i64> undef, i64 %719, i32 0
  %939 = insertelement <2 x i64> %938, i64 %721, i32 1
  %940 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %939, ptr addrspace(3) %940, align 16
  %941 = insertelement <2 x i64> undef, i64 %734, i32 0
  %942 = insertelement <2 x i64> %941, i64 %736, i32 1
  %943 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %942, ptr addrspace(3) %943, align 16
  %944 = insertelement <2 x i64> undef, i64 %735, i32 0
  %945 = insertelement <2 x i64> %944, i64 %737, i32 1
  %946 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %945, ptr addrspace(3) %946, align 16
  %947 = insertelement <2 x i64> undef, i64 %750, i32 0
  %948 = insertelement <2 x i64> %947, i64 %752, i32 1
  %949 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %948, ptr addrspace(3) %949, align 16
  %950 = insertelement <2 x i64> undef, i64 %751, i32 0
  %951 = insertelement <2 x i64> %950, i64 %753, i32 1
  %952 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %951, ptr addrspace(3) %952, align 16
  %953 = insertelement <2 x i64> undef, i64 %766, i32 0
  %954 = insertelement <2 x i64> %953, i64 %768, i32 1
  %955 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %954, ptr addrspace(3) %955, align 16
  %956 = insertelement <2 x i64> undef, i64 %767, i32 0
  %957 = insertelement <2 x i64> %956, i64 %769, i32 1
  %958 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %957, ptr addrspace(3) %958, align 16
  %959 = insertelement <2 x i64> undef, i64 %782, i32 0
  %960 = insertelement <2 x i64> %959, i64 %784, i32 1
  %961 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %960, ptr addrspace(3) %961, align 16
  %962 = insertelement <2 x i64> undef, i64 %783, i32 0
  %963 = insertelement <2 x i64> %962, i64 %785, i32 1
  %964 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %963, ptr addrspace(3) %964, align 16
  %965 = insertelement <2 x i64> undef, i64 %798, i32 0
  %966 = insertelement <2 x i64> %965, i64 %800, i32 1
  %967 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %966, ptr addrspace(3) %967, align 16
  %968 = insertelement <2 x i64> undef, i64 %799, i32 0
  %969 = insertelement <2 x i64> %968, i64 %801, i32 1
  %970 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %969, ptr addrspace(3) %970, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %971 = insertelement <2 x i64> undef, i64 %690, i32 0
  %972 = insertelement <2 x i64> %971, i64 %692, i32 1
  %973 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %972, ptr addrspace(3) %973, align 16
  %974 = insertelement <2 x i64> undef, i64 %691, i32 0
  %975 = insertelement <2 x i64> %974, i64 %693, i32 1
  %976 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %975, ptr addrspace(3) %976, align 16
  %977 = insertelement <2 x i64> undef, i64 %706, i32 0
  %978 = insertelement <2 x i64> %977, i64 %708, i32 1
  %979 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %978, ptr addrspace(3) %979, align 16
  %980 = insertelement <2 x i64> undef, i64 %707, i32 0
  %981 = insertelement <2 x i64> %980, i64 %709, i32 1
  %982 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %981, ptr addrspace(3) %982, align 16
  %983 = insertelement <2 x i64> undef, i64 %722, i32 0
  %984 = insertelement <2 x i64> %983, i64 %724, i32 1
  %985 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %984, ptr addrspace(3) %985, align 16
  %986 = insertelement <2 x i64> undef, i64 %723, i32 0
  %987 = insertelement <2 x i64> %986, i64 %725, i32 1
  %988 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %987, ptr addrspace(3) %988, align 16
  %989 = insertelement <2 x i64> undef, i64 %738, i32 0
  %990 = insertelement <2 x i64> %989, i64 %740, i32 1
  %991 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %990, ptr addrspace(3) %991, align 16
  %992 = insertelement <2 x i64> undef, i64 %739, i32 0
  %993 = insertelement <2 x i64> %992, i64 %741, i32 1
  %994 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %993, ptr addrspace(3) %994, align 16
  %995 = insertelement <2 x i64> undef, i64 %754, i32 0
  %996 = insertelement <2 x i64> %995, i64 %756, i32 1
  %997 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %996, ptr addrspace(3) %997, align 16
  %998 = insertelement <2 x i64> undef, i64 %755, i32 0
  %999 = insertelement <2 x i64> %998, i64 %757, i32 1
  %1000 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %999, ptr addrspace(3) %1000, align 16
  %1001 = insertelement <2 x i64> undef, i64 %770, i32 0
  %1002 = insertelement <2 x i64> %1001, i64 %772, i32 1
  %1003 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %1002, ptr addrspace(3) %1003, align 16
  %1004 = insertelement <2 x i64> undef, i64 %771, i32 0
  %1005 = insertelement <2 x i64> %1004, i64 %773, i32 1
  %1006 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %1005, ptr addrspace(3) %1006, align 16
  %1007 = insertelement <2 x i64> undef, i64 %786, i32 0
  %1008 = insertelement <2 x i64> %1007, i64 %788, i32 1
  %1009 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %1008, ptr addrspace(3) %1009, align 16
  %1010 = insertelement <2 x i64> undef, i64 %787, i32 0
  %1011 = insertelement <2 x i64> %1010, i64 %789, i32 1
  %1012 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %1011, ptr addrspace(3) %1012, align 16
  %1013 = insertelement <2 x i64> undef, i64 %802, i32 0
  %1014 = insertelement <2 x i64> %1013, i64 %804, i32 1
  %1015 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %1014, ptr addrspace(3) %1015, align 16
  %1016 = insertelement <2 x i64> undef, i64 %803, i32 0
  %1017 = insertelement <2 x i64> %1016, i64 %805, i32 1
  %1018 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %1017, ptr addrspace(3) %1018, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1019 = insertelement <2 x i64> undef, i64 %694, i32 0
  %1020 = insertelement <2 x i64> %1019, i64 %696, i32 1
  %1021 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %1020, ptr addrspace(3) %1021, align 16
  %1022 = insertelement <2 x i64> undef, i64 %695, i32 0
  %1023 = insertelement <2 x i64> %1022, i64 %697, i32 1
  %1024 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %1023, ptr addrspace(3) %1024, align 16
  %1025 = insertelement <2 x i64> undef, i64 %710, i32 0
  %1026 = insertelement <2 x i64> %1025, i64 %712, i32 1
  %1027 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %1026, ptr addrspace(3) %1027, align 16
  %1028 = insertelement <2 x i64> undef, i64 %711, i32 0
  %1029 = insertelement <2 x i64> %1028, i64 %713, i32 1
  %1030 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %1029, ptr addrspace(3) %1030, align 16
  %1031 = insertelement <2 x i64> undef, i64 %726, i32 0
  %1032 = insertelement <2 x i64> %1031, i64 %728, i32 1
  %1033 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %1032, ptr addrspace(3) %1033, align 16
  %1034 = insertelement <2 x i64> undef, i64 %727, i32 0
  %1035 = insertelement <2 x i64> %1034, i64 %729, i32 1
  %1036 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %1035, ptr addrspace(3) %1036, align 16
  %1037 = insertelement <2 x i64> undef, i64 %742, i32 0
  %1038 = insertelement <2 x i64> %1037, i64 %744, i32 1
  %1039 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %1038, ptr addrspace(3) %1039, align 16
  %1040 = insertelement <2 x i64> undef, i64 %743, i32 0
  %1041 = insertelement <2 x i64> %1040, i64 %745, i32 1
  %1042 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %1041, ptr addrspace(3) %1042, align 16
  %1043 = insertelement <2 x i64> undef, i64 %758, i32 0
  %1044 = insertelement <2 x i64> %1043, i64 %760, i32 1
  %1045 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %1044, ptr addrspace(3) %1045, align 16
  %1046 = insertelement <2 x i64> undef, i64 %759, i32 0
  %1047 = insertelement <2 x i64> %1046, i64 %761, i32 1
  %1048 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %1047, ptr addrspace(3) %1048, align 16
  %1049 = insertelement <2 x i64> undef, i64 %774, i32 0
  %1050 = insertelement <2 x i64> %1049, i64 %776, i32 1
  %1051 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %1050, ptr addrspace(3) %1051, align 16
  %1052 = insertelement <2 x i64> undef, i64 %775, i32 0
  %1053 = insertelement <2 x i64> %1052, i64 %777, i32 1
  %1054 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %1053, ptr addrspace(3) %1054, align 16
  %1055 = insertelement <2 x i64> undef, i64 %790, i32 0
  %1056 = insertelement <2 x i64> %1055, i64 %792, i32 1
  %1057 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %1056, ptr addrspace(3) %1057, align 16
  %1058 = insertelement <2 x i64> undef, i64 %791, i32 0
  %1059 = insertelement <2 x i64> %1058, i64 %793, i32 1
  %1060 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %1059, ptr addrspace(3) %1060, align 16
  %1061 = insertelement <2 x i64> undef, i64 %806, i32 0
  %1062 = insertelement <2 x i64> %1061, i64 %808, i32 1
  %1063 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %1062, ptr addrspace(3) %1063, align 16
  %1064 = insertelement <2 x i64> undef, i64 %807, i32 0
  %1065 = insertelement <2 x i64> %1064, i64 %809, i32 1
  %1066 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %1065, ptr addrspace(3) %1066, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1067 = inttoptr i64 %908 to ptr addrspace(1)
  br i1 %150, label %1068, label %1072

1068:                                             ; preds = %159
  %1069 = extractelement <2 x i64> %905, i32 0
  %1070 = inttoptr i64 %1069 to ptr addrspace(1)
  %1071 = load <4 x i32>, ptr addrspace(1) %1070, align 16
  br label %1072

1072:                                             ; preds = %1068, %159
  %1073 = phi <4 x i32> [ %1071, %1068 ], [ zeroinitializer, %159 ]
  %1074 = extractelement <4 x i32> %1073, i32 0
  %1075 = bitcast i32 %1074 to <2 x half>
  %1076 = extractelement <4 x i32> %1073, i32 1
  %1077 = bitcast i32 %1076 to <2 x half>
  %1078 = extractelement <4 x i32> %1073, i32 2
  %1079 = bitcast i32 %1078 to <2 x half>
  %1080 = extractelement <4 x i32> %1073, i32 3
  %1081 = bitcast i32 %1080 to <2 x half>
  %1082 = extractelement <2 x half> %1075, i32 0
  %1083 = extractelement <2 x half> %1075, i32 1
  %1084 = extractelement <2 x half> %1077, i32 0
  %1085 = extractelement <2 x half> %1077, i32 1
  %1086 = extractelement <2 x half> %1079, i32 0
  %1087 = extractelement <2 x half> %1079, i32 1
  %1088 = extractelement <2 x half> %1081, i32 0
  %1089 = extractelement <2 x half> %1081, i32 1
  br i1 %150, label %1090, label %1092

1090:                                             ; preds = %1072
  %1091 = load <4 x i32>, ptr addrspace(1) %1067, align 16
  br label %1092

1092:                                             ; preds = %1090, %1072
  %1093 = phi <4 x i32> [ %1091, %1090 ], [ zeroinitializer, %1072 ]
  %1094 = extractelement <4 x i32> %1093, i32 0
  %1095 = bitcast i32 %1094 to <2 x half>
  %1096 = extractelement <4 x i32> %1093, i32 1
  %1097 = bitcast i32 %1096 to <2 x half>
  %1098 = extractelement <4 x i32> %1093, i32 2
  %1099 = bitcast i32 %1098 to <2 x half>
  %1100 = extractelement <4 x i32> %1093, i32 3
  %1101 = bitcast i32 %1100 to <2 x half>
  %1102 = extractelement <2 x half> %1095, i32 0
  %1103 = extractelement <2 x half> %1095, i32 1
  %1104 = extractelement <2 x half> %1097, i32 0
  %1105 = extractelement <2 x half> %1097, i32 1
  %1106 = extractelement <2 x half> %1099, i32 0
  %1107 = extractelement <2 x half> %1099, i32 1
  %1108 = extractelement <2 x half> %1101, i32 0
  %1109 = extractelement <2 x half> %1101, i32 1
  %1110 = icmp ne i32 %138, 0
  %1111 = icmp ne i32 %137, 0
  %1112 = and i1 %142, %1110
  %1113 = and i1 %132, %1112
  %1114 = and i1 %133, %1111
  %1115 = and i1 %54, %1113
  %1116 = and i1 %55, %1114
  %1117 = and i1 %133, %55
  br i1 %1115, label %1118, label %1127

1118:                                             ; preds = %1092
  %1119 = shl nuw nsw i32 %138, 6, !spirv.Decorations !400
  %1120 = or i32 %50, -64
  %1121 = add nsw i32 %1120, %146, !spirv.Decorations !403
  %1122 = add nsw i32 %1121, %1119, !spirv.Decorations !403
  %1123 = zext i32 %1122 to i64
  %1124 = getelementptr i64, ptr addrspace(1) %0, i64 %1123
  %1125 = bitcast ptr addrspace(1) %1124 to ptr addrspace(1)
  %1126 = load <2 x i64>, ptr addrspace(1) %1125, align 16
  br label %1127

1127:                                             ; preds = %1118, %1092
  %1128 = phi <2 x i64> [ %1126, %1118 ], [ zeroinitializer, %1092 ]
  %1129 = extractelement <2 x i64> %1128, i32 0
  %1130 = extractelement <2 x i64> %1128, i32 1
  %1131 = and i64 %1129, 4095
  %1132 = and i64 %1130, 4095
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %.idx42 = mul nuw nsw i64 %1131, 1536, !spirv.Decorations !400
  %1133 = getelementptr i8, ptr addrspace(1) %1, i64 %.idx42
  %1134 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1135 = getelementptr half, ptr addrspace(1) %1134, i64 %230
  %1136 = getelementptr half, ptr addrspace(1) %1135, i64 %295
  %.idx43 = mul nuw nsw i64 %1132, 1536, !spirv.Decorations !400
  %1137 = getelementptr i8, ptr addrspace(1) %1, i64 %.idx43
  %1138 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1139 = getelementptr half, ptr addrspace(1) %1138, i64 %230
  %1140 = getelementptr half, ptr addrspace(1) %1139, i64 %295
  %1141 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1142 = getelementptr half, ptr addrspace(1) %1141, i64 %231
  %1143 = getelementptr half, ptr addrspace(1) %1142, i64 %295
  %1144 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1145 = getelementptr half, ptr addrspace(1) %1144, i64 %231
  %1146 = getelementptr half, ptr addrspace(1) %1145, i64 %295
  %1147 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1148 = getelementptr half, ptr addrspace(1) %1147, i64 %232
  %1149 = getelementptr half, ptr addrspace(1) %1148, i64 %295
  %1150 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1151 = getelementptr half, ptr addrspace(1) %1150, i64 %232
  %1152 = getelementptr half, ptr addrspace(1) %1151, i64 %295
  %1153 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1154 = getelementptr half, ptr addrspace(1) %1153, i64 %233
  %1155 = getelementptr half, ptr addrspace(1) %1154, i64 %295
  %1156 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1157 = getelementptr half, ptr addrspace(1) %1156, i64 %233
  %1158 = getelementptr half, ptr addrspace(1) %1157, i64 %295
  %1159 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1160 = getelementptr half, ptr addrspace(1) %1159, i64 %234
  %1161 = getelementptr half, ptr addrspace(1) %1160, i64 %295
  %1162 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1163 = getelementptr half, ptr addrspace(1) %1162, i64 %234
  %1164 = getelementptr half, ptr addrspace(1) %1163, i64 %295
  %1165 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1166 = getelementptr half, ptr addrspace(1) %1165, i64 %235
  %1167 = getelementptr half, ptr addrspace(1) %1166, i64 %295
  %1168 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1169 = getelementptr half, ptr addrspace(1) %1168, i64 %235
  %1170 = getelementptr half, ptr addrspace(1) %1169, i64 %295
  %1171 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1172 = getelementptr half, ptr addrspace(1) %1171, i64 %236
  %1173 = getelementptr half, ptr addrspace(1) %1172, i64 %295
  %1174 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1175 = getelementptr half, ptr addrspace(1) %1174, i64 %236
  %1176 = getelementptr half, ptr addrspace(1) %1175, i64 %295
  %1177 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1178 = getelementptr half, ptr addrspace(1) %1177, i64 %237
  %1179 = getelementptr half, ptr addrspace(1) %1178, i64 %295
  %1180 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1181 = getelementptr half, ptr addrspace(1) %1180, i64 %237
  %1182 = getelementptr half, ptr addrspace(1) %1181, i64 %295
  %1183 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1184 = getelementptr half, ptr addrspace(1) %1183, i64 %238
  %1185 = getelementptr half, ptr addrspace(1) %1184, i64 %295
  %1186 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1187 = getelementptr half, ptr addrspace(1) %1186, i64 %238
  %1188 = getelementptr half, ptr addrspace(1) %1187, i64 %295
  %1189 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1190 = getelementptr half, ptr addrspace(1) %1189, i64 %239
  %1191 = getelementptr half, ptr addrspace(1) %1190, i64 %295
  %1192 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1193 = getelementptr half, ptr addrspace(1) %1192, i64 %239
  %1194 = getelementptr half, ptr addrspace(1) %1193, i64 %295
  %1195 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1196 = getelementptr half, ptr addrspace(1) %1195, i64 %240
  %1197 = getelementptr half, ptr addrspace(1) %1196, i64 %295
  %1198 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1199 = getelementptr half, ptr addrspace(1) %1198, i64 %240
  %1200 = getelementptr half, ptr addrspace(1) %1199, i64 %295
  %1201 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1202 = getelementptr half, ptr addrspace(1) %1201, i64 %241
  %1203 = getelementptr half, ptr addrspace(1) %1202, i64 %295
  %1204 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1205 = getelementptr half, ptr addrspace(1) %1204, i64 %241
  %1206 = getelementptr half, ptr addrspace(1) %1205, i64 %295
  %1207 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1208 = getelementptr half, ptr addrspace(1) %1207, i64 %242
  %1209 = getelementptr half, ptr addrspace(1) %1208, i64 %295
  %1210 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1211 = getelementptr half, ptr addrspace(1) %1210, i64 %242
  %1212 = getelementptr half, ptr addrspace(1) %1211, i64 %295
  %1213 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1214 = getelementptr half, ptr addrspace(1) %1213, i64 %243
  %1215 = getelementptr half, ptr addrspace(1) %1214, i64 %295
  %1216 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1217 = getelementptr half, ptr addrspace(1) %1216, i64 %243
  %1218 = getelementptr half, ptr addrspace(1) %1217, i64 %295
  %1219 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1220 = getelementptr half, ptr addrspace(1) %1219, i64 %244
  %1221 = getelementptr half, ptr addrspace(1) %1220, i64 %295
  %1222 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1223 = getelementptr half, ptr addrspace(1) %1222, i64 %244
  %1224 = getelementptr half, ptr addrspace(1) %1223, i64 %295
  %1225 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1226 = getelementptr half, ptr addrspace(1) %1225, i64 %245
  %1227 = getelementptr half, ptr addrspace(1) %1226, i64 %295
  %1228 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1229 = getelementptr half, ptr addrspace(1) %1228, i64 %245
  %1230 = getelementptr half, ptr addrspace(1) %1229, i64 %295
  %1231 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1232 = getelementptr half, ptr addrspace(1) %1231, i64 %246
  %1233 = getelementptr half, ptr addrspace(1) %1232, i64 %295
  %1234 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1235 = getelementptr half, ptr addrspace(1) %1234, i64 %246
  %1236 = getelementptr half, ptr addrspace(1) %1235, i64 %295
  %1237 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1238 = getelementptr half, ptr addrspace(1) %1237, i64 %247
  %1239 = getelementptr half, ptr addrspace(1) %1238, i64 %295
  %1240 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1241 = getelementptr half, ptr addrspace(1) %1240, i64 %247
  %1242 = getelementptr half, ptr addrspace(1) %1241, i64 %295
  %1243 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1244 = getelementptr half, ptr addrspace(1) %1243, i64 %248
  %1245 = getelementptr half, ptr addrspace(1) %1244, i64 %295
  %1246 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1247 = getelementptr half, ptr addrspace(1) %1246, i64 %248
  %1248 = getelementptr half, ptr addrspace(1) %1247, i64 %295
  %1249 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1250 = getelementptr half, ptr addrspace(1) %1249, i64 %249
  %1251 = getelementptr half, ptr addrspace(1) %1250, i64 %295
  %1252 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1253 = getelementptr half, ptr addrspace(1) %1252, i64 %249
  %1254 = getelementptr half, ptr addrspace(1) %1253, i64 %295
  %1255 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1256 = getelementptr half, ptr addrspace(1) %1255, i64 %250
  %1257 = getelementptr half, ptr addrspace(1) %1256, i64 %295
  %1258 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1259 = getelementptr half, ptr addrspace(1) %1258, i64 %250
  %1260 = getelementptr half, ptr addrspace(1) %1259, i64 %295
  %1261 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1262 = getelementptr half, ptr addrspace(1) %1261, i64 %251
  %1263 = getelementptr half, ptr addrspace(1) %1262, i64 %295
  %1264 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1265 = getelementptr half, ptr addrspace(1) %1264, i64 %251
  %1266 = getelementptr half, ptr addrspace(1) %1265, i64 %295
  %1267 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1268 = getelementptr half, ptr addrspace(1) %1267, i64 %252
  %1269 = getelementptr half, ptr addrspace(1) %1268, i64 %295
  %1270 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1271 = getelementptr half, ptr addrspace(1) %1270, i64 %252
  %1272 = getelementptr half, ptr addrspace(1) %1271, i64 %295
  %1273 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1274 = getelementptr half, ptr addrspace(1) %1273, i64 %253
  %1275 = getelementptr half, ptr addrspace(1) %1274, i64 %295
  %1276 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1277 = getelementptr half, ptr addrspace(1) %1276, i64 %253
  %1278 = getelementptr half, ptr addrspace(1) %1277, i64 %295
  %1279 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1280 = getelementptr half, ptr addrspace(1) %1279, i64 %254
  %1281 = getelementptr half, ptr addrspace(1) %1280, i64 %295
  %1282 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1283 = getelementptr half, ptr addrspace(1) %1282, i64 %254
  %1284 = getelementptr half, ptr addrspace(1) %1283, i64 %295
  %1285 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1286 = getelementptr half, ptr addrspace(1) %1285, i64 %255
  %1287 = getelementptr half, ptr addrspace(1) %1286, i64 %295
  %1288 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1289 = getelementptr half, ptr addrspace(1) %1288, i64 %255
  %1290 = getelementptr half, ptr addrspace(1) %1289, i64 %295
  %1291 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1292 = getelementptr half, ptr addrspace(1) %1291, i64 %256
  %1293 = getelementptr half, ptr addrspace(1) %1292, i64 %295
  %1294 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1295 = getelementptr half, ptr addrspace(1) %1294, i64 %256
  %1296 = getelementptr half, ptr addrspace(1) %1295, i64 %295
  %1297 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1298 = getelementptr half, ptr addrspace(1) %1297, i64 %257
  %1299 = getelementptr half, ptr addrspace(1) %1298, i64 %295
  %1300 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1301 = getelementptr half, ptr addrspace(1) %1300, i64 %257
  %1302 = getelementptr half, ptr addrspace(1) %1301, i64 %295
  %1303 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1304 = getelementptr half, ptr addrspace(1) %1303, i64 %258
  %1305 = getelementptr half, ptr addrspace(1) %1304, i64 %295
  %1306 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1307 = getelementptr half, ptr addrspace(1) %1306, i64 %258
  %1308 = getelementptr half, ptr addrspace(1) %1307, i64 %295
  %1309 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1310 = getelementptr half, ptr addrspace(1) %1309, i64 %259
  %1311 = getelementptr half, ptr addrspace(1) %1310, i64 %295
  %1312 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1313 = getelementptr half, ptr addrspace(1) %1312, i64 %259
  %1314 = getelementptr half, ptr addrspace(1) %1313, i64 %295
  %1315 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1316 = getelementptr half, ptr addrspace(1) %1315, i64 %260
  %1317 = getelementptr half, ptr addrspace(1) %1316, i64 %295
  %1318 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1319 = getelementptr half, ptr addrspace(1) %1318, i64 %260
  %1320 = getelementptr half, ptr addrspace(1) %1319, i64 %295
  %1321 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1322 = getelementptr half, ptr addrspace(1) %1321, i64 %261
  %1323 = getelementptr half, ptr addrspace(1) %1322, i64 %295
  %1324 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1325 = getelementptr half, ptr addrspace(1) %1324, i64 %261
  %1326 = getelementptr half, ptr addrspace(1) %1325, i64 %295
  %1327 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1328 = getelementptr half, ptr addrspace(1) %1327, i64 %262
  %1329 = getelementptr half, ptr addrspace(1) %1328, i64 %295
  %1330 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1331 = getelementptr half, ptr addrspace(1) %1330, i64 %262
  %1332 = getelementptr half, ptr addrspace(1) %1331, i64 %295
  %1333 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1334 = getelementptr half, ptr addrspace(1) %1333, i64 %263
  %1335 = getelementptr half, ptr addrspace(1) %1334, i64 %295
  %1336 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1337 = getelementptr half, ptr addrspace(1) %1336, i64 %263
  %1338 = getelementptr half, ptr addrspace(1) %1337, i64 %295
  %1339 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1340 = getelementptr half, ptr addrspace(1) %1339, i64 %264
  %1341 = getelementptr half, ptr addrspace(1) %1340, i64 %295
  %1342 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1343 = getelementptr half, ptr addrspace(1) %1342, i64 %264
  %1344 = getelementptr half, ptr addrspace(1) %1343, i64 %295
  %1345 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1346 = getelementptr half, ptr addrspace(1) %1345, i64 %265
  %1347 = getelementptr half, ptr addrspace(1) %1346, i64 %295
  %1348 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1349 = getelementptr half, ptr addrspace(1) %1348, i64 %265
  %1350 = getelementptr half, ptr addrspace(1) %1349, i64 %295
  %1351 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1352 = getelementptr half, ptr addrspace(1) %1351, i64 %266
  %1353 = getelementptr half, ptr addrspace(1) %1352, i64 %295
  %1354 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1355 = getelementptr half, ptr addrspace(1) %1354, i64 %266
  %1356 = getelementptr half, ptr addrspace(1) %1355, i64 %295
  %1357 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1358 = getelementptr half, ptr addrspace(1) %1357, i64 %267
  %1359 = getelementptr half, ptr addrspace(1) %1358, i64 %295
  %1360 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1361 = getelementptr half, ptr addrspace(1) %1360, i64 %267
  %1362 = getelementptr half, ptr addrspace(1) %1361, i64 %295
  %1363 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1364 = getelementptr half, ptr addrspace(1) %1363, i64 %268
  %1365 = getelementptr half, ptr addrspace(1) %1364, i64 %295
  %1366 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1367 = getelementptr half, ptr addrspace(1) %1366, i64 %268
  %1368 = getelementptr half, ptr addrspace(1) %1367, i64 %295
  %1369 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1370 = getelementptr half, ptr addrspace(1) %1369, i64 %269
  %1371 = getelementptr half, ptr addrspace(1) %1370, i64 %295
  %1372 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1373 = getelementptr half, ptr addrspace(1) %1372, i64 %269
  %1374 = getelementptr half, ptr addrspace(1) %1373, i64 %295
  %1375 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1376 = getelementptr half, ptr addrspace(1) %1375, i64 %270
  %1377 = getelementptr half, ptr addrspace(1) %1376, i64 %295
  %1378 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1379 = getelementptr half, ptr addrspace(1) %1378, i64 %270
  %1380 = getelementptr half, ptr addrspace(1) %1379, i64 %295
  %1381 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1382 = getelementptr half, ptr addrspace(1) %1381, i64 %271
  %1383 = getelementptr half, ptr addrspace(1) %1382, i64 %295
  %1384 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1385 = getelementptr half, ptr addrspace(1) %1384, i64 %271
  %1386 = getelementptr half, ptr addrspace(1) %1385, i64 %295
  %1387 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1388 = getelementptr half, ptr addrspace(1) %1387, i64 %272
  %1389 = getelementptr half, ptr addrspace(1) %1388, i64 %295
  %1390 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1391 = getelementptr half, ptr addrspace(1) %1390, i64 %272
  %1392 = getelementptr half, ptr addrspace(1) %1391, i64 %295
  %1393 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1394 = getelementptr half, ptr addrspace(1) %1393, i64 %273
  %1395 = getelementptr half, ptr addrspace(1) %1394, i64 %295
  %1396 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1397 = getelementptr half, ptr addrspace(1) %1396, i64 %273
  %1398 = getelementptr half, ptr addrspace(1) %1397, i64 %295
  %1399 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1400 = getelementptr half, ptr addrspace(1) %1399, i64 %274
  %1401 = getelementptr half, ptr addrspace(1) %1400, i64 %295
  %1402 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1403 = getelementptr half, ptr addrspace(1) %1402, i64 %274
  %1404 = getelementptr half, ptr addrspace(1) %1403, i64 %295
  %1405 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1406 = getelementptr half, ptr addrspace(1) %1405, i64 %275
  %1407 = getelementptr half, ptr addrspace(1) %1406, i64 %295
  %1408 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1409 = getelementptr half, ptr addrspace(1) %1408, i64 %275
  %1410 = getelementptr half, ptr addrspace(1) %1409, i64 %295
  %1411 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1412 = getelementptr half, ptr addrspace(1) %1411, i64 %276
  %1413 = getelementptr half, ptr addrspace(1) %1412, i64 %295
  %1414 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1415 = getelementptr half, ptr addrspace(1) %1414, i64 %276
  %1416 = getelementptr half, ptr addrspace(1) %1415, i64 %295
  %1417 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1418 = getelementptr half, ptr addrspace(1) %1417, i64 %277
  %1419 = getelementptr half, ptr addrspace(1) %1418, i64 %295
  %1420 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1421 = getelementptr half, ptr addrspace(1) %1420, i64 %277
  %1422 = getelementptr half, ptr addrspace(1) %1421, i64 %295
  %1423 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1424 = getelementptr half, ptr addrspace(1) %1423, i64 %278
  %1425 = getelementptr half, ptr addrspace(1) %1424, i64 %295
  %1426 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1427 = getelementptr half, ptr addrspace(1) %1426, i64 %278
  %1428 = getelementptr half, ptr addrspace(1) %1427, i64 %295
  %1429 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1430 = getelementptr half, ptr addrspace(1) %1429, i64 %279
  %1431 = getelementptr half, ptr addrspace(1) %1430, i64 %295
  %1432 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1433 = getelementptr half, ptr addrspace(1) %1432, i64 %279
  %1434 = getelementptr half, ptr addrspace(1) %1433, i64 %295
  %1435 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1436 = getelementptr half, ptr addrspace(1) %1435, i64 %280
  %1437 = getelementptr half, ptr addrspace(1) %1436, i64 %295
  %1438 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1439 = getelementptr half, ptr addrspace(1) %1438, i64 %280
  %1440 = getelementptr half, ptr addrspace(1) %1439, i64 %295
  %1441 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1442 = getelementptr half, ptr addrspace(1) %1441, i64 %281
  %1443 = getelementptr half, ptr addrspace(1) %1442, i64 %295
  %1444 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1445 = getelementptr half, ptr addrspace(1) %1444, i64 %281
  %1446 = getelementptr half, ptr addrspace(1) %1445, i64 %295
  %1447 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1448 = getelementptr half, ptr addrspace(1) %1447, i64 %282
  %1449 = getelementptr half, ptr addrspace(1) %1448, i64 %295
  %1450 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1451 = getelementptr half, ptr addrspace(1) %1450, i64 %282
  %1452 = getelementptr half, ptr addrspace(1) %1451, i64 %295
  %1453 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1454 = getelementptr half, ptr addrspace(1) %1453, i64 %283
  %1455 = getelementptr half, ptr addrspace(1) %1454, i64 %295
  %1456 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1457 = getelementptr half, ptr addrspace(1) %1456, i64 %283
  %1458 = getelementptr half, ptr addrspace(1) %1457, i64 %295
  %1459 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1460 = getelementptr half, ptr addrspace(1) %1459, i64 %284
  %1461 = getelementptr half, ptr addrspace(1) %1460, i64 %295
  %1462 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1463 = getelementptr half, ptr addrspace(1) %1462, i64 %284
  %1464 = getelementptr half, ptr addrspace(1) %1463, i64 %295
  %1465 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1466 = getelementptr half, ptr addrspace(1) %1465, i64 %285
  %1467 = getelementptr half, ptr addrspace(1) %1466, i64 %295
  %1468 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1469 = getelementptr half, ptr addrspace(1) %1468, i64 %285
  %1470 = getelementptr half, ptr addrspace(1) %1469, i64 %295
  %1471 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1472 = getelementptr half, ptr addrspace(1) %1471, i64 %286
  %1473 = getelementptr half, ptr addrspace(1) %1472, i64 %295
  %1474 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1475 = getelementptr half, ptr addrspace(1) %1474, i64 %286
  %1476 = getelementptr half, ptr addrspace(1) %1475, i64 %295
  %1477 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1478 = getelementptr half, ptr addrspace(1) %1477, i64 %287
  %1479 = getelementptr half, ptr addrspace(1) %1478, i64 %295
  %1480 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1481 = getelementptr half, ptr addrspace(1) %1480, i64 %287
  %1482 = getelementptr half, ptr addrspace(1) %1481, i64 %295
  %1483 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1484 = getelementptr half, ptr addrspace(1) %1483, i64 %288
  %1485 = getelementptr half, ptr addrspace(1) %1484, i64 %295
  %1486 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1487 = getelementptr half, ptr addrspace(1) %1486, i64 %288
  %1488 = getelementptr half, ptr addrspace(1) %1487, i64 %295
  %1489 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1490 = getelementptr half, ptr addrspace(1) %1489, i64 %289
  %1491 = getelementptr half, ptr addrspace(1) %1490, i64 %295
  %1492 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1493 = getelementptr half, ptr addrspace(1) %1492, i64 %289
  %1494 = getelementptr half, ptr addrspace(1) %1493, i64 %295
  %1495 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1496 = getelementptr half, ptr addrspace(1) %1495, i64 %290
  %1497 = getelementptr half, ptr addrspace(1) %1496, i64 %295
  %1498 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1499 = getelementptr half, ptr addrspace(1) %1498, i64 %290
  %1500 = getelementptr half, ptr addrspace(1) %1499, i64 %295
  %1501 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1502 = getelementptr half, ptr addrspace(1) %1501, i64 %291
  %1503 = getelementptr half, ptr addrspace(1) %1502, i64 %295
  %1504 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1505 = getelementptr half, ptr addrspace(1) %1504, i64 %291
  %1506 = getelementptr half, ptr addrspace(1) %1505, i64 %295
  %1507 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1508 = getelementptr half, ptr addrspace(1) %1507, i64 %292
  %1509 = getelementptr half, ptr addrspace(1) %1508, i64 %295
  %1510 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1511 = getelementptr half, ptr addrspace(1) %1510, i64 %292
  %1512 = getelementptr half, ptr addrspace(1) %1511, i64 %295
  %1513 = bitcast ptr addrspace(1) %1133 to ptr addrspace(1)
  %1514 = getelementptr half, ptr addrspace(1) %1513, i64 %293
  %1515 = getelementptr half, ptr addrspace(1) %1514, i64 %295
  %1516 = bitcast ptr addrspace(1) %1137 to ptr addrspace(1)
  %1517 = getelementptr half, ptr addrspace(1) %1516, i64 %293
  %1518 = getelementptr half, ptr addrspace(1) %1517, i64 %295
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1519 = ptrtoint ptr addrspace(1) %1136 to i64
  %1520 = ptrtoint ptr addrspace(1) %1140 to i64
  %1521 = ptrtoint ptr addrspace(1) %1143 to i64
  %1522 = ptrtoint ptr addrspace(1) %1146 to i64
  %1523 = ptrtoint ptr addrspace(1) %1149 to i64
  %1524 = ptrtoint ptr addrspace(1) %1152 to i64
  %1525 = ptrtoint ptr addrspace(1) %1155 to i64
  %1526 = ptrtoint ptr addrspace(1) %1158 to i64
  %1527 = ptrtoint ptr addrspace(1) %1161 to i64
  %1528 = ptrtoint ptr addrspace(1) %1164 to i64
  %1529 = ptrtoint ptr addrspace(1) %1167 to i64
  %1530 = ptrtoint ptr addrspace(1) %1170 to i64
  %1531 = ptrtoint ptr addrspace(1) %1173 to i64
  %1532 = ptrtoint ptr addrspace(1) %1176 to i64
  %1533 = ptrtoint ptr addrspace(1) %1179 to i64
  %1534 = ptrtoint ptr addrspace(1) %1182 to i64
  %1535 = ptrtoint ptr addrspace(1) %1185 to i64
  %1536 = ptrtoint ptr addrspace(1) %1188 to i64
  %1537 = ptrtoint ptr addrspace(1) %1191 to i64
  %1538 = ptrtoint ptr addrspace(1) %1194 to i64
  %1539 = ptrtoint ptr addrspace(1) %1197 to i64
  %1540 = ptrtoint ptr addrspace(1) %1200 to i64
  %1541 = ptrtoint ptr addrspace(1) %1203 to i64
  %1542 = ptrtoint ptr addrspace(1) %1206 to i64
  %1543 = ptrtoint ptr addrspace(1) %1209 to i64
  %1544 = ptrtoint ptr addrspace(1) %1212 to i64
  %1545 = ptrtoint ptr addrspace(1) %1215 to i64
  %1546 = ptrtoint ptr addrspace(1) %1218 to i64
  %1547 = ptrtoint ptr addrspace(1) %1221 to i64
  %1548 = ptrtoint ptr addrspace(1) %1224 to i64
  %1549 = ptrtoint ptr addrspace(1) %1227 to i64
  %1550 = ptrtoint ptr addrspace(1) %1230 to i64
  %1551 = ptrtoint ptr addrspace(1) %1233 to i64
  %1552 = ptrtoint ptr addrspace(1) %1236 to i64
  %1553 = ptrtoint ptr addrspace(1) %1239 to i64
  %1554 = ptrtoint ptr addrspace(1) %1242 to i64
  %1555 = ptrtoint ptr addrspace(1) %1245 to i64
  %1556 = ptrtoint ptr addrspace(1) %1248 to i64
  %1557 = ptrtoint ptr addrspace(1) %1251 to i64
  %1558 = ptrtoint ptr addrspace(1) %1254 to i64
  %1559 = ptrtoint ptr addrspace(1) %1257 to i64
  %1560 = ptrtoint ptr addrspace(1) %1260 to i64
  %1561 = ptrtoint ptr addrspace(1) %1263 to i64
  %1562 = ptrtoint ptr addrspace(1) %1266 to i64
  %1563 = ptrtoint ptr addrspace(1) %1269 to i64
  %1564 = ptrtoint ptr addrspace(1) %1272 to i64
  %1565 = ptrtoint ptr addrspace(1) %1275 to i64
  %1566 = ptrtoint ptr addrspace(1) %1278 to i64
  %1567 = ptrtoint ptr addrspace(1) %1281 to i64
  %1568 = ptrtoint ptr addrspace(1) %1284 to i64
  %1569 = ptrtoint ptr addrspace(1) %1287 to i64
  %1570 = ptrtoint ptr addrspace(1) %1290 to i64
  %1571 = ptrtoint ptr addrspace(1) %1293 to i64
  %1572 = ptrtoint ptr addrspace(1) %1296 to i64
  %1573 = ptrtoint ptr addrspace(1) %1299 to i64
  %1574 = ptrtoint ptr addrspace(1) %1302 to i64
  %1575 = ptrtoint ptr addrspace(1) %1305 to i64
  %1576 = ptrtoint ptr addrspace(1) %1308 to i64
  %1577 = ptrtoint ptr addrspace(1) %1311 to i64
  %1578 = ptrtoint ptr addrspace(1) %1314 to i64
  %1579 = ptrtoint ptr addrspace(1) %1317 to i64
  %1580 = ptrtoint ptr addrspace(1) %1320 to i64
  %1581 = ptrtoint ptr addrspace(1) %1323 to i64
  %1582 = ptrtoint ptr addrspace(1) %1326 to i64
  %1583 = ptrtoint ptr addrspace(1) %1329 to i64
  %1584 = ptrtoint ptr addrspace(1) %1332 to i64
  %1585 = ptrtoint ptr addrspace(1) %1335 to i64
  %1586 = ptrtoint ptr addrspace(1) %1338 to i64
  %1587 = ptrtoint ptr addrspace(1) %1341 to i64
  %1588 = ptrtoint ptr addrspace(1) %1344 to i64
  %1589 = ptrtoint ptr addrspace(1) %1347 to i64
  %1590 = ptrtoint ptr addrspace(1) %1350 to i64
  %1591 = ptrtoint ptr addrspace(1) %1353 to i64
  %1592 = ptrtoint ptr addrspace(1) %1356 to i64
  %1593 = ptrtoint ptr addrspace(1) %1359 to i64
  %1594 = ptrtoint ptr addrspace(1) %1362 to i64
  %1595 = ptrtoint ptr addrspace(1) %1365 to i64
  %1596 = ptrtoint ptr addrspace(1) %1368 to i64
  %1597 = ptrtoint ptr addrspace(1) %1371 to i64
  %1598 = ptrtoint ptr addrspace(1) %1374 to i64
  %1599 = ptrtoint ptr addrspace(1) %1377 to i64
  %1600 = ptrtoint ptr addrspace(1) %1380 to i64
  %1601 = ptrtoint ptr addrspace(1) %1383 to i64
  %1602 = ptrtoint ptr addrspace(1) %1386 to i64
  %1603 = ptrtoint ptr addrspace(1) %1389 to i64
  %1604 = ptrtoint ptr addrspace(1) %1392 to i64
  %1605 = ptrtoint ptr addrspace(1) %1395 to i64
  %1606 = ptrtoint ptr addrspace(1) %1398 to i64
  %1607 = ptrtoint ptr addrspace(1) %1401 to i64
  %1608 = ptrtoint ptr addrspace(1) %1404 to i64
  %1609 = ptrtoint ptr addrspace(1) %1407 to i64
  %1610 = ptrtoint ptr addrspace(1) %1410 to i64
  %1611 = ptrtoint ptr addrspace(1) %1413 to i64
  %1612 = ptrtoint ptr addrspace(1) %1416 to i64
  %1613 = ptrtoint ptr addrspace(1) %1419 to i64
  %1614 = ptrtoint ptr addrspace(1) %1422 to i64
  %1615 = ptrtoint ptr addrspace(1) %1425 to i64
  %1616 = ptrtoint ptr addrspace(1) %1428 to i64
  %1617 = ptrtoint ptr addrspace(1) %1431 to i64
  %1618 = ptrtoint ptr addrspace(1) %1434 to i64
  %1619 = ptrtoint ptr addrspace(1) %1437 to i64
  %1620 = ptrtoint ptr addrspace(1) %1440 to i64
  %1621 = ptrtoint ptr addrspace(1) %1443 to i64
  %1622 = ptrtoint ptr addrspace(1) %1446 to i64
  %1623 = ptrtoint ptr addrspace(1) %1449 to i64
  %1624 = ptrtoint ptr addrspace(1) %1452 to i64
  %1625 = ptrtoint ptr addrspace(1) %1455 to i64
  %1626 = ptrtoint ptr addrspace(1) %1458 to i64
  %1627 = ptrtoint ptr addrspace(1) %1461 to i64
  %1628 = ptrtoint ptr addrspace(1) %1464 to i64
  %1629 = ptrtoint ptr addrspace(1) %1467 to i64
  %1630 = ptrtoint ptr addrspace(1) %1470 to i64
  %1631 = ptrtoint ptr addrspace(1) %1473 to i64
  %1632 = ptrtoint ptr addrspace(1) %1476 to i64
  %1633 = ptrtoint ptr addrspace(1) %1479 to i64
  %1634 = ptrtoint ptr addrspace(1) %1482 to i64
  %1635 = ptrtoint ptr addrspace(1) %1485 to i64
  %1636 = ptrtoint ptr addrspace(1) %1488 to i64
  %1637 = ptrtoint ptr addrspace(1) %1491 to i64
  %1638 = ptrtoint ptr addrspace(1) %1494 to i64
  %1639 = ptrtoint ptr addrspace(1) %1497 to i64
  %1640 = ptrtoint ptr addrspace(1) %1500 to i64
  %1641 = ptrtoint ptr addrspace(1) %1503 to i64
  %1642 = ptrtoint ptr addrspace(1) %1506 to i64
  %1643 = ptrtoint ptr addrspace(1) %1509 to i64
  %1644 = ptrtoint ptr addrspace(1) %1512 to i64
  %1645 = ptrtoint ptr addrspace(1) %1515 to i64
  %1646 = ptrtoint ptr addrspace(1) %1518 to i64
  %1647 = insertelement <2 x i64> undef, i64 %1519, i32 0
  %1648 = insertelement <2 x i64> %1647, i64 %1521, i32 1
  %1649 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %1648, ptr addrspace(3) %1649, align 16
  %1650 = insertelement <2 x i64> undef, i64 %1520, i32 0
  %1651 = insertelement <2 x i64> %1650, i64 %1522, i32 1
  %1652 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %1651, ptr addrspace(3) %1652, align 16
  %1653 = insertelement <2 x i64> undef, i64 %1535, i32 0
  %1654 = insertelement <2 x i64> %1653, i64 %1537, i32 1
  %1655 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %1654, ptr addrspace(3) %1655, align 16
  %1656 = insertelement <2 x i64> undef, i64 %1536, i32 0
  %1657 = insertelement <2 x i64> %1656, i64 %1538, i32 1
  %1658 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %1657, ptr addrspace(3) %1658, align 16
  %1659 = insertelement <2 x i64> undef, i64 %1551, i32 0
  %1660 = insertelement <2 x i64> %1659, i64 %1553, i32 1
  %1661 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %1660, ptr addrspace(3) %1661, align 16
  %1662 = insertelement <2 x i64> undef, i64 %1552, i32 0
  %1663 = insertelement <2 x i64> %1662, i64 %1554, i32 1
  %1664 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %1663, ptr addrspace(3) %1664, align 16
  %1665 = insertelement <2 x i64> undef, i64 %1567, i32 0
  %1666 = insertelement <2 x i64> %1665, i64 %1569, i32 1
  %1667 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %1666, ptr addrspace(3) %1667, align 16
  %1668 = insertelement <2 x i64> undef, i64 %1568, i32 0
  %1669 = insertelement <2 x i64> %1668, i64 %1570, i32 1
  %1670 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %1669, ptr addrspace(3) %1670, align 16
  %1671 = insertelement <2 x i64> undef, i64 %1583, i32 0
  %1672 = insertelement <2 x i64> %1671, i64 %1585, i32 1
  %1673 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %1672, ptr addrspace(3) %1673, align 16
  %1674 = insertelement <2 x i64> undef, i64 %1584, i32 0
  %1675 = insertelement <2 x i64> %1674, i64 %1586, i32 1
  %1676 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %1675, ptr addrspace(3) %1676, align 16
  %1677 = insertelement <2 x i64> undef, i64 %1599, i32 0
  %1678 = insertelement <2 x i64> %1677, i64 %1601, i32 1
  %1679 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %1678, ptr addrspace(3) %1679, align 16
  %1680 = insertelement <2 x i64> undef, i64 %1600, i32 0
  %1681 = insertelement <2 x i64> %1680, i64 %1602, i32 1
  %1682 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %1681, ptr addrspace(3) %1682, align 16
  %1683 = insertelement <2 x i64> undef, i64 %1615, i32 0
  %1684 = insertelement <2 x i64> %1683, i64 %1617, i32 1
  %1685 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %1684, ptr addrspace(3) %1685, align 16
  %1686 = insertelement <2 x i64> undef, i64 %1616, i32 0
  %1687 = insertelement <2 x i64> %1686, i64 %1618, i32 1
  %1688 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %1687, ptr addrspace(3) %1688, align 16
  %1689 = insertelement <2 x i64> undef, i64 %1631, i32 0
  %1690 = insertelement <2 x i64> %1689, i64 %1633, i32 1
  %1691 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %1690, ptr addrspace(3) %1691, align 16
  %1692 = insertelement <2 x i64> undef, i64 %1632, i32 0
  %1693 = insertelement <2 x i64> %1692, i64 %1634, i32 1
  %1694 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %1693, ptr addrspace(3) %1694, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1695 = bitcast ptr addrspace(3) %903 to ptr addrspace(3)
  %1696 = load <2 x i64>, ptr addrspace(3) %1695, align 16
  %1697 = bitcast ptr addrspace(3) %906 to ptr addrspace(3)
  %1698 = load i64, ptr addrspace(3) %1697, align 16
  %1699 = bitcast ptr addrspace(3) %911 to ptr addrspace(3)
  %1700 = load i64, ptr addrspace(3) %1699, align 16
  %1701 = bitcast ptr addrspace(3) %912 to ptr addrspace(3)
  %1702 = load i64, ptr addrspace(3) %1701, align 16
  %1703 = bitcast ptr addrspace(3) %915 to ptr addrspace(3)
  %1704 = load i64, ptr addrspace(3) %1703, align 16
  %1705 = bitcast ptr addrspace(3) %916 to ptr addrspace(3)
  %1706 = load i64, ptr addrspace(3) %1705, align 16
  %1707 = bitcast ptr addrspace(3) %919 to ptr addrspace(3)
  %1708 = load i64, ptr addrspace(3) %1707, align 16
  %1709 = bitcast ptr addrspace(3) %920 to ptr addrspace(3)
  %1710 = load i64, ptr addrspace(3) %1709, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1711 = insertelement <2 x i64> undef, i64 %1523, i32 0
  %1712 = insertelement <2 x i64> %1711, i64 %1525, i32 1
  %1713 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %1712, ptr addrspace(3) %1713, align 16
  %1714 = insertelement <2 x i64> undef, i64 %1524, i32 0
  %1715 = insertelement <2 x i64> %1714, i64 %1526, i32 1
  %1716 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %1715, ptr addrspace(3) %1716, align 16
  %1717 = insertelement <2 x i64> undef, i64 %1539, i32 0
  %1718 = insertelement <2 x i64> %1717, i64 %1541, i32 1
  %1719 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %1718, ptr addrspace(3) %1719, align 16
  %1720 = insertelement <2 x i64> undef, i64 %1540, i32 0
  %1721 = insertelement <2 x i64> %1720, i64 %1542, i32 1
  %1722 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %1721, ptr addrspace(3) %1722, align 16
  %1723 = insertelement <2 x i64> undef, i64 %1555, i32 0
  %1724 = insertelement <2 x i64> %1723, i64 %1557, i32 1
  %1725 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %1724, ptr addrspace(3) %1725, align 16
  %1726 = insertelement <2 x i64> undef, i64 %1556, i32 0
  %1727 = insertelement <2 x i64> %1726, i64 %1558, i32 1
  %1728 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %1727, ptr addrspace(3) %1728, align 16
  %1729 = insertelement <2 x i64> undef, i64 %1571, i32 0
  %1730 = insertelement <2 x i64> %1729, i64 %1573, i32 1
  %1731 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %1730, ptr addrspace(3) %1731, align 16
  %1732 = insertelement <2 x i64> undef, i64 %1572, i32 0
  %1733 = insertelement <2 x i64> %1732, i64 %1574, i32 1
  %1734 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %1733, ptr addrspace(3) %1734, align 16
  %1735 = insertelement <2 x i64> undef, i64 %1587, i32 0
  %1736 = insertelement <2 x i64> %1735, i64 %1589, i32 1
  %1737 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %1736, ptr addrspace(3) %1737, align 16
  %1738 = insertelement <2 x i64> undef, i64 %1588, i32 0
  %1739 = insertelement <2 x i64> %1738, i64 %1590, i32 1
  %1740 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %1739, ptr addrspace(3) %1740, align 16
  %1741 = insertelement <2 x i64> undef, i64 %1603, i32 0
  %1742 = insertelement <2 x i64> %1741, i64 %1605, i32 1
  %1743 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %1742, ptr addrspace(3) %1743, align 16
  %1744 = insertelement <2 x i64> undef, i64 %1604, i32 0
  %1745 = insertelement <2 x i64> %1744, i64 %1606, i32 1
  %1746 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %1745, ptr addrspace(3) %1746, align 16
  %1747 = insertelement <2 x i64> undef, i64 %1619, i32 0
  %1748 = insertelement <2 x i64> %1747, i64 %1621, i32 1
  %1749 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %1748, ptr addrspace(3) %1749, align 16
  %1750 = insertelement <2 x i64> undef, i64 %1620, i32 0
  %1751 = insertelement <2 x i64> %1750, i64 %1622, i32 1
  %1752 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %1751, ptr addrspace(3) %1752, align 16
  %1753 = insertelement <2 x i64> undef, i64 %1635, i32 0
  %1754 = insertelement <2 x i64> %1753, i64 %1637, i32 1
  %1755 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %1754, ptr addrspace(3) %1755, align 16
  %1756 = insertelement <2 x i64> undef, i64 %1636, i32 0
  %1757 = insertelement <2 x i64> %1756, i64 %1638, i32 1
  %1758 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %1757, ptr addrspace(3) %1758, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1759 = insertelement <2 x i64> undef, i64 %1527, i32 0
  %1760 = insertelement <2 x i64> %1759, i64 %1529, i32 1
  %1761 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %1760, ptr addrspace(3) %1761, align 16
  %1762 = insertelement <2 x i64> undef, i64 %1528, i32 0
  %1763 = insertelement <2 x i64> %1762, i64 %1530, i32 1
  %1764 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %1763, ptr addrspace(3) %1764, align 16
  %1765 = insertelement <2 x i64> undef, i64 %1543, i32 0
  %1766 = insertelement <2 x i64> %1765, i64 %1545, i32 1
  %1767 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %1766, ptr addrspace(3) %1767, align 16
  %1768 = insertelement <2 x i64> undef, i64 %1544, i32 0
  %1769 = insertelement <2 x i64> %1768, i64 %1546, i32 1
  %1770 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %1769, ptr addrspace(3) %1770, align 16
  %1771 = insertelement <2 x i64> undef, i64 %1559, i32 0
  %1772 = insertelement <2 x i64> %1771, i64 %1561, i32 1
  %1773 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %1772, ptr addrspace(3) %1773, align 16
  %1774 = insertelement <2 x i64> undef, i64 %1560, i32 0
  %1775 = insertelement <2 x i64> %1774, i64 %1562, i32 1
  %1776 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %1775, ptr addrspace(3) %1776, align 16
  %1777 = insertelement <2 x i64> undef, i64 %1575, i32 0
  %1778 = insertelement <2 x i64> %1777, i64 %1577, i32 1
  %1779 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %1778, ptr addrspace(3) %1779, align 16
  %1780 = insertelement <2 x i64> undef, i64 %1576, i32 0
  %1781 = insertelement <2 x i64> %1780, i64 %1578, i32 1
  %1782 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %1781, ptr addrspace(3) %1782, align 16
  %1783 = insertelement <2 x i64> undef, i64 %1591, i32 0
  %1784 = insertelement <2 x i64> %1783, i64 %1593, i32 1
  %1785 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %1784, ptr addrspace(3) %1785, align 16
  %1786 = insertelement <2 x i64> undef, i64 %1592, i32 0
  %1787 = insertelement <2 x i64> %1786, i64 %1594, i32 1
  %1788 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %1787, ptr addrspace(3) %1788, align 16
  %1789 = insertelement <2 x i64> undef, i64 %1607, i32 0
  %1790 = insertelement <2 x i64> %1789, i64 %1609, i32 1
  %1791 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %1790, ptr addrspace(3) %1791, align 16
  %1792 = insertelement <2 x i64> undef, i64 %1608, i32 0
  %1793 = insertelement <2 x i64> %1792, i64 %1610, i32 1
  %1794 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %1793, ptr addrspace(3) %1794, align 16
  %1795 = insertelement <2 x i64> undef, i64 %1623, i32 0
  %1796 = insertelement <2 x i64> %1795, i64 %1625, i32 1
  %1797 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %1796, ptr addrspace(3) %1797, align 16
  %1798 = insertelement <2 x i64> undef, i64 %1624, i32 0
  %1799 = insertelement <2 x i64> %1798, i64 %1626, i32 1
  %1800 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %1799, ptr addrspace(3) %1800, align 16
  %1801 = insertelement <2 x i64> undef, i64 %1639, i32 0
  %1802 = insertelement <2 x i64> %1801, i64 %1641, i32 1
  %1803 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %1802, ptr addrspace(3) %1803, align 16
  %1804 = insertelement <2 x i64> undef, i64 %1640, i32 0
  %1805 = insertelement <2 x i64> %1804, i64 %1642, i32 1
  %1806 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %1805, ptr addrspace(3) %1806, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1807 = insertelement <2 x i64> undef, i64 %1531, i32 0
  %1808 = insertelement <2 x i64> %1807, i64 %1533, i32 1
  %1809 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %1808, ptr addrspace(3) %1809, align 16
  %1810 = insertelement <2 x i64> undef, i64 %1532, i32 0
  %1811 = insertelement <2 x i64> %1810, i64 %1534, i32 1
  %1812 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %1811, ptr addrspace(3) %1812, align 16
  %1813 = insertelement <2 x i64> undef, i64 %1547, i32 0
  %1814 = insertelement <2 x i64> %1813, i64 %1549, i32 1
  %1815 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %1814, ptr addrspace(3) %1815, align 16
  %1816 = insertelement <2 x i64> undef, i64 %1548, i32 0
  %1817 = insertelement <2 x i64> %1816, i64 %1550, i32 1
  %1818 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %1817, ptr addrspace(3) %1818, align 16
  %1819 = insertelement <2 x i64> undef, i64 %1563, i32 0
  %1820 = insertelement <2 x i64> %1819, i64 %1565, i32 1
  %1821 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %1820, ptr addrspace(3) %1821, align 16
  %1822 = insertelement <2 x i64> undef, i64 %1564, i32 0
  %1823 = insertelement <2 x i64> %1822, i64 %1566, i32 1
  %1824 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %1823, ptr addrspace(3) %1824, align 16
  %1825 = insertelement <2 x i64> undef, i64 %1579, i32 0
  %1826 = insertelement <2 x i64> %1825, i64 %1581, i32 1
  %1827 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %1826, ptr addrspace(3) %1827, align 16
  %1828 = insertelement <2 x i64> undef, i64 %1580, i32 0
  %1829 = insertelement <2 x i64> %1828, i64 %1582, i32 1
  %1830 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %1829, ptr addrspace(3) %1830, align 16
  %1831 = insertelement <2 x i64> undef, i64 %1595, i32 0
  %1832 = insertelement <2 x i64> %1831, i64 %1597, i32 1
  %1833 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %1832, ptr addrspace(3) %1833, align 16
  %1834 = insertelement <2 x i64> undef, i64 %1596, i32 0
  %1835 = insertelement <2 x i64> %1834, i64 %1598, i32 1
  %1836 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %1835, ptr addrspace(3) %1836, align 16
  %1837 = insertelement <2 x i64> undef, i64 %1611, i32 0
  %1838 = insertelement <2 x i64> %1837, i64 %1613, i32 1
  %1839 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %1838, ptr addrspace(3) %1839, align 16
  %1840 = insertelement <2 x i64> undef, i64 %1612, i32 0
  %1841 = insertelement <2 x i64> %1840, i64 %1614, i32 1
  %1842 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %1841, ptr addrspace(3) %1842, align 16
  %1843 = insertelement <2 x i64> undef, i64 %1627, i32 0
  %1844 = insertelement <2 x i64> %1843, i64 %1629, i32 1
  %1845 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %1844, ptr addrspace(3) %1845, align 16
  %1846 = insertelement <2 x i64> undef, i64 %1628, i32 0
  %1847 = insertelement <2 x i64> %1846, i64 %1630, i32 1
  %1848 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %1847, ptr addrspace(3) %1848, align 16
  %1849 = insertelement <2 x i64> undef, i64 %1643, i32 0
  %1850 = insertelement <2 x i64> %1849, i64 %1645, i32 1
  %1851 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %1850, ptr addrspace(3) %1851, align 16
  %1852 = insertelement <2 x i64> undef, i64 %1644, i32 0
  %1853 = insertelement <2 x i64> %1852, i64 %1646, i32 1
  %1854 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %1853, ptr addrspace(3) %1854, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %1855 = inttoptr i64 %1698 to ptr addrspace(1)
  %1856 = inttoptr i64 %1700 to ptr addrspace(1)
  %1857 = inttoptr i64 %1702 to ptr addrspace(1)
  %1858 = inttoptr i64 %1704 to ptr addrspace(1)
  %1859 = inttoptr i64 %1706 to ptr addrspace(1)
  %1860 = inttoptr i64 %1708 to ptr addrspace(1)
  %1861 = inttoptr i64 %1710 to ptr addrspace(1)
  br i1 %1116, label %1862, label %1866

1862:                                             ; preds = %1127
  %1863 = extractelement <2 x i64> %1696, i32 0
  %1864 = inttoptr i64 %1863 to ptr addrspace(1)
  %1865 = load <4 x i32>, ptr addrspace(1) %1864, align 16
  br label %1866

1866:                                             ; preds = %1862, %1127
  %1867 = phi <4 x i32> [ %1865, %1862 ], [ zeroinitializer, %1127 ]
  %1868 = extractelement <4 x i32> %1867, i32 0
  %1869 = bitcast i32 %1868 to <2 x half>
  %1870 = extractelement <4 x i32> %1867, i32 1
  %1871 = bitcast i32 %1870 to <2 x half>
  %1872 = extractelement <4 x i32> %1867, i32 2
  %1873 = bitcast i32 %1872 to <2 x half>
  %1874 = extractelement <4 x i32> %1867, i32 3
  %1875 = bitcast i32 %1874 to <2 x half>
  %1876 = extractelement <2 x half> %1869, i32 0
  %1877 = extractelement <2 x half> %1869, i32 1
  %1878 = extractelement <2 x half> %1871, i32 0
  %1879 = extractelement <2 x half> %1871, i32 1
  %1880 = extractelement <2 x half> %1873, i32 0
  %1881 = extractelement <2 x half> %1873, i32 1
  %1882 = extractelement <2 x half> %1875, i32 0
  %1883 = extractelement <2 x half> %1875, i32 1
  br i1 %1116, label %1884, label %1886

1884:                                             ; preds = %1866
  %1885 = load <4 x i32>, ptr addrspace(1) %1855, align 16
  br label %1886

1886:                                             ; preds = %1884, %1866
  %1887 = phi <4 x i32> [ %1885, %1884 ], [ zeroinitializer, %1866 ]
  %1888 = extractelement <4 x i32> %1887, i32 0
  %1889 = bitcast i32 %1888 to <2 x half>
  %1890 = extractelement <4 x i32> %1887, i32 1
  %1891 = bitcast i32 %1890 to <2 x half>
  %1892 = extractelement <4 x i32> %1887, i32 2
  %1893 = bitcast i32 %1892 to <2 x half>
  %1894 = extractelement <4 x i32> %1887, i32 3
  %1895 = bitcast i32 %1894 to <2 x half>
  %1896 = extractelement <2 x half> %1889, i32 0
  %1897 = extractelement <2 x half> %1889, i32 1
  %1898 = extractelement <2 x half> %1891, i32 0
  %1899 = extractelement <2 x half> %1891, i32 1
  %1900 = extractelement <2 x half> %1893, i32 0
  %1901 = extractelement <2 x half> %1893, i32 1
  %1902 = extractelement <2 x half> %1895, i32 0
  %1903 = extractelement <2 x half> %1895, i32 1
  br i1 %1117, label %1904, label %1911

1904:                                             ; preds = %1886
  %1905 = load <4 x i32>, ptr addrspace(1) %1856, align 16
  %1906 = load <4 x i32>, ptr addrspace(1) %1857, align 16
  %1907 = load <4 x i32>, ptr addrspace(1) %1858, align 16
  %1908 = load <4 x i32>, ptr addrspace(1) %1859, align 16
  %1909 = load <4 x i32>, ptr addrspace(1) %1860, align 16
  %1910 = load <4 x i32>, ptr addrspace(1) %1861, align 16
  br label %1911

1911:                                             ; preds = %1904, %1886
  %1912 = phi <4 x i32> [ %1909, %1904 ], [ zeroinitializer, %1886 ]
  %1913 = phi <4 x i32> [ %1907, %1904 ], [ zeroinitializer, %1886 ]
  %1914 = phi <4 x i32> [ %1905, %1904 ], [ zeroinitializer, %1886 ]
  %1915 = phi <4 x i32> [ %1906, %1904 ], [ zeroinitializer, %1886 ]
  %1916 = phi <4 x i32> [ %1908, %1904 ], [ zeroinitializer, %1886 ]
  %1917 = phi <4 x i32> [ %1910, %1904 ], [ zeroinitializer, %1886 ]
  %.v = select i1 %143, half %1082, half %1876
  %.v1 = select i1 %143, half %1083, half %1877
  %.v2 = select i1 %143, half %1084, half %1878
  %.v3 = select i1 %143, half %1085, half %1879
  %.v4 = select i1 %143, half %1086, half %1880
  %.v5 = select i1 %143, half %1087, half %1881
  %.v6 = select i1 %143, half %1088, half %1882
  %.v7 = select i1 %143, half %1089, half %1883
  %.v8 = select i1 %143, half %1102, half %1896
  %.v9 = select i1 %143, half %1103, half %1897
  %.v10 = select i1 %143, half %1104, half %1898
  %.v11 = select i1 %143, half %1105, half %1899
  %.v12 = select i1 %143, half %1106, half %1900
  %.v13 = select i1 %143, half %1107, half %1901
  %.v14 = select i1 %143, half %1108, half %1902
  %.v15 = select i1 %143, half %1109, half %1903
  %1918 = icmp ugt i32 %134, 63
  %1919 = and i1 %1918, %132
  %1920 = and i1 %1919, %54
  br i1 %1920, label %1921, label %1929

1921:                                             ; preds = %1911
  %1922 = shl nuw nsw i32 %136, 6, !spirv.Decorations !400
  %1923 = add nsw i32 %134, -64, !spirv.Decorations !403
  %1924 = add nuw nsw i32 %1923, %1922, !spirv.Decorations !400
  %1925 = zext i32 %1924 to i64
  %1926 = getelementptr i64, ptr addrspace(1) %0, i64 %1925
  %1927 = bitcast ptr addrspace(1) %1926 to ptr addrspace(1)
  %1928 = load <2 x i64>, ptr addrspace(1) %1927, align 16
  br label %1929

1929:                                             ; preds = %1921, %1911
  %1930 = phi <2 x i64> [ %1928, %1921 ], [ zeroinitializer, %1911 ]
  %1931 = extractelement <2 x i64> %1930, i32 0
  %1932 = extractelement <2 x i64> %1930, i32 1
  %1933 = and i64 %1931, 4095
  %1934 = and i64 %1932, 4095
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %.idx44 = mul nuw nsw i64 %1933, 1536, !spirv.Decorations !400
  %1935 = getelementptr i8, ptr addrspace(1) %1, i64 %.idx44
  %1936 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1937 = getelementptr half, ptr addrspace(1) %1936, i64 %230
  %1938 = getelementptr half, ptr addrspace(1) %1937, i64 %295
  %.idx45 = mul nuw nsw i64 %1934, 1536, !spirv.Decorations !400
  %1939 = getelementptr i8, ptr addrspace(1) %1, i64 %.idx45
  %1940 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1941 = getelementptr half, ptr addrspace(1) %1940, i64 %230
  %1942 = getelementptr half, ptr addrspace(1) %1941, i64 %295
  %1943 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1944 = getelementptr half, ptr addrspace(1) %1943, i64 %231
  %1945 = getelementptr half, ptr addrspace(1) %1944, i64 %295
  %1946 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1947 = getelementptr half, ptr addrspace(1) %1946, i64 %231
  %1948 = getelementptr half, ptr addrspace(1) %1947, i64 %295
  %1949 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1950 = getelementptr half, ptr addrspace(1) %1949, i64 %232
  %1951 = getelementptr half, ptr addrspace(1) %1950, i64 %295
  %1952 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1953 = getelementptr half, ptr addrspace(1) %1952, i64 %232
  %1954 = getelementptr half, ptr addrspace(1) %1953, i64 %295
  %1955 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1956 = getelementptr half, ptr addrspace(1) %1955, i64 %233
  %1957 = getelementptr half, ptr addrspace(1) %1956, i64 %295
  %1958 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1959 = getelementptr half, ptr addrspace(1) %1958, i64 %233
  %1960 = getelementptr half, ptr addrspace(1) %1959, i64 %295
  %1961 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1962 = getelementptr half, ptr addrspace(1) %1961, i64 %234
  %1963 = getelementptr half, ptr addrspace(1) %1962, i64 %295
  %1964 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1965 = getelementptr half, ptr addrspace(1) %1964, i64 %234
  %1966 = getelementptr half, ptr addrspace(1) %1965, i64 %295
  %1967 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1968 = getelementptr half, ptr addrspace(1) %1967, i64 %235
  %1969 = getelementptr half, ptr addrspace(1) %1968, i64 %295
  %1970 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1971 = getelementptr half, ptr addrspace(1) %1970, i64 %235
  %1972 = getelementptr half, ptr addrspace(1) %1971, i64 %295
  %1973 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1974 = getelementptr half, ptr addrspace(1) %1973, i64 %236
  %1975 = getelementptr half, ptr addrspace(1) %1974, i64 %295
  %1976 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1977 = getelementptr half, ptr addrspace(1) %1976, i64 %236
  %1978 = getelementptr half, ptr addrspace(1) %1977, i64 %295
  %1979 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1980 = getelementptr half, ptr addrspace(1) %1979, i64 %237
  %1981 = getelementptr half, ptr addrspace(1) %1980, i64 %295
  %1982 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1983 = getelementptr half, ptr addrspace(1) %1982, i64 %237
  %1984 = getelementptr half, ptr addrspace(1) %1983, i64 %295
  %1985 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1986 = getelementptr half, ptr addrspace(1) %1985, i64 %238
  %1987 = getelementptr half, ptr addrspace(1) %1986, i64 %295
  %1988 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1989 = getelementptr half, ptr addrspace(1) %1988, i64 %238
  %1990 = getelementptr half, ptr addrspace(1) %1989, i64 %295
  %1991 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1992 = getelementptr half, ptr addrspace(1) %1991, i64 %239
  %1993 = getelementptr half, ptr addrspace(1) %1992, i64 %295
  %1994 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %1995 = getelementptr half, ptr addrspace(1) %1994, i64 %239
  %1996 = getelementptr half, ptr addrspace(1) %1995, i64 %295
  %1997 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %1998 = getelementptr half, ptr addrspace(1) %1997, i64 %240
  %1999 = getelementptr half, ptr addrspace(1) %1998, i64 %295
  %2000 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2001 = getelementptr half, ptr addrspace(1) %2000, i64 %240
  %2002 = getelementptr half, ptr addrspace(1) %2001, i64 %295
  %2003 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2004 = getelementptr half, ptr addrspace(1) %2003, i64 %241
  %2005 = getelementptr half, ptr addrspace(1) %2004, i64 %295
  %2006 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2007 = getelementptr half, ptr addrspace(1) %2006, i64 %241
  %2008 = getelementptr half, ptr addrspace(1) %2007, i64 %295
  %2009 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2010 = getelementptr half, ptr addrspace(1) %2009, i64 %242
  %2011 = getelementptr half, ptr addrspace(1) %2010, i64 %295
  %2012 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2013 = getelementptr half, ptr addrspace(1) %2012, i64 %242
  %2014 = getelementptr half, ptr addrspace(1) %2013, i64 %295
  %2015 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2016 = getelementptr half, ptr addrspace(1) %2015, i64 %243
  %2017 = getelementptr half, ptr addrspace(1) %2016, i64 %295
  %2018 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2019 = getelementptr half, ptr addrspace(1) %2018, i64 %243
  %2020 = getelementptr half, ptr addrspace(1) %2019, i64 %295
  %2021 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2022 = getelementptr half, ptr addrspace(1) %2021, i64 %244
  %2023 = getelementptr half, ptr addrspace(1) %2022, i64 %295
  %2024 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2025 = getelementptr half, ptr addrspace(1) %2024, i64 %244
  %2026 = getelementptr half, ptr addrspace(1) %2025, i64 %295
  %2027 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2028 = getelementptr half, ptr addrspace(1) %2027, i64 %245
  %2029 = getelementptr half, ptr addrspace(1) %2028, i64 %295
  %2030 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2031 = getelementptr half, ptr addrspace(1) %2030, i64 %245
  %2032 = getelementptr half, ptr addrspace(1) %2031, i64 %295
  %2033 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2034 = getelementptr half, ptr addrspace(1) %2033, i64 %246
  %2035 = getelementptr half, ptr addrspace(1) %2034, i64 %295
  %2036 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2037 = getelementptr half, ptr addrspace(1) %2036, i64 %246
  %2038 = getelementptr half, ptr addrspace(1) %2037, i64 %295
  %2039 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2040 = getelementptr half, ptr addrspace(1) %2039, i64 %247
  %2041 = getelementptr half, ptr addrspace(1) %2040, i64 %295
  %2042 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2043 = getelementptr half, ptr addrspace(1) %2042, i64 %247
  %2044 = getelementptr half, ptr addrspace(1) %2043, i64 %295
  %2045 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2046 = getelementptr half, ptr addrspace(1) %2045, i64 %248
  %2047 = getelementptr half, ptr addrspace(1) %2046, i64 %295
  %2048 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2049 = getelementptr half, ptr addrspace(1) %2048, i64 %248
  %2050 = getelementptr half, ptr addrspace(1) %2049, i64 %295
  %2051 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2052 = getelementptr half, ptr addrspace(1) %2051, i64 %249
  %2053 = getelementptr half, ptr addrspace(1) %2052, i64 %295
  %2054 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2055 = getelementptr half, ptr addrspace(1) %2054, i64 %249
  %2056 = getelementptr half, ptr addrspace(1) %2055, i64 %295
  %2057 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2058 = getelementptr half, ptr addrspace(1) %2057, i64 %250
  %2059 = getelementptr half, ptr addrspace(1) %2058, i64 %295
  %2060 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2061 = getelementptr half, ptr addrspace(1) %2060, i64 %250
  %2062 = getelementptr half, ptr addrspace(1) %2061, i64 %295
  %2063 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2064 = getelementptr half, ptr addrspace(1) %2063, i64 %251
  %2065 = getelementptr half, ptr addrspace(1) %2064, i64 %295
  %2066 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2067 = getelementptr half, ptr addrspace(1) %2066, i64 %251
  %2068 = getelementptr half, ptr addrspace(1) %2067, i64 %295
  %2069 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2070 = getelementptr half, ptr addrspace(1) %2069, i64 %252
  %2071 = getelementptr half, ptr addrspace(1) %2070, i64 %295
  %2072 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2073 = getelementptr half, ptr addrspace(1) %2072, i64 %252
  %2074 = getelementptr half, ptr addrspace(1) %2073, i64 %295
  %2075 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2076 = getelementptr half, ptr addrspace(1) %2075, i64 %253
  %2077 = getelementptr half, ptr addrspace(1) %2076, i64 %295
  %2078 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2079 = getelementptr half, ptr addrspace(1) %2078, i64 %253
  %2080 = getelementptr half, ptr addrspace(1) %2079, i64 %295
  %2081 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2082 = getelementptr half, ptr addrspace(1) %2081, i64 %254
  %2083 = getelementptr half, ptr addrspace(1) %2082, i64 %295
  %2084 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2085 = getelementptr half, ptr addrspace(1) %2084, i64 %254
  %2086 = getelementptr half, ptr addrspace(1) %2085, i64 %295
  %2087 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2088 = getelementptr half, ptr addrspace(1) %2087, i64 %255
  %2089 = getelementptr half, ptr addrspace(1) %2088, i64 %295
  %2090 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2091 = getelementptr half, ptr addrspace(1) %2090, i64 %255
  %2092 = getelementptr half, ptr addrspace(1) %2091, i64 %295
  %2093 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2094 = getelementptr half, ptr addrspace(1) %2093, i64 %256
  %2095 = getelementptr half, ptr addrspace(1) %2094, i64 %295
  %2096 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2097 = getelementptr half, ptr addrspace(1) %2096, i64 %256
  %2098 = getelementptr half, ptr addrspace(1) %2097, i64 %295
  %2099 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2100 = getelementptr half, ptr addrspace(1) %2099, i64 %257
  %2101 = getelementptr half, ptr addrspace(1) %2100, i64 %295
  %2102 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2103 = getelementptr half, ptr addrspace(1) %2102, i64 %257
  %2104 = getelementptr half, ptr addrspace(1) %2103, i64 %295
  %2105 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2106 = getelementptr half, ptr addrspace(1) %2105, i64 %258
  %2107 = getelementptr half, ptr addrspace(1) %2106, i64 %295
  %2108 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2109 = getelementptr half, ptr addrspace(1) %2108, i64 %258
  %2110 = getelementptr half, ptr addrspace(1) %2109, i64 %295
  %2111 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2112 = getelementptr half, ptr addrspace(1) %2111, i64 %259
  %2113 = getelementptr half, ptr addrspace(1) %2112, i64 %295
  %2114 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2115 = getelementptr half, ptr addrspace(1) %2114, i64 %259
  %2116 = getelementptr half, ptr addrspace(1) %2115, i64 %295
  %2117 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2118 = getelementptr half, ptr addrspace(1) %2117, i64 %260
  %2119 = getelementptr half, ptr addrspace(1) %2118, i64 %295
  %2120 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2121 = getelementptr half, ptr addrspace(1) %2120, i64 %260
  %2122 = getelementptr half, ptr addrspace(1) %2121, i64 %295
  %2123 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2124 = getelementptr half, ptr addrspace(1) %2123, i64 %261
  %2125 = getelementptr half, ptr addrspace(1) %2124, i64 %295
  %2126 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2127 = getelementptr half, ptr addrspace(1) %2126, i64 %261
  %2128 = getelementptr half, ptr addrspace(1) %2127, i64 %295
  %2129 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2130 = getelementptr half, ptr addrspace(1) %2129, i64 %262
  %2131 = getelementptr half, ptr addrspace(1) %2130, i64 %295
  %2132 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2133 = getelementptr half, ptr addrspace(1) %2132, i64 %262
  %2134 = getelementptr half, ptr addrspace(1) %2133, i64 %295
  %2135 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2136 = getelementptr half, ptr addrspace(1) %2135, i64 %263
  %2137 = getelementptr half, ptr addrspace(1) %2136, i64 %295
  %2138 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2139 = getelementptr half, ptr addrspace(1) %2138, i64 %263
  %2140 = getelementptr half, ptr addrspace(1) %2139, i64 %295
  %2141 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2142 = getelementptr half, ptr addrspace(1) %2141, i64 %264
  %2143 = getelementptr half, ptr addrspace(1) %2142, i64 %295
  %2144 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2145 = getelementptr half, ptr addrspace(1) %2144, i64 %264
  %2146 = getelementptr half, ptr addrspace(1) %2145, i64 %295
  %2147 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2148 = getelementptr half, ptr addrspace(1) %2147, i64 %265
  %2149 = getelementptr half, ptr addrspace(1) %2148, i64 %295
  %2150 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2151 = getelementptr half, ptr addrspace(1) %2150, i64 %265
  %2152 = getelementptr half, ptr addrspace(1) %2151, i64 %295
  %2153 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2154 = getelementptr half, ptr addrspace(1) %2153, i64 %266
  %2155 = getelementptr half, ptr addrspace(1) %2154, i64 %295
  %2156 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2157 = getelementptr half, ptr addrspace(1) %2156, i64 %266
  %2158 = getelementptr half, ptr addrspace(1) %2157, i64 %295
  %2159 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2160 = getelementptr half, ptr addrspace(1) %2159, i64 %267
  %2161 = getelementptr half, ptr addrspace(1) %2160, i64 %295
  %2162 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2163 = getelementptr half, ptr addrspace(1) %2162, i64 %267
  %2164 = getelementptr half, ptr addrspace(1) %2163, i64 %295
  %2165 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2166 = getelementptr half, ptr addrspace(1) %2165, i64 %268
  %2167 = getelementptr half, ptr addrspace(1) %2166, i64 %295
  %2168 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2169 = getelementptr half, ptr addrspace(1) %2168, i64 %268
  %2170 = getelementptr half, ptr addrspace(1) %2169, i64 %295
  %2171 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2172 = getelementptr half, ptr addrspace(1) %2171, i64 %269
  %2173 = getelementptr half, ptr addrspace(1) %2172, i64 %295
  %2174 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2175 = getelementptr half, ptr addrspace(1) %2174, i64 %269
  %2176 = getelementptr half, ptr addrspace(1) %2175, i64 %295
  %2177 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2178 = getelementptr half, ptr addrspace(1) %2177, i64 %270
  %2179 = getelementptr half, ptr addrspace(1) %2178, i64 %295
  %2180 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2181 = getelementptr half, ptr addrspace(1) %2180, i64 %270
  %2182 = getelementptr half, ptr addrspace(1) %2181, i64 %295
  %2183 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2184 = getelementptr half, ptr addrspace(1) %2183, i64 %271
  %2185 = getelementptr half, ptr addrspace(1) %2184, i64 %295
  %2186 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2187 = getelementptr half, ptr addrspace(1) %2186, i64 %271
  %2188 = getelementptr half, ptr addrspace(1) %2187, i64 %295
  %2189 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2190 = getelementptr half, ptr addrspace(1) %2189, i64 %272
  %2191 = getelementptr half, ptr addrspace(1) %2190, i64 %295
  %2192 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2193 = getelementptr half, ptr addrspace(1) %2192, i64 %272
  %2194 = getelementptr half, ptr addrspace(1) %2193, i64 %295
  %2195 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2196 = getelementptr half, ptr addrspace(1) %2195, i64 %273
  %2197 = getelementptr half, ptr addrspace(1) %2196, i64 %295
  %2198 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2199 = getelementptr half, ptr addrspace(1) %2198, i64 %273
  %2200 = getelementptr half, ptr addrspace(1) %2199, i64 %295
  %2201 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2202 = getelementptr half, ptr addrspace(1) %2201, i64 %274
  %2203 = getelementptr half, ptr addrspace(1) %2202, i64 %295
  %2204 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2205 = getelementptr half, ptr addrspace(1) %2204, i64 %274
  %2206 = getelementptr half, ptr addrspace(1) %2205, i64 %295
  %2207 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2208 = getelementptr half, ptr addrspace(1) %2207, i64 %275
  %2209 = getelementptr half, ptr addrspace(1) %2208, i64 %295
  %2210 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2211 = getelementptr half, ptr addrspace(1) %2210, i64 %275
  %2212 = getelementptr half, ptr addrspace(1) %2211, i64 %295
  %2213 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2214 = getelementptr half, ptr addrspace(1) %2213, i64 %276
  %2215 = getelementptr half, ptr addrspace(1) %2214, i64 %295
  %2216 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2217 = getelementptr half, ptr addrspace(1) %2216, i64 %276
  %2218 = getelementptr half, ptr addrspace(1) %2217, i64 %295
  %2219 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2220 = getelementptr half, ptr addrspace(1) %2219, i64 %277
  %2221 = getelementptr half, ptr addrspace(1) %2220, i64 %295
  %2222 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2223 = getelementptr half, ptr addrspace(1) %2222, i64 %277
  %2224 = getelementptr half, ptr addrspace(1) %2223, i64 %295
  %2225 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2226 = getelementptr half, ptr addrspace(1) %2225, i64 %278
  %2227 = getelementptr half, ptr addrspace(1) %2226, i64 %295
  %2228 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2229 = getelementptr half, ptr addrspace(1) %2228, i64 %278
  %2230 = getelementptr half, ptr addrspace(1) %2229, i64 %295
  %2231 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2232 = getelementptr half, ptr addrspace(1) %2231, i64 %279
  %2233 = getelementptr half, ptr addrspace(1) %2232, i64 %295
  %2234 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2235 = getelementptr half, ptr addrspace(1) %2234, i64 %279
  %2236 = getelementptr half, ptr addrspace(1) %2235, i64 %295
  %2237 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2238 = getelementptr half, ptr addrspace(1) %2237, i64 %280
  %2239 = getelementptr half, ptr addrspace(1) %2238, i64 %295
  %2240 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2241 = getelementptr half, ptr addrspace(1) %2240, i64 %280
  %2242 = getelementptr half, ptr addrspace(1) %2241, i64 %295
  %2243 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2244 = getelementptr half, ptr addrspace(1) %2243, i64 %281
  %2245 = getelementptr half, ptr addrspace(1) %2244, i64 %295
  %2246 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2247 = getelementptr half, ptr addrspace(1) %2246, i64 %281
  %2248 = getelementptr half, ptr addrspace(1) %2247, i64 %295
  %2249 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2250 = getelementptr half, ptr addrspace(1) %2249, i64 %282
  %2251 = getelementptr half, ptr addrspace(1) %2250, i64 %295
  %2252 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2253 = getelementptr half, ptr addrspace(1) %2252, i64 %282
  %2254 = getelementptr half, ptr addrspace(1) %2253, i64 %295
  %2255 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2256 = getelementptr half, ptr addrspace(1) %2255, i64 %283
  %2257 = getelementptr half, ptr addrspace(1) %2256, i64 %295
  %2258 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2259 = getelementptr half, ptr addrspace(1) %2258, i64 %283
  %2260 = getelementptr half, ptr addrspace(1) %2259, i64 %295
  %2261 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2262 = getelementptr half, ptr addrspace(1) %2261, i64 %284
  %2263 = getelementptr half, ptr addrspace(1) %2262, i64 %295
  %2264 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2265 = getelementptr half, ptr addrspace(1) %2264, i64 %284
  %2266 = getelementptr half, ptr addrspace(1) %2265, i64 %295
  %2267 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2268 = getelementptr half, ptr addrspace(1) %2267, i64 %285
  %2269 = getelementptr half, ptr addrspace(1) %2268, i64 %295
  %2270 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2271 = getelementptr half, ptr addrspace(1) %2270, i64 %285
  %2272 = getelementptr half, ptr addrspace(1) %2271, i64 %295
  %2273 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2274 = getelementptr half, ptr addrspace(1) %2273, i64 %286
  %2275 = getelementptr half, ptr addrspace(1) %2274, i64 %295
  %2276 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2277 = getelementptr half, ptr addrspace(1) %2276, i64 %286
  %2278 = getelementptr half, ptr addrspace(1) %2277, i64 %295
  %2279 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2280 = getelementptr half, ptr addrspace(1) %2279, i64 %287
  %2281 = getelementptr half, ptr addrspace(1) %2280, i64 %295
  %2282 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2283 = getelementptr half, ptr addrspace(1) %2282, i64 %287
  %2284 = getelementptr half, ptr addrspace(1) %2283, i64 %295
  %2285 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2286 = getelementptr half, ptr addrspace(1) %2285, i64 %288
  %2287 = getelementptr half, ptr addrspace(1) %2286, i64 %295
  %2288 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2289 = getelementptr half, ptr addrspace(1) %2288, i64 %288
  %2290 = getelementptr half, ptr addrspace(1) %2289, i64 %295
  %2291 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2292 = getelementptr half, ptr addrspace(1) %2291, i64 %289
  %2293 = getelementptr half, ptr addrspace(1) %2292, i64 %295
  %2294 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2295 = getelementptr half, ptr addrspace(1) %2294, i64 %289
  %2296 = getelementptr half, ptr addrspace(1) %2295, i64 %295
  %2297 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2298 = getelementptr half, ptr addrspace(1) %2297, i64 %290
  %2299 = getelementptr half, ptr addrspace(1) %2298, i64 %295
  %2300 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2301 = getelementptr half, ptr addrspace(1) %2300, i64 %290
  %2302 = getelementptr half, ptr addrspace(1) %2301, i64 %295
  %2303 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2304 = getelementptr half, ptr addrspace(1) %2303, i64 %291
  %2305 = getelementptr half, ptr addrspace(1) %2304, i64 %295
  %2306 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2307 = getelementptr half, ptr addrspace(1) %2306, i64 %291
  %2308 = getelementptr half, ptr addrspace(1) %2307, i64 %295
  %2309 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2310 = getelementptr half, ptr addrspace(1) %2309, i64 %292
  %2311 = getelementptr half, ptr addrspace(1) %2310, i64 %295
  %2312 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2313 = getelementptr half, ptr addrspace(1) %2312, i64 %292
  %2314 = getelementptr half, ptr addrspace(1) %2313, i64 %295
  %2315 = bitcast ptr addrspace(1) %1935 to ptr addrspace(1)
  %2316 = getelementptr half, ptr addrspace(1) %2315, i64 %293
  %2317 = getelementptr half, ptr addrspace(1) %2316, i64 %295
  %2318 = bitcast ptr addrspace(1) %1939 to ptr addrspace(1)
  %2319 = getelementptr half, ptr addrspace(1) %2318, i64 %293
  %2320 = getelementptr half, ptr addrspace(1) %2319, i64 %295
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %2321 = ptrtoint ptr addrspace(1) %1938 to i64
  %2322 = ptrtoint ptr addrspace(1) %1942 to i64
  %2323 = ptrtoint ptr addrspace(1) %1945 to i64
  %2324 = ptrtoint ptr addrspace(1) %1948 to i64
  %2325 = ptrtoint ptr addrspace(1) %1951 to i64
  %2326 = ptrtoint ptr addrspace(1) %1954 to i64
  %2327 = ptrtoint ptr addrspace(1) %1957 to i64
  %2328 = ptrtoint ptr addrspace(1) %1960 to i64
  %2329 = ptrtoint ptr addrspace(1) %1963 to i64
  %2330 = ptrtoint ptr addrspace(1) %1966 to i64
  %2331 = ptrtoint ptr addrspace(1) %1969 to i64
  %2332 = ptrtoint ptr addrspace(1) %1972 to i64
  %2333 = ptrtoint ptr addrspace(1) %1975 to i64
  %2334 = ptrtoint ptr addrspace(1) %1978 to i64
  %2335 = ptrtoint ptr addrspace(1) %1981 to i64
  %2336 = ptrtoint ptr addrspace(1) %1984 to i64
  %2337 = ptrtoint ptr addrspace(1) %1987 to i64
  %2338 = ptrtoint ptr addrspace(1) %1990 to i64
  %2339 = ptrtoint ptr addrspace(1) %1993 to i64
  %2340 = ptrtoint ptr addrspace(1) %1996 to i64
  %2341 = ptrtoint ptr addrspace(1) %1999 to i64
  %2342 = ptrtoint ptr addrspace(1) %2002 to i64
  %2343 = ptrtoint ptr addrspace(1) %2005 to i64
  %2344 = ptrtoint ptr addrspace(1) %2008 to i64
  %2345 = ptrtoint ptr addrspace(1) %2011 to i64
  %2346 = ptrtoint ptr addrspace(1) %2014 to i64
  %2347 = ptrtoint ptr addrspace(1) %2017 to i64
  %2348 = ptrtoint ptr addrspace(1) %2020 to i64
  %2349 = ptrtoint ptr addrspace(1) %2023 to i64
  %2350 = ptrtoint ptr addrspace(1) %2026 to i64
  %2351 = ptrtoint ptr addrspace(1) %2029 to i64
  %2352 = ptrtoint ptr addrspace(1) %2032 to i64
  %2353 = ptrtoint ptr addrspace(1) %2035 to i64
  %2354 = ptrtoint ptr addrspace(1) %2038 to i64
  %2355 = ptrtoint ptr addrspace(1) %2041 to i64
  %2356 = ptrtoint ptr addrspace(1) %2044 to i64
  %2357 = ptrtoint ptr addrspace(1) %2047 to i64
  %2358 = ptrtoint ptr addrspace(1) %2050 to i64
  %2359 = ptrtoint ptr addrspace(1) %2053 to i64
  %2360 = ptrtoint ptr addrspace(1) %2056 to i64
  %2361 = ptrtoint ptr addrspace(1) %2059 to i64
  %2362 = ptrtoint ptr addrspace(1) %2062 to i64
  %2363 = ptrtoint ptr addrspace(1) %2065 to i64
  %2364 = ptrtoint ptr addrspace(1) %2068 to i64
  %2365 = ptrtoint ptr addrspace(1) %2071 to i64
  %2366 = ptrtoint ptr addrspace(1) %2074 to i64
  %2367 = ptrtoint ptr addrspace(1) %2077 to i64
  %2368 = ptrtoint ptr addrspace(1) %2080 to i64
  %2369 = ptrtoint ptr addrspace(1) %2083 to i64
  %2370 = ptrtoint ptr addrspace(1) %2086 to i64
  %2371 = ptrtoint ptr addrspace(1) %2089 to i64
  %2372 = ptrtoint ptr addrspace(1) %2092 to i64
  %2373 = ptrtoint ptr addrspace(1) %2095 to i64
  %2374 = ptrtoint ptr addrspace(1) %2098 to i64
  %2375 = ptrtoint ptr addrspace(1) %2101 to i64
  %2376 = ptrtoint ptr addrspace(1) %2104 to i64
  %2377 = ptrtoint ptr addrspace(1) %2107 to i64
  %2378 = ptrtoint ptr addrspace(1) %2110 to i64
  %2379 = ptrtoint ptr addrspace(1) %2113 to i64
  %2380 = ptrtoint ptr addrspace(1) %2116 to i64
  %2381 = ptrtoint ptr addrspace(1) %2119 to i64
  %2382 = ptrtoint ptr addrspace(1) %2122 to i64
  %2383 = ptrtoint ptr addrspace(1) %2125 to i64
  %2384 = ptrtoint ptr addrspace(1) %2128 to i64
  %2385 = ptrtoint ptr addrspace(1) %2131 to i64
  %2386 = ptrtoint ptr addrspace(1) %2134 to i64
  %2387 = ptrtoint ptr addrspace(1) %2137 to i64
  %2388 = ptrtoint ptr addrspace(1) %2140 to i64
  %2389 = ptrtoint ptr addrspace(1) %2143 to i64
  %2390 = ptrtoint ptr addrspace(1) %2146 to i64
  %2391 = ptrtoint ptr addrspace(1) %2149 to i64
  %2392 = ptrtoint ptr addrspace(1) %2152 to i64
  %2393 = ptrtoint ptr addrspace(1) %2155 to i64
  %2394 = ptrtoint ptr addrspace(1) %2158 to i64
  %2395 = ptrtoint ptr addrspace(1) %2161 to i64
  %2396 = ptrtoint ptr addrspace(1) %2164 to i64
  %2397 = ptrtoint ptr addrspace(1) %2167 to i64
  %2398 = ptrtoint ptr addrspace(1) %2170 to i64
  %2399 = ptrtoint ptr addrspace(1) %2173 to i64
  %2400 = ptrtoint ptr addrspace(1) %2176 to i64
  %2401 = ptrtoint ptr addrspace(1) %2179 to i64
  %2402 = ptrtoint ptr addrspace(1) %2182 to i64
  %2403 = ptrtoint ptr addrspace(1) %2185 to i64
  %2404 = ptrtoint ptr addrspace(1) %2188 to i64
  %2405 = ptrtoint ptr addrspace(1) %2191 to i64
  %2406 = ptrtoint ptr addrspace(1) %2194 to i64
  %2407 = ptrtoint ptr addrspace(1) %2197 to i64
  %2408 = ptrtoint ptr addrspace(1) %2200 to i64
  %2409 = ptrtoint ptr addrspace(1) %2203 to i64
  %2410 = ptrtoint ptr addrspace(1) %2206 to i64
  %2411 = ptrtoint ptr addrspace(1) %2209 to i64
  %2412 = ptrtoint ptr addrspace(1) %2212 to i64
  %2413 = ptrtoint ptr addrspace(1) %2215 to i64
  %2414 = ptrtoint ptr addrspace(1) %2218 to i64
  %2415 = ptrtoint ptr addrspace(1) %2221 to i64
  %2416 = ptrtoint ptr addrspace(1) %2224 to i64
  %2417 = ptrtoint ptr addrspace(1) %2227 to i64
  %2418 = ptrtoint ptr addrspace(1) %2230 to i64
  %2419 = ptrtoint ptr addrspace(1) %2233 to i64
  %2420 = ptrtoint ptr addrspace(1) %2236 to i64
  %2421 = ptrtoint ptr addrspace(1) %2239 to i64
  %2422 = ptrtoint ptr addrspace(1) %2242 to i64
  %2423 = ptrtoint ptr addrspace(1) %2245 to i64
  %2424 = ptrtoint ptr addrspace(1) %2248 to i64
  %2425 = ptrtoint ptr addrspace(1) %2251 to i64
  %2426 = ptrtoint ptr addrspace(1) %2254 to i64
  %2427 = ptrtoint ptr addrspace(1) %2257 to i64
  %2428 = ptrtoint ptr addrspace(1) %2260 to i64
  %2429 = ptrtoint ptr addrspace(1) %2263 to i64
  %2430 = ptrtoint ptr addrspace(1) %2266 to i64
  %2431 = ptrtoint ptr addrspace(1) %2269 to i64
  %2432 = ptrtoint ptr addrspace(1) %2272 to i64
  %2433 = ptrtoint ptr addrspace(1) %2275 to i64
  %2434 = ptrtoint ptr addrspace(1) %2278 to i64
  %2435 = ptrtoint ptr addrspace(1) %2281 to i64
  %2436 = ptrtoint ptr addrspace(1) %2284 to i64
  %2437 = ptrtoint ptr addrspace(1) %2287 to i64
  %2438 = ptrtoint ptr addrspace(1) %2290 to i64
  %2439 = ptrtoint ptr addrspace(1) %2293 to i64
  %2440 = ptrtoint ptr addrspace(1) %2296 to i64
  %2441 = ptrtoint ptr addrspace(1) %2299 to i64
  %2442 = ptrtoint ptr addrspace(1) %2302 to i64
  %2443 = ptrtoint ptr addrspace(1) %2305 to i64
  %2444 = ptrtoint ptr addrspace(1) %2308 to i64
  %2445 = ptrtoint ptr addrspace(1) %2311 to i64
  %2446 = ptrtoint ptr addrspace(1) %2314 to i64
  %2447 = ptrtoint ptr addrspace(1) %2317 to i64
  %2448 = ptrtoint ptr addrspace(1) %2320 to i64
  %2449 = insertelement <2 x i64> undef, i64 %2321, i32 0
  %2450 = insertelement <2 x i64> %2449, i64 %2323, i32 1
  %2451 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %2450, ptr addrspace(3) %2451, align 16
  %2452 = insertelement <2 x i64> undef, i64 %2322, i32 0
  %2453 = insertelement <2 x i64> %2452, i64 %2324, i32 1
  %2454 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %2453, ptr addrspace(3) %2454, align 16
  %2455 = insertelement <2 x i64> undef, i64 %2337, i32 0
  %2456 = insertelement <2 x i64> %2455, i64 %2339, i32 1
  %2457 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %2456, ptr addrspace(3) %2457, align 16
  %2458 = insertelement <2 x i64> undef, i64 %2338, i32 0
  %2459 = insertelement <2 x i64> %2458, i64 %2340, i32 1
  %2460 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %2459, ptr addrspace(3) %2460, align 16
  %2461 = insertelement <2 x i64> undef, i64 %2353, i32 0
  %2462 = insertelement <2 x i64> %2461, i64 %2355, i32 1
  %2463 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %2462, ptr addrspace(3) %2463, align 16
  %2464 = insertelement <2 x i64> undef, i64 %2354, i32 0
  %2465 = insertelement <2 x i64> %2464, i64 %2356, i32 1
  %2466 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %2465, ptr addrspace(3) %2466, align 16
  %2467 = insertelement <2 x i64> undef, i64 %2369, i32 0
  %2468 = insertelement <2 x i64> %2467, i64 %2371, i32 1
  %2469 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %2468, ptr addrspace(3) %2469, align 16
  %2470 = insertelement <2 x i64> undef, i64 %2370, i32 0
  %2471 = insertelement <2 x i64> %2470, i64 %2372, i32 1
  %2472 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %2471, ptr addrspace(3) %2472, align 16
  %2473 = insertelement <2 x i64> undef, i64 %2385, i32 0
  %2474 = insertelement <2 x i64> %2473, i64 %2387, i32 1
  %2475 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %2474, ptr addrspace(3) %2475, align 16
  %2476 = insertelement <2 x i64> undef, i64 %2386, i32 0
  %2477 = insertelement <2 x i64> %2476, i64 %2388, i32 1
  %2478 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %2477, ptr addrspace(3) %2478, align 16
  %2479 = insertelement <2 x i64> undef, i64 %2401, i32 0
  %2480 = insertelement <2 x i64> %2479, i64 %2403, i32 1
  %2481 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %2480, ptr addrspace(3) %2481, align 16
  %2482 = insertelement <2 x i64> undef, i64 %2402, i32 0
  %2483 = insertelement <2 x i64> %2482, i64 %2404, i32 1
  %2484 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %2483, ptr addrspace(3) %2484, align 16
  %2485 = insertelement <2 x i64> undef, i64 %2417, i32 0
  %2486 = insertelement <2 x i64> %2485, i64 %2419, i32 1
  %2487 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %2486, ptr addrspace(3) %2487, align 16
  %2488 = insertelement <2 x i64> undef, i64 %2418, i32 0
  %2489 = insertelement <2 x i64> %2488, i64 %2420, i32 1
  %2490 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %2489, ptr addrspace(3) %2490, align 16
  %2491 = insertelement <2 x i64> undef, i64 %2433, i32 0
  %2492 = insertelement <2 x i64> %2491, i64 %2435, i32 1
  %2493 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %2492, ptr addrspace(3) %2493, align 16
  %2494 = insertelement <2 x i64> undef, i64 %2434, i32 0
  %2495 = insertelement <2 x i64> %2494, i64 %2436, i32 1
  %2496 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %2495, ptr addrspace(3) %2496, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %2497 = bitcast ptr addrspace(3) %909 to ptr addrspace(3)
  %2498 = load <2 x i64>, ptr addrspace(3) %2497, align 16
  %2499 = bitcast ptr addrspace(3) %910 to ptr addrspace(3)
  %2500 = load <2 x i64>, ptr addrspace(3) %2499, align 16
  %2501 = bitcast ptr addrspace(3) %913 to ptr addrspace(3)
  %2502 = load <2 x i64>, ptr addrspace(3) %2501, align 16
  %2503 = bitcast ptr addrspace(3) %914 to ptr addrspace(3)
  %2504 = load <2 x i64>, ptr addrspace(3) %2503, align 16
  %2505 = bitcast ptr addrspace(3) %917 to ptr addrspace(3)
  %2506 = load <2 x i64>, ptr addrspace(3) %2505, align 16
  %2507 = bitcast ptr addrspace(3) %918 to ptr addrspace(3)
  %2508 = load <2 x i64>, ptr addrspace(3) %2507, align 16
  %2509 = bitcast ptr addrspace(3) %921 to ptr addrspace(3)
  %2510 = load <2 x i64>, ptr addrspace(3) %2509, align 16
  %2511 = bitcast ptr addrspace(3) %922 to ptr addrspace(3)
  %2512 = load <2 x i64>, ptr addrspace(3) %2511, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %2513 = insertelement <2 x i64> undef, i64 %2325, i32 0
  %2514 = insertelement <2 x i64> %2513, i64 %2327, i32 1
  %2515 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %2514, ptr addrspace(3) %2515, align 16
  %2516 = insertelement <2 x i64> undef, i64 %2326, i32 0
  %2517 = insertelement <2 x i64> %2516, i64 %2328, i32 1
  %2518 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %2517, ptr addrspace(3) %2518, align 16
  %2519 = insertelement <2 x i64> undef, i64 %2341, i32 0
  %2520 = insertelement <2 x i64> %2519, i64 %2343, i32 1
  %2521 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %2520, ptr addrspace(3) %2521, align 16
  %2522 = insertelement <2 x i64> undef, i64 %2342, i32 0
  %2523 = insertelement <2 x i64> %2522, i64 %2344, i32 1
  %2524 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %2523, ptr addrspace(3) %2524, align 16
  %2525 = insertelement <2 x i64> undef, i64 %2357, i32 0
  %2526 = insertelement <2 x i64> %2525, i64 %2359, i32 1
  %2527 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %2526, ptr addrspace(3) %2527, align 16
  %2528 = insertelement <2 x i64> undef, i64 %2358, i32 0
  %2529 = insertelement <2 x i64> %2528, i64 %2360, i32 1
  %2530 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %2529, ptr addrspace(3) %2530, align 16
  %2531 = insertelement <2 x i64> undef, i64 %2373, i32 0
  %2532 = insertelement <2 x i64> %2531, i64 %2375, i32 1
  %2533 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %2532, ptr addrspace(3) %2533, align 16
  %2534 = insertelement <2 x i64> undef, i64 %2374, i32 0
  %2535 = insertelement <2 x i64> %2534, i64 %2376, i32 1
  %2536 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %2535, ptr addrspace(3) %2536, align 16
  %2537 = insertelement <2 x i64> undef, i64 %2389, i32 0
  %2538 = insertelement <2 x i64> %2537, i64 %2391, i32 1
  %2539 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %2538, ptr addrspace(3) %2539, align 16
  %2540 = insertelement <2 x i64> undef, i64 %2390, i32 0
  %2541 = insertelement <2 x i64> %2540, i64 %2392, i32 1
  %2542 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %2541, ptr addrspace(3) %2542, align 16
  %2543 = insertelement <2 x i64> undef, i64 %2405, i32 0
  %2544 = insertelement <2 x i64> %2543, i64 %2407, i32 1
  %2545 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %2544, ptr addrspace(3) %2545, align 16
  %2546 = insertelement <2 x i64> undef, i64 %2406, i32 0
  %2547 = insertelement <2 x i64> %2546, i64 %2408, i32 1
  %2548 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %2547, ptr addrspace(3) %2548, align 16
  %2549 = insertelement <2 x i64> undef, i64 %2421, i32 0
  %2550 = insertelement <2 x i64> %2549, i64 %2423, i32 1
  %2551 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %2550, ptr addrspace(3) %2551, align 16
  %2552 = insertelement <2 x i64> undef, i64 %2422, i32 0
  %2553 = insertelement <2 x i64> %2552, i64 %2424, i32 1
  %2554 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %2553, ptr addrspace(3) %2554, align 16
  %2555 = insertelement <2 x i64> undef, i64 %2437, i32 0
  %2556 = insertelement <2 x i64> %2555, i64 %2439, i32 1
  %2557 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %2556, ptr addrspace(3) %2557, align 16
  %2558 = insertelement <2 x i64> undef, i64 %2438, i32 0
  %2559 = insertelement <2 x i64> %2558, i64 %2440, i32 1
  %2560 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %2559, ptr addrspace(3) %2560, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %2561 = insertelement <2 x i64> undef, i64 %2329, i32 0
  %2562 = insertelement <2 x i64> %2561, i64 %2331, i32 1
  %2563 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %2562, ptr addrspace(3) %2563, align 16
  %2564 = insertelement <2 x i64> undef, i64 %2330, i32 0
  %2565 = insertelement <2 x i64> %2564, i64 %2332, i32 1
  %2566 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %2565, ptr addrspace(3) %2566, align 16
  %2567 = insertelement <2 x i64> undef, i64 %2345, i32 0
  %2568 = insertelement <2 x i64> %2567, i64 %2347, i32 1
  %2569 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %2568, ptr addrspace(3) %2569, align 16
  %2570 = insertelement <2 x i64> undef, i64 %2346, i32 0
  %2571 = insertelement <2 x i64> %2570, i64 %2348, i32 1
  %2572 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %2571, ptr addrspace(3) %2572, align 16
  %2573 = insertelement <2 x i64> undef, i64 %2361, i32 0
  %2574 = insertelement <2 x i64> %2573, i64 %2363, i32 1
  %2575 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %2574, ptr addrspace(3) %2575, align 16
  %2576 = insertelement <2 x i64> undef, i64 %2362, i32 0
  %2577 = insertelement <2 x i64> %2576, i64 %2364, i32 1
  %2578 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %2577, ptr addrspace(3) %2578, align 16
  %2579 = insertelement <2 x i64> undef, i64 %2377, i32 0
  %2580 = insertelement <2 x i64> %2579, i64 %2379, i32 1
  %2581 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %2580, ptr addrspace(3) %2581, align 16
  %2582 = insertelement <2 x i64> undef, i64 %2378, i32 0
  %2583 = insertelement <2 x i64> %2582, i64 %2380, i32 1
  %2584 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %2583, ptr addrspace(3) %2584, align 16
  %2585 = insertelement <2 x i64> undef, i64 %2393, i32 0
  %2586 = insertelement <2 x i64> %2585, i64 %2395, i32 1
  %2587 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %2586, ptr addrspace(3) %2587, align 16
  %2588 = insertelement <2 x i64> undef, i64 %2394, i32 0
  %2589 = insertelement <2 x i64> %2588, i64 %2396, i32 1
  %2590 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %2589, ptr addrspace(3) %2590, align 16
  %2591 = insertelement <2 x i64> undef, i64 %2409, i32 0
  %2592 = insertelement <2 x i64> %2591, i64 %2411, i32 1
  %2593 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %2592, ptr addrspace(3) %2593, align 16
  %2594 = insertelement <2 x i64> undef, i64 %2410, i32 0
  %2595 = insertelement <2 x i64> %2594, i64 %2412, i32 1
  %2596 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %2595, ptr addrspace(3) %2596, align 16
  %2597 = insertelement <2 x i64> undef, i64 %2425, i32 0
  %2598 = insertelement <2 x i64> %2597, i64 %2427, i32 1
  %2599 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %2598, ptr addrspace(3) %2599, align 16
  %2600 = insertelement <2 x i64> undef, i64 %2426, i32 0
  %2601 = insertelement <2 x i64> %2600, i64 %2428, i32 1
  %2602 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %2601, ptr addrspace(3) %2602, align 16
  %2603 = insertelement <2 x i64> undef, i64 %2441, i32 0
  %2604 = insertelement <2 x i64> %2603, i64 %2443, i32 1
  %2605 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %2604, ptr addrspace(3) %2605, align 16
  %2606 = insertelement <2 x i64> undef, i64 %2442, i32 0
  %2607 = insertelement <2 x i64> %2606, i64 %2444, i32 1
  %2608 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %2607, ptr addrspace(3) %2608, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  %2609 = insertelement <2 x i64> undef, i64 %2333, i32 0
  %2610 = insertelement <2 x i64> %2609, i64 %2335, i32 1
  %2611 = bitcast ptr addrspace(3) %816 to ptr addrspace(3)
  store <2 x i64> %2610, ptr addrspace(3) %2611, align 16
  %2612 = insertelement <2 x i64> undef, i64 %2334, i32 0
  %2613 = insertelement <2 x i64> %2612, i64 %2336, i32 1
  %2614 = bitcast ptr addrspace(3) %820 to ptr addrspace(3)
  store <2 x i64> %2613, ptr addrspace(3) %2614, align 16
  %2615 = insertelement <2 x i64> undef, i64 %2349, i32 0
  %2616 = insertelement <2 x i64> %2615, i64 %2351, i32 1
  %2617 = bitcast ptr addrspace(3) %826 to ptr addrspace(3)
  store <2 x i64> %2616, ptr addrspace(3) %2617, align 16
  %2618 = insertelement <2 x i64> undef, i64 %2350, i32 0
  %2619 = insertelement <2 x i64> %2618, i64 %2352, i32 1
  %2620 = bitcast ptr addrspace(3) %830 to ptr addrspace(3)
  store <2 x i64> %2619, ptr addrspace(3) %2620, align 16
  %2621 = insertelement <2 x i64> undef, i64 %2365, i32 0
  %2622 = insertelement <2 x i64> %2621, i64 %2367, i32 1
  %2623 = bitcast ptr addrspace(3) %836 to ptr addrspace(3)
  store <2 x i64> %2622, ptr addrspace(3) %2623, align 16
  %2624 = insertelement <2 x i64> undef, i64 %2366, i32 0
  %2625 = insertelement <2 x i64> %2624, i64 %2368, i32 1
  %2626 = bitcast ptr addrspace(3) %840 to ptr addrspace(3)
  store <2 x i64> %2625, ptr addrspace(3) %2626, align 16
  %2627 = insertelement <2 x i64> undef, i64 %2381, i32 0
  %2628 = insertelement <2 x i64> %2627, i64 %2383, i32 1
  %2629 = bitcast ptr addrspace(3) %846 to ptr addrspace(3)
  store <2 x i64> %2628, ptr addrspace(3) %2629, align 16
  %2630 = insertelement <2 x i64> undef, i64 %2382, i32 0
  %2631 = insertelement <2 x i64> %2630, i64 %2384, i32 1
  %2632 = bitcast ptr addrspace(3) %850 to ptr addrspace(3)
  store <2 x i64> %2631, ptr addrspace(3) %2632, align 16
  %2633 = insertelement <2 x i64> undef, i64 %2397, i32 0
  %2634 = insertelement <2 x i64> %2633, i64 %2399, i32 1
  %2635 = bitcast ptr addrspace(3) %856 to ptr addrspace(3)
  store <2 x i64> %2634, ptr addrspace(3) %2635, align 16
  %2636 = insertelement <2 x i64> undef, i64 %2398, i32 0
  %2637 = insertelement <2 x i64> %2636, i64 %2400, i32 1
  %2638 = bitcast ptr addrspace(3) %860 to ptr addrspace(3)
  store <2 x i64> %2637, ptr addrspace(3) %2638, align 16
  %2639 = insertelement <2 x i64> undef, i64 %2413, i32 0
  %2640 = insertelement <2 x i64> %2639, i64 %2415, i32 1
  %2641 = bitcast ptr addrspace(3) %866 to ptr addrspace(3)
  store <2 x i64> %2640, ptr addrspace(3) %2641, align 16
  %2642 = insertelement <2 x i64> undef, i64 %2414, i32 0
  %2643 = insertelement <2 x i64> %2642, i64 %2416, i32 1
  %2644 = bitcast ptr addrspace(3) %870 to ptr addrspace(3)
  store <2 x i64> %2643, ptr addrspace(3) %2644, align 16
  %2645 = insertelement <2 x i64> undef, i64 %2429, i32 0
  %2646 = insertelement <2 x i64> %2645, i64 %2431, i32 1
  %2647 = bitcast ptr addrspace(3) %876 to ptr addrspace(3)
  store <2 x i64> %2646, ptr addrspace(3) %2647, align 16
  %2648 = insertelement <2 x i64> undef, i64 %2430, i32 0
  %2649 = insertelement <2 x i64> %2648, i64 %2432, i32 1
  %2650 = bitcast ptr addrspace(3) %880 to ptr addrspace(3)
  store <2 x i64> %2649, ptr addrspace(3) %2650, align 16
  %2651 = insertelement <2 x i64> undef, i64 %2445, i32 0
  %2652 = insertelement <2 x i64> %2651, i64 %2447, i32 1
  %2653 = bitcast ptr addrspace(3) %886 to ptr addrspace(3)
  store <2 x i64> %2652, ptr addrspace(3) %2653, align 16
  %2654 = insertelement <2 x i64> undef, i64 %2446, i32 0
  %2655 = insertelement <2 x i64> %2654, i64 %2448, i32 1
  %2656 = bitcast ptr addrspace(3) %890 to ptr addrspace(3)
  store <2 x i64> %2655, ptr addrspace(3) %2656, align 16
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 2, i32 2, i32 272) #1
  br i1 %1117, label %.critedge, label %.critedge29

.critedge:                                        ; preds = %1929
  %2657 = extractelement <2 x i64> %2512, i32 0
  %2658 = inttoptr i64 %2657 to ptr addrspace(1)
  %2659 = extractelement <2 x i64> %2510, i32 0
  %2660 = inttoptr i64 %2659 to ptr addrspace(1)
  %2661 = extractelement <2 x i64> %2508, i32 0
  %2662 = inttoptr i64 %2661 to ptr addrspace(1)
  %2663 = extractelement <2 x i64> %2506, i32 0
  %2664 = inttoptr i64 %2663 to ptr addrspace(1)
  %2665 = extractelement <2 x i64> %2504, i32 0
  %2666 = inttoptr i64 %2665 to ptr addrspace(1)
  %2667 = extractelement <2 x i64> %2502, i32 0
  %2668 = inttoptr i64 %2667 to ptr addrspace(1)
  %2669 = extractelement <2 x i64> %2500, i32 0
  %2670 = inttoptr i64 %2669 to ptr addrspace(1)
  %2671 = extractelement <2 x i64> %2498, i32 0
  %2672 = inttoptr i64 %2671 to ptr addrspace(1)
  %2673 = load <4 x i32>, ptr addrspace(1) %2672, align 16
  %2674 = load <4 x i32>, ptr addrspace(1) %2670, align 16
  %2675 = load <4 x i32>, ptr addrspace(1) %2668, align 16
  %2676 = load <4 x i32>, ptr addrspace(1) %2666, align 16
  %2677 = load <4 x i32>, ptr addrspace(1) %2664, align 16
  %2678 = load <4 x i32>, ptr addrspace(1) %2662, align 16
  %2679 = load <4 x i32>, ptr addrspace(1) %2660, align 16
  %2680 = load <4 x i32>, ptr addrspace(1) %2658, align 16
  %2681 = shl i32 %52, 6
  %2682 = or i32 %2681, 30720
  %2683 = add i32 %2682, %131
  %2684 = sext i32 %2683 to i64
  %2685 = getelementptr half, ptr addrspace(1) %2, i64 %2684
  %2686 = or i32 %2681, 28672
  %2687 = add i32 %2686, %131
  %2688 = sext i32 %2687 to i64
  %2689 = getelementptr half, ptr addrspace(1) %2, i64 %2688
  %2690 = or i32 %2681, 26624
  %2691 = add i32 %2690, %131
  %2692 = sext i32 %2691 to i64
  %2693 = getelementptr half, ptr addrspace(1) %2, i64 %2692
  %2694 = or i32 %2681, 24576
  %2695 = add i32 %2694, %131
  %2696 = sext i32 %2695 to i64
  %2697 = getelementptr half, ptr addrspace(1) %2, i64 %2696
  %2698 = or i32 %2681, 22528
  %2699 = add i32 %2698, %131
  %2700 = sext i32 %2699 to i64
  %2701 = getelementptr half, ptr addrspace(1) %2, i64 %2700
  %2702 = or i32 %2681, 20480
  %2703 = add i32 %2702, %131
  %2704 = sext i32 %2703 to i64
  %2705 = getelementptr half, ptr addrspace(1) %2, i64 %2704
  %2706 = or i32 %2681, 18432
  %2707 = add i32 %2706, %131
  %2708 = sext i32 %2707 to i64
  %2709 = getelementptr half, ptr addrspace(1) %2, i64 %2708
  %2710 = or i32 %2681, 16384
  %2711 = add i32 %2710, %131
  %2712 = sext i32 %2711 to i64
  %2713 = getelementptr half, ptr addrspace(1) %2, i64 %2712
  %2714 = or i32 %2681, 14336
  %2715 = add i32 %2714, %131
  %2716 = sext i32 %2715 to i64
  %2717 = getelementptr half, ptr addrspace(1) %2, i64 %2716
  %2718 = or i32 %2681, 12288
  %2719 = add i32 %2718, %131
  %2720 = sext i32 %2719 to i64
  %2721 = getelementptr half, ptr addrspace(1) %2, i64 %2720
  %2722 = or i32 %2681, 10240
  %2723 = add i32 %2722, %131
  %2724 = sext i32 %2723 to i64
  %2725 = getelementptr half, ptr addrspace(1) %2, i64 %2724
  %2726 = or i32 %2681, 8192
  %2727 = add i32 %2726, %131
  %2728 = sext i32 %2727 to i64
  %2729 = getelementptr half, ptr addrspace(1) %2, i64 %2728
  %2730 = or i32 %2681, 6144
  %2731 = add i32 %2730, %131
  %2732 = sext i32 %2731 to i64
  %2733 = getelementptr half, ptr addrspace(1) %2, i64 %2732
  %2734 = or i32 %2681, 4096
  %2735 = add i32 %2734, %131
  %2736 = sext i32 %2735 to i64
  %2737 = getelementptr half, ptr addrspace(1) %2, i64 %2736
  %2738 = or i32 %2681, 2048
  %2739 = add i32 %2738, %131
  %2740 = sext i32 %2739 to i64
  %2741 = getelementptr half, ptr addrspace(1) %2, i64 %2740
  %2742 = insertelement <2 x half> undef, half %.v, i32 0
  %2743 = insertelement <2 x half> %2742, half %.v1, i32 1
  %2744 = bitcast <2 x half> %2743 to i32
  %2745 = insertelement <4 x i32> undef, i32 %2744, i32 0
  %2746 = insertelement <2 x half> undef, half %.v2, i32 0
  %2747 = insertelement <2 x half> %2746, half %.v3, i32 1
  %2748 = bitcast <2 x half> %2747 to i32
  %2749 = insertelement <4 x i32> %2745, i32 %2748, i32 1
  %2750 = insertelement <2 x half> undef, half %.v4, i32 0
  %2751 = insertelement <2 x half> %2750, half %.v5, i32 1
  %2752 = bitcast <2 x half> %2751 to i32
  %2753 = insertelement <4 x i32> %2749, i32 %2752, i32 2
  %2754 = insertelement <2 x half> undef, half %.v6, i32 0
  %2755 = insertelement <2 x half> %2754, half %.v7, i32 1
  %2756 = bitcast <2 x half> %2755 to i32
  %2757 = insertelement <4 x i32> %2753, i32 %2756, i32 3
  %2758 = add i32 %2681, %131
  %2759 = sext i32 %2758 to i64
  %2760 = getelementptr half, ptr addrspace(1) %2, i64 %2759
  %2761 = bitcast ptr addrspace(1) %2760 to ptr addrspace(1)
  store <4 x i32> %2757, ptr addrspace(1) %2761, align 16
  %2762 = insertelement <2 x half> undef, half %.v8, i32 0
  %2763 = insertelement <2 x half> %2762, half %.v9, i32 1
  %2764 = bitcast <2 x half> %2763 to i32
  %2765 = insertelement <4 x i32> undef, i32 %2764, i32 0
  %2766 = insertelement <2 x half> undef, half %.v10, i32 0
  %2767 = insertelement <2 x half> %2766, half %.v11, i32 1
  %2768 = bitcast <2 x half> %2767 to i32
  %2769 = insertelement <4 x i32> %2765, i32 %2768, i32 1
  %2770 = insertelement <2 x half> undef, half %.v12, i32 0
  %2771 = insertelement <2 x half> %2770, half %.v13, i32 1
  %2772 = bitcast <2 x half> %2771 to i32
  %2773 = insertelement <4 x i32> %2769, i32 %2772, i32 2
  %2774 = insertelement <2 x half> undef, half %.v14, i32 0
  %2775 = insertelement <2 x half> %2774, half %.v15, i32 1
  %2776 = bitcast <2 x half> %2775 to i32
  %2777 = insertelement <4 x i32> %2773, i32 %2776, i32 3
  %2778 = bitcast ptr addrspace(1) %2741 to ptr addrspace(1)
  store <4 x i32> %2777, ptr addrspace(1) %2778, align 16
  %2779 = bitcast ptr addrspace(1) %2737 to ptr addrspace(1)
  store <4 x i32> %2673, ptr addrspace(1) %2779, align 16
  %2780 = bitcast ptr addrspace(1) %2733 to ptr addrspace(1)
  store <4 x i32> %2674, ptr addrspace(1) %2780, align 16
  %2781 = bitcast ptr addrspace(1) %2729 to ptr addrspace(1)
  store <4 x i32> %1914, ptr addrspace(1) %2781, align 16
  %2782 = bitcast ptr addrspace(1) %2725 to ptr addrspace(1)
  store <4 x i32> %1915, ptr addrspace(1) %2782, align 16
  %2783 = bitcast ptr addrspace(1) %2721 to ptr addrspace(1)
  store <4 x i32> %2675, ptr addrspace(1) %2783, align 16
  %2784 = bitcast ptr addrspace(1) %2717 to ptr addrspace(1)
  store <4 x i32> %2676, ptr addrspace(1) %2784, align 16
  %2785 = bitcast ptr addrspace(1) %2713 to ptr addrspace(1)
  store <4 x i32> %1913, ptr addrspace(1) %2785, align 16
  %2786 = bitcast ptr addrspace(1) %2709 to ptr addrspace(1)
  store <4 x i32> %1916, ptr addrspace(1) %2786, align 16
  %2787 = bitcast ptr addrspace(1) %2705 to ptr addrspace(1)
  store <4 x i32> %2677, ptr addrspace(1) %2787, align 16
  %2788 = bitcast ptr addrspace(1) %2701 to ptr addrspace(1)
  store <4 x i32> %2678, ptr addrspace(1) %2788, align 16
  %2789 = bitcast ptr addrspace(1) %2697 to ptr addrspace(1)
  store <4 x i32> %1912, ptr addrspace(1) %2789, align 16
  %2790 = bitcast ptr addrspace(1) %2693 to ptr addrspace(1)
  store <4 x i32> %1917, ptr addrspace(1) %2790, align 16
  %2791 = bitcast ptr addrspace(1) %2689 to ptr addrspace(1)
  store <4 x i32> %2679, ptr addrspace(1) %2791, align 16
  %2792 = bitcast ptr addrspace(1) %2685 to ptr addrspace(1)
  store <4 x i32> %2680, ptr addrspace(1) %2792, align 16
  br label %.critedge29

.critedge29:                                      ; preds = %.critedge, %1929
  ret void
}

; Function Attrs: convergent nounwind
declare spir_func void @_Z22__spirv_ControlBarrieriii(i32, i32, i32) #1

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32) #2

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32) #2

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32) #2

; uselistorder directives
uselistorder ptr @_Z22__spirv_ControlBarrieriii, { 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder ptr @_Z26__spirv_BuiltInWorkgroupIdi, { 8, 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder ptr @_Z28__spirv_BuiltInNumWorkgroupsi, { 2, 1, 0 }
uselistorder ptr @_Z32__spirv_BuiltInLocalInvocationIdi, { 2, 1, 0 }

attributes #0 = { nounwind }
attributes #1 = { convergent nounwind }
attributes #2 = { nounwind willreturn memory(none) }


!6 = distinct !{}
!387 = distinct !{i32 1, i32 1, i32 1, i32 0, i32 0, i32 1, i32 1, i32 3}
!388 = distinct !{!"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!389 = distinct !{!"long*", !"char*", !"half*", !"int", !"int", !"char*", !"char*", !"char*"}
!390 = distinct !{!"", !"", !"", !"", !"", !"", !"", !""}
!391 = distinct !{!392, !6, !395, !6, !6, !396, !396, !6}
!392 = distinct !{!393, !394}
!393 = distinct !{i32 38, i32 5}
!394 = distinct !{i32 38, i32 6}
!395 = distinct !{!393}
!396 = distinct !{!393, !397}
!397 = distinct !{i32 38, i32 7}
!398 = distinct !{i32 256, i32 1, i32 1}
!399 = distinct !{i32 32}
!400 = distinct !{!401, !402}
!401 = distinct !{i32 4469}
!402 = distinct !{i32 4470}
!403 = distinct !{!401}
