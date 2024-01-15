/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GENX_UTIL_H
#define GENX_UTIL_H

#include "FunctionGroup.h"
#include "GenXRegionUtils.h"
#include "GenXSubtarget.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#include "Probe/Assertion.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

namespace llvm {

class GenXLiveness;
class GenXBaling;

namespace genx {

class Bale;

// Utility function to get the integral log base 2 of an integer, or -1 if
// the input is not a power of 2.
inline int exactLog2(unsigned Val)
{
  unsigned CLZ = countLeadingZeros(Val, ZB_Width);
  if (CLZ != 32 && 1U << (31 - CLZ) == Val)
    return 31 - CLZ;
  return -1;
}

// Utility function to get the log base 2 of an integer, truncated to an
// integer, or -1 if the number is 0 or negative.
template<typename T>
inline int log2(T Val)
{
  if (Val <= 0)
    return -1;
  unsigned CLZ = countLeadingZeros((uint32_t)Val, ZB_Width);
  return 31 - CLZ;
}

// Common functionality for media ld/st lowering and CISA builder
template <typename T> inline T roundedVal(T Val, T RoundUp) {
  IGC_ASSERT_EXIT(Val >= 1);
  T RoundedVal = static_cast<T>(1) << genx::log2(Val);
  if (RoundedVal < Val)
    RoundedVal *= 2;
  if (RoundedVal < RoundUp)
    RoundedVal = RoundUp;
  return RoundedVal;
}

// createConvert : create a genx_convert intrinsic call
CallInst *createConvert(Value *In, const Twine &Name, Instruction *InsertBefore,
                        Module *M = nullptr);

// createConvertAddr : create a genx_convert_addr intrinsic call
CallInst *createConvertAddr(Value *In, int Offset, const Twine &Name,
                            Instruction *InsertBefore, Module *M = nullptr);

// createAddAddr : create a genx_add_addr intrinsic call
CallInst *createAddAddr(Value *Lhs, Value *Rhs, const Twine &Name,
                        Instruction *InsertBefore, Module *M = nullptr);

CallInst *createUnifiedRet(Type *Ty, const Twine &Name, Module *M);

// getPredicateConstantAsInt : get a vXi1 constant's value as a single integer
unsigned getPredicateConstantAsInt(const Constant *C);

// getConstantSubvector : get a contiguous region from a vector constant
Constant *getConstantSubvector(const Constant *V, unsigned StartIdx,
                               unsigned Size);

// concatConstants : concatenate two possibly vector constants, giving a vector
// constant
Constant *concatConstants(Constant *C1, Constant *C2);

// findClosestCommonDominator : find latest common dominator of some
// instructions
Instruction *findClosestCommonDominator(const DominatorTree *DT,
                                        ArrayRef<Instruction *> Insts);

// convertShlShr : convert Shl followed by AShr/LShr by the same amount into
// trunc+sext/zext
Instruction *convertShlShr(Instruction *Inst);

// splitStructPhis : find struct phi nodes and split them
//
// Return:  whether code modified
//
// Each struct phi node is split into a separate phi node for each struct
// element. This is needed because the GenX backend's liveness and coalescing
// code cannot cope with a struct phi.
//
// This is run in two places: firstly in GenXLowering, so that pass can then
// simplify any InsertElement and ExtractElement instructions added by the
// struct phi splitting. But then it needs to be run again in GenXLiveness,
// because other passes can re-insert a struct phi. The case I saw in
// hevc_speed was something commoning up the struct return from two calls in an
// if..else..endif.
//
// BTW There's also GenXAggregatePseudoLowering pass that does the same.
bool splitStructPhis(Function *F);
bool splitStructPhi(PHINode *Phi);

// Is an instruction or contant expression bitcast.
inline bool isABitCast(const Value *const V) {
  return isa<BitCastInst>(V) ||
         (isa<ConstantExpr>(V) &&
          cast<ConstantExpr>(V)->getOpcode() == CastInst::BitCast);
}

// Get the first User found after traversing possible single-user chain of
// bitcasts.
inline const User *peelBitCastsWhileSingleUserChain(const User *U) {
  while (isABitCast(U) && std::distance(U->user_begin(), U->user_end()) == 1)
    U = *U->user_begin();
  return U;
}

// Get the first User found after traversing possible single-user chain of
// bitcasts.
inline User *peelBitCastsWhileSingleUserChain(User *U) {
  return const_cast<User *>(
      peelBitCastsWhileSingleUserChain(const_cast<const User *>(U)));
}

// Get Users set ignoring possible intermediate bitcast chains.
llvm::SmallPtrSet<const User *, 4> peelBitCastsGetUsers(const Value *const V);
// Get Users set ignoring possible intermediate bitcast chains.
llvm::SmallPtrSet<User *, 4> peelBitCastsGetUsers(Value *const V);

// Get Value def ignoring possible bitcasts chain.
inline const Value *getBitCastedValue(const Value *V) {
  IGC_ASSERT_MESSAGE(V, "non-null value expected");
  while (isa<BitCastInst>(V) ||
         (isa<ConstantExpr>(V) &&
          cast<ConstantExpr>(V)->getOpcode() == CastInst::BitCast))
    V = isa<BitCastInst>(V) ? cast<Instruction>(V)->getOperand(0)
                            : cast<ConstantExpr>(V)->getOperand(0);
  return V;
}

// Get Value def ignoring possible bitcasts chain.
inline Value *getBitCastedValue(Value *V) {
  return const_cast<Value *>(getBitCastedValue(const_cast<const Value *>(V)));
}

// normalize g_load with bitcasts.
//
// When a single g_load is being bitcast'ed to different types, clone g_loads.
bool normalizeGloads(Instruction *Inst);

// fold bitcast instruction to store/load pointer operand if possible.
// Return this new instruction or nullptr.
Instruction *foldBitCastInst(Instruction *Inst);

class Bale;

bool isGlobalStore(Instruction *I);
bool isGlobalStore(StoreInst *ST);
bool isGlobalLoad(Instruction *I);
bool isGlobalLoad(LoadInst* LI);

const Value *getAVLoadSrcOrNull(const Instruction *const I,
                                const Value *const CmpSrc = nullptr);
Value *getAVLoadSrcOrNull(Instruction *const I,
                          const Value *const CmpSrc = nullptr);
const Value *getAGVLoadSrcOrNull(const Instruction *const I,
                                 const Value *const CmpSrc = nullptr);
Value *getAGVLoadSrcOrNull(Instruction *const I,
                           const Value *const CmpSrc = nullptr);
bool isAVLoad(const Instruction *const I);
bool isAVLoad(const Instruction *const I, const Value *const CmpSrc);
bool isAGVLoad(const Instruction *const I,
               const Value *const CmpGvSrc = nullptr);

const Value *getAVStoreDstOrNull(const Instruction *const I,
                                 const Value *const CmpDst = nullptr);
Value *getAVStoreDstOrNull(Instruction *const I,
                           const Value *const CmpDst = nullptr);
const Value *getAGVStoreDstOrNull(const Instruction *const I,
                                  const Value *const CmpGvDst = nullptr);
Value *getAGVStoreDstOrNull(Instruction *const I,
                            const Value *const CmpGvDst = nullptr);
bool isAVStore(const Instruction *const I);
bool isAVStore(const Instruction *const I, const Value *const CmpDst);
bool isAGVStore(const Instruction *const I,
                const Value *const CmpGvDst = nullptr);

// Check that V is correct as value for global store to StorePtr.
// This implies:
// 1) V is wrregion W;
// 2) Old value of W is result of gload L;
// 3) Pointer operand of L is derived from global variable of StorePtr.
bool isLegalValueForGlobalStore(Value *V, Value *StorePtr);

// Check that global store ST operands meet condition of
// isLegalValueForGlobalStore.
bool isGlobalStoreLegal(StoreInst *ST);

bool isIdentityBale(const Bale &B);

// Check if region of value is OK for baling in to raw operand
//
// Enter:   V = value that is possibly rdregion/wrregion
//          IsWrite = true if caller wants to see wrregion, false for rdregion
//
// The region must be constant indexed, contiguous, and start on a GRF
// boundary.
bool isValueRegionOKForRaw(Value *V, bool IsWrite, const GenXSubtarget *ST);

// Check if region is OK for baling in to raw operand
//
// The region must be constant indexed, contiguous, and start on a GRF
// boundary.
bool isRegionOKForRaw(const genx::Region &R, const GenXSubtarget *ST);

// Skip optimizations on functions with large blocks.
inline bool skipOptWithLargeBlock(const Function &F) {
  return std::any_of(F.begin(), F.end(),
                     [](const BasicBlock &BB) { return BB.size() >= 5000; });
}

bool skipOptWithLargeBlock(FunctionGroup &FG);

// getTwoAddressOperandNum : get operand number of two address operand
llvm::Optional<unsigned> getTwoAddressOperandNum(CallInst *II);

// isPredicate : test whether an instruction has predicate (i1 or vector of i1)
// type
bool isPredicate(Instruction *Inst);

// isNot : test whether an instruction is a "not" instruction (an xor with
//    constant all ones)
bool isNot(Instruction *Inst);

// isPredNot : test whether an instruction is a "not" instruction (an xor
//    with constant all ones) with predicate (i1 or vector of i1) type
bool isPredNot(Instruction *Inst);

// isIntNot : test whether an instruction is a "not" instruction (an xor
//    with constant all ones) with non-predicate type
bool isIntNot(Instruction *Inst);

// getMaskOperand : get i1 vector type of genx intrinsic, return null
//    if there is no operand of such type or for non genx intrinsic.
//    If there are multiple operands of i1 vector type then return first
//    oparand.
Value *getMaskOperand(const Instruction *Inst);

// invertCondition : Invert the given predicate value, possibly reusing
//    an existing copy.
Value *invertCondition(Value *Condition);

// if V is a function pointer return function it points to,
//    nullptr otherwise
Function *getFunctionPointerFunc(Value *V);

// return true if V is a const vector of function pointers
// considering any casts and extractelems within
bool isFuncPointerVec(Value *V);

// isNoopCast : test if cast operation doesn't modify bitwise representation
// of value (in other words, it can be copy-coalesced).
bool isNoopCast(const CastInst *CI);

// isBFloat16Cast : test if cast operation extends bfloat or truncates to bfloat
bool isBFloat16Cast(const Instruction *I);

// ShuffleVectorAnalyzer : class to analyze a shufflevector
class ShuffleVectorAnalyzer {
  ShuffleVectorInst *SI;

public:
  ShuffleVectorAnalyzer(ShuffleVectorInst *SI) : SI(SI) {}
  // getAsSlice : return start index of slice, or -1 if shufflevector is not
  //  slice
  int getAsSlice();
  // Replicated slice descriptor.
  // Replicated slice (e.g. 1 2 3 1 2 3) can be parametrized by
  // initial offset (1), slice size (3) and replication count (2).
  struct ReplicatedSlice {
    unsigned InitialOffset;
    unsigned SliceSize;
    unsigned ReplicationCount;
    ReplicatedSlice(unsigned Offset, unsigned Size, unsigned Count)
        : InitialOffset(Offset), SliceSize(Size), ReplicationCount(Count) {}
  };

  // isReplicatedSlice : check whether shufflevector is replicated slice.
  // Example of replicated slice:
  // shufflevector <3 x T> x, undef, <6 x i32> <1, 2, 1, 2, 1, 2>.
  bool isReplicatedSlice() const;

  static bool isReplicatedSlice(ShuffleVectorInst *SI) {
    return ShuffleVectorAnalyzer(SI).isReplicatedSlice();
  }

  // When we have replicated slice, its parameters are ealisy deduced
  // from first and last elements of mask. This function decomposes
  // replicated slice to its parameters.
  ReplicatedSlice getReplicatedSliceDescriptor() const {
    IGC_ASSERT_MESSAGE(isReplicatedSlice(), "Expected replicated slice");
    const unsigned TotalSize =
        cast<IGCLLVM::FixedVectorType>(SI->getType())->getNumElements();
    const unsigned SliceStart = SI->getMaskValue(0);
    const unsigned SliceEnd = SI->getMaskValue(TotalSize - 1);
    const unsigned SliceSize = SliceEnd - SliceStart + 1;
    const unsigned ReplicationCount = TotalSize / SliceSize;
    return ReplicatedSlice(SliceStart, SliceSize, ReplicationCount);
  }

  static ReplicatedSlice getReplicatedSliceDescriptor(ShuffleVectorInst *SI) {
    return ShuffleVectorAnalyzer(SI).getReplicatedSliceDescriptor();
  }

  // getAsUnslice : see if the shufflevector is an
  //     unslice where the "old value" is operand 0 and operand 1 is another
  //     shufflevector and operand 0 of that is the "new value" Returns start
  //     index, or -1 if it is not an unslice
  int getAsUnslice();
  // getAsSplat : if shufflevector is a splat, get the splatted input, with the
  //  element's vector index if the input is a vector
  struct SplatInfo {
    Value *Input;
    unsigned Index;
    SplatInfo(Value *Input, unsigned Index) : Input(Input), Index(Index) {}
  };
  SplatInfo getAsSplat();

  // Serialize this shuffulevector instruction.
  Value *serialize();

  // Compute the cost in terms of number of insertelement instructions needed.
  unsigned getSerializeCost(unsigned i);

  // To describe the region of one of two shufflevector instruction operands.
  struct OperandRegionInfo {
    Value *Op;
    Region R;
  };
  OperandRegionInfo getMaskRegionPrefix(int StartIdx);
};

// class for splitting i64 (both vector and scalar) to subregions of i32 vectors
// Used in GenxLowering and emulation routines
class IVSplitter {
  Instruction &Inst;

  Type *ETy = nullptr;
  Type *VI32Ty = nullptr;
  size_t Len = 0;

  enum class RegionType { LoRegion, HiRegion, FirstHalf, SecondHalf };

  // Description of a RegionType in terms of initial offset and stride.
  // Both ELOffset and ElStride are in elements.
  struct RegionTrait {
    size_t ElOffset = 0;
    size_t ElStride = 0;
  };

  // describeSplit: given a requested RegionType and a number of source elements
  // returns the detailed descripton of how to form such a split (in terms of
  // an initial offset and stride).
  // Example:
  //    describeSplit(SecondHalf, 10) should return RegionTrait{ 5, 1 }
  static RegionTrait describeSplit(RegionType RT, size_t ElNum);

  // splitConstantVector: given a vector of constant values create
  // a new constant vector containing only values corresponding to the
  // desired RegionType
  // Example:
  //    splitConstantVector({ 1, 2, 3, 4}, HiRegion) -> {2, 4}
  // Note: since every RegionType needs half of the original elements, the
  // size of the input vector is expected to be even.
  static Constant *splitConstantVector(const SmallVectorImpl<Constant *> &KV,
                                       RegionType RT);
  // createSplitRegion: given a type of the source vector (expected to be
  // vector of i32 with even number of elements) and the desired RegionType
  // returns genx::Region that can be used to construct an equivalent
  // rdregion intrinsic
  static genx::Region createSplitRegion(Type *SrcTy, RegionType RT);

  std::pair<Value *, Value *> splitValue(Value &Val, RegionType RT1,
                                         const Twine &Name1, RegionType RT2,
                                         const Twine &Name2,
                                         bool FoldConstants);
  Value* combineSplit(Value &V1, Value &V2, RegionType RT1, RegionType RT2,
                      const Twine& Name, bool Scalarize);

public:

  struct LoHiSplit {
    Value *Lo;
    Value *Hi;
  };
  struct HalfSplit {
    Value *Left;
    Value *Right;
  };

  // Instruction is used as an insertion point, debug location source and
  // as a source of operands to split.
  // If BaseOpIdx indexes a scalar/vector operand of i64 type, then
  // IsI64Operation shall return true, otherwise the value type of an
  // instruction is used
  IVSplitter(Instruction &Inst, const unsigned *BaseOpIdx = nullptr);

  // Splitted Operand is expected to be a scalar/vector of i64 type
  LoHiSplit splitOperandLoHi(unsigned SourceIdx, bool FoldConstants = true);
  HalfSplit splitOperandHalf(unsigned SourceIdx, bool FoldConstants = true);

  LoHiSplit splitValueLoHi(Value &V, bool FoldConstants = true);
  HalfSplit splitValueHalf(Value &V, bool FoldConstants = true);

  // Combined values are expected to be a vector of i32 of the same size
  Value *combineLoHiSplit(const LoHiSplit &Split, const Twine &Name,
                          bool Scalarize);
  Value *combineHalfSplit(const HalfSplit &Split, const Twine &Name,
                          bool Scalarize);

  // convinence method for quick sanity checking
  bool IsI64Operation() const { return ETy->isIntegerTy(64); }
};

// adjustPhiNodesForBlockRemoval : adjust phi nodes when removing a block
void adjustPhiNodesForBlockRemoval(BasicBlock *Succ, BasicBlock *BB);

/***********************************************************************
 * sinkAdd : sink add(s) in address calculation
 *
 * Enter:   IdxVal = the original index value
 *
 * Return:  the new calculation for the index value
 *
 * This detects the case when a variable index in a region or element access
 * is one or more constant add/subs then some mul/shl/truncs. It sinks
 * the add/subs into a single add after the mul/shl/truncs, so the add
 * stands a chance of being baled in as a constant offset in the region.
 *
 * If add sinking is successfully applied, it may leave now unused
 * instructions behind, which need tidying by a later dead code removal
 * pass.
 */
Value *sinkAdd(Value *V);

// Check if this is a mask packing operation, i.e. a bitcast from Vxi1 to
// integer i8, i16 or i32.
static inline bool isMaskPacking(const Value *V) {
  if (auto BC = dyn_cast<BitCastInst>(V)) {
    auto SrcTy = dyn_cast<IGCLLVM::FixedVectorType>(BC->getSrcTy());
    if (!SrcTy || !SrcTy->getScalarType()->isIntegerTy(1))
      return false;
    unsigned NElts = SrcTy->getNumElements();
    if (NElts != 8 && NElts != 16 && NElts != 32)
      return false;
    return V->getType()->getScalarType()->isIntegerTy(NElts);
  }
  return false;
}

void LayoutBlocks(Function &func, LoopInfo &LI);
void LayoutBlocks(Function &func);

// Metadata name for inline assemly instruction
constexpr const char *MD_genx_inline_asm_info = "genx.inlasm.constraints.info";

// Inline assembly avaliable constraints
enum class ConstraintType : uint32_t {
  Constraint_r,
  Constraint_rw,
  Constraint_i,
  Constraint_n,
  Constraint_F,
  Constraint_a,
  Constraint_cr,
  Constraint_unknown
};

// Represents info about inline asssembly operand
class GenXInlineAsmInfo {
  genx::ConstraintType CTy = ConstraintType::Constraint_unknown;
  int MatchingInput = -1;
  bool IsOutput = false;

public:
  GenXInlineAsmInfo(genx::ConstraintType Ty, int MatchingInput, bool IsOutput)
      : CTy(Ty), MatchingInput(MatchingInput), IsOutput(IsOutput) {}
  bool hasMatchingInput() const { return MatchingInput != -1; }
  int getMatchingInput() const { return MatchingInput; }
  bool isOutput() const { return IsOutput; }
  genx::ConstraintType getConstraintType() const { return CTy; }
};

// If input input constraint has matched output operand with same constraint
bool isInlineAsmMatchingInputConstraint(const InlineAsm::ConstraintInfo &Info);

// Get matched output operand number for input operand
unsigned getInlineAsmMatchedOperand(const InlineAsm::ConstraintInfo &Info);

// Get joined string representation of constraints
std::string getInlineAsmCodes(const InlineAsm::ConstraintInfo &Info);

// Get constraint type
genx::ConstraintType getInlineAsmConstraintType(StringRef Codes);

// Get vector of inline asm info for inline assembly instruction.
// Return empty vector if no constraint string in inline asm or
// if called before GenXInlineAsmLowering pass.
std::vector<GenXInlineAsmInfo> getGenXInlineAsmInfo(CallInst *CI);

// Get vector of inline asm info from MDNode
std::vector<GenXInlineAsmInfo> getGenXInlineAsmInfo(MDNode *MD);

bool hasConstraintOfType(const std::vector<GenXInlineAsmInfo> &ConstraintsInfo,
                         genx::ConstraintType CTy);

// Get number of outputs for inline assembly instruction
unsigned getInlineAsmNumOutputs(CallInst *CI);

Type *getCorrespondingVectorOrScalar(Type *Ty);

// Get bitmap of instruction allowed execution sizes
unsigned getExecSizeAllowedBits(const Instruction *Inst,
                                const GenXSubtarget *ST);

// VC backend natively supports half, float and double data types
bool isSupportedFloatingPointType(const Type *Ty);

// Get type that represents OldType as vector of NewScalarType, e.g.
// <4 x i16> -> <2 x i32>, returns nullptr if it's inpossible.
IGCLLVM::FixedVectorType *changeVectorType(Type *OldType,
                                           Type *NewScalarType,
                                           const DataLayout *DL);

// Check if V is reading form predfined register.
bool isPredefRegSource(const Value *V);
// Check if V is writing to predefined register.
bool isPredefRegDestination(const Value *V);

/* scalarVectorizeIfNeeded: scalarize of vectorize \p Inst if it is required
 *
 * Result of some instructions can be both Ty and <1 x Ty> value e.g. rdregion.
 * It is sometimes required to replace uses of instructions with types
 * [\p FirstType, \p LastType) with \p Inst. If types don't
 * correspond this function places BitCastInst <1 x Ty> to Ty, or Ty to <1 x Ty>
 * after \p Inst and returns the pointer to the instruction. If no cast is
 * required, nullptr is returned.
 */
template <
    typename ConstIter,
    typename std::enable_if<
        std::is_base_of<
            Type, typename std::remove_pointer<typename std::iterator_traits<
                      ConstIter>::value_type>::type>::value,
        int>::type = 0>
CastInst *scalarizeOrVectorizeIfNeeded(Instruction *Inst, ConstIter FirstType,
                                       ConstIter LastType) {
  IGC_ASSERT_MESSAGE(Inst, "wrong argument");
  IGC_ASSERT_MESSAGE(std::all_of(FirstType, LastType,
                     [Inst](Type *Ty) {
                       return Ty == Inst->getType() ||
                              Ty == getCorrespondingVectorOrScalar(
                                        Inst->getType());
                     }),
         "wrong arguments: type of instructions must correspond");

  if (Inst->getType()->isVectorTy() &&
      cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements() > 1)
    return nullptr;
  bool needBitCast = std::any_of(
      FirstType, LastType, [Inst](Type *Ty) { return Ty != Inst->getType(); });
  if (!needBitCast)
    return nullptr;
  auto *CorrespondingTy = getCorrespondingVectorOrScalar(Inst->getType());
  auto *BC = CastInst::Create(Instruction::BitCast, Inst, CorrespondingTy);
  BC->insertAfter(Inst);
  return BC;
}
/* scalarVectorizeIfNeeded: scalarize of vectorize \p Inst if it is required
 *
 * Result of some instructions can be both Ty and <1 x Ty> value e.g. rdregion.
 * It is sometimes required to replace uses of instructions of [\p
 * FirstInstToReplace, \p LastInstToReplace) with \p Inst. If types don't
 * correspond this function places BitCastInst <1 x Ty> to Ty, or Ty to <1 x Ty>
 * after \p Inst and returns the pointer to the instruction. If no cast is
 * required, nullptr is returned.
 */
template <typename ConstIter,
          typename std::enable_if<
              std::is_base_of<
                  Instruction,
                  typename std::remove_pointer<typename std::iterator_traits<
                      ConstIter>::value_type>::type>::value,
              int>::type = 0>
CastInst *scalarizeOrVectorizeIfNeeded(Instruction *Inst,
                                       ConstIter FirstInstToReplace,
                                       ConstIter LastInstToReplace) {
  std::vector<Type *> Types;
  std::transform(FirstInstToReplace, LastInstToReplace,
                 std::back_inserter(Types),
                 [](Instruction *Inst) { return Inst->getType(); });
  return scalarizeOrVectorizeIfNeeded(Inst, Types.begin(), Types.end());
}

CastInst *scalarizeOrVectorizeIfNeeded(Instruction *Inst,
                                       Instruction *InstToReplace);

// Returns log alignment for align type and target grf width, because ALIGN_GRF
// must be target-dependent.
unsigned getLogAlignment(VISA_Align Align, unsigned GRFWidth);
// The opposite of getLogAlignment.
VISA_Align getVISA_Align(unsigned LogAlignment, unsigned GRFWidth);
// Some log alignments cannot be transparently transformed to VISA_Align. This
// chooses suitable log alignment which is convertible to VISA_Align.
unsigned ceilLogAlignment(unsigned LogAlignment, unsigned GRFWidth);

// Checks whether provided wrpredregion intrinsic can be encoded
// as legal SETP instruction.
bool isWrPredRegionLegalSetP(const CallInst &WrPredRegion);

// Checks if V is a CallInst representing a direct call to F
// Many of our analyzes do not check whether a function F's user
// which is a CallInst calls exactly F. This may not be true
// when a function pointer is passed as an argument of a call to
// another function.
// Returns V casted to CallInst if the check is true,
// nullptr otherwise.
const CallInst *checkFunctionCall(const Value *V, const Function *F);
CallInst *checkFunctionCall(Value *V, const Function *F);

// Get possible number of GRFs for indirect region
unsigned getNumGRFsPerIndirectForRegion(const genx::Region &R,
                                        const GenXSubtarget *ST, bool Allow2D);
// to control behavior of emulateI64Operation function
enum class EmulationFlag {
  RAUW,
  // RAUW and EraseFromParent, always returns a valid instruction
  // either the original one or the last one from the result emulation sequence
  RAUWE,
  None,
};
// transforms operation on i64 type to an equivalent sequence that do not
// operate on i64 (but rather on i32)
// The implementation is contained in GenXEmulate pass sources
// Note: ideally, i64 emulation should be handled by GenXEmulate pass,
// however, some of our late passes like GetXPostLegalization or GenXCategory
// may introduce additional instructions which violate Subtarget restrictions -
// this function is intended to cope with such cases
Instruction *emulateI64Operation(const GenXSubtarget *ST, Instruction *In,
                                 EmulationFlag AuxAction);
// BinaryDataAccumulator: it's a helper class to accumulate binary data
// in one buffer.
// Information about each stored section can be accessed via the key with
// which it was stored. The key must be unique.
// Accumulated/consolidated binary data can be accesed.
template <typename KeyT, typename DataT = uint8_t, DataT Zero = 0u>
struct BinaryDataAccumulator final {
  struct SectionInfoT {
    int Offset = 0;
    ArrayRef<DataT> Data;

    SectionInfoT() = default;
    SectionInfoT(const DataT *BasePtr, int First, int Last)
        : Offset{First}, Data{BasePtr + First, BasePtr + Last} {}

    int getSize() const { return Data.size(); }
  };

  struct SectionT {
    KeyT Key;
    SectionInfoT Info;
  };

private:
  std::vector<DataT> Data;
  using SectionSeq = std::vector<SectionT>;
  SectionSeq Sections;

public:
  using value_type = typename SectionSeq::value_type;
  using reference = typename SectionSeq::reference;
  using const_reference = typename SectionSeq::const_reference;
  using iterator = typename SectionSeq::iterator;
  using const_iterator = typename SectionSeq::const_iterator;

  iterator begin() { return Sections.begin(); }
  const_iterator begin() const { return Sections.begin(); }
  const_iterator cbegin() const { return Sections.cbegin(); }
  iterator end() { return Sections.end(); }
  const_iterator end() const { return Sections.end(); }
  const_iterator cend() const { return Sections.cend(); }
  reference front() { return *begin(); }
  const_reference front() const { return *begin(); }
  reference back() { return *std::prev(end()); }
  const_reference back() const { return *std::prev(end()); }

  // Pad the end of the buffer with \p Size zeros.
  void pad(int Size) {
    IGC_ASSERT_MESSAGE(Size >= 0,
                       "wrong argument: size must be a non-negative number");
    std::fill_n(std::back_inserter(Data), Size, Zero);
  }

  // Align the end of the buffer to \p Alignment bytes.
  void align(int Alignment) {
    IGC_ASSERT_MESSAGE(Alignment > 0,
                       "wrong argument: alignment must be a positive number");
    if (Alignment == 1 || Data.size() == 0)
      return;
    pad(alignTo(Data.size(), Alignment) - Data.size());
  }

  // Append the data that is referenced by a \p Key and represented
  // in range [\p First, \p Last), to the buffer.
  // The range must consist of DataT elements.
  // The data is placed with alignment \p Alignment.
  template <typename InputIter>
  void append(KeyT Key, InputIter First, InputIter Last, int Alignment = 1) {
    IGC_ASSERT_MESSAGE(
        std::none_of(Sections.begin(), Sections.end(),
                     [&Key](const SectionT &S) { return S.Key == Key; }),
        "There's already a section with such key");
    IGC_ASSERT_MESSAGE(Alignment > 0,
                       "wrong argument: alignment must be a positive number");

    align(Alignment);

    SectionT Section;
    Section.Key = std::move(Key);
    int Offset = Data.size();
    std::copy(First, Last, std::back_inserter(Data));
    Section.Info =
        SectionInfoT{Data.data(), Offset, static_cast<int>(Data.size())};
    Sections.push_back(std::move(Section));
  }

  void append(KeyT Key, ArrayRef<DataT> SectionBin) {
    append(std::move(Key), SectionBin.begin(), SectionBin.end());
  }

  // Get information about the section referenced by \p Key.
  SectionInfoT getSectionInfo(const KeyT &Key) const {
    auto SectionIt =
        std::find_if(Sections.begin(), Sections.end(),
                     [&Key](const SectionT &S) { return S.Key == Key; });
    IGC_ASSERT_MESSAGE(SectionIt != Sections.end(),
                       "There must be a section with such key");
    return SectionIt->Info;
  }

  // Get offset of the section referenced by \p Key.
  int getSectionOffset(const KeyT &Key) const {
    return getSectionInfo(Key).Offset;
  }
  // Get size of the section referenced by \p Key.
  int getSectionSize(const KeyT &Key) const { return getSectionInfo(Key).Size; }
  // Get size of the whole collected data.
  int getFullSize() const { return Data.size(); }
  int getNumSections() const { return Sections.size(); }
  // Data buffer empty.
  bool empty() const { return Data.empty(); }
  // Emit the whole consolidated data.
  std::vector<DataT> emitConsolidatedData() const & { return Data; }
  std::vector<DataT> emitConsolidatedData() && { return std::move(Data); }
};

// Get size of an struct field including the size of padding for the next field,
// or the tailing padding.
// For example for the 1st element of { i8, i32 } 4 bytes will be returned
// (likely in the most of layouts).
//
// Arguments:
//    \p ElemIdx - index of a struct field
//    \p NumOperands - the number of fields in struct
//                     (StructLayout doesn't expose it)
//    \p StructLayout - struct layout
//
// Returns the size in bytes.
std::size_t getStructElementPaddedSize(unsigned ElemIdx, unsigned NumOperands,
                                       const StructLayout &Layout);

// Return true if V is rdregion from a load result.
bool isRdRWithOldValueVLoadSrc(Value *V);

// Return true if wrregion has result of load as old value.
bool isWrRWithOldValueVLoadSrc(Value *V);

//----------------------------------------------------------
// If there's a genx.vload kill on the path (if this path exists) from
// LI to PosI, return it, otherwize returns nullptr.
// + NB: In VC BE semantics genx.vload value is not translated to any VISA
// instruction, rather it signifies either a reference to an object pinned in
// the register file (if genx.vload loads from a global value with genx_volatile
// attribute) or a kernel parameter passed by reference (in which case
// genx.vload is lowered early in pipeline to simple load and later converted to
// ssa form by mem2reg).
// + NB: This function may return a call to a user function as a potential
//     intervening store. If CallSites is supplied, we'll limit our lookup to
//     the call instructions mentioned in this list.
// + NB/TBD: There may be multiple intervening stores, for now we don't have
//     a use case where we want the whole list, hence this variant
//     only returns the first one found.
//----------------------------------------------------------
// P = value pointer, L = genx.vload(P), S = genx.vstore(P), U = use(L)
// S clobbers L if there's a path from S to U not through L:
// { S -> U } != { S -> L -> U }
//----------------------------------------------------------
const Instruction *getAVLoadKillOrNull(
    const Instruction *FromVLoad, const Instruction *PosI,
    const bool ChangeSearchDirectionBasedOnDominance = false,
    bool PosProvedReachableFromVLoad = false,
    const DominatorTree *const DT = nullptr,
    const SmallPtrSet<const BasicBlock *, 2>
        *const ExcludeBlocksOnCfgTraversal = nullptr,
    const llvm::SmallVector<const Instruction *, 8> *const KillCallSites =
        nullptr);

llvm::SmallPtrSet<const Instruction *, 1> getSrcVLoads(const Instruction *I);
llvm::SmallPtrSet<Instruction *, 1> getSrcVLoads(Instruction *I);
const Instruction *getSrcVLoadOrNull(const Instruction *const I);
Instruction *getSrcVLoadOrNull(Instruction *const I);
bool hasVLoadSource(const Instruction *const I);

// genx_volatile genx.vload's users sanity check.
// + A genx_volatile genx.vload's "forbidden use" is defined as a use on IR
// instruction whos optimization can not be controlled by VC backend-owned
// codebase and can result in clobbering after standard LLVM optimizations.
// Example:
// ------ mem2reg can optimize this sequence ------
// L=genx.vload(@genx_volatile_global*)
// store(L,%simple_global.localized*)
// <... no stores into %simple_global.localized* ...>
// <... possible genx.vstore(Y, @genx_volatile_global*) ...>
// L2 = load(%simple_global.localized*)
// use(L2)
//-------- into --------------
// L=genx.vload(@genx_volatile_global*)
// <... possible genx.vstore(Y, @genx_volatile_global*) ...>
// use(L)
// ---------------------------
// Which is clobbering situation, since L is not a VISA instruction in VC BE
// semantics, but only signifies a pointer to an object pinned in the register
// file.
inline bool isAGVLoadForbiddenUser(const User *U) {
  U = genx::peelBitCastsWhileSingleUserChain(
      U); // For simplicity a bitcast having multiple users is considered
          // forbidden, but bitcasts in single-use chain are not.
  return !vc::isAnyVcIntrinsic(U) &&
         !isa<PHINode>(U); // Currently PHI nodes are handeled by GenXCoalescing
                           // by inserting phicopy
}

// Safety checks for vloads clobbering when sinking a bale of instructions.
bool isSafeToSink_CheckAVLoadKill(const Bale &B, const Instruction *const To,
                                  const DominatorTree *const DT = nullptr);
// Safety checks for vloads clobbering when sinking a bale of instructions..
bool isSafeToSink_CheckAVLoadKill(const Instruction *const I,
                                  const Instruction *const To,
                                  const GenXBaling *const Baling,
                                  const DominatorTree *const DT = nullptr);

// Safety checks for vloads clobbering when looking to hoist or sink an
// instruction.
// + NB: Is also reused by isSafeToReplace_CheckAVLoadKillOrForbiddenUser(...).
// + NB: Besides checking for a vload kill, take into account that instruction
// "I" may not dominate "To" but if it has any VLoad sources - we'll not move
// them by ourselves even if there'd be no clobbering. Hence make sure that if
// any, all of them dominate "To" position by the time this check is called.
// + NB: If const DominatorTree * is not passed we assume that both "I" and
// potential VLoad sources of "I" dominate "To".
bool isSafeToMove_CheckAVLoadKill(const Instruction *const I,
                                  const Instruction *const To,
                                  const DominatorTree *const DT);

// Safety checks for vloads clobbering when replacing an instruction.
// + NB: When used for checks on hoisting, const DominatorTree* is mandatory and
// must not be nullptr.
bool isSafeToReplace_CheckAVLoadKillOrForbiddenUser(
    const Instruction *const OldI, const Instruction *const NewI,
    const DominatorTree *const DT);

// Safety checks for genx_volatile genx.vloads clobbering when
// targeting to modify a use on a UseTarget instruction.
// + NB: UseSrc must dominate UseTarget.
// + NB: This API can be augmented to optionally avoid the "forbidden user"
// check on certain pipeline stages when we know that no further standard LLVM
// optimizations shall be run.
// + NB: This API can be augmented by returning metadata
// differentiating between "kill" and "forbidden user" cases,
// returning "forbidden user" to make further legalization on the caller side.
bool isSafeToUse_CheckAVLoadKillOrForbiddenUser(
    const Instruction *const UseSrc, const Instruction *const UseTarget,
    const DominatorTree *const DT);

// See isAGVLoadForbiddenUser(...) description.
bool legalizeGVLoadForbiddenUsers(Instruction *GVLoad);

bool vloadsReadSameValue(const Instruction *L1, const Instruction *L2,
                         const DominatorTree *const DT);

// TBD: there's another, a bit different, isBitwiseIdentical in CMABI, make it
// common.
bool isBitwiseIdentical(const Value *V1, const Value *V2,
                        const DominatorTree *const DT = nullptr);

} // namespace genx
} // namespace llvm

#endif // GENX_UTIL_H
