/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

#include "cif/common/id.h"

namespace CIF {

constexpr Version_t CifFrameworkVersion = 2;

namespace RAII {
template <typename T>
struct ReleaseHelper {
    void operator() (T *handle) const {
        assert(handle != nullptr);
        handle->Release();
    }
};

template <typename T>
using UPtr_t = std::unique_ptr<T, ReleaseHelper<T>>;

template <typename T> static UPtr_t<T> UPtr(T *handle) {
    return UPtr_t<T>(handle);
}

template<typename DstType, typename SrcType>
UPtr_t<DstType> RetainAndPack(SrcType * ptr){
    if(ptr == nullptr){
        return UPtr<DstType>(nullptr);
    }

    ptr->Retain();
    return UPtr<DstType>(static_cast<DstType*>(ptr));
}

template<typename DstType, typename SrcType>
UPtr_t<DstType> Pack(SrcType * ptr){
    if(ptr == nullptr){
        return UPtr<DstType>(nullptr);
    }

    return UPtr<DstType>(static_cast<DstType*>(ptr));
}

}

// generic interface for all components
struct ICIF {
  ICIF(const ICIF &) = delete;
  ICIF &operator=(const ICIF &) = delete;
  ICIF(ICIF &&) = delete;
  ICIF *operator=(ICIF &&) = delete;

  virtual void Release() = 0;
  virtual void Retain() = 0;
  virtual uint32_t GetRefCount() const = 0;

  virtual Version_t GetEnabledVersion() const = 0;
  virtual Version_t GetUnderlyingVersion() const {
      return GetEnabledVersion(); // by default : redirect to enabled version
  }
  virtual bool GetSupportedVersions(InterfaceId_t intId, Version_t &verMin,
                                    Version_t &verMax) const {
      return false; // by default : no sub-interface are supported
  }

  // get specific version of interface
  template <typename InterfaceT, Version_t version>
  RAII::UPtr_t<InterfaceT> CreateInterface() {
    static_assert((version >= InterfaceT::GetBackwardsCompatibilityVersion()) &&
                      (version <= InterfaceT::GetVersion()),
                  "Invalid version requested");
    return RAII::UPtr(reinterpret_cast<InterfaceT *>(
        CreateInterfaceImpl(InterfaceT::GetInterfaceId(), version)));
  }

  // get latest version of interface that matches interface class
  template <typename InterfaceT>
  RAII::UPtr_t<InterfaceT> CreateInterface() {
    uint64_t minVerSupported = 0;
    uint64_t maxVerSupported = 0;
    if (false == GetSupportedVersions(InterfaceT::GetInterfaceId(),
                                      minVerSupported, maxVerSupported)) {
      // interface not supported
      return RAII::UPtr<InterfaceT>(nullptr);
    }

    if ((InterfaceT::GetVersion() < minVerSupported) ||
        (InterfaceT::GetVersion() > maxVerSupported)) {
      // interface version not supported
      return RAII::UPtr<InterfaceT>(nullptr);
    }

    // get latest compatible
    uint64_t chosenVersion =
        std::min(maxVerSupported, InterfaceT::GetVersion());

    return RAII::UPtr(reinterpret_cast<InterfaceT *>(
        CreateInterfaceImpl(InterfaceT::GetInterfaceId(), chosenVersion)));
  }

protected:
  ICIF() = default;
  virtual ~ICIF() = default;

  virtual ICIF *CreateInterfaceImpl(InterfaceId_t intId, Version_t version) {
      return nullptr; // by default : no sub-interface are supported
  }
};

template <InterfaceId_t id, typename BaseClass = ICIF>
struct NamedCIF : public BaseClass {
  static constexpr InterfaceId_t GetInterfaceId() { return id; };
};

template <template <Version_t> class BaseClass, Version_t VersionP,
          Version_t BackwardsCompatibilityVersionP = VersionP,
          Version_t InheritFromVersion =
              (VersionP == BackwardsCompatibilityVersionP)
                  ? 0
                  : BackwardsCompatibilityVersionP /* TODO : assertion - backwards compatibility chain*/>
struct VersionedCIF : public BaseClass<InheritFromVersion> {
  template <typename... Args>
  VersionedCIF(Args &&... args)
      : BaseClass<InheritFromVersion>(std::forward<Args>(args)...) {}

  static constexpr Version_t GetVersion(){ return VersionP; }
  static constexpr Version_t GetBackwardsCompatibilityVersion(){ return BackwardsCompatibilityVersionP; }
  Version_t GetUnderlyingVersion() const override
  {
      return VersionP;
  }
  // Compatible versions are all versions from BackwardsCompatibilityVersion
  // up-to Version

  static_assert((GetVersion() >= MinVersion) && (GetVersion() <= MaxVersion),
                "Invalid version");
};

template <typename Interface> class HasVersion {
  template <typename I>
  static constexpr typename std::enable_if<I::GetVersion() != InvalidVersion,
                                           std::true_type>::type
  Evaluate(int);

  template <typename I> static constexpr std::false_type Evaluate(...);

public:
    static constexpr bool GetValue(){ return decltype(Evaluate<Interface>(0))::value; }
};

template <template <Version_t> class Interface>
struct FindVersion {
  template <Version_t CurrVersion>
  static constexpr Version_t GetVersion() {
    return GetVersionHelper<CurrVersion>(typename std::integral_constant<bool, HasVersion<Interface<CurrVersion>>::GetValue()>::type{});
  }

  template <Version_t CurrVersion = 1>
  static constexpr Version_t FindOldest() {
    return FindOldestHelper<CurrVersion>(typename std::integral_constant<bool, HasVersion<Interface<CurrVersion>>::GetValue()>::type{},
                                         typename std::integral_constant<bool, HasVersion<Interface<CurrVersion - 1>>::GetValue()>::type{});
  }

  template <Version_t CurrVersion = 1>
  static constexpr Version_t FindLatest(){
      return FindLatestHelper<IsLatestVer<CurrVersion>(), CurrVersion>();
  }

protected:
  template <Version_t CurrVersion>
  static constexpr
  Version_t GetVersionHelper(std::true_type hasVersion) {
    return Interface<CurrVersion>::GetVersion();
  }

  template <Version_t CurrVersion>
  static constexpr Version_t GetVersionHelper(std::false_type hasVersion) {
    return InvalidVersion;
  }

 template <Version_t CurrVersion = 1>
  static constexpr Version_t FindOldestHelper(std::false_type hasVersion, std::true_type prevHasVersion) {
    return FindOldestHelper<CurrVersion + 1>(typename std::integral_constant<bool, HasVersion<Interface<CurrVersion + 1>>::GetValue()>::type{},
                                             typename std::integral_constant<bool, HasVersion<Interface<CurrVersion>>::GetValue()>::type{});
  }

  template <Version_t CurrVersion = 1>
  static constexpr Version_t FindOldestHelper(std::false_type hasVersion, std::false_type prevHasVersion) {
    return FindOldestHelper<CurrVersion + 1>(typename std::integral_constant<bool, HasVersion<Interface<CurrVersion + 1>>::GetValue()>::type{},
                                             typename std::integral_constant<bool, HasVersion<Interface<CurrVersion>>::GetValue()>::type{});
  }

  template <Version_t CurrVersion = 1>
  static constexpr Version_t FindOldestHelper(std::true_type hasVersion, std::true_type prevHasVersion) {
    return FindOldestHelper<CurrVersion - 1>(typename std::integral_constant<bool, HasVersion<Interface<CurrVersion - 1>>::GetValue()>::type{},
                                             typename std::integral_constant<bool, HasVersion<Interface<CurrVersion - 2>>::GetValue()>::type{});
  }

  template <Version_t CurrVersion = 1>
  static constexpr Version_t FindOldestHelper(std::true_type hasVersion, std::false_type prevHasVersion) {
    return Interface<CurrVersion>::GetVersion();
  }

  template<Version_t Version>
  static constexpr bool IsLatestVer(){
      return (false == HasVersion<Interface<Version + 1>>::GetValue()) && (HasVersion<Interface<Version>>::GetValue());
  }

  template <bool IsLatest, Version_t CurrVersion = 1>
  static constexpr typename std::enable_if<IsLatest, Version_t>::type FindLatestHelper() {
    return Interface<CurrVersion>::GetVersion();
  }

  template <bool IsLatest, Version_t CurrVersion = 1>
  static constexpr typename std::enable_if<false == IsLatest, Version_t>::type FindLatestHelper() {
      return FindLatestHelper<IsLatestVer<CurrVersion+1>(), CurrVersion + 1>();
  }
};

template <template <Version_t> class Interface,
          Version_t Oldest = CIF::MinVersion>
struct SupportedVersions {
    static constexpr Version_t GetOldestSupportedVersion()
    {
        return FindVersion<Interface>::template FindOldest<Oldest>();
    }

    static constexpr Version_t GetLatestSupportedVersion()
    {
        static_assert(CIF::FindVersion<Interface>::template FindLatest<SupportedVersions<Interface, Oldest>::GetOldestSupportedVersion()>(), "Workaround for VS 2015");
        return CIF::FindVersion<Interface>::template FindLatest<SupportedVersions<Interface, Oldest>::GetOldestSupportedVersion()>();
    }

    static constexpr Version_t GetNumSupportedVersions()
    {
        return static_cast<Version_t>((GetLatestSupportedVersion() == CIF::BaseVersion) ? 0 :
                                                                                        ((GetLatestSupportedVersion() - GetOldestSupportedVersion()) + 1)
                                     );
    }
};

template <template <Version_t> class... Interfaces> struct InterfacePack;

template <template <Version_t> class Interface,
          template <Version_t> class... Interfaces>
struct InterfacePack<Interface, Interfaces...> {
  template <Version_t V> using Current = Interface<V>;
  using Next = InterfacePack<Interfaces...>;
};

template <> struct InterfacePack<> {
  using Current = InterfacePack<>;
  using Next = InterfacePack<>;
};


struct IsInterfaceIdFwdToOne
{
    template<template <Version_t> class Interface>
    static bool Call(){
        return true;
    }
};

/// Storage for versioned CIF interfaces
/// Useful in operations on sets of interfaces
template <template <Version_t> class... SupportedInterfaces>
struct InterfacesList {
  /// Calls Callable::Call with requested interface as template parameter.
  /// Arguments (args) will be forwarded as regular function parameters to Callable::Call.
  /// If requested interface is not on the list, then defaultValue is returned.
  template <typename Callable, typename RetType, typename DefaultValueT, typename... Args>
  static RetType forwardToOne(InterfaceId_t requestedInterfaceId, DefaultValueT &&defaultValue, Args &&... args) {
    return forwardToOneImpl<0, RetType, Callable, DefaultValueT, InterfacePack<SupportedInterfaces...>, Args...>(
        requestedInterfaceId, std::forward<DefaultValueT>(defaultValue), std::forward<Args>(args)...);
  }

  /// Calls Callable::Call with all contained interfaces (sequentially, one at a time) as template parameters.
  /// Arguments will be forwarded as regular function parameters to Callable::Call.
  template <typename Callable, typename RetType, typename DefaultValueT>
  static RetType forwardToOne(InterfaceId_t requestedInterfaceId, DefaultValueT &&defaultValue) {
    return forwardToOneImpl<0, RetType, Callable, DefaultValueT, InterfacePack<SupportedInterfaces...>>(
        requestedInterfaceId, std::forward<DefaultValueT>(defaultValue));
  }

  /// Calls Callable::Call with all contained interfaces (sequentially, one at a time) as template parameters.
  /// Arguments will be forwarded as regular function parameters to Callable::Call.
  template <typename Callable, typename... Args>
  static void forwardToAll(Args &&... args) {
    forwardToAllImpl<0, Callable, InterfacePack<SupportedInterfaces...>, Args...>(std::forward<Args>(args)...);
  }

  /// Returns number of interfaces on the list
  /// Could do sizeof...(SupportedInterfaces) instead, but GCC 4.8 has a bug
  static constexpr uint32_t GetNumInterfaces() {
#if defined _WIN32
      return sizeof...(SupportedInterfaces);
#else
    return GetNumInterfacesImpl<SupportedInterfaces...>();
#endif
  }

  static bool ContainsInterface(InterfaceId_t id){
      return forwardToOne<IsInterfaceIdFwdToOne, bool, bool>(id, false);
  }

protected:
  template<typename T = void>
  static constexpr uint32_t GetNumInterfacesImpl(){
      return 0;
  }

  template<template <Version_t> class InterfaceT, template <Version_t> class... RestInterfacesT>
  static constexpr uint32_t GetNumInterfacesImpl(){
      return 1 + GetNumInterfacesImpl<RestInterfacesT...>();
  }

  template <uint32_t InterfaceIt, typename RetType, typename Callable,
            typename DefaultValueT, typename IP, typename... Args>
  static
      typename std::enable_if<InterfaceIt == GetNumInterfaces(), RetType>::type
      forwardToOneImpl(InterfaceId_t requestedInterfaceId,
                  DefaultValueT &&defaultValue, Args &&... args) {
    return defaultValue;
  }

  template <uint32_t InterfaceIt, typename RetType, typename Callable,
            typename DefaultValueT, typename IP, typename... Args>
      static typename std::enable_if <
      InterfaceIt<GetNumInterfaces(), RetType>::type
      forwardToOneImpl(InterfaceId_t requestedInterfaceId,
                  DefaultValueT &&defaultValue, Args &&... args) {
    if (IP::template Current<BaseVersion>::GetInterfaceId() ==
        requestedInterfaceId) {
      return Callable::template Call<IP::template Current>(std::forward<Args>(args)...);
    } else {
      return forwardToOneImpl<InterfaceIt + 1, RetType, Callable, DefaultValueT,
                         typename IP::Next, Args...>(
          requestedInterfaceId, std::forward<DefaultValueT>(defaultValue),
          std::forward<Args>(args)...);
    }
  }

  template <uint32_t InterfaceIt, typename Callable, typename IP, typename... Args>
  static typename std::enable_if<InterfaceIt == GetNumInterfaces(), void>::type
      forwardToAllImpl(Args &&... args) {
  }

  template <uint32_t InterfaceIt, typename Callable, typename IP, typename... Args>
  static typename std::enable_if<InterfaceIt < GetNumInterfaces(), void>::type
      forwardToAllImpl(Args &&... args) {
      Callable::template Call<IP::template Current>(std::forward<Args>(args)...);
      forwardToAllImpl<InterfaceIt + 1, Callable, typename IP::Next, Args...>(std::forward<Args>(args)...);
    }
};

template <template <Version_t> class... Interfaces> struct UsedInterfaces {
  using AllUsedInterfaces = InterfacesList<Interfaces...>;
};

namespace ExplicitTemplateArgs {
template <typename T> using Inherit = T;

constexpr Version_t SetVersion(Version_t v) { return v; }

constexpr Version_t SetCompatibilityV(Version_t v) { return v; }

constexpr Version_t SetLatestVersion(Version_t v) { return v; }

constexpr Version_t SetOldestVersion(Version_t v) { return v; }
}

namespace E // namespace alias for shorter typing
{
using namespace CIF::ExplicitTemplateArgs;
}

template <typename... Types> struct TypesPack;

template <typename Type, typename... RemainingTypes>
struct TypesPack<Type, RemainingTypes...> {
  using ThisType = TypesPack<Type, RemainingTypes...>;
  using Current = Type;
  using Next = TypesPack<RemainingTypes...>;

  template<size_t Num>
  constexpr static typename std::enable_if<Num == 0, typename ThisType::Current>::type GetTypeHelper();

  template<size_t Num>
  constexpr static typename std::enable_if<(Num > 0), typename Next::template GetType<Num-1>>::type GetTypeHelper();

  constexpr static size_t Count()
  {
      return 1 + Next::Count();
  }

  template<size_t Num>
  using GetType = decltype(GetTypeHelper<Num>());
};

template <> struct TypesPack<> {
  using Current = TypesPack<>;
  using Next = TypesPack<>;
  struct InvalidTypeS;
  using InvalidType = InvalidTypeS*;

  template<size_t Num>
  constexpr static InvalidType GetTypeHelper()
  {
      return nullptr;
  }

  constexpr static size_t Count()
  {
      return 0;
  }

  template<size_t Num>
  using GetType = decltype(GetTypeHelper<Num>());
};
}
