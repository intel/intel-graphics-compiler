/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <array>

#include "cif/common/cif.h"
#include "cif/export/interface_creator.h"

namespace CIF{

template <template <Version_t> class Interface>
struct Multiversion
{
    using ImplT = typename Interface<CIF::BaseVersion>::Impl;
    using BaseInterfaceT = Interface<CIF::BaseVersion>;

    Multiversion(){
        this->impl = nullptr;
        std::fill(versions.begin(), versions.end(), nullptr);
    }

    template<typename ArgT, typename ... ArgsT>
    Multiversion(ArgT && arg, ArgsT && ... args)
        : Multiversion(){
        impl = new ImplT(std::forward<ArgT>(arg), std::forward<ArgsT>(args)...);
        impl->retain(nullptr);
    }

    Multiversion(ImplT *impl)
        : Multiversion(){
        this->impl = impl;
        this->impl->retain(nullptr);
    }

    template<typename ... ArgsT>
    void CreateImpl(ArgsT && ... args){
        // release interface tied-up to previous impl (if any)
        for(BaseInterfaceT *&iVer : versions){
            if(iVer == nullptr){
                continue;
            }
            iVer->Release();
            iVer = nullptr;
        }

        if(this->impl != nullptr){
            this->impl->Release(nullptr);
        }
        this->impl = new ImplT(std::forward<ArgsT>(args)...);
        this->impl->Retain(nullptr);
    }

    ~Multiversion(){
        for(BaseInterfaceT * iVer : versions){
            if(iVer == nullptr){
                continue;
            }
            iVer->Release();
        }

        if(this->impl != nullptr){
            impl->Release(nullptr);
        }
    }

    Multiversion(const Multiversion &) = delete;
    Multiversion &operator=(const Multiversion &) = delete;
    Multiversion(Multiversion &&) = delete;
    Multiversion *operator=(Multiversion &&) = delete;

    ImplT *operator->() { return impl; }

    ImplT *operator->() const { return impl; }

    ImplT *GetImpl(){
        return impl;
    }

    const ImplT *GetImpl() const {
        return impl;
    }

    const BaseInterfaceT *GetVersion(Version_t v) const {
        constexpr Version_t oldestSupportedVersion = Interface<CIF::TraitsSpecialVersion>::GetOldestSupportedVersion();
        constexpr Version_t latestSupportedVersion = Interface<CIF::TraitsSpecialVersion>::GetLatestSupportedVersion();

        if((v < oldestSupportedVersion) || (v > latestSupportedVersion)){
            return nullptr;
        }

        size_t idx = static_cast<size_t>(v - oldestSupportedVersion);
        BaseInterfaceT *curr = versions[idx];
        if(curr != nullptr){
            return curr;
        }
        versions[idx] = InterfaceCreator<Interface>::CreateInterfaceVer(v, impl);
        return versions[idx];
    }

    BaseInterfaceT *GetVersion(Version_t v) {
        return const_cast<BaseInterfaceT*>(const_cast<const Multiversion<Interface>*>(this)->GetVersion(v));
    }
protected:
    mutable std::array<BaseInterfaceT*, Interface<CIF::TraitsSpecialVersion>::GetNumSupportedVersions()> versions = {};
    ImplT *impl = nullptr;
    bool ownsImpl = false;
};

}
