; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-14-plus
; XFAIL: llvm-16-plus

; LLVM with opaque pointers:
; TODO: llvm-as -opaque-pointers=1 %s -o %t.bc
; TODO: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_masked_gather_scatter -o %t.spv
; TODO: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_masked_gather_scatter -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: %[[#VECGATHER:]] = load <4 x {{i32|ptr}} addrspace(4){{.*}}>, {{<4 x i32 addrspace\(4\)\*>\*|ptr}}
; CHECK-LLVM: %[[#VECSCATTER:]] = load <4 x {{i32|ptr}} addrspace(4){{.*}}>, {{<4 x i32 addrspace\(4\)\*>\*|ptr}}
; CHECK-LLVM: %[[GATHER:[a-z0-9]+]] = call <4 x i32> @llvm.masked.gather.v4i32.{{v4p4i32|v4p4}}(<4 x {{i32|ptr}} addrspace(4){{.*}}> %[[#VECGATHER]], i32 4, <4 x i1> <i1 true, i1 false, i1 true, i1 true>, <4 x i32> <i32 4, i32 0, i32 1, i32 0>)
; CHECK-LLVM: call void @llvm.masked.scatter.v4i32.{{v4p4i32|v4p4}}(<4 x i32> %[[GATHER]], <4 x {{i32|ptr}} addrspace(4){{.*}}> %[[#VECSCATTER]], i32 4, <4 x i1> <i1 true, i1 true, i1 true, i1 true>)

; CHECK-LLVM-DAG: declare <4 x i32> @llvm.masked.gather.v4i32.{{v4p4i32|v4p4}}(<4 x {{i32|ptr}} addrspace(4){{.*}}>{{.*}}, i32 immarg{{.*}}, <4 x i1>{{.*}}, <4 x i32>{{.*}})
; CHECK-LLVM-DAG: declare void @llvm.masked.scatter.v4i32.{{v4p4i32|v4p4}}(<4 x i32>{{.*}}, <4 x {{i32|ptr}} addrspace(4){{.*}}>{{.*}}, i32 immarg{{.*}}, <4 x i1>{{.*}})

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir"

; Function Attrs: nounwind readnone
define spir_kernel void @foo() {
entry:
  %arg0 = alloca <4 x i32 addrspace(4)*>
  %arg1 = alloca <4 x i32 addrspace(4)*>
  %0 = load <4 x i32 addrspace(4)*>, <4 x i32 addrspace(4)*>* %arg0
  %1 = load <4 x i32 addrspace(4)*>, <4 x i32 addrspace(4)*>* %arg1
  %res = call <4 x i32> @llvm.masked.gather.v4i32.v4p4i32(<4 x i32 addrspace(4)*> %0, i32 4, <4 x i1> <i1 true, i1 false, i1 true, i1 true>, <4 x i32> <i32 4, i32 0, i32 1, i32 0>)
  call void @llvm.masked.scatter.v4i32.v4p4i32(<4 x i32> %res, <4 x i32 addrspace(4)*> %1, i32 4, <4 x i1> <i1 true, i1 true, i1 true, i1 true>)
  ret void
}

declare <4 x i32> @llvm.masked.gather.v4i32.v4p4i32(<4 x i32 addrspace(4)*>, i32, <4 x i1>, <4 x i32>)

declare void @llvm.masked.scatter.v4i32.v4p4i32(<4 x i32>, <4 x i32 addrspace(4)*>, i32, <4 x i1>)

!llvm.module.flags = !{!0}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
