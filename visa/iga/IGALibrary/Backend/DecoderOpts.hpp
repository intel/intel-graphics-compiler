/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BACKEND_DECODER_OPTS
#define IGA_BACKEND_DECODER_OPTS
namespace iga {
struct DecoderOpts {
  bool useNumericLabels;

  DecoderOpts(bool _useNumericLabels = false)
      : useNumericLabels(_useNumericLabels) {}
};
} // namespace iga
#endif
