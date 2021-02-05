/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

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

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

// Interface : PLATFORM
//             Platform
// Interface for defining target platform

namespace IGC {

using TypeErasedEnum = uint64_t;

CIF_DECLARE_INTERFACE(Platform, "PLATFORM")

CIF_DEFINE_INTERFACE_VER(Platform, 1){
  CIF_INHERIT_CONSTRUCTOR();

  virtual TypeErasedEnum GetProductFamily() const;
  virtual void SetProductFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetPCHProductFamily() const;
  virtual void SetPCHProductFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetDisplayCoreFamily() const;
  virtual void SetDisplayCoreFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetRenderCoreFamily() const;
  virtual void SetRenderCoreFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetPlatformType() const;
  virtual void SetPlatformType(TypeErasedEnum v);

  virtual unsigned short GetDeviceID() const;
  virtual void SetDeviceID(unsigned short v);
  virtual unsigned short GetRevId() const;
  virtual void SetRevId(unsigned short v);
  virtual unsigned short GetDeviceID_PCH() const;
  virtual void SetDeviceID_PCH(unsigned short v);
  virtual unsigned short GetRevId_PCH() const;
  virtual void SetRevId_PCH(unsigned short v);

  virtual TypeErasedEnum GetGTType() const;
  virtual void SetGTType(TypeErasedEnum v);
};

CIF_GENERATE_VERSIONS_LIST(Platform);
CIF_MARK_LATEST_VERSION(PlatformLatest, Platform);
using PlatformTagOCL = PlatformLatest; // Note : can tag with different version for
                                       //        transition periods

}

#include "cif/macros/disable.h"
