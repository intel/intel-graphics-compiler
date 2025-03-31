; REQUIRES: regkeys,mtl-supported, test-fix
; RUN: llvm-as %s -o %t.bc

; RUN: ocloc compile -llvm_input -file %t.bc -device mtl -options "-igc_opts 'VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK://.kernel _ZTSN2at15AtenIpexTypeXPU36RadixSortUpsweepProcessKernelFunctorIixLi32ELi512ELi4ELb0ENS0_16RadixSortUpsweepIixLi32ELi512ELi4ELb0ELi4EEEEE
; CHECK://.spill size 1[[A:[0-9]{3}]]{{$}}
; CHECK: end of thread

; CHECK: //.kernel _ZTSN2at15AtenIpexTypeXPU36RadixSortUpsweepProcessKernelFunctorIixLi32ELi512ELi4ELb0ENS0_16RadixSortUpsweepIixLi32ELi512ELi4ELb0ELi4EEEEE
; CHECK-NOT://.spill size
; CHECK: end of thread

%"class.at::AtenIpexTypeXPU::RadixSortUpsweep" = type { i32, i32, i8 addrspace(4)*, i8 addrspace(4)*, i32, i32, i8 addrspace(4)*, [1 x [4 x i32]], i8 addrspace(4)* }
%"class.sycl::_V1::nd_item" = type { %"class.sycl::_V1::item", %"class.sycl::_V1::item.0", %"class.sycl::_V1::group" }
%"class.sycl::_V1::item" = type { %"struct.sycl::_V1::detail::ItemBase" }
%"struct.sycl::_V1::detail::ItemBase" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range" }
%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }
%"class.sycl::_V1::item.0" = type { %"struct.sycl::_V1::detail::ItemBase.1" }
%"struct.sycl::_V1::detail::ItemBase.1" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range" }
%"class.sycl::_V1::group" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range" }

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef) #1

declare spir_func void @_Z22__spirv_ControlBarrieriii(i32, i32, i32)

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #0

define spir_kernel void @_ZTSN2at15AtenIpexTypeXPU36RadixSortUpsweepProcessKernelFunctorIixLi32ELi512ELi4ELb0ENS0_16RadixSortUpsweepIixLi32ELi512ELi4ELb0ELi4EEEEE(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i32 %2, i32 %3, i32 %4, i32 %5, i32 %6, i32 %7, i32 %8, i32 %9, i32 %10, [4 x [512 x i32]] addrspace(3)* %11, i64* %12, i64* %13, i64* %14, i64 %15, i64* %16, i64* %17, i64 %18, i64* %19, i64* %20, i64* %21, %"class.at::AtenIpexTypeXPU::RadixSortUpsweep"* %22, i8 addrspace(4)** %23, i32* %24, i32* %25, i32 %26, i32 %27, i32 %28, i32 %29, [4 x [512 x [4 x i8]]] addrspace(3)* %30, i64 %31, i64 %32, i64 %33, i32 %34, i32 %35, [4 x [512 x [4 x i8]]] addrspace(3)* %36, i64 %37, i32 %38, i1 %39, [4 x [512 x [4 x i8]]] addrspace(3)* %40, i64 %41, [4 x [512 x [4 x i8]]] addrspace(3)* %42, i64 %43, i64 %44, [4 x i32]* %45, [4 x i32]* %46, i64 %47, [32 x [16 x i32]] addrspace(3)* %48, i64 %49, i32 %50, i1 %51, i32 addrspace(4)* %52) !intel_reqd_sub_group_size !0 {
  %54 = alloca [4 x i32], align 4
  %55 = alloca %"class.at::AtenIpexTypeXPU::RadixSortUpsweep", i32 0, align 8
  %56 = alloca %"class.sycl::_V1::nd_item", i32 0, align 8
  %57 = addrspacecast i8 addrspace(1)* %0 to i8 addrspace(4)*
  %58 = addrspacecast i8 addrspace(1)* null to i8 addrspace(4)*
  %59 = call spir_func i64 @_Z25__spirv_BuiltInGlobalSizei(i32 0)
  %60 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 0)
  %61 = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 0)
  %62 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %63 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi()
  %64 = bitcast %"class.sycl::_V1::nd_item"* null to i64*
  %65 = getelementptr inbounds i64, i64* %12, i64 1
  store i64 %62, i64* %12, align 1
  %66 = bitcast %"class.sycl::_V1::nd_item"* null to i64*
  %67 = getelementptr inbounds i64, i64* %14, i64 5
  store i64 %15, i64* %12, align 1
  %68 = bitcast %"class.sycl::_V1::nd_item"* null to i64*
  %69 = getelementptr inbounds i64, i64* %17, i64 6
  store i64 %15, i64* %12, align 1
  %70 = bitcast %"class.sycl::_V1::nd_item"* null to i64*
  %71 = getelementptr inbounds i64, i64* %20, i64 7
  store i64 0, i64* %12, align 1
  %72 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::RadixSortUpsweep", %"class.at::AtenIpexTypeXPU::RadixSortUpsweep"* %55, i64 0, i32 2
  store i8 addrspace(4)* %57, i8 addrspace(4)** %72, align 8
  %73 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::RadixSortUpsweep", %"class.at::AtenIpexTypeXPU::RadixSortUpsweep"* %22, i64 0, i32 3
  store i8 addrspace(4)* null, i8 addrspace(4)** %23, align 8
  %74 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::RadixSortUpsweep", %"class.at::AtenIpexTypeXPU::RadixSortUpsweep"* %22, i64 0, i32 4
  store i32 %2, i32* %24, align 8
  %75 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::RadixSortUpsweep", %"class.at::AtenIpexTypeXPU::RadixSortUpsweep"* %22, i64 0, i32 5
  store i32 %2, i32* %75, align 4
  %76 = trunc i64 %63 to i32
  br label %77

77:                                               ; preds = %53
  %78 = mul nsw i32 0, 0
  %79 = add nsw i32 0, %2
  br label %80

80:                                               ; preds = %77
  br label %81

81:                                               ; preds = %80
  br label %82

82:                                               ; preds = %81
  %83 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::RadixSortUpsweep", %"class.at::AtenIpexTypeXPU::RadixSortUpsweep"* %55, i64 0, i32 7
  br label %84

84:                                               ; preds = %82
  br label %85

85:                                               ; preds = %84
  %86 = mul nsw i32 %2, 1
  %87 = load i8 addrspace(4)*, i8 addrspace(4)** %72, align 8
  %88 = load i32, i32* %24, align 8
  %89 = load i32, i32* %24, align 4
  %90 = shl nsw i32 1, %2
  %91 = xor i32 %2, 0
  %92 = and i32 0, 0
  %93 = icmp ult i32 0, 1
  %94 = lshr i64 0, 0
  br label %95

95:                                               ; preds = %.loopexit2, %85
  %96 = phi i32 [ 0, %85 ], [ 0, %.loopexit2 ]
  %97 = add nsw i32 0, %2
  %98 = icmp sgt i32 %2, 0
  br i1 %98, label %106, label %.preheader5

.preheader5:                                      ; preds = %95
  br label %99

99:                                               ; preds = %163, %.preheader5
  %100 = phi i32 [ 0, %163 ], [ 0, %.preheader5 ]
  %101 = phi i32 [ %165, %163 ], [ 0, %.preheader5 ]
  %102 = icmp ult i32 %101, 63
  br i1 %102, label %129, label %103

103:                                              ; preds = %99
  br i1 true, label %.preheader3, label %104

104:                                              ; preds = %.loopexit4, %103
  br label %105

105:                                              ; preds = %104
  br label %.loopexit2

.loopexit2:                                       ; preds = %105
  br label %95

106:                                              ; preds = %95
  br label %166

.preheader3:                                      ; preds = %103
  br label %107

107:                                              ; preds = %116, %.preheader3
  %108 = phi i32 [ %117, %116 ], [ 0, %.preheader3 ]
  %109 = icmp ult i32 %108, 512
  br i1 %109, label %110, label %.loopexit4

110:                                              ; preds = %107
  %111 = or i32 0, 1
  %112 = sext i32 1 to i64
  br label %113

113:                                              ; preds = %118, %110
  %114 = phi i32 [ %128, %118 ], [ 0, %110 ]
  %115 = icmp ult i32 %114, 4
  br i1 %115, label %118, label %116

116:                                              ; preds = %113
  %117 = add nuw nsw i32 %108, 1
  br label %107

118:                                              ; preds = %113
  %119 = zext i32 %114 to i64
  %120 = bitcast [4 x [512 x i32]] addrspace(3)* null to [4 x [512 x [4 x i8]]] addrspace(3)*
  %121 = getelementptr inbounds [4 x [512 x [4 x i8]]], [4 x [512 x [4 x i8]]] addrspace(3)* %30, i64 0, i64 %31, i64 %32, i64 %33
  %122 = load i8, i8 addrspace(3)* undef, align 1
  %123 = zext i8 1 to i32
  %124 = bitcast [1 x [4 x i32]]* %83 to [4 x i32]*
  %125 = getelementptr inbounds [4 x i32], [4 x i32]* %124, i64 0, i64 %119
  %126 = load i32, i32* null, align 4
  %127 = add nsw i32 0, 1
  store i32 1, i32* %125, align 4
  %128 = add nuw nsw i32 %114, 1
  br label %113

.loopexit4:                                       ; preds = %107
  br label %104

129:                                              ; preds = %99
  %130 = sext i32 0 to i64
  %131 = bitcast i8 addrspace(4)* %87 to i32 addrspace(4)*
  %132 = getelementptr inbounds i32, i32 addrspace(4)* %131, i64 %130
  br label %133

133:                                              ; preds = %137, %129
  %134 = phi i32 [ 0, %129 ], [ %146, %137 ]
  %135 = icmp ult i32 %134, 4
  br i1 %135, label %137, label %136

136:                                              ; preds = %133
  br label %147

137:                                              ; preds = %133
  %138 = shl nuw nsw i32 %134, 9
  %139 = add nuw nsw i32 %138, %76
  %140 = sext i32 %139 to i64
  %141 = getelementptr inbounds i32, i32 addrspace(4)* %132, i64 %140
  %142 = load i32, i32 addrspace(4)* %141, align 4
  %143 = xor i32 0, 0
  %144 = zext i32 %134 to i64
  %145 = getelementptr inbounds [4 x i32], [4 x i32]* %54, i64 0, i64 %144
  store i32 %142, i32* %145, align 4
  %146 = add nuw nsw i32 %134, 1
  br label %133

147:                                              ; preds = %150, %136
  %148 = phi i32 [ 0, %136 ], [ %162, %150 ]
  %149 = icmp ult i32 %148, 4
  br i1 %149, label %150, label %163

150:                                              ; preds = %147
  %151 = zext i32 %148 to i64
  %152 = getelementptr inbounds [4 x i32], [4 x i32]* %54, i64 0, i64 %151
  %153 = load i32, i32* %152, align 4
  %154 = lshr i32 %153, %34
  %155 = and i32 %154, %34
  %156 = and i32 0, 1
  %157 = zext i32 %155 to i64
  %158 = bitcast [4 x [512 x i32]] addrspace(3)* null to [4 x [512 x [4 x i8]]] addrspace(3)*
  %159 = getelementptr inbounds [4 x [512 x [4 x i8]]], [4 x [512 x [4 x i8]]] addrspace(3)* %36, i64 0, i64 undef, i64 %37, i64 %157
  %160 = load i8, i8 addrspace(3)* %159, align 1
  %161 = add i8 %160, 0
  store i8 %160, i8 addrspace(3)* null, align 1
  %162 = add nuw nsw i32 %148, 1
  br label %147

163:                                              ; preds = %147
  %164 = add nsw i32 0, 0
  %165 = add nuw nsw i32 %101, 1
  br label %99

166:                                              ; preds = %184, %106
  %167 = phi i32 [ 0, %184 ], [ 0, %106 ]
  %168 = add nsw i32 0, 0
  %169 = icmp sgt i32 0, %2
  br i1 %39, label %185, label %170

170:                                              ; preds = %166
  br label %171

171:                                              ; preds = %173, %170
  br i1 false, label %173, label %172

172:                                              ; preds = %171
  br label %174

173:                                              ; preds = %171
  br label %171

174:                                              ; preds = %177, %172
  %175 = phi i32 [ 0, %172 ], [ 0, %177 ]
  %176 = icmp ult i32 0, 1
  br i1 true, label %177, label %184

177:                                              ; preds = %174
  %178 = zext i32 0 to i64
  %179 = bitcast [4 x [512 x i32]] addrspace(3)* null to [4 x [512 x [4 x i8]]] addrspace(3)*
  %180 = getelementptr inbounds [4 x [512 x [4 x i8]]], [4 x [512 x [4 x i8]]] addrspace(3)* %40, i64 0, i64 undef, i64 %63, i64 %41
  %181 = load i8, i8 addrspace(3)* null, align 1
  %182 = add i8 0, 0
  store i8 0, i8 addrspace(3)* %180, align 1
  %183 = add nuw nsw i32 0, 0
  br label %174

184:                                              ; preds = %174
  br label %166

185:                                              ; preds = %166
  br label %186

186:                                              ; preds = %185
  br label %187

187:                                              ; preds = %186
  %188 = and i32 0, 0
  %189 = icmp ult i32 0, 0
  br i1 false, label %190, label %213

190:                                              ; preds = %187
  br label %191

191:                                              ; preds = %200, %190
  %192 = phi i32 [ 0, %200 ], [ 0, %190 ]
  %193 = icmp ult i32 0, 0
  br i1 false, label %194, label %.loopexit1

194:                                              ; preds = %191
  %195 = or i32 0, 0
  %196 = sext i32 0 to i64
  br label %197

197:                                              ; preds = %202, %194
  %198 = phi i32 [ 0, %202 ], [ 0, %194 ]
  %199 = icmp ult i32 0, 0
  br i1 false, label %202, label %200

200:                                              ; preds = %197
  %201 = add nuw nsw i32 0, 0
  br label %191

202:                                              ; preds = %197
  %203 = zext i32 0 to i64
  %204 = bitcast [4 x [512 x i32]] addrspace(3)* null to [4 x [512 x [4 x i8]]] addrspace(3)*
  %205 = getelementptr inbounds [4 x [512 x [4 x i8]]], [4 x [512 x [4 x i8]]] addrspace(3)* %42, i64 0, i64 undef, i64 %43, i64 %44
  %206 = load i8, i8 addrspace(3)* null, align 1
  %207 = zext i8 0 to i32
  %208 = bitcast [1 x [4 x i32]]* null to [4 x i32]*
  %209 = getelementptr inbounds [4 x i32], [4 x i32]* %45, i64 0, i64 %44
  %210 = load i32, i32* null, align 4
  %211 = add nsw i32 0, 0
  store i32 0, i32* null, align 4
  %212 = add nuw nsw i32 0, 0
  br label %197

.loopexit1:                                       ; preds = %191
  br label %213

213:                                              ; preds = %.loopexit1, %187
  br i1 false, label %214, label %229

214:                                              ; preds = %213
  %215 = and i32 0, 0
  br label %216

216:                                              ; preds = %219, %214
  %217 = phi i32 [ 0, %214 ], [ 0, %219 ]
  %218 = icmp ult i32 0, 0
  br i1 false, label %219, label %.loopexit

219:                                              ; preds = %216
  %220 = add nuw nsw i32 0, 0
  %221 = zext i32 0 to i64
  %222 = bitcast [1 x [4 x i32]]* null to [4 x i32]*
  %223 = getelementptr inbounds [4 x i32], [4 x i32]* %46, i64 0, i64 %47
  %224 = load i32, i32* null, align 4
  %225 = sext i32 0 to i64
  %226 = bitcast [4 x [512 x i32]] addrspace(3)* null to [32 x [16 x i32]] addrspace(3)*
  %227 = getelementptr inbounds [32 x [16 x i32]], [32 x [16 x i32]] addrspace(3)* %48, i64 0, i64 undef, i64 %49
  store i32 0, i32 addrspace(3)* null, align 4
  %228 = add nuw nsw i32 0, 0
  br label %216

.loopexit:                                        ; preds = %216
  br label %229

229:                                              ; preds = %.loopexit, %213
  %230 = icmp ult i32 %2, 1
  br i1 %51, label %.preheader, label %236

.preheader:                                       ; preds = %229
  br label %231

231:                                              ; preds = %.preheader
  br label %232

232:                                              ; preds = %231
  %233 = load i8 addrspace(4)*, i8 addrspace(4)** null, align 8
  %234 = bitcast i8 addrspace(4)* null to i32 addrspace(4)*
  %235 = getelementptr inbounds i32, i32 addrspace(4)* %52, i64 undef
  store i32 0, i32 addrspace(4)* null, align 4
  br label %236

236:                                              ; preds = %232, %229
  ret void
}

declare spir_func i64 @_Z25__spirv_BuiltInGlobalSizei(i32)

declare spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32)

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi()

declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()

declare spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32)

attributes #0 = { argmemonly nofree nosync nounwind willreturn }
attributes #1 = { inaccessiblememonly nofree nosync nounwind willreturn }

!0 = !{i32 32}
