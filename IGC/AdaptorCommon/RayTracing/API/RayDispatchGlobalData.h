/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <stddef.h>
#include <stdint.h>
#undef HAS_INCLUDE_TYPE_TRAITS
#ifdef _MSC_VER
#if !defined(__clang__) || (__clang_major__ > 16)
#define HAS_INCLUDE_TYPE_TRAITS
#endif
#endif  // _MSC_VER
#ifdef HAS_INCLUDE_TYPE_TRAITS
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

// clang-format off
#if defined(__clang__)
    #define DO_PRAGMA_DIAG(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA_DIAG(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA_DIAG(GCC diagnostic pop)
    #define DISABLE_WARNING(warningName) DO_PRAGMA_DIAG(GCC diagnostic ignored #warningName)  // internal utility
    // Add warnings that you want to disable here:
    #define DISABLE_WARNING_ANONYMOUS_STRUCT_UNION   \
        DISABLE_WARNING(-Wgnu-anonymous-struct)  // anonymous structs are a GNU extension
    #define DISABLE_WARNING_PADDING_AT_END_OF_STRUCT
    #define DISABLE_WARNING_ANON_TYPES_IN_ANON_UNION \
        DISABLE_WARNING(-Wnested-anon-types)     // anonymous types declared in an anonymous union are an extension
#elif defined(__GNUC__)
    #define DO_PRAGMA_DIAG(X) _Pragma(#X)                                                     // internal utility
    #define DISABLE_WARNING_PUSH         DO_PRAGMA_DIAG(GCC diagnostic push)
    #define DISABLE_WARNING_POP          DO_PRAGMA_DIAG(GCC diagnostic pop)
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
// clang-format on

// save the current pragma state, save the current compiler settings select the
// warnings that we want to disable, for this file only
DISABLE_WARNING_PUSH
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
    uint64_t GetStatelessScratchPtr() const { return 0; };
    uint32_t GetBaseSSHOffset() const { return 0; };
    uint32_t GetUberTilesMap() const { return 0; };
};

// Layout used to pass global data to the shaders
struct RayDispatchGlobalData
{
    // Stack size is encoded in # of 64 byte chunks.
    // e.g., 0x0 = 64 bytes, 0x1 = 128 bytes, etc.
    static constexpr uint32_t StackChunkSize = 64;
    static constexpr uint32_t LargeStack = StackChunkSize * 64;

    struct RayDispatchGlobalDataCommon
    {
        // For anything that appears above this line that we want to duplicate below
        // for a more packed layout, we prefix it with a 'p' to denote that it is a
        // 'private' copy of the value.  The strategy here is to order the fields
        // from top to bottom in order from least frequently used to most.
        uint64_t printfBufferBasePtr;       // base pointer of printf buffer
        uint64_t ProfilingBufferGpuVa;      // GTPin buffer to collect profiling data
        union
        {
            uint64_t padding;
        };
        union
        {
            uint64_t statelessScratchPtr;       // Stateless scratch buffer pointer
        };
        uint64_t pCallableShaderBasePtr;    // base pointer of callable shader record array (8-bytes alignment)
        uint32_t pCallableShaderStride;     // stride of callable shader records (8-bytes alignment)
        uint32_t pNumDSSRTStacks;           // number of stacks per DSS
        union
        {
            uint64_t bindlessHeapBasePtr;    // base pointer of bindless heap
        };
        uint64_t pHitGroupBasePtr;          // base pointer of hit group shader record array (16-bytes alignment)
        uint64_t pMissShaderBasePtr;        // base pointer of miss shader record array (8-bytes alignment)
        uint32_t pHitGroupStride;           // stride of hit group shader records (16-bytes alignment)
        uint32_t pMissShaderStride;         // stride of miss shader records (8-bytes alignment)
        uint64_t pRtMemBasePtr;             // base address of the allocated stack memory

        union
        {
            struct
            {
                uint32_t baseSSHOffset;             // (index/bindless offset) of first memory region for stateful indirect accesses
                uint32_t paddingBits3;              // 32-bits of padding
            };

        };

        uint64_t uberTilesMap;              // base address of the uber tiles map used for AtomicPull model
        uint32_t paddingBits4;              // 1 dword of padding to make sure the struct size is multiple of 8 on all platforms

        uint32_t pStackSizePerRay;          // maximal stack size of a ray
        uint32_t swStackSizePerRay;         // size in bytes per ray of the SWStack
        uint32_t dispatchRaysDimensions[3]; // dispatch dimensions of the thread grid


        template<class TAPIAdaptor>
        void populate(const TAPIAdaptor& umd)
        {
            printfBufferBasePtr       = umd.GetPrintfBufferBasePtr();
            ProfilingBufferGpuVa      = umd.GetProfilingBufferGpuVa();
            statelessScratchPtr       = umd.GetStatelessScratchPtr();
            pCallableShaderBasePtr    = umd.GetCallableShaderTable();
            pCallableShaderStride     = umd.GetCallableStride();
            bindlessHeapBasePtr       = umd.GetBindlessHeapBasePtr();
            pHitGroupBasePtr          = umd.GetHitGroupTable();
            pMissShaderBasePtr        = umd.GetMissShaderTable();
            pHitGroupStride           = umd.GetHitGroupStride();
            pMissShaderStride         = umd.GetMissStride();
            pRtMemBasePtr             = umd.GetRayStackBufferAddress();
            baseSSHOffset             = umd.GetBaseSSHOffset();
            pStackSizePerRay          = umd.GetStackSizePerRay();
            swStackSizePerRay         = umd.GetSWStackSizePerRay();
            pNumDSSRTStacks           = umd.GetNumDSSRTStacks();
            dispatchRaysDimensions[0] = umd.GetDispatchWidth();
            dispatchRaysDimensions[1] = umd.GetDispatchHeight();
            dispatchRaysDimensions[2] = umd.GetDispatchDepth();
            uberTilesMap              = umd.GetUberTilesMap();

        }
    };

    union RT
    {
        struct Xe
        {
            template<class TAPIAdaptor>
            void populate(const TAPIAdaptor& umd)
            {
                rtMemBasePtr        = umd.GetRayStackBufferAddress();
                callStackHandlerPtr = umd.GetCallStackHandlerPtr();
                stack_size_info.stackSizePerRay  = umd.GetStackSizePerRay();
                num_stacks_info.numDSSRTStacks   = umd.GetNumDSSRTStacks();
                rt_data_info.maxBVHLevels        = umd.GetMaxBVHLevels();

                common.populate(umd);
            }

            // Cached by HW (32 bytes)
            uint64_t rtMemBasePtr;          // base address of the allocated stack memory
            uint64_t callStackHandlerPtr;   // this is the KSP of the continuation handler that is invoked by BTD when the read KSP is 0
            union {
                uint32_t stackSizePerRay;       // maximal stack size of a ray
                struct {
                    uint32_t sizePerRay : 8;
                    uint32_t MBZ1       : 24;
                };
            } stack_size_info;
            union {
                uint32_t numDSSRTStacks;        // number of stacks per DSS
                struct {
                    uint32_t numRTStacks : 12;
                    uint32_t MBZ2        : 20;
                };
            } num_stacks_info;
            union {
                uint32_t maxBVHLevels;          // the maximal number of supported instancing levels
                struct {
                    uint32_t bvhLevels : 3;
                    uint32_t MBZ3      : 29;
                };
            } rt_data_info;
            // In addition to the dword of padding to align `common`, we also
            // add 8 dwords so Xe and Xe3 both have the same RTGlobals size.
            uint32_t paddingBits[1+8];          // padding

            // HW doesn't read anything below this point.
            RayDispatchGlobalDataCommon common;
        } xe;
        struct Xe3
        {
            template<class TAPIAdaptor>
            void populate(const TAPIAdaptor& umd)
            {
                rtMemBasePtr        = umd.GetRayStackBufferAddress();
                callStackHandlerPtr = umd.GetCallStackHandlerPtr();
                stack_size_info.stackSizePerRay = umd.GetStackSizePerRay();
                num_stacks_info.numRTStacks = umd.GetNumDSSRTStacks();

                // _pad1_mbz higher 16 bits must be zero.
                num_stacks_info.numRTStacks = (num_stacks_info.numRTStacks & 0x0000FFFF);

                constexpr uint32_t strideMask = (1 << 13) - 1;
                const uint32_t hgs = umd.GetHitGroupStride() & strideMask;
                const uint32_t mss = umd.GetMissStride()     & strideMask;
                rt_data_info.packedData = (umd.GetMaxBVHLevels() << 0) | (hgs << 3) | (mss << 16);

                hitGroupBasePtr   = umd.GetHitGroupTable();
                missShaderBasePtr = umd.GetMissShaderTable();

                common.populate(umd);
            }

            uint64_t rtMemBasePtr;              // base address of the allocated stack memory
            uint64_t callStackHandlerPtr;       // this is the KSP of the continuation handler that is invoked by BTD when the read KSP is 0
            union {
                uint32_t stackSizePerRay;       // async-RT stack size in 64 byte blocks
                uint32_t _pad0_mbz      : 32;
            } stack_size_info;
            union {
                uint32_t numRTStacks;        // number of stacks per DSS
                struct {
                    uint32_t numDSSRTStacks : 16; // number of asynch stacks per DSS
                    uint32_t _pad1_mbz : 16;
                };

            } num_stacks_info;
            union {
                uint32_t packedData;
                struct {
                    uint32_t maxBVHLevels     : 3;  // the maximal number of supported instancing levels (0->8, 1->1, 2->2, ...)
                    uint32_t hitGroupStride   : 13; // stride of hit group shader records (16-bytes alignment)
                    uint32_t missShaderStride : 13; // stride of miss shader records (8-bytes alignment)
                    uint32_t _pad2_mbz        : 3;
                };
            } rt_data_info;
            uint32_t flags   : 1;               // per context control flags
            uint32_t pad_mbz : 31;
            uint64_t hitGroupBasePtr;           // base pointer of hit group shader record array (16-bytes alignment)
            uint64_t missShaderBasePtr;         // base pointer of miss shader record array (8-bytes alignment)
            uint32_t _align_mbz[4];             // pad hardware section to 64 bytes

            // HW doesn't read anything below this point.
            RayDispatchGlobalDataCommon common;
        } xe3;
    } rt;
};

// The actual pointer will probably be aligned to a greater value than this,
// but just specify a value large enough that we can definitely do block reads.
constexpr uint32_t RTGlobalsAlign = 256;

// The 'stackSizePerRay' from 'RayDispatchGlobalData' counts the # of 64B
// chunks of stack so this must be, at minimum, 64-byte aligned.
constexpr uint32_t RTStackAlign = 128;
static_assert(RTStackAlign % RayDispatchGlobalData::StackChunkSize == 0, "no?");

static_assert((sizeof(RayDispatchGlobalData::RT::Xe) - sizeof(RayDispatchGlobalData::RayDispatchGlobalDataCommon)) % 64 == 0, "Unexpected GlobalData alignment");
static_assert((sizeof(RayDispatchGlobalData::RT::Xe3) - sizeof(RayDispatchGlobalData::RayDispatchGlobalDataCommon)) % 64 == 0, "Unexpected GlobalData alignment");

static_assert(sizeof(RayDispatchGlobalData) == 192, "unexpected size?");

static_assert(sizeof(RayDispatchGlobalData::RT::Xe) == sizeof(RayDispatchGlobalData), "unexpected size?");
static_assert(sizeof(RayDispatchGlobalData::RT::Xe3) == sizeof(RayDispatchGlobalData), "unexpected size?");
static_assert(offsetof(RayDispatchGlobalData::RT::Xe, common) == offsetof(RayDispatchGlobalData::RT::Xe3, common), "unexpected size?");
#ifdef HAS_INCLUDE_TYPE_TRAITS
static_assert(std::is_standard_layout<RayDispatchGlobalData>::value, "no?");
#endif // HAS_INCLUDE_TYPE_TRAITS

// This data is passed in as inline data into the raygeneration shader
// by the UMD.
struct RayDispatchInlinedData
{
    uint64_t RayDispatchDescriptorAddress; // ShaderRecord*, r2.0:uq
    uint64_t RayDispatchGlobalDataPtr;     //                r2.1:uq
    uint64_t AtomicPullGlobalQueuePtr;     //                r2.2:uq

    static constexpr unsigned NumElts = 3;
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

struct RTGlobalsAllocationData
{
    // Globals layout should look like this:
    // RayDispatchGlobalData #0 <- main
    // global root signature
    // RayDispatchGlobalData #1 <- auxiliary
    // ...
    // RayDispatchGlobalData #n
    uint32_t totalSize;
    uint32_t rootSigOffset;
    uint32_t auxRTGlobalsStartOffset;
    uint32_t auxRTGlobalsIncrementSize;
};

inline RTGlobalsAllocationData GlobalsAllocationSize(uint32_t GlobalRootSigSize, uint32_t numOfAuxRTGlobals = 0)
{
    RTGlobalsAllocationData data = {0, 0, 0, 0};
    constexpr uint32_t RTGlobalsPointerIncrementSize = uint32_t(Align(sizeof(RayDispatchGlobalData), RTGlobalsAlign));
    constexpr uint32_t GlobalRootSigOffset = uint32_t(Align(sizeof(RayDispatchGlobalData), sizeof(uint64_t)));

    uint32_t TotalSize = GlobalRootSigOffset + GlobalRootSigSize + MAX_BLOCK_SIZE;

    data.rootSigOffset = GlobalRootSigOffset;
    data.totalSize = Align(TotalSize, 64);

    if (numOfAuxRTGlobals)
    {
        data.auxRTGlobalsStartOffset = Align(data.totalSize, RTGlobalsAlign);
        data.auxRTGlobalsIncrementSize = RTGlobalsPointerIncrementSize;
        data.totalSize = Align(data.auxRTGlobalsStartOffset + numOfAuxRTGlobals * data.auxRTGlobalsIncrementSize, 64);
    }

    return data;
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
