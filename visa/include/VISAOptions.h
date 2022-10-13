/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_OPTIONS_H
#define VISA_OPTIONS_H

// List of available vISA options
#undef DEF_VISA_OPTION
#define DEF_VISA_OPTION(ENUM, TYPE, STR, VALorMSG, DEFAULT_VAL) ENUM,

typedef enum {
  vISA_OPTIONS_UNINIT = 0,
#include "VISAOptionsDefs.h"
  vISA_NUM_OPTIONS
} vISAOptions;

#endif
