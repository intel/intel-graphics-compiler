/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/builtins/builtins_registry.h"
#include "cif/common/cif_main.h"
#include "cif/export/build/binary_version.h"
#include "cif/export/cif_impl.h"
#include "cif/export/registry.h"

namespace CIF {

namespace Helpers {
  struct ForwardCreateInterfaceImpl {
    template <template <Version_t> class Interface, typename ... ArgsT>
    static Interface<CIF::BaseVersion> *Call(Version_t version, ArgsT && ... args) {
      return CIF::InterfaceCreator<Interface>::template CreateInterfaceVer<ArgsT...>(version, std::forward<ArgsT>(args)...);
    }
  };

  struct ForwardFindSupportedVersions {
      template <template <Version_t> class Interface>
      static bool Call(InterfaceId_t interfaceToFind, Version_t &verMin, Version_t &verMax) {
          return Interface<CIF::TraitsSpecialVersion>::AllUsedInterfaces::template forwardToOne<ForwardGetSupportedVersions, bool>(interfaceToFind, false, verMin, verMax);
      }
  };

  struct ForwardGetFirstIncompatible {
    template <template <Version_t> class Interface>
    static InterfaceId_t Call(CIF::CompatibilityDataHandle handle) {
        CIF::CompatibilityEncoder::IncompatibilityData ret;
        auto incompatible = CompatibilityEncoder::GetFirstIncompatible<Interface>(handle, ret);
        if(incompatible == false){
            return CIF::InvalidInterface;
        }

        return ret.Interface;
    }
  };
}

// STATIC VERSION - based on entry point templates list
template <template <Version_t> class... EntryPointInterfaces>
struct CIFMainImplStatic : ICIFImpl<CIF::CIFMain> {
  using EntryPointInterfacesList = InterfacesList<EntryPointInterfaces...>;

  CIFMainImplStatic()
      :  ICIFImpl<CIF::CIFMain>(CifFrameworkVersion){
  }

  Version_t GetBinaryVersion() const override {
    return CIF::Build::GetBinaryVersion();
  }

  ICIF *CreateInterfaceImpl(InterfaceId_t entryPointInterface, Version_t version) override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        return CIF::Builtins::Create(entryPointInterface, version, this);
    }
    return EntryPointInterfacesList::template forwardToOne<Helpers::ForwardCreateInterfaceImpl, ICIF *, ICIF *>(entryPointInterface, nullptr, version, version, this);
  }

  bool GetSupportedVersions(InterfaceId_t entryPointInterface, Version_t &verMin, Version_t &verMax) const override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        return CIF::Builtins::GetSupportedVersions(entryPointInterface, verMin, verMax);
    }
    return EntryPointInterfacesList::template forwardToOne<Helpers::ForwardGetSupportedVersions, bool>(entryPointInterface, false, verMin, verMax);
  }

  bool FindSupportedVersionsImpl(InterfaceId_t entryPointInterface, InterfaceId_t interfaceToFind, Version_t &verMin, Version_t &verMax) const override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        assert(entryPointInterface == interfaceToFind); // builtins don't have subinterfaces
        return CIF::Builtins::GetSupportedVersions(entryPointInterface, verMin, verMax);
    }
    return EntryPointInterfacesList::template forwardToOne<Helpers::ForwardFindSupportedVersions, bool>(entryPointInterface, false, interfaceToFind, verMin, verMax);
  }

  InterfaceId_t FindIncompatibleImpl(InterfaceId_t entryPointInterface, CIF::CompatibilityDataHandle handle) const override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        return CIF::Builtins::FindIncompatible(entryPointInterface, handle);
    }
    return EntryPointInterfacesList::template forwardToOne<Helpers::ForwardGetFirstIncompatible, InterfaceId_t>(entryPointInterface, entryPointInterface, handle);
  }
};

// DYNAMIC VERSION - based on registry
struct CIFMainImplRegistry : ICIFImpl<CIF::CIFMain> {
  CIFMainImplRegistry()
      :  ICIFImpl<CIF::CIFMain>(CifFrameworkVersion){
  }

  Version_t GetBinaryVersion() const override {
    return CIF::Build::GetBinaryVersion();
  }

  ICIF *CreateInterfaceImpl(InterfaceId_t entryPointInterface, Version_t version) override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        return CIF::Builtins::Create(entryPointInterface, version, this);
    }
    auto entryPointInfo = CIF::EntryPointRegistry::Get().GetEntryPointInterface(entryPointInterface);
    if(entryPointInfo == nullptr){
        // interface info not availabe in the registry
        return nullptr;
    }
    return entryPointInfo->Create(version, this);
  }

  bool GetSupportedVersions(InterfaceId_t entryPointInterface, Version_t &verMin, Version_t &verMax) const override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        return CIF::Builtins::GetSupportedVersions(entryPointInterface, verMin, verMax);
    }
    auto entryPointInfo = CIF::EntryPointRegistry::Get().GetEntryPointInterface(entryPointInterface);
    if(entryPointInfo == nullptr){
        // interace info not availabe in the registry
        return false;
    }
    entryPointInfo->GetSupportedVersions(verMin, verMax);
    return true;
  }

  bool FindSupportedVersionsImpl(InterfaceId_t entryPointInterface, InterfaceId_t interfaceToFind, Version_t &verMin, Version_t &verMax) const override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        assert(entryPointInterface == interfaceToFind); // builtins don't have subinterfaces
        GetSupportedVersions(entryPointInterface, verMin, verMax);
        return false;
    }
    auto entryPointInfo = CIF::EntryPointRegistry::Get().GetEntryPointInterface(entryPointInterface);
    if(entryPointInfo == nullptr){
        // interface info not availabe in the registry
        return false;
    }

    return entryPointInfo->FindSupportedVersions(interfaceToFind, verMin, verMax);
  }

  InterfaceId_t FindIncompatibleImpl(InterfaceId_t entryPointInterface, CIF::CompatibilityDataHandle handle) const override {
    if(CIF::Builtins::IsBuiltin(entryPointInterface)){
        return CIF::Builtins::FindIncompatible(entryPointInterface, handle);
    }
    auto entryPointInfo = CIF::EntryPointRegistry::Get().GetEntryPointInterface(entryPointInterface);
    if(entryPointInfo == nullptr){
        // interface info not availabe in the registry
        return entryPointInterface;
    }

    return entryPointInfo->GetFirstIncompatible(handle);
  }
};
}
