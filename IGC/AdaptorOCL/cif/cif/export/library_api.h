/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#pragma once
#ifdef CIF_EXPORT
#  undef CIF_EXPORT
#endif

#ifdef CIF_IMPORT
#  undef CIF_IMPORT
#endif

#define CIF_EXPORT
#include "cif/common/library_api.h"
#undef CIF_EXPORT

