/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/CISACodeGen.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Allocator.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    class FunctionPass;
    class raw_ostream;
}

// #define __DEBUG_SYMBEXPR__

namespace IGC
{
    /// Generic Polynomial Symbolic Expression (PSE)
    ///   PSE = C0*T0 + C1*T1 + C2*T2 + ... + Cn;     (n = #terms)
    ///   where each Ti = Vi0 * Vi1 * Vi2 * ... * Vim (m = #Values)
    //
    // class SymTerm : symbolic term for denoting Ci * Ti
    //
    // class SymProd : symbolic product for Ti.
    //   Right now, no SymProd will be shared among different expressions.
    //   (we could use FoldingSet to make SymProd unique & shared among all
    //    symbolic expressions in the way that the same product will always
    //    use the same object of SymProd. In doing so, comparison of two
    //    products are simply carried out by comparing their pointer value.)
    //
    class SymProd {
    public:
        llvm::SmallVector<const llvm::Value*, 2> Prod;

        SymProd() {}
        SymProd(const SymProd& P) : Prod(P.Prod) {}

        SymProd& operator=(const SymProd& P) = delete;
#if 0
        {
            Prod.append(P.Prod.begin(), P.Prod.end());
        }
#endif
    };

    class SymTerm {
    public:
        SymProd* Term;
        int64_t Coeff;

        SymTerm() : Term(nullptr), Coeff(1) {}

        SymTerm(const SymTerm& T) = delete;
        SymTerm& operator=(const SymTerm& T) = delete;
    };

    // class SymExpr : representation of Symbolic expression.
    //   SymTerms[0] + SymTerms[1] + ... + SymTerms[n] + ConstTerm
    // where n = SymTerms.size().
    class SymExpr {
    public:
        llvm::SmallVector<SymTerm*, 4> SymTerms;
        int64_t ConstTerm;

        SymExpr() : ConstTerm(0) {}
    };

    /*
     *  This is an integer symbolic evaluation, intended for address calculation
     *  of straight-line code.
     *
     *  The storage of symbolic expression is owned by this class. Once this
     *  class is destructed, so is its storage for the expression (including
     *  storage for SymTerm).
     */
    class SymbolicEvaluation
    {
    public:

        SymbolicEvaluation() :
            m_DL(nullptr),
            m_hasOverflow(false),
            m_nextValueID(0)
        {}
        void setDataLayout(const llvm::DataLayout* aDL) { m_DL = aDL; }

        ~SymbolicEvaluation()
        {
#if defined (__DEBUG_SYMBEXPR__)
            if (exceedMaxValues()) {
                std::cerr << "SymbolicEvaluation: #values exceeds max limit: "
                    << MAX_NUM_VALUES
                    << "\n";
            }
#endif
            clear();
        }

        SymbolicEvaluation(const SymbolicEvaluation&) = delete;
        SymbolicEvaluation& operator=(const SymbolicEvaluation&) = delete;

        // Return a Canonicalized Polynomial Expression.
        SymExpr* getSymExpr(const llvm::Value* V);

        // If S1 - S0 = constant, return true and set "COff" to that constant
        bool isOffByConstant(SymExpr* S0, SymExpr* S1, int64_t& COff);

        // Return the lexical order of two products. It is used to sort
        // an expression in canonical form:
        //    -1: P0 precedes P1
        //     0: P0 has the same order as P1 ( P0 must be equal to P1)
        //     1: P1 precedes P0
        int cmp(const SymProd* T0, const SymProd* T1);

        SymExpr* add(SymExpr* S0, SymExpr* S1, bool negateS1);
        SymExpr* add(SymExpr* S, int64_t C);
        SymExpr* mul(SymExpr* S, int64_t C);

        // If N is a factor of S's symbolic part, that is, N
        // can divide all coeffs of S's symbolic terms.
        bool isFactor(SymExpr* S, int N)
        {
            for (int i = 0, e = S->SymTerms.size(); i < e; ++i)
            {
                if ((S->SymTerms[i]->Coeff % N) != 0) {
                    return false;
                }
            }
            return true;
        }

        void copy(SymTerm* D, SymTerm* S)
        {
            D->Term = new (m_symEvaAllocator) SymProd(*S->Term);
            D->Coeff = S->Coeff;
        }

        void copy(SymExpr* D, const SymExpr* S)
        {
            for (int i = 0, e = S->SymTerms.size(); i < e; ++i)
            {
                SymTerm* newT = new (m_symEvaAllocator) SymTerm();
                copy(newT, S->SymTerms[i]);
                D->SymTerms.push_back(newT);
            }
            D->ConstTerm = S->ConstTerm;
        }

        void clear() {
            m_nextValueID = 0;
            m_DL = nullptr;
            m_symInfos.clear();
            m_symEvaAllocator.Reset();
        }

#if defined(_DEBUG)
        void print(llvm::raw_ostream& OS, const SymProd* P);
        void print(llvm::raw_ostream& OS, const SymTerm* T);
        void print(llvm::raw_ostream& OS, const SymExpr* SE);
        void print(llvm::raw_ostream& OS, const llvm::Value* V);
        void print_varMapping(llvm::raw_ostream& OS, const SymProd* P);
        void print_varMapping(llvm::raw_ostream& OS, const SymTerm* T);
        void print_varMapping(llvm::raw_ostream& OS, const SymExpr* SE);
        void print_varMapping(llvm::raw_ostream& OS, const llvm::Value* V);

        void dump_symbols();
        void dump(const SymProd* P);
        void dump(SymProd* P);
        void dump(const SymTerm* T);
        void dump(SymTerm* T);
        void dump(const SymExpr* SE);
        void dump(SymExpr* SE);
        void dump(const llvm::Value* V);
        void dump(llvm::Value* V);
#endif

    private:
        enum SymCastInfo:uint8_t {
            SYMCAST_NOCAST,        // no sext/zext/trunc
            SYMCAST_SEXT,          // sext
            SYMCAST_ZEXT           // zext
        };

        // false : assume no overflow on all operations
        // true  : some operations may overflow, need to check nsw/nuw, etc.
        bool m_hasOverflow;
        bool considerOverflow() const { return m_hasOverflow; }

        const llvm::DataLayout* m_DL = nullptr;
        // This struct is to hold info about symbol (Value), such as its ID,
        // and its equivalent symbolic expression.
        typedef struct {
            uint32_t ID : 16;
            uint32_t CastInfo : 8;  // SymCastInfo

            SymExpr* symExpr;
        } ValueSymInfo;
        typedef llvm::DenseMap<const llvm::Value*, ValueSymInfo*> SymInfoMap;

        // Used to assign a unique ID to ValueSymInfo
        uint16_t m_nextValueID;
        const uint16_t MAX_NUM_VALUES = 10000;
        bool exceedMaxValues() const { return m_nextValueID > MAX_NUM_VALUES; }

        SymInfoMap m_symInfos;
        llvm::BumpPtrAllocator m_symEvaAllocator;

        // A varaint of getSymExpr.  This one does not create SymExpr if
        // V is an integer constant. Instead, return constant as 'C'.
        void getSymExprOrConstant(const llvm::Value* V, SymExpr*& S, int64_t& C);

        ValueSymInfo* getSymInfo(const llvm::Value* V)
        {
            auto SIIter = m_symInfos.find(V);
            if (SIIter != m_symInfos.end())
            {
                ValueSymInfo* VSI = SIIter->second;
                return VSI;
            }
            return nullptr;
        }

        void setSymInfo(const llvm::Value* V, SymExpr* E)
        {
            ValueSymInfo* VSI = new (m_symEvaAllocator) ValueSymInfo();
            VSI->ID = m_nextValueID++;
            VSI->CastInfo = SymCastInfo::SYMCAST_NOCAST;
            VSI->symExpr = E;
            m_symInfos.insert(std::make_pair(V, VSI));
        }
    };

    llvm::FunctionPass* createSLMConstPropPass();
}  // namespace IGC
