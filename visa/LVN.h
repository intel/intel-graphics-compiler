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
#ifndef _G4_LVN_H_
#define _G4_LVN_H_

#include "Optimizer.h"
#include <fstream>
#include <sstream>
#include "G4_Opcode.h"
#include "Timer.h"
#include "G4Verifier.h"
#include <map>

typedef uint64_t Value_Hash;
namespace vISA
{
class LVNItemInfo;
class Value
{
public:
    Value_Hash hash;
    G4_Operand* opnd;

    void initializeEmptyValue() { hash = 0; opnd = NULL; inst = NULL; }
    void copyValue(Value& src) { hash = src.hash; opnd = src.opnd; inst = src.inst; }
    bool isValueEmpty()
    {
        bool retval = false;
        if (opnd == NULL)
        {
            retval = true;
        }
        return retval;
    }
    bool isEqualValueHash(Value& val2)
    {
        bool retval = false;
        if (hash == val2.hash)
        {
            retval = true;
        }
        return retval;
    }

    void setInst(G4_INST* i) { inst = i; }
    G4_INST* getInst() { return inst; }

private:

    G4_INST* inst;
};
}
#define VALUELENMAX 50

namespace vISA
{
class LVNItemInfo
{
public:
    Value variable;
    Value value;
    G4_INST* inst;
    G4_Declare* dstTopDcl;
    G4_Declare* srcTopDcls[G4_MAX_SRCS];
    // active field below determines whether value pointed to
    // by this instance is live. When a redef is seen for dst of an
    // LVN candidate we set active to false so the value is no longer
    // available for propagation. This is required because for an
    // instruction such as mov with dst and 1 src, we insert instance
    // of this class in 2 buckets - dst dcl id, src0 dcl id. Doing so
    // helps to easily invalidate values due to redefs.
    bool active;
};
}

// LvnTable uses dcl id or immediate value as key. This key is mapped to
// all operands with dcl id that have appeared so far in current BB. Or
// in case of immediates the key maps to respective operands. Having
// a map allows faster lookups and lesser number of comparisons than a
// running list of all instructions seen so far.
typedef std::map<int64_t, std::list<vISA::LVNItemInfo*>> LvnTable;
typedef struct UseInfo
{
    vISA::G4_INST* first;
    Gen4_Operand_Number second;
} UseInfo;
typedef std::list<UseInfo> UseList;
typedef struct DefUseInfo
{
    vISA::G4_INST* first;
    UseList second;
} DefUseInfo;
typedef std::multimap<unsigned int, DefUseInfo> DefUseTable;

typedef struct ActiveDef
{
    vISA::G4_Declare* first;
    vISA::G4_DstRegRegion* second;
} ActiveDef;
typedef std::multimap<unsigned int, ActiveDef> ActiveDefMMap;

namespace vISA
{
class LVN
{
private:
    G4_BB* bb;
    FlowGraph& fg;
    LvnTable lvnTable;
    ActiveDefMMap activeDefs;
    vISA::Mem_Manager& mem;
    IR_Builder& builder;
    unsigned int numInstsRemoved;
    bool duTablePopulated;
    PointsToAnalysis& p2a;

    static const int MaxLVNDistance = 250;

    void populateDuTable(INST_LIST_ITER inst_it);
    void removeAddrTaken(G4_AddrExp* opnd);
    void addUse(G4_DstRegRegion* dst, G4_INST* use, unsigned int srcIndex);
    void addValueToTable(G4_INST* inst, Value& oldValue);
    LVNItemInfo* isValueInTable(Value& value);
    bool isSameValue(Value& val1, Value& val2);
    void computeValue(G4_INST* inst, bool negate, bool& canNegate, bool& isGlobal, int64_t& tmpPosImm, bool posValValid, Value& valueStr);
    bool addValue(G4_INST* inst);
    void getValue(G4_DstRegRegion* dst, G4_INST* inst, Value& value);
    void getValue(G4_SrcRegRegion* src, G4_INST* inst, Value& value);
    void getValue(int64_t imm, G4_Operand* opnd, Value& value);
    const char* getModifierStr(G4_SrcModifier srcMod);
    int64_t getNegativeRepresentation(int64_t imm, G4_Type type);
    bool sameGRFRef(G4_Declare* dcl1, G4_Declare* dcl2);
    void removeVirtualVarRedefs(G4_DstRegRegion* dst);
    void removePhysicalVarRedefs(G4_DstRegRegion* dst);
    void removeRedefs(G4_INST* inst);
    void replaceAllUses(G4_INST* defInst, bool negate, UseList& uses, G4_INST* lvnInst, bool keepRegion);
    void transferAlign(G4_Declare* toDcl, G4_Declare* fromDcl);
    bool canReplaceUses(INST_LIST_ITER inst_it, UseList& uses, G4_INST* lvnInst, bool negMatch, bool noPartialUse);
    bool getAllUses(G4_INST* def, UseList& uses);
    bool getDstData(int64_t srcImm, G4_Type srcType, int64_t& dstImm, G4_Type dstType, bool& canNegate);
    bool valuesMatch(Value& val1, Value& val2);
    void removeAliases(G4_INST* inst);
    bool checkIfInPointsTo(G4_RegVar* addr, G4_RegVar* var);
    bool isRedundantMovToSelf(LVNItemInfo* lvnItem, G4_INST* inst);
    template<class T, class K>
    bool opndsMatch(T*, K*);

public:
    LVN(FlowGraph& flowGraph, G4_BB* curBB, vISA::Mem_Manager& mmgr, IR_Builder& irBuilder, PointsToAnalysis& p) :
        fg(flowGraph), mem(mmgr), builder(irBuilder), p2a(p)
    {
        bb = curBB;
        numInstsRemoved = 0;
        duTablePopulated = false;
    }

    void doLVN();
    unsigned int getNumInstsRemoved() { return numInstsRemoved; }
};
}
#endif
