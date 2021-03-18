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

#define USC_PARAM( ... )
#define USC_PARAM_DESC( ... )
#define USC_PARAM_IMPLICIT_VALUE( ... )
#define USC_PARAM_ADAPTER( adapter )
#define USC_PARAM_HIDE()
#define USC_PARAM_CALLBACK( ... )

#define USC_PARAM_NAME_ADAPTER "adapter"
#define USC_PARAM_NAME_PLATFORM "platform"
#define USC_PARAM_NAME_OPTIMIZERLEVEL "optimizerLevel"
#define USC_PARAM_NAME_SIMDLEVEL "simdLevel"
#define USC_PARAM_NAME_BTILAYOUT "BTILayout"
#define USC_PARAM_NAME_STEPPING "step"
#define USC_PARAM_NAME_GTSYSTEMINFO "GTSystemInfo"
#define USC_PARAM_NAME_ADAPTERINFO "AdapterInfo"

#define USC_PARAM_NAME_ADAPTER_D3D9 "d3d9"
#define USC_PARAM_NAME_ADAPTER_D3D10 "d3d10"
#define USC_PARAM_NAME_ADAPTER_ILTEXT "iltext"
#define USC_PARAM_NAME_ADAPTER_OGL "ogl"
#define USC_PARAM_NAME_ADAPTER_OCL "ocl"

// D3D9 adapter
#define USC_PARAM_NAME_D3D9_DXVERSION "DXinterfaceVersion"
#define USC_PARAM_NAME_D3D9_VSNOS "VSnos"
#define USC_PARAM_NAME_D3D9_PSNOS "PSnos"

// ILTEXT adapter
#define USC_PARAM_NAME_ILTEXT_CLIENTTYPE "clientType"

// OGL adapter
#define USC_PARAM_NAME_OGL_VSINPUTSTATE "VSinputState"
#define USC_PARAM_NAME_OGL_PSINPUTSTATE "PSinputState"
#define USC_PARAM_NAME_OGL_GSINPUTSTATE "GSinputState"
#define USC_PARAM_NAME_OGL_HSINPUTSTATE "HSinputState"
#define USC_PARAM_NAME_OGL_DSINPUTSTATE "DSinputState"
#define USC_PARAM_NAME_OGL_CSINPUTSTATE "CSinputState"

// OCL adapter
#define USC_PARAM_NAME_OCL_DUMPBINARY "dumpbinary"
