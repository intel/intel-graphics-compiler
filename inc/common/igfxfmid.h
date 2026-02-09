/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __IGFXFMID_H__
#define __IGFXFMID_H__

typedef enum {
  IGFX_UNKNOWN = 0,
  IGFX_BROADWELL = 16,
  IGFX_CHERRYVIEW,
  IGFX_SKYLAKE,
  IGFX_KABYLAKE,
  IGFX_COFFEELAKE,
  IGFX_WILLOWVIEW,
  IGFX_BROXTON,
  IGFX_GEMINILAKE,
  IGFX_CANNONLAKE,
  IGFX_ICELAKE,
  IGFX_ICELAKE_LP,
  IGFX_LAKEFIELD,
  IGFX_JASPERLAKE,
  IGFX_ELKHARTLAKE = IGFX_JASPERLAKE,

  IGFX_TIGERLAKE_LP,
  IGFX_ROCKETLAKE,
  IGFX_ALDERLAKE_S,
  IGFX_ALDERLAKE_P,
  IGFX_ALDERLAKE_N,

  IGFX_DG1 = 1210,
  IGFX_XE_HP_SDV = 1250,
  IGFX_DG2 = 1270, // aka - ACM/Alchemist
  IGFX_PVC = 1271,
  IGFX_METEORLAKE = 1272,
  IGFX_ARROWLAKE = 1273,
  IGFX_BMG = 1274,
  IGFX_LUNARLAKE = 1275,
  IGFX_PTL = 1300,
  IGFX_NVL_XE3G = 1340,
  IGFX_NVL = 1360,
  IGFX_CRI = 1380,
  IGFX_MAX_PRODUCT,

  IGFX_GENNEXT = 0x7ffffffe,
  PRODUCT_FAMILY_FORCE_ULONG = 0x7fffffff
} PRODUCT_FAMILY;

typedef enum {
    PCH_UNKNOWN    = 0,
    PCH_IBX,            // Ibexpeak
    PCH_CPT,            // Cougarpoint,
    PCH_CPTR,           // Cougarpoint Refresh,
    PCH_PPT,            // Panther Point
    PCH_LPT,            // Lynx Point
    PCH_LPTR,           // Lynx Point Refresh
    PCH_WPT,            // Wildcat point
    PCH_SPT,            // Sunrise point
    PCH_KBP,            // Kabylake PCH
    PCH_CNP_LP,         // Cannonlake LP PCH
    PCH_CNP_H,          // Cannonlake Halo PCH
    PCH_ICP_LP,         // ICL LP PCH
    PCH_ICP_N,          // ICL N PCH
    PCH_ICP_HP,         // ICL HP PCH
    PCH_LKF,            // LKF PCH
    PCH_TGL_LP,         // TGL LP PCH
    PCH_PRODUCT_FAMILY_FORCE_ULONG = 0x7fffffff
} PCH_PRODUCT_FAMILY;

typedef enum {
    IGFX_UNKNOWN_CORE = 0,
    IGFX_GEN3_CORE    = 1,      // Gen3 Family
    IGFX_GEN3_5_CORE  = 2,      // Gen3.5 Family
    IGFX_GEN4_CORE    = 3,      // Gen4 Family
    IGFX_GEN4_5_CORE  = 4,      // Gen4.5 Family
    IGFX_GEN5_CORE    = 5,      // Gen5 Family
    IGFX_GEN5_5_CORE  = 6,      // Gen5.5 Family
    IGFX_GEN5_75_CORE = 7,      // Gen5.75 Family
    IGFX_GEN6_CORE    = 8,      // Gen6 Family
    IGFX_GEN7_CORE    = 9,      // Gen7 Family
    IGFX_GEN7_5_CORE  = 10,     // Gen7.5 Family
    IGFX_GEN8_CORE    = 11,     // Gen8 Family
    IGFX_GEN9_CORE    = 12,     // Gen9 Family
    IGFX_GEN10_CORE   = 13,     // Gen10 Family
    IGFX_GEN10LP_CORE = 14,     // Gen10 LP Family
    IGFX_GEN11_CORE   = 15,     // Gen11 Family
    IGFX_GEN11LP_CORE = 16,     // Gen11 LP Family
    IGFX_GEN12_CORE   = 17,     // Gen12 Family
    IGFX_GEN12LP_CORE = 18,     // Gen12 LP Family
    IGFX_XE_HP_CORE   = 0x0c05, // XeHP Family
    IGFX_XE_HPG_CORE  = 0x0c07, // XE_HPG Family
    IGFX_XE_HPC_CORE  = 0x0c08, // XE_HPC Family
    IGFX_XE2_HPG_CORE = 0x0c09, // XE2_HPG Family
    IGFX_XE3_CORE     = 0x1e00, // XE3 Family
    IGFX_XE3P_CORE    = 0x2300, // XE3P Family
    IGFX_MAX_CORE,              // Max Family, for lookup table

    IGFX_GENNEXT_CORE          = 0x7ffffffe,  //GenNext
    GFXCORE_FAMILY_FORCE_ULONG = 0x7fffffff
} GFXCORE_FAMILY;

typedef enum {
    IGFX_SKU_NONE       = 0,
    IGFX_SKU_ULX        = 1,
    IGFX_SKU_ULT        = 2,
    IGFX_SKU_T          = 3,
    IGFX_SKU_ALL        = 0xff
} PLATFORM_SKU;

typedef enum __GTTYPE
{
    GTTYPE_GT1 = 0x0,
    GTTYPE_GT2,
    GTTYPE_GT2_FUSED_TO_GT1,
    GTTYPE_GT2_FUSED_TO_GT1_6, //IVB
    GTTYPE_GTL, // HSW
    GTTYPE_GTM, // HSW
    GTTYPE_GTH, // HSW
    GTTYPE_GT1_5,//HSW
    GTTYPE_GT1_75,//HSW
    GTTYPE_GT3,//BDW
    GTTYPE_GT4,//BDW
    GTTYPE_GT0,//BDW
    GTTYPE_GTA,// BXT
    GTTYPE_GTC,// BXT
    GTTYPE_GTX, // BXT
    GTTYPE_GT2_5,//CNL
    GTTYPE_GT3_5,//SKL
    GTTYPE_GT0_5,//CNL
    GTTYPE_UNDEFINED,//Always at the end.
}GTTYPE, *PGTTYPE;

// *************************************************************************
// GFX_GMD_ID: Definition to hold IP major/minor version and the revision ID
// *************************************************************************
typedef struct GFX_GMD_ID_DEF
{
    union
    {
        struct
        {
            unsigned int    RevisionID : 6;
            unsigned int    Reserved : 8;
            unsigned int    GMDRelease : 8;
            unsigned int    GMDArch : 10;
        }GmdID;
        unsigned int    Value;
    };
}GFX_GMD_ID;

#define GFX_GMD_ARCH_12                          (12)
#define GFX_GMD_ARCH_20                          (20)
#define GFX_GMD_ARCH_30                          (30)

#define GFX_GMD_ARCH_12_RELEASE_XE_LP_MD                 (70)
#define GFX_GMD_ARCH_12_RELEASE_XE_LP_LG                 (71)
#define GFX_GMD_ARCH_12_RELEASE_XE_LPG_PLUS_1274         (74)

#define GFX_GMD_ARCH_20_RELEASE_XE2_HPG_2001             (1)
#define GFX_GMD_ARCH_20_RELEASE_XE2_HPG_2002             (2)
#define GFX_GMD_ARCH_20_RELEASE_XE2_LPG                  (4)

#define GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3000             (0)
#define GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3001             (1)
#define GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3003             (3)
#define GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3004             (4)
#define GFX_GMD_ARCH_30_RELEASE_XE3_LPG_3005             (5)

#define GFX_GET_GMD_ARCH_VERSION_RENDER(p)                ((p).sRenderBlockID.GmdID.GMDArch)
#define GFX_GET_GMD_ARCH_VERSION_DISPLAY(p)               ((p).sDisplayBlockID.GmdID.GMDArch)
#define GFX_GET_GMD_ARCH_VERSION_MEDIA(p)                 ((p).sMediaBlockID.GmdID.GMDArch)
#define GFX_GET_GMD_RELEASE_VERSION_RENDER(p)             ((p).sRenderBlockID.GmdID.GMDRelease)
#define GFX_GET_GMD_RELEASE_VERSION_DISPLAY(p)            ((p).sDisplayBlockID.GmdID.GMDRelease)
#define GFX_GET_GMD_RELEASE_VERSION_MEDIA(p)              ((p).sMediaBlockID.GmdID.GMDRelease)
#define GFX_GET_GMD_REV_ID_RENDER(p)                      ((p).sRenderBlockID.GmdID.RevisionID)
#define GFX_GET_GMD_REV_ID_DISPLAY(p)                     ((p).sDisplayBlockID.GmdID.RevisionID)
#define GFX_GET_GMD_REV_ID_MEDIA(p)                       ((p).sMediaBlockID.GmdID.RevisionID)

/////////////////////////////////////////////////////////////////
//
//    Platform types which are used during Sku/Wa initialization.
//
#ifndef _COMMON_PPA
    typedef enum {
        PLATFORM_NONE       = 0x00,
        PLATFORM_DESKTOP    = 0x01,
        PLATFORM_MOBILE     = 0x02,
        PLATFORM_TABLET     = 0X03,
        PLATFORM_ALL        = 0xff, // flag used for applying any feature/WA for All platform types
    } PLATFORM_TYPE;
#endif
typedef struct PLATFORM_STR {
    PRODUCT_FAMILY      eProductFamily;
    PCH_PRODUCT_FAMILY  ePCHProductFamily;
    GFXCORE_FAMILY      eDisplayCoreFamily;
    GFXCORE_FAMILY      eRenderCoreFamily;
    #ifndef _COMMON_PPA
    PLATFORM_TYPE       ePlatformType;
    #endif

    unsigned short      usDeviceID;
    unsigned short      usRevId;
    unsigned short      usDeviceID_PCH;
    unsigned short      usRevId_PCH;
    // GT Type
    // Note: Is valid only till Gen9. From Gen10 SKUs are not identified by any GT flags. 'GT_SYSTEM_INFO' should be used instead.
    GTTYPE              eGTType;

    // Supported from MTL onwards that will hold IP's major/minor version and their individual RevID
    GFX_GMD_ID          sDisplayBlockID;
    GFX_GMD_ID          sDisplayPicaBlockID;
    GFX_GMD_ID          sRenderBlockID;
    GFX_GMD_ID          sMediaBlockID;

    // Since we override usRevId field for certain platforms ex: RPLS B0, RPLP J0,
    // This field is required to hold original revid derived from pci config space which is further used to share outside KMD
    unsigned short      usOriginalRevIdFromPciConfig;
    unsigned short      padding;
} PLATFORM;

// add enums at the end
typedef enum __SKUIDTYPE
{
    SKU_FULL_TYPE = 0x0,
    SKU_VALUE_TYPE,
    SKU_PLUS_FULL_TYPE,
    SKU_PLUS_VALUE_TYPE,
    SKU_T_TYPE,
    SKU_PLUS_T_TYPE,
    SKU_P_TYPE,
    SKU_PLUS_P_TYPE,
    SKU_SMALL_TYPE,
    SKU_LIGHT_TYPE,
    SKU_N_TYPE
}SKUIDTYPE, *PSKUIDTYPE;

typedef enum __CPUTYPE
{
    CPU_UNDEFINED = 0x0,
    CPU_CORE_I3,
    CPU_CORE_I5,
    CPU_CORE_I7,
    CPU_PENTIUM,
    CPU_CELERON,
    CPU_CORE,
    CPU_VPRO,
    CPU_SUPER_SKU,
    CPU_ATOM,
    CPU_CORE1,
    CPU_CORE2,
    CPU_WS,
    CPU_SERVER,
    CPU_CORE_I5_I7,
    CPU_COREX1_4,
    CPU_ULX_PENTIUM,
    CPU_MB_WORKSTATION,
    CPU_DT_WORKSTATION,
    CPU_M3,
    CPU_M5,
    CPU_M7,
    CPU_MEDIA_SERVER //Added for KBL
}CPUTYPE, *PCPUTYPE;

// the code below convert platform real revision number to pre-defined revision number, the revision will be set as follow
// REVISION_A0 - this will include all incarnations for A stepping in all packages types A = {A0}
// REVISION_A1 - this will include all incarnations for A stepping in all packages types A = {A1}
// REVISION_A3 - this will include all incarnations for A stepping in all packages types A = {A3,...,A7}
// REVISION_B - this will include all incarnations for B stepping in all packages types B = {B0,B1,..,B7}
// REVISION_C - this will include all incarnations for C stepping in all packages types C = {C0,C1,..,C7}
// REVISION_D - this will include all incarnations for C stepping in all packages types C = {D0,D1}
// REVISION_K - this will include all incarnations for K stepping in all packages types K = {K0,K1,..,K7}
typedef enum __REVID
{
    REVISION_A0 = 0,
    REVISION_A1, //1
    REVISION_A3,//2
    REVISION_B,//3
    REVISION_C,//4
    REVISION_D,//5
    REVISION_K//6
}REVID, *PREVID;

typedef enum __NATIVEGTTYPE
{
    NATIVEGTTYPE_HSW_UNDEFINED  = 0x00,
    NATIVEGTTYPE_HSW_GT1        = 0x01,
    NATIVEGTTYPE_HSW_GT2        = 0x02,
    NATIVEGTTYPE_HSW_GT3        = 0x03,
}NATIVEGTTYPE;

// Following macros return true/false depending on the current PCH family
#define PCH_IS_PRODUCT(p, r)            ( (p).ePCHProductFamily == r )
#define PCH_GET_CURRENT_PRODUCT(p)      ( (p).ePCHProductFamily )

// These macros return true/false depending on current product/core family.
#define GFX_IS_PRODUCT(p, r)           ( (p).eProductFamily == r )
#define GFX_IS_DISPLAYCORE(p, d)       ( (p).eDisplayCoreFamily == d )
#define GFX_IS_RENDERCORE(p, r)        ( (p).eRenderCoreFamily == r )
// These macros return the current product/core family enum.
// Relational compares (</>) should not be done when using GFX_GET_CURRENT_PRODUCT
// macro.  There are no relationships between the PRODUCT_FAMILY enum values.
#define GFX_GET_CURRENT_PRODUCT(p)     ( (p).eProductFamily )
#define GFX_GET_CURRENT_DISPLAYCORE(p) ( (p).eDisplayCoreFamily )
#define GFX_GET_CURRENT_RENDERCORE(p)  ( (p).eRenderCoreFamily )
#define GFX_IS_DISCRETE_FAMILY(p)      ( ( GFX_GET_CURRENT_PRODUCT(p) == IGFX_DG1 )             ||   \
                                         ( GFX_GET_CURRENT_PRODUCT(p) == IGFX_DG2 )             ||   \
                                         ( GFX_GET_CURRENT_PRODUCT(p) == IGFX_XE_HP_SDV ) )
// These macros return true/false depending on the current render family.
#define GFX_IS_NAPA_RENDER_FAMILY(p)   ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN3_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN3_5_CORE ) )

#define GFX_IS_GEN_RENDER_FAMILY(p)    ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN4_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN4_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN5_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN5_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN5_75_CORE ) ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN6_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN8_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_5_OR_LATER(p)       ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN5_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN5_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN5_75_CORE ) ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN6_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN8_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_5_75_OR_LATER(p)    ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN5_75_CORE ) ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN6_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN8_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_6_OR_LATER(p)       ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN6_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN8_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_7_OR_LATER(p)       ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_5_CORE )  ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN8_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_7_5_OR_LATER(p)     ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN7_5_CORE )  ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN8_CORE )    ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_8_OR_LATER(p)       ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN8_CORE )    ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_8_CHV_OR_LATER(p)   ( ( GFX_GET_CURRENT_PRODUCT(p) == IGFX_CHERRYVIEW )      ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_9_OR_LATER(p)       ( ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN9_CORE )    ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )   ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_10_OR_LATER(p)       (( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN10_CORE )  ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   || \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )  ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_11_OR_LATER(p)       (( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN11_CORE )   || \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GEN12_CORE )  ||  \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_XE_HP_CORE )   ||   \
                                         ( GFX_GET_CURRENT_RENDERCORE(p) == IGFX_GENNEXT_CORE ) )

#define GFX_IS_GEN_12_OR_LATER(p)       (( GFX_GET_CURRENT_RENDERCORE(p) >= IGFX_GEN12_CORE ))

#define GFX_IS_ATOM_PRODUCT_FAMILY(p)  ( GFX_IS_PRODUCT(p, IGFX_VALLEYVIEW)   ||  \
                                         GFX_IS_PRODUCT(p, IGFX_CHERRYVIEW)   ||  \
                                         GFX_IS_PRODUCT(p, IGFX_BROXTON) )

///////////////////////////////////////////////////////////////////
//
// macros for comparing Graphics family and products
//
///////////////////////////////////////////////////////////////////
#define GFX_IS_FAMILY_EQUAL_OR_ABOVE(family1, family2) ((family1)>=(family2) ? TRUE : FALSE)
#define GFX_IS_FAMILY_EQUAL_OR_BELOW(family1, family2) ((family1)<=(family2) ? TRUE : FALSE)
#define GFX_IS_FAMILY_BELOW(family1, family2) ((family1)<(family2) ? TRUE : FALSE)
#define GFX_IS_PRODUCT_EQUAL_OR_ABOVE(product1, product2) ((product1)>=(product2) ? TRUE : FALSE)
#define GFX_IS_PRODUCT_EQUAL_OR_BELOW(product1, product2) ((product1)<=(product2) ? TRUE : FALSE)
#define GFX_IS_PRODUCT_BELOW(product1, product2)  ((product1) <(product2) ? TRUE : FALSE)

//Feature ID: Graphics PRD PC11.0 - Brookdale-G Support
//Description: Move device and vendor ID's to igfxfmid.h.
//  Add #include "igfxfmid.h".
//Other Files Modified: dispconf.c, kcconfig.c, kchmisc.c, kchsys.c,
//  driver.h, igfxfmid.h, imdefs.h, kchialm.h, kchname.h, softbios.h,
//  swbios.h, vddcomm.h, vidmini.h

#define INTEL_VENDOR_ID              0x8086   // Intel Corporation

//Device IDs
#define UNKNOWN_DEVICE_ID            0xFFFF   // Unknown device

//CHV device ids
#define ICHV_MOBL_DEVICE_F0_ID           0x22B0   // CHV TABLET i.e CHT
#define ICHV_PLUS_MOBL_DEVICE_F0_ID      0x22B1   // Essential i.e Braswell
#define ICHV_DESK_DEVICE_F0_ID           0x22B2   // Reserved
#define ICHV_PLUS_DESK_DEVICE_F0_ID      0x22B3   // Reserved

//BDW device ids

#define IBDW_GT1_HALO_MOBL_DEVICE_F0_ID         0x1602
#define IBDW_GT1_ULT_MOBL_DEVICE_F0_ID          0x1606
#define IBDW_GT1_RSVD_DEVICE_F0_ID              0x160B
#define IBDW_GT1_SERV_DEVICE_F0_ID              0x160A
#define IBDW_GT1_WRK_DEVICE_F0_ID               0x160D
#define IBDW_GT1_ULX_DEVICE_F0_ID               0x160E
#define IBDW_GT2_HALO_MOBL_DEVICE_F0_ID         0x1612
#define IBDW_GT2_ULT_MOBL_DEVICE_F0_ID          0x1616
#define IBDW_GT2_RSVD_DEVICE_F0_ID              0x161B
#define IBDW_GT2_SERV_DEVICE_F0_ID              0x161A
#define IBDW_GT2_WRK_DEVICE_F0_ID               0x161D
#define IBDW_GT2_ULX_DEVICE_F0_ID               0x161E
#define IBDW_GT3_HALO_MOBL_DEVICE_F0_ID         0x1622
#define IBDW_GT3_ULT_MOBL_DEVICE_F0_ID          0x1626
#define IBDW_GT3_ULT25W_MOBL_DEVICE_F0_ID       0x162B
#define IBDW_GT3_SERV_DEVICE_F0_ID              0x162A
#define IBDW_GT3_WRK_DEVICE_F0_ID               0x162D
#define IBDW_GT3_ULX_DEVICE_F0_ID               0x162E
#define IBDW_RSVD_MRKT_DEVICE_F0_ID             0x1632
#define IBDW_RSVD_ULT_MOBL_DEVICE_F0_ID         0x1636
#define IBDW_RSVD_HALO_MOBL_DEVICE_F0_ID        0x163B
#define IBDW_RSVD_SERV_DEVICE_F0_ID             0x163A
#define IBDW_RSVD_WRK_DEVICE_F0_ID              0x163D
#define IBDW_RSVD_ULX_DEVICE_F0_ID              0x163E

//skl placeholder

#define ISKL_GT4_DT_DEVICE_F0_ID                0x1932
#define ISKL_GT2_DT_DEVICE_F0_ID                0x1912 // Used on actual Silicon

#define ISKL_GT1_DT_DEVICE_F0_ID                0x1902


#define ISKL_GT2_ULT_DEVICE_F0_ID               0x1916
#define ISKL_GT2F_ULT_DEVICE_F0_ID              0x1921
#define ISKL_GT3e_ULT_DEVICE_F0_ID_540          0x1926
#define ISKL_GT3e_ULT_DEVICE_F0_ID_550          0x1927

#define ISKL_GT2_ULX_DEVICE_F0_ID               0x191E
#define ISKL_GT1_ULT_DEVICE_F0_ID               0x1906
#define ISKL_GT3_MEDIA_SERV_DEVICE_F0_ID        0x192D
#define ISKL_GT1_5_ULT_DEVICE_F0_ID             0x1913

#define ISKL_GT3_ULT_DEVICE_F0_ID               0x1923

#define ISKL_GT2_HALO_MOBL_DEVICE_F0_ID         0x191B

#define ISKL_GT4_HALO_MOBL_DEVICE_F0_ID         0x193B
#define ISKL_GT4_SERV_DEVICE_F0_ID              0x193A
#define ISKL_GT2_WRK_DEVICE_F0_ID               0x191D
#define ISKL_GT4_WRK_DEVICE_F0_ID               0x193D


#define ISKL_GT0_DESK_DEVICE_F0_ID              0x0900
#define ISKL_GT1_DESK_DEVICE_F0_ID              0x0901
#define ISKL_GT2_DESK_DEVICE_F0_ID              0x0902
#define ISKL_GT3_DESK_DEVICE_F0_ID              0x0903
#define ISKL_GT4_DESK_DEVICE_F0_ID              0x0904
#define ISKL_GT1_ULX_DEVICE_F0_ID               0x190E
//SKL strings to be be deleted in future

#define ISKL_GT1_HALO_MOBL_DEVICE_F0_ID         0x190B
#define ISKL_GT1_SERV_DEVICE_F0_ID              0x190A
#define ISKL_GT1_5_ULX_DEVICE_F0_ID             0x1915
#define ISKL_GT1_5_DT_DEVICE_F0_ID              0x1917
#define ISKL_GT2_SERV_DEVICE_F0_ID              0x191A
#define ISKL_LP_DEVICE_F0_ID                    0x9905
#define ISKL_GT3_HALO_MOBL_DEVICE_F0_ID         0x192B
#define ISKL_GT3_SERV_DEVICE_F0_ID              0x192A
#define ISKL_GT0_MOBL_DEVICE_F0_ID              0xFFFF

// KabyLake Device ids
#define IKBL_GT1_ULT_DEVICE_F0_ID               0x5906
#define IKBL_GT1_5_ULT_DEVICE_F0_ID             0x5913
#define IKBL_GT2_ULT_DEVICE_F0_ID               0x5916
#define IKBL_GT2F_ULT_DEVICE_F0_ID              0x5921
#define IKBL_GT3_15W_ULT_DEVICE_F0_ID           0x5926
//#define IKBL_GT3E_ULT_DEVICE_F0_ID              0x5926
#define IKBL_GT1_ULX_DEVICE_F0_ID               0x590E
#define IKBL_GT1_5_ULX_DEVICE_F0_ID             0x5915
#define IKBL_GT2_ULX_DEVICE_F0_ID               0x591E
#define IKBL_GT1_DT_DEVICE_F0_ID                0x5902
#define IKBL_GT2_R_ULT_DEVICE_F0_ID             0x5917
#define IKBL_GT2_DT_DEVICE_F0_ID                0x5912
#define IKBL_GT1_HALO_DEVICE_F0_ID              0x590B
#define IKBL_GT1F_HALO_DEVICE_F0_ID             0x5908
#define IKBL_GT2_HALO_DEVICE_F0_ID              0x591B
#define IKBL_GT4_HALO_DEVICE_F0_ID              0x593B
#define IKBL_GT1_SERV_DEVICE_F0_ID              0x590A
#define IKBL_GT2_SERV_DEVICE_F0_ID              0x591A
#define IKBL_GT2_WRK_DEVICE_F0_ID               0x591D
#define IKBL_GT3_ULT_DEVICE_F0_ID               0x5923
#define IKBL_GT3_28W_ULT_DEVICE_F0_ID           0x5927
//keeping the below ids as its been used in linux . need to be removed once removed from linux files.
#define IKBL_GT4_DT_DEVICE_F0_ID                0x5932
#define IKBL_GT3_HALO_DEVICE_F0_ID              0x592B
#define IKBL_GT3_SERV_DEVICE_F0_ID              0x592A
#define IKBL_GT4_SERV_DEVICE_F0_ID              0x593A
#define IKBL_GT4_WRK_DEVICE_F0_ID               0x593D

//GLK Device ids
#define IGLK_GT2_ULT_18EU_DEVICE_F0_ID          0x3184
#define IGLK_GT2_ULT_12EU_DEVICE_F0_ID          0x3185

//BXT BIOS programmed Silicon ids.
#define IBXT_GT_3x6_DEVICE_ID                0x0A84
#define IBXT_PRO_3x6_DEVICE_ID               0x1A84 //18EU
#define IBXT_PRO_12EU_3x6_DEVICE_ID          0x1A85 //12 EU
#define IBXT_P_3x6_DEVICE_ID                 0x5A84 //18EU APL
#define IBXT_P_12EU_3x6_DEVICE_ID            0x5A85 //12EU APL

// CNL Placeholder
// These device ID defs to be removed later on after UMD switches to GT_SYSTEM_INFO interface.
#define ICNL_GT0_DESK_DEVICE_F0_ID              0XDEAD      // Not Valid - To be cleaned up.
#define ICNL_GT1_DESK_DEVICE_F0_ID              0x0A01
#define ICNL_GT2_DESK_DEVICE_F0_ID              0x0A02
#define ICNL_GT2_5_DESK_DEVICE_F0_ID            0x0A00      // Not POR - To be cleaned up.
#define ICNL_GT3_DESK_DEVICE_F0_ID              0x0A05
#define ICNL_GT4_DESK_DEVICE_F0_ID              0x0A07

// CNL Si device ids
#define ICNL_5x8_ULX_DEVICE_F0_ID               0x5A51      //GT2
#define ICNL_5x8_ULT_DEVICE_F0_ID               0x5A52      //GT2
#define ICNL_4x8_ULT_DEVICE_F0_ID               0x5A5A      //GT1.5
#define ICNL_3x8_ULT_DEVICE_F0_ID               0x5A42      //GT1
#define ICNL_2x8_ULT_DEVICE_F0_ID               0x5A4A      //GT0.5
#define ICNL_9x8_ULT_DEVICE_F0_ID               0x5A62
#define ICNL_9x8_SUPERSKU_DEVICE_F0_ID          0x5A60
#define ICNL_5x8_SUPERSKU_DEVICE_F0_ID          0x5A50      //GT2
#define ICNL_1x6_5x8_SUPERSKU_DEVICE_F0_ID      0x5A40      //GTx
#define ICNL_5x8_HALO_DEVICE_F0_ID              0x5A54      //GT2
#define ICNL_3x8_HALO_DEVICE_F0_ID              0x5A44      //GT1
#define ICNL_5x8_DESKTOP_DEVICE_F0_ID           0x5A55
#define ICNL_3x8_DESKTOP_DEVICE_F0_ID           0x5A45
#define ICNL_4x8_ULX_DEVICE_F0_ID               0x5A59      //GT1.5
#define ICNL_3x8_ULX_DEVICE_F0_ID               0x5A41      //GT1
#define ICNL_2x8_ULX_DEVICE_F0_ID               0x5A49      //GT0.5
#define ICNL_4x8_HALO_DEVICE_F0_ID              0x5A5C      //GT1.5

#define ICFL_GT1_S61_DT_DEVICE_F0_ID            0x3E90
#define ICFL_GT1_S41_DT_DEVICE_F0_ID            0x3E93
#define ICFL_GT2_S62_DT_DEVICE_F0_ID            0x3E92
#define ICFL_GT2_HALO_DEVICE_F0_ID              0x3E9B
#define ICFL_GT2_SERV_DEVICE_F0_ID              0x3E96
#define ICFL_GT2_HALO_WS_DEVICE_F0_ID           0x3E94
#define ICFL_GT2_S42_DT_DEVICE_F0_ID            0x3E91
#define ICFL_GT3_ULT_15W_DEVICE_F0_ID           0x3EA6
#define ICFL_GT3_ULT_15W_42EU_DEVICE_F0_ID      0x3EA7
#define ICFL_GT3_ULT_28W_DEVICE_F0_ID           0x3EA8
#define ICFL_GT3_ULT_DEVICE_F0_ID               0x3EA5
#define ICFL_HALO_DEVICE_F0_ID                  0x3E95
#define ICFL_GT2_WKS_DEVICE_P0_ID               0x9BC6

//GLV
#define IGLV_GT1_MOB_SIM_DEVICE_F0_ID           0xFF10
#define IGLV_GT1_MOB_DEVICE_F0_ID               0x3E04

//GEN11LP
#define IICL_LP_GT1_MOB_DEVICE_F0_ID            0xFF05
#define IICL_LP_1x8x8_SUPERSKU_DEVICE_F0_ID     0x8A50
#define IICL_LP_1x8x8_ULX_DEVICE_F0_ID          0x8A51
#define IICL_LP_1x6x8_ULX_DEVICE_F0_ID          0x8A5C
#define IICL_LP_1x4x8_ULX_DEVICE_F0_ID          0x8A5D
#define IICL_LP_1x8x8_ULT_DEVICE_F0_ID          0x8A52
#define IICL_LP_1x6x8_ULT_DEVICE_F0_ID          0x8A5A
#define IICL_LP_1x4x8_ULT_DEVICE_F0_ID          0x8A5B
#define IICL_LP_0x0x0_ULT_DEVICE_A0_ID          0x8A70
#define IICL_LP_1x1x8_ULT_DEVICE_A0_ID          0x8A71
#define IICL_LP_1x4x8_LOW_MEDIA_ULT_DEVICE_F0_ID 0x8A56

//TGL LP
#define IGEN12LP_GT1_MOB_DEVICE_F0_ID           0xFF20
#define ITGL_LP_1x6x16_UNKNOWN_SKU_F0_ID_5      0x9A49
#define ITGL_LP_1x6x16_ULT_15W_DEVICE_F0_ID     0x9A49
#define ITGL_LP_1x6x16_ULX_5_2W_DEVICE_F0_ID    0x9A40
#define ITGL_LP_1x6x16_ULT_12W_DEVICE_F0_ID     0x9A59
#define ITGL_LP_1x2x16_HALO_45W_DEVICE_F0_ID    0x9A60
#define ITGL_LP_1x2x16_DESK_65W_DEVICE_F0_ID    0x9A68
#define ITGL_LP_1x2x16_HALO_WS_45W_DEVICE_F0_ID 0x9A70
#define ITGL_LP_1x2x16_DESK_WS_65W_DEVICE_F0_ID 0x9A78
#define ITGL_LP_GT0_ULT_DEVICE_F0_ID            0x9A7F

#define DEV_ID_4905                             0x4905
#define IRKL_1x2x16_GT1_ULT_SKU_DEVICE_F0_ID_0     0x4C80 // Obsolete, to be removed once removed from linux code base
#define IRKL_1x2x16_GT1_SUPER_SKU_DEVICE_F0_ID_1   0x4C8A // Obsolete, to be removed once removed from linux code base
#define IRKL_1X2X12_GT1_SUPER_SKU_DEVICE_F0_ID_2   0x4C8B // Obsolete, to be removed once removed from linux code base
#define IRKL_1X1X16_GT0P5_SUPER_SKU_DEVICE_F0_ID_3 0x4C8C // Obsolete, to be removed once removed from linux code base
#define IRKL_1X2X16_GT1_SUPER_SKU_DEVICE_F0_ID_4   0x4C90 // Obsolete, to be removed once removed from linux code base
#define IRKL_1X2X16_GT1_SUPER_SKU_DEVICE_F0_ID_5   0x4C9A // Obsolete, to be removed once removed from linux code base
#define IRKL_GT0_SKU_DEVICE_F0_ID                  0x4C9F // Obsolete, to be removed once removed from linux code base

#define DEV_ID_4C80         0x4C80
#define DEV_ID_4C8A         0x4C8A
#define DEV_ID_4C8B         0x4C8B
#define DEV_ID_4C8C         0x4C8C
#define DEV_ID_4C90         0x4C90
#define DEV_ID_4C9A         0x4C9A
#define DEV_ID_4C9F         0x4C9F
//LKF
#define ILKF_1x8x8_DESK_DEVICE_F0_ID            0x9840
#define ILKF_GT0_DESK_DEVICE_A0_ID              0x9850

//EHL
#define IEHL_1x4x8_SUPERSKU_DEVICE_A0_ID        0x4500
#define IEHL_1x2x4_DEVICE_A0_ID                 0x4541
#define IEHL_1x4x4_DEVICE_A0_ID                 0x4551
#define IEHL_1x4x8_DEVICE_A0_ID                 0x4571
#define IEHL_VAL_0x0x0_DEVICE_A0_ID             0x4569

//JSL
#define IJSL_1x4x8_DEVICE_A0_ID                 0x4500

// ADL-S
#define IADLS_1X2X16_GT1_UNKNOWN_SKU_ID_0                       0x4680
#define IADLS_1X2X16_GT1_UNKNOWN_SKU_ID_1                       0x4681
#define IADLS_1X2X12_GT1_UNKNOWN_SKU_ID_2                       0X4682
#define IADLS_1X1X16_GT0P5_UNKNOWN_SKU_ID_3                     0x4683
#define IADLS_1X2X16_GT1_UNKNOWN_SKU_ID_4                       0x4690
#define IADLS_1X2X16_GT1_UNKNOWN_SKU_ID_5                       0x4691
#define IADLS_1X2X12_GT1_UNKNOWN_SKU_ID_6                       0x4692
#define IADLS_1X1X16_GT1_UNKNOWN_SKU_ID_7                       0x4693
#define IADLS_1X2X16_GT1_UNKNOWN_SKU_ID_8                       0x4698
#define IADLS_1X2X16_GT1_UNKNOWN_SKU_ID_9                       0x4699


#define IADLS_GT0_DEVICE_A0_ID                                  0x469F
#define DEV_ID_4600                                             0x4600
#define DEV_ID_461F                                             0x461F

// ADL-P
#define DEV_ID_46A0                             0x46A0
#define DEV_ID_46A1                             0x46A1
#define DEV_ID_46A2                             0x46A2
#define DEV_ID_46A3                             0x46A3
#define DEV_ID_46A6                             0x46A6
#define DEV_ID_46A8                             0x46A8
#define DEV_ID_46AA                             0x46AA
#define DEV_ID_4626                             0x4626
#define DEV_ID_4628                             0x4628
#define DEV_ID_462A                             0x462A
#define DEV_ID_46B0                             0x46B0
#define DEV_ID_46B1                             0x46B1
#define DEV_ID_46B2                             0x46B2
#define DEV_ID_46B3                             0x46B3
#define DEV_ID_46C0                             0x46C0
#define DEV_ID_46C1                             0x46C1
#define DEV_ID_46C2                             0x46C2
#define DEV_ID_46C3                             0x46C3

// DG2
#define DEV_ID_4F80                             0x4F80
#define DEV_ID_4F81                             0x4F81
#define DEV_ID_4F82                             0x4F82
#define DEV_ID_4F83                             0x4F83
#define DEV_ID_4F84                             0x4F84
#define DEV_ID_4F85                             0x4F85
#define DEV_ID_4F86                             0x4F86
#define DEV_ID_4F87                             0x4F87
#define DEV_ID_4F88                             0x4F88
#define DEV_ID_5690                             0x5690
#define DEV_ID_5691                             0x5691
#define DEV_ID_5692                             0x5692
#define DEV_ID_5693                             0x5693
#define DEV_ID_5694                             0x5694
#define DEV_ID_5695                             0x5695
#define DEV_ID_5696                             0x5696
#define DEV_ID_5697                             0x5697
#define DEV_ID_5698                             0x5698
#define DEV_ID_56A0                             0x56A0
#define DEV_ID_56A1                             0x56A1
#define DEV_ID_56A2                             0x56A2
#define DEV_ID_56A3                             0x56A3
#define DEV_ID_56A4                             0x56A4
#define DEV_ID_56A5                             0x56A5
#define DEV_ID_56A6                             0x56A6
#define DEV_ID_56A7                             0x56A7
#define DEV_ID_56A8                             0x56A8
#define DEV_ID_56A9                             0x56A9
#define DEV_ID_56B0                             0x56B0
#define DEV_ID_56B1                             0x56B1
#define DEV_ID_56B2                             0x56B2
#define DEV_ID_56B3                             0x56B3
#define DEV_ID_56C0                             0x56C0
#define DEV_ID_56C1                             0x56C1
#define DEV_ID_56C2                             0x56C2
#define DEV_ID_56CF                             0x56CF
#define DEV_ID_56BA                             0x56BA
#define DEV_ID_56BC                             0x56BC
#define DEV_ID_56BE                             0x56BE

// ARL
#define DEV_ID_7D67                             0x7D67

// PVC
#define DEV_ID_0BD0                            0x0BD0
#define DEV_ID_0BD4                            0x0BD4
#define DEV_ID_0BD5                            0x0BD5
#define DEV_ID_0BD6                            0x0BD6
#define DEV_ID_0BD7                            0x0BD7
#define DEV_ID_0BD8                            0x0BD8
#define DEV_ID_0BD9                            0x0BD9
#define DEV_ID_0BDA                            0x0BDA
#define DEV_ID_0BDB                            0x0BDB
#define DEV_ID_0B69                            0x0B69
#define DEV_ID_0B6E                            0x0B6E

// BMG
#define DEV_ID_E202                             0xE202
#define DEV_ID_E20B                             0xE20B
#define DEV_ID_E20C                             0xE20C
#define DEV_ID_E20D                             0xE20D
#define DEV_ID_E210                             0xE210
#define DEV_ID_E212                             0xE212
#define DEV_ID_E215                             0xE215
#define DEV_ID_E216                             0xE216
#define DEV_ID_E220                             0xE220
#define DEV_ID_E221                             0xE221
#define DEV_ID_E222                             0xE222
#define DEV_ID_E223                             0xE223

// PTL
#define DEV_ID_B080                             0xB080
#define DEV_ID_B081                             0xB081
#define DEV_ID_B082                             0xB082
#define DEV_ID_B083                             0xB083
#define DEV_ID_B08F                             0xB08F
#define DEV_ID_B090                             0xB090
#define DEV_ID_B0A0                             0xB0A0
#define DEV_ID_B0B0                             0xB0B0

// WCL
#define DEV_ID_FD80                             0xFD80
#define DEV_ID_FD81                             0xFD81

//CRI
#define DEV_ID_674C                             0x674C

// NVL
#define DEV_ID_D740                             0xD740
#define DEV_ID_D741                             0xD741
#define DEV_ID_D742                             0xD742
#define DEV_ID_D743                             0xD743
#define DEV_ID_D744                             0xD744

#define GFX_IS_DG2_G11_CONFIG(d) ( ( d == DEV_ID_56A5 )             ||   \
                                 ( d == DEV_ID_56A6 )             ||   \
                                 ( d == DEV_ID_5693 )             ||   \
                                 ( d == DEV_ID_5694 )             ||   \
                                 ( d == DEV_ID_5695 )             ||   \
                                 ( d == DEV_ID_56B0 )             ||   \
                                 ( d == DEV_ID_56B1 )             ||   \
                                 ( d == DEV_ID_56C1 )             ||   \
                                 ( d == DEV_ID_4F87 )             ||   \
                                 ( d == DEV_ID_4F88 ))

#define GFX_IS_DG2_G10_CONFIG(d) (    ( d == DEV_ID_56A0 )                              ||   \
                                      ( d == DEV_ID_56A1 )                              ||   \
                                      ( d == DEV_ID_56A2 )                              ||   \
                                      ( d == DEV_ID_5690 )                              ||   \
                                      ( d == DEV_ID_5691 )                              ||   \
                                      ( d == DEV_ID_5692 )                              ||   \
                                      ( d == DEV_ID_56C0 )                              ||   \
                                      ( d == DEV_ID_56C2 )                              ||   \
                                      ( d == DEV_ID_4F80 )                              ||   \
                                      ( d == DEV_ID_4F81 )                              ||   \
                                      ( d == DEV_ID_4F82 )                              ||   \
                                      ( d == DEV_ID_4F83 )                              ||   \
                                      ( d == DEV_ID_4F84 ))

#define GFX_IS_DG2_G12_CONFIG(d)   ( ( d == DEV_ID_4F85 )                              ||   \
                                      ( d == DEV_ID_4F86 )                              ||   \
                                      ( d == DEV_ID_56A3 )                              ||   \
                                      ( d == DEV_ID_56A4 )                              ||   \
                                      ( d == DEV_ID_5696 )                              ||   \
                                      ( d == DEV_ID_5697 )                              ||   \
                                      ( d == DEV_ID_56B2 )                              ||   \
                                      ( d == DEV_ID_56B3 ))

#define GFX_IS_ARL_S(d)  ( ( d == DEV_ID_7D67 ) )

#define GFX_IS_XT_CONFIG(d) ((d == DEV_ID_0BD5) || \
                             (d == DEV_ID_0BD6) || \
                             (d == DEV_ID_0BD7) || \
                             (d == DEV_ID_0BD8) || \
                             (d == DEV_ID_0BD9) || \
                             (d == DEV_ID_0BDA) || \
                             (d == DEV_ID_0BDB) || \
                             (d == DEV_ID_0B69) || \
                             (d == DEV_ID_0B6E))
#define GFX_IS_VG_CONFIG(d) ((d == DEV_ID_0BD4))

#endif
