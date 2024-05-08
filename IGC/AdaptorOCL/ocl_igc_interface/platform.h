/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

// Interface : PLATFORM
//             Platform
// Interface for defining target platform

namespace IGC {

using TypeErasedEnum = uint64_t;

CIF_DECLARE_INTERFACE(Platform, "PLATFORM")

CIF_DEFINE_INTERFACE_VER(Platform, 1){
  CIF_INHERIT_CONSTRUCTOR();

  virtual TypeErasedEnum GetProductFamily() const;
  virtual void SetProductFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetPCHProductFamily() const;
  virtual void SetPCHProductFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetDisplayCoreFamily() const;
  virtual void SetDisplayCoreFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetRenderCoreFamily() const;
  virtual void SetRenderCoreFamily(TypeErasedEnum v);
  virtual TypeErasedEnum GetPlatformType() const;
  virtual void SetPlatformType(TypeErasedEnum v);

  virtual unsigned short GetDeviceID() const;
  virtual void SetDeviceID(unsigned short v);
  virtual unsigned short GetRevId() const;
  virtual void SetRevId(unsigned short v);
  virtual unsigned short GetDeviceID_PCH() const;
  virtual void SetDeviceID_PCH(unsigned short v);
  virtual unsigned short GetRevId_PCH() const;
  virtual void SetRevId_PCH(unsigned short v);

  virtual TypeErasedEnum GetGTType() const;
  virtual void SetGTType(TypeErasedEnum v);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(Platform, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  virtual void SetRenderBlockID(unsigned int v);
  virtual unsigned int GetRenderBlockID() const;
  virtual void SetDisplayBlockID(unsigned int v);
  virtual unsigned int GetDisplayBlockID() const;
  virtual void SetMediaBlockID(unsigned int v);
  virtual unsigned int GetMediaBlockID() const;
};

CIF_GENERATE_VERSIONS_LIST(Platform);
CIF_MARK_LATEST_VERSION(PlatformLatest, Platform);
using PlatformTagOCL = Platform<2>; // Note : can tag with different version for
                                    //        transition periods
}

#include "cif/macros/disable.h"
