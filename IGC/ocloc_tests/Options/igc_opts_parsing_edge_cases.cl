/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Tests edge cases in igc_opts parsing with PrintDebugSettings verification.
// Number flags support both comma and whitespace separators.
// String flags require comma separators; they cannot contain '='.

// REQUIRES: regkeys, dg2-supported

// Comma-separated: numbers and strings in mixed order.
// RUN: ocloc compile -file %s \
// RUN:   -options "-igc_opts 'PrintDebugSettings=1,ForceOCLSIMDWidth=8,VISAOptions=-dotAll -disableSrc2AccSub,SetLoopUnrollThreshold=100'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=COMMA-MIXED
// COMMA-MIXED-DAG: PrintDebugSettings 1
// COMMA-MIXED-DAG: ForceOCLSIMDWidth 8
// COMMA-MIXED-DAG: VISAOptions -dotAll -disableSrc2AccSub
// COMMA-MIXED-DAG: SetLoopUnrollThreshold 100

// Comma-separated with additional spaces: numbers and strings in mixed order.
// RUN: ocloc compile -file %s \
// RUN:   -options "-igc_opts 'PrintDebugSettings=1, ForceOCLSIMDWidth=8 , VISAOptions=-dotAll -disableSrc2AccSub ,SetLoopUnrollThreshold=100'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=COMMA-SPACE-MIXED
// COMMA-SPACE-MIXED-DAG: PrintDebugSettings 1
// COMMA-SPACE-MIXED-DAG: ForceOCLSIMDWidth 8
// COMMA-SPACE-MIXED-DAG: VISAOptions -dotAll -disableSrc2AccSub
// COMMA-SPACE-MIXED-DAG: SetLoopUnrollThreshold 100

// Whitespace-separated number flags.
// Space between number flags.
// RUN: ocloc compile -file %s \
// RUN:   -options "-igc_opts 'PrintDebugSettings=1 ForceOCLSIMDWidth=8 SetLoopUnrollThreshold=100'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=SPACE-NUMS
// SPACE-NUMS-DAG: PrintDebugSettings 1
// SPACE-NUMS-DAG: ForceOCLSIMDWidth 8
// SPACE-NUMS-DAG: SetLoopUnrollThreshold 100

// Numbers then string at end, space-separated.
// RUN: ocloc compile -file %s \
// RUN:   -options "-igc_opts 'PrintDebugSettings=1 ForceOCLSIMDWidth=8 PrintAfter=SomePass'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=SPACE-NUM-STR
// SPACE-NUM-STR-DAG: PrintDebugSettings 1
// SPACE-NUM-STR-DAG: ForceOCLSIMDWidth 8
// SPACE-NUM-STR-DAG: PrintAfter SomePass

// Trailing whitespace after number value.
// RUN: ocloc compile -file %s \
// RUN:   -options "-igc_opts 'PrintDebugSettings=1 ForceOCLSIMDWidth=8 '" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=TRAILING-SPACE
// TRAILING-SPACE-DAG: PrintDebugSettings 1
// TRAILING-SPACE-DAG: ForceOCLSIMDWidth 8

// String flag not at end with whitespace separator — error ('=' in value).
// String followed by number.
// RUN: env IGC_PrintDebugSettings=1 ocloc compile -file %s \
// RUN:   -options "-igc_opts 'PrintAfter=SomePass ForceOCLSIMDWidth=8'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=STR-NOT-LAST
// STR-NOT-LAST-DAG: Failed to parse flag 'PrintAfter'
// STR-NOT-LAST-DAG: ForceOCLSIMDWidth 8

// String between two numbers, space-separated.
// RUN: env IGC_PrintDebugSettings=1 ocloc compile -file %s \
// RUN:   -options "-igc_opts 'ForceOCLSIMDWidth=8 PrintAfter=SomePass SetLoopUnrollThreshold=100'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=STR-MIDDLE
// STR-MIDDLE-DAG: Failed to parse flag 'PrintAfter'
// STR-MIDDLE-DAG: ForceOCLSIMDWidth 8
// STR-MIDDLE-DAG: SetLoopUnrollThreshold 100

// No separator — boundary check rejects the second flag.
// RUN: env IGC_PrintDebugSettings=1 ocloc compile -file %s \
// RUN:   -options "-igc_opts 'ForceOCLSIMDWidth=8SetLoopUnrollThreshold=100'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=NO-SEP
// NO-SEP: ForceOCLSIMDWidth 8
// NO-SEP-NOT: SetLoopUnrollThreshold 100

// Environment variables (IGC_<FlagName>).
// Numbers via env vars.
// RUN: env IGC_PrintDebugSettings=1 IGC_ForceOCLSIMDWidth=8 \
// RUN:   ocloc compile -file %s -device dg2 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ENV-NUMS
// ENV-NUMS-DAG: PrintDebugSettings 1
// ENV-NUMS-DAG: ForceOCLSIMDWidth 8

// String via env var.
// RUN: env IGC_PrintDebugSettings=1 IGC_PrintAfter=SomePass \
// RUN:   ocloc compile -file %s -device dg2 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ENV-STR
// ENV-STR-DAG: PrintDebugSettings 1
// ENV-STR-DAG: PrintAfter SomePass

// igc_opts overrides env var.
// RUN: env IGC_PrintDebugSettings=1 IGC_ForceOCLSIMDWidth=32 \
// RUN:   ocloc compile -file %s \
// RUN:   -options "-igc_opts 'ForceOCLSIMDWidth=8'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=ENV-OVERRIDE
// ENV-OVERRIDE-DAG: PrintDebugSettings 1
// ENV-OVERRIDE: ForceOCLSIMDWidth 8

// Number from env, string from igc_opts.
// RUN: env IGC_PrintDebugSettings=1 IGC_ForceOCLSIMDWidth=8 \
// RUN:   ocloc compile -file %s \
// RUN:   -options "-igc_opts 'PrintAfter=SomePass'" \
// RUN:   -device dg2 2>&1 | FileCheck %s --check-prefix=ENV-MIXED
// ENV-MIXED-DAG: PrintDebugSettings 1
// ENV-MIXED-DAG: ForceOCLSIMDWidth 8
// ENV-MIXED-DAG: PrintAfter SomePass

__kernel void test_kernel(int a, int b, __global int *res) { *res = a + b; }
