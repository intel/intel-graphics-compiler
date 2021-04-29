/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/cif.h"
#include "cif/common/compatibility.h"
#include "cif/common/id.h"

namespace CIF {

namespace Builtins {

bool IsBuiltin(InterfaceId_t intId);

ICIF *Create(InterfaceId_t entryPointInterface, Version_t version, ICIF *parentInterface);

bool GetSupportedVersions(InterfaceId_t entryPointInterface, Version_t &verMin, Version_t &verMax);

InterfaceId_t FindIncompatible(InterfaceId_t entryPointInterface, CIF::CompatibilityDataHandle handle);

}

}
