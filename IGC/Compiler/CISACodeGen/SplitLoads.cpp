/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/SplitLoads.h"

#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/MathExtras.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Value.h"
// clang-format on

#include "IGC/common/Types.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <optional>
#include <string>

using namespace llvm;
using namespace IGC;
using namespace IGC::LS;

#define DEBUG_TYPE "igc-split-loads"

// ============================================================================
// The goal of this feature:
// ============================================================================
//
// The file provides basic tools for splitting 2D LSC block loads of the form:
//  -- <N x iX> @llvm.genx.GenISA.LSC2DBlockRead.vNiX(i64, i32, i32, i32, i32,
//  i32, i32, i32, i32, i32, i1, i1, i32)
//
// For the load to be eligible for splitting, the loaded vector must be
// subsequently split into smaller chunks. For example, consider the load of a
// 16-element vector
//  -- %vec = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %ptr,
//  i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1
//  false, i1 false, i32 0)
//
// that is subsequently split into two 8-element vectors:
//  -- %pick.0 = shufflevector <16 x i16> %vec, <16 x i16> undef, <8 x i32> <i32
//  0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
//  -- %pick.1 = shufflevector <16 x i16> %vec, <16 x i16> undef, <8 x i32> <i32
//  8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
// that are finally fed into some users:
//  -- call void @fun_v8i16(<8 x i16> %pick.0)
//  -- call void @fun_v8i16(<8 x i16> %pick.1)
//
// This sequence can be replaced by 2 smaller loads that feed directly into the
// users:
//  -- %vec.0 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %ptr,
//  i32 127, i32 63, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1
//  false, i1 false, i32 0)
//  -- %vec.1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %ptr,
//  i32 127, i32 63, i32 127, i32 0, i32 8, i32 16, i32 16, i32 8, i32 1, i1
//  false, i1 false, i32 0)
//  -- call void @fun_v8i16(<8 x i16> %vec.0)
//  -- call void @fun_v8i16(<8 x i16> %vec.1)
//
// Whether this is beneficial or not depends on the register pressure and
// rescheduling possibilities.
//
// ============================================================================
// Outline of the tool:
// ============================================================================
//
// A given load can be split by the instance of `LoadSplitter` created by the
// factory function
//  -- static std::unique_ptr<LoadSplitter> Create(Function *inF, CodeGenContext
//  *inCGC, IGCLivenessAnalysis *inRPE);
//
// Given a load `GenIntrinsicInst *GII`, all possible split dimensions (see
// below for details) can be obtained by calling
//  -- PossibleDims LoadSplitter::possibleDims(GenIntrinsicInst *GII);
//
// The splitting is then carried out by
//  -- bool LoadSplitter::split(GenIntrinsicInst *GII, Dims dims);
// where `dims` represent the desired dimensions of the split.
//
// To split all loads in a basic block use:
//  -- bool LoadSplitter::splitAllToSmallest(BasicBlock *BB);
//
// Splitting can be carried out automatically by the pass `SplitLoads`.
// To activate the pass, set the IGC flag `LS_enableLoadSplitting=1`.
//
// The splitting procedure consist of the following phases:
// 1. Process the load and its users to figure out the split structure of the
// load.
// 2. Calculate possible split dimensions.
// 3. Split.
//
// ============================================================================
// I.    Process the load
// ============================================================================
//
// Parameters of the intrinsic are stored and managed in the class `LoadData`.
// After verifying the validity of the parameters, the users of the load are
// traced and stored by the class `TraceData`. The tracing is carried out in
// `TraceData::tracePicks` and is done as follows:
//
// 1. A subvector of the loaded vector can be picked by either
// `ShuffleVectorInst` or a sequence of `InsertElementInst` and
// `ExtractElementInst`. If the picking is carried out by `ShuffleVectorInst`,
// the picks must come entirely from one of its arguments, with another one
// being an explicit constant (this includes undefs, zeroinitializers). The
// indices must be constant. Here the indices can repeat and undefs are allowed.
//
// If the picking is carried out by the sequence of `InsertElementInst` and
// `ExtractElementInst`, the sequence must start with:
//  -- %ext.0 = extractelement <16 x i16> %vec, i32 [from]
//  -- %pick.0 = insertelement <8 x i16> undef, i16 %ext, i32 [to]
// and continue by the repetition of:
//  -- %ext.n = extractelement <16 x i16> %vec, i32 [from]
//  -- %pick.n = insertelement <8 x i16> %pick.(n-1), i16 %ext.n, i32 [to]
// The indices [from] and [to] must be constant. Each extractelement must have a
// single user, which is the corresponding insertelement. Each insertelement,
// except for the last, must have a single user, which is the next
// extractelement.
//
// The picks can be stacked. For example,
// -- %pick.1 = shufflevector <16 x i16> %vec, <16 x i16> undef, <8 x i32> <i32
// 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
// -- %pick.1.1 = shufflevector <8 x i16> %pick.1, <8 x i16> undef, <4 x i32>
// <i32 4, i32 5, i32 6, i32 7>
//
// picks elements {12, 13, 14, 15} of the original %vec.
//
// The conditions above guarantee that the picks form a tree. Furthermore, no
// other values, except explicit constants and undefs, are used in the picks.
// This guarantees that the instructions can be safely erased after the split
// is done.
//
// 2. `BitCastInst` are allowed to appear in the tree of the picks if the bit
// width of the scalars does not change. Thus, a load of i32's can be cast to
// float, i16's to hf's, etc. If multiple bitcasts appear in the tree, the
// scalar type is recalculated each time.
//
// 3. Once a user that is not a node in the tree of picks is identified, it is
// saved. The users are grouped by the picks they use as well as the types they
// are cast to. Thus, it is possible to have multiple users of the same pick,
// and with different types as well.
//
// ============================================================================
// II.   Calculate possible split dimensions
// ============================================================================
//
// Once the tree of picks (and casts) is created, we want to calculate possible
// split dimensions. This is carried out in Load::possibleDims().
// We require that the picks obey two conditions:
// 1. Each pick must a multi-block range (`MBRange`)
// 2. All picks must be grid-uniform.
//
// Ad 1. A multi-brock range (MBRange) is a sequence of groups, each group
// containing the same number of consecutive integers. The gaps between the
// consecutive groups must also be equal. For example, a pick
//  -- {0,1,4,5,8,9,12,13}
// is a valid MBRange as it contains four groups of equal size of consecutive
// integers ( {0,1}, {4,5}, {8,9}, {12,13} ) and the gap
// between the groups is constant. All picks must be valid MBRanges.
//
// Given an MBRange, we say it has dimensions RxC, where R is the size of each
// group and C the number of groups. In the example above, RxC = 2x4. We can
// think about R and C as numbers of rows and columns in the RxC grid.
//
// If the pick consists of consecutive integers only, e.g. {2,3,4,5}, we prefer
// using C=1, so here we would have RxC = 4x1. However, 2x2 and 1x4 are also
// valid dimensions for this pick.
//
// Ad 2. Possible splits are determined by the size of the loaded vector and the
// number of blocks read by the intrinsic. Let's say the vector has length V
// (such as <V x i16>) and we have B block read (B is the 9-th argument in the
// intrinsic or "vB" in the OpenCL intrinsic such as
// @__builtin_IB_subgroup_block_read_cacheopts_u32_m8k16v2). We will say that
// the dimensions of the load are GxB, where G=V/B is the size of the group. We
// can think about the vector as a grid with G rows and B columns.
//
// The picks are
// grid-uniform if they are all MBRanges of the same dimensions and they
// constitute a tiling of the grid. They cannot overlap, repeat indices or miss
// any indices.
//
// For example, consider a 16-element vector loaded by an intrinsic with 4
// blocks. This constitutes a 4x4 grid. The grid can be tiled by the MBRanges of
// the following dimensions:
//  -- 1x1 ( {0}, {1}, ..., {15} )
//  -- 2x1 ( {0,1}, {2,3}, {4,5}, {6,7}, {8,9}, {10,11}, {12,13}, {14,15})
//  -- 4x1 ( {0,1,2,3}, {4,5,6,7}, {8,9,10,11}, {12,13,14,15})
//  -- 8x1 = 4x2 ( {0,1,2,3,4,5,6,7}, {8,9,10,11,12,13,14,15})
//  -- 16x1 = 8x2 = 4x4 ( {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15})
//  -- 1x2 ( {0,4}, {1,5}, {2,6}, {3,7}, {8,12}, {9,13}, {10,14}, {11,15} )
//  -- 2x2 ( {0,1,4,5}, {2,3,6,7}, {8,9,12,13}, {10,11,14,15} )
//  -- 1x4 ( {0,4,8,12}, {1,5,9,13}, {2,6,10,14}, {3,7,11,15} )
//  -- 2x4 ( {0,1,4,5,8,9,11,12}, {2,3,6,7,10,11,14,15} )
// Note that 8x1 = 4x2 and 16x1 = 8x2 = 4x4.
//
// In Load::possibleDims() we first check conditions 1 and 2. If they are
// satisfied, we calculate the dimensions of the picks. This gives us the
// smallest possible split. In addition, we calculate all other possible
// dimensions. For example, if the picks form a uniform subgrid of dimension 2x2
// of the grid 4x4, then the loads can be split into:
//  -- 2x2, 2x4, 8x1, 16x1.
//
// ============================================================================
// III.  Split
// ============================================================================
//
// First, for each MBRange, the new parameters of the load after the split are
// calculated and stored in the new instance of the `Load` class. After this is
// done, the new loads are created and inserted into the IR. If the size of the
// new loads is equal exactly to the size of the picks, the users can be
// connected directly to the new loads (with possible bitcasts). If the new
// loads produce larger vectors than the users consume, a sequence of
// `InsertElementInst` and `ExtractElementInst` is inserted in order to split
// the vectors into smaller chunks.
//
// As a final example consider a load has 4 blocks and produces a 16-element
// vector, i.e., it has dimension 4x4,
//  -- %vec = call <16 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v16i8(i64 %ptr,
//  i32 127, i32 63, i32 127, i32 0, i32 0, i32 8, i32 16, i32 4, i32 4, i1
//  false, i1 false, i32 0)
//
// The load is split into four 4-element vectors and the picks
// are grid-uniform of dimension 2x2,
//  -- %pick.0 = shufflevector <16 x i8> %vec, <16 x i8> undef, <4 x i32> <i32
//  0, i32 1, i32 4, i32 5>
//  -- %pick.1 = shufflevector <16 x i8> %vec, <16 x i8> undef, <4 x i32> <i32
//  2, i32 3, i32 6, i32 7>
//  -- %pick.2 = shufflevector <16 x i8> %vec, <16 x i8> undef, <4 x i32> <i32
//  8, i32 9, i32 12, i32 13>
//  -- %pick.3 = shufflevector <16 x i8> %vec, <16 x i8> undef, <4 x i32> <i32
//  10, i32 11, i32 14, i32 15>
//
// The minimal possible split has dimension 2x2. However, one of the conditions
// for the splits to work is that the size of the split vector x SIMD fills up
// at least 1 GRF. Otherwise the load involves padding, which would break the
// validity of the splits. Thus, the smallest valid split has dimension 4x2 =
// 8x1. In such a case the new loads produce 8-element vector and have 2 blocks:
//  -- %vec.0 = call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %ptr,
//  i32 127, i32 63, i32 127, i32 0, i32 0, i32 8, i32 16, i32 4, i32 2, i1
//  false, i1 false, i32 0)
//  -- %vec.1 = call <8 x i8> @llvm.genx.GenISA.LSC2DBlockRead.v8i8(i64 %ptr,
//  i32 127, i32 63, i32 127, i32 32, i32 0, i32 8, i32 16, i32 4, i32 2, i1
//  false, i1 false, i32 0)
//
// We still have to pick the 4-element subvectors %pick.n. n=0,1,2,3 of the
// 8-element vectors %vec.0 and %vec.1. This is done by inserting a sequence of
// `InsertElementInst` and `ExtractElementInst`. In this particular case the
// picks are {0,1,4,5} and {2,3,6,7} for both vectors. This example (and many
// others) can be found in the test file `isa_flak_k16.ll` as the test function
// `@i8_4x4_to_2x2`.
//
// ============================================================================

namespace IGC::LS {
Config &config() { return Config::get(); }
} // namespace IGC::LS

namespace {

constexpr unsigned DEF_PICK_SIZE = 64;
constexpr unsigned DEF_NUM_OF_LOADS = 4;
constexpr unsigned DEF_NUM_OF_PICKS_PER_LOAD = 4;
constexpr unsigned DEF_NUM_OF_CASTS_OR_USERS_PER_PICK = 2;
constexpr unsigned DEF_NUM_OF_USERS_PER_PICK = DEF_NUM_OF_CASTS_OR_USERS_PER_PICK * DEF_NUM_OF_CASTS_OR_USERS_PER_PICK;
constexpr unsigned DEF_NUM_OF_OPTS = 4;

struct Pick;
struct MBRange;

/// `Pick` represents a mask with some additional information.
/// It is a vector of integers, where each element is either an index or `-1`
/// (undef).
struct Pick : public SmallVector<int, DEF_PICK_SIZE> {
  explicit Pick() : SmallVector<int, DEF_PICK_SIZE>() {}
  explicit Pick(unsigned size, int init = -1) : SmallVector<int, DEF_PICK_SIZE>(size, init) {}
  Pick(std::initializer_list<int> init) : SmallVector<int, DEF_PICK_SIZE>(init) {}

  /// Returns `true` if the pick is `{0,1,2,...,length-1}`.
  bool isTrivial(unsigned length) const;

  /// Returns `true` if the element is contained in the pick.
  bool contains(int x) const { return std::find(begin(), end(), x) != end(); }

  /// Uses elements of `this` as indices to pick the elements from `origin`.
  /// If the index is out of range, places -1.
  Pick pickFrom(const Pick &origin) const;

  /// Creates the pick from a given mask. The elements must belong to the range
  /// [`begin`, `end`] or be undefs. If successful, returns the `Pick` with
  /// `begin` subtracted from all elements. Otherwise std::nullopt is returned.
  static std::optional<Pick> fromMask(ArrayRef<int> mask, int begin, int end);

  /// The index corresponding to the `n`-th element of `mbr` in `this` is mapped
  /// to `n`. If the index is absent in `mbr`, places -1.
  Pick narrowTo(const MBRange &mbr) const;

  /// Creates the pick containing all numbers from `begin` to `begin + size -
  /// 1`.
  static Pick createIdentityPick(unsigned size, int begin = 0);
};

/// `MBRange` (multi-block range) represents a sequence of groups, each group
/// containing the same number of consecutive integers.
/// - `first` denotes the first element in the `MBRange`.
/// - `grSize` denotes the number of elements in each group.
/// - `grPitch` denotes the distance between the first elements of two
/// consecutive groups.
/// - `numOfGr` denotes the number of groups.
/// If `grPitch <= grSize`, then `numOfGr` must be equal to `1`.
/// - `strd` denotes the stride between elements in each group.
/// if the elements are contiguous, `strd` is equal to `0` (not 1)
/// to simplify distinguishing between strided and non-strided cases.
/// Currently only stride 0 and 2 are supported.
struct MBRange {
  int first = 0;
  unsigned grSize = 0;
  int grPitch = 0;
  unsigned numOfGr = 0;
  unsigned strd = 0;

  explicit MBRange() = default;
  MBRange(int first, unsigned grSize, int grPitch, unsigned numOfGr, unsigned strd)
      : first(first), grSize(grSize), grPitch(grPitch), numOfGr(numOfGr), strd(strd) {}

  /// Returns empty range.
  static MBRange getEmpty() { return MBRange(); }

  /// Returns `true` if the range is empty.
  bool empty() const { return !grSize || !numOfGr || !grPitch; }

  /// Returns the total number of elements in the range.
  int size() const { return grSize * numOfGr; }

  /// Returns the stride between elements in the range.
  int stride() const { return strd; }

  /// Returns the last element of the range.
  int last() const {
    if (empty())
      return first;
    return grPitch * (static_cast<int>(numOfGr) - 1) + first + (static_cast<int>(grSize) - 1) * (strd ? strd : 1);
  }

  /// Returns the `n`-th element of the range.
  int operator[](int n) const;

  /// Returns the `elt` element of the `group`-th group.
  int operator()(int group, int elt) const;

  /// Returns the index corresponding to element `x` of the range. If `x` is not
  /// in the range, returns `-1`.
  int indexOf(int x) const;

  /// Returns `true` if the element is contained in the range.
  bool contains(int x) const;

  enum class Containment { Contains, Excludes, Intersects };
  /// Checks the relation between the range and the pick.
  /// - Returns `Containment::Contains` if the pick is contained fully in the
  /// range.
  /// - Returns `Containment::Excludes` if the pick lies entirely outside of the
  /// range.
  /// - Returns `Containment::Intersects` if the pick is sliced by the range.
  /// Undefs are discarded if `allowUndefs` is `true`. Otherise
  /// `Containment::Excludes` is returned.
  Containment containsOrExcludes(const Pick &pick, bool allowUndefs) const;

  /// Returns the pick of successive elements corresponding to the range.
  Pick toPick() const;

  /// Converts the pick to `MBRange`. The pick must contain consecutive indices
  /// and undefs are not allowed. If the pick is not a valid `MBRange`, returns
  /// `std::nullopt`.
  static std::optional<MBRange> fromPick(const Pick &pick);
};

} // unnamed namespace

// ===========================================================================

bool Pick::isTrivial(unsigned length) const {
  if (size() != length)
    return false;
  for (unsigned n = 0; n < length; ++n) {
    if ((*this)[n] != static_cast<int>(n))
      return false;
  }
  return true;
}

Pick Pick::pickFrom(const Pick &origin) const {
  Pick newPick = Pick(size());
  std::transform(begin(), end(), newPick.begin(),
                 [&](int n) -> int { return 0 <= n && static_cast<unsigned>(n) < origin.size() ? origin[n] : -1; });
  return newPick;
}

Pick Pick::narrowTo(const MBRange &mbr) const {
  Pick newPick = Pick(size());
  for (unsigned n = 0; n < size(); ++n) {
    int idx = mbr.indexOf((*this)[n]);
    newPick[n] = 0 <= idx ? idx : -1;
  }
  return newPick;
}

Pick Pick::createIdentityPick(unsigned size, int begin) {
  Pick interval = Pick(size);
  std::generate(interval.begin(), interval.end(), [&]() -> int { return begin++; });
  return interval;
}

std::optional<Pick> Pick::fromMask(ArrayRef<int> mask, int begin, int end) {
  Pick newPick = Pick(mask.size());
  for (unsigned n = 0; n < mask.size(); ++n) {
    int val = mask[n];
    if (0 <= val) {
      if (val < begin || end < val)
        return std::nullopt;
      newPick[n] = val - begin;
    } else {
      newPick[n] = -1;
    }
  }
  return std::make_optional(std::move(newPick));
}

[[maybe_unused]] raw_ostream &operator<<(raw_ostream &os, const Pick &pick) {
  os << "{ ";
  for (int x : pick) {
    os << x << ' ';
  }
  os << '}';
  return os;
}

// ===========================================================================

int MBRange::operator[](int n) const { return (*this)(n / grSize, n % grSize); }

int MBRange::operator()(int group, int elt) const { return group * grPitch + first + elt * (strd ? strd : 1); }

bool MBRange::contains(int x) const {
  if (empty())
    return false;
  if (!strd) {
    return 1 < numOfGr ? first <= x && x <= last() && (x - first) % grPitch < static_cast<int>(grSize)
                       : first <= x && x <= last();
  }

  // Strided case: use indexOf to check containment
  return indexOf(x) >= 0;
}

MBRange::Containment MBRange::containsOrExcludes(const Pick &pick, bool allowUndefs) const {
  if (pick.empty())
    return MBRange::Containment::Contains;
  if (empty())
    return MBRange::Containment::Excludes;
  return std::any_of(pick.begin(), pick.end(), [&](int x) -> bool { return contains(x); })
             ? std::all_of(pick.begin(), pick.end(),
                           [&](int x) -> bool { return contains(x) || (allowUndefs && x < 0); })
                   ? MBRange::Containment::Contains
                   : MBRange::Containment::Intersects
         : std::all_of(pick.begin(), pick.end(), [&](int x) -> bool { return !contains(x); })
             ? MBRange::Containment::Excludes
             : MBRange::Containment::Intersects;
}

int MBRange::indexOf(int x) const {
  if (empty())
    return -1;

  if (!strd) {
    // Non-strided case: elements are contiguous within groups
    if (1 < numOfGr) {
      if (first <= x && x <= last() && (x - first) % grPitch < static_cast<int>(grSize))
        return (x - first) / grPitch * grSize + (x - first) % grPitch;
    } else {
      if (first <= x && x <= last())
        return x - first;
    }
    return -1;
  }

  // Strided case: need to find which group and which element within the group
  for (unsigned gr = 0; gr < numOfGr; ++gr) {
    for (unsigned el = 0; el < grSize; ++el) {
      if (x == (*this)(gr, el))
        return gr * grSize + el;
    }
  }
  return -1;
}

Pick MBRange::toPick() const {
  if (empty())
    return Pick();
  Pick pick = Pick(size());
  for (int n = 0; n < size(); ++n) {
    pick[n] = (*this)[n];
  }
  return pick;
}

std::optional<MBRange> MBRange::fromPick(const Pick &pick) {
  if (pick.empty()) {
    return MBRange::getEmpty();
  }
  if (pick.front() < 0) {
    return std::nullopt;
  }

  int val;
  MBRange ret(pick.front(), 0, 0, 0, 0);
  for (unsigned n = 1; n < pick.size(); ++n) {
    val = pick[n];
    // We don't allow undefs.
    if (val < 0) {
      return std::nullopt;
    }
    // The value jumps, so we possibly reached the group pitch.
    if (val != ret.first + static_cast<int>(n)) {
      ret.grSize = n;
      ret.grPitch = val - ret.first;
      break;
    }
  }
  // Single group range.
  if (ret.grSize == 0) {
    ret.grSize = pick.size();
    ret.numOfGr = 1;
    return ret;
  }
  // grPitch should be positive and larger than the grSize
  if (ret.grPitch <= static_cast<int>(ret.grSize)) {
    return std::nullopt;
  }

  if (pick.size() % ret.grSize) {
    return std::nullopt;
  }
  ret.numOfGr = pick.size() / ret.grSize;
  for (unsigned gr = 1; gr < ret.numOfGr; ++gr) {
    for (unsigned el = 0; el < ret.grSize; ++el) {
      val = pick[gr * ret.grSize + el];
      if (val < 0) {
        return std::nullopt;
      }
      if (val != ret(gr, el)) {
        return std::nullopt;
      }
    }
  }

  // If pick is {0, 2, 4, 6, 8, ...} we transform it to a strided MBRange
  // (strd=2 and the rest adjusted).
  // It makes sense because the loads with the strided picks can be split later
  // under some conditions (transposed load, every WI gets 2 elements after the load).
  // Otherwise it's usually not possible to split the load that has stride (or, more generally,
  // many groups of size = 1 and grPitch > 1).
  // We only support this case of the strided access for now.
  if (ret.grSize == 1 && ret.grPitch == 2) {
    ret.grPitch = 1;
    ret.numOfGr = 1;
    ret.strd = 2;
    ret.grSize = pick.size();
  }

  return ret;
}

[[maybe_unused]] raw_ostream &operator<<(raw_ostream &os, const MBRange &range) {
  for (unsigned gr = 0; gr < range.numOfGr; ++gr) {
    os << '[' << range(gr, 0) << ", " << range(gr, range.grSize - 1);
    if (range.strd)
      os << ", (" << range.strd << ")";
    os << "] ";
  }
  return os;
}

// ==========================================================================

namespace {

constexpr unsigned NUM_OF_BLOCKLOAD_ARGS = 13;

/// Indices for arguments of GenISA_LSC2DBlockRead.
namespace LSC2D_BlockRead {
enum : unsigned {
  argSurfacePtr = 0,
  argSurfaceWidthLessOne_inBytes = 1,
  argSurfaceHeightLessOne_inPitches = 2,
  argSurfacePitchLessOne_inBytes = 3,
  argXOffset_inElts = 4,
  argYOffset_inPitches = 5,
  argSizeInBits = 6,
  argBlockWidth_inElts = 7,
  argBlockHeight_inElts = 8,
  argNumOfBlocks = 9,
  argIsTranspose = 10,
  argIsVNNI = 11,
  argCacheFlags = 12
};
}

/// Returns the numeric value of the argument number `n` to the intrinsic `GII`
/// as `unsigned int`. Assumes `GII->getArgOperand(n)` exists and can be cast to
/// `ConstantInt`.
static unsigned getArgZ(GenIntrinsicInst *GII, unsigned n) {
  return static_cast<unsigned>(cast<ConstantInt>(GII->getArgOperand(n))->getZExtValue());
}

#define DBG(x) LLVM_DEBUG(x);

using Picks = SmallVector<Pick, DEF_NUM_OF_PICKS_PER_LOAD>;
using MBRanges = SmallVector<MBRange, DEF_NUM_OF_PICKS_PER_LOAD>;

/// `TraceData` contains all the data about the structure of the splits of a
/// load vector. It gathers all picks from the load together with the associated
/// bitcasts and their users.
struct TraceData {
  using Cast = SmallDenseMap<Type *, SmallVector<Instruction *, DEF_NUM_OF_CASTS_OR_USERS_PER_PICK>,
                             DEF_NUM_OF_CASTS_OR_USERS_PER_PICK>;
  using Casts = SmallVector<Cast, DEF_NUM_OF_PICKS_PER_LOAD>;
  using ToRemove = SmallVector<Instruction *, DEF_NUM_OF_PICKS_PER_LOAD + 1>;

  BasicBlock *BB = nullptr;
  Pick initialPick = Pick();
  std::unique_ptr<Picks> picks = nullptr;
  Casts typesToCastTo = Casts();
  ToRemove toRemove = ToRemove();

  /// Returns total vector length.
  unsigned vectorLength() const { return initialPick.size(); }

  /// Traces the pick tree starting from the load `GII` and returns `true` if
  /// the `TraceData` is valid. If `true` is returned, then `picks` is non-null.
  bool tracePicks(GenIntrinsicInst *GII);

  /// Uses `builder` to create the sequence of LLVM instructions that represent
  /// the splits: i) The tree is attached to `load` and ii) For each pick,
  /// the corresponding sequence of `InsertElementInst` and `ExtractElementInst`
  /// is inserted. iii) Final bitcasts are attached and the users are updated
  /// appropriately.
  void putPicks(IRBuilder<> &builder, Value *load);

  /// Removes all instructions marked to remove.
  void removeOldInstructions();

  /// Returns new `TraceData` containing those picks in this, which are subpicks
  /// of `largeRange`.
  std::unique_ptr<TraceData> pickSubpicksOf(const MBRange &largeRange);

private:
  struct Node : public Pick {
    Type *type;
    Instruction *fun;

    explicit Node() : Pick(), type(nullptr), fun(nullptr) {}
    Node(const Pick &pick, Type *type, Instruction *fun) : Pick(pick), type{type}, fun{fun} {}
  };

  std::optional<Node> addBitCast(const Node &previous, BitCastInst *BCI);
  std::optional<Node> addShuffle(const Node &previous, ShuffleVectorInst *SVI);
  std::optional<Node> addExtractInsertSequence(const Node &previous, ExtractElementInst *EEI,
                                               SmallPtrSet<Value *, DEF_NUM_OF_PICKS_PER_LOAD> &extractsToSkip);
  void addLeaf(const Node &leaf);
};

/// `LoadData` represents the data that is associated with a block load.
struct LoadData {
  /// Block width in elements as specified in the intrinsic.
  unsigned blockWidth_E = 0;

  /// Block height in elements as specified in the intrinsic.
  unsigned blockHeight_E = 0;

  /// Number of blocks as specified in the intrinsic.
  unsigned numOfBlocks = 0;

  /// Length of the loaded vector in elements.
  unsigned vectorLength = 0;

  /// Bit width of a single element in the loaded vector.
  unsigned scalarBitWidth = 0;

  /// Bit width of a single element on the surface as specified in the
  /// intrinsic.
  unsigned elementBitWidth = 0;

  /// Is the load transposed.
  bool transposed = false;

  /// Is the load VNNI-transformed.
  bool vnni = false;

  /// Returns the length of a single block in the vector.
  unsigned groupLength() const { return vectorLength / numOfBlocks; }

  /// Uses parameters of the intrinsic to figure out SIMD.
  /// If the SIMD is reported correctly, this is equal to config().actualSimd.
  unsigned SIMD() const {
    // From the point of view of the intrinsic, the total bit width of the load
    // is: totalBW = blockWidth * blockHeight * numOfBlocks * elementBitWidth.
    // From the point of view of the loaded vectors, the total bit width is:
    // totalBW = vectorLength * SIMD * scalarBitWidth.
    // From this:
    return (blockWidth_E * blockHeight_E * numOfBlocks * elementBitWidth) / (vectorLength * scalarBitWidth);
  }

  /// Returns the size of a single scalar multiplied by the SIMD.
  unsigned scalarMemSize_B() const {
    return (blockWidth_E * blockHeight_E * numOfBlocks * elementBitWidth) / (8 * vectorLength);
  }

  /// Returns the minimum valid group length for the split load.
  unsigned getMinGroupLength(unsigned atLeastThisLarge = 0) const;

  /// Checks if the load has valid parameters as long as this pass in concerned.
  bool isValidLoad() const;

  /// Returns the name of the intrinsic with the data represented by this
  /// `LoadData` and corresponding to the stand-alone block load.
  std::string getBlockLoadName() const;

protected:
  IntegerType *scalarTy = nullptr;

private:
  bool isValidTransposed() const;
  bool isValidVNNI() const;
};

/// `Load` contains all the data associated with the load and its picks.
/// It provides methods for splitting the load and creating the corresponding
/// LLVM IR.
struct Load : public LoadData {
  GenIntrinsicInst *GII = nullptr;
  int xOffset_E = 0; // X Offset in elements
  int yOffset_P = 0; // Y Offset in surface pitch
  std::unique_ptr<TraceData> trace = nullptr;

  explicit Load() = default;
  Load(const LoadData &data) : LoadData(data) {}
  Load(const Load &rhs) : LoadData(rhs), GII(rhs.GII), xOffset_E(rhs.xOffset_E), yOffset_P(rhs.yOffset_P) {}
  Load &operator=(const Load &rhs) {
    if (this != &rhs) {
      LoadData::operator=(rhs);
      GII = rhs.GII;
      xOffset_E = rhs.xOffset_E;
      yOffset_P = rhs.yOffset_P;
    }
    return *this;
  }
  Load(Load &&) = default;
  Load &operator=(Load &&) = default;
  ~Load() = default;

  /// Reads the data from the load intrinsic.
  bool readFromLoad(GenIntrinsicInst *GII);

  /// Traces all picks attached to the load and returns `true` if the
  /// `TraceData` is valid. If `true` is returned, then `trace` and
  /// `trace->picks` is non-null.
  bool tracePicks();

  /// Returns the set of possible grid-uniform dimensions into which the load
  /// can be split. It takes into account the limits from `Config`.
  PossibleDims possibleDims();

  /// Returns the new load corresponding to loading only part of the original
  /// load. If the load is not valid, returns `nullptr`.
  std::unique_ptr<Load> split(const MBRange &range);

  /// Creates the LLVM call for the stand-alone block load.
  /// Uses `GII` as the insertion point, so `GII` should point to the old load.
  CallInst *putBlockLoad(IRBuilder<> &builder);

  /// Deletes the original load and its picks.
  void removeOldInstructions();

private:
  bool fillBlockData();

  Load &splitFlat(const MBRange &range);
  Load &splitTransposed(const MBRange &range);
  Load &splitVNNI(const MBRange &range);

  std::unique_ptr<Load> splitLoadData(const MBRange &range);
};

} // unnamed namespace

// ==========================================================================

[[maybe_unused]] raw_ostream &operator<<(raw_ostream &os, const Dims &dims) {
  os << dims.grSize << " x " << dims.numOfGr;
  if (dims.stride > 1)
    os << " (stride " << dims.stride << ")";
  return os;
}

// ==========================================================================

/// Given a `Value` of a vectorial type, returns the pair representing the
/// scalar type and the size of the vector.
static std::pair<IntegerType *, unsigned> getScalarTypeAndSize(Value *V) {
  std::pair<IntegerType *, unsigned> ret{nullptr, 0};
  IGCLLVM::FixedVectorType *vectorTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType());
  if (!vectorTy)
    return ret;
  ret.first = dyn_cast<IntegerType>(vectorTy->getElementType());
  if (!ret.first)
    return ret;
  ret.second = vectorTy->getNumElements();
  return ret;
}

/// Creates the function of a given name in LLVM.
static Function *createFunction(StringRef name, Module *currModule, ArrayRef<Value *> args, Type *retTy,
                                Function *copyAttrAfter = nullptr) {
  SmallVector<Type *, NUM_OF_BLOCKLOAD_ARGS> argsTy;
  argsTy.assign(args.size(), nullptr);
  for (unsigned i = 0; i < argsTy.size(); ++i) {
    argsTy[i] = args[i]->getType();
  }
  FunctionType *newFunTy = FunctionType::get(retTy, argsTy, false);
  Function *newFun = Function::Create(newFunTy, GlobalValue::ExternalLinkage, name, currModule);
  if (copyAttrAfter) {
    newFun->copyAttributesFrom(copyAttrAfter);
    if (isa<GlobalObject>(copyAttrAfter)) {
      newFun->copyMetadata(cast<GlobalObject>(copyAttrAfter), 0);
    }
  }
  return newFun;
}

/// Adds `offset` to `value` if `offset` is non-zero.
static Value *createAdd(IRBuilder<> &builder, Value *value, unsigned offset) {
  return offset ? (isa<ConstantInt>(value)
                       ? builder.getInt32(static_cast<unsigned>(cast<ConstantInt>(value)->getZExtValue()) + offset)
                       : builder.CreateAdd(value, builder.getInt32(offset), "", true, true))
                : value;
}

// ===========================================================================

bool Config::initialize(Function *F, CodeGenContext *inCGC, IGCLivenessAnalysisRunner *inRPE) {
  CGC = inCGC;
  RPE = inRPE;
  if (!F || !CGC || !RPE)
    return false;
  if (!CGC->platform.hasLSC()) {
    DBG(dbgs() << " [SKIP] No support for LSC on this platform.\n");
    return false;
  }
  if (!IGC::ForceAlwaysInline(CGC)) {
    if (F->isDeclaration())
      return false;
  } else {
    if (!F->getReturnType()->isVoidTy())
      return false;
  }

  // Actual SIMD is the SIMD as reported by the compiler.
  // Default SIMD is the default SIMD associated with the architecture.
  // Default SIMD is used only if actual SIMD is absent and mostly for testing
  // purposes.
  defaultSimd = 0;
  switch (CGC->platform.getPlatformInfo().eProductFamily) {
  case IGFX_DG2:
  case IGFX_METEORLAKE:
  case IGFX_ARROWLAKE:
    defaultSimd = 16;
    break;
  default:
    defaultSimd = 32;
    break;
  }
  actualSimd = 0;
  if (RPE->MDUtils && RPE->MDUtils->findFunctionsInfoItem(F) != RPE->MDUtils->end_FunctionsInfo()) {
    IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = RPE->MDUtils->getFunctionsInfoItem(F);
    actualSimd = funcInfoMD->getSubGroupSize()->getSIMDSize();
  }
  if (IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth)) {
    actualSimd = IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth);
  }

  isLegitW8 = false;
  isLegitW8 = CGC->platform.supports2dBlockTranspose64ByteWidth();
  sizeOfRegs_B = RPE->registerSizeInBytes();
  numOfRegs = CGC->getNumGRFPerThread();

  minSplitSize_B = minSplitSize_GRF * sizeOfRegs_B;
  splitThreshold_B = (static_cast<int>(numOfRegs) + splitThresholdDelta_GRF) * sizeOfRegs_B;

  DBG(Module *newM = F->getParent(); if (newM != M) {
    M = newM;
    dbgs() << "CONFIG DATA:\n";
    dbgs() << " -- SPLITTING ENABLED / ignore reg pressure          = " << enableLoadSplitting << " / "
           << ignoreSplitThreshold << "\n";
    dbgs() << " -- register size [B] / number of registers          = " << sizeOfRegs_B << " / " << numOfRegs << "\n";
    dbgs() << " -- default SIMD / actual SIMD                       = " << defaultSimd << " / " << actualSimd << "\n";
    dbgs() << " -- split threshold [B]                              = " << splitThreshold_B << "\n";
    dbgs() << " -- min split size [E] / min split size [B]          = " << minSplitSize_E << " / " << minSplitSize_B
           << "\n";
  });
  return true;
}

// ===========================================================================

std::optional<TraceData::Node> TraceData::addBitCast(const Node &previous, BitCastInst *BCI) {
  IGCLLVM::FixedVectorType *srcTy = dyn_cast<IGCLLVM::FixedVectorType>(BCI->getSrcTy());
  IGCLLVM::FixedVectorType *destTy = dyn_cast<IGCLLVM::FixedVectorType>(BCI->getDestTy());
  if (!(srcTy && destTy))
    return std::nullopt;
  // We only allow bitcasts that preserve the size of the underlying scalar
  // type.
  if (srcTy->getElementType()->getScalarSizeInBits() != destTy->getElementType()->getScalarSizeInBits())
    return std::nullopt;
  return TraceData::Node(previous, destTy->getElementType(), BCI);
}

std::optional<TraceData::Node> TraceData::addShuffle(const Node &previous, ShuffleVectorInst *SVI) {
  if (!isa<IGCLLVM::FixedVectorType>(SVI->getType()))
    return std::nullopt;
  // Previous node is a bitcast, shuffle vector, insert element, or the original
  // load, so we checked that previous->fun has the type of a fixed vector.
  unsigned previousVectorLength = cast<IGCLLVM::FixedVectorType>(previous.fun->getType())->getNumElements();
  // We must make sure that the shuffle vector is a pick from the previous
  // vector. The other vector must be undef.
  int beginPos, endPos;
  if (SVI->getOperand(0) == previous.fun) {
    beginPos = 0;
    endPos = previousVectorLength - 1;
    if (!isa<Constant>(SVI->getOperand(1)))
      return std::nullopt;
  } else {
    beginPos = previousVectorLength;
    endPos = 2 * previousVectorLength - 1;
    if (!isa<Constant>(SVI->getOperand(0)))
      return std::nullopt;
  }
  // Pick::fromMask guarantees that the pick is entirely contained in [beginPos,
  // endPos].
  std::optional<Pick> newPick = Pick::fromMask(SVI->getShuffleMask(), beginPos, endPos);
  if (!newPick)
    return std::nullopt;
  return TraceData::Node(newPick->pickFrom(previous), previous.type, SVI);
}

std::optional<TraceData::Node>
TraceData::addExtractInsertSequence(const TraceData::Node &previous, ExtractElementInst *EEI,
                                    SmallPtrSet<Value *, DEF_NUM_OF_PICKS_PER_LOAD> &extractsToSkip) {

  auto isValidExtract = [&](ExtractElementInst *E) -> bool {
    if (!isa<IGCLLVM::FixedVectorType>(E->getVectorOperand()->getType()))
      return false;
    if (!isa<ConstantInt>(E->getIndexOperand()))
      return false;
    if (!E->hasOneUse())
      return false;
    return true;
  };

  auto isValidInsert = [&](InsertElementInst *I) -> bool {
    if (!isa<IGCLLVM::FixedVectorType>(I->getType()))
      return false;
    if (!isa<ConstantInt>(I->getOperand(2)))
      return false;
    return true;
  };

  auto getAssociatedInsert = [&](ExtractElementInst *E) -> InsertElementInst * {
    return dyn_cast<InsertElementInst>(*E->user_begin());
  };

  auto getAssociatedExtract = [&](InsertElementInst *I) -> ExtractElementInst * {
    return dyn_cast<ExtractElementInst>(I->getOperand(1));
  };

  auto getPreviousInsert = [&](InsertElementInst *I) -> InsertElementInst * {
    return dyn_cast<InsertElementInst>(I->getOperand(0));
  };

  auto getNextInsert = [&](InsertElementInst *I) -> InsertElementInst * {
    return dyn_cast<InsertElementInst>(*I->user_begin());
  };

  std::deque<std::pair<ExtractElementInst *, InsertElementInst *>> EIs;

  // We want to find the chain of extract/insert elements.
  // There is no guarantee that the first user of the load is the first extract
  // element, so we must traverse the chain both up and down.

  auto addPairFromExtract = [&](ExtractElementInst *E, bool front) -> bool {
    if (!E)
      return false;
    extractsToSkip.insert(E);
    if (!isValidExtract(E))
      return false;
    InsertElementInst *I = getAssociatedInsert(E);
    if (!I || !isValidInsert(I))
      return false;
    if (front) {
      EIs.push_front(std::make_pair(E, I));
    } else {
      EIs.push_back(std::make_pair(E, I));
    }
    return true;
  };

  auto addPairFromInsert = [&](InsertElementInst *I, bool front) -> bool {
    if (!I || !isValidInsert(I))
      return false;
    ExtractElementInst *E = getAssociatedExtract(I);
    if (!E)
      return false;
    extractsToSkip.insert(E);
    if (!isValidExtract(E))
      return false;
    if (front) {
      EIs.push_front(std::make_pair(E, I));
    } else {
      EIs.push_back(std::make_pair(E, I));
    }
    return true;
  };

  // First we move "up" to find the first pair of insert/extract elements.
  if (!addPairFromExtract(EEI, true))
    return std::nullopt;
  InsertElementInst *insert = EIs.front().second;
  do {
    insert = getPreviousInsert(insert);
  } while (addPairFromInsert(insert, true));
  // Since addPairFromExtract succeeded, we know that the first pair in EIs
  // exists. This pair is valid but the previous pair failed. So either there
  // was no previous pair (which is what we want) or the previous pair was
  // invalid and we bail out.
  if (!isa<UndefValue>(EIs.front().second->getOperand(0)))
    return std::nullopt;

  // Now we move "down".
  insert = EIs.back().second;
  do {
    // If the insert has more than one user, the sequence ends.
    if (!insert->hasOneUse())
      break;
    insert = getNextInsert(insert);
  } while (addPairFromInsert(insert, false));
  // It is the role of tracePicks to figure out if the next instructions are
  // valid leaves.

  // The first pair in EIs determines:
  //  - from which vector we pick
  //  - how many elements we pick

  unsigned newVectorLength = cast<IGCLLVM::FixedVectorType>(EIs.front().second->getType())->getNumElements();
  Value *pickingFrom = EIs.front().first->getVectorOperand();
  Value *currentBuildVector = nullptr;
  Pick pick = Pick(newVectorLength, -1);

  // While building the picks, we have to check that:
  //  - we pick from the same vector
  //  - we keep building the same vector
  //  - we assign each index only once

  auto addPick = [&](std::pair<ExtractElementInst *, InsertElementInst *> &ei) -> bool {
    if (ei.first->getVectorOperand() != pickingFrom)
      return false;
    if (currentBuildVector) {
      if (currentBuildVector != ei.second->getOperand(0))
        return false;
    }
    currentBuildVector = ei.second;

    unsigned origIdx = cast<ConstantInt>(ei.first->getIndexOperand())->getZExtValue();
    unsigned newIdx = cast<ConstantInt>(ei.second->getOperand(2))->getZExtValue();
    if (pick[newIdx] != -1)
      return false;
    pick[newIdx] = origIdx;
    return true;
  };

  for (unsigned n = 0; n < EIs.size() - 1; ++n) {
    if (!addPick(EIs[n]))
      return std::nullopt;
    toRemove.push_back(EIs[n].first);
    toRemove.push_back(EIs[n].second);
  }
  if (!addPick(EIs.back()))
    return std::nullopt;
  toRemove.push_back(EIs.back().first);
  return TraceData::Node(pick.pickFrom(previous), previous.type, EIs.back().second);
}

void TraceData::addLeaf(const Node &leaf) {
  if (leaf.empty())
    return;
  auto it = std::find(picks->begin(), picks->end(), leaf);
  if (it != picks->end()) {
    typesToCastTo[it - picks->begin()][leaf.type].push_back(leaf.fun);
  } else {
    picks->push_back(leaf);
    typesToCastTo.push_back(Cast{{leaf.type, SmallVector<Instruction *, 1>(1, leaf.fun)}});
  }
}

bool TraceData::tracePicks(GenIntrinsicInst *GII) {
  auto [_, vecLen] = getScalarTypeAndSize(GII);
  if (!vecLen)
    return false;
  BB = GII->getParent();
  initialPick = Pick::createIdentityPick(vecLen);
  picks = std::make_unique<Picks>();
  typesToCastTo.clear();
  toRemove.clear();

  SmallVector<TraceData::Node, DEF_NUM_OF_PICKS_PER_LOAD + 1> activeNodes;
  activeNodes.emplace_back(Pick::createIdentityPick(vectorLength()),
                           cast<IGCLLVM::FixedVectorType>(GII->getType())->getElementType(), GII);
  TraceData::Node currNode;
  // We need to skip the extract elements that are part of the already processed
  // insert/extract sequence.
  SmallPtrSet<Value *, DEF_NUM_OF_PICKS_PER_LOAD> extractsToSkip;

  while (!activeNodes.empty()) {
    currNode = activeNodes.back();
    activeNodes.pop_back();
    toRemove.push_back(currNode.fun);
    for (User *nextUser : currNode.fun->users()) {
      Instruction *next = dyn_cast<Instruction>(nextUser);
      if (!next) {
        return false;
      }
      if (ShuffleVectorInst *SVI = dyn_cast<ShuffleVectorInst>(next)) {
        // currNode->fun can be the starting load, bitcast, insert element, or
        // shufflevector. In all those cases we checked that
        // currNode->fun->getType() is a fixed vector type.
        std::optional<TraceData::Node> newNode = addShuffle(currNode, SVI);
        if (!newNode) {
          return false;
        }
        activeNodes.push_back(std::move(*newNode));
      } else if (BitCastInst *BCI = dyn_cast<BitCastInst>(next)) {
        std::optional<TraceData::Node> newNode = addBitCast(currNode, BCI);
        if (!newNode) {
          return false;
        }
        activeNodes.push_back(std::move(*newNode));
      } else if (ExtractElementInst *EEI = dyn_cast<ExtractElementInst>(next)) {
        if (extractsToSkip.count(EEI))
          continue;
        std::optional<TraceData::Node> newNode = addExtractInsertSequence(currNode, EEI, extractsToSkip);
        if (!newNode) {
          return false;
        }
        activeNodes.push_back(std::move(*newNode));
      } else {
        addLeaf(currNode);
      }
    }
  }
  return true;
}

void TraceData::putPicks(IRBuilder<> &builder, Value *load) {
  if (!picks)
    return;

  // This assumes no repeated values in the pick.
  auto putPick = [&](const Pick &pick, Value *loadOrCast, Type *scalarTy) -> Value * {
    Value *elem;
    Value *newVector = UndefValue::get(IGCLLVM::FixedVectorType::get(scalarTy, pick.size()));
    for (unsigned insertPos = 0; insertPos < pick.size(); ++insertPos) {
      if (pick[insertPos] < 0)
        continue;
      unsigned extractPos = static_cast<unsigned>(pick[insertPos]);
      elem = builder.CreateExtractElement(loadOrCast, extractPos);
      newVector = builder.CreateInsertElement(newVector, elem, insertPos);
    }
    return newVector;
  };

  // We attach the pick tree to the load.

  Value *loadOrCast = load;
  Type *scalarTy;
  std::tie(scalarTy, std::ignore) = getScalarTypeAndSize(load);

  Value *picksVal;
  Value *pickAndCastVal;
  for (unsigned n = 0; n < picks->size(); ++n) {
    const Pick &pick = (*picks)[n];
    // If a pick is non-trivial, we insert the insert/extract sequence.
    // picksVal is the resulting value, or the origianl load if no pick is
    // required.
    picksVal = pick.isTrivial(vectorLength()) ? loadOrCast : putPick(pick, loadOrCast, scalarTy);

    // For each pick we cast picksVal to the appropriate type and replace the
    // users. pickAndCastVal is the resulting value of the cast, if needed, or
    // is the original pick.
    for (auto &[type, users] : typesToCastTo[n]) {
      if (type == scalarTy) {
        pickAndCastVal = picksVal;
      } else {
        pickAndCastVal = builder.CreateBitCast(picksVal, IGCLLVM::FixedVectorType::get(type, pick.size()));
      }
      for (Instruction *user : users) {
        // The only case when fun == newCallAndCast is when the load goes
        // directly into the call, without any shuffles or bitcasts.
        if (user != pickAndCastVal) {
          user->replaceAllUsesWith(pickAndCastVal);
        }
      }
      users.clear();
      users.push_back(dyn_cast<Instruction>(pickAndCastVal));
    }
  }
}

void TraceData::removeOldInstructions() {
  for (Instruction *instr : toRemove) {
    if (instr && instr->getType()) {
      instr->replaceAllUsesWith(UndefValue::get(instr->getType()));
    }
  }
  for (Instruction *instr : toRemove) {
    if (instr) {
      instr->eraseFromParent();
    }
  }
  toRemove.clear();
}

std::unique_ptr<TraceData> TraceData::pickSubpicksOf(const MBRange &mbr) {
  if (!picks)
    return nullptr;
  std::unique_ptr<TraceData> ret = std::make_unique<TraceData>();
  ret->BB = BB;
  ret->picks = std::make_unique<Picks>();
  ret->initialPick = mbr.toPick();
  for (unsigned n = 0; n < picks->size(); ++n) {
    const Pick &pick = (*picks)[n];
    if (mbr.contains(pick[0])) {
      Pick narrowed = pick.narrowTo(mbr);
      ret->picks->push_back(narrowed);
      ret->typesToCastTo.push_back(typesToCastTo[n]);
    }
  }
  return ret;
}

// ===========================================================================

/// Creates a vector of MBRange's, each of the same `grSize`, `grPitch`, and
/// `numOfGr`. The ranges cover the entire vector of length `vectorLength`.
static std::optional<MBRanges> makeUniform(unsigned grSize, int grPitch, unsigned numOfGr, unsigned vectorLength,
                                           unsigned stride) {
  if (!vectorLength)
    return std::nullopt;
  if (grSize == grPitch) {
    grSize *= numOfGr;
    numOfGr = 1;
  }
  MBRanges mbrs;
  if (numOfGr <= 1) {
    if (vectorLength % grSize)
      return std::nullopt;
    unsigned numOfGroups = vectorLength / grSize;
    mbrs.resize(numOfGroups);
    for (unsigned n = 0; n < numOfGroups; ++n) {
      mbrs[n] = MBRange(n * (stride ? 1 : grSize), grSize, grPitch, 1, stride);
    }
  } else {
    if (stride) {
      // Strided multi-group ranges are not supported.
      return std::nullopt;
    }
    if (vectorLength % (grPitch * numOfGr) || grPitch % grSize || grPitch <= static_cast<int>(grSize) || grPitch <= 0)
      return std::nullopt;
    unsigned numInBlock = grPitch / grSize;
    unsigned blockPitch = grPitch * numOfGr;
    unsigned numOfBlocks = vectorLength / blockPitch;
    mbrs.resize(numInBlock * numOfBlocks);
    for (unsigned n = 0; n < numOfBlocks; ++n) {
      for (unsigned m = 0; m < numInBlock; ++m) {
        mbrs[n * numInBlock + m] = MBRange(n * blockPitch + m * grSize, grSize, grPitch, numOfGr, 0);
      }
    }
  }
  return std::make_optional(std::move(mbrs));
}

/// Creates a vector of MBRange's, each of the same `grSize`, `grPitch`, and
/// `numOfGr` and covering the entire vector of length `vectorLength`
/// grid-uniformly. Each pick of `picks` must fit into exactly one range.
static std::optional<MBRanges> makeGridUniformPicks(const Picks &picks, unsigned vectorLength, unsigned blockLength) {
  if (picks.empty())
    return std::nullopt;
  MBRanges mbrs;
  mbrs.reserve(picks.size());
  for (unsigned n = 0; n < picks.size(); ++n) {
    // The picks must be grid-uniform. No undefs allowed.
    std::optional<MBRange> mbr = MBRange::fromPick(picks[n]);
    if (!mbr) {
      DBG(dbgs() << "    -- Pick " << picks[n] << " is not a valid multi-block range.\n");
      return std::nullopt;
    }
    if (mbr->numOfGr == 1) {
      mbr->grPitch = blockLength;
    }
    mbrs.push_back(*mbr);
  }
  MBRange mbr = mbrs.front();
  if (mbrs.size() * mbr.size() != vectorLength) {
    DBG(dbgs() << "    -- Invalid sizes of multi-block ranges.\n");
    return std::nullopt;
  }
  if (!std::all_of(std::next(mbrs.begin()), mbrs.end(), [&](const MBRange &x) {
        return x.grPitch == mbr.grPitch && x.grSize == mbr.grSize && x.numOfGr == mbr.numOfGr;
      })) {
    DBG(dbgs() << "    -- Multi-block ranges of different sizes.\n");
    return std::nullopt;
  }

  bool stride = false;

  if (mbr.stride()) {
    stride = true;
    if (mbr.stride() != 2) {
      DBG(dbgs() << "    -- Invalid stride of multi-block range " << mbr << ".\n");
      return std::nullopt;
    }
  }

  if (mbr.numOfGr == 1) {
    if (!isPowerOf2_32(mbr.grSize) || vectorLength % mbr.grSize || mbr.first % mbr.grSize) {
      DBG(dbgs() << "    -- Invalid size of multi-block range " << mbr << ".\n");
      return std::nullopt;
    }
  } else if (!stride) {
    if (!(isPowerOf2_32(mbr.grSize) && isPowerOf2_32(mbr.grPitch) && isPowerOf2_32(mbr.numOfGr))) {
      DBG(dbgs() << "    -- Invalid size of multi-block range " << mbr << ".\n");
      return std::nullopt;
    }

    // - group size must be smaller than block length and be its divisor
    // - group pitch must match the block length (there are no gaps between
    // blocks)
    if (blockLength <= mbr.grSize || blockLength % mbr.grSize || mbr.grPitch != blockLength) {
      DBG(dbgs() << "    -- Invalid size of multi-block range " << mbr << ".\n");
      return std::nullopt;
    }
    unsigned numOfBlocks = vectorLength / blockLength;
    unsigned firstInGr = mbr.first % blockLength;
    unsigned grStart = mbr.first / blockLength;
    // - first elt in each group must be a multiple of the group size
    // - number of groups must be a divisor of the number of blocks
    // - the block with first elt must be a multiple of the number of groups
    if (firstInGr % mbr.grSize || numOfBlocks % mbr.numOfGr || grStart % mbr.numOfGr) {
      DBG(dbgs() << "    -- Invalid size of multi-block range " << mbr << ".\n");
      return std::nullopt;
    }
  }
  SmallVector<int, DEF_PICK_SIZE> allPicks(vectorLength, 0);
  for (const MBRange &range : mbrs) {
    for (int n = 0; n < range.size(); ++n) {
      int &alreadyPicked = allPicks[range[n]];
      if (alreadyPicked) {
        DBG(dbgs() << " -- Multi-block ranges overlap.\n");
        return std::nullopt;
      }
      alreadyPicked = 1;
    }
  }
  if (std::find(allPicks.begin(), allPicks.end(), 0) != allPicks.end()) {
    DBG(dbgs() << "    -- Multi-block ranges do not cover the entire vector.\n");
    return std::nullopt;
  }
  DBG(dbgs() << "    -- Multi-block ranges are valid.\n";
      dbgs() << "       -- First MB range is = " << mbrs.front() << ".\n";);
  return std::make_optional(std::move(mbrs));
}

// ===========================================================================

bool LoadData::isValidLoad() const {
  // For this pass we assume the following:
  // 1. We must avoid padding, otherwise the splitting would fail.
  // 2. All parameters are power of two, so that the splitting is possible.
  // 3. The length of the vector is at least 2 (so we don't deal with v1s).
  if (!isPowerOf2_32(vectorLength) || !isPowerOf2_32(blockWidth_E) || !isPowerOf2_32(blockHeight_E) ||
      !isPowerOf2_32(numOfBlocks) || !isPowerOf2_32(scalarBitWidth) || !isPowerOf2_32(elementBitWidth))
    return false;
  if ((blockHeight_E * blockWidth_E * elementBitWidth / 8) % config().sizeOfRegs_B)
    return false; // so we don't deal with padding
  if (scalarBitWidth % elementBitWidth)
    return false;       // so scalarBitLength >= elementBitLength
  if (vectorLength < 2) // We don't want to deal with v1's.
    return false;


  DBG(
      bool ok = true; if (transposed && vnni) ok = false; else if (!transposed and !vnni) {
        unsigned rowBytesPerBlk = ((elementBitWidth / 8) * blockWidth_E);
        if ((rowBytesPerBlk * numOfBlocks) > 64 || rowBytesPerBlk < 4)
          ok = false;
      } else if (transposed) {
        bool isValid64 = (elementBitWidth == 64 && blockHeight_E == 8 &&
                          (blockWidth_E <= 4 || (blockWidth_E == 8 && config().isLegitW8)));
        bool isValid32 = (elementBitWidth == 32 && blockHeight_E <= 32 && blockWidth_E <= 8);
        if (numOfBlocks != 1 || !(isValid32 || isValid64))
          ok = false;
      } else if (vnni) {
        // scalarBitWidth / elementBitWidth is ok since scalarBitLength %
        // elementBitLength == 0.
        bool isValid8 = (elementBitWidth == 8 && blockHeight_E >= 4 && blockWidth_E >= 4);
        bool isValid16 = (elementBitWidth == 16 && blockHeight_E >= 2 && blockWidth_E >= 2 && blockWidth_E <= 32);
        if (!(isValid8 || isValid16))
          ok = false;
      } if (!ok) {
        dbgs() << " -- [ERROR] Load is invalid. Parameters mismatch.\n";
        return false;
      }

      if (config().actualSimd) {
        if (config().actualSimd != SIMD()) {
          dbgs() << " -- [ERROR] Load is invalid. SIMD mismatch.\n";
          return false;
        }
      });

  return true;
}

unsigned LoadData::getMinGroupLength(unsigned atLeastThisLarge) const {
  // There are no lower bounds for straight loads.
  unsigned minGroupLen = 1;

  if (transposed)
    minGroupLen = divideCeil(32, elementBitWidth);

  else if (vnni)
    minGroupLen = divideCeil(32, scalarBitWidth);

  // Minimal block length must be equal or larger than:
  // 1. minGroupLen
  // 2. atLeastThisLarge
  // 3. its size in bytes must be at least equal to the size of 1 GRF to avoid
  // padding.
  // 4. It also must be a power of 2.

  // For 3: the bit width of a load group of size N is N * scalarMemSize_B()
  // so N must be greater or equal than the bit width of 1 GRF.
  minGroupLen = PowerOf2Ceil(std::max(minGroupLen, atLeastThisLarge));
  if (minGroupLen * scalarMemSize_B() < config().sizeOfRegs_B) {
    minGroupLen = PowerOf2Ceil(divideCeil(config().sizeOfRegs_B, scalarMemSize_B()));
  }
  return minGroupLen;
}

std::string LoadData::getBlockLoadName() const {
  return std::string("llvm.genx.GenISA.LSC2DBlockRead.") +
         (1 < vectorLength ? "v" + std::to_string(vectorLength) : std::string()) + "i" + std::to_string(scalarBitWidth);
}

// ===========================================================================

bool Load::readFromLoad(GenIntrinsicInst *inGII) {
  GII = inGII;
  if (!fillBlockData()) {
    DBG(dbgs() << "    -- Could not fill block data.\n");
    return false;
  }
  if (!isValidLoad()) {
    DBG(dbgs() << " -- [ERROR] Load is invalid.\n");
    return false;
  }
  return true;
}

bool Load::tracePicks() {
  trace = std::make_unique<TraceData>();
  if (!trace->tracePicks(GII)) {
    DBG(dbgs() << "    -- Tracing picks failed.\n");
    return false;
  }
  if (trace->picks->empty()) {
    DBG(dbgs() << "    -- Empty picks.\n");
    return false;
  }
  return true;
}

PossibleDims Load::possibleDims() {
  if (!trace || !trace->picks || trace->picks->empty())
    return {};
  std::optional<MBRanges> minSplitOpt = makeGridUniformPicks(*trace->picks, vectorLength, groupLength());
  if (!minSplitOpt || !minSplitOpt->size())
    return {};
  // those are already PowerOf2Ceil'ed:
  unsigned minGrSize = getMinGroupLength(minSplitOpt->front().grSize);
  unsigned minNumOfGr = minSplitOpt->front().numOfGr;
  DBG(dbgs() << "    -- Minimal block size = " << minGrSize << " x " << minNumOfGr << ".\n");

  unsigned stride = minSplitOpt->front().stride();
  PossibleDims dims;

  if (stride) {
    // We support one strided case: stride == 2 for transposed loads.
    // In this case we can split it into two vectors. To distinguish this from the non-strided case
    // we preserve stride == 2 in the resulting dims and set numOfGr == 1.
    if (stride != 2) {
      DBG(dbgs() << " -- [SKIP] Only stride == 2 is supported.\n");
      return {};
    }
    if (!transposed) {
      DBG(dbgs() << " -- [SKIP] Stride == 2 is only supported for transposed loads.\n");
      return {};
    }
    unsigned grSize = vectorLength;
    dims.insert({grSize, 1, stride});
    if ((grSize / 2 >= config().minSplitSize_E) && ((grSize / 2) * scalarMemSize_B() >= config().minSplitSize_B)) {
      dims.insert({grSize / 2, 1, stride});
    }
  } else if (config().allowNonStrided) {
    // For multiple blocks, if dims.numOfGr > 1, the subgroup cannot cover the
    // entire group, i.e., dims.grSize < groupLength(). However, if dims.numOfGr =
    // 1, the group size can be as large as the vector size. For example, 2x2 is a
    // proper subdimension of 4x4, but 4x2 is not, as it is equivalent to 8x1.
    // First, dimensions with a single group.
    for (unsigned grSize = std::max(groupLength() * minNumOfGr, minGrSize); grSize <= vectorLength; grSize *= 2) {
      if (grSize < config().minSplitSize_E || grSize * scalarMemSize_B() < config().minSplitSize_B)
        continue;
      dims.insert({grSize, 1, 0});
    }
    // Next, dimensions with a multiple groups.
    for (unsigned grSize = minGrSize; grSize < groupLength(); grSize *= 2) {
      for (unsigned numOfGr = minNumOfGr; numOfGr <= numOfBlocks; numOfGr *= 2) {
        if (grSize * numOfGr < config().minSplitSize_E ||
            grSize * numOfGr * scalarMemSize_B() < config().minSplitSize_B)
          continue;
        dims.insert({grSize, numOfGr, 0});
      }
    }
  }

  if (dims.empty()) {
    DBG(dbgs() << " -- [SKIP] No possible dimensions (including no split) "
                  "satisfy all the conditions.\n");
  } else if (dims.size() == 1) {
    DBG(dbgs() << " -- [SKIP] No possible splits.\n");
  }
  DBG(dbgs() << "    -- Possible dimensions:\n"; for (const Dims &d : dims) { dbgs() << "       -- " << d << "\n"; });
  return dims;
}

Load &Load::splitFlat(const MBRange &range) {
  unsigned blockStart = range.first / blockHeight_E;
  unsigned blockEnd = range.last() / blockHeight_E;
  vectorLength = static_cast<unsigned>(range.size());
  numOfBlocks = blockEnd - blockStart + 1;
  xOffset_E += static_cast<int>(blockStart * blockWidth_E);
  yOffset_P += static_cast<int>(range.first % blockHeight_E); // old block height
  blockHeight_E = range.size() / numOfBlocks;                 // new block height
  return *this;
}

Load &Load::splitTransposed(const MBRange &range) {
  vectorLength = static_cast<unsigned>(range.size());

  if (range.stride() == 2) {
    blockHeight_E = static_cast<unsigned>(blockHeight_E / 2);
    yOffset_P += static_cast<int>(range.first * blockHeight_E);
  } else {
    IGC_ASSERT_MESSAGE(range.stride() == 0, "Only stride of 2 is supported for transposed loads.");
    blockWidth_E = static_cast<unsigned>(range.size());
    xOffset_E += range.first;
  }

  return *this;
}

Load &Load::splitVNNI(const MBRange &range) {
  unsigned scalarInElts = scalarBitWidth / elementBitWidth;
  unsigned blockHeight_S = blockHeight_E / scalarInElts;
  unsigned blockStart = range.first / blockHeight_S;
  unsigned blockEnd = range.last() / blockHeight_S;
  vectorLength = static_cast<unsigned>(range.size());
  numOfBlocks = blockEnd - blockStart + 1;
  xOffset_E += static_cast<int>(blockStart * blockWidth_E);
  yOffset_P += static_cast<int>((range.first % blockHeight_S) * scalarInElts); // old block height
  blockHeight_E = (range.size() / numOfBlocks) * scalarInElts;                 // new block height
  return *this;
}

std::unique_ptr<Load> Load::splitLoadData(const MBRange &range) {
  unsigned scalarToEltRatio = scalarBitWidth / elementBitWidth; // scalarBitWidth % elementBitWidth == 0 was checked.
  if (range.first % scalarToEltRatio) {
    DBG(dbgs() << " -- [ERROR] Position of the first element does not "
                  "divide the scalar to element ratio.\n");
    return nullptr;
  }

  // We copy GII and AP as well, because we will use them as the insertion point
  // for new instructions. The copy constructor does NOT copy traces.
  std::unique_ptr<Load> ret = std::make_unique<Load>(*this);

  if (!transposed && !vnni) {
    ret->splitFlat(range);
  } else if (transposed) {
    ret->splitTransposed(range);
  } else {
    ret->splitVNNI(range);
  }

  if (!ret->isValidLoad())
    return nullptr;
  return ret;
}

std::unique_ptr<Load> Load::split(const MBRange &range) {
  if (vectorLength == range.size())
    return nullptr;
  std::unique_ptr<Load> ret = splitLoadData(range);
  if (!ret)
    return nullptr;
  DBG(dbgs() << "    -- [OK] Split load created for range " << range << ".\n");
  if (trace)
    ret->trace = trace->pickSubpicksOf(range);
  DBG(dbgs() << "    -- [OK] Subpicks picked.\n");
  return ret;
}

CallInst *Load::putBlockLoad(IRBuilder<> &builder) {
  builder.SetInsertPoint(GII);
  std::array<Value *, NUM_OF_BLOCKLOAD_ARGS> args;
  for (unsigned i = 0; i < args.size(); ++i) {
    args[i] = GII->getArgOperand(i);
  }

  Function *newLoadFun = builder.GetInsertBlock()->getModule()->getFunction(getBlockLoadName());
  if (!newLoadFun) {
    newLoadFun = createFunction(getBlockLoadName(), builder.GetInsertBlock()->getModule(), args,
                                IGCLLVM::FixedVectorType::get(scalarTy, vectorLength), GII->getCalledFunction());
  }

  args[LSC2D_BlockRead::argBlockWidth_inElts] = builder.getInt32(blockWidth_E);
  args[LSC2D_BlockRead::argBlockHeight_inElts] = builder.getInt32(blockHeight_E);
  args[LSC2D_BlockRead::argNumOfBlocks] = builder.getInt32(numOfBlocks);
  Value *valX = createAdd(builder, GII->getArgOperand(LSC2D_BlockRead::argXOffset_inElts), xOffset_E);
  Value *valY = createAdd(builder, GII->getArgOperand(LSC2D_BlockRead::argYOffset_inPitches), yOffset_P);
  args[LSC2D_BlockRead::argXOffset_inElts] = valX;
  args[LSC2D_BlockRead::argYOffset_inPitches] = valY;

  CallInst *newGII = builder.CreateCall(newLoadFun->getFunctionType(), newLoadFun, args);
  GII = cast<GenIntrinsicInst>(newGII);
  return newGII;
}

bool Load::fillBlockData() {
  if (!(isa<ConstantInt>(GII->getArgOperand(LSC2D_BlockRead::argBlockHeight_inElts)) &&
        isa<ConstantInt>(GII->getArgOperand(LSC2D_BlockRead::argBlockWidth_inElts)) &&
        isa<ConstantInt>(GII->getArgOperand(LSC2D_BlockRead::argSizeInBits)) &&
        isa<ConstantInt>(GII->getArgOperand(LSC2D_BlockRead::argNumOfBlocks)) &&
        isa<ConstantInt>(GII->getArgOperand(LSC2D_BlockRead::argIsTranspose)) &&
        isa<ConstantInt>(GII->getArgOperand(LSC2D_BlockRead::argIsVNNI)))) {
    return false;
  }

  std::tie(scalarTy, vectorLength) = getScalarTypeAndSize(GII);
  if (!scalarTy)
    return false;
  // For block loads actual offsets may not be constant.
  // We only track the relative offsets in this case.
  xOffset_E = 0;
  yOffset_P = 0;
  elementBitWidth = getArgZ(GII, LSC2D_BlockRead::argSizeInBits);
  blockWidth_E = getArgZ(GII, LSC2D_BlockRead::argBlockWidth_inElts);
  blockHeight_E = getArgZ(GII, LSC2D_BlockRead::argBlockHeight_inElts);
  numOfBlocks = getArgZ(GII, LSC2D_BlockRead::argNumOfBlocks);
  transposed = getArgZ(GII, LSC2D_BlockRead::argIsTranspose);
  vnni = getArgZ(GII, LSC2D_BlockRead::argIsVNNI);
  // We don't do anything with cache flags or the read address, so we will just
  // copy their values in putBlockLoad.
  scalarBitWidth = scalarTy->getBitWidth();

  return true;
}

void Load::removeOldInstructions() {
  if (trace)
    trace->removeOldInstructions(); // This already contains the GII call.
}

// ==========================================================================

namespace IGC::LS {

struct LoadSplitter::Impl {
  static std::unique_ptr<LoadSplitter::Impl> Create(Function *inF, CodeGenContext *inCGC,
                                                    IGCLivenessAnalysisRunner *inRPE);

  bool isRPHigh(BasicBlock *BB);
  PossibleDims possibleDims(GenIntrinsicInst *GII);
  bool split(GenIntrinsicInst *GII, Dims dims);
  bool splitAllToSmallest(BasicBlock *BB);

private:
  SmallDenseMap<GenIntrinsicInst *, std::unique_ptr<Load>, DEF_NUM_OF_LOADS> blockLoadsMap;
  SmallDenseMap<GenIntrinsicInst *, Dims, DEF_NUM_OF_LOADS> dimsMap;

  using SplitLoads = SmallVector<std::unique_ptr<Load>, DEF_NUM_OF_LOADS>;
  bool processBlockLoad(GenIntrinsicInst *GII);
  SplitLoads splitBlockLoad(Load &load, const MBRanges &splits);
  bool putBlockLoad(Load &load, const std::string &nameExt = std::string());
};

// ==========================================================================

std::unique_ptr<LoadSplitter> LoadSplitter::Create(Function *inF, CodeGenContext *inCGC,
                                                   IGCLivenessAnalysisRunner *inRPE) {
  std::unique_ptr<LoadSplitter> ret = std::unique_ptr<LoadSplitter>(new LoadSplitter());
  ret->impl = Impl::Create(inF, inCGC, inRPE);
  if (!ret->impl) {
    return nullptr;
  }
  return ret;
}

bool LoadSplitter::isRPHigh(BasicBlock *BB) { return impl->isRPHigh(BB); }

PossibleDims LoadSplitter::possibleDims(GenIntrinsicInst *GII) { return impl->possibleDims(GII); }

bool LoadSplitter::splitAllToSmallest(BasicBlock *BB) { return impl->splitAllToSmallest(BB); }

bool LoadSplitter::split(GenIntrinsicInst *GII, Dims dims) { return impl->split(GII, dims); }

// ==========================================================================

std::unique_ptr<LoadSplitter::Impl> LoadSplitter::Impl::Create(Function *inF, CodeGenContext *inCGC,
                                                               IGCLivenessAnalysisRunner *inRPE) {
  std::unique_ptr<LoadSplitter::Impl> ret = std::unique_ptr<LoadSplitter::Impl>(new LoadSplitter::Impl());
  if (!config().initialize(inF, inCGC, inRPE)) {
    return nullptr;
  }
  return ret;
}

bool LoadSplitter::Impl::isRPHigh(BasicBlock *BB) {
  int regPressure = config().RPE->getMaxRegCountForBB(*BB, config().SIMD()) * config().sizeOfRegs_B;
  DBG(dbgs() << " -- Reg Pressure = " << regPressure << " B, threshold = " << config().splitThreshold_B << " B.\n");
  if (regPressure <= config().splitThreshold_B) {
    DBG(dbgs() << " [SKIP] Register pressure below threshold.\n");
    return false;
  }
  DBG(dbgs() << " [OK] Reg pressure high.\n");
  return true;
}

bool LoadSplitter::Impl::processBlockLoad(GenIntrinsicInst *GII) {
  if (!GII)
    return false;
  auto ptr = blockLoadsMap.find(GII);
  if (ptr != blockLoadsMap.end())
    return true;

  std::unique_ptr<Load> load = std::make_unique<Load>();
  if (!load->readFromLoad(GII)) {
    DBG(dbgs() << " -- [SKIP] Invalid block load.\n");
    return false;
  }
  DBG(dbgs() << " -- [OK] Valid block load.\n");
  if (!load->tracePicks()) {
    DBG(dbgs() << " -- [SKIP] Invalid picks.\n");
    return false;
  }
  DBG(dbgs() << " -- [OK] Picks are valid:\n";
      for (const Pick &pick : *load->trace->picks) { dbgs() << "    -- " << pick << "\n"; });
  blockLoadsMap[GII] = std::move(load);
  return true;
}

PossibleDims LoadSplitter::Impl::possibleDims(GenIntrinsicInst *GII) {
  if (!GII)
    return {};
  DBG(dbgs() << "\nPossible split dimensions for: " << *GII << ".\n");
  PossibleDims dims;
  if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead) {
    if (!processBlockLoad(GII))
      return {};
    Load &load = *blockLoadsMap[GII];
    DBG(dbgs() << " -- load = " << *load.GII << ".\n");
    dims = load.possibleDims();
  } else {
    DBG(dbgs() << " [ERROR] The intrinsic: " << *GII << " is not a load.\n");
    return {};
  }
  return dims;
}

LoadSplitter::Impl::SplitLoads LoadSplitter::Impl::splitBlockLoad(Load &load, const MBRanges &splits) {
  unsigned numOfSplits = load.vectorLength / splits.front().size();
  if (numOfSplits == 1) {
    DBG(dbgs() << " -- [SKIP] No need to split the block load.\n");
    return {};
  }
  SplitLoads splitLoads;
  splitLoads.reserve(numOfSplits);
  for (unsigned m = 0; m < numOfSplits; ++m) {
    std::unique_ptr<Load> splitLoad = load.split(splits[m]);
    if (!splitLoad) {
      DBG(dbgs() << " -- [ERROR] Split is not valid.\n");
      return {};
    }
    if (splitLoad->trace->picks->empty()) {
      DBG(dbgs() << " -- [SKIP] The new load has no users.\n");
      continue;
    }
    splitLoads.push_back(std::move(splitLoad));
  }
  return splitLoads;
}

bool LoadSplitter::Impl::putBlockLoad(Load &load, const std::string &nameExt) {
  IRBuilder<> builder(load.GII);
  CallInst *V = load.putBlockLoad(builder);
  DBG(dbgs() << "    -- [OK] New load put: " << *V << ".\n");
  if (load.GII->hasName()) {
    V->setName(load.GII->getName() + nameExt);
  }
  load.trace->putPicks(builder, V);
  DBG(dbgs() << "    -- [OK] Picks put.\n");
  return true;
}

bool LoadSplitter::Impl::split(GenIntrinsicInst *GII, Dims dims) {
  if (!GII)
    return false;
  if (GII->getIntrinsicID() != GenISAIntrinsic::GenISA_LSC2DBlockRead) {
    DBG(dbgs() << " -- [ERROR] The intrinsic: " << *GII << " is not a load.\n");
    return false;
  }
  DBG(dbgs() << " -- Block load: " << *GII << "\n");
  DBG(dbgs() << "    -- Dimensions: " << dims << "\n");

  auto doPicksFit = [](const MBRanges &mbrs, const Picks &picks) -> bool {
    for (unsigned r = 0; r < mbrs.size(); ++r) {
      const MBRange &mbr = mbrs[r];
      for (const Pick &pick : picks) {
        if (mbr.containsOrExcludes(pick, false) == MBRange::Containment::Intersects) {
          return false;
        }
      }
    }
    return true;
  };

  auto checkNumOfScalarsPerWI = [](const MBRanges &mbrs, const Load &load) -> bool {
    // For strided loads, we can only split if the original load had 2 elements per WI
    // This ensures that:
    // 1. The load is transposed
    // 2. Each work-item gets exactly 2 elements (blockHeight_E / vectorLength == 2)
    // 3. We're splitting into exactly 2 parts with stride 2
    // 4. The SIMD size matches the vectorLength

    if (!load.transposed) {
      DBG(dbgs() << "    -- [SKIP] Load is not transposed.\n");
      return false;
    }

    if (mbrs.front().stride() != 2 || mbrs.size() != 2) {
      DBG(dbgs() << "    -- [SKIP] Must have exactly 2 splits with stride 2.\n");
      return false;
    }

    unsigned scalarsPerWI = load.blockHeight_E / load.vectorLength;
    if (scalarsPerWI != 2) {
      DBG(dbgs() << "    -- [SKIP] Each work-item must get exactly 2 elements (got " << scalarsPerWI << ").\n");
      return false;
    }

    // In this case it's essential that we know the SIMD, so
    // don't do the transformation if the SIMD size is not known
    if (!config().actualSimd) {
      DBG(dbgs() << "    -- [SKIP] Actual SIMD size is unknown.\n");
      return false;
    }

    if (config().actualSimd != load.vectorLength) {
      DBG(dbgs() << "    -- [SKIP] SIMD mismatch: actual SIMD = " << config().actualSimd
                 << ", vectorLength = " << load.vectorLength << ".\n");
      return false;
    }

    DBG(dbgs() << "    -- [OK] Strided load validation passed.\n");
    return true;
  };

  if (!processBlockLoad(GII))
    return false;
  Load &load = *blockLoadsMap[GII];
  if (load.vectorLength <= dims.size()) {
    DBG(dbgs() << " -- [SKIP] Nothing to split.\n");
    return false;
  }
  std::optional<MBRanges> splitsOpt =
      makeUniform(dims.grSize, dims.stride ? 1 : load.groupLength(), dims.numOfGr, load.vectorLength, dims.stride);
  if (!splitsOpt) {
    DBG(dbgs() << " -- [ERROR] Split is not valid.\n");
    return false;
  }
  MBRanges &splits = *splitsOpt;
  if (!doPicksFit(splits, *load.trace->picks)) {
    DBG(dbgs() << " -- [ERROR] Picks do not fit the splits.\n");
    return false;
  }
  if (dims.stride && !checkNumOfScalarsPerWI(splits, load)) {
    DBG(dbgs() << " -- [ERROR] Cannot perform split of a strided load.\n");
    return false;
  }
  auto newLoads = splitBlockLoad(load, splits);
  if (newLoads.empty()) {
    DBG(dbgs() << " -- [ERROR] Splitting failed.\n");
    return false;
  }
  for (unsigned n = 0; n < newLoads.size(); ++n) {
    putBlockLoad(*newLoads[n], ".split." + std::to_string(n));
  }
  load.removeOldInstructions();
  return true;
}

bool LoadSplitter::Impl::splitAllToSmallest(BasicBlock *BB) {
  DBG(dbgs() << "\nNew BB: " << BB->getName() << ".\n");

  if (!config().ignoreSplitThreshold && !isRPHigh(BB)) {
    DBG(dbgs() << " [SKIP] Register pressure below threshold.\n");
    return false;
  }

  blockLoadsMap.clear();
  dimsMap.clear();

  for (auto I = BB->begin(); I != BB->end(); ++I) {
    GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(&*I);
    if (!GII)
      continue;
    if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead) {
      DBG(dbgs() << "\nNew block load: " << *GII << ".\n");
      if (!processBlockLoad(GII)) {
        DBG(dbgs() << " [SKIP] Invalid block load.\n");
        continue;
      }
      DBG(dbgs() << " [OK] Load is potentially splittable.\n");
    }
  }
  DBG(dbgs() << "\nLoads tracing done.\n");
  if (blockLoadsMap.empty()) {
    DBG(dbgs() << " [SKIP] No loads left to split.\n");
    return false;
  }
  DBG(dbgs() << " -- Number of block loads = " << blockLoadsMap.size() << ".\n");

  DBG(dbgs() << "\nCalculating possible splits:\n");
  for (auto &[GII, loadPtr] : blockLoadsMap) {
    DBG(dbgs() << " -- Block load: " << *GII << ".\n");
    PossibleDims dims = possibleDims(GII);
    if (dims.empty())
      continue;
    dimsMap[GII] = *std::min_element(dims.begin(), dims.end());
  }
  if (dimsMap.empty()) {
    DBG(dbgs() << " [SKIP] No splits left.\n");
    return false;
  }
  DBG(dbgs() << " [OK] Smallest possible splits found.\n");

  bool codeChanges = false;
  DBG(dbgs() << "\nSplitting loads:\n");
  for (auto &[GII, dims] : dimsMap) {
    if (!split(GII, dims)) {
      DBG(dbgs() << " [SKIP] Split failed.\n");
      continue;
    }
    codeChanges = true;
  }
  DBG(dbgs() << " [OK] Splitting done.\n");
  return codeChanges;
}

} // namespace IGC::LS

// ==========================================================================

namespace {
class SplitLoads : public FunctionPass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  virtual StringRef getPassName() const override { return "SplitLoads"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<IGCLivenessAnalysis>();
    AU.setPreservesCFG();
  }

  SplitLoads();
  SplitLoads(const SplitLoads &) = delete;
  SplitLoads &operator=(const SplitLoads &) = delete;

  virtual bool runOnFunction(Function &F) override;

private:
  std::unique_ptr<LoadSplitter> loadSplitter = nullptr;
};
} // namespace

char SplitLoads::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-split-loads"
#define PASS_DESCRIPTION "Splits 2D LSC block loads into smaller chunks"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SplitLoads, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_END(SplitLoads, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

FunctionPass *IGC::createSplitLoadsPass() { return new SplitLoads(); }

SplitLoads::SplitLoads() : FunctionPass(ID) { initializeSplitLoadsPass(*PassRegistry::getPassRegistry()); }

bool SplitLoads::runOnFunction(Function &F) {
  if (!config().enableLoadSplitting || skipFunction(F)) {
    return false;
  }
  loadSplitter = LoadSplitter::Create(&F, getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                                      &getAnalysis<IGCLivenessAnalysis>().getLivenessRunner());
  if (!loadSplitter) {
    return false;
  }

  DBG(dbgs() << "\nSPLITLOADS ON: " << F.getName() << "\n");

  [[maybe_unused]] auto pad = [](const std::string &s, size_t len) -> std::string {
    return s.size() < len ? s + std::string(len - s.size(), ' ') : s.substr(0, len);
  };

  bool codeChanged = false;
  for (BasicBlock &BB : F) {
    bool splitterChangesCode = loadSplitter->splitAllToSmallest(&BB);
    codeChanged |= splitterChangesCode;
    if (splitterChangesCode) {
      DBG(dbgs() << "BB: " << pad(BB.getName().str(), 20) << " : SPLIT SUCCESSFUL.\n");
    } else {
      DBG(dbgs() << "BB: " << pad(BB.getName().str(), 20) << " : NO SPLITS.\n");
    }
  }
  DBG(dbgs() << "\n");

  return codeChanged;
}

#undef DEBUG_TYPE
