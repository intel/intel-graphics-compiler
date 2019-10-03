/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

// Definition of integer Registry-key Values
// (may merge this into igc_flags.def's macro)
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

    FLAG_LAST_Entry
};

