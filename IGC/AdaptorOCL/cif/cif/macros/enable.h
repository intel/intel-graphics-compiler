/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define CIF_FORWARD_DECLARE(TYPE, STRUCT_NAME)                                \
  template <CIF::Version_t ver> TYPE STRUCT_NAME

#define CIF_FORWARD_DECLARE_S(NAME) CIF_FORWARD_DECLARE(struct, NAME)
#define CIF_FORWARD_DECLARE_C(NAME) CIF_FORWARD_DECLARE(class, NAME)

#if defined CIF_ULT
#  define CIF_ULT_EXPOSE(FUNC_NAME)               \
      template<typename ... ArgsT>                \
      auto FUNC_NAME##_CIF_ULT(ArgsT && ... args) -> decltype(this->FUNC_NAME(std::forward<ArgsT>(args)...)) {\
           return FUNC_NAME(std::forward<ArgsT>(args)...);   \
      }
#else
#  define CIF_ULT_EXPOSE(FUNC_NAME)
#endif

#define CIF_DECLARE_INTERFACE(NAME, ID)                            \
    template <CIF::Version_t version = CIF::InvalidVersion>        \
    struct NAME;                                                   \
                                                                   \
    template <>                                                    \
    struct NAME<CIF::BaseVersion>                                  \
        : public CIF::NamedCIF<CIF::InterfaceIdCoder::Enc(ID)>     \
    {                                                              \
        struct Impl;                                               \
        virtual Impl * GetImpl(){ return pImpl; }                  \
        virtual const Impl * GetImpl() const { return pImpl; }     \
    protected:                                                     \
        ~NAME()override;                                           \
        Impl * pImpl;                                              \
        template<typename ... ArgsT>                               \
        NAME(ArgsT &&... args);                                    \
        NAME(Impl * pImpl);                                        \
        NAME(const NAME&) = delete;                                \
        NAME& operator=(const NAME&) = delete;                     \
        NAME& operator=(NAME&&) = delete;                          \
    };                                                             \
    using NAME##Base = NAME<CIF::BaseVersion>;

// For common (i.e. shared) interfaces (such as builtins), access to pimpl
// is blocked, becasue we can't reliably check if given dynamic library
// knows the correct underlying version of interface's pimpl class
#define CIF_DECLARE_COMMON_INTERFACE(NAME, ID)                     \
    template <CIF::Version_t version = CIF::InvalidVersion>        \
    struct NAME;                                                   \
                                                                   \
    template <>                                                    \
    struct NAME<CIF::BaseVersion>                                  \
        : public CIF::NamedCIF<CIF::InterfaceIdCoder::Enc(ID)>     \
    {                                                              \
        struct Impl;                                               \
    protected:                                                     \
        virtual Impl * GetImpl(){ return pImpl; }                  \
        virtual const Impl * GetImpl() const { return pImpl; }     \
        ~NAME()override;                                           \
        Impl * pImpl;                                              \
        template<typename ... ArgsT>                               \
        NAME(ArgsT &&... args);                                    \
        NAME(Impl * pImpl);                                        \
        NAME(const NAME&) = delete;                                \
        NAME& operator=(const NAME&) = delete;                     \
        NAME& operator=(NAME&&) = delete;                          \
    };                                                             \
    using NAME##Base = NAME<CIF::BaseVersion>;

#define CIF_DEFINE_INTERFACE_VER(NAME, VER)                         \
    template<>                                                      \
    struct NAME<VER> : public CIF::VersionedCIF<NAME, VER>

#define CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(NAME, VER, COMPATIBILITY_VER)\
    template<>                                                                   \
    struct NAME<VER> : public CIF::VersionedCIF<NAME, VER, COMPATIBILITY_VER>

#define CIF_INHERIT_CONSTRUCTOR() using VersionedCIF::VersionedCIF;

#define CIF_DECLARE_INTERFACE_DEPENDENCIES(DEPENDEE_INTERFACE, ...)

#define CIF_GENERATE_VERSIONS_LIST(INTERFACE)\
    template<>\
    struct INTERFACE<CIF::TraitsSpecialVersion> : public CIF::SupportedVersions<INTERFACE>, public CIF::UsedInterfaces<> {};

#define CIF_GENERATE_VERSIONS_LIST_WITH_BASE(INTERFACE, BASE)\
    template<>\
    struct INTERFACE<CIF::TraitsSpecialVersion> : public CIF::SupportedVersions<INTERFACE, BASE>, public CIF::UsedInterfaces<> {};

#define CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(INTERFACE, ...)\
    template<>\
    struct INTERFACE<CIF::TraitsSpecialVersion> : public CIF::SupportedVersions<INTERFACE>, public CIF::UsedInterfaces<__VA_ARGS__> {};

#define CIF_GENERATE_VERSIONS_LIST_WITH_BASE_AND_DECLARE_INTERFACE_DEPENDENCIES(INTERFACE, BASE, ...)\
    template<>\
    struct INTERFACE<CIF::TraitsSpecialVersion> : public CIF::SupportedVersions<INTERFACE, BASE>, public CIF::UsedInterfaces<__VA_ARGS__> {};

#define CIF_MARK_LATEST_VERSION(LATEST_INTERFACE_ALIAS, INTERFACE)\
    using LATEST_INTERFACE_ALIAS = INTERFACE<INTERFACE<CIF::TraitsSpecialVersion>::GetLatestSupportedVersion()>;

#define CIF_DECLARE_INTERFACE_PIMPL(NAME) struct NAME<CIF::BaseVersion>::Impl
#define CIF_TYPE_PIMPL(NAME) NAME<CIF::BaseVersion>::Impl
#define CIF_PIMPL_DECLARE_CONSTRUCTOR(...) Impl(__VA_ARGS__)
#define CIF_PIMPL_DECLARE_DESTRUCTOR() ~Impl()
#define CIF_GET_INTERFACE_CLASS(NAME, VER) NAME<VER>
#define CIF_GET_PIMPL() GetImpl()
#define CIF_EXPORT_ENTRY_POINTS_STATIC(...)                         \
    extern CIF::CIFMain *CreateCIFMainImpl(){                       \
        return new CIF::CIFMainImplStatic<__VA_ARGS__>;             \
    }
#define CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(NAME) \
    template<typename ... ArgsT> \
    inline NAME<CIF::BaseVersion>::NAME(ArgsT &&... args)\
                : pImpl(new Impl(std::forward<ArgsT>(args)...)){ pImpl->Retain(this); }\
    inline NAME<CIF::BaseVersion>::NAME(Impl *pImpl)\
                : pImpl(pImpl){ pImpl->Retain(this); }\
    inline NAME<CIF::BaseVersion>::~NAME() { pImpl->Release(this); }

#define CIF_PIMPL(T)\
    T<CIF::BaseVersion>::Impl
#define CIF_PIMPL_TYPENAME(T)\
     typename T<CIF::BaseVersion>::Impl

#define CREATE_CIF_PIMPL(T, ...)\
    new CIF_PIMPL_T(T)(__VA_ARGS__)
#if defined(_WIN32)
    #define CIF_CALLING_CONV __cdecl
#else
    #define CIF_CALLING_CONV
#endif
