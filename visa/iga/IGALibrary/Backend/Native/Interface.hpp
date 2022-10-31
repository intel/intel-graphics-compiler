/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_NATIVE_INTERFACE_HPP
#define IGA_NATIVE_INTERFACE_HPP
////////////////////////////////////////////
// NATIVE ENCODER USERS USE THIS INTERFACE
#include "../../ErrorHandler.hpp"
#include "../../IR/Kernel.hpp"
#include "../DecoderOpts.hpp"
#include "../EncoderOpts.hpp"
#include "Field.hpp"
#include "InstEncoder.hpp"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace iga {
typedef std::pair<Fragment, std::string> FragmentListElem;
typedef std::vector<FragmentListElem> FragmentList;
struct FieldPtrCmp {
  bool operator()(const Fragment *f1, const Fragment *f2) const {
    return *f1 < *f2;
  }
};
typedef std::set<const Fragment *, FieldPtrCmp> FieldSet;
} // namespace iga

namespace iga {
namespace native {
bool IsEncodeSupported(const Model &m, const EncoderOpts &opts);
void Encode(const Model &m, const EncoderOpts &opts, ErrorHandler &eh,
            Kernel &k, void *&bits, size_t &bitsLen);

bool IsDecodeSupported(const Model &m, const DecoderOpts &opts);
Kernel *Decode(const Model &m, const DecoderOpts &dopts, ErrorHandler &eh,
               const void *bits, size_t bitsLen);

// for -Xifs and -Xdcmp
void DecodeFields(Loc loc, const Model &model, const void *bits,
                  FragmentList &fields, ErrorHandler &errHandler);
CompactionResult DebugCompaction(const Model &m,
                                 const void *uncompactedBits, // native bits
                                 void *compactedOutput,       // optional param
                                 CompactionDebugInfo &cbdi);

} // namespace native
} // namespace iga

#endif
