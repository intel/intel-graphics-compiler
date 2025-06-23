/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../AdaptorCommon/LivenessUtils/AllocationLivenessAnalyzer.h"
#include "llvm/PassRegistry.h"
#include "RTBuilder.h"
#include "RTStackFormat.h"

namespace llvm {
class Instruction;
class Function;
class PassRegistry;
class PHINode;
class StructType;
class Value;
} // namespace llvm

namespace IGC {
class CodeGenContext;
class InlineRaytracing : public AllocationLivenessAnalyzer {
public:
  static char ID;

  InlineRaytracing();

  bool runOnFunction(llvm::Function &F) override;
  void getAdditionalAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  llvm::StringRef getPassName() const override { return "InlineRaytracing"; }

private:
  using LivenessDataMap = llvm::MapVector<llvm::Instruction *, LivenessData>;
  CodeGenContext *m_pCGCtx = nullptr;
  llvm::StructType *m_RQObjectType = nullptr;
  uint32_t m_numSlotsUsed = 0;

  void LowerIntrinsics(llvm::Function &F);
  bool LowerAllocations(llvm::Function &F);
  LivenessDataMap AnalyzeLiveness(llvm::Function &F, llvm::DominatorTree &DT,
                                  llvm::LoopInfo &LI);
  void AssignSlots(llvm::Function &F, const LivenessDataMap &livenessDataMap);
  void HandleOptimizationsAndSpills(llvm::Function &F,
                                    LivenessDataMap &livenessDataMap,
                                    llvm::DominatorTree &DT,
                                    llvm::LoopInfo &LI);
  void LowerSlotAssignments(llvm::Function &F);
  void LowerStackPtrs(llvm::Function &F);

  enum RQTraceRayCtrl : uint8_t {
    TRACE_RAY_INITIAL =
        static_cast<uint8_t>(RTStackFormat::TraceRayCtrl::TRACE_RAY_INITIAL),
    TRACE_RAY_INSTANCE =
        static_cast<uint8_t>(RTStackFormat::TraceRayCtrl::TRACE_RAY_INSTANCE),
    TRACE_RAY_COMMIT =
        static_cast<uint8_t>(RTStackFormat::TraceRayCtrl::TRACE_RAY_COMMIT),
    TRACE_RAY_CONTINUE =
        static_cast<uint8_t>(RTStackFormat::TraceRayCtrl::TRACE_RAY_CONTINUE),
    TRACE_RAY_DONE = UINT8_MAX
  };

  enum Functions : uint8_t {
    GET_STACK_POINTER_FROM_RQ_OBJECT,
    GET_RQ_HANDLE_FROM_RQ_OJECT,
    CREATE_RQ_OBJECT,
    NUM_FUNCTIONS
  };

  enum CommittedDataLocation : bool { CommittedHit = 0, PotentialHit = 1 };

  enum RQPackedDataBitOffsets : uint8_t {
    Start = 0,
    TraceRayCtrl = 8,
    CommittedStatus = 10,
    CandidateType = 11,
    HasAcceptHitAndEndSearchFlag = 12,
    CommittedDataLocation = 13,
  };

  std::array<llvm::Function *, NUM_FUNCTIONS> m_Functions = {nullptr};

  // helper functions used when lowering intrinsics
  llvm::Value *getAtIndexFromRayQueryObject(llvm::RTBuilder &IRB,
                                            llvm::Value *rqObject, uint32_t i) {
    return IRB.CreateInBoundsGEP(m_RQObjectType, rqObject,
                                 {IRB.getInt32(0), IRB.getInt32(i)});
  }

  llvm::Value *getGlobalBufferPtr(llvm::RTBuilder &IRB, llvm::Value *rqObject) {
    auto *slot = IRB.CreateLoad(IRB.getInt32Ty(),
                                getAtIndexFromRayQueryObject(IRB, rqObject, 0));
    return IRB.getGlobalBufferPtrForSlot(ADDRESS_SPACE_CONSTANT, slot);
  }

  struct UnpackedData {
    llvm::Value *TraceRayCtrl = nullptr;
    llvm::Value *CommittedStatus = nullptr;
    llvm::Value *CandidateType = nullptr;
    llvm::Value *HasAcceptHitAndEndSearchFlag = nullptr;
    llvm::Value *CommittedDataLocation = nullptr;
  };

  UnpackedData getPackedData(llvm::RTBuilder &IRB, llvm::Value *rqObject) {
    UnpackedData data;

    auto *packedData = IRB.CreateLoad(
        IRB.getInt32Ty(), getAtIndexFromRayQueryObject(IRB, rqObject, 1));

    auto iterator = {
        std::make_tuple(&data.TraceRayCtrl, RQPackedDataBitOffsets::Start,
                        RQPackedDataBitOffsets::TraceRayCtrl),
        std::make_tuple(&data.CommittedStatus,
                        RQPackedDataBitOffsets::TraceRayCtrl,
                        RQPackedDataBitOffsets::CommittedStatus),
        std::make_tuple(&data.CandidateType,
                        RQPackedDataBitOffsets::CommittedStatus,
                        RQPackedDataBitOffsets::CandidateType),
        std::make_tuple(&data.HasAcceptHitAndEndSearchFlag,
                        RQPackedDataBitOffsets::CandidateType,
                        RQPackedDataBitOffsets::HasAcceptHitAndEndSearchFlag),
        std::make_tuple(&data.CommittedDataLocation,
                        RQPackedDataBitOffsets::HasAcceptHitAndEndSearchFlag,
                        RQPackedDataBitOffsets::CommittedDataLocation),
    };

    for (auto [field, bitoffset, nextbitoffset] : iterator) {
      *field =
          IRB.CreateAnd({IRB.CreateLShr(packedData, bitoffset),
                         IRB.getInt32((1 << (nextbitoffset - bitoffset)) - 1)});
    }

    return data;
  }

  void setPackedData(llvm::RTBuilder &IRB, llvm::Value *rqObject,
                     const UnpackedData &data) {
    auto iterator = {
        std::make_tuple(&data.TraceRayCtrl, RQPackedDataBitOffsets::Start,
                        RQPackedDataBitOffsets::TraceRayCtrl),
        std::make_tuple(&data.CommittedStatus,
                        RQPackedDataBitOffsets::TraceRayCtrl,
                        RQPackedDataBitOffsets::CommittedStatus),
        std::make_tuple(&data.CandidateType,
                        RQPackedDataBitOffsets::CommittedStatus,
                        RQPackedDataBitOffsets::CandidateType),
        std::make_tuple(&data.HasAcceptHitAndEndSearchFlag,
                        RQPackedDataBitOffsets::CandidateType,
                        RQPackedDataBitOffsets::HasAcceptHitAndEndSearchFlag),
        std::make_tuple(&data.CommittedDataLocation,
                        RQPackedDataBitOffsets::HasAcceptHitAndEndSearchFlag,
                        RQPackedDataBitOffsets::CommittedDataLocation),
    };

    llvm::Value *packedData = IRB.getInt32(0);

    for (auto [field, bitoffset, nextbitoffset] : iterator) {
      packedData = IRB.CreateOr(
          packedData,
          IRB.CreateShl(
              IRB.CreateAnd(*field, (1 << (nextbitoffset - bitoffset)) - 1),
              bitoffset));
    }

    IRB.CreateStore(packedData, getAtIndexFromRayQueryObject(IRB, rqObject, 1));
  }

  llvm::RTBuilder::SyncStackPointerVal *getStackPtr(llvm::RTBuilder &IRB,
                                                    llvm::Value *rqObject) {
    return static_cast<llvm::RTBuilder::SyncStackPointerVal *>(
        llvm::cast<llvm::Value>(IRB.CreateCall(
            m_Functions[GET_STACK_POINTER_FROM_RQ_OBJECT], rqObject)));
  }

  void EmitPreTraceRayFence(llvm::RTBuilder &IRB, llvm::Value *rqObject);
  void InsertCacheControl(llvm::RTBuilder &IRB,
                          llvm::RTBuilder::SyncStackPointerVal *stackPtr);
  void StopAndStartRayquery(llvm::RTBuilder &IRB, llvm::Instruction *I,
                            llvm::Value *rqObject, bool doSpillFill,
                            bool doRQCheckRelease);

};

llvm::Pass *createInlineRaytracing();
} // namespace IGC

void initializeInlineRaytracingPass(llvm::PassRegistry &);
