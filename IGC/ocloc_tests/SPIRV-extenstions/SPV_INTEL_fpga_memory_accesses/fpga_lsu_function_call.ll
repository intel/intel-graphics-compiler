; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fpga_memory_accesses -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: internal unnamed_addr addrspace(1) constant [11 x i8] c"{params:1}\00"
; CHECK-LLVM: %[[Call:[a-z0-9_.]+]] = call spir_func i32 addrspace(4)* @accessor
; CHECK-LLVM: call spir_func i32 addrspace(4)* @_ZN8MyStructaSERKS_(i32 addrspace(4)* %[[Call]], %struct.MyStruct addrspace(4)*

; ModuleID = 'test.bc'
source_filename = "llvm-link"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%struct.MyStruct = type { i32 }

$_ZN8MyStructaSERKS_ = comdat any

$accessor = comdat any

@.str.1 = private unnamed_addr addrspace(1) constant [14 x i8] c"<invalid loc>\00", section "llvm.metadata"
@.str.2 = private unnamed_addr addrspace(1) constant [11 x i8] c"{params:1}\00", section "llvm.metadata"

define spir_func void @foo(i32* %Ptr, %struct.MyStruct* byval(%struct.MyStruct) align 4 %Val) {
entry:
  %Ptr.ascast = addrspacecast i32* %Ptr to i32 addrspace(4)*
  %Val.ascast = addrspacecast %struct.MyStruct* %Val to %struct.MyStruct addrspace(4)*
  %call = call spir_func noundef i32 addrspace(4)* @accessor(i32 addrspace(4)* %Ptr.ascast)
  %ptr1 = getelementptr inbounds [14 x i8], [14 x i8] addrspace(1)* @.str.1, i64 0, i64 0
  %ptr1.p4 = addrspacecast i8 addrspace(1)* %ptr1 to i8 addrspace(4)*
  %ptr1.p0 = addrspacecast i8 addrspace(4)* %ptr1.p4 to i8*
  %ptr2 = getelementptr inbounds [11 x i8], [11 x i8] addrspace(1)* @.str.2, i64 0, i64 0
  %ptr2.p4 = addrspacecast i8 addrspace(1)* %ptr2 to i8 addrspace(4)*
  %ptr2.p0 = addrspacecast i8 addrspace(4)* %ptr2.p4 to i8*
  %0 = call i32 addrspace(4)* @llvm.ptr.annotation.p4.p1(i32 addrspace(4)* %call, i8* %ptr2.p0, i8* %ptr1.p0, i32 0)
  %call1 = call spir_func i32 addrspace(4)* @_ZN8MyStructaSERKS_(i32 addrspace(4)* %0, %struct.MyStruct addrspace(4)* %Val.ascast)
  ret void
}

declare i32 addrspace(4)* @llvm.ptr.annotation.p4.p1(i32 addrspace(4)*, i8*, i8*, i32)

declare spir_func i32 addrspace(4)* @_ZN8MyStructaSERKS_(i32 addrspace(4)* %this, %struct.MyStruct addrspace(4)* %op)

declare spir_func i32 addrspace(4)* @accessor(i32 addrspace(4)* %this)