/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "IGCLivenessAnalysis.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"

#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/IR/Function.h"

#include <fstream>
#include <queue>
#include <regex>

char IGCLivenessAnalysis::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "igc-df-liveness"
#define PASS_DESCRIPTION2                                                                                              \
  "computes full dataflow liveness analysis & and register pressure "                                                  \
  "estimator"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 true
IGC_INITIALIZE_PASS_BEGIN(IGCLivenessAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(IGCLivenessAnalysis, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
#define PRINT(str) llvm::errs() << str

IGCLivenessAnalysis::IGCLivenessAnalysis() : FunctionPass(ID) {
  initializeIGCLivenessAnalysisPass(*PassRegistry::getPassRegistry());
};

unsigned int IGCLivenessAnalysisBase::registerSizeInBytes() {
  if (CGCtx->platform.isProductChildOf(IGFX_PVC))
    return 64;
  return 32;
}

SIMDMode IGCLivenessAnalysisBase::bestGuessSIMDSize(Function *F) {
  switch (IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth)) {
  case 0:
    break;
  case 8:
    return SIMDMode::SIMD8;
  case 16:
    return SIMDMode::SIMD16;
  case 32:
    return SIMDMode::SIMD32;
  }

  // if we can find metadata with stipulations how we should compile we use it
  if (F && MDUtils->findFunctionsInfoItem(F) != MDUtils->end_FunctionsInfo()) {
    IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = MDUtils->getFunctionsInfoItem(F);
    unsigned SimdSize = funcInfoMD->getSubGroupSize()->getSIMDSize();
    if (SimdSize)
      return lanesToSIMDMode(SimdSize);
  }

  // rule for pvc
  if (CGCtx->platform.isProductChildOf(IGFX_PVC)) {
    bool abortOnSpills =
        IGC_GET_FLAG_VALUE(AllowSIMD16DropForXE2Plus) && (CGCtx->platform.isCoreXE2() || CGCtx->platform.isCoreXE3());
    auto FG = FGA ? FGA->getGroup(F) : nullptr;
    bool hasStackCall = (FG && FG->hasStackCall()) || (F && F->hasFnAttribute("visaStackCall"));
    bool isIndirectGroup = FG && FGA->isIndirectCallGroup(FG);
    bool hasSubroutine = FG && !FG->isSingle() && !hasStackCall && !isIndirectGroup;
    if (abortOnSpills || hasSubroutine) {
      return SIMDMode::SIMD16;
    }
    return SIMDMode::SIMD32;
  }

  return SIMDMode::SIMD8;
}

ValueSet IGCLivenessAnalysisBase::getDefs(llvm::BasicBlock &BB) {

  ValueSet &BBIn = In[&BB];
  ValueSet &BBOut = Out[&BB];

  ValueSet Difference;
  for (auto *elem : BBOut)
    if (!BBIn.count(elem))
      Difference.insert(elem);

  return Difference;
}

// we scan through all successors and merge their INSETs as our OUTSET
void IGCLivenessAnalysisBase::combineOut(llvm::BasicBlock *BB) {
  ValueSet *OutSet = &Out[BB];
  for (llvm::succ_iterator SI = llvm::succ_begin(BB), SE = llvm::succ_end(BB); SI != SE; ++SI) {
    // for every successor we take all of it's IN values
    // and the PHI values that are coming from our BB
    OutSet->insert(In[*SI].begin(), In[*SI].end());
  }
}

unsigned int computeSizeInBytes(Value *V, unsigned int SIMD, WIAnalysisRunner *WI, const DataLayout &DL) {

  // when we check size of operands, this check is redundant
  // but allows for a nicer code
  bool NoRetVal = V->getType()->isVoidTy();
  if (NoRetVal)
    return 0;

  auto Type = V->getType();
  unsigned int TypeSizeInBits = (unsigned int)DL.getTypeSizeInBits(Type);
  unsigned int Multiplier = SIMD;
  if (WI && WI->isUniform(V))
    Multiplier = 1;
  unsigned int SizeInBytes = TypeSizeInBits * Multiplier / 8;
  return SizeInBytes;
}

unsigned int IGCLivenessAnalysisBase::addOperandsToSet(llvm::Instruction *Inst, ValueSet &Set, unsigned int SIMD,
                                                       WIAnalysisRunner *WI, const DataLayout &DL) {

  // we do not process PHI's there
  auto Phi = llvm::dyn_cast<llvm::PHINode>(Inst);
  if (Phi)
    return 0;

  // do not process debug instructions and lifetimehints in any way
  if (Inst->isDebugOrPseudoInst() || Inst->isLifetimeStartOrEnd())
    return 0;

  unsigned int ResultSizeInBytes = 0;

  for (auto &Op : Inst->operands()) {
    llvm::Value *V = Op.get();
    // We are counting only instructions right now
    // potetntially we should also count globals, but
    // we defintely shouldn't count:
    // br label %bb1 (basic block names)
    // call %functionName (function names)
    // add %a, 1 (constants)
    if (!(llvm::isa<llvm::Instruction>(V) || llvm::isa<llvm::Argument>(V)))
      continue;

    unsigned int SizeOfOperand = computeSizeInBytes(V, SIMD, WI, DL);
    if (!Set.count(V))
      ResultSizeInBytes += SizeOfOperand;

    Set.insert(V);
  }

  return ResultSizeInBytes;
}

void IGCLivenessAnalysisBase::addNonLocalOperandsToSet(llvm::Instruction *Inst, ValueSet &Set) {

  // do not process debug instructions and lifetimehints in any way
  if (Inst->isDebugOrPseudoInst() || Inst->isLifetimeStartOrEnd())
    return;

  for (auto &Op : Inst->operands()) {
    llvm::Value *V = Op.get();
    // We are counting only instructions right now
    // potetntially we should also count globals, but
    // we defintely shouldn't count:
    // br label %bb1 (basic block names)
    // call %functionName (function names)
    // add %a, 1 (constants)
    Instruction *I = dyn_cast<Instruction>(V);
    bool IsInstruction = I != nullptr;
    bool OperandInDifferentBB = IsInstruction && (I->getParent() != Inst->getParent());
    bool IsArgument = !IsInstruction && llvm::isa<llvm::Argument>(V);
    if (OperandInDifferentBB || IsArgument) {
      Set.insert(V);
    }
  }
}

// idea is simple, each predecessor block that converges into our block
// has its own set of PHI values, that it has to deliver
// so we take values that are coming from each block
// and add them to their OUT set directly
void IGCLivenessAnalysisBase::addToPhiSet(llvm::PHINode *Phi, PhiSet *InPhiSet) {
  for (auto BB : Phi->blocks()) {
    auto ValueFromOurBlock = Phi->getIncomingValueForBlock(BB);
    auto *OutSet = &Out[BB];
    if (llvm::isa<llvm::Constant>(ValueFromOurBlock))
      continue;
    OutSet->insert(ValueFromOurBlock);
    // for visualization purposes also add them to INPHI set
    // this way it will be easier to scan them and print
    (*InPhiSet)[BB].insert(ValueFromOurBlock);
  }
}

// scan through block in reversed order and add each operand
// into IN block while deleting defined values
void IGCLivenessAnalysisBase::processBlock(llvm::BasicBlock *BB, ValueSet &Set, PhiSet *PhiSet) {
  for (auto RI = BB->rbegin(), RE = BB->rend(); RI != RE; ++RI) {
    llvm::Instruction *Inst = &(*RI);
    Set.erase(Inst);

    auto Phi = llvm::dyn_cast<llvm::PHINode>(Inst);
    if (Phi) {
      addToPhiSet(Phi, PhiSet);
      continue;
    }
    addNonLocalOperandsToSet(Inst, Set);
  }
}

void IGCLivenessAnalysisBase::livenessAnalysis(llvm::Function &F, BBSet *StartBBs) {
  std::queue<llvm::BasicBlock *> Worklist;

  if (StartBBs != nullptr) {
    // If StartBBs are provided we know that only these BBs could be changed
    // Add only them to the initial Worklist
    for (BasicBlock *BB : *StartBBs)
      Worklist.push(BB);
  } else {
    // Start with adding all BBs to the Worklist
    // to make sure In set is populated for every BB
    for (auto BBIt = IGCLLVM::rbegin(&F); BBIt != IGCLLVM::rend(&F); ++BBIt) {
      Worklist.push(&*BBIt);
    }
  }

  while (!Worklist.empty()) {

    llvm::BasicBlock *BB = Worklist.front();
    Worklist.pop();

    ValueSet *InSet = &In[BB];
    ValueSet *OutSet = &Out[BB];
    PhiSet *InPhiSet = &InPhi[BB];

    combineOut(BB);

    unsigned int SizeBefore = InSet->size();
    unsigned int SizeBeforePhi = InPhiSet->size();

    // this should be a copy, everything that should go OUT
    // minus what is defined and what was intermediate, should go IN
    // so we are feeding copy of the OUT as IN, and delete everything
    // that was defined in the block while processing it
    *InSet = *OutSet;
    processBlock(BB, *InSet, InPhiSet);

    // sets can only grow monotonically, updated set is a bigger set
    bool IsSetChanged = InSet->size() != SizeBefore;
    bool IsPhiSetChanged = InPhiSet->size() != SizeBeforePhi;
    if (IsSetChanged || IsPhiSetChanged)
      for (auto *Pred : predecessors(BB))
        Worklist.push(Pred);
  }
}

unsigned int IGCLivenessAnalysisBase::estimateSizeInBytes(ValueSet &Set, Function &F, unsigned int SIMD,
                                                          WIAnalysisRunner *WI) {
  const DataLayout &DL = F.getParent()->getDataLayout();

  unsigned int Result = 0;
  for (auto El : Set) {
    unsigned int SizeInBytes = computeSizeInBytes(El, SIMD, WI, DL);
    Result += SizeInBytes;
  }
  return Result;
}

void IGCLivenessAnalysisBase::collectPressureForBB(llvm::BasicBlock &BB, InsideBlockPressureMap &BBListing,
                                                   unsigned int SIMD, WIAnalysisRunner *WI) {

  const DataLayout &DL = BB.getParent()->getParent()->getDataLayout();
  ValueSet &BBOut = Out[&BB];
  // this should be a copy
  ValueSet BBSet = BBOut;

  unsigned int SizeInBytes = estimateSizeInBytes(BBSet, *BB.getParent(), SIMD, WI);

  for (auto RI = BB.rbegin(), RE = BB.rend(); RI != RE; ++RI) {

    llvm::Instruction *Inst = &(*RI);

    unsigned int SizeUpdate = 0;
    SizeUpdate = addOperandsToSet(Inst, BBSet, SIMD, WI, DL);

    BBListing[Inst] = SizeInBytes;
    SizeInBytes += SizeUpdate;

    if (!BBSet.count(Inst))
      continue;
    unsigned int InstSizeInBytes = computeSizeInBytes(Inst, SIMD, WI, DL);
    SizeInBytes -= InstSizeInBytes;
    BBSet.erase(Inst);
  }
}

bool IGCLivenessAnalysis::runOnFunction(llvm::Function &F) {
  auto *FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
  auto *MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto *CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  LivenessRunner = IGCLivenessAnalysisRunner(CGCtx, MDUtils, FGA);
  LivenessRunner.livenessAnalysis(F, nullptr);
  return false;
}

char IGCRegisterPressurePrinter::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG1 "igc-pressure-printer"
#define PASS_DESCRIPTION1 "prints register pressure estimation"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(IGCRegisterPressurePrinter, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCFunctionExternalRegPressureAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
IGC_INITIALIZE_PASS_END(IGCRegisterPressurePrinter, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
#define PRINT(str) llvm::errs() << str

IGCRegisterPressurePrinter::IGCRegisterPressurePrinter() : FunctionPass(ID) {
  initializeIGCRegisterPressurePrinterPass(*PassRegistry::getPassRegistry());
};

IGCRegisterPressurePrinter::IGCRegisterPressurePrinter(const std::string &FileName) : FunctionPass(ID) {
  initializeIGCRegisterPressurePrinterPass(*PassRegistry::getPassRegistry());
  DumpFileName = FileName;
  DumpToFile = true;
};

void IGCRegisterPressurePrinter::printInstruction(llvm::Instruction *Inst, std::string &Str) {
  llvm::raw_string_ostream rso(Str);
  Inst->print(rso, false);
  rso << "\n";
}

void IGCRegisterPressurePrinter::printIntraBlock(llvm::BasicBlock &BB, std::string &Output,
                                                 InsideBlockPressureMap &BBListing) {
  for (auto &I : BB) {
    llvm::Instruction *Inst = &I;
    if (WI->isUniform(Inst)) {
      Output += "U: ";
    } else {
      Output += "N: ";
    }
    unsigned int SizeInBytes = BBListing[Inst];
    unsigned int AmountOfRegistersRoundUp = RPE->bytesToRegisters(SizeInBytes);
    MaxPressureInFunction = std::max(MaxPressureInFunction, AmountOfRegistersRoundUp);
    Output += std::to_string(SizeInBytes) + " (" + std::to_string(AmountOfRegistersRoundUp) + ")" + "    \t";
    printInstruction(Inst, Output);
  }
}

void IGCRegisterPressurePrinter::intraBlock(llvm::BasicBlock &BB, std::string &Output, unsigned int SIMD) {
  InsideBlockPressureMap BBListing;
  RPE->collectPressureForBB(BB, BBListing, SIMD, &WI->Runner);
  printIntraBlock(BB, Output, BBListing);
}

void IGCRegisterPressurePrinter::dumpRegPressure(llvm::Function &F, unsigned int SIMD) {
  const char *Filter = IGC_GET_REGKEYSTRING(DumpRegPressureEstimateFilter);
  if (Filter && *Filter != '\0' && !std::regex_search(F.getName().str(), std::regex(Filter)))
    return;

  std::string Output;
  Output.reserve(32768);

  IGC::Debug::DumpLock();
  {
    const int MaxFunctionLength = 100;

    std::stringstream ss;
    std::string FunctionName = F.getName().str();

    std::replace_if(FunctionName.begin(), FunctionName.end(), [](char c) { return !std::isalnum(c) && c != '_'; }, '_');

    if (FunctionName.length() > MaxFunctionLength) {
      FunctionName = FunctionName.substr(0, MaxFunctionLength);
    }

    ss << FunctionName << "_" << DumpFileName << "_RegEst";
    auto Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                    .Hash(CGCtx->hash)
                    .Type(CGCtx->type)
                    .Retry(CGCtx->m_retryManager->GetRetryId())
                    .Pass(ss.str().c_str())
                    .Extension("ll");

    int Counter = 1;
    std::string originalName = Name.str();
    std::ifstream FileToCheck(Name.str());
    while (FileToCheck.good()) {
      FileToCheck.close();
      Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                 .Hash(CGCtx->hash)
                 .Type(CGCtx->type)
                 .Retry(CGCtx->m_retryManager->GetRetryId())
                 .Pass((ss.str() + "_" + std::to_string(Counter)).c_str())
                 .Extension("ll");
      FileToCheck.open(Name.str());
      Counter++;
    }
    FileToCheck.close();

    std::ofstream OutputFile(Name.str());
    OutputFile << "SIMD: " << SIMD << ", external pressure: " << ExternalPressure << "\n";

    for (BasicBlock &BB : F) {
      // prints information for one BB
      printSets(&BB, Output, SIMD);
      if (OutputFile.is_open())
        OutputFile << Output;
      Output.clear();
    }

    OutputFile << "==============================================" << "\n";
    OutputFile << "MaxPressure In Function: " << MaxPressureInFunction + ExternalPressure << "\n";
    OutputFile << "MaxPressure In Function Isolated: " << MaxPressureInFunction << "\n";

    OutputFile.close();
  }
  IGC::Debug::DumpUnlock();
}

void IGCRegisterPressurePrinter::printDefs(const ValueSet &In, const ValueSet &Out, std::string &Output) {
  ValueSet Difference, Common, Kills;

  for (auto *elem : Out)
    if (!In.count(elem))
      Difference.insert(elem);

  for (auto *elem : Out)
    if (In.count(elem))
      Common.insert(elem);

  for (auto *elem : In)
    if (!Common.count(elem))
      Kills.insert(elem);

  if (!Kills.empty()) {
    Output += "KILL:\t[\t" + std::to_string(Kills.size()) + "\t] ";
    if (PrinterType > 2)
      printNames(Kills, Output);
    Output += "\n";
  }

  if (!Difference.empty()) {
    Output += "DEF:\t[\t" + std::to_string(Difference.size()) + "\t] ";
    if (PrinterType > 2)
      printDefNames(Difference, Output);
    Output += "\n";
  }
}

void IGCRegisterPressurePrinter::printName(llvm::Value *Val, std::string &String) {
  llvm::raw_string_ostream Rso(String);
  // false means don't print type
  Val->printAsOperand(Rso, false);
}

void IGCRegisterPressurePrinter::printSets(llvm::BasicBlock *BB, std::string &Output, unsigned int SIMD) {
  if (PrinterType <= 0)
    return;

  Output += "block: " + BB->getName().str() + " ";
  Output += "function: " + BB->getParent()->getName().str() + "\n";

  ValueSet &PtrSetIn = RPE->getInSet()[BB];
  ValueSet &PtrSetOut = RPE->getOutSet()[BB];
  PhiSet &PtrSetPhi = RPE->getInPhiSet()[BB];

  printPhi(PtrSetPhi, Output);

  Output += "IN: \t[\t" + std::to_string(PtrSetIn.size()) + "\t] ";
  if (PrinterType > 2)
    printNames(PtrSetIn, Output);
  Output += "\n";

  printDefs(PtrSetIn, PtrSetOut, Output);

  Output += "OUT:\t[\t" + std::to_string(PtrSetOut.size()) + "\t] ";
  if (PrinterType > 2)
    printNames(PtrSetOut, Output);
  Output += "\n";

  intraBlock(*BB, Output, SIMD);
}

void IGCRegisterPressurePrinter::printNames(const ValueSet &Set, std::string &Name) {
  for (auto *val : Set) {
    llvm::raw_string_ostream Rso(Name);
    // false means don't print type
    val->printAsOperand(Rso, false);
    Rso << ", ";
  }
}

int getAmountOfBB(llvm::Value *Val) {
  llvm::SmallPtrSet<llvm::BasicBlock *, 16> BBSet;
  for (auto U : Val->users()) {

    auto UserInst = llvm::dyn_cast<Instruction>(U);
    if (!UserInst)
      continue;
    auto UserBB = UserInst->getParent();
    BBSet.insert(UserBB);
  }
  return BBSet.size();
}

void IGCRegisterPressurePrinter::printDefNames(const ValueSet &Set, std::string &Name) {

  for (auto *Val : Set) {
    llvm::raw_string_ostream Rso(Name);
    // false means don't print type
    Val->printAsOperand(Rso, false);
    int HowManyBB = getAmountOfBB(Val);
    Rso << "(" << std::to_string(Val->getNumUses()) << ")[" + std::to_string(HowManyBB) << "], ";
  }
}

void IGCRegisterPressurePrinter::printPhi(const PhiSet &Set, std::string &Output) {

  if (Set.empty())
    return;

  Output += "PHIS:\t[\t" + std::to_string(Set.size()) + "\t] ";
  for (const auto &Elem : Set) {
    auto &OneBBSet = Elem.second;
    unsigned int Size = OneBBSet.size();
    if (PrinterType > 1) {
      Output += "{ ";
      printNames(OneBBSet, Output);
      Output += "} ";
      Output += "(" + std::to_string(Size) + ")";
    }
  }
  Output += "\n";
}

bool IGCRegisterPressurePrinter::runOnFunction(llvm::Function &F) {

  ExternalPressure = getAnalysis<IGCFunctionExternalRegPressureAnalysis>().getExternalPressureForFunction(&F);
  RPE = &getAnalysis<IGCLivenessAnalysis>().getLivenessRunner();
  WI = &getAnalysis<WIAnalysis>();
  CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  MaxPressureInFunction = 0;

  unsigned int SIMD = numLanes(RPE->bestGuessSIMDSize(&F));

  if (DumpToFile) {
    dumpRegPressure(F, SIMD);
  } else {
    // basically only for LIT testing
    std::string Output;
    // no particular reason behind this, just big enough power of 2
    // helps to reduce printing time, by preemptively allocating memory
    Output.reserve(32768);
    Output += "SIMD: " + std::to_string(SIMD) + ", external pressure: " + std::to_string(ExternalPressure) + "\n";
    for (BasicBlock &BB : F) {
      printSets(&BB, Output, SIMD);
    }
    Output += "\n";
    Output += "==============================================\n";
    Output += "MaxPressure In Function: " + F.getName().str() + " --> " + std::to_string(MaxPressureInFunction) + "\n";
    PRINT(Output);
    Output.clear();
  }

  return false;
}

char IGCRegisterPressurePublisher::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG3 "igc-pressure-publisher"
#define PASS_DESCRIPTION3 "puts metadata with register pressure estimation"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(IGCRegisterPressurePublisher, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(IGCFunctionExternalRegPressureAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(IGCRegisterPressurePublisher, PASS_FLAG3, PASS_DESCRIPTION3, PASS_CFG_ONLY1, PASS_ANALYSIS1)

IGCRegisterPressurePublisher::IGCRegisterPressurePublisher() : llvm::ModulePass(ID) {
  initializeIGCRegisterPressurePublisherPass(*PassRegistry::getPassRegistry());
};

bool IGCRegisterPressurePublisher::runOnModule(llvm::Module &M) {
  if (skipModule(M)) {
    return false;
  }

  auto *CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto *MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto *ModMD = CGCtx->getModuleMetaData();
  auto *FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();

  for (auto &F : M) {
    if (F.isDeclaration() || F.isIntrinsic() || F.hasOptNone()) {
      continue;
    }

    IGCLivenessAnalysisRunner RPE(CGCtx, MDUtils, FGA);
    RPE.livenessAnalysis(F, nullptr);

    unsigned int MaxPressureInFunction = 0;

    unsigned int SimdSize = numLanes(RPE.bestGuessSIMDSize(&F));
    // If we have some published metadata already don't do anything
    bool AlreadyPublished = (RPE.checkPublishRegPressureMetadata(F) != 0);

    if (!AlreadyPublished) {
      auto ExternalPressure = getAnalysis<IGCFunctionExternalRegPressureAnalysis>().getExternalPressureForFunction(&F);

      auto *DT = &getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
      auto *PDT = &getAnalysis<PostDominatorTreeWrapperPass>(F).getPostDomTree();
      auto *LI = &getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

      TranslationTable TT;
      TT.run(F);
      WIAnalysisRunner WI(&F, LI, DT, PDT, MDUtils, CGCtx, ModMD, &TT, false);
      WI.run();

      MaxPressureInFunction = RPE.getMaxRegCountForFunction(F, SimdSize, &WI);
      RPE.publishRegPressureMetadata(F, MaxPressureInFunction + ExternalPressure);
    }
  }

  return false;
}
