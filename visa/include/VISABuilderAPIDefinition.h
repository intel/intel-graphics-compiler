/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_BUILDER_DEFINITION_H
#define VISA_BUILDER_DEFINITION_H

#include "JitterDataStruct.h"
#include "KernelInfo.h"
#include "RelocationInfo.h"
#include "KernelCostInfo.h"
#include "VISAOptions.h"
#include "visa_igc_common_header.h"

#include <unordered_set>

#define VISA_BUILDER_API

// Forward declares of vISA variable and operand types.
struct VISA_GenVar;
struct VISA_AddrVar;
struct VISA_PredVar;
struct VISA_SamplerVar;
struct VISA_SurfaceVar;

struct VISA_LabelOpnd;
struct VISA_VectorOpnd;
struct VISA_RawOpnd;
struct VISA_PredOpnd;
struct VISA_StateOpndHandle;

class VISAKernel {
public:
  /********** CREATE VARIABLE APIS START ******************/
  ///  CreateVISAGenVar - create an instance of vISA general variable and return
  ///  it via decl. varName must not be NULL, but need not be unique
  ///  numberElemetns must be [1,4096]
  ///  dataType may be one of UD, D, UW, W, UB, B, UQ, Q, DF, HF, F.
  ///  varAlign must be greater than or equal to the data type of the variable,
  ///  and may be specified only when parentDecl is NULL parentDecl, if
  ///  specified, means that this variable is an alias of another variable
  ///  starting at aliasOffset, which is in bytes.  parentDecl is permitted to
  ///  be itself an alias of another variable.  It is an error if the variable's
  ///  actual offset in the root (non-aliased) variable is not aligned to
  ///  dataType
  VISA_BUILDER_API virtual int
  CreateVISAGenVar(VISA_GenVar *&decl, const char *varName, int numberElements,
                   VISA_Type dataType, VISA_Align varAlign,
                   VISA_GenVar *parentDecl = NULL, int aliasOffset = 0) = 0;

  /// CreateVISAAddrVar - create an instance of vISA address variable and return
  /// it via decl. an address variablbe must have type UW numberelements must be
  /// [1, 16]
  VISA_BUILDER_API virtual int
  CreateVISAAddrVar(VISA_AddrVar *&decl, const char *varName,
                    unsigned int numberElements) = 0;

  /// AddKernelAttribute - create an attribute for the kernel.  See the vISA
  /// spec for a list of recognized attributes. name must be a ASCII string with
  /// length <=64
  VISA_BUILDER_API virtual int AddKernelAttribute(const char *name, int size,
                                                  const void *value) = 0;

  /// CreateVISAPredVar - create an instance of vISA predicate variable and
  /// return it via decl. a predicate variable must have type Bool
  /// numberelements must be {1,2,4,8,16,32}
  VISA_BUILDER_API virtual int
  CreateVISAPredVar(VISA_PredVar *&decl, const char *varName,
                    unsigned short numberElements) = 0;

  /// FIXME: we should have different names for them
  /// AddAttributeToVar - add an attribute for the variable.
  VISA_BUILDER_API virtual int AddAttributeToVar(VISA_PredVar *decl,
                                                 const char *varName,
                                                 unsigned int size,
                                                 void *val) = 0;

  VISA_BUILDER_API virtual int AddAttributeToVar(VISA_SurfaceVar *decl,
                                                 const char *varName,
                                                 unsigned int size,
                                                 void *val) = 0;

  VISA_BUILDER_API virtual int AddAttributeToVar(VISA_GenVar *decl,
                                                 const char *name,
                                                 unsigned int size,
                                                 void *val) = 0;

  VISA_BUILDER_API virtual int AddAttributeToVar(VISA_AddrVar *decl,
                                                 const char *name,
                                                 unsigned int size,
                                                 void *val) = 0;

  /// CreateVISASamplerVar - create an instance of vISA sampler variable and
  /// return it via decl. a sampler variable must have type UD numberElements
  /// must be [1,128]
  VISA_BUILDER_API virtual int
  CreateVISASamplerVar(VISA_SamplerVar *&decl, const char *name,
                       unsigned int numberElements) = 0;

  /// CreateVISASurfaceVar - create an instance of vISA surface variable and
  /// return it via decl. a surface variable must have type UD numberElements
  /// must be [1,128]
  VISA_BUILDER_API virtual int
  CreateVISASurfaceVar(VISA_SurfaceVar *&decl, const char *name,
                       unsigned int numberElements) = 0;

  /// CreateVISALabelVar - create an instance of vISA label variable and return
  /// it via decl. a label is either a block label or a subroutine label. name
  /// is for IR dump and debugging only; a fresh label is always returned even
  /// if another label with same name exists. The user should be responsible for
  /// managing the created label.
  VISA_BUILDER_API virtual int CreateVISALabelVar(VISA_LabelOpnd *&opnd,
                                                  const char *name,
                                                  VISA_Label_Kind kind) = 0;

  /// CreateVISAImplicitInputVar - create an input variable from a vISA general
  /// variable offset is the zero-based byte offset of the input size is in
  /// number of bytes for this variable. It must match the size of the variable
  /// kind What type of impliict argument it is
  VISA_BUILDER_API virtual int
  CreateVISAImplicitInputVar(VISA_GenVar *decl, unsigned short offset,
                             unsigned short size, unsigned short kind) = 0;

  /// CreateVISAInputVar - create an input variable from a vISA general variable
  /// offset is the zero-based byte offset of the input
  /// size is in number of bytes for this variable. It must match the size of
  /// the variable
  VISA_BUILDER_API virtual int CreateVISAInputVar(VISA_GenVar *decl,
                                                  unsigned short offset,
                                                  unsigned short size) = 0;

  /// CreateVISAInputVar - create an input variable from a vISA sampler variable
  /// offset is the zero-based byte offset of the input
  /// size is in number of bytes for this variable. It must match the size of
  /// the variable
  VISA_BUILDER_API virtual int CreateVISAInputVar(VISA_SamplerVar *decl,
                                                  unsigned short offset,
                                                  unsigned short size) = 0;

  /// CreateVISAInputVar - create an input variable from a vISA surface variable
  /// offset is the zero-based byte offset of the input
  /// size is in number of bytes for this variable. It must match the size of
  /// the variable
  VISA_BUILDER_API virtual int CreateVISAInputVar(VISA_SurfaceVar *decl,
                                                  unsigned short offset,
                                                  unsigned short size) = 0;

  /// GetPredefinedVar - return a handle to a predefined general variable (e.g.,
  /// r0)
  VISA_BUILDER_API virtual int GetPredefinedVar(VISA_GenVar *&predDcl,
                                                PreDefined_Vars varName) = 0;

  /// GetPredefinedSurface - return a handle to a predefined surface (e.g., SLM
  /// surface)
  VISA_BUILDER_API virtual int
  GetPredefinedSurface(VISA_SurfaceVar *&surfDcl,
                       PreDefined_Surface surfaceName) = 0;

  /// GetBindlessSampler - return the pre-defined bindless sampler index
  VISA_BUILDER_API virtual int
  GetBindlessSampler(VISA_SamplerVar *&samplerDcl) = 0;
  /********** CREATE VARIALBE APIS END ******************/

  /********** CREATE OPERAND APIS START ******************/
  /// It is not permitted to use an operand in more than one instruction; fresh
  /// operands should be created from the variable for each instruction.

  /// CreateVISAAddressSrcOperand -- create a vISA address source operand
  /// (A_N(i)) from an address variable offset is the offset in number of
  /// elements width is the number of contiguous data elements accessed for this
  /// operand
  VISA_BUILDER_API virtual int
  CreateVISAAddressSrcOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl,
                              unsigned int offset, unsigned int width) = 0;

  /// CreateVISAAddressDstOperand -- create a vISA address destination operand
  /// (A_N(i)) from an address variable offset is the offset in number of
  /// elements
  VISA_BUILDER_API virtual int
  CreateVISAAddressDstOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl,
                              unsigned int offset) = 0;

  /// CreateVISAAddressOfOperand -- create a vISA addressof operand
  /// (&V_N+offset) from a general variable offset is the offset in bytes from
  /// the variable base
  VISA_BUILDER_API virtual int
  CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd, VISA_GenVar *decl,
                             unsigned int offset) = 0;

  /// CreateVISAAddressOfOperand -- create a vISA addressof operand
  /// (&V_N+offset) from a surface variable offset is the offset in bytes from
  /// the variable base
  VISA_BUILDER_API virtual int
  CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd, VISA_SurfaceVar *decl,
                             unsigned int offset) = 0;

  /// CreateVISAIndirectSrcOperand -- create a vISA indirect source operand
  /// (r[A_N(addrOffset),
  /// immedOffset]<verticalStride;width,horizontalStride>:type) mod is the
  /// source modifier addrOffset is in number of elements immedOffset is in byte
  /// and must be [-512, 511] verticalStride, width, and horizontalStride encode
  /// the 2D source region
  /// FIXME: should this be removed?
  VISA_BUILDER_API virtual int CreateVISAIndirectSrcOperand(
      VISA_VectorOpnd *&opnd, VISA_AddrVar *cisa_decl, VISA_Modifier mod,
      unsigned int addrOffset, short immediateOffset,
      unsigned short verticalStride, unsigned short width,
      unsigned short horizontalStride, VISA_Type type) = 0;

  /// CreateVISAIndirectDstOperand -- create a vISA indirect dst operand
  /// (r[A_N(addrOffset), immedOffset]<horizontalStride>:type)
  /// addrOffset is in number of elements
  /// immedOffset is in byte and must be [-512, 511]
  /// horizontalStride encode the 1D destination region
  VISA_BUILDER_API virtual int
  CreateVISAIndirectDstOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl,
                               unsigned int addrOffset, short immediateOffset,
                               unsigned short horizontalStride,
                               VISA_Type type) = 0;

  /// CreateVISAIndirectOperandVxH -- create a vISA indirect source operand with
  /// multiple addresses (r[A_N(addrOffset), immedOffset]<1,0>:type) mod is the
  /// source modifier addrOffset is in number of elements
  /// immedOffset is in byte and must be [-512, 511]
  VISA_BUILDER_API virtual int
  CreateVISAIndirectOperandVxH(VISA_VectorOpnd *&cisa_opnd, VISA_AddrVar *decl,
                               VISA_Modifier mod, unsigned int addrOffset,
                               short immediateOffset, VISA_Type type) = 0;

  /// CreateVISAPredicateOperand -- create a vISA predicate operand that can be
  /// used for predicated execution
  VISA_BUILDER_API virtual int
  CreateVISAPredicateOperand(VISA_PredOpnd *&opnd, VISA_PredVar *decl,
                             VISA_PREDICATE_STATE state,
                             VISA_PREDICATE_CONTROL cntrl) = 0;

  /// CreateVISASrcOperand -- create a vISA direct source operand from a general
  /// variable (V_N(offset)<vStride;widht,hStride>)
  /// FIXME: combine rowOffset and colOffsret into a single element offset
  VISA_BUILDER_API virtual int
  CreateVISASrcOperand(VISA_VectorOpnd *&opnd, VISA_GenVar *cisa_decl,
                       VISA_Modifier mod, unsigned short vStride,
                       unsigned short width, unsigned short hStride,
                       unsigned char rowOffset, unsigned char colOffset) = 0;

  /// CreateVISADstOperand -- create a vISA direct destination operand from a
  /// general variable (V_N(offset)<hStride>)
  /// FIXME: combine rowOffset and colOffsret into a single element offset
  VISA_BUILDER_API virtual int
  CreateVISADstOperand(VISA_VectorOpnd *&opnd, VISA_GenVar *decl,
                       unsigned short hStride, unsigned char rowOffset,
                       unsigned char colOffset) = 0;

  /// CreateVISAImmediate -- create a vISA immediate operand based on val and
  /// type sizeof(type) bytes will be read from val and converted to the
  /// immediate value with the specified type
  VISA_BUILDER_API virtual int CreateVISAImmediate(VISA_VectorOpnd *&opnd,
                                                   const void *val,
                                                   VISA_Type type) = 0;

  /// CreateVISAStateOperand -- create a vISA state operand (S_N(offset)) from
  /// either a surface variable offset is in number of elements
  VISA_BUILDER_API virtual int CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                                      VISA_SurfaceVar *decl,
                                                      unsigned char offset,
                                                      bool useAsDst) = 0;

  /// CreateVISAStateOperand -- create a vISA state operand (S_N(offset)) from
  /// either a surface variable offset is in number of elements
  VISA_BUILDER_API virtual int
  CreateVISAStateOperand(VISA_VectorOpnd *&opnd, VISA_SurfaceVar *decl,
                         uint8_t size, unsigned char offset, bool useAsDst) = 0;

  /// CreateVISAStateOperand -- create a vISA state operand (S_N(offset)) from
  /// either a sampler variable offset is in number of elements
  VISA_BUILDER_API virtual int CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                                      VISA_SamplerVar *decl,
                                                      unsigned char offset,
                                                      bool useAsDst) = 0;

  VISA_BUILDER_API virtual int
  CreateVISAStateOperand(VISA_VectorOpnd *&opnd, VISA_SamplerVar *decl,
                         uint8_t size, unsigned char offset, bool useAsDst) = 0;

  VISA_BUILDER_API virtual int
  CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd,
                               VISA_SurfaceVar *decl) = 0;

  VISA_BUILDER_API virtual int
  CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd,
                               VISA_SamplerVar *decl) = 0;

  /// CreateVISARawOperand -- create a vISA raw operand (V_N + offset) from a
  /// general variable offset is in number of bytes a raw operand must be GRF
  /// (256 byte) aligned
  VISA_BUILDER_API virtual int CreateVISARawOperand(VISA_RawOpnd *&opnd,
                                                    VISA_GenVar *decl,
                                                    unsigned short offset) = 0;

  /// CreateVISANullRawOperand -- A short hand for createing V0<0;1,0>
  VISA_BUILDER_API virtual int CreateVISANullRawOperand(VISA_RawOpnd *&opnd,
                                                        bool isDst) = 0;
  /********** CREATE OPERAND APIS END ******************/

  /********** APPEND INSTRUCTION APIS START ******************/

  /// AppendVISAArithmeticInst -- append an one-source vISA arithmetic
  /// instruction to this kernel [pred] op[.sat] (emask, execSize) dst src0
  VISA_BUILDER_API virtual int
  AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
                           VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                           VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0) = 0;

  /// AppendVISAArithmeticInst -- append a two-source vISA arithmetic
  /// instruction to this kernel [pred] op[.sat] (emask, execSize) dst src0 src1
  VISA_BUILDER_API virtual int
  AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
                           VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                           VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0,
                           VISA_VectorOpnd *src1) = 0;

  /// AppendVISAArithmeticInst -- append a three-source vISA arithmetic
  /// instruction to this kernel [pred] op[.sat] (emask, execSize) dst src0 src1
  /// src2
  VISA_BUILDER_API virtual int
  AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
                           VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                           VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0,
                           VISA_VectorOpnd *src1, VISA_VectorOpnd *src2) = 0;

  /// AppendVISAArithmeticTwoDstInst -- append a two-dst, two-source vISA
  /// arithmetic instruction to this kernel [pred] op (emask, execSize) dst0
  /// dst1 src0 src1 This is used by addc and subb
  VISA_BUILDER_API virtual int AppendVISATwoDstArithmeticInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
      VISA_Exec_Size executionSize, VISA_VectorOpnd *dst1,
      VISA_VectorOpnd *carry_borrow, VISA_VectorOpnd *src0,
      VISA_VectorOpnd *src1) = 0;

  // AppendVISAPredDstArithmeticInst -- append an arithmetic instruction with
  // two dsts (one grf dst, one pred dst) and one or two srcs to this kernel.
  //   (p) op (emask, execSize) dst predDst, src0  src1
  VISA_BUILDER_API virtual int AppendVISAPredDstArithmeticInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
      VISA_Exec_Size executionSize, VISA_VectorOpnd *vecDst,
      VISA_PredVar *predDst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) = 0;

  /// AppendVISADpasInst -- append a DPAS instruction to this kernel.
  ///    op (execSize) dst src0 src1 src2
  /// Precision, depth, repeat count are constant and considered as fields
  /// of the inst (not operands.). Note that src0 could be null operand.
  VISA_BUILDER_API virtual int
  AppendVISADpasInst(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                     VISA_Exec_Size executionSize, VISA_RawOpnd *tmpDst,
                     VISA_RawOpnd *src0, VISA_RawOpnd *src1,
                     VISA_VectorOpnd *src2, GenPrecision A, GenPrecision W,
                     uint8_t D, uint8_t C) = 0;

  /// AppendVISABfnInst -- append a BFN instruction to this kernel.
  ///    [pred] op.booleanFuncCtrl[.sat] (emask, execSize) dst src0 src1 src2
  /// <booleanFuncCtrl> is constant and is considered as a field of the inst,
  /// not as an operand.
  VISA_BUILDER_API virtual int
  AppendVISABfnInst(uint8_t booleanFuncCtrl, VISA_PredOpnd *pred, bool satMode,
                    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                    VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0,
                    VISA_VectorOpnd *src1, VISA_VectorOpnd *sdrc2) = 0;

  /// AppendVISAQwordGatherInst -- append a qword scattered read instruction to
  /// this kernel
  VISA_BUILDER_API virtual int AppendVISAQwordGatherInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Num numBlocks, VISA_StateOpndHandle *surface,
      VISA_RawOpnd *address, VISA_RawOpnd *src) = 0;

  /// AppendVISAQwordScatterInst -- append a qword scattered write instruction
  /// to this kernel
  VISA_BUILDER_API virtual int AppendVISAQwordScatterInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Num numBlocks, VISA_StateOpndHandle *surface,
      VISA_RawOpnd *address, VISA_RawOpnd *dst) = 0;


  ///////////////////////////////////////////////////////////////////////////
  // LSC untyped operations
  //
  /// append an *untyped* LSC load operation
  ///
  ///  * dstData can be nullptr if the load is a prefetch only
  ///
  VISA_BUILDER_API virtual int
  AppendVISALscUntypedLoad(LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
                           VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
                           LSC_CACHE_OPTS cacheOpts, bool ov,
                           LSC_ADDR addr, LSC_DATA_SHAPE data,
                           VISA_VectorOpnd *surface, unsigned surfaceIndex,
                           VISA_RawOpnd *dstData, VISA_RawOpnd *src0Addr) = 0;
  /// append an *untyped* LSC store operation
  VISA_BUILDER_API virtual int AppendVISALscUntypedStore(
      LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_ADDR addr, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *src0Addr, VISA_RawOpnd *src1Data) = 0;
  /// append an *untyped* LSC atomic operation
  //
  /// atomic unary operations (e.g. lsc_atomic_iinc) take no extra data
  /// parameters and can be passed nullptr; atomic binary operations
  /// (e.g. lsc_atomic_add) takes src1AtomOpnd1 and leaves src2 nullptr.
  /// Atomic ternary (lsc_atomic_{i,f}cas) takes both extra src parameters.
  VISA_BUILDER_API virtual int AppendVISALscUntypedAtomic(
      LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_ADDR addr, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstReadBack, VISA_RawOpnd *src0Addr,
      VISA_RawOpnd *src1AtomOpnd1 = nullptr,
      VISA_RawOpnd *src2AtomOpnd2 = nullptr) = 0;

  //
  /// A generic constructor for untyped LSC operations.  Prefer the explicit
  /// functions above when possible.  The SFID passed in must not be LSC_TGM.
  /// (use AppendVISALscTyped* for typed).
  VISA_BUILDER_API virtual int
  AppendVISALscUntypedInst(LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
                           VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
                           LSC_CACHE_OPTS cacheOpts, bool ov, LSC_ADDR addr,
                           LSC_DATA_SHAPE data,
                           VISA_VectorOpnd *surface, unsigned surfaceIndex,
                           VISA_RawOpnd *dst, VISA_RawOpnd *src0,
                           VISA_RawOpnd *src1, VISA_RawOpnd *src2) = 0;

  /// A generic constructor for lsc_load_strided and lsc_store_strided.
  ///
  VISA_BUILDER_API virtual int AppendVISALscUntypedStridedInst(
      LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_ADDR addrInfo, LSC_DATA_SHAPE dataShape,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstData, VISA_RawOpnd *src0AddrBase,
      VISA_VectorOpnd *src0AddrPitch, VISA_RawOpnd *src1Data) = 0;
  /// A generic constructor for lsc_load_block2d and lsc_store_block2d.
  ///
  VISA_BUILDER_API virtual int AppendVISALscUntypedBlock2DInst(
      LSC_OP subOpcode, LSC_SFID lscSfid, VISA_PredOpnd *pred,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_DATA_SHAPE_BLOCK2D dataShape, VISA_RawOpnd *dstData,
      VISA_VectorOpnd *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS],
      int xImmOffset, int yImmOffset, VISA_RawOpnd *src1Data) = 0;
  VISA_BUILDER_API virtual int AppendVISALscUntypedBlock2DInst(
      LSC_OP subOpcode, VISA_PredOpnd *pred,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_DATA_SHAPE_BLOCK2D dataShape, VISA_RawOpnd *dstData,
      VISA_VectorOpnd *src0AddrPayload,
      int xImmOffset, int yImmOffset, VISA_RawOpnd *src1Data) = 0;
  ///////////////////////////////////////////////////////////////////////////
  // LSC typed operations
  //
  //
  /// append a *typed* LSC load to this kernel
  VISA_BUILDER_API virtual int AppendVISALscTypedLoad(
      LSC_OP subOpcode, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstData,
      VISA_RawOpnd *Us, int uOffset,
      VISA_RawOpnd *Vs, int vOffset,
      VISA_RawOpnd *Rs, int rOffset,
      VISA_RawOpnd *LODs) = 0;
  /// append a *typed* LSC store to this kernel
  VISA_BUILDER_API virtual int AppendVISALscTypedStore(
      LSC_OP subOpcode, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *Us, int uOffset,
      VISA_RawOpnd *Vs, int vOffset,
      VISA_RawOpnd *Rs, int rOffset,
      VISA_RawOpnd *LODs,
      VISA_RawOpnd *src1Data) = 0;
  /// append a *typed* LSC atomic to this kernel; src1 and src2 may be
  /// nullptr ternary atomic ops (e.g. atomic_icas/atomic_fcas take both
  /// extra parameters, binary (e.g. atomic_add) takes only src1 and unary
  /// (e.g. atomic_inc) takes neither.
  VISA_BUILDER_API virtual int AppendVISALscTypedAtomic(
      LSC_OP subOpcode, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrModel,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstReadBack,
      VISA_RawOpnd *Us, int uOffset,
      VISA_RawOpnd *Vs, int vOffset,
      VISA_RawOpnd *Rs, int rOffset,
      VISA_RawOpnd *coord3,
      VISA_RawOpnd *src1AtomicOpnd1, VISA_RawOpnd *src2AtomicOpnd2) = 0;
  VISA_BUILDER_API virtual int AppendVISALscTypedInst(
      LSC_OP subOpcode, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dst,
      VISA_RawOpnd *coord0s, int coord0Offset,
      VISA_RawOpnd *coord1s, int coord1Offset,
      VISA_RawOpnd *coord2s, int coord2Offset,
      VISA_RawOpnd *features,
      VISA_RawOpnd *src1, VISA_RawOpnd *src2) = 0;
  ///////////////////////////////////////////////////////////////////////////
  // LSC fences
  //
  /// append a fence instruction to this kernel; the SFID passed in
  /// controls which unit the fence applies to
  VISA_BUILDER_API virtual int AppendVISALscFence(LSC_SFID lscSfid,
                                                  LSC_FENCE_OP fenceOp,
                                                  LSC_SCOPE scope) = 0;


  VISA_BUILDER_API virtual int AppendVISALscTypedBlock2DInst(
      LSC_OP subOpcode, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrModel,
      LSC_DATA_SHAPE_TYPED_BLOCK2D dataShape,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstData, VISA_VectorOpnd *blockStartX,
      VISA_VectorOpnd *blockStartY, int xImmOffset, int yImmOffset,
      VISA_RawOpnd *src1Data) = 0;

  VISA_BUILDER_API virtual int AppendVISALscUntypedAppendCounterAtomicInst(
      LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
      LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dst, VISA_RawOpnd *src1Data) = 0;

  ///////////////////////////////////////////////////////////////////////////
  // Named barriers functions
  //
  VISA_BUILDER_API virtual int
  AppendVISANamedBarrierWait(VISA_VectorOpnd *barrierId) = 0;

  VISA_BUILDER_API virtual int
  AppendVISANamedBarrierSignal(VISA_VectorOpnd *barrierId,
                               VISA_VectorOpnd *barrierCount) = 0;

  VISA_BUILDER_API virtual int AppendVISANamedBarrierSignal(
      VISA_VectorOpnd *barrierId, VISA_VectorOpnd *barrierType,
      VISA_VectorOpnd *numProducers, VISA_VectorOpnd *numConsumers) = 0;

  /// FIXME: we should probably have separate API for logic and shift
  /// instructions, as the arguments they expect are quite different
  /// AppendVISALogicOrShiftInst -- append a two-dst, two-source vISA arithmetic
  /// instruction to this kernel [pred] op[.sat] (emask, execSize) dst src0 src1
  /// [src2 src3] src2 is used only for bfe and bfi src3 is used only for bfi
  VISA_BUILDER_API virtual int AppendVISALogicOrShiftInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_VectorOpnd *dst,
      VISA_VectorOpnd *src0, VISA_VectorOpnd *src1,
      VISA_VectorOpnd *src2 = NULL, VISA_VectorOpnd *src3 = NULL) = 0;

  /// AppendVISALogicOrShiftInst
  /// Used to perform logic operations on predicates.
  /// The operand is constructed internally.
  /// Enforces none matching types
  VISA_BUILDER_API virtual int
  AppendVISALogicOrShiftInst(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                             VISA_Exec_Size executionSize, VISA_PredVar *dst,
                             VISA_PredVar *src0, VISA_PredVar *src1) = 0;
  /// FIXME: why not use the addressof_opnd if we had introduced it earlier?
  /// AppendVISAAddrAddInst -- append an address add instruction to this kernel
  /// addr_add (emask, execSize) dst src0 src1
  VISA_BUILDER_API virtual int
  AppendVISAAddrAddInst(VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                        VISA_VectorOpnd *dst, VISA_VectorOpnd *src0,
                        VISA_VectorOpnd *src1) = 0;

  /// AppendVISABreakpointInst -- append breakpoint intrinsic to this kernel
  VISA_BUILDER_API virtual int AppendVISABreakpointInst() = 0;

  /// AppendVISADataMovementInst -- append a one-source data movement
  /// instruction to this kernel [pred] op[.sat] (emask, execSize) dst src0
  VISA_BUILDER_API virtual int
  AppendVISADataMovementInst(ISA_Opcode opcode, VISA_PredOpnd *pred,
                             bool satMod, VISA_EMask_Ctrl emask,
                             VISA_Exec_Size executionSize, VISA_VectorOpnd *dst,
                             VISA_VectorOpnd *src0) = 0;

  /// AppendVISADataMovementInst -- append a two-source data movement
  /// instruction to this kernel [pred] op[.sat] (emask, execSize) dst src0 src1
  VISA_BUILDER_API virtual int
  AppendVISADataMovementInst(ISA_Opcode opcode, VISA_PredOpnd *pred,
                             bool satMod, VISA_EMask_Ctrl emask,
                             VISA_Exec_Size executionSize, VISA_VectorOpnd *dst,
                             VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) = 0;

  /// AppendVISAPredicateMove
  /// Moves the context of the Predicate in to a Vector Operand.
  /// Predicate operand is constructed internally
  VISA_BUILDER_API virtual int AppendVISAPredicateMove(VISA_VectorOpnd *dst,
                                                       VISA_PredVar *src0) = 0;

  /// AppendVISASetP
  /// Moves the content from the vector operand back in to Predicate operand.
  /// Destination Predicate operand is constructed internally.
  VISA_BUILDER_API virtual int AppendVISASetP(VISA_EMask_Ctrl emask,
                                              VISA_Exec_Size executionSize,
                                              VISA_PredVar *dst,
                                              VISA_VectorOpnd *src0) = 0;

  /// AppendVISAMinMaxInst -- append a two-source data movement instruction to
  /// this kernel min/max[.sat] (emask, execSize) dst src0 src1
  VISA_BUILDER_API virtual int
  AppendVISAMinMaxInst(CISA_MIN_MAX_SUB_OPCODE subOpcode, bool satMod,
                       VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                       VISA_VectorOpnd *dst, VISA_VectorOpnd *src0,
                       VISA_VectorOpnd *src1) = 0;

  /// AppendVISAComparisonInst -- append a two-source comparison instruction to
  /// this kernel
  ///                             Destination is a flag register.
  /// cmp.sub_op.f# (emask, execSize) nullDst src0 src1
  VISA_BUILDER_API virtual int
  AppendVISAComparisonInst(VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask,
                           VISA_Exec_Size executionSize, VISA_PredVar *dst,
                           VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) = 0;

  /// AppendVISAComparisonInst -- append a two-source comparison instruction to
  /// this kernel.
  ///                             Destination is a GRF register.
  ///                             Flag register is updated but live range is
  ///                             only this instruction.
  /// cmp.sub_op.f# (emask, execSize) dst src0 src1
  VISA_BUILDER_API virtual int
  AppendVISAComparisonInst(VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask,
                           VISA_Exec_Size executionSize, VISA_VectorOpnd *dst,
                           VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) = 0;

  /// AppendVISACFGotoInst -- append a possibly divergent goto instruction to
  /// this kernel [pred] goto (emask, execSize) label
  VISA_BUILDER_API virtual int
  AppendVISACFGotoInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                       VISA_Exec_Size executionSize, VISA_LabelOpnd *label) = 0;

  /// AppendVISACFLabelInst -- append a label instruction to this kernel
  /// label:
  VISA_BUILDER_API virtual int AppendVISACFLabelInst(VISA_LabelOpnd *label) = 0;

  /// AppendVISACFJmpInst -- append a scalar jmp instruction to this kernel
  /// [pred] jmp (NoMask, 1) label
  VISA_BUILDER_API virtual int AppendVISACFJmpInst(VISA_PredOpnd *pred,
                                                   VISA_LabelOpnd *label) = 0;

  /// AppendVISACFCallInst -- append a subroutine call instruction to this
  /// kernel [pred] call (emask, execSize) label
  VISA_BUILDER_API virtual int
  AppendVISACFCallInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                       VISA_Exec_Size executionSize, VISA_LabelOpnd *label) = 0;

  /// AppendVISACFRetInst -- append a subroutine return instruction to this
  /// kernel [pred] ret (emask, execSize)
  VISA_BUILDER_API virtual int
  AppendVISACFRetInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                      VISA_Exec_Size executionSize) = 0;

  /// AppendVISACFFunctionCallInst -- append a function call instruction to this
  /// kernel [pred] fcall (emask, execSize) function argSize must be [0,
  /// sizeof(Arg)] returnSize must be [0, sizeof(RetVal)]
  VISA_BUILDER_API virtual int
  AppendVISACFFunctionCallInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                               VISA_Exec_Size executionSize,
                               const std::string& funcName, unsigned char argSize,
                               unsigned char returnSize) = 0;

  /// AppendVISACFIndirectFuncCallInst -- append an indirect function call to
  /// this kernel [pred] fcall (emask, execSize) funcAddr funcAddr is a 32-bit
  /// int value representing the logical address of the function argSize must be
  /// [0, sizeof(Arg)] returnSize must be [0, sizeof(RetVal)]
  VISA_BUILDER_API virtual int AppendVISACFIndirectFuncCallInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      bool isUniform, VISA_VectorOpnd *funcAddr, unsigned char argSize,
      unsigned char returnSize) = 0;

  /// AppendVISACFFuncAddrInst -- stores the address of a symbol <symbolName>
  /// into <dst> faddr symbolName dst symbolName is the unique string to
  /// identify the symbol whose address is taken dst must have UD type with
  /// scalar region
  VISA_BUILDER_API virtual int AppendVISACFSymbolInst(const std::string& symbolName,
                                                      VISA_VectorOpnd *dst) = 0;

  /// AppendVISACFFunctionRetInst -- append a function return instruction to
  /// this kernel [pred] fret (emask, execSize)
  VISA_BUILDER_API virtual int
  AppendVISACFFunctionRetInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                              VISA_Exec_Size executionSize) = 0;

  /// AppendVISACFSwitchJmpInst -- append a switch jump instruction to this
  /// kernel switchjmp (NoMask, 1) index (label0, label1, ... labelN-1)
  /// labelCount must be in [1,32]
  VISA_BUILDER_API virtual int
  AppendVISACFSwitchJMPInst(VISA_VectorOpnd *index, unsigned char labelCount,
                            VISA_LabelOpnd **labels) = 0;

  /// AppendVISASurfAccessDWordAtomicInst -- append a dword atomic write
  /// instruction to this kernel NOTE: offsets are both in unit of BYTE!
  VISA_BUILDER_API virtual int AppendVISASurfAccessDwordAtomicInst(
      VISA_PredOpnd *pred, VISAAtomicOps subOpc, bool is16Bit,
      VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *offsets, VISA_RawOpnd *src0,
      VISA_RawOpnd *src1, VISA_RawOpnd *dst) = 0;

  /// FIXME: why not have separate functions for them?
  /// AppendVISASurfAccessGatherScatterInst -- append gather/scatter instruction
  /// to this kernel globalOffset and elementOffset are both in unit of element
  /// size
  VISA_BUILDER_API virtual int AppendVISASurfAccessGatherScatterInst(
      ISA_Opcode opcode, VISA_EMask_Ctrl emask,
      GATHER_SCATTER_ELEMENT_SIZE elementSize, VISA_Exec_Size executionSize,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
      VISA_RawOpnd *elementOffset, VISA_RawOpnd *srcDst) = 0;

  /// AppendVISASurfAccessGather4Scatter4TypedInst -- append a typed dword
  /// gather4/scatter4 instruction to this kernel uOffset, vOffset, and rOffset
  /// are all in unit of pixels. vOffset should be set to V0 for 1D surfaces
  /// rOffset should be set to V0 for 1D and 2D surfaces
  VISA_BUILDER_API virtual int AppendVISASurfAccessGather4Scatter4TypedInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISAChannelMask chMask,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *uOffset,
      VISA_RawOpnd *vOffset, VISA_RawOpnd *rOffset, VISA_RawOpnd *lod,
      VISA_RawOpnd *dst) = 0;

  /// AppendVISASurfAccessGather4Scatter4ScaledInst
  /// --append a dword gather4/scatter4 instruction to this kernel
  /// @globalOffset
  /// @offsets
  /// @dstSrc
  VISA_BUILDER_API virtual int AppendVISASurfAccessGather4Scatter4ScaledInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
      VISA_Exec_Size execSize, VISAChannelMask chMask,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
      VISA_RawOpnd *offsets, VISA_RawOpnd *dstSrc) = 0;

  /// AppendVISASurfAccessScatterScaledInst
  /// --append a (1/2/4) byte gather/scatter instruction to this kernel
  /// @globalOffset
  /// @offsets
  /// @dstSrc
  VISA_BUILDER_API virtual int AppendVISASurfAccessScatterScaledInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
      VISA_Exec_Size execSize, VISA_SVM_Block_Num numBlocks,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
      VISA_RawOpnd *offsets, VISA_RawOpnd *dstSrc) = 0;

  /// AppendVISASurfAccessMediaLoadStoreInst -- append a media block load/store
  /// instruction to this kernel blockWdith, blockHeight, and xOffset are in
  /// unit of bytes yOffset is in number of rows
  VISA_BUILDER_API virtual int AppendVISASurfAccessMediaLoadStoreInst(
      ISA_Opcode opcode, MEDIA_LD_mod modifier, VISA_StateOpndHandle *surface,
      unsigned char blockWidth, unsigned char blockHeight,
      VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset, VISA_RawOpnd *srcDst,
      CISA_PLANE_ID plane = CISA_PLANE_Y) = 0;

  /// AppendVISASurfAccessOWordLoadStoreInst -- append an oword load/store
  /// instruction to this kernel oword_load_aligned: offset is in unit of owords
  /// (16 bytes) oword_load_unaligned: offset is in unit of bytes, but must be
  /// dword aligned
  VISA_BUILDER_API virtual int AppendVISASurfAccessOwordLoadStoreInst(
      ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface,
      VISA_Oword_Num size, VISA_VectorOpnd *offset, VISA_RawOpnd *srcDst) = 0;

  /// AppendVISASvmBlockStoreInst -- append an A64 oword store instruction to
  /// this kernel address is in unit of owords (16 bytes) unaligned must be
  /// false
  VISA_BUILDER_API virtual int
  AppendVISASvmBlockStoreInst(VISA_Oword_Num size, bool unaligned,
                              VISA_VectorOpnd *address, VISA_RawOpnd *src) = 0;

  /// AppendVISASvmBlockLoadInst -- append an A64 oword load instruction to this
  /// kernel address is in unit of owords (16 bytes)
  VISA_BUILDER_API virtual int
  AppendVISASvmBlockLoadInst(VISA_Oword_Num size, bool unaligned,
                             VISA_VectorOpnd *address, VISA_RawOpnd *dst) = 0;

  /// AppendVISASvmScatterInst -- append an A64 byte scattered write instruction
  /// to this kernel address is in unit of bytes
  VISA_BUILDER_API virtual int AppendVISASvmScatterInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
      VISA_RawOpnd *address, VISA_RawOpnd *src) = 0;

  /// AppendVISASvmGatherInst -- append an A64 byte scattered read instruction
  /// to this kernel address is in unit of bytes
  VISA_BUILDER_API virtual int AppendVISASvmGatherInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
      VISA_RawOpnd *address, VISA_RawOpnd *dst) = 0;

  /// AppendVISASvmAtomicInst -- append an A64 untyped atomic integer
  /// instruction to this kernel address is in unit of bytes
  VISA_BUILDER_API virtual int AppendVISASvmAtomicInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISAAtomicOps op, unsigned short bitwidth, VISA_RawOpnd *addresses,
      VISA_RawOpnd *src0, VISA_RawOpnd *src1, VISA_RawOpnd *dst) = 0;

  /// AppendVISASvmGather4ScaledInst -- append an A64 byte scaled read
  /// instruction to this kernel address is in unit of bytes
  VISA_BUILDER_API virtual int AppendVISASvmGather4ScaledInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
      VISAChannelMask channelMask, VISA_VectorOpnd *address,
      VISA_RawOpnd *offsets, VISA_RawOpnd *dst) = 0;

  /// AppendVISASvmScatter4ScaledInst -- append an A64 byte scaled write
  /// instruction to this kernel address is in unit of bytes
  VISA_BUILDER_API virtual int AppendVISASvmScatter4ScaledInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
      VISAChannelMask channelMask, VISA_VectorOpnd *address,
      VISA_RawOpnd *offsets, VISA_RawOpnd *src) = 0;

  VISA_BUILDER_API virtual int
  AppendVISASILoad(VISA_StateOpndHandle *surface, VISAChannelMask channel,
                   bool isSIMD16, VISA_RawOpnd *uOffset, VISA_RawOpnd *vOffset,
                   VISA_RawOpnd *rOffset, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISASISample(VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface,
                     VISA_StateOpndHandle *sampler, VISAChannelMask channel,
                     bool isSIMD16, VISA_RawOpnd *uOffset,
                     VISA_RawOpnd *vOffset, VISA_RawOpnd *rOffset,
                     VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISASISampleUnorm(VISA_StateOpndHandle *surface,
                          VISA_StateOpndHandle *sampler,
                          VISAChannelMask channel, VISA_VectorOpnd *uOffset,
                          VISA_VectorOpnd *vOffset, VISA_VectorOpnd *deltaU,
                          VISA_VectorOpnd *deltaV, VISA_RawOpnd *dst,
                          CHANNEL_OUTPUT_FORMAT out) = 0;

  /// AppendVISASyncInst -- appends one of barrier, fence, yield
  /// The optional argument is applicable only to the fence instruction
  VISA_BUILDER_API virtual int AppendVISASyncInst(ISA_Opcode opcode,
                                                  unsigned char mask = 0) = 0;

  /// AppendVISASyncInst -- create a sendc fence instruction to enforce thread
  /// dependencies The mask, if not NULL, can be used to selectively disable
  /// waiting on some of the parent threads
  VISA_BUILDER_API virtual int AppendVISAWaitInst(VISA_VectorOpnd *mask) = 0;

  /// AppendVISASplitBarrierInst -- create a split-phase barrier
  /// isSignal indicates whether this is the signal or wait for the barrier
  VISA_BUILDER_API virtual int AppendVISASplitBarrierInst(bool isSignal) = 0;

  VISA_BUILDER_API virtual int AppendVISAMiscFileInst(const char *fileName) = 0;

  VISA_BUILDER_API virtual int AppendVISAMiscLOC(unsigned int lineNumber) = 0;

  VISA_BUILDER_API virtual int AppendVISADebugLinePlaceholder() = 0;

  /// AppendVISAMiscRawSend -- create a GEN send instruction
  /// [pred] send/sendc (esize) <dst> <src> <exMsgDesc> <desc> {emask}
  /// bit 0 of modifiers controls whether it's send (0) or sendc (1)
  VISA_BUILDER_API virtual int
  AppendVISAMiscRawSend(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                        VISA_Exec_Size executionSize, unsigned char modifiers,
                        unsigned int exMsgDesc, unsigned char srcSize,
                        unsigned char dstSize, VISA_VectorOpnd *desc,
                        VISA_RawOpnd *src, VISA_RawOpnd *dst) = 0;

  /// AppendVISAMiscRawSend -- create a GEN split send instruction
  /// [pred] sends/sendsc (esize) <dst> <src0> <src1> <exMsgDesc> <desc> {emask}
  /// bit 0 of modifiers controls whether it's sends (0) or sendsc (1)
  //  bit 1 of modifiers has EOT flag for raw sends
  VISA_BUILDER_API virtual int AppendVISAMiscRawSends(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      unsigned char modifiers, unsigned ffid, VISA_VectorOpnd *exMsgDesc,
      unsigned char src0Size, unsigned char src1Size, unsigned char dstSize,
      VISA_VectorOpnd *desc, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
      VISA_RawOpnd *dst, bool hasEOT) = 0;

  VISA_BUILDER_API virtual int AppendVISALifetime(VISAVarLifetime startOrEnd,
                                                  VISA_VectorOpnd *varId) = 0;

  /********** APPEND MEDIA Instructions START ******************/

  VISA_BUILDER_API virtual int
  AppendVISAMiscVME_FBR(VISA_StateOpndHandle *surface, VISA_RawOpnd *UNIInput,
                        VISA_RawOpnd *FBRInput, VISA_VectorOpnd *FBRMbMode,
                        VISA_VectorOpnd *FBRSubMbShape,
                        VISA_VectorOpnd *FBRSubPredMode,
                        VISA_RawOpnd *output) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAMiscVME_IME(VISA_StateOpndHandle *surface, unsigned char streamMode,
                        unsigned char searchControlMode, VISA_RawOpnd *UNIInput,
                        VISA_RawOpnd *IMEInput, VISA_RawOpnd *ref0,
                        VISA_RawOpnd *ref1, VISA_RawOpnd *costCenter,
                        VISA_RawOpnd *output) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAMiscVME_SIC(VISA_StateOpndHandle *surface, VISA_RawOpnd *UNIInput,
                        VISA_RawOpnd *SICInput, VISA_RawOpnd *output) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAMiscVME_IDM(VISA_StateOpndHandle *surface, VISA_RawOpnd *UNIInput,
                        VISA_RawOpnd *IDMInput, VISA_RawOpnd *output) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAMEAVS(VISA_StateOpndHandle *surface, VISA_StateOpndHandle *sampler,
                  VISAChannelMask channel, VISA_VectorOpnd *uOffset,
                  VISA_VectorOpnd *vOffset, VISA_VectorOpnd *deltaU,
                  VISA_VectorOpnd *deltaV, VISA_VectorOpnd *u2d,
                  VISA_VectorOpnd *v2d, VISA_VectorOpnd *groupID,
                  VISA_VectorOpnd *verticalBlockNumber,
                  OutputFormatControl cntrl, AVSExecMode execMode,
                  VISA_VectorOpnd *iefBypass, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVABooleanCentroid(VISA_StateOpndHandle *surface,
                              VISA_VectorOpnd *uOffset,
                              VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vSize,
                              VISA_VectorOpnd *hSize, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVACentroid(VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
                       VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vSize,
                       VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVAConvolve(VISA_StateOpndHandle *sampler,
                       VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
                       VISA_VectorOpnd *vOffset, CONVExecMode execMode,
                       bool isBigKernel, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVAErodeDilate(EDMode subOp, VISA_StateOpndHandle *sampler,
                          VISA_StateOpndHandle *surface,
                          VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
                          EDExecMode execMode, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAMinMax(VISA_StateOpndHandle *surface,
                                                  VISA_VectorOpnd *uOffset,
                                                  VISA_VectorOpnd *vOffset,
                                                  VISA_VectorOpnd *mmMode,
                                                  VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVAMinMaxFilter(VISA_StateOpndHandle *sampler,
                           VISA_StateOpndHandle *surface,
                           VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
                           OutputFormatControl cntrl, MMFExecMode execMode,
                           VISA_VectorOpnd *mmfMode, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int AppendVISAVACorrelationSearch(
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vOrigin,
      VISA_VectorOpnd *hOrigin, VISA_VectorOpnd *xDirectionSize,
      VISA_VectorOpnd *yDirectionSize, VISA_VectorOpnd *xDirectionSearchSize,
      VISA_VectorOpnd *yDirectionSearchSize, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVAFloodFill(bool is8Connect, VISA_RawOpnd *pixelMaskHDirection,
                        VISA_VectorOpnd *pixelMaskVDirectionLeft,
                        VISA_VectorOpnd *pixelMaskVDirectionRight,
                        VISA_VectorOpnd *loopCount, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVALBPCorrelation(VISA_StateOpndHandle *surface,
                             VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
                             VISA_VectorOpnd *disparity, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int
  AppendVISAVALBPCreation(VISA_StateOpndHandle *surface,
                          VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
                          LBPCreationMode mode, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAConvolve1D(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset, CONVExecMode mode,
      Convolve1DDirection direction, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAConvolve1Pixel(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      CONV1PixelExecMode mode, VISA_RawOpnd *offsets, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAHDCConvolve(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, CONVHDCRegionSize regionSize,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAHDCErodeDilate(
      EDMode subOp, VISA_StateOpndHandle *sampler,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, VISA_StateOpndHandle *dstSurface,
      VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAHDCMinMaxFilter(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, MMFEnableMode mmfMode,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAHDCLBPCorrelation(
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, VISA_VectorOpnd *disparity,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAHDCLBPCreation(
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, LBPCreationMode mode,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAHDCConvolve1D(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, Convolve1DDirection direction,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) = 0;

  VISA_BUILDER_API virtual int AppendVISAVAHDCConvolve1Pixel(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, VISA_RawOpnd *offsets,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) = 0;

  /********** APPEND MEDIA Instructions END ******************/

  /********** APPEND 3D Instructions START ******************/
  /// AppendVISA3dSampler -- create a vISA sampler message
  /// @subOpcode the sampler op (sample, sample_c, sample_d, etc.)
  /// @pixelNullMask whether an extra GRF is returned for the null mask bit
  /// @cpsEnable whether CPS LOD compensation is enabled
  /// @uniformSampler whether the sampler state is uniform across all work items
  /// @pred predicate variable
  /// @emask execution mask
  /// @executionSize (8, 16)
  /// @channelMask the RGBA channel mask for the return value
  /// @aoffimmi Dx10 _aoffimmi modifier. [0:3] - R offset, [4:7] - V offset,
  /// [8:11] - U offset
  /// @sampler sampler index
  /// @surface surface index
  /// @pairedSurface paired surface index
  /// @dst return value.  Type may be either 32-bit or 16-bit
  /// @numMsgSepcificOpnds.  Number of payload arguments
  /// @opndArray.  payload arguments.  All operands must have the same type,
  /// which is either F or HF
  VISA_BUILDER_API virtual int AppendVISA3dSampler(
      VISASampler3DSubOpCode subOpcode, bool pixelNullMask, bool cpsEnable,
      bool uniformSampler, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
      VISA_Exec_Size executionSize, VISAChannelMask channelMask,
      VISA_VectorOpnd *aoffimmi, VISA_StateOpndHandle *sampler,
      VISA_StateOpndHandle *surface,
      VISA_RawOpnd *pairedSurface,
      VISA_RawOpnd *dst, int numMsgSpecificOpnds, VISA_RawOpnd **opndArray) = 0;

  VISA_BUILDER_API virtual int AppendVISA3dSampler(
      VISASampler3DSubOpCode subOpcode, bool pixelNullMask, bool cpsEnable,
      bool uniformSampler, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
      VISA_Exec_Size executionSize, VISAChannelMask channelMask,
      VISA_VectorOpnd *aoffimmi, VISA_StateOpndHandle *sampler, unsigned int samplerIdx,
      VISA_StateOpndHandle *surface, unsigned int surfaceIdx,
      VISA_RawOpnd *pairedSurface,
      VISA_RawOpnd *dst, int numMsgSpecificOpnds, VISA_RawOpnd **opndArray) = 0;

  /// AppendVISA3dLoad -- create a vISA sampler load message
  /// @subOpcode the sampler op (ld, ld_lz, etc.)
  /// @pixelNullMask whether an extra GRF is returned for the null mask bit
  /// @pred predicate variable
  /// @emask execution mask
  /// @executionSize (8, 16)
  /// @channelMask the RGBA channel mask for the return value
  /// @aoffimmi Dx10 _aoffimmi modifier. [0:3] - R offset, [4:7] - V offset,
  /// [8:11] - U offset
  /// @surface surface index
  /// @pairedSurface paired surface index
  /// @dst return value.  Type may be either 32-bit or 16-bit
  /// @numMsgSepcificOpnds.  Number of payload arguments
  /// @opndArray.  payload arguments.  All operands must have the type UD
  VISA_BUILDER_API virtual int
  AppendVISA3dLoad(VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
                   VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                   VISA_Exec_Size executionSize, VISAChannelMask channelMask,
                   VISA_VectorOpnd *aoffimmi, VISA_StateOpndHandle *surface,
                   VISA_RawOpnd *pairedSurface,
                   VISA_RawOpnd *dst, int numMsgSpecificOpnds,
                   VISA_RawOpnd **opndArray) = 0;

  VISA_BUILDER_API virtual int
  AppendVISA3dLoad(VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
                   VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                   VISA_Exec_Size executionSize, VISAChannelMask channelMask,
                   VISA_VectorOpnd *aoffimmi,
                   VISA_StateOpndHandle *surface, unsigned int surfaceIndex,
                   VISA_RawOpnd *pairedSurface,
                   VISA_RawOpnd *dst, int numMsgSpecificOpnds,
                   VISA_RawOpnd **opndArray) = 0;
  /// AppendVISA3dGather4 -- create a vISA sampler gather4 message
  /// @subOpcode the gather4 op (gather4, gather4_c, etc.)
  /// @pixelNullMask whether an extra GRF is returned for the null mask bit
  /// @pred predicate variable
  /// @emask execution mask
  /// @executionSize (8, 16)
  /// @srcChannel one of {R,G,B,A} channel to be returned
  /// @aoffimmi Dx10 _aoffimmi modifier. [0:3] - R offset, [4:7] - V offset,
  /// [8:11] - U offset
  /// @sampler sampler index
  /// @surface surface index
  /// @pairedSurface paired surface index
  /// @dst return value.  Type may be either 32-bit or 16-bit
  /// @numMsgSepcificOpnds.  Number of payload arguments
  /// @opndArray.  payload arguments.  All operands must have the same type,
  /// which is either F or HF
  VISA_BUILDER_API virtual int AppendVISA3dGather4(
      VISASampler3DSubOpCode subOpcode, bool pixelNullMask, VISA_PredOpnd *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISASourceSingleChannel srcChannel, VISA_VectorOpnd *aoffimmi,
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_RawOpnd *pairedSurface,
      VISA_RawOpnd *dst, int numMsgSpecificOpnds, VISA_RawOpnd **opndArray) = 0;

  VISA_BUILDER_API virtual int AppendVISA3dGather4(
      VISASampler3DSubOpCode subOpcode, bool pixelNullMask, VISA_PredOpnd *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISASourceSingleChannel srcChannel, VISA_VectorOpnd *aoffimmi,
      VISA_StateOpndHandle *sampler, unsigned int samplerIndex,
      VISA_StateOpndHandle *surface, unsigned int surfaceIndex,
      VISA_RawOpnd *pairedSurface,
      VISA_RawOpnd *dst, int numMsgSpecificOpnds, VISA_RawOpnd **opndArray) = 0;

  VISA_BUILDER_API virtual int
  AppendVISA3dInfo(VISASampler3DSubOpCode subOpcode, VISA_EMask_Ctrl emask,
                   VISA_Exec_Size executionSize, VISAChannelMask srcChannel,
                   VISA_StateOpndHandle *surface, unsigned int surfaceIndex,
                   VISA_RawOpnd *lod, VISA_RawOpnd *dst) = 0;

  VISA_BUILDER_API virtual int AppendVISA3dRTWrite(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_VectorOpnd *renderTargetIndex, vISA_RT_CONTROLS cntrls,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *r1HeaderOpnd,
      VISA_VectorOpnd *sampleIndex, uint8_t numMsgSpecificOpnds,
      VISA_RawOpnd **opndArray) = 0;

  VISA_BUILDER_API virtual int AppendVISA3dRTWriteCPS(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_VectorOpnd *renderTargetIndex, vISA_RT_CONTROLS cntrls,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *r1HeaderOpnd,
      VISA_VectorOpnd *sampleIndex, VISA_VectorOpnd *cPSCounter,
      uint8_t numMsgSpecificOpnds, VISA_RawOpnd **opndArray
      ) = 0;

  VISA_BUILDER_API virtual int AppendVISA3dURBWrite(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      unsigned char numberOutputParams, VISA_RawOpnd *channelMask,
      unsigned short globalOffset, VISA_RawOpnd *URBHandle,
      VISA_RawOpnd *perSLotOffset, VISA_RawOpnd *vertexData) = 0;

  VISA_BUILDER_API virtual int AppendVISA3dTypedAtomic(
      VISAAtomicOps subOp, bool is16Bit, VISA_PredOpnd *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *u, VISA_RawOpnd *v,
      VISA_RawOpnd *r, VISA_RawOpnd *lod, VISA_RawOpnd *src0,
      VISA_RawOpnd *src1, VISA_RawOpnd *dst) = 0;

  /********** APPEND 3D Instructions END ******************/

  /********** APPEND INSTRUCTION APIS END   ******************/

  /// GetGenxBinary -- returns the GEN binary in <buffer> and update its size in
  /// <size> This function may only be called after Compile() is called If
  /// finalization fails, buffer will be set to null and size will be set to 0.
  /// buffer must be de-allocated using freeBlock API.
  VISA_BUILDER_API virtual int GetGenxBinary(void *&buffer,
                                             int &size) const = 0;

  /// GetJitInfo -- returns auxiliary information collected during finalization
  /// This function may only be called after Compile() is called
  /// vISA Builder is responsible for managing this memory.
  /// it will be freed when vISA builder is destroyed.
  VISA_BUILDER_API virtual int GetJitInfo(vISA::FINALIZER_INFO *&jitInfo) const = 0;
  /// GetKernelInfo -- returns metrics information about kernel
  /// This function may only be called after Compile() is called
  /// vISA Builder is responsible for managing this memory.
  /// it will be freed when vISA builder is destroyed.
  VISA_BUILDER_API virtual int
  GetKernelInfo(KERNEL_INFO *&kernelInfo) const = 0;

  /// GetErrorMessage -- returns the error message during finalization
  VISA_BUILDER_API virtual int GetErrorMessage(const char *&errorMsg) const = 0;

  /// GetGenxDebugInfo -- returns the GEN debug info binary in <buffer>
  /// and its size in <size>.
  /// This function may only be called after Compile() is called
  /// If finalization fails, buffer will be set to null and size will be set to
  /// 0. buffer must be de-allocated using freeBlock API.
  VISA_BUILDER_API virtual int GetGenxDebugInfo(void *&buffer,
                                                unsigned int &size) const = 0;

  /// GetGenRelocEntryBuffer -- allocate and return a buffer of all
  /// GenRelocEntry that are created by vISA
  VISA_BUILDER_API virtual int
  GetGenRelocEntryBuffer(void *&buffer, unsigned int &byteSize,
                         unsigned int &numEntries) = 0;

  /// GetRelocations -- add vISA created relocations into given relocation list
  /// This get the same information as GetGenRelocEntryBuffer, but in different
  /// foramt
  typedef std::vector<vISA::ZERelocEntry> RelocListType;
  VISA_BUILDER_API virtual int GetRelocations(RelocListType &relocs) = 0;

  /// SetGTPinInit -- pass igc_init_t struct instance
  /// VISA decodes this struct and enables options accordingly
  VISA_BUILDER_API virtual int SetGTPinInit(void *buffer) = 0;

  /// GetGTPinBuffer -- returns data buffer for gtpin (eg, free GRF info)
  /// scratchOffset - passed by IGC. This argument contains offset at which
  /// gtpin can store its data. IGC computes total scratch space when
  /// stack calls are used, therefore, this is passed as argument.
  VISA_BUILDER_API virtual int GetGTPinBuffer(void *&buffer, unsigned int &size,
                                              unsigned int scratchOffset) = 0;

  /// GetFreeGRFInfo -- returns free GRF information for gtpin
  VISA_BUILDER_API virtual int GetFreeGRFInfo(void *&buffer,
                                              unsigned int &size) = 0;

  /// Gets declaration id GenVar
  VISA_BUILDER_API virtual int getDeclarationID(VISA_GenVar *decl) const = 0;

  /// Gets declaration id VISA_AddrVar
  VISA_BUILDER_API virtual int getDeclarationID(VISA_AddrVar *decl) const = 0;

  /// Gets declaration id VISA_PredVar
  VISA_BUILDER_API virtual int getDeclarationID(VISA_PredVar *decl) const = 0;

  /// Gets declaration id VISA_SamplerVar
  VISA_BUILDER_API virtual int
  getDeclarationID(VISA_SamplerVar *decl) const = 0;

  /// Gets declaration id VISA_SurfaceVar
  VISA_BUILDER_API virtual int
  getDeclarationID(VISA_SurfaceVar *decl) const = 0;

  /// Gets visa instruction counter value
  VISA_BUILDER_API virtual unsigned getvIsaInstCount() const = 0;

  // Gets the VISA string format for the variable
  VISA_BUILDER_API virtual std::string getVarName(VISA_GenVar *decl) const = 0;
  VISA_BUILDER_API virtual std::string getVarName(VISA_PredVar *decl) const = 0;
  VISA_BUILDER_API virtual std::string getVarName(VISA_AddrVar *decl) const = 0;
  VISA_BUILDER_API virtual std::string
  getVarName(VISA_SurfaceVar *decl) const = 0;
  VISA_BUILDER_API virtual std::string
  getVarName(VISA_SamplerVar *decl) const = 0;

  // Gets the VISA string format for the operand
  VISA_BUILDER_API virtual std::string
  getVectorOperandName(VISA_VectorOpnd *opnd, bool showRegion) const = 0;
  VISA_BUILDER_API virtual std::string
  getPredicateOperandName(VISA_PredOpnd *opnd) const = 0;

  /// getGenSize -- Get gen binary size of this kernel/function
  VISA_BUILDER_API virtual int64_t getGenSize() const = 0;

  VISA_BUILDER_API virtual unsigned getNumRegTotal() const = 0;

  /// getVISAAsm -- Get the compiled .visaasm of the kernel.
  VISA_BUILDER_API virtual std::string getVISAAsm() const = 0;

  /// set or get current block frequency information
  VISA_BUILDER_API virtual int
  encodeBlockFrequency(uint64_t digits, int16_t scale) = 0;

  /// getKernelCost -- get cost for a kernel whose loops' counts are
  /// based on kernel input arguments.
  VISA_BUILDER_API virtual int
  getKernelCostInfo(const vISA::KernelCostInfo *&KCInfo) const = 0;
};

class VISAFunction : public VISAKernel {
public:
  /// SetFunctionInputSize -- Set only for VISAFunction objects.
  /// This value should be set to size of arg pre-defined register.
  VISA_BUILDER_API virtual int SetFunctionInputSize(unsigned int size) = 0;

  /// SetFunctionReturnSize -- Set only for VISAFunction objects.
  /// This value should be set to size of ret pre-defined register.
  VISA_BUILDER_API virtual int SetFunctionReturnSize(unsigned int size) = 0;

  /// GetFunctionId -- Get function id for a stack call function VISAFunction
  /// instance. This id is used by API client for invoking correct stack
  /// function using fcall.
  VISA_BUILDER_API virtual int GetFunctionId(unsigned int &id) const = 0;

  /// getGenOffset -- Get gen binary offset of this function
  VISA_BUILDER_API virtual int64_t getGenOffset() const = 0;

  /// getFunctionName -- Get function name.
  VISA_BUILDER_API virtual const char *getFunctionName() const = 0;
};

typedef enum {
  VISA_BUILDER_VISA,
  VISA_BUILDER_GEN,
  VISA_BUILDER_BOTH
} VISA_BUILDER_OPTION;

class VISABuilder {
public:
  VISA_BUILDER_API virtual int AddKernel(VISAKernel *&kernel,
                                         const char *kernelName) = 0;
  VISA_BUILDER_API virtual int SetPrevKernel(VISAKernel *&prevKernel) = 0;
  VISA_BUILDER_API virtual int AddFunction(VISAFunction *&function,
                                           const char *functionName) = 0;
  VISA_BUILDER_API virtual int AddPayloadSection(VISAFunction *&function,
                                                 const char *functionName) = 0;
  VISA_BUILDER_API virtual int Compile(const char *isaasmFileName,
                                       bool emit_visa_only = false) = 0;

  VISA_BUILDER_API virtual int GetuInt32Option(vISAOptions option) = 0;
  VISA_BUILDER_API virtual void SetOption(vISAOptions option, bool val) = 0;
  VISA_BUILDER_API virtual void SetOption(vISAOptions option, uint32_t val) = 0;
  VISA_BUILDER_API virtual void SetOption(vISAOptions option,
                                          const char *val) = 0;

  VISA_BUILDER_API virtual void SetDirectCallFunctionSet(
      const std::unordered_set<std::string> &directCallFunctions) = 0;

  // For inline asm code generation
  VISA_BUILDER_API virtual int
  ParseVISAText(const std::string &visaText,
                const std::string &visaTextFile) = 0;
  VISA_BUILDER_API virtual int ParseVISAText(const std::string &visaFile) = 0;
  VISA_BUILDER_API virtual std::stringstream &GetAsmTextStream() = 0;
  VISA_BUILDER_API virtual VISAKernel *
  GetVISAKernel(const std::string &kernelName = "") const = 0;
  VISA_BUILDER_API virtual int ClearAsmTextStreams() = 0;
  VISA_BUILDER_API virtual std::string GetCriticalMsg() = 0;
};
#endif
