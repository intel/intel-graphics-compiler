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
