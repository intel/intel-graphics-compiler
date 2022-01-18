/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cppcompiler.h"
#include "types.h"
#include "osinlines.h"
#include "Print.h"
#include "Debug.h"
#include "utility.h"
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
