/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Definition of integer Registry-key Values
// (may merge this into igc_flags.h's macro)
enum {
    // EnableVISAStructurizer
    FLAG_SCF_Disable = 0,      // UCF (goto/join)
    FLAG_SCF_Default = 1,      // SCF as much as possible
    FLAG_SCF_Aggressive = 2,   // May do more transformations to favor SCF, not
                               // necesssarily better perf than FLAG_SCF_Default
    FLAG_SCF_UCFOnly = 3,      // [debugging] Using the structurizer, but only generate
                               // UCF (unstructured control flow, that is, goto/join/jmpi).
                               // Note that turnning off (0) the structurizer will end up
                               // with UCF as well, but going through different code path

    // Generic debug level
    FLAG_LEVEL_0 = 0,          // Debugging is off
    FLAG_LEVEL_1 = 1,          // Level 1
    FLAG_LEVEL_2 = 2,          // Level 2 : level 1 + more

    // Function-call-handling control
    FLAG_FCALL_DEFAULT = 0,    // default, compiler best-effort
    FLAG_FCALL_FORCE_INLINE = 1,
    FLAG_FCALL_FORCE_SUBROUTINE = 2,
    FLAG_FCALL_FORCE_STACKCALL = 3,
    FLAG_FCALL_FORCE_INDIRECTCALL = 4,
    FLAG_FCALL_DUMP_CALLABLE_FUNCTIONS = 5,

    // General flags with compiler-defined default value
    DEFAULTABLE_FLAG_DEFAULT = 0,
    DEFAULTABLE_FLAG_ENABLE = 1,
    DEFAULTABLE_FLAG_DISABLE = 2,

    // Debug information strip control
    FLAG_DEBUG_INFO_DONTSTRIP = 0, // default, do not strip debug information
    FLAG_DEBUG_INFO_STRIP_ALL = 1, // discard all debug information
    FLAG_DEBUG_INFO_STRIP_NONLINE = 2, // downgrade to line info only

    FLAG_LAST_Entry
};

