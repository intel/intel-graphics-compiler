/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_MEMOPT_H_
#define _CISA_MEMOPT_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/ArrayRef.h>
#include "common/LLVMWarningsPop.hpp"
#include <stdint.h>

namespace llvm {
    class FunctionPass;
    class DataLayout;
    class StructType;
    class Type;
}

namespace IGC {
    llvm::FunctionPass* createMemOptPass(bool AllowNegativeSymPtrsForLoad, bool AllowVector8LoadStore);
    llvm::FunctionPass* createLdStCombinePass();

    // Utility for struct manipulation

    inline const char* getStructNameForSOALayout() {
        return "__StructSOALayout_";
    }
    inline const char* getStructNameForAOSLayout() {
        return "__StructAOSLayout_";
    }

    // If struct type is created by LdStCombine() for layout, return true.
    bool isLayoutStructType(const StructType* StTy);
    // If struct type is created by LdStCombine() in AOS layout, return true.
    //   note: if a struct type is a layout struct, but not in AOS layout,
    //         it must be in SOA layout.
    bool isLayoutStructTypeAOS(const StructType* StTy);

    // bitcastToUI64:
    //   return C as ui64. C must fit into 64bits.
    uint64_t bitcastToUI64(Constant* C, const DataLayout* DL);

    // getStructMemberOffsetType_1:
    //   Given a struct type 'StTy' and its 'Indices', return the type and
    //   the byte offset of the member referred to by {StTy, Indices}.
    //   This member must be at level 1.
    //   The function checks to make sure 'Indices' has size of 1.
    void getStructMemberByteOffsetAndType_1(const DataLayout* DL,
        StructType* StTy, const ArrayRef<unsigned>& Indices,
        Type*& Ty, uint32_t& ByteOffset);

    // getStructMemberOffsetType_2:
    //   Given a struct type 'StTy' and its 'Indices', return the types and
    //   the byte offsets of the member referred to by {StTy, Indices}.
    //   This member is at most at level 2. If it is at level 2, its enclosing
    //   type at level 1 must be of struct type.
    //   The returned results {Ty0, ByteOffset0} is for level 1 member; and
    //   {Ty1, ByteOffset1} is for level 2 if it is at level 2 (Ty1 must not
    //   be nullptr).
    //   The function checks to maek sure 'Indices' has size of 2 at most.
    void getStructMemberOffsetAndType_2(const DataLayout* DL,
        StructType* StTy, const ArrayRef<unsigned>& Indices,
        Type*& Ty0, uint32_t& ByteOffset0,
        Type*& Ty1, uint32_t& ByteOffset1);

    // getAllDefinedMembers
    //   Given a value 'V' of struct type, return all the indices of members
    //   that have been defined (via insertValueInst), if not sure, all
    //   indices of the struct type are returned.
    // For the example,
    //   Given struct = {i32, i32, i32, i32 }
    //
    //   %8 = insertvalue undef, i32 %3, 0
    //   %9 = insertvalue %8,    i32 %4, 1
    //  %10 = insertvalue %9,    i32 %5, 2
    //
    //  The value %9 will return {0, 1}, and %10 will return {0, 1, 2}.
    void getAllDefinedMembers(const Value* IVI,
        std::list<ArrayRef<unsigned>>& fieldsTBC);
}

#endif // _CISA_MEMOPT_H_
