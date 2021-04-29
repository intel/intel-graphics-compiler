/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cif/builtins/builtins_registry.h"

#include "cif/export/cif_main_impl.h"

#include "cif/builtins/memory/buffer/impl/buffer_impl.h"

namespace CIF {

namespace Builtins {

// List of all supported builtins
using AllBuiltinsListT =  InterfacesList<CIF::Builtins::Buffer>;

bool IsBuiltin(InterfaceId_t intId){
    return AllBuiltinsListT::ContainsInterface(intId);
}

ICIF *Create(InterfaceId_t entryPointInterface, Version_t version, ICIF *parentInterface){
    return AllBuiltinsListT::template forwardToOne<Helpers::ForwardCreateInterfaceImpl, ICIF *, ICIF *>(entryPointInterface, nullptr, version, version, parentInterface);
}

bool GetSupportedVersions(InterfaceId_t entryPointInterface, Version_t &verMin, Version_t &verMax){
    return AllBuiltinsListT::template forwardToOne<Helpers::ForwardGetSupportedVersions, bool>(entryPointInterface, false, verMin, verMax);
}

InterfaceId_t FindIncompatible(InterfaceId_t entryPointInterface, CIF::CompatibilityDataHandle handle){
    return AllBuiltinsListT::template forwardToOne<Helpers::ForwardGetFirstIncompatible, InterfaceId_t>(entryPointInterface, entryPointInterface, handle);
}

}

}
