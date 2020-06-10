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

#ifndef _GEN4_IR_HPP_
#define _GEN4_IR_HPP_


#include <set>
#include <list>
#include <string>
#include <bitset>
#include <vector>
#include <climits>
#include <cstdlib>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <stack>

#include "Mem_Manager.h"
#include "G4_Opcode.h"
#include "Option.h"
#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_GEN.h"
#include "Attributes.hpp"
#include "JitterDataStruct.h"
#include "Metadata.h"
#include "BitSet.h"

#include <memory>

namespace vISA
{
    template <class T>
    class std_arena_based_allocator
    {
    protected:
    std::shared_ptr<Mem_Manager> mem_manager_ptr;

    public:

        //for allocator_traits
        typedef std::size_t    size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T*             pointer;
        typedef const T*       const_pointer;
        typedef T&             reference;
        typedef const T&       const_reference;
        typedef T              value_type;

    explicit std_arena_based_allocator(std::shared_ptr<Mem_Manager> _other_ptr)
            :mem_manager_ptr(_other_ptr)
        {
        }

        explicit std_arena_based_allocator()
            :mem_manager_ptr(nullptr)
        {
            //This implicitly calls Mem_manager constructor.
        mem_manager_ptr = std::make_shared<Mem_Manager>(4096);
        }

        explicit std_arena_based_allocator(const std_arena_based_allocator& other)
            : mem_manager_ptr(other.mem_manager_ptr)
        {}


        template <class U>
        std_arena_based_allocator(const std_arena_based_allocator<U>& other)
            : mem_manager_ptr(other.mem_manager_ptr)
        {}

        template <class U>
        std_arena_based_allocator& operator=(const std_arena_based_allocator<U>& other)
        {
            mem_manager_ptr = other.mem_manager_ptr;
            return *this;
        }

        template <class U>
        struct rebind { typedef std_arena_based_allocator<U> other; };

        template <class U> friend class std_arena_based_allocator;

        pointer allocate(size_type n, const void * = 0)
        {
            T* t = (T*)mem_manager_ptr->alloc(n * sizeof(T));
            return t;
        }

        void deallocate(void* p, size_type)
        {
            //No deallocation for arena allocator.
        }

        pointer           address(reference x) const { return &x; }
        const_pointer     address(const_reference x) const { return &x; }

        std_arena_based_allocator<T>&  operator=(const std_arena_based_allocator&)
        {
            return *this;
        }

        void              construct(pointer p, const T& val)
        {
            new ((T*)p) T(val);
        }
        void              destroy(pointer p) { p->~T(); }

        size_type         max_size() const { return size_t(-1); }

        bool operator==(const std_arena_based_allocator &) const { return true; }

        bool operator!=(const std_arena_based_allocator & a) const { return !operator==(a); }
    };
}
void resetRightBound(vISA::G4_Operand* opnd);

// We use memory manager.  Memory manager will free all the space at once so that
// there is no need to call destructor or delete to free up space.
#ifdef _MSC_VER
#pragma warning (disable: 4291)
#pragma warning (disable: 4996)
#endif

namespace vISA
{
// forward declaration
class G4_INST;
class G4_Areg;
class G4_RegVar;
class G4_Declare;
class G4_Operand;
class G4_CondMod;
class G4_Predicate;
class GlobalRA;

class G4_Imm;
class G4_Greg;
class G4_Label;
class G4_AddrExp;
class G4_DstRegRegion;
class G4_SrcRegRegion;

class IR_Builder;

class LocalLiveRange;
class G4_Kernel;
class G4_VarBase;

class G4_SpillIntrinsic;
class G4_FillIntrinsic;


}
void associateOpndWithInst(vISA::G4_Operand*, vISA::G4_INST*);

// Forward declarations for global opt report related functions
void getOptReportStream(std::ofstream& reportStream, const Options *options);
void closeOptReportStream(std::ofstream& reportStream);

vISA::G4_Declare* GetTopDclFromRegRegion(vISA::G4_Operand* opnd);

enum BankConflict {
    BANK_CONFLICT_NONE,
    BANK_CONFLICT_FIRST_HALF_EVEN,
    BANK_CONFLICT_FIRST_HALF_ODD,
    BANK_CONFLICT_SECOND_HALF_EVEN,
    BANK_CONFLICT_SECOND_HALF_ODD};

typedef enum
{
    MATH_RESERVED = 0,
    MATH_INV = 1,
    MATH_LOG = 2,
    MATH_EXP = 3,
    MATH_SQRT = 4,
    MATH_RSQ = 5,
    MATH_SIN = 6,
    MATH_COS = 7,
    // 8 is skipped
    MATH_FDIV = 9,
    MATH_POW = 0xA,
    MATH_INT_DIV = 0xB,
    MATH_INT_DIV_QUOT = 0xC,
    MATH_INT_DIV_REM = 0xD,
    MATH_INVM = 0xE,
    MATH_RSQRTM = 0xF
} G4_MathOp;

inline const char* MathOpNames[16] =
{
    "reserved",
    "inv",
    "log",
    "exp",
    "sqrt",
    "rsq",
    "sin",
    "cos",
    "undefined",
    "fdiv",
    "pow",
    "intdiv",
    "quot",
    "rem",
    "invm",
    "rsqrtm"
};


typedef vISA::std_arena_based_allocator<vISA::G4_INST*> INST_LIST_NODE_ALLOCATOR;

typedef std::list<vISA::G4_INST*, INST_LIST_NODE_ALLOCATOR>           INST_LIST;
typedef std::list<vISA::G4_INST*, INST_LIST_NODE_ALLOCATOR>::iterator INST_LIST_ITER;
typedef std::list<vISA::G4_INST*, INST_LIST_NODE_ALLOCATOR>::reverse_iterator INST_LIST_RITER;

typedef std::pair<vISA::G4_INST*, Gen4_Operand_Number> USE_DEF_NODE;
typedef vISA::std_arena_based_allocator<USE_DEF_NODE> USE_DEF_ALLOCATOR;

typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR > USE_EDGE_LIST;
typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR >::iterator USE_EDGE_LIST_ITER;
typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR > DEF_EDGE_LIST;
typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR >::iterator DEF_EDGE_LIST_ITER;

namespace vISA
{

    enum class SendAccess
    {
        READ_ONLY,
        WRITE_ONLY,
        READ_WRITE
    };
class G4_SendMsgDescriptor
{
private:
    /// Structure describes a send message descriptor. Only expose
    /// several data fields; others are unnamed.
    struct MsgDescLayout {
        uint32_t funcCtrl : 19;     // Function control (bit 0:18)
        uint32_t headerPresent : 1; // Header present (bit 19)
        uint32_t rspLength : 5;     // Response length (bit 20:24)
        uint32_t msgLength : 4;     // Message length (bit 25:28)
        uint32_t simdMode2 : 1;     // 16-bit input (bit 29)
        uint32_t returnFormat : 1;  // 16-bit return (bit 30)
        uint32_t EOT : 1;           // EOT
    };

    /// View a message descriptor in two different ways:
    /// - as a 32-bit unsigned integer
    /// - as a structure
    /// This simplifies the implementation of extracting subfields.
    union DescData {
        uint32_t value;
        MsgDescLayout layout;
    } desc;

    /// Structure describes an extended send message descriptor.
    /// Only expose several data fields; others are unnamed.
    struct ExtendedMsgDescLayout {
        uint32_t funcID : 4;       // bit 0:3
        uint32_t unnamed1 : 1;     // bit 4
        uint32_t eot : 1;          // bit 5
        uint32_t extMsgLength : 5; // bit 6:10
        uint32_t cps : 1;          // bit 11
        uint32_t RTIndex : 3;      // bit 12-14
        uint32_t src0Alpha : 1;    // bit 15
        uint32_t extFuncCtrl : 16; // bit 16:31
    };

    /// View an extended message descriptor in two different ways:
    /// - as a 32-bit unsigned integer
    /// - as a structure
    /// This simplifies the implementation of extracting subfields.
    union ExtDescData {
        uint32_t value;
        ExtendedMsgDescLayout layout;
    } extDesc;

    SendAccess accessType;

    /// Whether funcCtrl is valid
    bool funcCtrlValid;

    G4_Operand *m_sti;
    G4_Operand *m_bti;

    SFID        sfid;
    int         src1Len;
    bool        eotAfterMessage = false;

public:
    static const int SLMIndex = 0xFE;

    G4_SendMsgDescriptor(uint32_t fCtrl, uint32_t regs2rcv, uint32_t regs2snd,
        SFID fID, uint16_t extMsgLength, uint32_t extFCtrl,
        SendAccess access, G4_Operand* bti, G4_Operand* sti, IR_Builder& builder);

    /// Construct a object with descriptor and extended descriptor values.
    /// used in IR_Builder::createSendMsgDesc(uint32_t desc, uint32_t extDesc, SendAccess access)
    G4_SendMsgDescriptor(uint32_t desc, uint32_t extDesc, SendAccess access,
        G4_Operand* bti, G4_Operand* sti);

    /// Preferred constructor takes an explicit SFID and src1 length
    G4_SendMsgDescriptor(
        SFID sfid,
        uint32_t desc,
        uint32_t extDesc,
        int src1Len,
        SendAccess access,
        G4_Operand *bti,
        bool isValidFuncCtrl);

    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

    static uint32_t createExtDesc(SFID funcID, bool isEot = false)
    {
        return createExtDesc(funcID, isEot, 0, 0);
    }

    static uint32_t createMRTExtDesc(bool src0Alpha, uint8_t RTIndex, bool isEOT, uint32_t extMsgLen)
    {
        ExtDescData data;
        data.value = 0;
        data.layout.funcID = SFIDtoInt(SFID::DP_WRITE);
        data.layout.RTIndex = RTIndex;
        data.layout.src0Alpha = src0Alpha;
        data.layout.eot = isEOT;
        data.layout.extMsgLength = extMsgLen;
        return data.value;
    }

    static uint32_t createExtDesc(SFID funcID,
        bool isEot,
        unsigned extMsgLen,
        unsigned extFCtrl = 0)
    {
        ExtDescData data;
        data.value = 0;
        data.layout.funcID = SFIDtoInt(funcID);
        data.layout.eot = isEot;
        data.layout.extMsgLength = extMsgLen;
        data.layout.extFuncCtrl = extFCtrl;
        return data.value;
    }

    static uint32_t createDesc(uint32_t fc, bool headerPresent,
        unsigned msgLength, unsigned rspLength)
    {
        DescData data;
        data.value = fc;
        data.layout.headerPresent = headerPresent;
        data.layout.msgLength = static_cast<uint16_t>(msgLength);
        data.layout.rspLength = static_cast<uint16_t>(rspLength);
        return data.value;
    }

    SFID      getFuncId() const {return sfid;}

    uint32_t getFuncCtrl() const {
        return desc.layout.funcCtrl;
    }


    bool isEOTInst() const { return eotAfterMessage; }
    void setEOT();

    uint16_t getExtFuncCtrl() const {
        MUST_BE_TRUE(isHDC(), "getExtFuncCtrl on non-HDC message");
        return extDesc.layout.extFuncCtrl;
    }

    /* Info methods */
    // common for all sends
    uint16_t ResponseLength() const { return desc.layout.rspLength; }
    uint16_t MessageLength() const { return desc.layout.msgLength; }
    uint16_t extMessageLength() const { return (uint16_t)src1Len; }
    bool isDataPortRead() const { return accessType != SendAccess::WRITE_ONLY; }
    bool isDataPortWrite() const { return accessType != SendAccess::READ_ONLY; }
    SendAccess getAccess() const { return accessType; }
    bool isValidFuncCtrl() const { return funcCtrlValid; }
    bool isHeaderPresent() const;
    void setHeaderPresent(bool val);

    // for HDC messages only (DC0/DC1/DC2)
    bool isHDC() const
    {
        auto funcID = getFuncId();
        return
            funcID == SFID::DP_DC ||
            funcID == SFID::DP_DC1 ||
            funcID == SFID::DP_DC2 ||
            funcID == SFID::DP_CC;
    }

    uint32_t getHdcMessageType() const;
    bool isAtomicMessage() const;
    uint16_t getHdcAtomicOp() const;

    bool isSLMMessage() const;

    unsigned int getEnabledChannelNum() const;
    unsigned int getBlockNum() const;
    unsigned int getBlockSize() const;
    bool isOwordLoad() const;

    bool isHdcTypedSurfaceWrite() const;

    // return offset in unit of GRF
    uint16_t getScratchRWOffset() const
    {
        MUST_BE_TRUE(isScratchRW(), "Message is not scratch space R/W.");
        return (getFuncCtrl() & 0xFFFu);
    }

    bool isScratchRW() const
    {
        // scratch msg: DC0, bit 18 = 1
        return getFuncId() == SFID::DP_DC && ((getFuncCtrl() & 0x40000u) != 0);
    }
    bool isScratchRead() const
    {
        return isScratchRW() && (getFuncCtrl() & 0x20000u) == 0;
    }
    bool isScratchWrite() const
    {
        return isScratchRW() && (getFuncCtrl() & 0x20000u) != 0;
    }
    uint16_t getScratchRWSize() const
    {
        MUST_BE_TRUE(isScratchRW(), "Message is not scratch space R/W.");
        uint16_t bitV = ((getFuncCtrl() & 0x3000u) >> 12);
        return  0x1 << bitV;
    }

    bool isA64Message() const;

    // for sampler mesasges only
    bool isSampler() const { return getFuncId() == SFID::SAMPLER; }
    bool isCPSEnabled() const { return extDesc.layout.cps != 0; }
    bool is16BitInput() const;
    bool is16BitReturn() const;

    bool isThreadMessage() const
    {
        return getFuncId() == SFID::GATEWAY || getFuncId() == SFID::SPAWNER;
    }

    bool isBarrierMsg() const;
    bool isFence() const;
    bool isSendBarrier() const {
        return isAtomicMessage() || isBarrierMsg();  // atomic write or explicit barrier
    }

    const G4_Operand *getBti() const {return m_bti;}
          G4_Operand *getBti()       {return m_bti;}
    const G4_Operand *getSti() const {return m_sti;}
          G4_Operand *getSti()       {return m_sti;}

    uint32_t getDesc() const { return desc.value; }
    uint32_t getExtendedDesc() const { return extDesc.value; }

    const char* getDescType() const;
private:
    void setBindingTableIdx(unsigned idx);
};

//forward declaration for the binary of an instruction
class BinInst;

class G4_FCALL
{
    uint16_t argSize;
    uint16_t retSize;

public:
    G4_FCALL(uint16_t argVarSz, uint16_t retVarSz) : argSize(argVarSz), retSize(retVarSz)
    {}

    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    uint16_t getArgSize() const { return argSize; }
    uint16_t getRetSize() const { return retSize; }
};

//forward references.
class G4_InstMath;
class G4_InstCF;
class G4_InstIntrinsic;
class G4_InstSend;


class G4_INST
{
    friend class G4_SendMsgDescriptor;
    friend class IR_Builder;

protected:
    G4_opcode        op;
    G4_Operand*      srcs[G4_MAX_SRCS];
    G4_DstRegRegion* dst;
    G4_Predicate*    predicate;
    G4_CondMod*      mod;
    unsigned int     option;     // inst option
    G4_Operand*             implAccSrc;
    G4_DstRegRegion*        implAccDst;

    // def-use chain: list of <inst, opndPos> such that this[dst/condMod] defines inst[opndPos]
    // opndNum must be one of src0, src1, src2, pred, implAccSrc
    USE_EDGE_LIST useInstList;

    // use-def chain: list of <inst, opndPos> such that inst[dst/condMod] defines this[opndPos]
    DEF_EDGE_LIST defInstList;

    // instruction's id in BB. Each optimization should re-initialize before using
    int32_t   local_id;

    int srcCISAoff; // record CISA inst offset that resulted in this instruction

    MDLocation* location;

#define UNDEFINED_GEN_OFFSET -1
    int64_t genOffset = UNDEFINED_GEN_OFFSET;

    void emit_options(std::ostream& output) const;

    //WARNING: if adding new options, please make sure that bitfield does not
    //overflow.
    unsigned short sat : 1;
    // during optimization, an inst may become redundant and be marked dead
    unsigned short dead : 1;
    unsigned short evenlySplitInst : 1;
    unsigned char    execSize;

    BinInst *bin;

    // make it private so only the IR_Builder can create new instructions
    void *operator new(size_t sz, Mem_Manager& m){ return m.alloc(sz); }
    uint32_t global_id = (uint32_t) -1;

    const IR_Builder& builder;  // link to builder to access the various compilation options
    const void *llvmInst;

public:
    const void* GetLLVMInst() { return llvmInst; };
    G4_INST* InheritLLVMInst(G4_INST* i) { llvmInst = i->GetLLVMInst(); return this; };
    enum SWSBTokenType {
        TOKEN_NONE,
        AFTER_READ,
        AFTER_WRITE,
        READ_ALL,
        WRITE_ALL
    };


protected:
    int ALUID = -1;
    unsigned char depDistance = 0;
    unsigned short SBToken = -1;
    bool operandTypeIndicated = true;

    struct DepToken {
        unsigned short token;
        SWSBTokenType type;
    };
    std::vector <DepToken> depTokens;

public:
    void setDistance(unsigned char dep_distance) {depDistance = dep_distance;}
    void setOperandTypeIndicated(bool indicated) { operandTypeIndicated = indicated; }
    void setToken(unsigned short token) {SBToken = token;}

    void setDepToken(unsigned short token, SWSBTokenType type)
    {
        for (size_t i = 0, size = depTokens.size(); i < size; i++)
        {
            DepToken &depToken = depTokens[i];
            if (depToken.token == token)
            {
                if (depToken.type == AFTER_WRITE)
                {
                    return;
                }
                if (type == AFTER_WRITE)
                {
                    depToken.type = type;
                }
                return;
            }
        }

        struct DepToken dt;
        dt.token = token;
        dt.type = type;
        depTokens.push_back(dt);
    }
    void eraseDepToken(unsigned i)
    {
        depTokens.erase(depTokens.begin() + i);
    }

    bool isOperandTypeIndicated() {return operandTypeIndicated;}
    unsigned char getDistance() { return depDistance; }
    unsigned short getToken() { return SBToken; }

    size_t getDepTokenNum() { return depTokens.size(); }
    unsigned short getDepToken(unsigned int i, SWSBTokenType &type) const
    {
        type = depTokens[i].type;
        return depTokens[i].token;
    }

    bool isDFInstruction() const;

    bool isMathPipeInst() const;

    bool distanceHonourInstruction() const;
    bool tokenHonourInstruction() const;

    void setALUID(int i) { ALUID = i; }
    int getALUID() const { return ALUID; }


public:
    G4_INST(const IR_Builder& builder,
        G4_Predicate* prd,
        G4_opcode o,
        G4_CondMod* m,
        bool s,
        unsigned char size,
        G4_DstRegRegion* d,
        G4_Operand* s0,
        G4_Operand* s1,
        unsigned int opt);

    G4_INST(
        const IR_Builder& builder,
        G4_Predicate* prd,
        G4_opcode o,
        G4_CondMod* m,
        bool s,
        unsigned char size,
        G4_DstRegRegion* d,
        G4_Operand* s0,
        G4_Operand* s1,
        G4_Operand* s2,
        unsigned int opt);

    virtual ~G4_INST() {}

    // The method is declared virtual so subclasses of G4_INST
    // should also implement this method to populate members
    // unique to them.
    virtual G4_INST* cloneInst();
    virtual bool isBaseInst() const { return true; }
    virtual bool isCFInst() const { return false; }

    uint32_t getLexicalId() const { return global_id; }
    void setLexicalId(uint32_t id) { global_id = id; }
    void setPredicate(G4_Predicate* p);
    G4_Predicate* getPredicate() const { return predicate; }
    void setSaturate(bool s) { sat = s; }
    bool getSaturate() const { return sat; }
    G4_opcode opcode()  const { return op; }

    void setOpcode(G4_opcode opcd);

    G4_DstRegRegion* getDst() const { return dst; }
    bool supportsNullDst() const;

    bool isPseudoKill() const;
    bool isLifeTimeEnd() const;
    bool isSpillIntrinsic() const;
    G4_SpillIntrinsic* asSpillIntrinsic() const;
    bool isFillIntrinsic() const;
    G4_FillIntrinsic* asFillIntrinsic() const;
    bool isSplitIntrinsic() const;
    bool isCallerSave() const;
    bool isCallerRestore() const;
    bool isCalleeSave() const;
    bool isCalleeRestore() const;
    bool isRelocationMov() const;
    bool isMov() const { return G4_Inst_Table[op].instType == InstTypeMov; }
    bool isLogic() const { return G4_Inst_Table[op].instType == InstTypeLogic; }
    bool isCompare() const
    {
        return G4_Inst_Table[op].instType == InstTypeCompare;
    }
    bool isFlowControl() const
    {
        return G4_Inst_Table[op].instType == InstTypeFlow;
    }
    bool isArithmetic() const
    {
        return G4_Inst_Table[op].instType == InstTypeArith;
    }
    bool isVector() const
    {
        return G4_Inst_Table[op].instType == InstTypeVector;
    }
    bool isLabel() const { return op == G4_label; }
    bool isCall() const { return op == G4_call; }
    bool isFCall() const { return op == G4_pseudo_fcall; }
    bool isReturn() const { return op == G4_return; }
    bool isFReturn() const { return (op == G4_pseudo_fret); }
    bool isMath() const { return op == G4_math; }
    bool isIntrinsic() const { return op == G4_intrinsic; }
    bool isSend() const { return op == G4_send || op == G4_sendc || op == G4_sends || op == G4_sendsc; }
    bool isSplitSend() const { return op == G4_sends || op == G4_sendsc; }

    // ToDo: get rid of these functions which don't make sense for non-sends
    virtual bool isEOT() const { return false; }
    virtual G4_SendMsgDescriptor* getMsgDesc() const { return nullptr; }

    virtual bool mayExceedTwoGRF() const
    {
        return false;
    }
    // special instructions(e.g., send) should override
    virtual void computeRightBound(G4_Operand* opnd);

    bool isWait() const { return op == G4_wait; }
    static bool isSyncOpcode(G4_opcode opcode) { return opcode == G4_sync_nop || opcode == G4_sync_allrd || opcode == G4_sync_allwr; }
    bool isSWSBSync() const
    {
        return G4_INST::isSyncOpcode(op);
    }

    bool isPseudoLogic() const
    {
        return op == G4_pseudo_and || op == G4_pseudo_or || op == G4_pseudo_xor || op == G4_pseudo_not;
    }

    bool isPartialWrite() const;
    bool isPartialWriteForSpill(bool inSIMDCF) const;
    bool isArithAddr() const;
    bool isMovAddr() const;
    bool isAccSrcInst() const;
    bool isAccDstInst() const;

    G4_InstMath* asMathInst() const
    {
        MUST_BE_TRUE(isMath(), ERROR_UNKNOWN);
        return ((G4_InstMath*) this);
    }

    G4_InstCF* asCFInst() const
    {
        MUST_BE_TRUE(isFlowControl(), ERROR_UNKNOWN);
        return ((G4_InstCF*) this);
    }

    G4_InstIntrinsic* asIntrinsicInst() const
    {
        MUST_BE_TRUE(isIntrinsic(), ERROR_UNKNOWN);
        return (G4_InstIntrinsic*) this;
    }

    G4_InstSend* asSendInst()
    {
        if (!isSend())
        {
            return nullptr;
        }
        return reinterpret_cast<G4_InstSend*>(this);
    }


    bool isPseudoUse() const;
    G4_Type getExecType() const;
    G4_Type getExecType2() const;
    bool isComprInst() const
    {
        return detectComprInst();
    }
    bool isComprInvariantSrcRegion(G4_SrcRegRegion* src, int srcPos);

    G4_Operand* getSrc(unsigned i) const
    {
        MUST_BE_TRUE(i < G4_MAX_SRCS, ERROR_INTERNAL_ARGUMENT);
        return srcs[i];
    }
    void setSrc(G4_Operand* opnd, unsigned i);
    int getNumSrc() const;
    int getNumDst() const;

    // this assume we don't have to recompute bound for the swapped source
    // Note that def-use chain is not maintained after this; call swapDefUse
    // if you want to update the du-chain.
    void swapSrc(int src1, int src2)
    {
        assert(src1 >= 0 && src1 < getNumSrc() && src2 >= 0 && src2 < getNumSrc() && "illegal src number");
        std::swap(srcs[src1], srcs[src2]);
    }

    G4_Label* getLabel()
    {
        MUST_BE_TRUE( op == G4_label, "inst must be a label");
        return (G4_Label*) getSrc(0);
    }

    MDLocation *getLocation() const { return location; }
    void setLocation(MDLocation* loc) {
        location = loc;
    }
    void setLineNo(int i) {
        if (location != nullptr)
            location->setLineNo(i);
    }
    int getLineNo() const {
        if (location == nullptr)
            return 0;
        return location->getLineNo();
    }
    void setSrcFilename(const char* filename) {
        if (location != nullptr)
            location->setSrcFilename(filename);
    }
    const char* getSrcFilename() const {
        if (location == nullptr)
            return nullptr;
        return location->getSrcFilename();
    }
    void setDest(G4_DstRegRegion* opnd);
    void setExecSize(unsigned char s);

    void computeARFRightBound();

    static bool isMaskOption(G4_InstOption opt)
    {
        return (opt & InstOpt_QuarterMasks) != 0;
    }

    void setOptions(unsigned int o)
    {
        unsigned int oldMaskOffset = getMaskOffset();
        option = o;
        unsigned int newMaskOffset = getMaskOffset();

        if (oldMaskOffset != newMaskOffset)
        {
            // Change in mask offset requires change in
            // bounds for pred/cond mod/impl acc src/dst
            computeARFRightBound();
        }
    }

    void setOptionOn(G4_InstOption o)
    {
        assert(!isMaskOption(o) && "use setMaskOption() to change emask instead");
        option |= o;
    }

    void setOptionOff(G4_InstOption o)
    {
        assert(!isMaskOption(o) && "use setMaskOption() to change emask instead");
        option &= (~o);
    }
    unsigned int getOption() const {return option;}
    unsigned int getMaskOption() const {return option & InstOpt_Masks;}
    void setMaskOption(G4_InstOption opt)
    {
        // mask options are mutually exclusive, so we have to clear any previous setting
        // note that this does not clear NoMask
        MUST_BE_TRUE(opt & InstOpt_QuarterMasks, "opt is not a valid mask option");
        setOptions((option & ~InstOpt_QuarterMasks) | opt);
    }

    void setNoMask(bool clearEMask)
    {
        if (clearEMask)
        {
            // Clear the M0/M4/M8 emask as well
            setOptions((getOption() & ~InstOpt_Masks) | InstOpt_WriteEnable);
        }
        else
        {
            setOptionOn(InstOpt_WriteEnable);
        }
    }

    bool is1QInst() const { return execSize == 8 && getMaskOffset() == 0; }
    bool isWriteEnableInst() const { return (option & InstOpt_WriteEnable) ? true : false; }
    bool isYieldInst() const { return (option & InstOpt_Switch) ? true : false; }
    bool isNoPreemptInst() const { return (option & InstOpt_NoPreempt) ? true : false; }

    void emit_inst(std::ostream& output, bool symbol_dst, bool *symbol_srcs);
    void emit(std::ostream& output, bool symbolreg = false, bool dotStyle = false);
    void emitDefUse(std::ostream& output);
    void print(std::ostream& OS) const;
    void dump() const;
    bool isValidSymbolOperand(bool &dst_valid, bool *srcs_valid) const;
    const char *getLabelStr() const;

    // get simd lane mask for this instruction. For example,
    //      add  (8|M8) ...
    // will have 0xFF00, which lane 8-15
    unsigned getExecLaneMask() const;
    unsigned char  getExecSize() const {return execSize;}
    const G4_CondMod*    getCondMod() const {return mod;}
          G4_CondMod*    getCondMod()       {return mod;}
    const G4_VarBase*    getCondModBase() const;
          G4_VarBase*    getCondModBase() {
              return const_cast<G4_VarBase*>(((const G4_INST*)this)->getCondModBase());
          }
    void setCondMod(G4_CondMod* m);

    bool isDead() const {return dead;}
    void markDead()        {dead = true;}

    bool isAligned1Inst() const { return !isAligned16Inst(); }
    bool isAligned16Inst() const { return (option & InstOpt_Align16)    ? true : false; }
    bool isAccWrCtrlInst() const { return (option & InstOpt_AccWrCtrl) ? true : false; }
    bool isAtomicInst()    const { return (option & InstOpt_Atomic)     ? true : false; }
    bool isNoDDChkInst()   const { return (option & InstOpt_NoDDChk)    ? true : false; }
    bool isNoDDClrInst()   const { return (option & InstOpt_NoDDClr)    ? true : false; }
    bool isBreakPointInst() const { return (option & InstOpt_BreakPoint) ? true : false; }

    // true if inst reads/writes acc either implicitly or explicitly
    bool useAcc() const
    {
        return isAccDstInst() || isAccSrcInst() || implAccDst != NULL || implAccSrc != NULL;
    }

    bool defAcc() const {
        return isAccDstInst() || implAccDst != NULL;
    }

    void setCompacted()      { option = option | InstOpt_Compacted; }
    void setNoCompacted()    { option = option | InstOpt_NoCompact; }
    bool isCompactedInst()  const { return (option & InstOpt_Compacted) ? true : false; }
    bool isNoCompactedInst() const { return (option & InstOpt_NoCompact) ? true : false; }

    void setLocalId(int32_t lid)  { local_id = lid; }
    int32_t getLocalId() const { return local_id; }

    void setEvenlySplitInst(bool val) { evenlySplitInst = val; }
    bool getEvenlySplitInst() { return evenlySplitInst; }

    void setCISAOff(int offset) { srcCISAoff = offset; }
    int getCISAOff() const { return srcCISAoff; }

    bool isOptBarrier() const;
    bool hasImplicitAccSrc() const
    {
       return op == G4_mac || op == G4_mach || op == G4_sada2;
    }

    bool hasImplicitAccDst() const
    {
        return op == G4_addc || op == G4_subb;
    }

    bool mayExpandToAccMacro() const;

    Gen4_Operand_Number getSrcOperandNum(int srcPos) const
    {
        switch (srcPos)
        {
        case 0:
            return Opnd_src0;
        case 1:
            return Opnd_src1;
        case 2:
            return Opnd_src2;
        case 3:
            return Opnd_src3;
        default:
            MUST_BE_TRUE(false, "bad source id");
            return Opnd_src0;
        }
    }
    static int getSrcNum(Gen4_Operand_Number opndNum)
    {
        MUST_BE_TRUE(isSrcNum(opndNum), "not a source number");
        return opndNum - 1;
    }
    static bool isSrcNum(Gen4_Operand_Number opndNum)
    {
        return opndNum == Opnd_src0 || opndNum == Opnd_src1 ||
               opndNum == Opnd_src2 || opndNum == Opnd_src3;
    }
    const G4_Operand* getOperand(Gen4_Operand_Number opnd_num) const;
          G4_Operand* getOperand(Gen4_Operand_Number opnd_num)
    {
        return const_cast<G4_Operand*>(((const G4_INST *)this)->getOperand(opnd_num));
    }

    /// Remove all definitons that contribute to this[opndNum] and remove all
    /// uses from their corresponding definitions. To maintain def-use's, this
    /// is required while resetting a source operand.
    void removeDefUse(Gen4_Operand_Number opndNum);
    /// Remove a use from this instruction and update its correponding def.
    /// Returns the next use iterator of this instruction.
    USE_EDGE_LIST_ITER eraseUse(USE_EDGE_LIST_ITER iter);
    /// Remove all uses defined by this. To maintain def-use's, this is
    /// required to clear useInstList.
    void removeAllUses();
    /// Remove all defs that used by this. To maintain def-use's, this is
    /// required to clear defInstList.
    void removeAllDefs();
    void transferDef(G4_INST *inst2, Gen4_Operand_Number opndNum1,
                     Gen4_Operand_Number opndNum2);
    void transferUse(G4_INST *inst2, bool keepExisting = false);
    /// Copy this[opndNum1]'s definition to inst2[opndNum2]'s definition.
    void copyDef(G4_INST *inst2, Gen4_Operand_Number opndNum1,
                 Gen4_Operand_Number opndNum2, bool checked = false);
    /// Copy this instructions's defs to inst2. If checked is true, then only
    /// copy those effective defs.
    void copyDefsTo(G4_INST *inst2, bool checked);
    /// Copy this instruction's uses to inst2. If checked is true, then only
    /// copy those effective uses.
    void copyUsesTo(G4_INST *inst2, bool checked);
    void removeUseOfInst( );
    void trimDefInstList();
    void swapDefUse();
    void addDefUse(G4_INST* use, Gen4_Operand_Number usePos);
    void uniqueDefUse()
    {
        useInstList.unique();
        defInstList.unique();
    }
    void clearUse() { useInstList.clear(); }
    void clearDef() { defInstList.clear(); }
    bool useEmpty() const { return useInstList.empty(); }
    bool hasOneUse() const { return useInstList.size() == 1; }
    /// Returns its definition if this's operand has a single definition. Returns
    /// 0 otherwise.
    G4_INST *getSingleDef(Gen4_Operand_Number opndNum, bool MakeUnique = false);
    USE_EDGE_LIST::const_iterator use_begin() const { return useInstList.begin(); }
    USE_EDGE_LIST::iterator       use_begin()       { return useInstList.begin(); }
    USE_EDGE_LIST::const_iterator use_end() const { return useInstList.end(); }
    USE_EDGE_LIST::iterator       use_end()       { return useInstList.end(); }
    USE_EDGE_LIST::reference      use_front() { return useInstList.front(); }
    USE_EDGE_LIST::reference      use_back() { return useInstList.back(); }
    DEF_EDGE_LIST::const_iterator def_begin() const { return defInstList.begin(); }
    DEF_EDGE_LIST::iterator       def_begin()       { return defInstList.begin(); }
    DEF_EDGE_LIST::const_iterator def_end() const { return defInstList.end(); }
    DEF_EDGE_LIST::iterator       def_end()       { return defInstList.end(); }
    DEF_EDGE_LIST::reference      def_front() { return defInstList.front(); }
    DEF_EDGE_LIST::reference      def_back() { return defInstList.back(); }
    size_t use_size() const { return useInstList.size(); }
    size_t def_size() const { return defInstList.size(); }
    void dumpDefUse();
    template <typename Compare> void sortUses(Compare Cmp)
    {
        useInstList.sort(Cmp);
    }

    void fixMACSrc2DefUse();
    void setImplAccSrc( G4_Operand* opnd );
    void setImplAccDst( G4_DstRegRegion* opnd );

    bool isWAWdep( G4_INST *inst ); /* not const: may compute bound */
    bool isWARdep( G4_INST *inst ); /* not const: may compute bound */
    bool isRAWdep( G4_INST *inst ); /* not const: may compute bound */
    const G4_Operand* getImplAccSrc() const { return implAccSrc; }
          G4_Operand* getImplAccSrc()       { return implAccSrc; }
    const G4_DstRegRegion* getImplAccDst() const { return implAccDst; }
          G4_DstRegRegion* getImplAccDst()       { return implAccDst; }
    uint16_t getMaskOffset() const;
    static G4_InstOption offsetToMask(int execSize, int offset, bool nibOk);
    bool isRawMov() const;
    bool hasACCSrc() const;
    bool hasACCOpnd() const;
    G4_Type getOpExecType( int& extypesize );
    bool canHoistTo(const G4_INST *defInst, bool simdBB) const;
    enum MovType {
        Copy        = 0,        // MOV is a copy.
        ZExt        = 1,        // MOV is a zero extension.
        SExt        = 2,        // MOV is a sign extension.
        Trunc       = 3,        // MOV is a truncation.
        IntToFP     = 4,        // MOV is a conversion from Int to Float.
        FPToInt     = 5,        // MOV is a conversion from Float to Int.
        FPUpConv    = 6,        // MOV is a conversion from low precision to
                                // high precision.
        FPDownConv  = 7,        // MOV is a conversion from high precision to
                                // low precision.
        FPDownConvSafe  = 8,        // Float down conversion for DX shaders.
        SuperMov        = 9,        // MOV is a mov with other effects.
    };
    MovType canPropagate() const;
    bool canPropagateTo(G4_INST *useInst, Gen4_Operand_Number opndNum, MovType MT, bool inSimdFlow); /* not const */
    G4_Type getPropType(Gen4_Operand_Number opndNum, MovType MT, const G4_INST *mov) const;
    bool isSignSensitive(Gen4_Operand_Number opndNum) const;
    bool canHoist(bool simdBB, const Options *opt) const;
    bool isCommutative() const;
    bool canUseACCOpt(bool handleComprInst, bool checkRegion,
        uint16_t &hs, bool allowTypeDemotion, bool insertMov = false);

    bool hasNULLDst() const;
    bool goodTwoGRFDst(bool& evenSplitDst);
    const BinInst *getBinInst() const { return bin; };
          BinInst *getBinInst()       { return bin; };
    void        setBinInst(BinInst *_bin) { bin = _bin; };
    void setGenOffset(int64_t off) { genOffset = off; }
    int64_t getGenOffset() const { return genOffset; }

    void computeLeftBoundForImplAcc(G4_Operand* opnd);

    void setNoSrcDepSet(bool val)
    {
         if (val)
         {
             option |= InstOpt_NoSrcDepSet;
         }
         else
         {
             option &= ~InstOpt_NoSrcDepSet;
         }
    }

    bool isNoSrcDepSet() const
    {
        return (option & InstOpt_NoSrcDepSet) != 0;
    }
    bool isMixedMode() const;
    bool canSupportCondMod() const;
    bool canSwapSource() const;
    bool canSupportSaturate() const;
    bool canSupportSrcModifier() const;

    bool usesFlag() const
    {
        return predicate != nullptr || (op != G4_sel && mod != nullptr);
    }

    bool is2SrcAlign16() const
    {
        return op == G4_dp2 || op == G4_dp3 || op == G4_dp4 || op == G4_dph;
    }
    bool isFastHFInstruction(void) const;

    bool isAlign1Ternary() const;

    // if instruction requries operansd to have DW (D/UD) type
    bool needsDWType() const
    {
        return op == G4_mulh;
    }

    bool canExecSizeBeAcc(Gen4_Operand_Number opndNum) const;
    bool canDstBeAcc() const;
    bool canSrcBeAcc(Gen4_Operand_Number opndNum) const;

    TARGET_PLATFORM getPlatform() const;

private:
    bool detectComprInst() const;
    bool isLegalType(G4_Type type, Gen4_Operand_Number opndNum) const;
    bool isFloatOnly() const;
};
} // namespace vISA

std::ostream& operator<<(std::ostream& os, vISA::G4_INST& inst);

namespace vISA
{


class G4_InstMath : public G4_INST
{
    G4_MathOp mathOp;
public:

    G4_InstMath(
        const IR_Builder& builder,
        G4_Predicate* prd,
        G4_opcode o,
        G4_CondMod* m,
        bool sat,
        unsigned char size,
        G4_DstRegRegion* d,
        G4_Operand* s0,
        G4_Operand* s1,
        unsigned int opt,
        G4_MathOp mOp = MATH_RESERVED) :
        G4_INST(builder, prd, o, m, sat, size, d, s0, s1, opt),
        mathOp(mOp)
    {

    }

    G4_INST* cloneInst() override;

    bool isIEEEMath() const { return mathOp == MATH_INVM || mathOp == MATH_RSQRTM; }
    bool isMathIntDiv() const { return mathOp >= MATH_INT_DIV && mathOp < MATH_INVM; }
    bool isOneSrcMath() const
    {
        return
            mathOp == MATH_INV ||
            mathOp == MATH_LOG ||
            mathOp == MATH_EXP ||
            mathOp == MATH_SQRT ||
            mathOp == MATH_RSQ ||
            mathOp == MATH_SIN ||
            mathOp == MATH_COS ||
            mathOp == MATH_RSQRTM;
    }

    G4_MathOp getMathCtrl() const { return mathOp; }
};

class G4_InstCF : public G4_INST
{
    // operands for CF instructions
    // -- if, else, endif, while, break, cont, goto: JIP and UIP are used for the branch target.
    // -- jmpi: src0 stores the branch target, and it can be either a label (direct) or a SrcRegRegion (indirect)
    // -- call: src0 stores the callee address, and it must be a label
    // -- fcall: src0 stores the callee address, and it can be either a label (direct) or a SrcRegRegion (indirect).
    //           dst contains the ret IP and call mask.
    // -- ret, fret: src0 contains the ret IP and call mask
    // Note that for call/ret the retIP variable is not created till RA
    G4_Label*       jip; // GT JIP.
    G4_Label*         uip; // GT UIP.
    // list of labels that this instruction could jump to.  Only used for switch jmps
    std::list<G4_Label*> indirectJmpTarget;

    // True if this is a backward branch (including while)
    bool isBackwardBr;

    // True if this branch is a uniform. By uniform, it means that all active lanes
    // at the branch goes to the same target (Valid for if/while/break/goto/jmpi only.
    // This info could be encoded in instOpt.).  Note that all active lanes at the
    // branch could be subset of all active lanes on entry to shader/kernel.
    bool isUniformBr;

public:

    static const uint32_t unknownCallee = 0xFFFF;

    // used by non jmp/call/ret instructions
    G4_InstCF(const IR_Builder& builder,
        G4_Predicate* prd,
        G4_opcode op,
        unsigned char size,
        G4_Label* jipLabel,
        G4_Label* uipLabel,
        uint32_t instOpt) :
        G4_INST(builder, prd, op, nullptr, false, size, nullptr, nullptr, nullptr, instOpt),
        jip(jipLabel), uip(uipLabel), isBackwardBr(op == G4_while), isUniformBr(false)
    {
        isUniformBr = (op == G4_jmpi ||
                       (op == G4_goto && (size == 1 || prd == nullptr)));
    }

    // used by jump/call/ret
    G4_InstCF(
        const IR_Builder& builder,
        G4_Predicate* prd,
        G4_opcode o,
        G4_CondMod* m,
        bool sat,
        unsigned char size,
        G4_DstRegRegion* d,
        G4_Operand* s0,
        unsigned int opt) :
        G4_INST(builder, prd, o, m, sat, size, d, s0, nullptr, opt),
        jip(NULL), uip(NULL), isBackwardBr(o == G4_while), isUniformBr(false)
    {
        isUniformBr = (op == G4_jmpi ||
            (op == G4_goto && (size == 1 || prd == nullptr)));
    }

    bool isCFInst() const override { return true; }

    void setJip(G4_Label* opnd) {jip = opnd;}
    const G4_Label* getJip() const {return jip;}
          G4_Label* getJip()       {return jip;}
    const char* getJipLabelStr() const;

    void setUip(G4_Label* opnd) {uip = opnd;}
    const G4_Label* getUip() const {return uip;}
          G4_Label* getUip()       {return uip;}
    const char* getUipLabelStr() const;

    void addIndirectJmpLabel(G4_Label* label)
    {
        MUST_BE_TRUE(isIndirectJmp(), "may only be called for indirect jmp");
        indirectJmpTarget.push_back(label);
    }
    const std::list<G4_Label*>& getIndirectJmpLabels()
    {
        MUST_BE_TRUE(isIndirectJmp(), "may only be called for indirect jmp");
        return indirectJmpTarget;
    }

    void setBackward(bool val) {isBackwardBr = val;}

    bool isBackward() const {return isBackwardBr;}

    void setUniform(bool val) { isUniformBr = val; }
    bool isUniform() const { return isUniformBr; }

    bool isIndirectJmp() const;

    bool isUniformGoto(unsigned KernelSimdSize) const;

    bool isIndirectCall() const;

    // for direct call, this is null till after the compilation units are stitched together
    // for indirect call, this is src0
    G4_Operand* getCalleeAddress() const
    {
        if (op == G4_pseudo_fcall)
        {
            return getSrc(0);
        }
        else
        {
            return nullptr;
        }
    }
};

class G4_InstSend : public G4_INST
{
    G4_SendMsgDescriptor* msgDesc;

public:

    // send (one source)
    // desc is either imm or a0.0 and in src1
    // extDesc is always immediate and encoded in md
    G4_InstSend(
        const IR_Builder& builder,
        G4_Predicate* prd,
        G4_opcode o,
        unsigned char size,
        G4_DstRegRegion* dst,
        G4_SrcRegRegion* payload,
        G4_Operand* desc,
        uint32_t opt,
        G4_SendMsgDescriptor* md);

    // split send (two source)
    // desc is either imm or a0.0 and in src2
    // extDesc is either imm or a0.N and in src3
    G4_InstSend(
        const IR_Builder& builder,
        G4_Predicate* prd,
        G4_opcode o,
        unsigned char size,
        G4_DstRegRegion* dst,
        G4_SrcRegRegion* payload,
        G4_SrcRegRegion* src1,
        G4_Operand* desc,
        G4_Operand* extDesc,
        uint32_t opt,
        G4_SendMsgDescriptor* md);

    bool isSendc() const { return op == G4_sendc || op == G4_sendsc; }
    void setSendc()
    {
        // no effect if op is already G4_sendc/G4_sendsc
        if (op == G4_send)
        {
            op = G4_sendc;
        }
        else if (op == G4_sends)
        {
            op = G4_sendsc;
        }
    }
    bool mayExceedTwoGRF() const override { return true; }

    G4_Operand* getMsgDescOperand() const
    {
        return isSplitSend() ? srcs[2] : srcs[1];
    }

    G4_Operand* getMsgExtDescOperand() const
    {
        assert(isSplitSend() && "must be a split send instruction");
        return srcs[3];
    }

    G4_SendMsgDescriptor *getMsgDesc() const override
    {
        return msgDesc;
    }

    void setMsgDesc(G4_SendMsgDescriptor *in)
    {
        assert(in && "null descriptor not expected");
        msgDesc = in;
        resetRightBound((G4_Operand*)dst);
        resetRightBound(srcs[0]);
    }

    // restrictions on whether a send may be EOT:
    // -- The posted destination operand must be null
    // -- A thread must terminate with a send instruction with message to a shared function on the output message bus;
    //    therefore, it cannot terminate with a send instruction with message to the following shared functions: Sampler unit, NULL function
    //    For example, a thread may terminate with a URB write message or a render cache write message.
    // -- A root thread originated from the media (generic) pipeline must terminate
    //    with a send instruction with message to the Thread Spawner unit. A child
    //    thread should also terminate with a send to TS.
    bool canBeEOT()
    {
        bool canEOT = msgDesc->ResponseLength() == 0 &&
            (msgDesc->getFuncId() != SFID::NULL_SFID &&
                msgDesc->getFuncId() != SFID::SAMPLER);

        return canEOT;
    }

    bool isFence() const {return getMsgDesc()->isFence();}

    bool isEOT() const override {return msgDesc->isEOTInst();}

    bool isDirectSplittableSend();

    void computeRightBound(G4_Operand* opnd) override;

    void emit_send(std::ostream& output, bool symbol_dst, bool *symbol_srcs);
    void emit_send(std::ostream& output, bool dotStyle = false);
    void emit_send_desc(std::ostream& output);

    void setSerialize()
    {
        option = option | InstOpt_Serialize;
    }
    bool isSerializedInst() const { return (option & InstOpt_Serialize) ? true : false; }

};


}

enum PseudoKillType
{
    FromLiveness = 1,
    Src = 2,
    Other = 3
};

// a special intrinsic instruction for any pseudo operations. An intrinsic inst has the following characteristics
// -- it is modeled as a call to some unknown function
// -- 1 dst and up to 3 srcs are allowed for the intrinsic
// -- conditonal modifier and saturation are currently not allowed (can add later)
// -- an intrinsic may reserve additional GRF/addr/flag for its code gen, which RA needs to honor
// -- it must be lowered/deleted before certain phases in the finalizer (no later than binary encoding)

// all intrinsic opcode go here
// order must match that of the G4_Intrinsics table
enum class Intrinsic
{
    Wait,
    Use,
    MemFence,
    PseudoKill,
    PseudoUse,  // ToDo: can we merge Use and PseudoUse? former is from input while latter is internally generated.
    Spill,
    Fill,
    Split,
    CallerSave,
    CallerRestore,
    CalleeSave,
    CalleeRestore,
    NumIntrinsics
};

enum class Phase
{
    CFG,
    Optimizer,
    HWConformity,
    RA,
    Scheduler,
    BinaryEncoding
};

struct IntrinsicInfo
{
    Intrinsic id;
    const char* name;
    int numDst;
    int numSrc;
    Phase loweredBy;    //intrinsic must be lowered before entering this phase
    struct {
        int numTmpGRF;       //number of tmp GRFs needed for this intrinsic
        int numTmpAddr;     //number of tmp addresses needed (in unit of uw)
        int numTmpFlag;     //number of tmp flags needed (in unit of 16-bit)
        bool useR0;
        bool useA0;
    } temps;
};

static const IntrinsicInfo G4_Intrinsics[(int)Intrinsic::NumIntrinsics] =
{
    //  id                      name            numDst  numSrc  loweredBy               temp
    {Intrinsic::Wait,           "wait",         0,      0,      Phase::Optimizer,       { 0, 0, 0, false, false } },
    {Intrinsic::Use,            "use",          0,      1,      Phase::Scheduler,       { 0, 0, 0, false, false } },
    {Intrinsic::MemFence,       "mem_fence",    0,      0,      Phase::BinaryEncoding,  { 0, 0, 0, false, false } },
    {Intrinsic::PseudoKill,     "pseudo_kill",  1,      1,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::PseudoUse,      "pseudo_use",   0,      1,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::Spill,          "spill",        1,      2,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::Fill,           "fill",         1,      1,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::Split,          "split",        1,      1,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::CallerSave,     "caller_save",  1,      0,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::CallerRestore,  "caller_restore", 0,    1,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::CalleeSave,     "callee_save",  1,      0,      Phase::RA,              { 0, 0, 0, false, false } },
    {Intrinsic::CalleeRestore,  "callee_restore", 0,    1,      Phase::RA,              { 0, 0, 0, false, false } },
};

namespace vISA
{
class G4_InstIntrinsic : public G4_INST
{
    const Intrinsic intrinsicId;

    // these should be set by RA if intrinsic requires tmp GRF/addr/flag
    int tmpGRFStart;
    int tmpAddrStart;
    int tmpFlagStart;

public:

    G4_InstIntrinsic(
        const IR_Builder& builder,
        G4_Predicate* prd,
        Intrinsic intrinId,
        uint8_t size,
        G4_DstRegRegion* d,
        G4_Operand* s0,
        G4_Operand* s1,
        G4_Operand* s2,
        unsigned int opt) :
        G4_INST(builder, prd, G4_intrinsic, nullptr, false, size, d, s0, s1, s2, opt),
        intrinsicId(intrinId), tmpGRFStart(-1), tmpAddrStart(-1), tmpFlagStart(-1)
    {

    }

    G4_INST* cloneInst() override;

    int getNumDst() const { return G4_Intrinsics[(int) intrinsicId].numDst; }
    int getNumSrc() const { return G4_Intrinsics[(int) intrinsicId].numSrc; }

    Intrinsic   getIntrinsicId()    const { return intrinsicId; }
    const char* getName()           const { return G4_Intrinsics[(int) intrinsicId].name; }
    Phase       getLoweredByPhase() const { return G4_Intrinsics[(int)intrinsicId].loweredBy; }

    int getTmpGRFStart() const { return tmpGRFStart; }
    void setTmpGRFStart(int startGRF) { tmpGRFStart = startGRF; }
    int getTmpAddrStart() const { return tmpAddrStart; }
    void setTmpAddrStart(int startAddr) { tmpAddrStart = startAddr; }
    int getTmpFlagStart() const { return tmpFlagStart; }
    void setTmpFlagStart(int startFlag) { tmpFlagStart = startFlag; }
};
}

// RegionWH and RegionV are special for the different modes of source register indirect addressing
// RegionWH = <width, horzStride>, we set vertStride to UNDEFINED_SHORT
// RegionV = <horzStride>, we set both vertStride and width to UNDEFINED_SHORT
//
struct RegionDesc
{
    const uint16_t vertStride;
    const uint16_t width;
    const uint16_t horzStride;

    RegionDesc(uint16_t vs, uint16_t w, uint16_t hs) : vertStride(vs), width(w), horzStride(hs)
    {
        assert(isLegal() && "illegal region desc");
    }
    void* operator new(size_t sz, vISA::Mem_Manager& m) {return m.alloc(sz);}

    // The legal values for Width are {1, 2, 4, 8, 16}.
    // The legal values for VertStride are {0, 1, 2, 4, 8, 16, 32}.
    // The legal values for HorzStride are {0, 1, 2, 4}.
    bool isLegal() const {return isLegal(vertStride, width, horzStride);}

    static bool isLegal(unsigned vs, unsigned w, unsigned hs);

    enum RegionDescKind {
        RK_Other,   // all others like <4; 2, 1> etc.
        RK_Stride0, // <0;1,0> aka scalar
        RK_Stride1, // <1;1,0> aka contiguous
        RK_Stride2, // <2;1,0>
        RK_Stride4  // <4;1,0>
    };

    // Determine the region description kind. Strided case only.
    static RegionDescKind getRegionDescKind(uint16_t size, uint16_t vstride,
                                            uint16_t width, uint16_t hstride);


    bool isRegionWH() const {return vertStride == UNDEFINED_SHORT && width != UNDEFINED_SHORT;}
    bool isRegionV() const {return vertStride == UNDEFINED_SHORT && width == UNDEFINED_SHORT;}
    bool isScalar() const { return ( vertStride == 0 && horzStride == 0 ) || ( width == 1 && vertStride == 0 ); }        // to support decompression
    bool isRegionSW() const {return vertStride != UNDEFINED_SHORT && width == UNDEFINED_SHORT && horzStride == UNDEFINED_SHORT;}
    bool isEqual(const RegionDesc *r) const { return vertStride == r->vertStride && width == r->width && horzStride == r->horzStride; }        // to support re-compression
    void emit(std::ostream& output) const;
    bool isPackedRegion() const { return ( ( horzStride == 0 && vertStride <= 1 ) || ( horzStride == 1 && vertStride <= width ) ); }
    bool isFlatRegion() const { return ( isScalar() || vertStride == horzStride * width ); }
    bool isRepeatRegion(unsigned short execSize) const { return ( !isScalar() && ( execSize > width && vertStride < horzStride * width ) ); }

    // Contiguous regions are:
    // (1) ExSize is 1, or
    // (2) <1; 1, *> with arbitrary ExSize, or
    // (3) <N; N, 1> with arbitrary ExSize, or
    // (4) <*; N, 1> with ExSize == N.
    //
    // A region is contiguous iff sequence
    // { f(0, 0), f(0, 1), ..., f(1, 0), ..., f(ExSize / width - 1, width - 1) }
    // has a common difference 1, where
    //
    // f(i, j) = i x vstride + j x hstride
    //
    // for 0 <= i < ExSize / width and 0 <= j < width
    bool isContiguous(unsigned ExSize) const;
    bool isSingleNonUnitStride(uint32_t execSize, uint16_t& stride) const;
    bool isSingleStride(uint32_t execSize, uint16_t &stride) const;
    bool isSingleStride(uint32_t execSize) const
    {
        uint16_t stride = 0;
        return isSingleStride(execSize, stride);
    }
};

namespace vISA
{
    class LiveIntervalInfo
    {
    public:
        enum DebugLiveIntervalState
        {
            Open = 0,
            Closed = 1
        };

    private:
        std::list<std::pair<uint32_t, uint32_t>> liveIntervals;
        uint32_t cleanedAt;
        DebugLiveIntervalState state;
        uint32_t openIntervalVISAIndex;

    public:
        void *operator new(size_t sz, Mem_Manager& m){ return m.alloc(sz); }

        void addLiveInterval(uint32_t start, uint32_t end);
        void liveAt(uint32_t cisaOff);
        void getLiveIntervals(std::vector<std::pair<uint32_t, uint32_t>>& intervals);
        void clearLiveIntervals() { liveIntervals.clear(); }

        DebugLiveIntervalState getState() { return state; }

        void setStateOpen(uint32_t VISAIndex)
        {
            //MUST_BE_TRUE(state == Closed, "Cannot open internal in Open state");
            state = Open;
            openIntervalVISAIndex = VISAIndex;
        }

        void setStateClosed(uint32_t VISAIndex)
        {
            //MUST_BE_TRUE(state == Open, "Cannot close interval in Close state");
            state = Closed;
            addLiveInterval(VISAIndex, openIntervalVISAIndex);
        }

        LiveIntervalInfo() { cleanedAt = 0; state = Closed; openIntervalVISAIndex = 0; }
        ~LiveIntervalInfo() { }
    };
}

typedef enum class AugmentationMasks
{
    Undetermined = 0,
    Default16Bit = 1,
    Default32Bit = 2,
    Default64Bit = 3,
    DefaultPredicateMask = 4,
    NonDefault = 5,
} AugmentationMasks;

namespace vISA
{
class G4_Declare
{
    friend class IR_Builder;

    const char*        name;        // Var_Name
    G4_RegFileKind    regFile;    // from which reg file
    G4_Type            elemType;    // element type

    G4_RegVar*        regVar;        // corresponding reg var

    G4_Declare *    AliasDCL;    // Alias Declare
    unsigned        AliasOffset;    // Alias Offset

    unsigned        startID;

    uint16_t spillFlag : 1;    // Indicate this declare gets spill reg
    uint16_t addressed : 1;     // whether this declare is address-taken

    uint16_t doNotSpill : 1;    // indicates that this declare should never be spilled

    uint16_t liveIn : 1;   // indicate if this varaible has "Input" or "Input_Output" attribute
    uint16_t liveOut : 1;  // indicate if this varaible has "Output" or "Input_Output" attribute

    // This is an optimization *hint* to indicate if optimizer should skip
    // widening this variable or not (e.g. byte to word).
    uint16_t noWidening : 1;

    uint16_t capableOfReuse : 1;
    uint16_t isSplittedDcl : 1;
    uint16_t isPartialDcl : 1;
    uint16_t refInSend : 1;

    unsigned int   decl_id;     // global decl id for this builder

    uint32_t numElements;
    unsigned int numFlagElements;

    // byte offset of this declare from the base declare.  For top-level declares this value is 0
    int offsetFromBase;

    // if set to nonzero, indicates the declare is only used by subroutine "scopeID".
    // it is used to prevent a subroutin-local declare from escaping its subroutine when doing liveness
    unsigned scopeID;

    // For GRFs, store byte offset of allocated GRF
    unsigned int GRFBaseOffset;

    // fields that are only ever referenced by RA and spill code
    // ToDo: they should be moved out of G4_Declare and stored as maps in RA/spill
    G4_Declare*     spillDCL;  // if an addr/flag var is spilled, SpillDCL is the location (GRF) holding spilled value

    G4_Declare* addrTakenSpillFillDcl; // dcl to use for address taken spill/fill temp

    // this should only be called by builder
    void setNumberFlagElements(uint8_t numEl)
    {
        assert(regFile == G4_FLAG && "may only be called on a flag");
        numFlagElements = numEl;
    }

public:
    G4_Declare(const char*    n,
               G4_RegFileKind k,
               uint32_t numElems,
               G4_Type        ty,
               std::vector<G4_Declare*>& dcllist) :
      name(n), regFile(k), elemType(ty), addressed(false), liveIn(false),
      liveOut(false), noWidening(false), isSplittedDcl(false), isPartialDcl(false),
      refInSend(false), numElements(numElems), offsetFromBase(-1)
    {
        //
        // set the rest values to default uninitialized values
        //

        regVar        = NULL;
        AliasDCL = NULL;
        AliasOffset = 0;

        if (k == G4_FLAG)
        {
            //need original number of elements for any*
            numFlagElements = numElems * 16;
        }
        else
        {
            numFlagElements = 0;
        }

        spillFlag = false;
        spillDCL = NULL;

        addrTakenSpillFillDcl = NULL;

        startID = 0;

        doNotSpill = false;
        capableOfReuse = false;

        scopeID = 0;

        GRFBaseOffset = 0;
        decl_id = (uint32_t) dcllist.size();
        dcllist.push_back(this);
    }

    ~G4_Declare()
    {
    }

    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    void setGRFBaseOffset( unsigned int offset ) { GRFBaseOffset = offset; }
    unsigned int getGRFBaseOffset() const { return GRFBaseOffset; }

    void setLiveIn() { liveIn = true; }
    bool isLiveIn() const { return liveIn; }
    void setLiveOut() { liveOut = true; }

    void setDoNotWiden() { noWidening = true; }
    bool doNotWiden() const { return noWidening; }

    unsigned getScopeID() const { return scopeID;  }
    void updateScopeID(unsigned id) { if (!isInput() && (scopeID < id)) scopeID = id; }

    //
    // functions for setting values
    //
    void setRegVar(G4_RegVar* rv)
    {
        MUST_BE_TRUE(regVar == NULL, ERROR_UNKNOWN);
        regVar = rv;
    }

    // caller manages the name str
    void setName(const char* newName)
    {
        name = newName;
    }

    unsigned int getByteSize() const { return numElements * getElemSize(); }

    unsigned int getWordSize() const {return (getByteSize() + 1)/2;}

    void resizeNumRows( unsigned int numrows )
    {
        int byteSize = numrows * GENX_GRF_REG_SIZ;
        setTotalElems(byteSize / getElemSize());
    }

    void setAddrTakenSpillFill( G4_Declare* dcl )
    {
        addrTakenSpillFillDcl = dcl;
    }

    const G4_Declare* getAddrTakenSpillFill() const { return addrTakenSpillFillDcl; }
          G4_Declare* getAddrTakenSpillFill()       { return addrTakenSpillFillDcl; }

    // declare this to be aliased to dcl+offset
    // This is an error if dcl+offset is not aligned to the type of this dcl
    void setAliasDeclare(G4_Declare* dcl, unsigned int offset)
    {
        AliasDCL = dcl;
        AliasOffset = offset;
        offsetFromBase = -1;
    }

    void resetSpillFlag()
    {
        // This function is invoked from rematerialization pass.
        if (getAliasDeclare())
            getAliasDeclare()->resetSpillFlag();
        spillFlag = false;
    }
    void setSpillFlag()
    {
        if(getAliasDeclare() != NULL)
        {
            // Iterate to top level dcl to set spill flag
            getAliasDeclare()->setSpillFlag();
        }
        spillFlag = true;
    }
    bool isSpilled() const
    {
        if (getAliasDeclare() != NULL)
        {
            return getAliasDeclare()->isSpilled();
        }

        // Following executed only if G4_Declare doesnt have an alias
        return spillFlag;
    }

    bool isEvenAlign() const;
    G4_SubReg_Align getSubRegAlign() const;
    void        setEvenAlign();
    void        setSubRegAlign(G4_SubReg_Align subAl);

    void copyAlign(G4_Declare* dcl);

    unsigned getByteAlignment() const
    {
        // we only consider subalign here
        unsigned byteAlign = getSubRegAlign() * G4_Type_Table[Type_UW].byteSize;
        return byteAlign < G4_Type_Table[elemType].byteSize ?
            G4_Type_Table[elemType].byteSize : byteAlign;
    }

    void setRegFile(G4_RegFileKind rfile) { regFile = rfile; }

    bool useGRF() const { return (regFile & (G4_GRF | G4_INPUT)) != 0; }
    bool isInput() const { return liveIn || ((regFile & G4_INPUT) != 0); }
    bool isOutput() const { return liveOut; }

    //
    // retrieving functions
    //
    unsigned          getAliasOffset() const {return AliasOffset;}
    const G4_Declare *getAliasDeclare() const {return AliasDCL;}
          G4_Declare *getAliasDeclare()       {return AliasDCL;}
    const G4_Declare *getRootDeclare() const
    {
        const G4_Declare* rootDcl = this;
        while (rootDcl->getAliasDeclare() != NULL)
        {
            rootDcl = rootDcl->getAliasDeclare();
        }
        return rootDcl;
    }
    G4_Declare       *getRootDeclare() {
        return const_cast<G4_Declare*>(((const G4_Declare *)this)->getRootDeclare());
    }

    // like above, but also return the alias offset in bytes
    const G4_Declare*    getRootDeclare(uint32_t& offset) const
    {
        const G4_Declare* rootDcl = this;
        offset = 0;
        while (rootDcl->getAliasDeclare() != NULL)
        {
            offset += AliasOffset;
            rootDcl = rootDcl->getAliasDeclare();
        }
        return rootDcl;
    }
    G4_Declare* getRootDeclare(uint32_t& offset) {
        return const_cast<G4_Declare*>(((const G4_Declare *)this)->getRootDeclare(offset));
    }

    const char*       getName() const {return name;}
    G4_RegFileKind getRegFile() const {return regFile;}

    // returns number of elements per row
    unsigned short getNumElems() const
    {
        return getNumRows() > 1 ? G4_GRF_REG_NBYTES / getElemSize() : numElements;
    }
    unsigned short getNumRows() const
    {
        return (getByteSize() + (G4_GRF_REG_NBYTES - 1))/G4_GRF_REG_NBYTES;
    }
    unsigned short getTotalElems() const
    {
        return (unsigned short)numElements;
    }

    void setTotalElems(uint32_t numElems) { numElements = numElems; }
    unsigned short getNumberFlagElements() const
    {
        assert(regFile == G4_FLAG && "should only be called for flag vars");
        return numFlagElements;
    }

    G4_Type          getElemType() const {return elemType;}
    uint16_t         getElemSize() const {return static_cast<uint16_t>(G4_Type_Table[elemType].byteSize);}
    const G4_RegVar *getRegVar() const {return regVar;}
          G4_RegVar *getRegVar()       {return regVar;}

    int getOffsetFromBase()
    {
        if( offsetFromBase == -1 )
        {
            offsetFromBase = 0;
            for( G4_Declare *dcl = this; dcl->getAliasDeclare() != NULL; dcl = dcl->getAliasDeclare() )
            {
                offsetFromBase += dcl->getAliasOffset();
            }
        }
        return offsetFromBase;
    }

    void        setSpilledDeclare(G4_Declare* sd) {spillDCL = sd;}
    const G4_Declare* getSpilledDeclare() const {return spillDCL;}
          G4_Declare* getSpilledDeclare()  {return spillDCL;}

    unsigned getDeclId() const { return decl_id; }

    void setIsSplittedDcl(bool b) { isSplittedDcl = b; }
    bool getIsSplittedDcl() const { return isSplittedDcl; }

    void setIsPartialDcl(bool b) { isPartialDcl = b; }
    bool getIsPartialDcl() const { return isPartialDcl; }

    void setIsRefInSendDcl(bool b) { refInSend |= b;}
    bool getIsRefInSendDcl() const { return refInSend; }

    void        setSplitVarStartID(unsigned id) { startID = id; };
    unsigned    getSplitVarStartID() { return startID; };

    void setDoNotSpill()      { doNotSpill = true; }
    bool isDoNotSpill() const { return doNotSpill; }

    bool isMsgDesc() const { return regFile == G4_ADDRESS && elemType == Type_UD; }

    void setCapableOfReuse()       { capableOfReuse = true; }
    bool getCapableOfReuse() const { return capableOfReuse; }

    void    setAddressed() { addressed = true; }
    bool    getAddressed() const {
        if (addressed)
        {
            return true;
        }
        if (AliasDCL)
        {
            return AliasDCL->getAddressed();
        }
        else
        {
            return false;
        }
    }

    void emit(std::ostream& output) const;

    void dump() const { emit(std::cerr); }

    void prepareForRealloc(G4_Kernel*);
};
}
typedef std::vector<vISA::G4_Declare*> DECLARE_LIST;
typedef std::vector<vISA::G4_Declare*>::iterator DECLARE_LIST_ITER;

namespace vISA
{
class G4_VarBase;

class G4_Operand
{
    friend class G4_INST;
    friend class G4_InstSend;
    friend class G4_FillIntrinsic;
    friend class G4_SpillIntrinsic;


public:
    enum Kind {
        immediate,
        srcRegRegion,
        dstRegRegion,
        predicate,      // instruction predicate
        condMod,        // condition modifier
        addrExp,
        label
    };
    virtual ~G4_Operand() {}
protected:
    Kind kind;
    G4_Type type;
    G4_INST *inst;

    // fields used to compare operands
    G4_Declare *top_dcl;
    G4_VarBase *base;

    uint64_t bitVec[2];  // bit masks at byte granularity (for flags, at bit granularity)

    bool rightBoundSet;
    unsigned byteOffset;
    G4_AccRegSel accRegSel;

    // [left_bound, right_bound] describes the region in the root variable that this operand touches.
    // for variables and addresses:
    //  lb = offset of the first byte of the first element
    //  rb = offset of the last byte of the last element
    //  for non-send instructions, (rb - lb) < 64 always holds since operand can't cross 2GRF boundary
    //  for send instructions, rb is determined by the message/response length
    // for flags:
    //  lb = bit offset of the first flag bit
    //  rb = bit offset of the last flag bit
    //  (rb - lb) < 32 always holds for flags
    //  for predicate and conditonal modifers, the bounds are also effected by the quarter control
    unsigned left_bound;
    unsigned right_bound;

    explicit G4_Operand(Kind k, G4_Type ty = Type_UNDEF,
                        G4_VarBase *base = nullptr)
        : kind(k), type(ty), inst(nullptr), top_dcl(nullptr), base(base),
          rightBoundSet(false), byteOffset(0), accRegSel(ACC_UNDEFINED),
          left_bound(0), right_bound(0)
    {
        bitVec[0] = bitVec[1] = 0;
    }

    G4_Operand(Kind k, G4_VarBase *base)
        : kind(k), type(Type_UNDEF), inst(nullptr), top_dcl(nullptr), base(base),
          rightBoundSet(false), byteOffset(0), accRegSel(ACC_UNDEFINED),
          left_bound(0), right_bound(0)
    {
        bitVec[0] = bitVec[1] = 0;
    }

public:
    Kind getKind() const { return kind; }
    G4_Type getType() const { return type; }

    bool isImm() const { return kind == Kind::immediate; }
    bool isSrcRegRegion() const { return kind == Kind::srcRegRegion; }
    bool isDstRegRegion() const { return kind == Kind::dstRegRegion; }
    bool isRegRegion() const
    {
        return kind == srcRegRegion || kind == dstRegRegion;
    }
    bool isPredicate() const { return kind == predicate; }
    bool isCondMod() const { return kind == condMod; }
    bool isLabel() const { return kind == label; }
    bool isAddrExp() const { return kind == addrExp; }

    const G4_Declare *getTopDcl() const { return top_dcl; }
          G4_Declare* getTopDcl() { return top_dcl; }

    const G4_VarBase *getBase() const { return base; }
          G4_VarBase *getBase() { return base; }
    void setBase(G4_VarBase *b) { base = b; }
    G4_RegAccess getRegAccess() const;

    virtual bool isRelocImm() const { return false; }
    virtual void emit(std::ostream &output, bool symbolreg = false) = 0;
    void dump() const;

    bool isGreg() const;
    bool isAreg() const;
    bool isNullReg() const;
    bool isIpReg() const;
    bool isNReg() const;
    bool isAccReg() const;
    bool isFlag() const;
    bool isMaskReg() const;
    bool isMsReg() const;
    bool isSrReg() const;
    bool isCrReg() const;
    bool isDbgReg() const;
    bool isTmReg() const;
    bool isTDRReg() const;
    bool isA0() const;
    bool isAddress() const;

    const G4_AddrExp* asAddrExp() const
    {
#ifdef _DEBUG
        if (!isAddrExp())
        {
            return nullptr;
        }
#endif
        return reinterpret_cast<const G4_AddrExp*>(this);
    }
    G4_AddrExp* asAddrExp() {
        return const_cast<G4_AddrExp*>(((const G4_Operand *)this)->asAddrExp());
    }

    const G4_DstRegRegion* asDstRegRegion() const
    {
#ifdef _DEBUG
        if (!isDstRegRegion())
        {
            return nullptr;
        }
#endif
        return reinterpret_cast<const G4_DstRegRegion*>(this);
    }
    G4_DstRegRegion* asDstRegRegion() {
        return const_cast<G4_DstRegRegion*>(((const G4_Operand *)this)->asDstRegRegion());
    }

    const G4_SrcRegRegion* asSrcRegRegion() const
    {
#ifdef _DEBUG
        if (!isSrcRegRegion())
        {
            return nullptr;
        }
#endif
        return reinterpret_cast<const G4_SrcRegRegion*>(this);
    }
    G4_SrcRegRegion* asSrcRegRegion() {
        return const_cast<G4_SrcRegRegion*>(((const G4_Operand *)this)->asSrcRegRegion());
    }

    const G4_Imm* asImm() const
    {
#ifdef _DEBUG
        if (!isImm())
        {
            return nullptr;
        }
#endif
        return reinterpret_cast<const G4_Imm*>(this);
    }
    G4_Imm* asImm() {
        return const_cast<G4_Imm*>(((const G4_Operand *)this)->asImm());
    }

    const G4_Predicate* asPredicate() const
    {
#ifdef _DEBUG
        if (!isPredicate())
        {
            return nullptr;
        }
#endif
        return reinterpret_cast<const G4_Predicate*>(this);
    }
    G4_Predicate* asPredicate() {
        return const_cast<G4_Predicate*>(((const G4_Operand *)this)->asPredicate());
    }

    const G4_CondMod*    asCondMod() const {
#ifdef _DEBUG
        if (!isCondMod())
        {
            return nullptr;
        }
#endif
        return reinterpret_cast<const G4_CondMod*>(this);
    }
    G4_CondMod* asCondMod()
    {
        return const_cast<G4_CondMod*>(((const G4_Operand *)this)->asCondMod());
    }

    const G4_Label *asLabel() const
    {
#ifdef _DEBUG
        if (!isLabel())
        {
            return nullptr;
        }
#endif
        return reinterpret_cast<const G4_Label*>(this);
    }
    G4_Label* asLabel()
    {
        return const_cast<G4_Label*>(((const G4_Operand *)this)->asLabel());
    }

    bool crossGRF()
    {
        return getRightBound() / G4_GRF_REG_NBYTES !=
               getLeftBound() / G4_GRF_REG_NBYTES;
    }

    unsigned getLeftBound()
    {
        // The default left bound does not take emask into account for flags.
        // Compute the right bound in which updates the left bound accordingly.
        if (isRightBoundSet() == false && !isNullReg())
        {
            inst->computeRightBound(this);
        }
        return left_bound;
    }
    unsigned getRightBound()
    {
        if (isRightBoundSet() == false && !isNullReg())
        {
            inst->computeRightBound(this);
        }
        return right_bound;
    }
    bool isRightBoundSet() const { return rightBoundSet; }
    uint64_t getBitVecL()
    {
        if (isRightBoundSet() == false && !isNullReg())
        {
            // computeRightBound also computes bitVec
            inst->computeRightBound(this);
        }
        return bitVec[0];
    }
    uint64_t getBitVecH()
    {
        if (isRightBoundSet() == false && !isNullReg())
        {
            // computeRightBound also computes bitVec
            inst->computeRightBound(this);
        }
        if (getGRFSize() == 32)
        {
            assert(bitVec[1] == 0 && "upper bits should be 0");
        }
        return bitVec[1];
    }
    /*
        For operands that do use it, it is computed during left bound compuation.
    */
    unsigned getByteOffset() const { return byteOffset; }

    // ToDo: get rid of this setter
    void setBitVecL(uint64_t bvl )
    {
        bitVec[0] = bvl;
    }

    void setBitVecFromSize(uint32_t NBytes);

    void updateFootPrint(BitSet& footprint, bool isSet);

    virtual unsigned computeRightBound( uint8_t exec_size ) { return left_bound; }
    void setRightBound(unsigned val)
    {
        rightBoundSet = true;
        right_bound = val;
    }
    void unsetRightBound() { rightBoundSet = false; }
    void setLeftBound(unsigned val) { left_bound = val; }
    const G4_INST* getInst() const { return inst; }
          G4_INST* getInst()       { return inst; }
    void setInst(G4_INST* op) { inst = op; }
    void setAccRegSel( G4_AccRegSel value ) { accRegSel = value; }
    G4_AccRegSel getAccRegSel() const { return accRegSel; }
    bool isAccRegValid() const { return accRegSel != ACC_UNDEFINED;}

    unsigned getLinearizedStart();
    unsigned getLinearizedEnd();

    // compare if this operand is the same as the input w.r.t physical register in the end
    virtual G4_CmpRelation compareOperand(G4_Operand *opnd)
    {
        return Rel_disjoint;
    }

    static G4_Type GetNonVectorImmType(G4_Type type)
    {
        switch (type)
        {
            case Type_V:
                return Type_W;
            case Type_UV:
                return Type_UW;
            case Type_VF:
                return Type_F;
            default:
                return type;
        }
    }
};

class G4_VarBase
{
public:
    enum G4_VarKind {
        VK_regVar,  // register allocation candidate
        VK_phyGReg, // physical general register
        VK_phyAReg  // physical architecture register
    };

protected:
    G4_VarKind Kind;
    explicit G4_VarBase(G4_VarKind K) : Kind(K) {}

public:
    G4_VarKind getKind() const { return Kind; }

    bool isRegVar() const { return getKind() == VK_regVar; }
    bool isPhyReg() const { return !isRegVar(); }
    bool isPhyGreg() const { return getKind() == VK_phyGReg; }
    bool isPhyAreg() const { return getKind() == VK_phyAReg; }

    G4_RegVar *asRegVar() const
    {
        MUST_BE_TRUE(isRegVar(), ERROR_UNKNOWN);
        return (G4_RegVar *)this;
    }
    G4_Greg *asGreg() const
    {
        MUST_BE_TRUE(isPhyGreg(), ERROR_UNKNOWN);
        return (G4_Greg *)this;
    }
    G4_Areg *asAreg() const
    {
        MUST_BE_TRUE(isPhyAreg(), ERROR_UNKNOWN);
        return (G4_Areg *)this;
    }

    bool isAreg() const;
    bool isGreg() const;
    bool isNullReg() const;
    bool isIpReg() const;
    bool isFlag() const;
    bool isNReg() const;
    bool isAccReg() const;
    bool isMaskReg() const;
    bool isMsReg() const;
    bool isSrReg() const;
    bool isCrReg() const;
    bool isDbgReg() const;
    bool isTmReg() const;
    bool isTDRReg() const;
    bool isSpReg() const;
    bool isA0() const;
    bool isAddress() const;
    bool isRegAllocPartaker() const;

    bool noScoreBoard() const;

    G4_Areg* getAreg() const;

    virtual unsigned short ExRegNum(bool &valid)
    {
        valid = false;
        return UNDEFINED_SHORT;
    }
    virtual unsigned short ExIndRegNum(bool &valid)
    {
        valid = false;
        return UNDEFINED_SHORT;
    }
    virtual unsigned short ExSubRegNum(bool &valid)
    {
        valid = false;
        return UNDEFINED_SHORT;
    }

    virtual void emit(std::ostream &output, bool symbolreg = false) = 0;
};

//
// General Register File
//
class G4_Greg final : public G4_VarBase
{
    unsigned RegNum;
public:
    explicit G4_Greg(unsigned num) : G4_VarBase(VK_phyGReg), RegNum(num) {}
    void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
    G4_RegFileKind getRegFile() const { return G4_GRF; }

    unsigned getRegNum() const { return RegNum; }

    unsigned short ExRegNum(bool &valid) override
    {
        valid = true;
        return (unsigned short)getRegNum();
    }

    void emit(std::ostream &output, bool symbolreg = false) override;
};

//
// Architecture Register File
//
class G4_Areg final : public G4_VarBase
{
    G4_ArchRegKind ArchRegType;
public:
    explicit G4_Areg(G4_ArchRegKind k)
        : G4_VarBase(VK_phyAReg), ArchRegType(k) {}
    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    G4_ArchRegKind getArchRegType() const { return ArchRegType; }

    void emit(std::ostream& output, bool symbolreg=false) override;

    bool isNullReg() const { return getArchRegType() == AREG_NULL; }
    bool isFlag() const
    {
        switch (getArchRegType())
        {
            case AREG_F0:
            case AREG_F1:
                return true;
            default:
                return false;
        }
    }
    bool isIpReg()  const { return getArchRegType() == AREG_IP;      }
    bool isA0()     const { return getArchRegType() == AREG_A0;      }
    bool isNReg()   const { return getArchRegType() == AREG_N0      ||
                               getArchRegType() == AREG_N1;      }
    bool isAcc0Reg() const { return getArchRegType() == AREG_ACC0;    }
    bool isAccReg()  const { return getArchRegType() == AREG_ACC0    ||
                               getArchRegType() == AREG_ACC1;    }
    bool isMaskReg() const { return getArchRegType() == AREG_MASK0;   }
    bool isMsReg()   const { return getArchRegType() == AREG_MS0;     }
    bool isDbgReg()  const { return getArchRegType() == AREG_DBG;     }
    bool isSrReg()   const { return getArchRegType() == AREG_SR0;     }
    bool isCrReg()   const { return getArchRegType() == AREG_CR0;     }
    bool isTmReg()   const { return getArchRegType() == AREG_TM0;     }
    bool isTDRReg()  const { return getArchRegType() == AREG_TDR0;    }
    bool isSpReg()   const { return getArchRegType() == AREG_SP;      }

    unsigned short ExRegNum(bool &valid) override
    {
        unsigned short rNum = UNDEFINED_SHORT;
        valid = true;

        if (isFlag())
        {
            return getFlagNum();
        }

        switch (getArchRegType()) {
        case AREG_NULL:
        case AREG_A0:
        case AREG_ACC0:
        case AREG_MASK0:
        case AREG_MS0:
        case AREG_DBG:
        case AREG_SR0:
        case AREG_CR0:
        case AREG_TM0:
        case AREG_N0:
        case AREG_IP:
        case AREG_TDR0:
        case AREG_SP:
            rNum = 0;
            break;
        case AREG_ACC1:
        case AREG_N1:
            rNum = 1;
            break;
        default:
            valid = false;
        }
        return rNum;
    }

    unsigned short ExIndRegNum(bool &valid) override
    {
        unsigned short rIndNum = UNDEFINED_SHORT;
        valid = false;
        if (getArchRegType() == AREG_A0)
        {
            rIndNum = 0;
            valid = true;
        }
        return rIndNum;
    }

    int getFlagNum() const
    {
        switch (getArchRegType())
        {
        case AREG_F0:
            return 0;
        case AREG_F1:
            return 1;
        default:
            assert(false && "should only be called on flag ARF");
            return -1;
        }
    }
};

class G4_Imm : public G4_Operand
{
    // Requirement for the immediate value 'imm'
    //   Given a value V of type T, and let <V-as-uint> be its bit pattern as
    //   unsigned integer type whose size == sizeof(T). Let 'imm' be the
    //   immediate for V, the following must hold:
    //     (uint64_t)(<V-as-uint>) == (uint64_t)imm.num
    //     i.e.  int16_t v ---> (uint64_t)(*(uint16_t*)&v) == (uint64_t)imm.num
    //           float f   ---> (uint64_t)(*(uint32_t*)&f) == (uint64_t)imm.num
    union {
        int64_t  num;
        uint32_t num32;
        double   fp;
        float    fp32;
    } imm;

public:
    G4_Imm(int64_t i, G4_Type ty)
        : G4_Operand(G4_Operand::immediate, ty)
    {
        imm.num = i;
    }

    G4_Imm(double fp, G4_Type ty)
        : G4_Operand(G4_Operand::immediate, ty)
    {
        imm.fp = fp;
    }

    G4_Imm(float fp)
        : G4_Operand(G4_Operand::immediate, Type_F)
    {
        imm.num = 0;  // make sure to clear all the bits
        imm.fp32 = fp;
    }

    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}
    int64_t getImm() const {return imm.num;}  // Get bits of imm AS integer.
    int64_t getInt() const
    {
        MUST_BE_TRUE(!IS_TYPE_F32_F64(type), ERROR_UNKNOWN);
        return imm.num;
    }
    float getFloat() const
    {
        // if fp32 is sNAN, it will return qNAN. Be careful!
        MUST_BE_TRUE(IS_FTYPE(type), ERROR_UNKNOWN);
        return imm.fp32;
    }
    double getDouble() const
    {
        MUST_BE_TRUE(IS_DFTYPE(type), ERROR_UNKNOWN);
        return imm.fp;
    }
    bool isZero() const;
    // True if this is a signed integer and its sign bit(s) are 0.
    bool isSignBitZero() const;
    void emit(std::ostream& output, bool symbolreg=false) override;
    void emitAutoFmt(std::ostream& output);

    bool isEqualTo(G4_Imm& imm1) const;
    bool isEqualTo(G4_Imm* imm1) const { return isEqualTo(*imm1); }

    G4_CmpRelation compareOperand(G4_Operand *opnd) override;
    G4_RegFileKind getRegFile() const { return G4_UndefinedRF; }

    static bool isInTypeRange(int64_t imm, G4_Type ty);

    static int64_t typecastVals(int64_t value, G4_Type type);
};

class G4_Reloc_Imm : public G4_Imm
{

public:
    void *operator new(size_t sz, Mem_Manager& m){ return m.alloc(sz); }
    bool isRelocImm() const override { return true; }

    G4_Reloc_Imm(G4_Type ty) : G4_Imm((int64_t)0x6e10ca2e, ty)
    {
    }
};

//
// for label and jmp inst
//
class G4_Label: public G4_Operand
{
    friend class OperandHashTable; // labels are hashed, and only OperandHashTable may create a label

    const char* label;
    bool funcLabel;
    bool start_loop_label;
    bool isFC;

    G4_Label(const char* l) : G4_Operand(G4_Operand::label), label(l)
    {
        funcLabel = false;
        start_loop_label = false;
        isFC = false;
    }

public:

    const char* getLabel() const {return label;}
    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}
    void emit(std::ostream& output, bool symbolreg=false) override;
    void setFuncLabel( bool val ) { funcLabel = val; }
    bool isFuncLabel( ) const { return funcLabel; }
    void setStartLoopLabel() { start_loop_label = true; }
    bool isStartLoopLabel() const { return start_loop_label; }
    bool isFCLabel() const { return isFC; }
    void setFCLabel(bool fcLabel) { isFC = fcLabel; }
};
}
//
// Since the sub regs of address reg a0 can be allocated individually,
// we use subRegOff to indicate the sub registers
//
struct AssignedReg
{
    vISA::G4_VarBase* phyReg;
    unsigned    subRegOff;

    AssignedReg (): phyReg (NULL), subRegOff (0) {}
};

namespace vISA
{
    class G4_RegVar : public G4_VarBase
    {
        friend class G4_Declare;

    public:
        enum RegVarType
        {
            Default = 0,
            GRFSpillTmp = 1,
            AddrSpillLoc = 2,
            Transient = 3,
            Coalesced = 4,
        };

    private:
        // G4_RegVar now has an enum that holds its type. Each subclass of G4_RegVar
        // will initialize the type according to its specific class. For eg,
        // Spill/Fill transient ranges will set this type to RegVarType::Transient.
        unsigned    id;        // id for register allocation
        RegVarType type;
        G4_Declare* decl;    // corresponding declare
        AssignedReg reg;    // assigned physical register; set after reg alloc
        unsigned    disp;   // displacement offset in spill memory
        G4_SubReg_Align subAlign;    // To support sub register alignment
        bool evenAlignment = false; // Align this regVar to even GRFs regardless of its size

    public:

        // To support sub register alignment
        G4_RegVar(G4_Declare* d, RegVarType t) :
            G4_VarBase(VK_regVar), id(UNDEFINED_VAL), type(t), decl(d),
            disp(UINT_MAX), subAlign(Any)
        {
        }
        void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}
        unsigned    getId() const { return id; }
        void        setId(unsigned i) { id = i; }
        const char*        getName() const { return decl->getName(); }
        const G4_Declare* getDeclare() const { return decl; }
              G4_Declare* getDeclare()       { return decl; }
        bool        isPhyRegAssigned() const { return reg.phyReg != NULL; }
        bool        isFlag()    const { return decl->getRegFile() == G4_FLAG; }
        bool        isAreg()    const { return (reg.phyReg != NULL) && (reg.phyReg->isAreg()); }
        bool        isA0()      const { return (reg.phyReg != NULL) && (reg.phyReg->isA0()); }
        bool        isCrReg()   const { return (reg.phyReg != NULL) && (reg.phyReg->isCrReg()); }
        bool        isDbgReg()  const { return (reg.phyReg != NULL) && (reg.phyReg->isDbgReg()); }
        bool        isGreg()    const { return (reg.phyReg != NULL) && (reg.phyReg->isGreg()); }
        bool        isNReg()    const { return (reg.phyReg != NULL) && (reg.phyReg->isNReg()); }
        bool        isNullReg() const { return (reg.phyReg != NULL) && (reg.phyReg->isNullReg()); }
        bool        isSrReg()   const { return (reg.phyReg != NULL) && (reg.phyReg->isSrReg()); }
        bool        isTDRReg()  const { return (reg.phyReg != NULL) && (reg.phyReg->isTDRReg()); }
        bool        isTmReg()   const { return (reg.phyReg != NULL) && (reg.phyReg->isTmReg()); }
        bool        isAccReg()  const { return (reg.phyReg != NULL) && (reg.phyReg->isAccReg()); }
        bool        isIpReg()   const { return (reg.phyReg != NULL) && (reg.phyReg->isIpReg()); }
        bool        isMaskReg() const { return (reg.phyReg != NULL) && (reg.phyReg->isMaskReg()); }
        bool        isMsReg()   const { return (reg.phyReg != NULL) && (reg.phyReg->isMsReg()); }
        bool        isSpReg()   const { return (reg.phyReg != NULL) && (reg.phyReg->isSpReg()); }

        bool        isRegAllocPartaker() const { return id != UNDEFINED_VAL; }
        bool        isAddress()  const { return decl->getRegFile() == G4_ADDRESS; }
        const G4_VarBase* getPhyReg() const { return reg.phyReg; }
              G4_VarBase* getPhyReg()       { return reg.phyReg; }
        unsigned    getByteAddr() const;
        unsigned    getPhyRegOff() const { return reg.subRegOff; }
        void        setPhyReg(G4_VarBase* pr, unsigned off)
        {
            MUST_BE_TRUE(pr == NULL || pr->isPhyReg(), ERROR_UNKNOWN);
            reg.phyReg = pr;
            reg.subRegOff = off;
        }
        void        resetPhyReg() { reg.phyReg = NULL; reg.subRegOff = 0; }
        bool        isSpilled() const { return decl->isSpilled(); }
        void        setDisp(unsigned offset) { disp = offset; }
        unsigned    getDisp() const { return disp; }
        bool        isAliased() const { return decl->getAliasDeclare() != NULL; }
        unsigned getLocId() const;

        bool isRegVarTransient() const { return type == RegVarType::Transient; }
        bool isRegVarSpill() const;
        bool isRegVarFill()  const;

        bool isRegVarTmp()          const { return type == RegVarType::GRFSpillTmp; }
        bool isRegVarAddrSpillLoc() const { return type == RegVarType::AddrSpillLoc; }

        bool isRegVarCoalesced() const { return type == RegVarType::Coalesced; }

        G4_RegVar * getBaseRegVar();
        G4_RegVar * getAbsBaseRegVar();

        G4_RegVar * getNonTransientBaseRegVar();

        void emit(std::ostream& output, bool symbolreg = false) override;

        unsigned short ExRegNum(bool &valid) override { return reg.phyReg->ExRegNum(valid); }
        unsigned short ExSubRegNum(bool &valid) override { valid = true; return (unsigned short)reg.subRegOff; }
        unsigned short ExIndRegNum(bool &valid) override { return reg.phyReg->ExIndRegNum(valid); }

    protected:
        bool isEvenAlign() const { return evenAlignment; }
        void setEvenAlign() { evenAlignment = true; }
        G4_SubReg_Align getSubRegAlignment()
        {
            return subAlign;
        }

        void setSubRegAlignment(G4_SubReg_Align subAlg);
    };

    class G4_RegVarTransient : public G4_RegVar
    {
    public:
        enum TransientType
        {
            Spill = 0,
            Fill = 1,
        };

    private:
        G4_RegVar*  baseRegVar;
        G4_Operand* repRegion;
        unsigned    execSize;
        TransientType type;

    public:
        G4_RegVarTransient(G4_Declare* d, G4_RegVar* base, G4_Operand* reprRegion,
            unsigned eSize, TransientType t) :
            G4_RegVar(d, Transient), baseRegVar(base), repRegion(reprRegion),
            execSize(eSize), type(t)
        {
        }

        void *operator new(size_t sz, Mem_Manager& m){ return m.alloc(sz); }

        G4_RegVar * getBaseRegVar() { return baseRegVar; }

        G4_Operand * getRepRegion() const { return repRegion; }
        G4_RegVar * getAbsBaseRegVar();
        G4_RegVar * getNonTransientBaseRegVar();
        unsigned getExecSize() const { return execSize; }

        bool isRegVarSpill() const { return type == TransientType::Spill; }
        bool isRegVarFill() const { return type == TransientType::Fill; }
        G4_DstRegRegion* getDstRepRegion() const { return repRegion->asDstRegRegion(); }
        G4_SrcRegRegion* getSrcRepRegion() const { return repRegion->asSrcRegRegion(); }

    };

    class G4_RegVarTmp : public G4_RegVar
    {
        G4_RegVar * baseRegVar;

    public:
        G4_RegVarTmp(G4_Declare * d, G4_RegVar * base) : G4_RegVar(d, RegVarType::GRFSpillTmp), baseRegVar(base)
        {
            assert(base->isRegVarTransient() == false);
            assert(base == base->getBaseRegVar());
        }
        void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

        G4_RegVar * getBaseRegVar() { return baseRegVar; }
        G4_RegVar * getAbsBaseRegVar();
        G4_RegVar * getNonTransientBaseRegVar()
        {
            return baseRegVar;
        }
    };

    class G4_RegVarAddrSpillLoc : public G4_RegVar
    {
        unsigned         loc_id;
    public:
        G4_RegVarAddrSpillLoc(G4_Declare * d, int& loc_count) : G4_RegVar(d, RegVarType::AddrSpillLoc)
        {
            if (d->getAliasDeclare() != NULL)
            {
                unsigned elemSize = d->getRegVar()->getDeclare()->getElemSize();
                loc_id = d->getRegVar()->getLocId() + d->getAliasOffset() / elemSize;
            }
            else {
                loc_id = (++loc_count) * getNumAddrRegisters();
            }
        }
        void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}
        unsigned getLocId() const { return loc_id; }
    };

    class G4_RegVarCoalesced : public G4_RegVar
    {
        // If spill, set f to false
        bool f;
    public:
        G4_RegVarCoalesced(G4_Declare* dcl, bool fill) : G4_RegVar(dcl, RegVarType::Coalesced)
        {
            f = fill;
        }

        void *operator new(size_t sz, Mem_Manager& m){ return m.alloc(sz); }
        bool isSpill() const { return !f; }
        bool isFill() const { return f; }
    };

    class G4_SrcRegRegion final : public G4_Operand
    {
        friend class IR_Builder;

        const static int max_swizzle = 5;
        char swizzle[max_swizzle];     // this should only be set in binary encoding

        G4_SrcModifier mod;
        const G4_RegAccess acc;
        const RegionDesc *desc;
        const short    regOff;        // base+regOff is the starting register of the region
        const short    subRegOff;    // sub reg offset related to the regVar in "base"
        short          immAddrOff;    // imm addr offset

        G4_SrcRegRegion(G4_SrcModifier m,
            G4_RegAccess   a,
            G4_VarBase*    b,
            short roff,
            short sroff,
            const RegionDesc* rd,
            G4_Type        ty,
            G4_AccRegSel regSel = ACC_UNDEFINED) :
            G4_Operand(G4_Operand::srcRegRegion, ty, b), mod(m), acc(a), desc(rd),
            regOff(roff), subRegOff(sroff)
        {
            immAddrOff = 0;
            swizzle[0] = '\0';
            accRegSel = regSel;

            computeLeftBound();
            right_bound = 0;
        }

        void setSrcBitVec(uint8_t exec_size);

    public:
        G4_SrcRegRegion(G4_SrcRegRegion& rgn);
        void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

        bool operator==(const G4_SrcRegRegion &other)
        {
                if (base != other.base || regOff != other.regOff || subRegOff != other.subRegOff ||
                    desc->vertStride != other.desc->vertStride ||
                    desc->horzStride != other.desc->horzStride ||
                    desc->width != other.desc->width ||
                    mod != other.mod || acc != other.acc || immAddrOff != other.immAddrOff)
                    return false;
                else
                    return true;
        }

        void computeLeftBound();
        short getRegOff() const { return regOff; }
        short getSubRegOff() const { return subRegOff; }

        const char*       getSwizzle() const { return swizzle; }
        G4_SrcModifier    getModifier() const  { return mod; }
        bool              hasModifier() const  { return mod != Mod_src_undef; }
        const RegionDesc* getRegion() const  { return desc; }
        G4_RegAccess      getRegAccess() const { return acc; }
        short             getAddrImm()  const { return immAddrOff; }
        unsigned short    getElemSize() const { return (unsigned short)G4_Type_Table[type].byteSize; }

        void setImmAddrOff(short off) { immAddrOff = off; }
        void setModifier(G4_SrcModifier m) { mod = m; }
        void setSwizzle(const char* sw);

        bool sameSrcRegRegion(G4_SrcRegRegion& rgn);
        bool obeySymbolRegRule() const;
        void emit(std::ostream& output, bool symbolreg = false) override;
        void emitRegVarOff(std::ostream& output, bool symbolreg = false);

        bool isAreg()    const { return base->isAreg(); }
        bool isNullReg() const { return base->isNullReg(); }
        bool isIpReg()   const { return base->isIpReg();}
        bool isFlag()    const {return base->isFlag();}
        bool isNReg()    const {return base->isNReg();}
        bool isAccReg()  const {return base->isAccReg();}
        bool isMaskReg() const {return base->isMaskReg();}
        bool isMsReg()   const {return base->isMsReg();}
        bool isSrReg()   const {return base->isSrReg();}
        bool isCrReg()   const {return base->isCrReg();}
        bool isDbgReg()  const { return base->isDbgReg(); }
        bool isTmReg()   const { return base->isTmReg(); }
        bool isTDRReg()  const {return base->isTDRReg();}
        bool isA0()      const {return base->isA0();}
        bool isGreg()    const { return base->isGreg(); }
        bool isWithSwizzle() const {return (swizzle[0] != '\0');}
        bool isScalar() const;
        bool isAddress() const {return base->isAddress();}

        unsigned short             ExRegNum(bool&) const;
        unsigned short             ExSubRegNum(bool&);
        unsigned short             ExIndRegNum(bool&);
        unsigned short             ExIndSubRegNum(bool&);
        short                      ExIndImmVal(void);
        bool                       ExNegMod(bool&);

        void                       computePReg();

        bool isIndirect() const { return acc != Direct; }

        unsigned computeRightBound(uint8_t exec_size) override;
        G4_CmpRelation compareOperand(G4_Operand *opnd) override;

        void setType(G4_Type ty)
        {
            // FIXME: we should forbid setType() where ty has a different size than old type
            bool recomputeLeftBound = false;

            if (G4_Type_Table[type].byteSize != G4_Type_Table[ty].byteSize)
            {
                unsetRightBound();
                recomputeLeftBound = true;
            }

            type = ty;

            if (recomputeLeftBound)
            {
                computeLeftBound();
            }
        }

        void setRegion(const RegionDesc* rd, bool isInvariant = false)
        {
            if (!isInvariant && !desc->isEqual(rd))
            {
                unsetRightBound();
                desc = rd;
                computeLeftBound();
            }
            else
            {
                desc = rd;
            }
        }

        bool isNativeType() const;
        bool isNativePackedRowRegion() const;
        bool isNativePackedRegion() const;
        bool evenlySplitCrossGRF(uint8_t execSize, bool &sameSubRegOff, bool &vertCrossGRF, bool &contRegion, uint8_t &eleInFirstGRF);
        bool evenlySplitCrossGRF(uint8_t execSize);
        bool coverTwoGRF();
        bool checkGRFAlign();
        bool hasFixedSubregOffset(uint32_t& offset);
        bool isNativePackedSrcRegion();
        uint8_t getMaxExecSize(int pos, uint8_t maxExSize, bool allowCrossGRF, uint16_t &vs, uint16_t &wd, bool &twoGRFsrc);

        bool isSpilled() const
        {
            if (getBase() && getBase()->isRegVar())
            {
                return getBase()->asRegVar()->isSpilled();
            }

            return false;
        }

        // return the byte offset from the region start for the element at "pos"
        int getByteOffset(int pos) const
        {
            int rowIdx = pos / desc->width;
            int colIdx = pos % desc->width;
            return rowIdx * desc->vertStride * getElemSize() + colIdx * desc->horzStride * getElemSize();
        }

        void rewriteContiguousRegion(IR_Builder& builder, uint16_t opNum);

    };
}
enum ChannelEnable {
    NoChannelEnable = 0,
    ChannelEnable_X = 1,
    ChannelEnable_Y = 2,
    ChannelEnable_XY = 3,
    ChannelEnable_Z = 4,
    ChannelEnable_W = 8,
    ChannelEnable_ZW = 0xC,
    ChannelEnable_XYZW = 0xF
};

namespace vISA
{

class G4_DstRegRegion final : public G4_Operand
{
    friend class IR_Builder;
    ChannelEnable  writeMask;   // this should only be set in binary encoding

    G4_RegAccess   acc;         // direct, indirect GenReg or indirect MsgReg
    short          regOff;        // base+regOff is the starting register of the region
    short          subRegOff;    // sub reg offset related to the regVar in "base"
    short          immAddrOff;    // imm addr offset for indirect dst
    unsigned short horzStride;    // <DstRegion> has only horzStride

    G4_DstRegRegion(G4_RegAccess a,
        G4_VarBase* b,
        short roff,
        short sroff,
        unsigned short hstride,
        G4_Type ty,
        G4_AccRegSel regSel = ACC_UNDEFINED) :
        G4_Operand(G4_Operand::dstRegRegion, ty, b), acc(a), horzStride(hstride)
    {
        immAddrOff = 0;
        writeMask = NoChannelEnable;
        accRegSel = regSel;

        regOff = (roff == ((short)UNDEFINED_SHORT)) ? 0 : roff;
        subRegOff = sroff;

        computeLeftBound();
        right_bound = 0;
    }

    // DstRegRegion should only be constructed through IR_Builder
    void *operator new(size_t sz, Mem_Manager& m){ return m.alloc(sz); }

public:
    G4_DstRegRegion(G4_DstRegRegion& rgn);
    G4_DstRegRegion(G4_DstRegRegion& rgn, G4_VarBase* new_base);

    void computeLeftBound();

    G4_RegAccess   getRegAccess() const { return acc; }
    short          getRegOff() const { return regOff; }
    short          getSubRegOff() const { return subRegOff; }
    void  setSubRegOff(short off)
    {
        if (subRegOff != off)
        {
            subRegOff = off;
            computeLeftBound();
            unsetRightBound();
        }
    }

    bool isCrossGRFDst()
    {
        if (isNullReg())
        {
            return inst != NULL &&
                inst->getExecSize() * G4_Type_Table[type].byteSize * horzStride > G4_GRF_REG_NBYTES;
        }
        if (isRightBoundSet() == false)
        {
            // computeRightBound populates crossGRFDst field
            getInst()->computeRightBound(this);
        }

        return (left_bound / GENX_GRF_REG_SIZ) != right_bound / GENX_GRF_REG_SIZ;
    }
    unsigned short getHorzStride() const { return horzStride; }
    ChannelEnable  getWriteMask() const { return writeMask; }
    void           setWriteMask(ChannelEnable channels);
    short          getAddrImm() const { return immAddrOff; }
    unsigned short getElemSize() const { return (unsigned short)(G4_Type_Table[type].byteSize); }
    unsigned short getExecTypeSize() const { return horzStride * getElemSize(); }

    void setImmAddrOff(short off) { immAddrOff = off; }
    bool obeySymbolRegRule() const;
    void emit(std::ostream& output, bool symbolreg = false) override;
    void emitRegVarOff(std::ostream& output, bool symbolreg = false);

    bool isAreg()    const { return base->isAreg(); }
    bool isNullReg() const { return base->isNullReg(); }
    bool isIpReg()   const { return base->isIpReg(); }
    bool isFlag()    const { return base->isFlag(); }
    bool isNReg()    const { return base->isNReg(); }
    bool isAccReg()  const { return base->isAccReg(); }
    bool isMaskReg() const { return base->isMaskReg(); }
    bool isMsReg()   const { return base->isMsReg(); }
    bool isSrReg()   const { return base->isSrReg(); }
    bool isCrReg()   const { return base->isCrReg(); }
    bool isDbgReg()  const { return base->isDbgReg(); }
    bool isTmReg()   const { return base->isTmReg(); }
    bool isTDRReg()  const { return base->isTDRReg(); }
    bool isA0()      const { return base->isA0(); }
    bool isGreg()    const { return base->isGreg(); }
    bool isAddress() const { return base->isAddress(); }

    unsigned short             ExRegNum(bool&);
    unsigned short             ExSubRegNum(bool&);
    unsigned short             ExIndRegNum(bool&);
    unsigned short             ExIndSubRegNum(bool&);
    short                      ExIndImmVal(void);
    void                       computePReg();

    bool isIndirect() const { return acc != Direct; }

    void setType(G4_Type ty)
    {
        bool recomputeLeftBound = false;

        if (G4_Type_Table[type].byteSize != G4_Type_Table[ty].byteSize)
        {
            unsetRightBound();
            recomputeLeftBound = true;
        }

        type = ty;

        if (recomputeLeftBound)
        {
            computeLeftBound();

            if (getInst())
            {
                getInst()->computeLeftBoundForImplAcc(getInst()->getImplAccDst());
                getInst()->computeLeftBoundForImplAcc(getInst()->getImplAccSrc());
            }
        }
    }

    void setHorzStride(unsigned short hs)
    {
        if (horzStride != hs)
        {
            unsetRightBound();
        }

        horzStride = hs;
    }
    void setDstBitVec(uint8_t exec_size);
    unsigned computeRightBound(uint8_t exec_size) override;
    G4_CmpRelation compareOperand(G4_Operand *opnd) override;
    bool isNativeType() const;
    bool isNativePackedRowRegion() const;
    bool isNativePackedRegion() const;
    bool coverGRF(uint16_t numGRF, uint8_t execSize);
    bool goodOneGRFDst(uint8_t execSize);
    bool goodtwoGRFDst(uint8_t execSize);
    bool evenlySplitCrossGRF(uint8_t execSize);
    bool checkGRFAlign() const;
    bool hasFixedSubregOffset(uint32_t& offset);
    uint8_t getMaxExecSize(int pos, uint8_t maxExSize, bool twoGRFsrc);
    bool isSpilled() const
    {
        if (getBase() && getBase()->isRegVar())
        {
            return getBase()->asRegVar()->isSpilled();
        }

        return false;
    }

};
}

typedef enum
{
    PRED_DEFAULT,
    PRED_ANY2H,
    PRED_ANY4H,
    PRED_ANY8H,
    PRED_ANY16H,
    PRED_ANY32H,
    PRED_ALL2H,
    PRED_ALL4H,
    PRED_ALL8H,
    PRED_ALL16H,
    PRED_ALL32H,
    PRED_ANYV,
    PRED_ALLV,
    PRED_ANY_WHOLE,   // any of the flag-bits
    PRED_ALL_WHOLE    // all of the flag-bits
} G4_Predicate_Control;

typedef enum
{
    PRED_ALIGN16_DEFAULT = 1,
    PRED_ALIGN16_X = 2,
    PRED_ALIGN16_Y = 3,
    PRED_ALIGN16_Z = 4,
    PRED_ALIGN16_W = 5,
    PRED_ALIGN16_ANY4H = 6,
    PRED_ALIGN16_ALL4H = 7
} G4_Align16_Predicate_Control;

namespace vISA
{
//
// predicate control for inst
//
class G4_Predicate final : public G4_Operand
{
    friend class IR_Builder;

    G4_PredState   state;         // + or -
    unsigned short subRegOff;
    G4_Predicate_Control control;

    // this is only used at the very end by binary and asm emission, and
    // internally the align1 control above is always used instead even for align16 instructions.
    // currently this is always PRED_ALIGN16_DEFAULT except for simd1 inst,
    // for which it's PRED_ALIGN16_X
    G4_Align16_Predicate_Control align16Control;

    // Special predicate : it's equivalent to noMask and used for WA
    bool isPredicateSameAsNoMask;

    G4_Predicate(G4_PredState s, G4_VarBase *flag, unsigned short srOff,
                 G4_Predicate_Control ctrl)
        : G4_Operand(G4_Operand::predicate, flag), state(s), subRegOff(srOff),
          control(ctrl), align16Control(PRED_ALIGN16_DEFAULT),
          isPredicateSameAsNoMask(false)
    {
        top_dcl = getBase()->asRegVar()->getDeclare();
        MUST_BE_TRUE(flag->isFlag(), ERROR_INTERNAL_ARGUMENT);
        if (getBase()->asRegVar()->getPhyReg())
        {
            left_bound = srOff * 16;

            byteOffset = srOff * 2;

            auto flagNum = getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
            left_bound += flagNum * 32;
            byteOffset += flagNum * 4;
        }
        else
        {
            left_bound = 0;
            byteOffset = 0;
        }
    }

public:

    G4_Predicate(G4_Predicate& prd);

    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}
    unsigned short getSubRegOff() const { return subRegOff; }
    unsigned short getRegOff() const
    {
        MUST_BE_TRUE(getBase()->isAreg(), ERROR_INTERNAL_ARGUMENT);
        return getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
    }

    G4_PredState   getState() const { return state; }
    void   setState(G4_PredState s) { state = s; }
    G4_Predicate_Control    getControl() const { return control; }
    void setControl(G4_Predicate_Control PredCtrl) { control = PredCtrl; }
    bool samePredicate(const G4_Predicate& prd) const;
    void emit(std::ostream& output, bool symbolreg = false) override;

    void setAlign16PredicateControl(G4_Align16_Predicate_Control control) { align16Control = control; }
    G4_Align16_Predicate_Control getAlign16PredicateControl() const { return align16Control; }

    unsigned computeRightBound(uint8_t exec_size) override;
    G4_CmpRelation compareOperand(G4_Operand *opnd) override;
    void splitPred();
    void setSameAsNoMask(bool v) { isPredicateSameAsNoMask = v; };
    bool isSameAsNoMask() const { return isPredicateSameAsNoMask; }
    unsigned getPredCtrlGroupSize() const
    {
        switch (control)
        {
        case PRED_ANY2H:
        case PRED_ALL2H:
            return 2;
        case PRED_ANY4H:
        case PRED_ALL4H:
            return 4;
        case PRED_ANY8H:
        case PRED_ALL8H:
            return 8;
        case PRED_ANY16H:
        case PRED_ALL16H:
            return 16;
        case PRED_ANY32H:
        case PRED_ALL32H:
            return 32;
        default:
            return 1;
        }
    }
    static bool isAnyH(G4_Predicate_Control Ctrl)
    {
        switch (Ctrl)
        {
        default:
            break;
        case PRED_ANY2H:
        case PRED_ANY4H:
        case PRED_ANY8H:
        case PRED_ANY16H:
        case PRED_ANY32H:
            return true;
        }
        return false;
    }
    static bool isAllH(G4_Predicate_Control Ctrl)
    {
        switch (Ctrl)
        {
        default:
            break;
        case PRED_ALL2H:
        case PRED_ALL4H:
        case PRED_ALL8H:
        case PRED_ALL16H:
        case PRED_ALL32H:
            return true;
        }
        return false;
    }
};

//
// condition modifier for inst
//
class G4_CondMod final : public G4_Operand
{
    friend class IR_Builder;
    G4_CondModifier   mod;
    unsigned short subRegOff;

    G4_CondMod(G4_CondModifier m, G4_VarBase *flag, unsigned short off)
        : G4_Operand(G4_Operand::condMod, flag), mod(m), subRegOff(off)
    {
        if (flag != nullptr)
        {
            top_dcl = getBase()->asRegVar()->getDeclare();
            MUST_BE_TRUE(flag->isFlag(), ERROR_INTERNAL_ARGUMENT);
            if (getBase()->asRegVar()->getPhyReg())
            {
                left_bound = off * 16;
                byteOffset = off * 2;

                auto flagNum = getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
                left_bound += flagNum * 32;
                byteOffset += flagNum * 4;
            }
            else
            {
                left_bound = 0;
                byteOffset = 0;
            }
        }
    }

public:

    G4_CondMod(G4_CondMod &cMod);
    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}
    G4_CondModifier getMod() const { return mod; }
    unsigned short getRegOff() const
    {
        MUST_BE_TRUE(getBase()->isAreg(), ERROR_INTERNAL_ARGUMENT);
        MUST_BE_TRUE(getBase()->asRegVar()->getPhyReg(), "getRegOff is called for non-PhyReg");
        return getBase()->asRegVar()->getPhyReg()->asAreg()->getFlagNum();
    }
    unsigned short getSubRegOff() const { return subRegOff; }
    bool sameCondMod(const G4_CondMod& prd) const;
    void emit(std::ostream& output, bool symbolreg = false) override;

    // Get condition modifier when operands are reversed.
    static G4_CondModifier getReverseCondMod(G4_CondModifier mod)
    {
        switch (mod)
        {
        default:
            break;
        case Mod_g:
            return Mod_le;
        case Mod_ge:
            return Mod_l;
        case Mod_l:
            return Mod_ge;
        case Mod_le:
            return Mod_g;
        }

        return mod;
    }

    unsigned computeRightBound(uint8_t exec_size) override;
    G4_CmpRelation compareOperand(G4_Operand *opnd) override;
    void splitCondMod();
};

class G4_AddrExp final : public G4_Operand
{
    G4_RegVar* m_addressedReg;
    int m_offset;  //current implementation: byte offset

public:
    G4_AddrExp(G4_RegVar *reg, int offset, G4_Type ty)
      : G4_Operand(G4_Operand::addrExp, ty), m_addressedReg(reg),
        m_offset(offset) {}

    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    const G4_RegVar* getRegVar() const { return m_addressedReg; }
          G4_RegVar* getRegVar()       { return m_addressedReg; }
    int getOffset() const { return m_offset; }
    void setOffset(int tOffset) { m_offset = tOffset; }

    int eval();
    bool isRegAllocPartaker() const { return m_addressedReg->isRegAllocPartaker(); }

    void emit(std::ostream& output, bool symbolreg = false);
};

inline G4_RegAccess G4_Operand::getRegAccess() const
{
    if (isSrcRegRegion())
        return asSrcRegRegion()->getRegAccess();
    else if (isDstRegRegion())
        return asDstRegRegion()->getRegAccess();
    return Direct;
}

inline bool G4_Operand::isGreg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isGreg();
}
inline bool G4_Operand::isAreg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isAreg();
}
inline bool G4_Operand::isNullReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isNullReg();
}
inline bool G4_Operand::isIpReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isIpReg();
}
inline bool G4_Operand::isNReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isNReg();
}
inline bool G4_Operand::isAccReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isAccReg();
}
inline bool G4_Operand::isFlag() const
{
    if (isRegRegion() && const_cast<G4_VarBase *>(getBase())->isFlag())
        return true;
    return isPredicate() || isCondMod();
}
inline bool G4_Operand::isMaskReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isMaskReg();
}
inline bool G4_Operand::isMsReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isMsReg();
}
inline bool G4_Operand::isSrReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isSrReg();
}
inline bool G4_Operand::isCrReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isCrReg();
}
inline bool G4_Operand::isDbgReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isDbgReg();
}
inline bool G4_Operand::isTmReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isTmReg();
}
inline bool G4_Operand::isTDRReg() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isTDRReg();
}
inline bool G4_Operand::isA0() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isA0();
}
inline bool G4_Operand::isAddress() const
{
    return isRegRegion() && const_cast<G4_VarBase *>(getBase())->isAddress();
}

// Inlined members of G4_VarBase
inline bool G4_VarBase::isAreg() const
{
    if (isRegVar())
        return asRegVar()->isAreg();
    return isPhyAreg();
}
inline bool G4_VarBase::isGreg() const
{
    if (isRegVar())
        return asRegVar()->isGreg();
    return isPhyGreg();
}
inline bool G4_VarBase::isNullReg() const
{
    if (isRegVar())
        return asRegVar()->isNullReg();
    return isPhyAreg() && asAreg()->isNullReg();
}
inline bool G4_VarBase::isIpReg() const
{
    if (isRegVar())
        return asRegVar()->isIpReg();
    return isPhyAreg() && asAreg()->isIpReg();
}
inline bool G4_VarBase::isFlag() const
{
    if (isRegVar())
        return asRegVar()->isFlag();
    return isPhyAreg() && asAreg()->isFlag();
}
inline bool G4_VarBase::isNReg() const
{
    if (isRegVar())
        return asRegVar()->isNReg();
    return isPhyAreg() && asAreg()->isNReg();
}
inline bool G4_VarBase::isAccReg() const
{
    if (isRegVar())
        return asRegVar()->isAccReg();
    return isPhyAreg() && asAreg()->isAccReg();
}
inline bool G4_VarBase::isMaskReg() const
{
    if (isRegVar())
        return asRegVar()->isMaskReg();
    return isPhyAreg() && asAreg()->isMaskReg();
}
inline bool G4_VarBase::isMsReg() const
{
    if (isRegVar())
        return asRegVar()->isMsReg();
    return isPhyAreg() && asAreg()->isMsReg();
}
inline bool G4_VarBase::isSrReg() const
{
    if (isRegVar())
        return asRegVar()->isSrReg();
    return isPhyAreg() && asAreg()->isSrReg();
}
inline bool G4_VarBase::isCrReg() const
{
    if (isRegVar())
        return asRegVar()->isCrReg();
    return isPhyAreg() && asAreg()->isCrReg();
}
inline bool G4_VarBase::isDbgReg() const
{
    if (isRegVar())
        return asRegVar()->isDbgReg();
    return isPhyAreg() && asAreg()->isDbgReg();
}
inline bool G4_VarBase::isTmReg() const
{
    if (isRegVar())
        return asRegVar()->isTmReg();
    return isPhyAreg() && asAreg()->isTmReg();
}
inline bool G4_VarBase::isTDRReg() const
{
    if (isRegVar())
        return asRegVar()->isTDRReg();
    return isPhyAreg() && asAreg()->isTDRReg();
}
inline bool G4_VarBase::isA0() const
{
    if (isRegVar())
        return asRegVar()->isA0();
    return isPhyAreg() && asAreg()->isA0();
}
inline bool G4_VarBase::isAddress() const
{
    if (isRegVar())
        return asRegVar()->isAddress();
    return isPhyAreg() && asAreg()->isA0();
}
inline bool G4_VarBase::isSpReg() const
{
    if (isRegVar())
    {
        return asRegVar()->isSpReg();
    }
    return isPhyAreg() && asAreg()->isSpReg();
}

/// return the physical AReg associated with this VarBase objkect.
/// This is either the VarBase itself, or if this is a RegVar the phyAReg it is allocated to.
/// return null if VarBase is not a AReg or it's a RegVar that has not been assigned a AReg yet
inline G4_Areg* G4_VarBase::getAreg() const
{
    G4_Areg* areg = nullptr;
    if (isRegVar())
    {
        G4_VarBase* phyReg = asRegVar()->getPhyReg();
        if (phyReg && phyReg->isAreg())
        {
            areg = phyReg->asAreg();
        }
    }
    else if (isPhyAreg())
    {
        areg = asAreg();
    }
    return areg;
}

// CR/SR/SP/TM0/IP do not have scoreboard
inline bool G4_VarBase::noScoreBoard() const
{
    G4_Areg* areg = getAreg();

    if (areg != nullptr)
    {
        return areg->isCrReg() || areg->isSrReg() || areg->isSpReg() ||
            areg->isTmReg() || areg->isIpReg() || areg->isDbgReg();
    }
    else
    {
        return false;
    }
}


inline bool G4_VarBase::isRegAllocPartaker() const
{
    return isRegVar() && asRegVar()->isRegAllocPartaker();
}

// G4_RegVar methods
inline unsigned G4_RegVar::getLocId() const
{
    MUST_BE_TRUE(type == RegVarType::AddrSpillLoc, "Unexpected type in getLocId()");

    G4_RegVarAddrSpillLoc* addrSpillLoc =
    static_cast <G4_RegVarAddrSpillLoc*>(const_cast<G4_RegVar*>(this));
    return addrSpillLoc->getLocId();
}

inline  bool G4_RegVar::isRegVarSpill() const
{
    if (isRegVarTransient())
    {
        G4_RegVarTransient* transientVar =
            static_cast<G4_RegVarTransient*>(const_cast<G4_RegVar*>(this));
        return transientVar->isRegVarSpill();
    }
    return false;
}

inline bool G4_RegVar::isRegVarFill() const
{
    if (isRegVarTransient())
    {
        G4_RegVarTransient* transientVar =
            static_cast<G4_RegVarTransient*>(const_cast<G4_RegVar*>(this));
        return transientVar->isRegVarFill();
    }
    return false;
}

inline G4_RegVar* G4_RegVar::getBaseRegVar()
{
    if (type == RegVarType::Transient)
    {
        G4_RegVarTransient* transient = static_cast<G4_RegVarTransient*>(this);
        return transient->getBaseRegVar();
    }
    else if (type == RegVarType::GRFSpillTmp)
    {
        G4_RegVarTmp* tmp = static_cast<G4_RegVarTmp*>(this);
        return tmp->getBaseRegVar();
    }

    // For Default, AddrSpillLoc
    return this;
}

inline G4_RegVar* G4_RegVar::getAbsBaseRegVar()
{
    if (type == RegVarType::Transient || type == RegVarType::GRFSpillTmp)
    {
        G4_RegVar * base;
        for (base = getBaseRegVar(); base->getBaseRegVar() != base; base = base->getBaseRegVar());
        return base;
    }

    return this;
}

inline G4_RegVar* G4_RegVar::getNonTransientBaseRegVar()
{
    if (type == RegVarType::Transient)
    {
        G4_RegVarTransient* transient = static_cast<G4_RegVarTransient*>(this);
        return transient->getNonTransientBaseRegVar();
    }
    else if (type == RegVarType::GRFSpillTmp)
    {
        G4_RegVarTmp* tmp = static_cast<G4_RegVarTmp*>(this);
        return tmp->getNonTransientBaseRegVar();
    }

    return this;
}

//
// place for holding all physical register operands
//
class PhyRegPool
{
    unsigned maxGRFNum;
    G4_Greg** GRF_Table;
    G4_Areg* ARF_Table[AREG_LAST];
public:
    PhyRegPool(Mem_Manager&m, unsigned int maxRegisterNumber); // create all physical register operands
    void rebuildRegPool(Mem_Manager& m, unsigned int numRegisters);
    G4_Greg* getGreg(unsigned i)
    {
        MUST_BE_TRUE(i < maxGRFNum, "invalid GRF");
        return GRF_Table[i];
    }

    G4_Areg* getNullReg() { return ARF_Table[AREG_NULL]; }
    G4_Areg* getMask0Reg() { return ARF_Table[AREG_MASK0]; }
    G4_Areg* getAcc0Reg() { return ARF_Table[AREG_ACC0]; }
    G4_Areg* getAcc1Reg() { return ARF_Table[AREG_ACC1]; }
    G4_Areg* getDbgReg() { return ARF_Table[AREG_DBG]; }
    G4_Areg* getMs0Reg() { return ARF_Table[AREG_MS0]; }
    G4_Areg* getSr0Reg() { return ARF_Table[AREG_SR0]; }
    G4_Areg* getCr0Reg() { return ARF_Table[AREG_CR0]; }
    G4_Areg* getTm0Reg() { return ARF_Table[AREG_TM0]; }
    G4_Areg* getAddrReg() { return ARF_Table[AREG_A0]; }
    G4_Areg* getN0Reg() { return ARF_Table[AREG_N0]; }
    G4_Areg* getN1Reg() { return ARF_Table[AREG_N1]; }
    G4_Areg* getIpReg() { return ARF_Table[AREG_IP]; }
    G4_Areg* getF0Reg() { return ARF_Table[AREG_F0]; }
    G4_Areg* getF1Reg() { return ARF_Table[AREG_F1]; }
    G4_Areg* getTDRReg() { return ARF_Table[AREG_TDR0]; }
    G4_Areg* getSPReg() { return ARF_Table[AREG_SP]; }

    // map int to flag areg
    G4_Areg* getFlagAreg(int flagNum)
    {
        switch (flagNum)
        {
            case 0:
                return getF0Reg();
            case 1:
                return getF1Reg();
            default:
                assert(false && "unexpected flag register value");
                return nullptr;
        }
    }
};

inline int G4_INST::getNumSrc() const
{
    return isIntrinsic() ? asIntrinsicInst()->getNumSrc()
                         : G4_Inst_Table[op].n_srcs;
}

inline int G4_INST::getNumDst() const
{
    return isIntrinsic() ? asIntrinsicInst()->getNumDst()
        : G4_Inst_Table[op].n_dst;
}

inline bool G4_INST::isPseudoUse() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::Use;
}

inline bool G4_INST::isPseudoKill() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::PseudoKill;
}

inline bool G4_INST::isLifeTimeEnd() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::PseudoUse;
}

inline bool G4_INST::isSpillIntrinsic() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::Spill;
}

inline G4_SpillIntrinsic* G4_INST::asSpillIntrinsic() const
{
    MUST_BE_TRUE(isSpillIntrinsic(), "not a spill intrinsic");
    return const_cast<G4_SpillIntrinsic*>(reinterpret_cast<const G4_SpillIntrinsic*>(this));
}

inline bool G4_INST::isFillIntrinsic() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::Fill;
}

inline G4_FillIntrinsic* G4_INST::asFillIntrinsic() const
{
    MUST_BE_TRUE(isFillIntrinsic(), "not a fill intrinsic");
    return const_cast<G4_FillIntrinsic*>(reinterpret_cast<const G4_FillIntrinsic*>(this));
}

inline bool G4_INST::isSplitIntrinsic() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::Split;
}

inline bool G4_INST::isCallerSave() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::CallerSave;
}

inline bool G4_INST::isCallerRestore() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::CallerRestore;
}

inline bool G4_INST::isCalleeSave() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::CalleeSave;
}

inline bool G4_INST::isCalleeRestore() const
{
    return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::CalleeRestore;
}

inline bool G4_INST::isRelocationMov() const
{
    return isMov() && srcs[0]->isRelocImm();
}

inline const char* G4_INST::getLabelStr() const
{
    MUST_BE_TRUE(srcs[0] != NULL && srcs[0]->isLabel(), ERROR_UNKNOWN);
    return srcs[0]->asLabel()->getLabel();
}

inline bool G4_InstCF::isUniformGoto(unsigned KernelSimdSize) const
{
    assert(op == G4_goto);
    const G4_Predicate *pred = getPredicate();
    if (getExecSize() == 1 || pred == nullptr)
        return true;

    // This is uniform if group size equals to the kernel simd size.
    return pred->getPredCtrlGroupSize() == KernelSimdSize;
}

inline bool G4_InstCF::isIndirectJmp() const
{
    return op == G4_jmpi && !srcs[0]->isLabel();
}

inline const char* G4_InstCF::getJipLabelStr() const
{
    MUST_BE_TRUE(jip != NULL && jip->isLabel(), ERROR_UNKNOWN);
    return jip->asLabel()->getLabel();
}

inline const char* G4_InstCF::getUipLabelStr() const
{
    MUST_BE_TRUE(uip != NULL && uip->isLabel(), ERROR_UNKNOWN);
    return uip->asLabel()->getLabel();
}

inline bool G4_InstCF::isIndirectCall() const
{
    return op == G4_pseudo_fcall && !getSrc(0)->isLabel();
}

static void computeSpillFillOperandBound(G4_Operand* opnd, unsigned int LB, int numReg)
{
    if (numReg == 0)
    {
        return;
    }

    // read/write in units of GRF.
    unsigned RB = std::min(opnd->getTopDcl()->getByteSize(),
        LB + numReg * G4_GRF_REG_NBYTES) - 1;

    unsigned NBytes = RB - LB + 1;
    opnd->setBitVecFromSize(NBytes);
    opnd->setRightBound(RB);
}

class G4_SpillIntrinsic : public G4_InstIntrinsic
{
public:
    G4_SpillIntrinsic(
        const IR_Builder& builder,
        G4_Predicate* prd,
        Intrinsic intrinId,
        uint8_t size,
        G4_DstRegRegion* d,
        G4_Operand* s0,
        G4_Operand* s1,
        G4_Operand* s2,
        unsigned int opt) :
        G4_InstIntrinsic(builder, prd, intrinId, size, d, s0, s1, s2, opt)
    {

    }

    const static unsigned int InvalidOffset = 0xffffffff;

    bool isOffBP() const { return getFP() != nullptr; }

    uint32_t getNumRows() const { return numRows; }
    uint32_t getOffset() const { return offset; }
    G4_Declare* getFP() const { return fp; }
    G4_SrcRegRegion* getHeader() const { return getSrc(0)->asSrcRegRegion(); }

    void setNumRows(uint32_t r) { numRows = r; }
    void setOffset(uint32_t o) { offset = o; }
    void setFP(G4_Declare* f) { fp = f; }

    bool isOffsetValid() { return offset != InvalidOffset; }

    void computeRightBound(G4_Operand* opnd)
    {
        uint16_t numReg = 0;
        if (opnd == getSrc(1))
        {
            numReg = asSpillIntrinsic()->getNumRows();
        }
        else if (opnd->isSrcRegRegion() && opnd == getSrc(0))
        {
            numReg = 1;
        }
        computeSpillFillOperandBound(opnd, opnd->left_bound, numReg);
    }

private:
    G4_Declare* fp = nullptr;
    uint32_t numRows = 0;
    uint32_t offset = InvalidOffset;
};

class G4_FillIntrinsic : public G4_InstIntrinsic
{
public:
    G4_FillIntrinsic(
        const IR_Builder& builder,
        G4_Predicate* prd,
        Intrinsic intrinId,
        uint8_t size,
        G4_DstRegRegion* d,
        G4_Operand* s0,
        G4_Operand* s1,
        G4_Operand* s2,
        unsigned int opt) :
        G4_InstIntrinsic(builder, prd, intrinId, size, d, s0, s1, s2, opt)
    {

    }

    const static unsigned int InvalidOffset = 0xffffffff;

    bool isOffBP() const { return getFP() != nullptr; }

    uint32_t getNumRows() const { return numRows; }
    uint32_t getOffset() const { return offset; }
    G4_Declare* getFP() const { return fp; }
    G4_SrcRegRegion* getHeader() const { return getSrc(0)->asSrcRegRegion(); }

    void setNumRows(uint32_t r) { numRows = r; }
    void setOffset(uint32_t o) { offset = o; }
    void setFP(G4_Declare* f) { fp = f; }



    bool isOffsetValid() { return offset != InvalidOffset; }

    void computeRightBound(G4_Operand* opnd)
    {
        uint16_t numReg = 0;
        if (opnd == getDst())
        {
            numReg = asFillIntrinsic()->getNumRows();
        }
        else if (opnd->isSrcRegRegion() &&
            (opnd == getSrc(0) || opnd == getSrc(1)))
        {
            numReg = 1;
        }
        computeSpillFillOperandBound(opnd, opnd->left_bound, numReg);
    }

private:
    G4_Declare* fp = nullptr;
    uint32_t numRows = 0;
    uint32_t offset = InvalidOffset;
};

} // namespace vISA

#endif
