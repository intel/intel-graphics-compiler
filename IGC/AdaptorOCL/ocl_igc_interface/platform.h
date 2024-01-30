/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

// Interface : PLATFORM
//             Platform
// Interface for defining target platform

namespace IGC {

using TypeErasedEnum = uint64_t;

CIF_DECLARE_INTERFACE(Platform, "PLATFORM")

CIF_DEFINE_INTERFACE_VER(Platform, 1){
  CIF_INHERIT_CONSTRUCTOR();

  OCL_API_CALL virtual TypeErasedEnum GetProductFamily() const;
  OCL_API_CALL virtual void SetProductFamily(TypeErasedEnum v);
  OCL_API_CALL virtual TypeErasedEnum GetPCHProductFamily() const;
  OCL_API_CALL virtual void SetPCHProductFamily(TypeErasedEnum v);
  OCL_API_CALL virtual TypeErasedEnum GetDisplayCoreFamily() const;
  OCL_API_CALL virtual void SetDisplayCoreFamily(TypeErasedEnum v);
  OCL_API_CALL virtual TypeErasedEnum GetRenderCoreFamily() const;
  OCL_API_CALL virtual void SetRenderCoreFamily(TypeErasedEnum v);
  OCL_API_CALL virtual TypeErasedEnum GetPlatformType() const;
  OCL_API_CALL virtual void SetPlatformType(TypeErasedEnum v);

  OCL_API_CALL virtual unsigned short GetDeviceID() const;
  OCL_API_CALL virtual void SetDeviceID(unsigned short v);
  OCL_API_CALL virtual unsigned short GetRevId() const;
  OCL_API_CALL virtual void SetRevId(unsigned short v);
  OCL_API_CALL virtual unsigned short GetDeviceID_PCH() const;
  OCL_API_CALL virtual void SetDeviceID_PCH(unsigned short v);
  OCL_API_CALL virtual unsigned short GetRevId_PCH() const;
  OCL_API_CALL virtual void SetRevId_PCH(unsigned short v);

  OCL_API_CALL virtual TypeErasedEnum GetGTType() const;
  OCL_API_CALL virtual void SetGTType(TypeErasedEnum v);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(Platform, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  OCL_API_CALL virtual void SetRenderBlockID(unsigned int v);
  OCL_API_CALL virtual unsigned int GetRenderBlockID() const;
  OCL_API_CALL virtual void SetDisplayBlockID(unsigned int v);
  OCL_API_CALL virtual unsigned int GetDisplayBlockID() const;
  OCL_API_CALL virtual void SetMediaBlockID(unsigned int v);
  OCL_API_CALL virtual unsigned int GetMediaBlockID() const;
};

CIF_GENERATE_VERSIONS_LIST(Platform);
CIF_MARK_LATEST_VERSION(PlatformLatest, Platform);
using PlatformTagOCL = Platform<2>; // Note : can tag with different version for
                                    //        transition periods
}

#include "cif/macros/disable.h"
