/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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
#include "vc/GenXOpts/Utils/RegCategory.h"
#include "visaBuilder_interface.h"
#include <map>
#include <string>
#include <vector>

namespace llvm {

  class Function;
  class FunctionPass;
  class raw_ostream;
  class Type;
  class Value;

  FunctionGroupPass *createGenXGroupPrinterPass(raw_ostream &O, const std::string &Banner);

  // GenXVisaRegAlloc : vISA virtual register allocator pass
  class GenXVisaRegAlloc : public FunctionGroupPass {
  public:

    // Reg : a virtual register
    class Reg {
    public:
      unsigned short Category = genx::RegCategory::NONE;
      // Register ID. First value of it depends on count of predefined
      // variablse in category. F.e. for general var it is 32.
      unsigned short Num = 0;
      // Pointer to register that is aliased by this register.
      Reg* AliasTo = nullptr;
      // Single linked list to store all aliases of real register.
      std::map<const Function*, Reg*> NextAlias;
      genx::Signedness Signed = genx::DONTCARESIGNED;
      Type *Ty = nullptr;
      // log2 min alignment requested by user of register
      unsigned Alignment;
      // String representation of register, mostly it is combination of
      // Category and Num
      std::string NameStr;
      // Attributes
      std::vector<std::pair<unsigned, std::string>> Attributes;
      // Pointer to VISA variable. It is set by CisaBuilder when it creates
      // VISA variables for all registers in RegMap.
      std::map<VISAKernel*, void*> GenVar;

      explicit Reg(
          unsigned Category,
          unsigned Num,
          Type *Ty = 0,
          genx::Signedness Signed = genx::DONTCARESIGNED,
          unsigned LogAlignment = 0,
          Reg* AliasTo = nullptr)
          : Category(Category), Num(Num), AliasTo(AliasTo), Signed(Signed),
            Ty(Ty), Alignment(LogAlignment) {
        static const char* Prefix[] = { "ERR", "V", "A", "P", "S", "T", "VME" };
        assert(Category && Category < genx::RegCategory::NUMREALCATEGORIES);
        NameStr = Prefix[Category] + std::to_string(Num);
      }

      // Get VISA variable assigned to register.
      // Template T is just cast for return Type. Normally, we need to assert
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
    using KernRegMap_t = std::map<genx::SimpleValue, Reg*>;
    using RegMap_t = std::map<const Function*, KernRegMap_t>;
  private:
    FunctionGroup *FG;
    GenXLiveness *Liveness;
    GenXNumbering *Numbering;
    FunctionGroupAnalysis *FGA;

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

    // Array of current indexes being assigned to new register.
    unsigned CurrentRegId[genx::RegCategory::NUMREALCATEGORIES];

  public:
    static char ID;
    explicit GenXVisaRegAlloc() : FunctionGroupPass(ID) { }
    virtual StringRef getPassName() const { return "GenX vISA virtual register allocator"; }
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnFunctionGroup(FunctionGroup &FG);

    std::list<Reg>& getRegStorage() {
      return RegStorage;
    }
    // Get the vISA virtual register for a value (assert if none)
    Reg* getRegForValue(const Function *kernel, genx::SimpleValue V,
        genx::Signedness Signed = genx::DONTCARESIGNED, Type *OverrideType = 0)
    {
      Reg* R = getRegForValueOrNull(kernel, V, Signed, OverrideType);
      assert(R && "no register allocated for this value");
      return R;
    }
    // Get the vISA virtual register for a value or nullptr if there is no
    // register associated with given value.
    Reg* getRegForValueOrNull(const Function *kernel, genx::SimpleValue V,
      genx::Signedness Signed = genx::DONTCARESIGNED, Type *OverrideType = 0);

    // Get the vISA virtual register for a value (0 if none), ignoring type
    // and signedness so it can be a const method usable from print().
    Reg* getRegForValueUntyped(const Function* kernel, genx::SimpleValue V) const;

    // Get the signedness of a register.
    genx::Signedness getSigned(Reg* R);

    // Set callback that will be called each time new register is created.
    // It is used in CisaBuilder when new aliases are created.
    void SetRegPushHook(void* Object, RegPushHook Callback) {
      TheRegPushHook = Callback;
      TheRegPushHookObject = Object;
    }

    // Create new register and push it in storage.
    // If RegPushHook was specified it will be called with created register as
    // parameter. Thus, all needed register's variables must be specified
    // at this moment, for example AliasTo.
    template<class ... Args>
    Reg* createReg(unsigned Category, Args&& ... args) {
      RegStorage.emplace_back(Category, CurrentRegId[Category]++,
        std::forward<Args>(args) ...);
      Reg& R = RegStorage.back();
      if (TheRegPushHook)
        TheRegPushHook(TheRegPushHookObject, R);
      return &R;
    }

    // createPrinterPass : get a pass to print the IR, together with the GenX
    // specific analyses
    virtual Pass *createPrinterPass(raw_ostream &O, const std::string &Banner) const
    { return createGenXGroupPrinterPass(O, Banner); }
    // print : dump the state of the pass. This is used by -genx-dump-regalloc
    virtual void print(raw_ostream &O, const Module *M) const;
  private:
    void getLiveRanges(std::vector<genx::LiveRange *> *LRs) const;
    void getLiveRangesForValue(Value *V, std::vector<genx::LiveRange *> *LRs) const;
    void extraCoalescing();
    void allocReg(genx::LiveRange *LR);
  public:
    // Add special RetIP argument.
    Reg* getRetIPArgument() const { return RetIP; }
    void addRetIPArgument();
  private:
    unsigned CoalescingCount = 0;
    Reg* RetIP;
  };

  namespace visa {
    // Details of a type required for a vISA general register declaration
    // or an indirect operand.
    struct TypeDetails {
      const DataLayout &DL;
      unsigned NumElements;
      unsigned BytesPerElement;
      unsigned VisaType;
      TypeDetails(const DataLayout &DL, Type *Ty, genx::Signedness Signed);
    };
  } // end namespace visa

  void initializeGenXVisaRegAllocPass(PassRegistry &);

} // end namespace llvm
#endif //ndef GENXVISAREGALLOC_H

