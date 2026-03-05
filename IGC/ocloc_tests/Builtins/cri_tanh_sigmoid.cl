/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float -DTYPE1=float -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-FLOAT

// CHECK-TANH-FLOAT: math.tanh (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float2 -DTYPE1=float2 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-FLOAT2

// CHECK-TANH-FLOAT2-COUNT-2: math.tanh (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float4 -DTYPE1=float4 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-FLOAT4

// CHECK-TANH-FLOAT4-COUNT-4: math.tanh (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float8 -DTYPE1=float8 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-FLOAT8

// CHECK-TANH-FLOAT8-COUNT-8: math.tanh (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float16 -DTYPE1=float16 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-FLOAT16

// CHECK-TANH-FLOAT16-COUNT-16: math.tanh (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole -GAReArchACC false' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half -DTYPE1=half -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-HALF

// CHECK-TANH-HALF: math.tanh (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half2 -DTYPE1=half2 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-HALF2

// CHECK-TANH-HALF2-COUNT-2: math.tanh (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half4 -DTYPE1=half4 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-HALF4

// CHECK-TANH-HALF4-COUNT-4: math.tanh (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half8 -DTYPE1=half8 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-HALF8

// CHECK-TANH-HALF8-COUNT-8: math.tanh (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=1,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half16 -DTYPE1=half16 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-HALF16

// CHECK-TANH-HALF16-COUNT-16: math.tanh (32|M0)        r{{.+}}:hf    r{{.+}}:hf


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableNativeTanh=0,VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float16 -DTYPE1=float16 -DFUNCTION=tanh" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-TANH-NONNATIVE

// CHECK-TANH-NONNATIVE-NOT: math.tanh


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float -DTYPE1=float -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-FLOAT

// CHECK-SIGM-FLOAT: math.sigm (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float2 -DTYPE1=float2 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-FLOAT2

// CHECK-SIGM-FLOAT2-COUNT-2: math.sigm (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float4 -DTYPE1=float4 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-FLOAT4

// CHECK-SIGM-FLOAT4-COUNT-4: math.sigm (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float8 -DTYPE1=float8 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-FLOAT8

// CHECK-SIGM-FLOAT8-COUNT-8: math.sigm (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole' -cl-std=CL3.0 \
// RUN: -DRETTYPE=float16 -DTYPE1=float16 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-FLOAT16

// CHECK-SIGM-FLOAT16-COUNT-16: math.sigm (32|M0)        r{{[0-9]+}}.0<1>:f    r{{[0-9]+}}.0<1;1,0>:f


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole -GAReArchACC false' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half -DTYPE1=half -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-HALF

// CHECK-SIGM-HALF: math.sigm (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole -GAReArchACC false' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half2 -DTYPE1=half2 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-HALF2

// CHECK-SIGM-HALF2-COUNT-2: math.sigm (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole -GAReArchACC false' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half4 -DTYPE1=half4 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-HALF4

// CHECK-SIGM-HALF4-COUNT-4: math.sigm (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole -GAReArchACC false' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half8 -DTYPE1=half8 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-HALF8

// CHECK-SIGM-HALF8-COUNT-8: math.sigm (32|M0)        r{{.+}}:hf    r{{.+}}:hf

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'VISAOptions=-asmToConsole -GAReArchACC false' -cl-std=CL3.0 \
// RUN: -DRETTYPE=half16 -DTYPE1=half16 -DFUNCTION=sigm" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp16" | FileCheck %s --check-prefix=CHECK-SIGM-HALF16

// CHECK-SIGM-HALF16-COUNT-16: math.sigm


__kernel void math_kernel2(__global RETTYPE* out,
                          __global TYPE1* in1)
{
    size_t i = get_global_id(0);
    out[i] = FUNCTION(in1[i]);
}
