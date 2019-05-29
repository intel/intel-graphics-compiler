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
#include "cif/helpers/error.h"

namespace CIF {

struct CIFMain : public ICIF {
  virtual Version_t GetBinaryVersion() const = 0;

  template <template <Version_t> class InterfaceEntryPoint> bool IsCompatible(const std::vector<InterfaceId_t> *interfacesToIgnore = nullptr) {
      return FindIncompatible<InterfaceEntryPoint>(interfacesToIgnore) == InvalidInterface;
  }

  template <template <Version_t> class InterfaceEntryPoint>
  InterfaceId_t FindIncompatible(const std::vector<InterfaceId_t> *interfacesToIgnore = nullptr) {
      CompatibilityEncoder encoder;
      auto encoded = encoder.Encode<InterfaceEntryPoint>(interfacesToIgnore);
      return FindIncompatibleImpl(InterfaceEntryPoint<BaseVersion>::GetInterfaceId(), encoded); 
  }

  template <template <Version_t> class InterfaceEntryPoint>
  bool FindSupportedVersions(InterfaceId_t subIntId, Version_t &verMin, Version_t &verMax) const {
      return FindSupportedVersionsImpl(InterfaceEntryPoint<BaseVersion>::GetInterfaceId(), subIntId, verMin, verMax);
  }

  // get latest version of builtin that matches interface class
  template <typename BuiltinT> 
  RAII::UPtr_t<BuiltinT> CreateBuiltin() {
      return CreateInterface<BuiltinT>();
  }

protected:
    CIFMain() = default;

    virtual InterfaceId_t FindIncompatibleImpl(InterfaceId_t entryPointInterface, CIF::CompatibilityDataHandle handle) const = 0;
    virtual bool FindSupportedVersionsImpl(InterfaceId_t entryPointInterface, InterfaceId_t interfaceToFind, Version_t &verMin, Version_t &verMax) const {
        Abort(); // make pure-virtual once all mocks addapt to new interface
        return false;
    }
};
}
