/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXVisaRegAlloc
/// ----------------
///
/// GenXVisaRegAlloc is a function group pass that allocates vISA registers to
/// LLVM IR values.
///
/// Before allocating registers, this pass does "extra coalescing", over and above
/// what GenXCoalescing does. Two otherwise independent live ranges that are
/// related by being an operand and the result of the same instruction (and are
/// the same type) get coalesced and thus allocated into the same register.
///
/// However, extra coalescing is not performed when the result of the instruction
/// is used in a non-alu intrinsic, to try and avoid the danger of the jitter
/// needing to add an extra move in the send.
///
/// Other than that, all this pass does is allocate a different vISA register to
/// each LiveRange.
///
/// The pass is also an analysis for GenXKernelBuilder to query to find out
/// what vISA register is allocated to a particular Value. In fact, the query
/// from GenXKernelBuilder can specify what type it wants the register to be,
/// and it is at that point that an alias is allocated if there is no existing
/// alias of the requested type.
///
/// Finally, there are callbacks in the analysis to generate the vISA variable
/// tables to put into the vISA file.
///
//===----------------------------------------------------------------------===//
#ifndef GENXVISAREGALLOC_H
#define GENXVISAREGALLOC_H

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXLiveness.h"
#include "GenXModule.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/RegCategory.h"

#include "Probe/Assertion.h"
#include "visaBuilder_interface.h"

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

namespace llvm {

  class Function;
  class FunctionPass;
  class raw_ostream;
  class Type;
  class Value;

  ModulePass *createGenXGroupPrinterPass(raw_ostream &O,
                                         const std::string &Banner);

  // GenXVisaRegAlloc : vISA virtual register allocator pass
  class GenXVisaRegAlloc : public FGPassImplInterface,
                           public IDMixin<GenXVisaRegAlloc> {
  public:
    inline static const char *Prefix[] = {"ERR", "V", "A", "P", "S", "T"};

    static StringRef categoryToString(vc::RegCategory Category) {
      if (static_cast<unsigned>(Category) >= array_lengthof(Prefix))
        Category = vc::RegCategory::None;
      return accessContainer(Prefix, Category);
    }
    static void getAnalysisUsage(AnalysisUsage &Info);
    static StringRef getPassName() {
      return "GenX vISA virtual register allocator Wrapper";
    }

    // Reg : a virtual register
    class Reg {
    public:
      vc::RegCategory Category = vc::RegCategory::None;
      // Register ID. First value of it depends on count of predefined
      // variablse in category. F.e. for general var it is 32.
      unsigned Num = 0;
      // Pointer to register that is aliased by this register.
      Reg* AliasTo = nullptr;
      // Single linked list to store all aliases of real register.
      Reg *NextAlias = nullptr;
      genx::Signedness Signed = genx::DONTCARESIGNED;
      Type *Ty = nullptr;
      // Template, as bfloat not llvm-10 type
      bool IsBF = false;

      // log2 min alignment requested by user of register
      unsigned Alignment = 0;
      // String representation of register, mostly it is combination of
      // Category and Num
      std::string NameStr;
      // Attributes
      std::vector<std::pair<unsigned, std::string>> Attributes;
      // Pointer to VISA variable. It is set by CisaBuilder when it creates
      // VISA variables for all registers in RegMap.
      std::unordered_map<VISAKernel*, void*> GenVar;

      Reg(vc::RegCategory Category, unsigned Num, Type *Ty = 0,
          genx::Signedness Signed = genx::DONTCARESIGNED,
          unsigned LogAlignment = 0, Reg *AliasTo = nullptr,
          bool ArgIsBF = false)
          : Category(Category), Num(Num), AliasTo(AliasTo), Signed(Signed),
            Ty(Ty), Alignment(LogAlignment), IsBF(ArgIsBF) {
        IGC_ASSERT(vc::isRealCategory(Category));
        NameStr = (Twine(categoryToString(Category)) + Twine(Num)).str();
      }

      // Get VISA variable assigned to register.
      // Template T is just cast for return Type. Normally, we need to do an assertion test
      // here that required Var type is equal of real type in GenVar.
      template<class T>
      T* GetVar(VISAKernel* F) {
        return reinterpret_cast<T*>(GenVar[F]);
      }

      // Set VISA variable for current register.
      void SetVar(VISAKernel *F, void* Var) {
        GenVar[F] = Var;
      }

      void addAttribute(unsigned AttrName, Twine AttrVal) {
        Attributes.push_back(std::make_pair(AttrName, AttrVal.str()));
      }

      void print(raw_ostream &OS) const;
    };

    using RegPushHook = void(*)(void* Object, Reg&);
    using RegMap_t = std::map<genx::SimpleValue, Reg *>;
    using LRPtrVect = std::vector<genx::LiveRange *>;
    using LRCPtrVect = std::vector<const genx::LiveRange *>;
    void print(raw_ostream &OS, const FunctionGroup *FG) const override;

  private:
    FunctionGroup *FG = nullptr;
    GenXLiveness *Liveness = nullptr;
    GenXNumbering *Numbering = nullptr;
    FunctionGroupAnalysis *FGA = nullptr;
    const GenXSubtarget *ST = nullptr;
    const GenXBackendConfig *BackendConfig = nullptr;
    const DataLayout *DL = nullptr;

    // pushReg callback that will be called once new register is created
    RegPushHook TheRegPushHook = nullptr;
    // Object that will be passed to hook, likely it is 'this' of hook owner.
    void* TheRegPushHookObject = nullptr;

    // Storage for all created registers. It is list because we use pointers
    // to stored registers, thus we need to non-reallocable storage.
    std::list<Reg> RegStorage;
    // Map from LLVM Value to pointer to register associed with it.
    RegMap_t RegMap;
    // List of pointers to predefined surface registers.
    std::vector<Reg*> PredefinedSurfaceRegs;
    std::vector<Reg*> PredefinedRegs;

    // Array of current indexes being assigned to new register.
    unsigned CurrentRegId[vc::numRegCategories()];

    struct RegAllocStats {
      const LRCPtrVect *getLRs(const FunctionGroup *FG) const;
      void recordLRs(const FunctionGroup *FG, const LRPtrVect &LRs);

    private:
      std::map<const FunctionGroup *, LRCPtrVect> LRs;
    } Stats;

  public:
    explicit GenXVisaRegAlloc() {}
    void releaseMemory() override;
    bool runOnFunctionGroup(FunctionGroup &FG) override;

    std::list<Reg>& getRegStorage() {
      return RegStorage;
    }
    // Get the vISA virtual register for a value or nullptr if there is no
    // register associated with given value.
    Reg *getRegForValueOrNull(genx::SimpleValue V,
                              genx::Signedness Signed = genx::DONTCARESIGNED,
                              Type *OverrideType = nullptr,
                              bool IsBF = false) const;

    // Get the vISA virtual register for a value or create alias if there is no
    // register associated with given value.
    Reg *getOrCreateRegForValue(genx::SimpleValue V,
                                genx::Signedness Signed = genx::DONTCARESIGNED,
                                Type *OverrideType = nullptr,
                                bool IsBF = false);

    // Get the vISA virtual register for a value (0 if none), ignoring type
    // and signedness so it can be a const method usable from print().
    Reg *getRegForValueUntyped(genx::SimpleValue V) const;

    // Get the signedness of a register.
    genx::Signedness getSigned(const Reg *R) const;

    // Set callback that will be called each time new register is created.
    // It is used in CisaBuilder when new aliases are created.
    void SetRegPushHook(void* Object, RegPushHook Callback) {
      TheRegPushHook = Callback;
      TheRegPushHookObject = Object;
    }

    void reportVisaVarableNumberLimitError(vc::RegCategory Category,
                                           unsigned ID) const;

    static unsigned getMaximumVariableIDForCategory(vc::RegCategory Category);

    static bool isVisaVaribleLimitExceeded(vc::RegCategory Category,
                                           unsigned CurrentID) {
      const auto IDLimit = getMaximumVariableIDForCategory(Category);
      return CurrentID > IDLimit;
    }

    // Create new register and push it in storage.
    // If RegPushHook was specified it will be called with created register as
    // parameter. Thus, all needed register's variables must be specified
    // at this moment, for example AliasTo.
    template <class... Args>
    Reg *createReg(vc::RegCategory Category, Args &&... args) {
      if (isVisaVaribleLimitExceeded(
              Category, vc::accessContainer(CurrentRegId, Category))) {
        reportVisaVarableNumberLimitError(
            Category, vc::accessContainer(CurrentRegId, Category));
      }
      auto NewID = vc::accessContainer(CurrentRegId, Category)++;
      IGC_ASSERT(vc::accessContainer(CurrentRegId, Category) != 0);
      RegStorage.emplace_back(Category, NewID, std::forward<Args>(args)...);
      Reg& R = RegStorage.back();
      if (TheRegPushHook)
        TheRegPushHook(TheRegPushHookObject, R);
      return &R;
    }

  private:
    void getLiveRanges(LRPtrVect &LRs) const;
    void getLiveRangesForValue(Value *V, LRPtrVect &LRs) const;
    void localizeLiveRangesForAccUsage(LRPtrVect &LRs);
    void extraCoalescing();
    void allocReg(genx::LiveRange *LR);
  public:
    // Add special RetIP argument.
    Reg* getRetIPArgument() const { return RetIP; }
    void addRetIPArgument();
  private:
    unsigned CoalescingCount = 0;
    Reg* RetIP = nullptr;
  };
  using GenXVisaRegAllocWrapper = FunctionGroupWrapperPass<GenXVisaRegAlloc>;

  namespace visa {
    // Details of a type required for a vISA general register declaration
    // or an indirect operand.
    struct TypeDetails {
      const DataLayout &DL;
      unsigned NumElements;
      unsigned BytesPerElement;
      unsigned VisaType;
      TypeDetails(const DataLayout &DL, Type *Ty, genx::Signedness Signed,
                  bool IsBF = false);
    };
  } // end namespace visa

  void initializeGenXVisaRegAllocWrapperPass(PassRegistry &);

} // end namespace llvm
#endif //ndef GENXVISAREGALLOC_H

