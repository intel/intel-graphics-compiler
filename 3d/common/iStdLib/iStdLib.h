/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "cppcompiler.h"
#include "types.h"
#include "osinlines.h"
#include "Print.h"
#include "Debug.h"
#include "utility.h"
#include "utilitySSE.h"
#include "CpuUtil.h"
#include "FloatUtil.h"
#include "FloatSafe.h"
#include "Alloc.h"
#include "MemCopy.h"
#include "Threading.h"
#include "File.h"
#include "Timestamp.h"

// Clients may choose not to included template classes
#ifndef ISTD_DISABLE_OBJECT_TYPES
#include "Object.h"
#include "Array.h"
#include "BinaryTree.h"
#include "BitSet.h"
#include "FastMask.h"
#include "StaticBitSet.h"
#include "LinearAllocator.h"
#include "Buffer.h"
#include "DisjointSet.h"
#include "HashTable.h"
#include "LinkedList.h"
#include "Queue.h"
#include "SparseSet.h"
#include "Stack.h"
#include "LinearStack.h"
#include "Stream.h"
#include "String.h"
#include "Vector.h"
#include "LRUSet.h"
#include "LruHashTable.h"
#endif

