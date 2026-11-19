/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// fake stdint.h
// This file is needed, due the fact we are including this header for BiFModule: inc/common/igfxfmid.h
// where we need stdint.h - It's better to create own fake stdint.h (for few typedef declaration) for this,
// rather than including the system stdint.h (and the whole dependency of including other header needed by stdint.h)

#pragma once

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef int int32_t;
typedef short int16_t;
typedef char int8_t;
