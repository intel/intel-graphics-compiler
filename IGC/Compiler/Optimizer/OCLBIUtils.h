/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/Optimizer/ValueTracker.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/Intrinsics.h>
#include "GenISAIntrinsics/GenIntrinsics.h"
#include <map>
#include "Probe/Assertion.h"

namespace llvm {
class CallInst;
}

namespace IGC {
llvm::CallInst *CallMemoryFenceWorkgroup(llvm::Instruction *pInsertBefore);

class CCommand {
public:
  /// @brief  gets llvm function declaration for GenISA_ISA Intrinsic
  /// @param  Id          the wanted GenISA_ISA intrinsic
  /// @param  ptrTy       for intrinsic with overloaded types.
  /// @returns    llvm Function
  llvm::Function *getFunctionDeclaration(llvm::GenISAIntrinsic::ID id,
                                         llvm::ArrayRef<llvm::Type *> Tys = llvm::ArrayRef<llvm::Type *>());

  /// @brief  gets llvm function declaration for LLVM Intrinsic
  /// @param  Id          the wanted LLVM intrinsic
  /// @param  ptrTy       for intrinsic with overloaded types.
  /// @returns    llvm Function
  llvm::Function *getFunctionDeclaration(IGCLLVM::Intrinsic id,
                                         llvm::ArrayRef<llvm::Type *> Tys = llvm::ArrayRef<llvm::Type *>());

  /// @brief  Creates llvm function call for LLVM Intrinsic and replace the
  ///         current instruction with the new function call.
  /// @param  Id          the wanted LLVM intrinsic
  /// @param  ptrTy       for intrinsic with overloaded types.
  void replaceCallInst(IGCLLVM::Intrinsic intrinsicName,
                       llvm::ArrayRef<llvm::Type *> Tys = llvm::ArrayRef<llvm::Type *>());

  /// @brief  Creates llvm function call for GenISA_ISA Intrinsic and replace the
  ///         current instruction with the new function call.
  /// @param  Id          the wanted GenISA_ISA intrinsic
  /// @param  ptrTy       for intrinsic with overloaded types.
  void replaceGenISACallInst(llvm::GenISAIntrinsic::ID intrinsicName,
                             llvm::ArrayRef<llvm::Type *> Tys = llvm::ArrayRef<llvm::Type *>());

  /// @brief  reap init() and createIntrinsic().
  /// @param  Inst            the call instruction that need to be replaced.
  /// @param  CodeGenContext  context to return errors
  void execute(llvm::CallInst *Inst, IGC::CodeGenContext *CodeGenContext);

  /// @brief  replace the old __builtin_IB function call with the match llvm/GenISA_ISA
  ///         instructions.
  /// @param  Inst        the call instruction that need to be replaced.
  virtual void createIntrinsic() = 0;

  /// @brief  verify that there are no user errors
  virtual void verifyCommand() {}

  /// @brief  initialize the callInst,  function, context, constants and clear the arg list.
  /// @param  Inst            the call instruction that need to be replaced.
  /// @param  CodeGenContext  context to return errors
  void init(llvm::CallInst *Inst, IGC::CodeGenContext *CodeGenContext);
  void clearArgsList(void);

  /// @brief returns true if command executed with errors
  bool hasError() const { return m_bHasError; }

  enum CoordType { FloatType, IntType };

  CCommand(void)
      : m_pFloatZero(nullptr), m_pIntZero(nullptr), m_pIntOne(nullptr), m_pCallInst(nullptr), m_pFunc(nullptr),
        m_pCtx(nullptr), m_pCodeGenContext(nullptr), m_pFloatType(nullptr), m_pIntType(nullptr), m_bHasError(false) {}
  virtual ~CCommand(void) {};

protected:
  /// @brief emits error using CodeGenContext, sets error state of CCommand to true
  /// @param ErrorStr error string to emit
  /// @param Context Value to get debug location to print out the relevant info.
  void emitError(const char *ErrorStr, const llvm::Value *Context);

  llvm::Value *m_pFloatZero;
  llvm::Value *m_pIntZero;
  llvm::Value *m_pIntOne;
  llvm::CallInst *m_pCallInst;
  llvm::Function *m_pFunc;
  llvm::LLVMContext *m_pCtx;
  IGC::CodeGenContext *m_pCodeGenContext;
  llvm::SmallVector<llvm::Value *, 10> m_args;
  llvm::DebugLoc m_DL;
  llvm::Type *m_pFloatType;
  llvm::IntegerType *m_pIntType;
  bool m_bHasError;
};

static inline BufferType ResourceTypeMap(ResourceTypeEnum type) {
  switch (type) {
  case IGC::UAVResourceType:
    return UAV;
  case IGC::BindlessUAVResourceType:
    return BINDLESS;
  case IGC::SRVResourceType:
    return RESOURCE;
  case IGC::SamplerResourceType:
    return SAMPLER;
  case IGC::BindlessSamplerResourceType:
    return BINDLESS_SAMPLER;
  case IGC::OtherResourceType:
    return BUFFER_TYPE_UNKNOWN;
  default:
    IGC_ASSERT_MESSAGE(0, "unknown type!");
    return BUFFER_TYPE_UNKNOWN;
  }
};

class CImagesBI : public CCommand {
public:
  enum Dimension { DIM_UNKNOWN, DIM_1D, DIM_1D_ARRAY, DIM_2D, DIM_2D_ARRAY, DIM_3D };

  enum CoordComponents { COORD_X, COORD_Y, COORD_Z, COORD_W };

  enum ImageProperty { WIDTH, HEIGHT, DEPTH, ARRAY_SIZE_1D, ARRAY_SIZE_2D };

  struct ParamInfo {
    ParamInfo(int i, ResourceTypeEnum t, ResourceExtensionTypeEnum extension)
        : index(i), type(ResourceTypeMap(t)), extension(extension) {}
    ParamInfo() : index(0), type(BUFFER_TYPE_UNKNOWN), extension(NonExtensionType) {}

    int index;
    BufferType type;
    ResourceExtensionTypeEnum extension;
  };

  typedef llvm::MapVector<llvm::Value *, ParamInfo> ParamMap;
  typedef std::map<int, int> InlineMap;

  class CImagesUtils {
  public:
    /// @brief  find the BTI of the image argumant
    /// @param  paramIndex  the index of the image paramtere in the call isntruciton
    /// @returns  the image index
    static llvm::ConstantInt *getImageIndex(ParamMap *pParamMap, llvm::CallInst *pCallInst, unsigned int paramIndex,
                                            llvm::Value *&imageParam);

    /// @brief  find the type (UAV/RESOURCE) of the image argumant
    /// @returns  the image type
    static BufferType getImageType(ParamMap *pParamMap, llvm::CallInst *pCallInst, unsigned int paramIndex);

    /// @brief  find the image argument associated with the given bufType and idx
    /// @returns the image argument
    static llvm::Argument *findImageFromBufferPtr(const IGC::IGCMD::MetaDataUtils &MdUtils, llvm::Function *F,
                                                  BufferType bufType, uint64_t idx, const IGC::ModuleMetaData *modMD);
  };

  CImagesBI(ParamMap *paramMap, InlineMap *inlineMap, int *nextSampler, Dimension Dim)
      : m_pParamMap(paramMap), m_pInlineMap(inlineMap), m_pNextSampler(nextSampler), m_dim(Dim), CoordX(nullptr),
        CoordY(nullptr), CoordZ(nullptr), m_IncorrectBti(false) {}

  CImagesBI(ParamMap *paramMap)
      : m_pParamMap(paramMap), m_pInlineMap(nullptr), m_pNextSampler(nullptr), m_dim(Dimension::DIM_UNKNOWN),
        CoordX(nullptr), CoordY(nullptr), CoordZ(nullptr), m_IncorrectBti(false) {}

  /// @brief  push 3 zeros offset to the function argumant list
  void prepareZeroOffsets(void);

  /// @brief  initilaize CoordX/Y/Z with the given coordinates.
  /// @param  Dim         the image dimension
  /// @param  Coord       the coords from the original intrinsic.
  /// @param  Zero        const zero with the correct type (i.e. float or int)
  ///                     it been use only for image_1d and image_2d
  void prepareCoords(Dimension Dim, llvm::Value *Coord, llvm::Value *Zero);

  /// @brief  push the colors to the function argument list
  /// @param  Color       the Color value
  void prepareColor(llvm::Value *Color);

  /// @brief  set LOD to Zero and push the function argument list
  /// @param  Coord       the type of zero (int or float)
  void prepareLOD(CoordType Coord);

  /// @brief  push the sampler value into the function argument list
  void prepareSamplerValue(void);

  /// @brief  create annotations for inline sampler based on the value used for initialization
  /// @brief  this method doesn't fill out the 'index' field of InlineSamplersMD.
  static void CreateInlineSamplerAnnotations(llvm::Module *M, InlineSamplersMD &inlineSamplerMD, int samplerValue);

  /// @brief  create a call to the GetBufferPtr intrinsic pseudo-instruction
  /// @brief  push the image index into the function argument list
  void createGetBufferPtr(void);

  /// @brief  returns "true" if the "val" is integer or float with fractional part = 0.
  static bool derivedFromInt(const llvm::Value *pVal);

  void verifyCommand();

protected:
  /// @brief  push the image index into the function argument list
  void prepareImageBTI(void);

  /// @brief  push the paired resource pointer into the function argument list
  void preparePairedResource(void);

  /// @brief  Prepare args for GenISA_typedread intrinsic call.
  /// @brief  Calls createGetBufferPtr, adds CoordX, Y and Z, then LOD.
  void prepareTypedReadArgs();

  /// @brief  Replace m_pCallInst with call to GenISA_typedread intrinsic.
  /// @brief  Method specifically for handling read_write images.
  void replaceGenISATypedRead();

  // m_pParamMap - maps image and sampler kernel parameters to BTIs
  //               and sampler array indexes, respecitvely
  ParamMap *m_pParamMap;

  // m_pInlineMap - maps inline sampler values (CLK_...) to sampler
  //                array indexes.
  InlineMap *m_pInlineMap;

  // The next sampler index to allocate a newly encountered
  // inline sampler
  int *m_pNextSampler;

  Dimension m_dim;
  llvm::Value *CoordX;
  llvm::Value *CoordY;
  llvm::Value *CoordZ;
  bool m_IncorrectBti;
  ModuleMetaData *m_modMD = nullptr;
};

class CBuiltinsResolver {
private:
  std::map<llvm::StringRef, std::unique_ptr<CCommand>> m_CommandMap;

  // the list of known builtins not to be resolved
  std::vector<llvm::StringRef> m_KnownBuiltins;
  CodeGenContext *m_CodeGenContext;

public:
  CBuiltinsResolver(CImagesBI::ParamMap *paramMap, CImagesBI::InlineMap *inlineMap, int *nextSampler,
                    CodeGenContext *ctx);
  ~CBuiltinsResolver(void) {}

  // resolve __builtin_IB_* calls.
  // return values:
  //              true - when CallInst was __builtin_IB_* and successfully repleced
  //              false - when CallInst wasn't __builtin_IB*
  bool resolveBI(llvm::CallInst *Inst);
};

union InlineSamplerState {
  struct _state {
    uint64_t TCXAddressMode : 3;
    uint64_t TCYAddressMode : 3;
    uint64_t TCZAddressMode : 3;
    uint64_t MagFilterMode : 2;
    uint64_t MinFilterMode : 2;
    uint64_t MipFilterMode : 2;
    uint64_t NormalizedCoords : 1;
    uint64_t CompareFunction : 4;
    uint64_t _reserved : 44;
  } state;
  uint64_t _Value;

  InlineSamplerState() : _Value(0) {}
  InlineSamplerState(uint64_t samplerValue) : _Value(samplerValue) {}

  enum ADDRESS_MODE {
    ADDRESS_MODE_WRAP,
    ADDRESS_MODE_CLAMP,
    ADDRESS_MODE_MIRROR,
    ADDRESS_MODE_BORDER,
    ADDRESS_MODE_MIRRORONCE,
    ADDRESS_MODE_MIRROR101,
    NUM_ADDRESS_MODE
  };

  enum MAP_FILTER_MODE // Applies for MagFilterMode and MinFilterMode
  {
    MAP_FILTER_MODE_POINT,
    MAP_FILTER_MODE_LINEAR,
    MAP_FILTER_MODE_ANISOTROPIC,
    MAP_FILTER_MODE_GAUSSIANQUAD,
    MAP_FILTER_MODE_PYRAMIDALQUAD,
    MAP_FILTER_MODE_MONO,
    NUM_MAP_FILTER_MODE
  };

  enum MIP_FILTER_MODE { MIP_FILTER_MODE_POINT, MIP_FILTER_MODE_LINEAR, MIP_FILTER_MODE_NONE, NUM_MIP_FILTER_MODE };

  enum NORMALIZED_MODE {
    NORMALIZED_MODE_FALSE,
    NORMALIZED_MODE_TRUE,
  };

  enum COMPARE_FUNC {
    COMPARE_FUNC_NONE,
    COMPARE_FUNC_LESS,
    COMPARE_FUNC_LESS_EQUAL = 2,
    COMPARE_FUNC_GREATER = 3,
    COMPARE_FUNC_GREATER_EQUAL = 4,
    COMPARE_FUNC_EQUAL = 5,
    COMPARE_FUNC_NOT_EQUAL = 6,
    COMPARE_FUNC_ALWAYS = 7,
    COMPARE_FUNC_NEVER = 8,
    NUM_COMPARE_FUNC
  };
};

// Built-in image functions
// These values need to match the runtime equivalent
#define LEGACY_CLK_ADDRESS_NONE 0x00
#define LEGACY_CLK_ADDRESS_CLAMP 0x01
#define LEGACY_CLK_ADDRESS_CLAMP_TO_EDGE 0x02
#define LEGACY_CLK_ADDRESS_REPEAT 0x03
#define LEGACY_CLK_ADDRESS_MIRRORED_REPEAT 0x04
#define LEGACY_CLK_ADDRESS_MIRRORED_REPEAT_101_INTEL 0x05

#define LEGACY_CLK_NORMALIZED_COORDS_FALSE 0x00
#define LEGACY_CLK_NORMALIZED_COORDS_TRUE 0x08

#define LEGACY_CLK_FILTER_NEAREST 0x00
#define LEGACY_CLK_FILTER_LINEAR 0x10

// Mask values for the various components
#define LEGACY_SAMPLER_ADDRESS_MASK 0x07
#define LEGACY_SAMPLER_NORMALIZED_MASK 0x08
#define LEGACY_SAMPLER_FILTER_MASK 0x10

// These values need to match SPIR sampler enum
#define SPIR_CLK_ADDRESS_NONE 0x00
#define SPIR_CLK_ADDRESS_CLAMP_TO_EDGE 0x02
#define SPIR_CLK_ADDRESS_CLAMP 0x04
#define SPIR_CLK_ADDRESS_REPEAT 0x06
#define SPIR_CLK_ADDRESS_MIRRORED_REPEAT 0x08
#define SPIR_CLK_ADDRESS_MIRRORED_REPEAT_101_INTEL 0x0A

#define SPIR_CLK_NORMALIZED_COORDS_FALSE 0x00
#define SPIR_CLK_NORMALIZED_COORDS_TRUE 0x01

#define SPIR_CLK_FILTER_NEAREST 0x0010
#define SPIR_CLK_FILTER_LINEAR 0x0020

// Mask values for the various components
#define SPIR_SAMPLER_NORMALIZED_MASK 0x01
#define SPIR_SAMPLER_ADDRESS_MASK 0x0e
#define SPIR_SAMPLER_FILTER_MASK 0x30

} // namespace IGC
