/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <type_traits>
#include <utility>

#include "cif/common/cif.h"
#include "cif/common/library_handle.h"

namespace CIF {

template <typename BaseClass = ICIF>
class ICIFImpl : public BaseClass {
  static_assert(std::is_base_of<ICIF, BaseClass>::value, "Invalid BaseClass");

public:
  template <typename... ArgsT>
  ICIFImpl(Version_t version, ArgsT && ... args)
      : BaseClass(std::forward<ArgsT>(args)...),
        refCount(1)
  {
    this->version = version;
  }

  ~ICIFImpl() override = default;

  void Release() override {
    auto prev = refCount--;
    assert(prev >= 1);
    if (prev == 1) {
      delete this;
    }
  }

  void Retain() override { ++refCount; }

  uint32_t GetRefCount() const override {
      return refCount.load();
  }

  Version_t GetEnabledVersion() const override { return version; }

protected:
  std::atomic<uint32_t> refCount;
  Version_t version;
};

template<typename InterfaceT, typename ... ArgsT>
InterfaceT* CreateImplVer(ArgsT && ... args){
    return new ICIFImpl<InterfaceT>(InterfaceT::GetVersion(), std::forward<ArgsT>(args)...);
}

}
