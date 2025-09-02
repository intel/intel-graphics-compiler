; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_masked_gather_scatter -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s

; CHECK: alloca <4 x ptr addrspace(4)>
; CHECK: alloca <4 x ptr addrspace(4)>
; CHECK: load <4 x ptr addrspace(4)>, ptr
; CHECK: store <4 x ptr addrspace(4)> %[[#]], ptr
; CHECK: addrspacecast <4 x ptr addrspace(4)> %{{.*}} to <4 x ptr addrspace(1)>
; CHECK: call spir_func <4 x ptr addrspace(1)> @boo(<4 x ptr addrspace(1)>
; CHECK: getelementptr inbounds i32, <4 x ptr addrspace(1)> %{{.*}}, i32 1

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir"

; Function Attrs: nounwind readnone
define spir_kernel void @foo() {
entry:
  %arg1 = alloca <4 x ptr addrspace(4)>
  %arg2 = alloca <4 x ptr addrspace(4)>
  %0 = load <4 x ptr addrspace(4)>, ptr %arg1
  store <4 x ptr addrspace(4)> %0, ptr %arg2
  %tmp1 = addrspacecast <4 x ptr addrspace(4)> %0 to  <4 x ptr addrspace(1)>
  %tmp2 = call <4 x ptr addrspace(1)> @boo(<4 x ptr addrspace(1)> %tmp1)
  %tmp3 = getelementptr inbounds i32, <4 x ptr addrspace(1)> %tmp2, i32 1
  ret void
}

declare <4 x ptr addrspace(1)> @boo(<4 x ptr addrspace(1)> %a)

!llvm.module.flags = !{!0}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}