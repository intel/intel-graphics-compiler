/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <stddef.h>
#include <stdint.h>
#if !defined(__clang__) || (__clang_major__ >= 10)
#include <type_traits>
#endif


// This code defines a set of macros, that allows us to intentionally disable certain warnings
// in a way that is both concise and portable across compilers.
// We need to disable the warnings in such way because this C++ file may be compiled with Clang, GCC, and Microsoft Visual Studio Compiler
// This works around (at least) two known problems:
//  - QuickBuild treats compiler warnings as errors
//  - RTStackReflection.exe generates warnings when compiling this file
//
// The only macros that are used by the main code are:
//     DISABLE_WARNING_PUSH
//     DISABLE_WARNING_POP
//     and the macros for the individual warnings
// The others are lower level internal utilities, used only by higher level macros.
//
// Explanation of code in original resource : https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/
#if defined(__clang__)
    #define DO_PRAGMA(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA(GCC diagnostic pop)
    #define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored #warningName)  // internal utility
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION   \
        DISABLE_WARNING(-Wgnu-anonymous-struct)  // anonymous structs are a GNU extension
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION \
        DISABLE_WARNING(-Wnested-anon-types)     // anonymous types declared in an anonymous union are an extension
#elif defined(__GNUC__)
    #define DO_PRAGMA(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA(GCC diagnostic pop)
    // Treats only this header file as a system header, AKA disables all warnings in this header file only.
    // The effect does not extend into any file that includes this header file.
    #pragma GCC system_header
    // define the macros that are used in the code down below to prevent compiler failure
    // NOTE: Apparently GCC compiler doesn't support disabling individual warnings.
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION
#elif defined(_MSC_VER)
    #define DISABLE_WARNING_PUSH           __pragma(warning( push ))
    #define DISABLE_WARNING_POP            __pragma(warning( pop ))
    #define DISABLE_WARNING(warningNumber) __pragma(warning( disable : warningNumber ))  // internal utility
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION   \
        DISABLE_WARNING(4201)  // nonstandard extension used: nameless struct/union
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT \
        DISABLE_WARNING(4820)  // 'MemTravStack::<unnamed-tag>': '4' bytes padding added after data member 'MemTravStack::<unnamed-tag>::offset'
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION
#else
    // define the macros that are used in the code down below to prevent compiler failure
    // NOTE: internal utility macros should not be defined here
    #define DISABLE_WARNING_PUSH
    #define DISABLE_WARNING_POP
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION
#endif


DISABLE_WARNING_PUSH       // save the current pragma state, save the current compiler settings
// select the warnings that we want to disable, for this file only
DISABLE_WARNING_ANONYMOUS_STRUCT_UNION
DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION


namespace IGC
{
// A class to be used, by the UMD, as a base for implementation
// of a class for populating the content of RayDispatchGlobalData
// structure.
class RayDispatchGlobalDataAdaptor
{
public:
    uint64_t GetRayStackBufferAddress() const;
    uint64_t GetCallStackHandlerPtr() const;
    uint32_t GetStackSizePerRay() const;
    uint32_t GetNumDSSRTStacks() const;
    uint32_t GetMaxBVHLevels() const;
    uint64_t GetHitGroupTable() const;
    uint64_t GetMissShaderTable() const;
    uint64_t GetCallableShaderTable() const;
    uint32_t GetHitGroupStride() const;
    uint32_t GetMissStride() const;
    uint32_t GetCallableStride() const;
    uint32_t GetDispatchWidth() const;
    uint32_t GetDispatchHeight() const;
    uint32_t GetDispatchDepth() const;
    uint32_t GetSWStackSizePerRay() const;
    uint64_t GetBindlessHeapBasePtr() const { return 0; };
    uint64_t GetPrintfBufferBasePtr() const { return 0; };
    uint64_t GetProfilingBufferGpuVa() const { return 0; };
    uint32_t GetBaseSSHOffset() const { return 0; };
};

// Layout used to pass global data to the shaders
struct RayDispatchGlobalData
{
    template<class TAPIAdaptor>
    void populate(const TAPIAdaptor& umd)
    {
        rtMemBasePtr = umd.GetRayStackBufferAddress();
        pRtMemBasePtr = rtMemBasePtr;

        callStackHandlerPtr = umd.GetCallStackHandlerPtr();

        stackSizePerRay = umd.GetStackSizePerRay();
        pStackSizePerRay = stackSizePerRay;

        numDSSRTStacks = umd.GetNumDSSRTStacks();
        pNumDSSRTStacks = numDSSRTStacks;

        maxBVHLevels = umd.GetMaxBVHLevels();

        hitGroupBasePtr = umd.GetHitGroupTable();
        pHitGroupBasePtr = hitGroupBasePtr;

        missShaderBasePtr = umd.GetMissShaderTable();
        pMissShaderBasePtr = missShaderBasePtr;

        callableShaderBasePtr = umd.GetCallableShaderTable();
        pCallableShaderBasePtr = callableShaderBasePtr;

        hitGroupStride = umd.GetHitGroupStride();
        pHitGroupStride = hitGroupStride;

        missShaderStride = umd.GetMissStride();
        pMissShaderStride = missShaderStride;

        callableShaderStride = umd.GetCallableStride();
        pCallableShaderStride = callableShaderStride;

        dispatchRaysDimensions[0] = umd.GetDispatchWidth();
        dispatchRaysDimensions[1] = umd.GetDispatchHeight();
        dispatchRaysDimensions[2] = umd.GetDispatchDepth();

        bindlessHeapBasePtr = umd.GetBindlessHeapBasePtr();
        printfBufferBasePtr = umd.GetPrintfBufferBasePtr();
        swStackSizePerRay   = umd.GetSWStackSizePerRay();

        ProfilingBufferGpuVa = umd.GetProfilingBufferGpuVa();

        baseSSHOffset = umd.GetBaseSSHOffset();
    }

    using T = uint32_t;

    enum class Bits : uint8_t
    {
        stackSizePerRay = 8,
        numDSSRTStacks = 12,
    };

    // Cached by HW (32 bytes)
    uint64_t rtMemBasePtr;          // base address of the allocated stack memory
    uint64_t callStackHandlerPtr;   // this is the KSP of the continuation handler that is invoked by BTD when the read KSP is 0

    // Stack size is encoded in # of 64 byte chunks.
    // e.g., 0x0 = 64 bytes, 0x1 = 128 bytes, etc.
    static constexpr uint32_t StackChunkSize = 64;

    union
    {
        uint32_t stackSizePerRay;       // maximal stack size of a ray

        struct {
            uint32_t MBZ1       : 32 - (T)Bits::stackSizePerRay;
            uint32_t sizePerRay : (T)Bits::stackSizePerRay;
        };
    };
    union
    {
        uint32_t numDSSRTStacks;        // number of stacks per DSS

        struct {
            uint32_t MBZ2        : 32 - (T)Bits::numDSSRTStacks;
            uint32_t numRTStacks : (T)Bits::numDSSRTStacks;
        };
    };
    uint32_t maxBVHLevels;          // the maximal number of supported instancing levels
    uint32_t paddingBits;           // 32-bits of padding

    // Not cached by HW
    uint64_t hitGroupBasePtr;       // base pointer of hit group shader record array (16-bytes alignment)
    uint64_t missShaderBasePtr;     // base pointer of miss shader record array (8-bytes alignment)
    uint64_t callableShaderBasePtr; // base pointer of callable shader record array (8-bytes alignment)
    uint32_t hitGroupStride;        // stride of hit group shader records (16-bytes alignment)
    uint32_t missShaderStride;      // stride of miss shader records (8-bytes alignment)
    uint32_t callableShaderStride;  // stride of callable shader records (8-bytes alignment)
    // HW doesn't read anything below this point.
    // For anything that appears above this line that we want to duplicate below
    // for a more packed layout, we prefix it with a 'p' to denote that it is a
    // 'private' copy of the value.  The strategy here is to order the fields
    // from top to bottom in order from least frequently used to most.
    uint32_t paddingBits2;              // 32-bits of padding
    uint64_t printfBufferBasePtr;       // base pointer of printf buffer
    uint64_t ProfilingBufferGpuVa;      // GTPin buffer to collect profiling data
    uint64_t pCallableShaderBasePtr;    // base pointer of callable shader record array (8-bytes alignment)
    uint32_t pCallableShaderStride;     // stride of callable shader records (8-bytes alignment)
    uint32_t paddingBits3;              // 32-bits of padding
    uint64_t bindlessHeapBasePtr;       // base pointer of bindless heap
    uint64_t pHitGroupBasePtr;          // base pointer of hit group shader record array (16-bytes alignment)
    uint64_t pMissShaderBasePtr;        // base pointer of miss shader record array (8-bytes alignment)
    uint32_t pHitGroupStride;           // stride of hit group shader records (16-bytes alignment)
    uint32_t pMissShaderStride;         // stride of miss shader records (8-bytes alignment)
    uint64_t pRtMemBasePtr;             // base address of the allocated stack memory
    uint32_t baseSSHOffset;             // (index/bindless offset) of first memory region for stateful indirect accesses
    uint32_t pStackSizePerRay;          // maximal stack size of a ray
    uint32_t swStackSizePerRay;         // size in bytes per ray of the SWStack
    uint32_t pNumDSSRTStacks;           // number of stacks per DSS
    uint32_t dispatchRaysDimensions[3]; // dispatch dimensions of the thread grid
    uint32_t paddingBits4;
};

// The actual pointer will probably be aligned to a greater value than this,
// but just specify a value large enough that we can definitely do block reads.
constexpr uint32_t RTGlobalsAlign = 256;

// The 'stackSizePerRay' from 'RayDispatchGlobalData' counts the # of 64B
// chunks of stack so this must be, at minimum, 64-byte aligned.
constexpr uint32_t RTStackAlign = 128;
static_assert(RTStackAlign % RayDispatchGlobalData::StackChunkSize == 0, "no?");

static_assert(sizeof(RayDispatchGlobalData) == 176, "unexpected size?");
#if !defined(__clang__) || (__clang_major__ >= 10)
static_assert(std::is_standard_layout<RayDispatchGlobalData>::value, "no?");
#endif

// This data is passed in as inline data into the raygeneration shader
// by the UMD.
struct RayDispatchInlinedData
{
    uint64_t RayDispatchDescriptorAddress; // ShaderRecord*, r2.0:uq
    uint64_t RayDispatchGlobalDataPtr;     //                r2.1:uq
};

template<typename Type>
constexpr Type Align(const Type value, const size_t alignment)
{
    // alignment must be power of 2.
    Type mask = static_cast<Type>(alignment) - 1;
    return (value + mask) & ~mask;
}

// Maximum size of a block read that we will do from RayDispatchGlobalData.
static constexpr uint32_t MAX_BLOCK_SIZE = 256;

inline uint32_t GlobalsAllocationSize(uint32_t GlobalRootSigSize, uint32_t *RTGlobalsSize = nullptr)
{
    constexpr uint32_t GlobalRootSigOffset = uint32_t(Align(sizeof(RayDispatchGlobalData), sizeof(uint64_t)));

    if (RTGlobalsSize)
        *RTGlobalsSize = GlobalRootSigOffset;

    uint32_t TotalSize = GlobalRootSigOffset + GlobalRootSigSize + MAX_BLOCK_SIZE;

    return Align(TotalSize, 64);
}

enum class RT_TILE_LAYOUT { _1D, _2D };

// We only really care about the 'Y' dimension right now. Even though a dispatch
// like (1024, 1, 1024) could be labelled as 2D, we would better off labelling
// it as 1D. Regardless, we mostly expect the dimensions to be populated
// from X -> Y -> Z so we wouldn't do well on, for example, (1, 1024, 1024).
//
// Pass in the Width, Height, Depth from D3D12_DISPATCH_RAYS_DESC.
constexpr RT_TILE_LAYOUT selectRTTileLayout(
    uint32_t /*Width*/, uint32_t Height, uint32_t /*Depth*/)
{
    return (Height == 1) ? RT_TILE_LAYOUT::_1D : RT_TILE_LAYOUT::_2D;
}

}  // namespace IGC

DISABLE_WARNING_POP  // restore the previously saved pragma state, restore former compiler settings
