/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/cif.h"
#include "cif/export/cif_impl.h"

namespace CIF{

template <template <Version_t> class Interface>
struct InterfaceCreator
{
protected:
    template <Version_t currVer>
    static constexpr bool IsTooOld()
    {
        return currVer < Interface<CIF::TraitsSpecialVersion>::GetOldestSupportedVersion();
    }

    template <bool TooOld, Version_t CurrVer, typename... ArgsT>
    static typename std::enable_if<TooOld, Interface<CIF::BaseVersion>*>::type CreateInterfaceVerHelper(Version_t ver, ArgsT &&... args) {
      return nullptr;
    }

    template <bool TooOld, Version_t CurrVer, typename... ArgsT>
    static typename std::enable_if<false == TooOld, Interface<CIF::BaseVersion>*>::type CreateInterfaceVerHelper(Version_t ver, ArgsT &&... args) {
      if ((ver <= Interface<CurrVer>::GetVersion()) &&
          (ver >= Interface<CurrVer>::GetBackwardsCompatibilityVersion())) {
        return new ICIFImpl<Interface<CurrVer>>(ver, std::forward<ArgsT>(args)...);
      } else {
          return CreateInterfaceVerHelper<IsTooOld<CurrVer - 1>(), CurrVer - 1, ArgsT...>(ver, std::forward<ArgsT>(args)...);
      }
    }
public:
    template <typename... ArgsT>
    static Interface<BaseVersion> *CreateInterfaceVer(Version_t ver, ArgsT &&... args) {
      constexpr Version_t latestVer = Interface<CIF::TraitsSpecialVersion>::GetLatestSupportedVersion();
      constexpr Version_t oldestVer = Interface<CIF::TraitsSpecialVersion>::GetOldestSupportedVersion();
      if ((ver <= latestVer) && (ver >= oldestVer)) {

         return CreateInterfaceVerHelper<false, latestVer,
                                          ArgsT...>(ver, std::forward<ArgsT>(args)...);
      }

      return nullptr;
    }
};

}
