/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG1
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG1-WIDEOFF

// CHECK-LONG1-COUNT-1: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-LONG1-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long2" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG2
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long2" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG2-WIDEOFF

// CHECK-LONG2-COUNT-2: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-LONG2-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long4" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG4
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long4" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG4-WIDEOFF

// CHECK-LONG4-COUNT-4: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-LONG4-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long8" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG8
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long8" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG8-WIDEOFF

// CHECK-LONG8-COUNT-8: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-LONG8-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long16" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG16
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=long16" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-LONG16-WIDEOFF

// CHECK-LONG16-COUNT-16: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-LONG16-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG1
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG1-WIDEOFF

// CHECK-ULONG1-COUNT-1: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-ULONG1-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong2" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG2
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong2" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG2-WIDEOFF

// CHECK-ULONG2-COUNT-2: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-ULONG2-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong4" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG4
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong4" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG4-WIDEOFF

// CHECK-ULONG4-COUNT-4: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-ULONG4-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong8" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG8
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong8" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG8-WIDEOFF

// CHECK-ULONG8-COUNT-8: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-ULONG8-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong16" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG16
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts ',EnableWideMulMad=0,PrintToConsole=1,PrintAfter=BuiltinsConverterFunction' -DLONG_TYPE=ulong16" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s --check-prefix=CHECK-ULONG16-WIDEOFF

// CHECK-ULONG16-COUNT-16: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})
// CHECK-ULONG16-WIDEOFF-NOT: call i64 @llvm.genx.GenISA.WideUMulHi(i64 %{{.+}}, i64 %{{.+}})

__kernel void sample_test(__global LONG_TYPE *sourceA,
                          __global LONG_TYPE *sourceB,
                          __global LONG_TYPE *destValues) {
  int tid = get_global_id(0);
  LONG_TYPE sA = sourceA[tid];
  LONG_TYPE sB = sourceB[tid];
  LONG_TYPE dst = mul_hi(sA, sB);
  destValues[tid] = dst;
}
