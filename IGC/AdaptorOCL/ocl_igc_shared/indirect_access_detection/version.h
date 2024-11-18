/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file stores version of indirect access detection mechanism that is supported
// by IGC. The version is used to generate .note.intelgt.compat in ZEBinary. It is also
// shared with Neo, so that a decision whether a kernel binary requires recompilation can
// be made.

#pragma once

const uint32_t INDIRECT_ACCESS_DETECTION_VERSION = 8;
