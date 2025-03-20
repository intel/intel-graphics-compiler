/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "GenISAIntrinsics/GenIntrinsics.h"

#include <vector>
#include <set>
#include <map>
#include <string>

namespace llvm
{
    class LLVMContext;
    class Function;
    class Type;
}

namespace IGC
{
    /// @brief  ImplicitArg is used for representing the implict information that is passed from the
    ///         OpenCL runtime to IGC, characterize it and allow IGC to use it.
    /// @Author Marina Yatsina
    class ImplicitArg {
    public:

        /// @brief  Type of implicit information passed from the OpenCL runtime
        enum ArgType {
            R0,
            START_ID = R0,
            PAYLOAD_HEADER,
            GLOBAL_OFFSET, // previously packed in 8xi32 PayloadHeader

            // WI information
            WORK_DIM,
            NUM_GROUPS,
            GLOBAL_SIZE,
            LOCAL_SIZE,
            ENQUEUED_LOCAL_WORK_SIZE,
            LOCAL_ID_X,
            LOCAL_ID_Y,
            LOCAL_ID_Z,

            // Pointer bases and buffers
            CONSTANT_BASE,
            GLOBAL_BASE,
            PRIVATE_BASE,
            PRINTF_BUFFER,

            // Buffer offset (for stateless to stateful optim)
            BUFFER_OFFSET,

            // Aggregates
            STRUCT_START,
            CONSTANT_REG_FP32 = STRUCT_START,
            CONSTANT_REG_QWORD,
            CONSTANT_REG_DWORD,
            CONSTANT_REG_WORD,
            CONSTANT_REG_BYTE,
            STRUCT_END = CONSTANT_REG_BYTE,

            // Images
            IMAGES_START,
            IMAGE_HEIGHT = IMAGES_START,
            IMAGE_WIDTH,
            IMAGE_DEPTH,
            IMAGE_NUM_MIP_LEVELS,
            IMAGE_CHANNEL_DATA_TYPE,
            IMAGE_CHANNEL_ORDER,
            IMAGE_SRGB_CHANNEL_ORDER,
            IMAGE_ARRAY_SIZE,
            IMAGE_NUM_SAMPLES,
            SAMPLER_ADDRESS,
            SAMPLER_NORMALIZED,
            SAMPLER_SNAP_WA,
            INLINE_SAMPLER,
            FLAT_IMAGE_BASEOFFSET,
            FLAT_IMAGE_HEIGHT,
            FLAT_IMAGE_WIDTH,
            FLAT_IMAGE_PITCH,
            IMAGES_END = FLAT_IMAGE_PITCH,

            // VME
            VME_MB_BLOCK_TYPE,
            VME_SUBPIXEL_MODE,
            VME_SAD_ADJUST_MODE,
            VME_SEARCH_PATH_TYPE,

            // Device Enqueue
            DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE,
            DEVICE_ENQUEUE_EVENT_POOL,
            DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE,
            DEVICE_ENQUEUE_PARENT_EVENT,
            DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE,
            GET_OBJECT_ID,
            GET_BLOCK_SIMD_SIZE,

            // GAS
            LOCAL_MEMORY_STATELESS_WINDOW_START_ADDRESS,
            LOCAL_MEMORY_STATELESS_WINDOW_SIZE,
            PRIVATE_MEMORY_STATELESS_SIZE,

            STAGE_IN_GRID_ORIGIN,
            STAGE_IN_GRID_SIZE,

            SYNC_BUFFER,

            // Raytracing args
            RT_GLOBAL_BUFFER_POINTER,
            RT_LOCAL_BUFFER_POINTER,
            RT_INLINED_DATA,
            RT_STACK_ID,

            // Bindless buffer (for stateless to bindless optim)
            BINDLESS_OFFSET,

            // Pointer to implicit arguments prepared by runtime
            IMPLICIT_ARG_BUFFER_PTR,

            ASSERT_BUFFER_POINTER,

            // BufferBoundsChecking
            BUFFER_SIZE,

            NUM_IMPLICIT_ARGS
        };

        /// @brief This set type is used to contain function arguments
        typedef std::set<int> ArgValSet;

        /// @brief This map type is used to map implicit argument types to
        ///        sets that contain the relevant Values.
        typedef std::map<ImplicitArg::ArgType, ArgValSet> ArgMap;

        typedef std::pair<ImplicitArg::ArgType, unsigned int> StructArgElement;
        typedef std::vector<StructArgElement> StructArgList;

        /// @brief  LLVM Type
        enum ValType {
            BYTE,
            SHORT,
            INT,
            LONG,
            FP32,
            CONSTPTR,
            PRIVATEPTR,
            GLOBALPTR
        };

        enum ValAlign {
            ALIGN_QWORD,
            ALIGN_DWORD,
            ALIGN_GRF,
            ALIGN_PTR
        };

        ImplicitArg(
            const ArgType&                  argType,
            const std::string&              name,
            const ValType                   valType,
            WIAnalysis::WIDependancy        dependency,
            unsigned int                    nbElement,
            ValAlign                        align,
            bool                            isConstantBuf);

        ImplicitArg(
            const ArgType& argType,
            const std::string& name,
            const ValType                   valType,
            WIAnalysis::WIDependancy        dependency,
            unsigned int                    nbElement,
            ValAlign                        align,
            bool                            isConstantBuf,
            llvm::GenISAIntrinsic::ID       GenIntrinsicID);

        /// @brief  Getter functions
        ArgType                         getArgType() const;
        const std::string&              getName() const;
        llvm::Type*                     getLLVMType(llvm::LLVMContext& context) const;
        WIAnalysis::WIDependancy        getDependency() const;
        unsigned int                    getAllocateSize(const llvm::DataLayout& DL) const;
        unsigned int                    getNumberElements() const;
        VISA_Type                       getVISAType(const llvm::DataLayout& DL) const;
        IGC::e_alignment                getAlignType(const llvm::DataLayout& DL) const;
        size_t                          getAlignment(const llvm::DataLayout& DL) const;
        unsigned int                    getPointerSize(const llvm::DataLayout& DL) const;
        bool                            isConstantBuf() const;
        bool                            isLocalIDs() const;
        llvm::GenISAIntrinsic::ID       getGenIntrinsicID() const;

    private:
        const ArgType                   m_argType;
        const std::string               m_name;
        const ValType                   m_valType;
        const WIAnalysis::WIDependancy  m_dependency;
        const unsigned int              m_nbElement;
        const ValAlign                  m_align;
        const bool                      m_isConstantBuf;
        const llvm::GenISAIntrinsic::ID m_GenIntrinsicID;
    };

    /// @brief  ImplicitArgs is used for accessing the actual implict information that is passed from
    ///         the OpenCL runtime to IGC.
    /// @Author Marina Yatsina
    class ImplicitArgs {
    public:
        // Constructors
        ImplicitArgs() {}

        /// @brief  Constructor.
        ///         Constructs the function's implicit arguments based on the given function's metadata
        ///         It actually constructs a mapping to a subset of IMPLICIT_ARGS
        /// @param  func the function to get impilcit args for.
        /// @param  the metadata utils object
        ImplicitArgs(const llvm::Function& func, const IGCMD::MetaDataUtils* pMdUtils);

        /// @brief  Returns the number of implicit arguments that are passed from the runtime
        /// @return The number of implicit arguments
        unsigned int size() const;

        /// @brief  Returns the i-th implicit argument
        /// @param  i               The implicit argument index
        /// @return The i-th implicit argument
        const ImplicitArg& operator[](unsigned int i) const;

        /// @brief  Returns the index of the appropriate implicit argument based on the given argument type
        /// @param  argType         The implicit argument type
        /// @return The implicit argument's index for a given argument type
        unsigned int getArgIndex(ImplicitArg::ArgType argType) const;

        /// @brief  Returns the index of the appropriate implicit image or sampler argument
        ///         based on the given argument type and the associated image argument
        /// @param  argType         The implicit argument type
        ///                         (height/width/depth for images, normalized/address/snapwa for samplers)
        /// @param  image           The associated image/sampler argument
        /// @return The implicit argument's index for a given argument type
        unsigned int getImageArgIndex(ImplicitArg::ArgType argType, const llvm::Argument* image) const;

        /// @brief  Returns the index of the appropriate implicit argument
        ///         based on the given argument type and the argument number
        /// @param  argType         The implicit argument type
        ///                         (height/width/depth for images, normalized/address/snapwa for samplers)
        /// @param  argNum          The explicit argument number of the type
        /// @return The implicit argument's index for a given argument type
        unsigned int getNumberedArgIndex(ImplicitArg::ArgType argType, int argNum) const;

        /// @brief  Returns if an implicit arg exists for the given explicit argument
        ///         based on the argument type and argument number
        /// @param  argType         The implicit argument type
        ///                         (height/width/depth for images, normalized/address/snapwa for samplers)
        /// @param  argNum          The explicit argument number of the type
        bool isImplicitArgExistForNumberedArg(ImplicitArg::ArgType argType, int argNum) const;

        /// @brief  Returns the argument type of the argument at the given index
        /// @param  i               The implicit argument index
        /// @return The argument type of the argument at the given index
        ImplicitArg::ArgType getArgType(unsigned int i) const;

        /// @brief  Returns the argument type of the given matching intrinsic ID
        /// @param  i               The GenISAIntrinsic ID
        /// @return The argument type
        static ImplicitArg::ArgType getArgType(llvm::GenISAIntrinsic::ID id);

        /// @brief  Returns the argument dependency of the given matching intrinsic ID
        /// @param  i               The GenISAIntrinsic ID
        /// @return The argument dependency
        static IGC::WIAnalysis::WIDependancy getArgDep(llvm::GenISAIntrinsic::ID id);

        /// @brief  Returns if the given arg type supports the GenISAIntrinsic instruction
        /// @param  i               The ImplicitArg type
        /// @return If intrinsic supported
        static bool hasIntrinsicSupport(ImplicitArg::ArgType i);

        /// @brief  Returns the explicit argument number of the given implicit argument index
        /// @param  i               The implicit argument index
        /// @return The explicit argument number of the given implicit argument index
        int32_t getExplicitArgNum(uint index) const;

        /// @brief  Returns the structure offset of the given implicit argument index
        /// @param  i               The implicit argument index
        /// @return The structure offset of the given implicit argument index
        int32_t getStructArgOffset(uint index) const;

        /// @brief  Creates implict arguments metadata for the given function based on the given implicit arguments
        ///         it should receive. If implicit metadata exists, it adds to it.
        /// @param  F               The function for which to create the implicit argument's metadata
        /// @param  implicitArgs    The implicit argument that are required by the given function
        /// @param  pMdUtils The Metadata API object
        static void addImplicitArgs(llvm::Function& F, const llvm::SmallVectorImpl<ImplicitArg::ArgType>& implicitArgs, const IGCMD::MetaDataUtils* pMdUtils);
        //TODO doc

        /// @brief  Creates implict arguments metadata for the given function amd all callers
        ///         based on the given implicit arguments it should receive. If implicit metadata exists, it adds to it.
        /// @param  F               The function for which to create the implicit argument's metadata
        /// @param  implicitArgs    The implicit argument that are required by the given function
        /// @param  pMdUtils The Metadata API object
        static void addImplicitArgsTotally(llvm::Function& F, const llvm::SmallVectorImpl<ImplicitArg::ArgType>& implicitArgs, const IGCMD::MetaDataUtils* pMdUtils);

        /// @brief  Creates implict image arguments metadata for the given function based on the given implicit image
        ///         arguments it should receive. If implicit image metadata exists, it adds to it.
        /// @param  F The function for which to create the implicit argument's metadata
        /// @param  argMap          A map of implict argument types to the Value pointers to the arguments
        /// @param  pMdUtils The Metadata API object
        static void addImageArgs(llvm::Function& F, const ImplicitArg::ArgMap& argMap, const IGCMD::MetaDataUtils* pMdUtils);
        // TODO doc

        static void addStructArgs(llvm::Function& F, const llvm::Argument* A, const ImplicitArg::StructArgList& S, const IGCMD::MetaDataUtils* pMdUtils);

        /// @brief  Creates implict arguments metadata for the given function based on the given implicit arguments
        ///         it should receive. If implicit metadata exists, it adds to it.
        /// @param  F               The function for which to create the implicit argument's metadata
        /// @param  argMap          A map of implict argument types to the set of numbers of arguments
        /// @param  pMdUtils        The Metadata API object
        static void addNumberedArgs(llvm::Function& F, const ImplicitArg::ArgMap& argMap, const IGCMD::MetaDataUtils* pMdUtils);

        /// @brief  Create implicit arguments metadata for the given function. It adds one
        ///         implicit argument for each explicit pointer argument to global or constant buffer.
        /// @param  F               The function for which to create the implicit argument's metadata
        /// @param  pMdUtils        The Metadata API object
        static void addBufferOffsetArgs(llvm::Function& F, const IGCMD::MetaDataUtils* pMdUtils, IGC::ModuleMetaData* modMD);

        /// @brief  Create implicit arguments metadata for the given function. It adds one
        ///         implicit argument for each explicit pointer argument to global or constant buffer.
        /// @param  F               The function for which to create the implicit argument's metadata
        /// @param  pMdUtils        The Metadata API object
        static void addBindlessOffsetArgs(llvm::Function& F, const IGCMD::MetaDataUtils* pMdUtils, IGC::ModuleMetaData* modMD);

        /// @brief  Check if the given implicit argument type exist in the(implicit) function argument associated
        /// @param  argType         The type of the implict argument that should be checked
        /// @return true if the argument exist, false otherwise.
        bool isImplicitArgExist(ImplicitArg::ArgType argType) const;
        static bool isImplicitArgExist(llvm::Function& F, ImplicitArg::ArgType argType, const IGCMD::MetaDataUtils* pMdUtils);

        /// @brief  Returns the (implicit) function argument associated with the given implicit argument type
        /// @param  F           The Function for which the implict argument should be returned
        /// @param  argType         The type of the implict argument that should be returned
        /// @return The (implicit) function argument associated with the given argument type
        ///         In case the argument doesn't exist, return nullptr
        llvm::Argument* getImplicitArg(llvm::Function &F, ImplicitArg::ArgType argType) const;

        /// @brief  Returns the (implicit) function argument associated with the given implicit argument type
        ///         and argument number
        /// @param  F        The Function for which the implict argument should be returned
        /// @param  argType  The type of the implict argument that should be returned
        /// @param  argNum   The explicit argument number of the type
        /// @return The (implicit) function argument associated with the given argument type and number
        ///         In case the argument doesn't exist, return nullptr
        llvm::Argument* getNumberedImplicitArg(llvm::Function &F, ImplicitArg::ArgType argType, int argNum) const;

        /// @brief  Returns true if the given argument type is an image or sampler.
        /// @param  argType The argument type to check.
        static bool isImplicitImage(ImplicitArg::ArgType argType);

        /// @brief  Returns true if the given argument type is a struct
        /// @param  argType The argument type to check.
        static bool isImplicitStruct(ImplicitArg::ArgType argType);

        /// @brief  Returns true if the given argument is an implicit argument.
        /// @param  arg The argument to check.
        bool isImplicitArg(llvm::Argument *arg) const;

        /// @brief  Returns explicit argument number associated with the given implicit argument.
        /// @param  implicitArg The associated implicit argument.
        int getExplicitArgNumForArg(llvm::Argument *implicitArg) const;

        llvm::Value* getImplicitArgValue(llvm::Function& F, ImplicitArg::ArgType argType, const IGCMD::MetaDataUtils* pMdUtils);

    private:
        /// @brief The function's metadata information.
        IGCMD::FunctionInfoMetaDataHandle m_funcInfoMD;

        static bool isImplicitArgExist(
            const IGCMD::FunctionInfoMetaDataHandle& funcInfo,
            ImplicitArg::ArgType argType);
    };

} // namespace IGC
