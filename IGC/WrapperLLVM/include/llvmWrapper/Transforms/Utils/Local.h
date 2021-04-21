/*========================== begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

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

#ifndef IGCLLVM_TRANSFORMS_UTILS_LOCAL_H
#define IGCLLVM_TRANSFORMS_UTILS_LOCAL_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/ValueHandle.h"

namespace IGCLLVM
{
	/// In LLVM 11 first argument changed type from SmallVectorImpl<Instruction*> to SmallVectorImpl<WeakTrackingVH>
	/// For LLVM >= 11: Proxy llvm:: call.
	/// For LLVM < 11: Unpack WeakTrackingVH and call normally.
	inline void RecursivelyDeleteTriviallyDeadInstructions(
		llvm::SmallVectorImpl<llvm::WeakTrackingVH>& DeadInsts,
		const llvm::TargetLibraryInfo*				 TLI = nullptr,
		llvm::MemorySSAUpdater*					     MSSAU = nullptr)
	{
		using namespace llvm;

#if LLVM_VERSION_MAJOR < 11
		SmallVector<Instruction*, 8> instPtrsVector = SmallVector<Instruction*, 8>();

		// Unpack items of type 'WeakTrackingVH' to 'Instruction*'
		for (unsigned i = 0; i < DeadInsts.size(); i++)
		{
			Value* tmpVecItem = DeadInsts[i]; // Using 'WeakTrackingVH::operator Value*()'
			instPtrsVector.push_back(cast<Instruction>(tmpVecItem));
		}

		llvm::RecursivelyDeleteTriviallyDeadInstructions(instPtrsVector, TLI, MSSAU);
#else
		llvm::RecursivelyDeleteTriviallyDeadInstructions(DeadInsts, TLI, MSSAU);
#endif
	}
}
#endif
