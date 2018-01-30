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

//===----------------------------------------------------------------------===//
//
// This file implements special handling for generic address pointers.
//
//===----------------------------------------------------------------------===//

#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressStaticResolution.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/ValueMap.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;


namespace IGC
{

// Register pass to igc-opt
#define PASS_FLAG "igc-generic-static-analysis"
#define PASS_DESCRIPTION "Static Analysis of generic address space"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(IGILGenericAddressStaticResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
//IGC_INITIALIZE_PASS_DEPENDENCY(DataLayout)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(IGILGenericAddressStaticResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

/// @brief  GenericAddressStaticResolution class resolves generic address space 
/// @brief  pointers to a named address space: local, global or private.

static void assocDebugLocWith(
    Instruction*  pNew, 
    const Instruction*  pOld ) 
{
    auto DL = pOld->getDebugLoc();
    if(!DL) 
    {
        pNew->setDebugLoc( pOld->getDebugLoc() );
    }
}

static bool isSinglePtr( const Type* pPtrType ) 
{
    bool singlePtr = false;
    Type* pPtrElementType = pPtrType->getPointerElementType();
    if( pPtrElementType != NULL )
    {
        if( ( pPtrElementType->isArrayTy() == false ) && 
            ( pPtrElementType->isPointerTy() == false ) )
        {
            singlePtr = true;
        }
    }

    return singlePtr;
}

IGILGenericAddressStaticResolution::IGILGenericAddressStaticResolution() : FunctionPass(ID), m_handlePrivateSpace(false)
{
    initializeIGILGenericAddressStaticResolutionPass(*PassRegistry::getPassRegistry());
}

IGILGenericAddressStaticResolution::IGILGenericAddressStaticResolution(bool HandlePrivateSpace) : FunctionPass(ID), m_handlePrivateSpace(HandlePrivateSpace)
{
    initializeIGILGenericAddressStaticResolutionPass(*PassRegistry::getPassRegistry());
}

bool IGILGenericAddressStaticResolution::runOnFunction( Function &F ) {
    bool changed = false;

    m_failCount = 0;
    m_GASPointers.clear();
    m_GASEstimate.clear();
    m_replaceMap.clear();
    m_replaceVector.clear();

    // Prepare per-function elements of the collection
    analyzeGASPointers( F );

    // Static resolution of the collected instructions
    changed |= resolveGASPointers( F );

    return changed;
}

void IGILGenericAddressStaticResolution::analyzeGASPointers( Function &F ) 
{
    inst_iterator inst_it, inst_it_end;
    TPointerList::iterator ptr_it;

    // Collect GAS pointers initializations in the function (together with their uses' tree)
    for (inst_it = inst_begin( F ), 
         inst_it_end = inst_end( F ); 
         inst_it != inst_it_end; inst_it++ )
    {
        Instruction*  pInstr = &(*inst_it);
        const AllocaInst* pAlloca = dyn_cast<const AllocaInst>( pInstr );

        // Filter-out unsupported cases of structs of GAS pointers
        if( ( pAlloca != NULL ) && 
            ( isAllocaStructGASPointer( pAlloca->getAllocatedType(), false) == true ) ) 
        {
            assert( 0 && "No support for structs of generic address space pointers!" );
            continue;
        }

        bool okToProcess = false;
        unsigned opCode = pInstr->getOpcode();

        // At first, we check the most frequent initialization cases: 
        //     <named>-to-<generic> address space conversion of pointer value by BitCast and GEP
        if( opCode == Instruction::BitCast || opCode == Instruction::AddrSpaceCast || opCode == Instruction::GetElementPtr ) 
        {
            const PointerType*  pPtrType = dyn_cast<const PointerType>( pInstr->getType() ); 
            if( (pPtrType != NULL) && 
                (pPtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC))
            {
                Value*  pOperand = pInstr->getOperand( 0 );
                const PointerType *pSrcPtrType = dyn_cast<PointerType>( pOperand->getType() );
                Instruction* srcInst = dyn_cast<Instruction>( pOperand );

                // require connectivity for BitCast for select/cmp sources
                if(opCode == Instruction::AddrSpaceCast || opCode == Instruction::BitCast )
                {
                    if( srcInst != NULL )
                    {
                        switch( srcInst->getOpcode() )
                        {
                            case Instruction::Select:
                            case Instruction::ICmp:
                            {
                                for( ptr_it = m_GASPointers.begin(); 
                                     ptr_it != m_GASPointers.end(); 
                                     ptr_it++ ) 
                                {
                                    Instruction*  pCurInstr = *ptr_it;
                                    if( srcInst == pCurInstr )
                                    {
                                        okToProcess = true;
                                    }
                                }
                                break;
                            }

                            default:
                                okToProcess = true;
                                break;
                        }
                    }
                    else if (isa<Argument>(pOperand))
                    {
                        okToProcess = true;
                    }
                    else if (isa<GlobalVariable>(pOperand))
                    {
                        okToProcess = true;
                    }
                }
                else
                {
                    okToProcess = true;
                }

                if ( ( pSrcPtrType != NULL ) && 
                     ( okToProcess == true ) &&
                     ( isSinglePtr(pSrcPtrType) == true ) && 
                     ( pSrcPtrType->getAddressSpace() != ADDRESS_SPACE_GENERIC ) ) 
                {
                    // If this is a conversion from named pointer type to GAS pointer: 
                    // store GAS pointer info into the collection (together with its uses - recursively)
                    addGASInstr( pInstr, pSrcPtrType->getAddressSpace(), nullptr );
                    continue;
                }
            }
        }
        else if( opCode == Instruction::ICmp || opCode == Instruction::Select )
        {
            // Then - look for constant expression producing generic addr-space pointer value
            // out of named one (inside a ConstantExpr operand)
            unsigned int val[2];
            unsigned startIdx = ( opCode == Instruction::ICmp ) ? 0 : 1;
            for( unsigned idx = 0; idx < 2; idx++ ) 
            {
                val[idx] = evalGASConstantExprIfNeeded( pInstr->getOperand( idx + startIdx ), pInstr );
            }
            
            // We want to guard against a race on address space propagation if two pointers do not have
            // similar address spaces
            if( val[0] != val[1] )
            {
                if( (val[0] == ADDRESS_SPACE_NUM_ADDRESSES) || (val[1] == ADDRESS_SPACE_NUM_ADDRESSES) )
                {
                    okToProcess = true;
                }
                else if( ( val[0] == ADDRESS_SPACE_GENERIC ) || ( val[1] == ADDRESS_SPACE_GENERIC ) )
                {
                    okToProcess = true;
                }
            }
        }
        else
        {
            okToProcess = true;
        }
        
        if( okToProcess )
        {
            for( unsigned idx = 0; idx < pInstr->getNumOperands(); idx++ ) 
            {
                if( handleGASConstantExprIfNeeded( pInstr->getOperand( idx ), pInstr ) ) 
                {
                    break;
                }
            }
        }
        // We don't handle 'IntToPtr' case (another initialization case) here because we cannot
        // guess about its named space origin. We will reach it during propagation of
        // corresponding integer value (generated by an 'PtrToInt' instruction)
    }

    // Now collect use trees of GAS pointer initializations
    for( ptr_it = m_GASPointers.begin(); 
         ptr_it != m_GASPointers.end(); 
         ptr_it++ ) 
    {
        Instruction*  pInstr = *ptr_it;
        TPointerMap::const_iterator estimate = m_GASEstimate.find( pInstr );
        assert( estimate != m_GASEstimate.end() && "GAS Collection is broken!" );

        // We can add new pointers during propagation because they are collected
        // into list - whose iterator is safe after insertion
        propagateSpace( pInstr, estimate->second, nullptr );
    }
}

bool IGILGenericAddressStaticResolution::resolveGASPointers( Function &F ) 
{
    bool changed = false;

    // Iterate through the collection of GAS pointers and try to resolve them statically
    // to named address space pointer  
    TPointerList::iterator ptr_it, ptr_end;
    for( ptr_it = m_GASPointers.begin(), 
         ptr_end = m_GASPointers.end();
         ptr_it != ptr_end; ptr_it++ )
    {
        Instruction *pInstr = *ptr_it;
        TPointerMap::const_iterator map_it = m_GASEstimate.find( pInstr );

        assert( map_it != m_GASEstimate.end() && "GAS pointer collection is broken!" );
        unsigned int space = map_it->second;

        // Ignore instructions which cannot be resolved
        if( space == ADDRESS_SPACE_GENERIC ) 
        {
            continue;
        }

        // Resolve GAS pointers from collection:
      
        // 1. Prepare replacements with named addr space pointers
        switch( pInstr->getOpcode() ) 
        {
            case Instruction::IntToPtr:
            case Instruction::AddrSpaceCast:
            case Instruction::BitCast:
            case Instruction::GetElementPtr:
                changed |= resolveInstructionConvert( pInstr, space );
                break;

            case Instruction::ZExt:
            case Instruction::Trunc:
                // Nothing to resolve for this operation - it served only as transit
                // node for propagation of integer-out-of-GAS-pointer
                break;

            case Instruction::Load:
            case Instruction::Store:
            case Instruction::AtomicCmpXchg:
            case Instruction::AtomicRMW:
            case Instruction::PtrToInt:
            case Instruction::Call:
                changed |= resolveInstructionOnePointer( pInstr, space );
                break;

            case Instruction::PHI:
                changed |= resolveInstructionPhiNode( cast<PHINode>( pInstr ), space );
                break;

            case Instruction::Select:
            case Instruction::ICmp:
                changed |= resolveInstructionTwoPointers( pInstr, space );
                break;

            default:
                if( pInstr->isBinaryOp() ) 
                {
                    // Nothing to resolve for binary operation - it served only as transit
                    // node for propagation of integer-out-of-GAS-pointer
                    break;
                }
                assert( 0 && "Unexpected instruction with generic address space pointer" );
                break;
        }
    }

    // 2. Integrate replacements into the function body
    TReplaceVector::const_reverse_iterator repl_it, repl_end;
    for( repl_it = m_replaceVector.rbegin(),
         repl_end = m_replaceVector.rend();
         repl_it != repl_end; repl_it++ ) 
    {
        Instruction*  pOldInstr = repl_it->first;
        Value*  pNewVal = repl_it->second;

        // Replace uses of original instruction with those of new value
        switch( pOldInstr->getOpcode() ) 
        {
            case Instruction::Load:
            case Instruction::Store:
            case Instruction::AtomicCmpXchg:
            case Instruction::AtomicRMW:
            case Instruction::PtrToInt:
            case Instruction::ICmp:
                // For instruction which doesn't produce a pointer: replace uses with new value
                pOldInstr->replaceAllUsesWith( pNewVal );
                break;

            case Instruction::AddrSpaceCast:
            case Instruction::BitCast:
            case Instruction::IntToPtr: 
            case Instruction::GetElementPtr: 
            {
                // For Int2Ptr/Bitcast/GEP instruction which ORIGINALLY produced NAMED addr-space pointer 
                // or integer: replace uses with new value
                PointerType*  pDestType = dyn_cast<PointerType>( pOldInstr->getType() );

                if( ( pDestType == NULL ) || 
                    ( pDestType->getAddressSpace() != ADDRESS_SPACE_GENERIC ) ) 
                {
                    pOldInstr->replaceAllUsesWith( pNewVal );
                } 
                else 
                {
                    // Clean-up is need because BFS tree of GAS data flow is not guaranteed 
                    // to be balanced, and yet may have cycles
                    //pOldInstr->replaceAllUsesWith( Constant::getNullValue( pOldInstr->getType() ) );
                }

                break;
            }

            default:
                // For instruction which produces a pointer (less bitcast/GEP special case above): 
                // its use is already set during address space resolution, however
                // clean-up is yet needed because BFS tree of GAS data flow is not guaranteed
                // to be balanced, and yet may have cycles
                //pOldInstr->replaceAllUsesWith( Constant::getNullValue( pOldInstr->getType() ) );
                break;
        }

        // Fix-up debug info for new instruction
        Instruction*  pNewInstr = dyn_cast<Instruction>( pNewVal );

        if( pNewInstr != NULL )
        {
            assocDebugLocWith( pNewInstr, pOldInstr );
        }

        // Remove original instruction
        if (pOldInstr->use_empty())
        {
            pOldInstr->eraseFromParent();
        }
    }

    return changed;
}

void IGILGenericAddressStaticResolution::addGASInstr(
    Instruction*  pInstr, 
    unsigned int space,
    Instruction*  pParentInst) 
{
    IntrinsicInst*  pInstrinInstr = dyn_cast<IntrinsicInst>( pInstr );

    // Special case: call to LLVM intrinsic which is not overloadable.
    // In such case we should preserve GAS pointer as is.
    if( pInstrinInstr != NULL ) 
    {
        if (!Intrinsic::isOverloaded(pInstrinInstr->getIntrinsicID() ) ) 
        {
            space = ADDRESS_SPACE_GENERIC;
        }
    }

    if( llvm::StoreInst * pSI = llvm::dyn_cast<llvm::StoreInst>( pInstr ) )
    {
        if( ( pParentInst != NULL ) && ( pSI->getValueOperand() == pParentInst ) 
            && ( pParentInst->getType()->isPointerTy() == false ) )
        {
            // We've got here by tracking the store instruction's non-pointer value operand - don't 
            //  go any further
            return;
        }
    }

    TPointerMap::iterator ptr_it = m_GASEstimate.find( pInstr );

    if( ptr_it == m_GASEstimate.end() ) 
    {
        // For first-seen instruction - record it
        m_GASPointers.push_back( pInstr );

        if( pInstr->getOpcode() == Instruction::Ret ) 
        {
            // Enforce GAS return value on Ret - on order to be symmetrical to Call
            space = ADDRESS_SPACE_GENERIC;
        }

        if( pInstr->getOpcode() == Instruction::Store ) 
        {
            const PointerType*  pValuePtrType = 
                dyn_cast<const PointerType>( pInstr->getOperand(0)->getType() );

            // If Store instruction encountered due to its VALUE operand (rather than
            // ADDRESS operand) - we should not resolve it. That is because not all 
            // uses of corresponding 'alloca' GAS pointer may be resolvable.
            if( ( pValuePtrType != NULL ) && 
                ( pValuePtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) 
            {
                // The Store is due to GAS pointer value operand - enforce it as GAS case
                assert(
                    ( cast<PointerType>( pInstr->getOperand(1)->getType() )->getAddressSpace() != ADDRESS_SPACE_GENERIC ) &&
                    "GAS pointer cannot be stored to memory pointed-to by GAS pointer!" );
                space = ADDRESS_SPACE_GENERIC;
            }
        }

        m_GASEstimate.insert( TPointerInfo( pInstr, space ) );

        return;
    }

    // If we reached already traversed node - validate its type 
    if( ptr_it->second == space ) 
    {
        // Original addr space is confirmed - nothing to do
        return;
    }

    // In the case of conflicting types - revert named space to generic
    if ( space != ADDRESS_SPACE_GENERIC ) 
    {
        // Account for failure
        m_failCount++;
    }

    if( ptr_it->second != ADDRESS_SPACE_GENERIC ) 
    {
        // In the case of conflict - enforce GAS type and ...
        ptr_it->second = ADDRESS_SPACE_GENERIC;
        // ... proceed to uses in order to revert them to generic space as well
        propagateSpace( pInstr, ptr_it->second, nullptr );
    }
}

void IGILGenericAddressStaticResolution::propagateSpace(
    Instruction*  pInstr,
    unsigned int space,
    Instruction*  pParentInst) 
{
    bool toPropagate = false;

    switch ( pInstr->getOpcode() ) 
    {
        // Instructions which don't generate new pointer - no propagation
        case Instruction::Load:
        case Instruction::Store:
        case Instruction::AtomicCmpXchg:
        case Instruction::AtomicRMW:
        case Instruction::ICmp:
        case Instruction::Ret:
        case Instruction::Call:
        case Instruction::ZExt:
        case Instruction::Trunc:
            break;

        // Ptr2Int generates integer which may become a pointer back later:
        // propagate to uses
        case Instruction::PtrToInt:
            toPropagate = true;
            break;

        // Instructions which generate new GAS pointer or integer-out-of-GAS-pointer:
        // propagate to uses
        case Instruction::PHI:
        case Instruction::Select:
            toPropagate = true;
            break;

        // GEP/Int2Ptr may or may not generate new GAS pointer - analyze further
        case Instruction::IntToPtr:
        case Instruction::GetElementPtr: 
        {
            const PointerType*  pPtrType = cast<const PointerType>( pInstr->getType() );

            // Filter-out GEP/Int2Ptr which doesn't propagate GAS pointer
            if( ( pPtrType != NULL ) && 
                ( pPtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) 
            {
                toPropagate = true;
            }
            break;
        }

        // Bitcast may or may not generate new GAS pointer - analyze further
        case Instruction::AddrSpaceCast: 
        case Instruction::BitCast: 
        {
            const PointerType*  pPtrType = dyn_cast<const PointerType>( pInstr->getType() );

            // Filter-out Bitcasts which don't propagate GAS pointer
            if( ( pPtrType != NULL ) && 
                ( pPtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) )
            {
                toPropagate = true;
            }
            break;
        }

        default:
        {
            if( pInstr->isBinaryOp() ) 
            {
                // Integer which is produced out of pointer can be propagated by binary operation
                toPropagate = true;
                break;
            }

            // No other instruction is expected (as bitwise and numerical conversions of
            // integer produced out of pointer are not expected)
            assert( 0 && "Unexpected instruction with generic address space pointer" );
            break;
        }
    }

    if( toPropagate ) 
    {
        Value::user_iterator use_it, use_end;
        for( use_it = pInstr->user_begin(), 
             use_end = pInstr->user_end();
             use_it != use_end; use_it++ ) 
        {
            Instruction*  pUse = dyn_cast<Instruction>( *use_it );
            if( pUse != NULL )
            {
                addGASInstr( pUse, space, pInstr );
            }
        }
    }
}

bool IGILGenericAddressStaticResolution::handleGASConstantExprIfNeeded(
    Value*  pOperand, 
    Instruction*  pInstr )
{
    bool retValue = false;
    PointerType*  pPtrType = dyn_cast<PointerType>( pOperand->getType() );

    // Check that the operand produces GAS pointer
    if( ( pPtrType != NULL ) && 
        ( pPtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) )
    {
        ConstantExpr* pCE = dyn_cast<ConstantExpr>( pOperand );

        // Check for constant expression
        if( pCE != NULL ) 
        {
            // Check operands of the constant expression
            for( unsigned idx = 0; idx < pCE->getNumOperands(); idx++ ) 
            {
                Value*  pOpVal = pCE->getOperand( idx );
                PointerType*  pOpPtrType = dyn_cast<PointerType>( pOpVal->getType() );

                if( pOpPtrType != NULL ) 
                {
                    unsigned int opPtrSpace = pOpPtrType->getAddressSpace();

                    // We're looking only for pointer operands of the expression
                    if( opPtrSpace == ADDRESS_SPACE_GENERIC ) 
                    {
                        // If the pointer is GAS - look for a named addr-space pointer behind him
                        retValue = handleGASConstantExprIfNeeded( pOpVal, pInstr );
                        break;
                    } 
                    else 
                    {
                        // If the pointer is named - add the instruction to the collection
                        addGASInstr( pInstr, opPtrSpace, nullptr );
                        retValue = true;
                        break;
                    }
                }
            }
        }
    }

    return retValue;
}

unsigned int IGILGenericAddressStaticResolution::evalGASConstantExprIfNeeded(
    Value*  pOperand, 
    Instruction*  pInstr )
{
    unsigned int retValue = ADDRESS_SPACE_NUM_ADDRESSES;
    PointerType*  pPtrType = dyn_cast<PointerType>( pOperand->getType() );

    // Check that the operand produces GAS pointer
    if( pPtrType != NULL )
    {
        ConstantExpr* pCE = dyn_cast<ConstantExpr>( pOperand );

        // Check for constant expression
        if( pPtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC )
        {
            retValue = ADDRESS_SPACE_GENERIC;
        }

        if( pCE != NULL ) 
        {
            // Check operands of the constant expression
            for( unsigned idx = 0; idx < pCE->getNumOperands(); idx++ ) 
            {
                Value*  pOpVal = pCE->getOperand( idx );
                PointerType*  pOpPtrType = dyn_cast<PointerType>( pOpVal->getType() );

                if( pOpPtrType != NULL ) 
                {
                    unsigned int opPtrSpace = pOpPtrType->getAddressSpace();

                    // We're looking only for pointer operands of the expression
                    if( opPtrSpace == ADDRESS_SPACE_GENERIC ) 
                    {
                        // If the pointer is GAS - look for a named addr-space pointer behind him
                        retValue = evalGASConstantExprIfNeeded( pOpVal, pInstr );
                        break;
                    } 
                    else 
                    {

                        // record what we found
                        retValue = opPtrSpace;
                        break;
                    }
                }
            }
        }
    }

    return retValue;
}

bool IGILGenericAddressStaticResolution::resolveInstructionConvert(
    Instruction*  pInstr, 
    unsigned int space ) 
{
    bool retValue = false;
    Value*  pPrevValue = pInstr->getOperand( 0 );
    if( pPrevValue != NULL )
    {
        const PointerType*  pSrcType = dyn_cast<PointerType>( pPrevValue->getType() );
        PointerType*  pDestType = dyn_cast<PointerType>( pInstr->getType() );

        if( ( pSrcType != NULL ) && 
            ( pDestType != NULL ) ) {
            assert( ( ( pSrcType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) || 
                      ( pDestType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) &&
                    "Cannot reach this point with named-to-named conversion!");
        }

        Instruction*  pNewInstr = NULL;

        if( ( pDestType != NULL ) && 
            ( pDestType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) 
        {
            // Original conversion is to GAS pointer:
            //   - produce BitCast/GEP for <named>-to-<named> out of <named/generic>-to-<generic>
            //                     OR
            //   - produce Int2Ptr for <int>-to<named> out of <int>-to<generic>

            // At first - try to find a resolved value for source operand
            Value*  pNewPrevValue = getResolvedOperand( pPrevValue, space );
            if( pNewPrevValue != NULL ) 
            {
                pPrevValue = pNewPrevValue;
            }

            // Cannot remove generic from private memory
            if( ( m_handlePrivateSpace == false ) && ( pSrcType != NULL ) && 
                ( pSrcType->getAddressSpace() == ADDRESS_SPACE_PRIVATE ) ) 
            {
                return retValue;
            }

            // Cannot remove generic dst for private memory usage
            if( ( m_handlePrivateSpace == false ) && ( space == ADDRESS_SPACE_PRIVATE ) )
            {
                return retValue;
            }

            // Then - generate the instruction
            switch( pInstr->getOpcode() ) 
            {
                case Instruction::IntToPtr: 
                {
                    pNewInstr = new IntToPtrInst(
                        pPrevValue,
                        PointerType::get( pDestType->getElementType(), space ),
                        pInstr->getName(), 
                        pInstr );

                    break;
                }

                case Instruction::AddrSpaceCast: 
                case Instruction::BitCast : 
                {
                    pNewInstr = new BitCastInst(
                        pPrevValue,
                        PointerType::get( pDestType->getElementType(), space ),
                        pInstr->getName(), 
                        pInstr );

                    break;
                }

                case Instruction::GetElementPtr : 
                {
                    GetElementPtrInst*  pGepInstr = cast<GetElementPtrInst>( pInstr );
                    SmallVector<Value*, 8> idxList;
                    GetElementPtrInst::const_op_iterator idx_it, idx_end;

                    for( idx_it = pGepInstr->idx_begin(), 
                          idx_end = pGepInstr->idx_end();
                          idx_it != idx_end; idx_it++ ) 
                    {
                        idxList.push_back( *idx_it );
                    }

                    GetElementPtrInst*  pNewGEP = 
                        GetElementPtrInst::Create(
                            nullptr,
                            pPrevValue, 
                            idxList,
                            pInstr->getName(), 
                            pInstr );

                    pNewGEP->setIsInBounds( pGepInstr->isInBounds() );
                    pNewInstr = pNewGEP;

                    break;
                }

                default:
                    assert( 0 && "Unexpected instruction with generic address space pointer target" );
                    break;
            }

            if( pNewInstr != NULL )
            {
                // Fix-up pointer usages
                fixUpPointerUsages( pNewInstr, pInstr );
            }
        } 
        else if ( ( pSrcType != NULL ) && 
                  ( pSrcType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) 
        {
            // Original bitcast is from generic to named: check validity of the conversion 
            //                                            and produce named-to-named anyway
            // Original bitcast to from generic to integer: produce named-to-integer

            // At first - find a resolution for source value
            if (pInstr->getOpcode() == Instruction::BitCast ||
                pInstr->getOpcode() == Instruction::AddrSpaceCast)
            {
                pPrevValue = getResolvedOperand( pPrevValue, space );
                if( pPrevValue != NULL )
                {
                    const PointerType*  pNewType = dyn_cast<PointerType>( pPrevValue->getType() );
                    if( ( pNewType != NULL ) &&
                        ( pNewType->getAddressSpace() == space ) ) 
                    {
                        pNewInstr = new BitCastInst(
                            pPrevValue, 
                            pInstr->getType(), 
                            pInstr->getName(), 
                            pInstr );
                    }
                }
            }
        } 
        else if( ( pDestType == NULL ) || 
                 ( pDestType->getAddressSpace() != ADDRESS_SPACE_GENERIC ) ) 
        {
            // The only reasons for that case are:
            // - BitCast from named addr-space pointer to non-pointer
            // - Int2Ptr to named addr-space
            // In both case we just need to generate new instruction on the basis of replaced operand

            // At first - find a resolution for source value
            pPrevValue = getResolvedOperand( pPrevValue, space );
            if( pPrevValue != NULL )
            {
                // Then - generate the instruction
                switch( pInstr->getOpcode() ) 
                {
                    case Instruction::AddrSpaceCast: 
                    case Instruction::BitCast:
                    {
                        pNewInstr = new BitCastInst(
                            pPrevValue, 
                            pInstr->getType(), 
                            pInstr->getName(), 
                            pInstr );

                        break;
                    }

                    case Instruction::IntToPtr:
                    {
                        pNewInstr = new IntToPtrInst(
                            pPrevValue, 
                            pInstr->getType(), 
                            pInstr->getName(), 
                            pInstr );

                        break;
                    }

                    default:
                        assert( 0 && "Unexpected instruction with named address space pointer or integer target!" );
                        break;
                }
            }
        } 
        else 
        {
            assert( 0 && "Unexpected convert instruction!" );
        }

        if( pNewInstr != NULL )
        {
            // Schedule the original instruction for replacement
            m_replaceMap.insert( TMapPair( pInstr, pNewInstr ) );
            m_replaceVector.push_back( TMapPair( pInstr, pNewInstr ) );
            retValue = true;
        }
    }

    return retValue;
}

bool IGILGenericAddressStaticResolution::resolveInstructionOnePointer(
    Instruction*  pInstr, 
    unsigned int space ) 
{
    unsigned ptrOperandIdx = pInstr->getOpcode() == Instruction::Store ? 1 : 0;
    const PointerType*  pSrcType = dyn_cast<PointerType>( pInstr->getOperand( ptrOperandIdx )->getType() );
    bool retValue = false;

    if( ( pSrcType == NULL ) || 
        ( pSrcType->getAddressSpace() != ADDRESS_SPACE_GENERIC ) ) 
    {
        assert( 0 && "Only GAS pointer is expected here!" );
        return retValue;
    }

    // At first - finding preceding instruction (original and its resolution)
    Value*  pPrevValue = pInstr->getOperand( ptrOperandIdx );
    if( conformOperand( pPrevValue, space ) )
    {
        Value*  pNewValue = getResolvedOperand( pPrevValue, space ); 
        if( pNewValue != NULL )
        {
            Instruction *pNewInstr = NULL;

            // Then - generate the instruction
            switch( pInstr->getOpcode() ) 
            {
                case Instruction::Load: 
                {
                    LoadInst* pLoadInstr = cast<LoadInst>( pInstr );
                    LoadInst* pNewLoad = new LoadInst( 
                        pNewValue, 
                        pLoadInstr->getName(), 
                        pLoadInstr->isVolatile(),  
                        pLoadInstr->getAlignment(),
                        pLoadInstr->getOrdering(), 
                        pLoadInstr->getSynchScope(),
                        pLoadInstr);
                    pNewInstr = pNewLoad;

                    break;
                }

                case Instruction::Store: 
                {
                    StoreInst*  pStoreInstr = cast<StoreInst>( pInstr ); 
                    StoreInst*  pNewStore = new StoreInst( 
                        pStoreInstr->getValueOperand(), 
                        pNewValue,
                        pStoreInstr->isVolatile(),  
                        pStoreInstr->getAlignment(),
                        pStoreInstr->getOrdering(), 
                        pStoreInstr->getSynchScope(),
                        pStoreInstr );
                    pNewInstr = pNewStore;

                    break;
                }

                case Instruction::AtomicCmpXchg:
                {
                    AtomicCmpXchgInst* pCmpXchg = cast<AtomicCmpXchgInst>( pInstr );
                    AtomicCmpXchgInst*  pNewCmpXchg = new AtomicCmpXchgInst(
                        pNewValue, 
                        pCmpXchg->getCompareOperand(), 
                        pCmpXchg->getNewValOperand(), 
                        pCmpXchg->getSuccessOrdering(),
                        pCmpXchg->getFailureOrdering(),
                        pCmpXchg->getSynchScope(), 
                        pCmpXchg );
                    pNewCmpXchg->setVolatile( pCmpXchg->isVolatile() );
                    pNewInstr = pNewCmpXchg;
                    break;
                }

                case Instruction::AtomicRMW: 
                {
                    AtomicRMWInst* pAtomicRMW = cast<AtomicRMWInst>( pInstr );
                    AtomicRMWInst* pNewAtomicRMW = new AtomicRMWInst(
                        pAtomicRMW->getOperation(), 
                        pNewValue,
                        pAtomicRMW->getValOperand(),
                        pAtomicRMW->getOrdering(),
                        pAtomicRMW->getSynchScope(), 
                        pAtomicRMW );
                    pNewAtomicRMW->setVolatile( pAtomicRMW->isVolatile() );
                    pNewInstr = pNewAtomicRMW;

                    break;
                }

                case Instruction::PtrToInt: 
                {
                    pNewInstr = new PtrToIntInst(
                        pNewValue, 
                        pInstr->getType(), 
                        pInstr->getName(), 
                        pInstr );

                    break;
                }

                case Instruction::Call:
                {
                    const PointerType*  pNewType = dyn_cast<PointerType>(pNewValue->getType());
                    if ((pNewType == NULL) ||
                        (pNewType->getAddressSpace() == ADDRESS_SPACE_GENERIC))
                    {
                        // If the new value is still a generic address space pointer,
                        // simply update the instruction operand.
                        pInstr->setOperand(0, pNewValue);
                    }
                    else
                    {
                        unsigned    addressSpace = pNewType->getAddressSpace();

                        Function*   pFunction = cast<CallInst>(pInstr)->getCalledFunction();

                        // We should have already checked this, but better safe than sorry.
                        bool isQueryIntrinsic =
                            pFunction->getName().startswith("_Z9to_global") ||
                            pFunction->getName().startswith("_Z10to_private") ||
                            pFunction->getName().startswith("_Z8to_local");

                        if (isQueryIntrinsic)
                        {
                            bool result =
                                (addressSpace == ADDRESS_SPACE_LOCAL && pFunction->getName().startswith("_Z8to_local")) ||
                                (addressSpace == ADDRESS_SPACE_PRIVATE && pFunction->getName().startswith("_Z10to_private")) ||
                                (addressSpace == ADDRESS_SPACE_GLOBAL && pFunction->getName().startswith("_Z9to_global"));

                            if (!result)
                            {
                                Constant *value = Constant::getNullValue(pInstr->getType());
                                pInstr->replaceAllUsesWith(value);
                                pInstr->eraseFromParent();
                            }
                            else
                            {
                                // Look for NULL comparisons with known address space pointers
                                // which must statically evaluate the false.
                                SmallVector<Instruction*, 4> toErase;
                                for (auto U = pInstr->user_begin(); U != pInstr->user_end(); U++)
                                {
                                    if (ICmpInst *pCmp = dyn_cast<ICmpInst>(*U))
                                    {
                                        if (isa<ConstantPointerNull>(pCmp->getOperand(0)) ||
                                            isa<ConstantPointerNull>(pCmp->getOperand(1)))
                                        {
                                            pCmp->replaceAllUsesWith(
                                                ConstantInt::getFalse(
                                                    Type::getInt1Ty(pInstr->getContext())));
                                            toErase.push_back(pCmp);
                                        }
                                    }
                                }
                                for (auto I : toErase)
                                {
                                    I->eraseFromParent();
                                }
                            }

                            if (pInstr->use_empty())
                            {
                                pInstr->eraseFromParent();
                            }
                        }
                        else
                        {
                            // Handle llvm memory intrinsics
                            llvm::SmallVector<llvm::Type*, 3> intrinsicArgTypes;
                            llvm::SmallVector<llvm::Value*, 5> newCallArgs;
                            retValue = true;
                            switch(pFunction->getIntrinsicID())
                            {
                                default :
                                    retValue = false;
                                    break;

                                case llvm::Intrinsic::memcpy:
                                {
                                    llvm::MemCpyInst * mCpI = llvm::cast<llvm::MemCpyInst>(pInstr);
                                    llvm::Value * oldDest = mCpI->getRawDest();
                                    llvm::Value * oldSrc = mCpI->getRawSource();
                                    if(conformOperand(oldDest, space) || conformOperand(oldSrc, space))
                                    {
                                        llvm::Value * newDest = getResolvedOperand(oldDest, space);
                                        llvm::Value * newSrc = getResolvedOperand(oldSrc, space);
                                        assert(newDest || newSrc);
                                        newDest = (newDest) ? newDest : oldDest;
                                        newSrc = (newSrc) ? newSrc : oldSrc;
                                        intrinsicArgTypes.push_back(newDest->getType());
                                        intrinsicArgTypes.push_back(newSrc->getType());
                                        intrinsicArgTypes.push_back(mCpI->getLength()->getType());
                                        llvm::Function * newMemCpyF = llvm::Intrinsic::getDeclaration(pFunction->getParent(), 
                                                                                                      llvm::Intrinsic::memcpy, intrinsicArgTypes);
                                        for(unsigned int I = 0, E = mCpI->getNumArgOperands(); I != E; ++I)
                                        {
                                            llvm::Value * arg = mCpI->getArgOperand(I);
                                            arg = (arg == oldDest)?newDest:arg;
                                            arg = (arg == oldSrc)?newSrc:arg;
                                            newCallArgs.push_back(arg);
                                        }
                                        llvm::CallInst * newCallInst = llvm::IntrinsicInst::Create(newMemCpyF, newCallArgs, "", pInstr);
                                        llvm::MemCpyInst * newMCpI = llvm::cast<llvm::MemCpyInst>(newCallInst);
                                        std::string name = pInstr->getName();
                                        pInstr->eraseFromParent();
                                        newMCpI->setName(name);
                                    }
                                    break;
                                }
                                case llvm::Intrinsic::memset:
                                {
                                    llvm::MemSetInst * mSetI = llvm::cast<llvm::MemSetInst>(pInstr);
                                    llvm::Value * oldDest = mSetI->getRawDest();
                                    if(conformOperand(oldDest, space))
                                    {
                                        llvm::Value * newDest = getResolvedOperand(oldDest, space);
                                        assert(newDest);
                                        intrinsicArgTypes.push_back(newDest->getType());
                                        intrinsicArgTypes.push_back(mSetI->getLength()->getType());
                                        llvm::Function * newMemSetF = llvm::Intrinsic::getDeclaration(pFunction->getParent(), 
                                                                                                      llvm::Intrinsic::memset, intrinsicArgTypes);
                                        for(unsigned int I = 0, E = mSetI->getNumArgOperands(); I != E; ++I)
                                        {
                                            llvm::Value * arg = mSetI->getArgOperand(I);
                                            arg = (arg == oldDest)?newDest:arg;
                                            newCallArgs.push_back(arg);
                                        }
                                        llvm::CallInst * newCallInst = llvm::IntrinsicInst::Create(newMemSetF, newCallArgs, "", pInstr);
                                        llvm::MemSetInst * newMSetI = llvm::cast<llvm::MemSetInst>(newCallInst);
                                        std::string name = pInstr->getName();
                                        pInstr->eraseFromParent();
                                        newMSetI->setName(name);
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    retValue = true;
                    break;
                }

                default:
                    assert( 0 && "Unexpected instruction with generic address space pointer" );
                    break;
            }

            if( pNewInstr != NULL )
            {
                // Schedule the original instruction for replacement
                m_replaceMap.insert( TMapPair( pInstr, pNewInstr ) );
                m_replaceVector.push_back( TMapPair( pInstr, pNewInstr ) );
                retValue = true;
            }
        }
    }

    return retValue;
}

bool IGILGenericAddressStaticResolution::resolveInstructionTwoPointers( 
    Instruction*  pInstr, 
    unsigned int space ) 
{
    unsigned firstIdx = 0;
    bool retValue = false;
    Value*  Op = NULL;

    if( pInstr->getOpcode() == Instruction::ICmp )
    {
        Value*  Op0 = pInstr->getOperand( 0 );
        Value*  Op1 = pInstr->getOperand( 1 );

        bool  op0IsNull = isa<ConstantPointerNull>( Op0 );
        bool  op1IsNull = isa<ConstantPointerNull>( Op1 );

        // if both ptrs are null, there is no action
        if( op0IsNull && op1IsNull )
        {
            return retValue;
        }

        if( op0IsNull || op1IsNull )
        {
            Op = ( op0IsNull ? Op1 : Op0 );
        }
        else
        {
            Op = Op0;
        }
    }
    else
    {
        firstIdx = 1;
        Op = pInstr->getOperand( firstIdx );
    }

    if( Op != NULL )
    {
        const PointerType*  pSrcType = dyn_cast<PointerType>( Op->getType() );
        if( ( pSrcType != NULL ) &&
            ( pSrcType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) )
        {
            // At first - check available resolution(s) and complement them by NULL pointer if required
            Constant* pNull = ConstantPointerNull::get(
                PointerType::get( pSrcType->getElementType(), space ) );
            Value*  pVal[2] = { pNull, pNull };
            unsigned count = 0;

            for( unsigned idx = 0; idx < 2; idx++ ) 
            {
                Value*  pPrevValue = pInstr->getOperand( firstIdx + idx );
                Value*  pNewVal = getResolvedOperand( pPrevValue, space );

                if( pNewVal != NULL ) 
                {
                    pVal[idx] = pNewVal;
                    count++;
                }
            }

            if( count > 0 )
            {
                Instruction*  pNewInstr = NULL;

                // Then - generate the instruction on basis of input values above
                switch( pInstr->getOpcode() )
                {
                    case Instruction::Select: 
                    {
                        SelectInst* pSelectInstr = cast<SelectInst>( pInstr );
                        SelectInst* pNewSelect = SelectInst::Create(
                            pSelectInstr->getCondition(), 
                            pVal[0], 
                            pVal[1], 
                            pSelectInstr->getName(), 
                            pSelectInstr );
                        pNewInstr = pNewSelect;

                        // Fix-up pointer usages
                        fixUpPointerUsages( pNewSelect, pInstr );

                        break;
                    }

                    case Instruction::ICmp: 
                    {
                        ICmpInst* pIcmpInstr = cast<ICmpInst>( pInstr );
                        ICmpInst* pNewICmp = new ICmpInst(
                            pInstr, 
                            pIcmpInstr->getPredicate(), 
                            pVal[0], 
                            pVal[1],
                            pInstr->getName() );
                        pNewInstr = pNewICmp;

                        break;
                    }

                    default:
                      assert( 0 && "Unexpected instruction with generic address space pointer" );
                      break;
                }

                if( pNewInstr != NULL )
                {
                    // Schedule the original instruction for replacement
                    m_replaceMap.insert( TMapPair( pInstr, pNewInstr ) );
                    m_replaceVector.push_back( TMapPair( pInstr, pNewInstr) );
                    retValue = true;
                }
            }
        }
    }

    return retValue;
}

bool IGILGenericAddressStaticResolution::resolveInstructionPhiNode( 
    PHINode*  pPhiInstr, 
    unsigned int space ) 
{
    const PointerType*  pDestType = dyn_cast<PointerType>( pPhiInstr->getType() );
    bool retValue = false;

    if( ( pDestType != NULL ) &&
        ( pDestType->getAddressSpace() == ADDRESS_SPACE_GENERIC ) )
    {
        bool operandsConform = true;
        unsigned numIncoming = pPhiInstr->getNumIncomingValues();
        unsigned idx;

        // All address spaces on the incoming args must conform to space
        for( idx = 0; idx < numIncoming; idx++ )
        {
            Value*  pValue = pPhiInstr->getIncomingValue( idx );
            if( conformOperand( pValue, space ) == false )
            {
                operandsConform = false;
                break;
            }
        }

        if( operandsConform )
        { 
            // At first - create empty PHI node of required type
            PHINode*  pNewPHI = PHINode::Create(
                PointerType::get( pDestType->getElementType(), space ), 
                numIncoming, 
                pPhiInstr->getName(), 
                pPhiInstr );

            if( pNewPHI != NULL )
            {
                // Then - add all incoming edges which can be resolved
                for( idx = 0; idx < numIncoming; idx++ )
                {
                    Value*  pPrevValue = pPhiInstr->getIncomingValue( idx );
                    Value*  pNewVal = getResolvedOperand(pPrevValue, space );
                    if( pNewVal != NULL )
                    {
                        pNewPHI->addIncoming(
                            pNewVal,
                            pPhiInstr->getIncomingBlock( idx ));
                    }
                }

                if (numIncoming > 0)
                {
                    // Fix-up pointer usages
                    fixUpPointerUsages( pNewPHI, pPhiInstr );

                    // Schedule the original instruction for replacement
                    m_replaceMap.insert( TMapPair( pPhiInstr, pNewPHI ) );
                    m_replaceVector.push_back( TMapPair( pPhiInstr, pNewPHI ) );
                    retValue = true;
                }
            }
        }
    }

    return retValue;
}

 Value* IGILGenericAddressStaticResolution::resolveConstantExpression(
      Value*  pVal, 
      unsigned int space ) 
 {
    PointerType *pPtrType = dyn_cast<PointerType>( pVal->getType() );
    Value*  retValue = NULL;
    
    // Check that the operand produces a pointer
    if( pPtrType != NULL ) 
    {
        // If pointer is already of named addr-space type - nothing to do
        if( pPtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC )
        {
            ConstantExpr* pCE = dyn_cast<ConstantExpr>( pVal );

            // Check for constant expression
            if( pCE != NULL ) 
            {
                // Generate new constant expression with resolved pointer's address space
                switch( pCE->getOpcode() )
                {
                    case Instruction::IntToPtr:
                    {
                        // IntToPtr case: enforce target addr space type on the result
                        retValue = ConstantExpr::getIntToPtr( 
                            pCE->getOperand( 0 ), 
                            PointerType::get( pPtrType->getElementType(), space ) );

                        break;
                    }

                    case Instruction::AddrSpaceCast:
                    case Instruction::BitCast:
                    {
                        // Bitcast case: modify BitCast expression towards target addr space type
                        retValue = ConstantExpr::getPointerCast(
                            pCE->getOperand( 0 ), 
                            PointerType::get( pPtrType->getElementType(), space ) );

                        break;
                    }

                    case Instruction::GetElementPtr: 
                    {
                        // GEP case: modify Pointer operand to target addr space type and the constant result type
                        Value*  newValue = resolveConstantExpression( pCE->getOperand( 0 ), space );
                        if( newValue != NULL )
                        {
                            SmallVector<Constant*, 8> operands;
                            operands.push_back( cast<Constant>( newValue ) );
                            for (unsigned idx = 1; idx < pCE->getNumOperands(); idx++ )
                            {
                                operands.push_back( pCE->getOperand( idx ) );
                            }

                            retValue = pCE->getWithOperands(
                                operands, 
                                PointerType::get( pPtrType->getElementType(), space ) );
                        }

                        break;
                    }

                    case Instruction::Select: 
                    {
                        // Select case: modify Pointer operand to target addr space type and the constant result type
                        SmallVector<Constant*, 8> operands;
                        operands.push_back( pCE->getOperand( 0 ) );
                        for( unsigned idx = 1; idx < pCE->getNumOperands(); idx++ ) 
                        {
                            Value*  curValue = pCE->getOperand( idx );
                            Value*  newValue = resolveConstantExpression( curValue, space );
                            if( newValue != NULL )
                            {
                                operands.push_back( cast<Constant>( newValue ) );
                            }
                            else
                            {
                                operands.push_back( cast<Constant>( curValue ) );
                            }
                        }

                        retValue = pCE->getWithOperands(
                            operands, 
                            PointerType::get( pPtrType->getElementType(), space ) );

                        break;
                    }

                    default:
                        // A binary or bitwise expression cannot be reached here, because we enter the constant expression
                        // with pointer value, and then stop on IntToPtr and Bitcast (who are the only ones which could 
                        // lead to integer type involved)
                        assert( 0 && "Unexpected instruction with generic address space constant expression pointer" );
                }
            } 
        }
    }

    return retValue;
}

bool IGILGenericAddressStaticResolution::conformOperand(
    Value*  pVal, 
    unsigned int space ) 
{
    PointerType *pPtrType = dyn_cast<PointerType>( pVal->getType() );
    bool  retValue = false;
    
    // Check that the operand produces a pointer
    if( pPtrType != NULL ) 
    {
        // If pointer is already of named addr-space type - nothing to do
        if( pPtrType->getAddressSpace() == ADDRESS_SPACE_GENERIC )
        {
            ConstantExpr* pCE = dyn_cast<ConstantExpr>( pVal );

            // Check for constant expression
            if( pCE != NULL ) 
            {
                // Generate new constant expression with resolved pointer's address space
                switch( pCE->getOpcode() )
                {
                    case Instruction::PtrToInt:
                    {
                        // IntToPtr case: enforce target addr space type on the result
                        Value*  operand = pCE->getOperand( 0 );
                        PointerType *pPtrType = dyn_cast<PointerType>( operand->getType() );
                        if( ( pPtrType ) &&
                            ( pPtrType->getAddressSpace() == space ) )
                        {
                            retValue = true;
                        }

                        break;
                    }

                    case Instruction::IntToPtr:
                    {
                        retValue = true;
                        break;
                    }

                    case Instruction::AddrSpaceCast:
                    case Instruction::BitCast:
                    {
                        // Bitcast case: modify BitCast expression towards target addr space type
                        Value*  operand = pCE->getOperand( 0 );
                        PointerType *pPtrType = dyn_cast<PointerType>( operand->getType() );
                        if( ( pPtrType ) &&
                            ( pPtrType->getAddressSpace() == space ) )
                        {
                            retValue = true;
                        }

                        break;
                    }

                    case Instruction::GetElementPtr: 
                    case Instruction::Select:
                    {
                        // Check all gep/select operands for conformance
                        retValue = true;
                        for (unsigned idx = 0; idx < pCE->getNumOperands(); idx++ )
                        {
                            if( conformOperand( pCE->getOperand( idx ), space ) == false )
                            {
                                retValue = false;
                                break;
                            }
                        }

                        break;
                    }

                    default:
                        // A binary or bitwise expression cannot be reached here, because we enter the constant expression
                        // with pointer value, and then stop on IntToPtr and Bitcast (who are the only ones which could 
                        // lead to integer type involved)
                        assert( 0 && "Unexpected instruction with generic address space constant expression pointer" );
                }
            }
            else
            {
                // If there is no constant expression to map another address space, then the operation conforms
                retValue = true;
            }
        }
    }

    return retValue;
}

Value*  IGILGenericAddressStaticResolution::getReplacementForInstr( Instruction* pInstr )
{
    TReplaceMap::const_iterator mapping = m_replaceMap.find( pInstr );
    Value*  retValue = NULL;

    if( mapping != m_replaceMap.end() ) 
    {
        retValue = mapping->second;
    }

    return retValue;
}

Value*  IGILGenericAddressStaticResolution::getResolvedOperand(
    Value*  pOperand, 
    unsigned int space ) 
{
    // At first - try to find a replacement for the operand
    Value* pResolvedValue = getReplacementForInstr( dyn_cast<Instruction>( pOperand ) );

    if( pResolvedValue == NULL )
    {
        // Then - check for ConstantExpr case for resolution
        pResolvedValue = resolveConstantExpression( pOperand, space );
    }

    // note: both calculations are allowed to fail, so NULL is an acceptable return value
    return pResolvedValue;
}

void IGILGenericAddressStaticResolution::fixUpPointerUsages(
      Instruction*  pNewInstr, 
      Instruction*  pOldInstr ) 
{
    Value::user_iterator use_it, use_end;

    // Iterate through usages of original instruction
    for( use_it = pOldInstr->user_begin(), 
         use_end = pOldInstr->user_end();
         use_it != use_end; use_it++ )
    {
        Instruction*  pUse = dyn_cast<Instruction>( *use_it );
        if( pUse != NULL )
        {
            // Check whether we should update this usage
            TPointerMap::const_iterator ptr_it = m_GASEstimate.find( pUse );
            if( ptr_it != m_GASEstimate.end() ) 
            {
                // Check whether we have to induce bitcast to GAS pointer
                if( ptr_it->second == ADDRESS_SPACE_GENERIC ) 
                {
                    const PointerType*  pSrcType = cast<PointerType>( pNewInstr->getType() );
                    // Induce conversion from named type to generic one
                    BitCastInst*  pInducedBitcast = new BitCastInst(
                        pNewInstr, 
                        PointerType::get(
                            pSrcType->getElementType(), 
                            ADDRESS_SPACE_GENERIC ),
                        pOldInstr->getName(), 
                        pOldInstr );

                    // Replace uses of original instruction with bitcast
                    pUse->replaceUsesOfWith( pOldInstr, pInducedBitcast );        
                } 
                else
                {
                    Value*  repVal = getReplacementForInstr( pUse );

                    // Check whether the use's replacement is already created
                    // (i.e., there are several GAS pointers as operands)
                    if( repVal != NULL ) 
                    {
                        Instruction*  repInstr = dyn_cast<Instruction>( repVal );
                        if( repInstr != NULL )
                        {
                            // Update the use replacement if it is: PHI, Select, Icmp, Call
                            switch( repInstr->getOpcode() ) 
                            {
                                case Instruction::PHI: 
                                {
                                    // For PHI - just add new edge
                                    PHINode*  PhiInstr = cast<PHINode>( repInstr );
                                    PhiInstr->addIncoming( pNewInstr, pNewInstr->getParent() );
                                    break;
                                }

                                case Instruction::Select:
                                case Instruction::ICmp:
                                {
                                    // For Select, Icmp, Call - update corresponding operand(s)
                                    assert( ( pUse->getNumOperands() == repInstr->getNumOperands() ) &&
                                            "Replacement info is broken!" );

                                    for( unsigned idx = 0; idx < pUse->getNumOperands(); idx++) 
                                    {
                                        if( pUse->getOperand(idx) == pOldInstr ) 
                                        {
                                            repInstr->setOperand( idx, pNewInstr );
                                        }
                                    }
                                    break;
                                }

                                default:
                                  assert( 0 && "Unexpected instruction with generic address space pointer" );
                                  break;
                            }
                        }
                    } // check multi-pointer operand case
                } // check induced bitcast case
            }  // check if this usage need to be updated
        } // check for valid use
    }  // iteration through usages
}

bool IGILGenericAddressStaticResolution::isAllocaStructGASPointer(
    const Type* pType, 
    bool isStructDetected ) 
{
    bool retValue = false;

    if( pType->isStructTy() ) 
    {
        // Look into the structure fields for arrays, structs and primitive types of GAS pointers
        for( unsigned idx = 0; idx < pType->getStructNumElements(); idx++ ) 
        {
            const Type *pElemType = pType->getStructElementType( idx );
            if( pElemType != NULL )
            {
                if( pElemType->isStructTy() && 
                    isAllocaStructGASPointer( pElemType, true ) ) 
                {
                    retValue = true;
                } 
                else if( pElemType->isArrayTy() ) 
                {
                    const Type *pArrayElemType = pElemType->getArrayElementType();

                    if( pArrayElemType->isAggregateType() && 
                        isAllocaStructGASPointer(pArrayElemType, true ) ) 
                    {
                        retValue = true;
                    } 
                    else if( pArrayElemType->isPointerTy() &&
                             ( cast<const PointerType>( pArrayElemType)->getAddressSpace() == ADDRESS_SPACE_GENERIC ) )
                    {
                        retValue = true;
                    }
                } 
                else if( pElemType->isPointerTy() && 
                         ( cast<const PointerType>(pElemType)->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) 
                {
                    retValue = true;
                }
            }
        }
    } 
    else if( pType->isArrayTy() ) 
    {
        const Type *pArrayElemType = pType->getArrayElementType();

        // Look into the array element for structs of GAS pointers
        if( pArrayElemType->isAggregateType() && 
            isAllocaStructGASPointer( pArrayElemType, isStructDetected ) ) 
        {
            retValue = true;
        } 
        else if( isStructDetected && pArrayElemType->isPointerTy() &&
                 ( cast<const PointerType>( pArrayElemType )->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) 
        {
            retValue = true;
        }
    } 
    else if( isStructDetected && 
             pType->isPointerTy() &&
             ( cast<const PointerType>( pType )->getAddressSpace() == ADDRESS_SPACE_GENERIC ) ) 
    {
        retValue = true;
    }

    return retValue;
}

char IGILGenericAddressStaticResolution::ID = 0;

}
