/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
