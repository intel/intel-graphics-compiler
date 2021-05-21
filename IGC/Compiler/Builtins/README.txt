<!---======================= begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

This directory contains source code and tools (py, bat) of functions that emulate
instructions that have no native support on some platforms, such as mul, div, etc
on double type.

So far, those functions are generated using clang (or ocloc) into llvm bitcode offline,
with scripts provided. The bitcode is included in C++ header file. The bitcode is
loaded and linked if it is needed, during codegen pass PreCompiledFuncImport.

For now, those header files are generated in the source directory manually first,
and then manually move to the right directory. Some minor editing might be needed
for those files.  In the future, they shall be auto-generated.
