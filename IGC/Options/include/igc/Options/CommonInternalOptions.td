/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines the common options accepted both by scalar and vector
// compiler.

// Common internal options {{
let Flags = InternalHelper.CommonInternalFlags in {


// -cl-intel-use-bindless-buffers
defm use_bindless_buffers : CommonFlag<"use-bindless-buffers">,
  HelpText<"Use bindless mode for buffers">;

// -cl-intel-use-bindless-images
defm use_bindless_images : CommonFlag<"use-bindless-images">,
  HelpText<"Use bindless mode for images">;

defm emit_zebin_visa_sections : CommonFlag<"emit-zebin-visa-sections">,
  HelpText<"Add vISA asm as sections in ZeBin">;

def emit_visa_only : PlainFlag<"emit-visa-only">,
  HelpText<"Compile until vISA">;

defm buffer_bounds_checking : CommonFlag<"buffer-bounds-checking">,
  HelpText<"Enable buffer bounds checking">;


defm minimum_valid_address_checking : CommonSeparate<"minimum-valid-address-checking">,
  HelpText<"Set minimal valid address">;

// -cl-intel-disable-sendwarwa, -ze-opt-disable-sendwarwa
defm disable_sendwarwa : CommonFlag<"disable-sendwarwa">,
  HelpText<"Disable SendWAR WA PVC platform only">;
}
// }} Common internal options
