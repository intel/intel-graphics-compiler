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


#pragma once

#include <cstdint>
#include <vector>

#include "cif/common/cif.h"
#include "cif/common/compatibility.h"

namespace CIF {

struct CIFMain : public ICIF {
  virtual Version_t GetBinaryVersion() const = 0;

  template <template <Version_t> class InterfaceEntryPoint> bool IsCompatible() {
      return FindIncompatible<InterfaceEntryPoint>() == InvalidInterface;
  }

  template <template <Version_t> class InterfaceEntryPoint> InterfaceId_t FindIncompatible() {
      CompatibilityEncoder encoder;
      auto encoded = encoder.Encode<InterfaceEntryPoint>();
      return FindIncompatibleImpl(InterfaceEntryPoint<BaseVersion>::GetInterfaceId(), encoded); 
  }

  // get latest version of builtin that matches interface class
  template <typename BuiltinT> 
  RAII::UPtr_t<BuiltinT> CreateBuiltin() {
      return CreateInterface<BuiltinT>();
  }

protected:
    CIFMain() = default;

    virtual InterfaceId_t FindIncompatibleImpl(InterfaceId_t entryPointInterface, CIF::CompatibilityDataHandle handle) const = 0;
};
}
