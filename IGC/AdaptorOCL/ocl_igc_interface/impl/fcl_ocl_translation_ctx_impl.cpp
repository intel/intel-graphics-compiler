/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "ocl_igc_interface/fcl_ocl_translation_ctx.h"
#include "ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.h"

#include "cif/macros/enable.h"

namespace IGC {

OclTranslationOutputBase *CIF_GET_INTERFACE_CLASS(FclOclTranslationCtx, 1)::TranslateImpl(
                                                CIF::Version_t outVersion,
                                                CIF::Builtins::BufferSimple *src,
                                                CIF::Builtins::BufferSimple *options,
                                                CIF::Builtins::BufferSimple *internalOptions,
                                                CIF::Builtins::BufferSimple *tracingOptions,
                                                uint32_t tracingOptionsCount) {
    return CIF_GET_PIMPL()->Translate(outVersion, src, options, internalOptions,
                                      tracingOptions, tracingOptionsCount);
}

}

#include "cif/macros/disable.h"
