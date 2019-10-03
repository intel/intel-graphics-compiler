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
