//========================== begin_copyright_notice ============================
//
// Copyright (c) 2010-2021 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
//=========================== end_copyright_notice =============================

This directory contains source code and tools (py, bat) of functions that emulate
instructions that have no native support on some platforms, such as mul, div, etc
on double type.

So far, those functions are generated using clang (or ocloc) into llvm bitcode offline,
with scripts provided. The bitcode is included in C++ header file. The bitcode is
loaded and linked if it is needed, during codegen pass PreCompiledFuncImport.

For now, those header files are generated in the source directory manually first,
and then manually move to the right directory. Some minor editing might be needed
for those files.  In the future, they shall be auto-generated.
