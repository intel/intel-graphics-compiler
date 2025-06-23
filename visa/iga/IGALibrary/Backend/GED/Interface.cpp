/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Interface.hpp"

#include "Decoder.hpp"
#include "Encoder.hpp"

using namespace iga;

bool iga::ged::IsEncodeSupported(const Model &, const EncoderOpts &) {
  return true;
}
void iga::ged::Encode(const Model &m, const EncoderOpts &eopts,
                      ErrorHandler &eh, Kernel &k, void *&bits,
                      size_t &bitsLen) {
  Encoder enc(m, eh, eopts);
  uint32_t bitsLen32 = 0;
  try {
    enc.encodeKernel(k, k.getMemManager(), bits, bitsLen32);
  } catch (FatalError &) {
    // error already reported
  }
  bitsLen = bitsLen32;
}

bool iga::ged::IsDecodeSupported(const Model &, const DecoderOpts &) {
  return true;
}
Kernel *iga::ged::Decode(const Model &m, const DecoderOpts &dopts,
                         ErrorHandler &eh, const void *bits, size_t bitsLen) {
  Kernel *k = nullptr;
  try {
    iga::Decoder decoder(m, eh);
    k = dopts.useNumericLabels ? decoder.decodeKernelNumeric(bits, bitsLen)
                               : decoder.decodeKernelBlocks(bits, bitsLen);
  } catch (FatalError &) {
    // error already reported
  }
  return k;
}
