/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef DEPRECATION_HPP
#define DEPRECATION_HPP

// A deprecation macro
// This has to be placed on both sides of the function since
// MSVC wants it prefixed and everyone else wants it suffixed.
#ifdef _MSC_VER
#define IGA_DEPRECATED(FUNC) __declspec(deprecated) FUNC
#else
#define IGA_DEPRECATED(FUNC) FUNC __attribute__((deprecated))
#endif

#endif // DEPRECATION_HPP
