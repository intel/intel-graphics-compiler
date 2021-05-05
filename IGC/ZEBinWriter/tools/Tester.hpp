/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===- Tester.hpp -----------------------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// \file
// The tester of ZEInfo reader and writer
//===----------------------------------------------------------------------===//

#ifndef ZE_TESTER_HPP
#define ZE_TESTER_HPP
namespace zebin {

class Tester {
public:
    static void testZEInfoOutput();
    static void testELFOutput();
};

} // namespace zebin

#endif // ZE_TESTER_HPP
