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

namespace IGC
{
    /// Generic Polynomial Symbolic Expression (PSE)
    ///   PSE = C0*T0 + C1*T2 + C2*T3 + ... + Cn;     (n = #terms)
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
        llvm::SmallVector<llvm::Value*, 2> Prod;

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

        SymbolicEvaluation() : m_nextValueID(0) {}

        // Return a Canonicalized Polynomial Expression.
        SymExpr* getSymExpr(llvm::Value* V);

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

#if defined(_DEBUG)
        void print(llvm::raw_ostream& OS, SymProd* P);
        void print(llvm::raw_ostream& OS, SymTerm* T);
        void print(llvm::raw_ostream& OS, SymExpr* SE);

        void dump_symbols();
        void dump(SymProd* P);
        void dump(SymTerm* T);
        void dump(SymExpr* SE);
#endif


    private:
        // This struct is to hold info about symbol (Value), such as its ID,
        // and its equivalent symbolic expression.
        typedef struct {
            int ID;
            SymExpr* symExpr;
        } ValueSymInfo;
        typedef llvm::DenseMap<llvm::Value*, ValueSymInfo*> SymInfoMap;

        // Used to assign a unique ID to ValueSymInfo
        int m_nextValueID;

        SymInfoMap m_symInfos;
        llvm::BumpPtrAllocator m_symEvaAllocator;

        // A varaint of getSymExpr.  This one does not create SymExpr if
        // V is an integer constant. Instead, return constant as 'C'.
        void getSymExprOrConstant(llvm::Value* V, SymExpr*& S, int64_t& C);

        // set & get functions to SymInfoMap
        ValueSymInfo* getSymInfo(llvm::Value* V)
        {
            auto SIIter = m_symInfos.find(V);
            if (SIIter != m_symInfos.end())
            {
                ValueSymInfo* VSI = SIIter->second;
                return VSI;
            }
            return nullptr;
        }

        void setSymInfo(llvm::Value* V, SymExpr* E)
        {
            ValueSymInfo* VSI = new (m_symEvaAllocator) ValueSymInfo();
            VSI->ID = m_nextValueID++;
            VSI->symExpr = E;
            m_symInfos.insert(std::make_pair(V, VSI));
        }
    };

    llvm::FunctionPass* createSLMConstPropPass();
}  // namespace IGC
