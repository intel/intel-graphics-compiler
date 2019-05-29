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
