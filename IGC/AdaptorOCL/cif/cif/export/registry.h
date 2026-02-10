/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <memory>
#include <unordered_map>

#include "cif/common/cif.h"
#include "cif/common/id.h"
#include "cif/common/compatibility.h"
#include "cif/export/interface_creator.h"

namespace CIF {

namespace Helpers {
struct ForwardGetSupportedVersions {
  template <template <Version_t> class Interface> static bool Call(Version_t &verMin, Version_t &verMax) {
    verMax = Interface<CIF::TraitsSpecialVersion>::GetLatestSupportedVersion();
    verMin = Interface<CIF::TraitsSpecialVersion>::GetOldestSupportedVersion();
    return true;
  }
};
} // namespace Helpers

struct EntryPointInterfaceBase {
  EntryPointInterfaceBase() {}
  virtual ~EntryPointInterfaceBase() = default;

  virtual ICIF *Create(Version_t version, ICIF *parent) const = 0;
  virtual InterfaceId_t GetFirstIncompatible(CIF::CompatibilityDataHandle handle) const = 0;
  virtual void GetSupportedVersions(Version_t &verMin, Version_t &verMax) const = 0;
  virtual bool FindSupportedVersions(CIF::InterfaceId_t interfaceToFind, Version_t &verMin,
                                     Version_t &verMax) const = 0;
};

template <template <Version_t> class Interface> struct EntryPointInterface : EntryPointInterfaceBase {
  EntryPointInterface() {}

  ICIF *Create(Version_t version, ICIF *parent) const override {
    return CIF::InterfaceCreator<Interface>::template CreateInterfaceVer<>(version, version, parent);
  }

  InterfaceId_t GetFirstIncompatible(CIF::CompatibilityDataHandle handle) const override {
    CIF::CompatibilityEncoder::IncompatibilityData ret;
    auto incompatible = CompatibilityEncoder::GetFirstIncompatible<Interface>(handle, ret);
    if (incompatible == false) {
      return CIF::InvalidInterface;
    }

    return ret.Interface;
  }

  void GetSupportedVersions(Version_t &verMin, Version_t &verMax) const override {
    verMax = Interface<CIF::TraitsSpecialVersion>::GetLatestSupportedVersion();
    verMin = Interface<CIF::TraitsSpecialVersion>::GetOldestSupportedVersion();
  }

  bool FindSupportedVersions(CIF::InterfaceId_t interfaceToFind, Version_t &verMin, Version_t &verMax) const override {
    return Interface<CIF::TraitsSpecialVersion>::AllUsedInterfaces::template forwardToOne<
        Helpers::ForwardGetSupportedVersions, bool>(interfaceToFind, false, verMin, verMax);
  }
};

struct EntryPointRegistry {
protected:
  void Clear() { registeredEntryPoints.clear(); }

public:
  static EntryPointRegistry &Get() {
    static EntryPointRegistry r;
    return r;
  }

  template <template <Version_t> class Interface> void Register() {
    assert(registeredEntryPoints.find(Interface<BaseVersion>::GetInterfaceId()) == registeredEntryPoints.end());
    registeredEntryPoints[Interface<BaseVersion>::GetInterfaceId()] =
        std::unique_ptr<EntryPointInterfaceBase>(new EntryPointInterface<Interface>{});
  }

  EntryPointInterfaceBase *GetEntryPointInterface(InterfaceId_t interface) const {
    auto it = registeredEntryPoints.find(interface);
    if (it == registeredEntryPoints.end()) {
      return nullptr;
    }

    return &*it->second;
  }

  template <typename Sentinel = void> void RegisterAll() {}

  template <template <Version_t> class Interface, template <Version_t> class... RestInterfaces> void RegisterAll() {
    Register<Interface>();
    RegisterAll<RestInterfaces...>();
  }

#include "cif/macros/enable.h"
  CIF_ULT_EXPOSE(Clear)
#include "cif/macros/disable.h"

protected:
  using RegistryContainerT = std::unordered_map<InterfaceId_t, std::unique_ptr<EntryPointInterfaceBase>>;
  RegistryContainerT registeredEntryPoints;
};

template <template <Version_t> class Interface> struct RegisterEntryPoint {
  RegisterEntryPoint() { EntryPointRegistry::Get().Register<Interface>(); }
};

} // namespace CIF
