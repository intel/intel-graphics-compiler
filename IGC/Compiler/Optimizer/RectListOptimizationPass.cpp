/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RectListOptimizationPass.hpp"
#include "IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "ShaderTypesEnum.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/Casting.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/IGCIRBuilder.h"
#include "common/igc_regkeys.hpp"
#include <vector>
#include <unordered_map>
#include <set>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

/************************************************************************
This transformation is not safe in general. It can be applied only in those case:
-We know that the resouce is MCS compressed
-We need to know that we don't access out of bound sample index
************************************************************************/

class RectListOptimizationPass : public FunctionPass
{
    static const unsigned int m_NUM_OUTPUT_VERT_FOR_RECTLIST = 4;

    typedef unsigned int attribute_idx;
    typedef unsigned int vertex_idx;
    typedef unsigned int channel_idx;

    //Data structure related to Each attribute mapping to rectangle values
    typedef std::unordered_map<Value*, std::set<vertex_idx>> coordToVertMap;
    typedef std::unordered_map<channel_idx, std::set<Value*>> channelToCoordMap;

    typedef struct _ATTRIB_SOURCELIST_STRUCT {
        ShaderOutputType outType;
        coordToVertMap rectCoordToVertM;
        channelToCoordMap channelToCoordM;
    } ATTRIB_SOURCELIST_ST;
    typedef std::unordered_map<attribute_idx, ATTRIB_SOURCELIST_ST> atrribRectListMap;
    atrribRectListMap m_rectListPerAttrib; //rectlist info per attribute

    //GS ouput instructions grouped together by all attributes
    //Map => Vertex Idx ---> <attribute_idx, output_gs instruction>
    typedef std::unordered_map<attribute_idx, Instruction*> attributeInstMap;
    typedef std::vector<attributeInstMap> vetexAttribVec;
    vetexAttribVec m_gsouputM;


    // GenIS_OUTPUTGS intrinsics argument indexes. These are constants
    // If the API signature changes, these indices will have to change
    static const unsigned int m_OUTPUTGS_shaderTypeArgIdx = 4;
    static const unsigned int m_OUTPUTGS_shaderAttrArgIdx = 5;
    static const unsigned int m_OUTPUTGS_EmitCountArgIdx = 6;


    //Helper Methods
    bool isGSTriStripPrimTopology(Function& F);
    bool isGSOutputVertValid(Function& F);
    bool scanAndInserOutputGSList(Function& F);
    void analyzeForRectList(Function& F, IGC::GeometryShaderContext* pgsctx);
    bool isRectCoords(std::set<Value*>& X, std::set<Value*>& Y, coordToVertMap& rectCoordToVertM);
    bool isConstantCoords(std::set<Value*>& ChannelValues, coordToVertMap& rectCoordToVertM);
public:
    static char ID;

    RectListOptimizationPass() :
        FunctionPass(ID)
    {
        m_gsouputM.clear();
        m_gsouputM.resize(m_NUM_OUTPUT_VERT_FOR_RECTLIST);
    }

    virtual bool runOnFunction(llvm::Function& F) override;
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }
    virtual llvm::StringRef getPassName() const override { return "RectListOptimization"; }
    void visitCallInst(llvm::CallInst& I);


};

char RectListOptimizationPass::ID = 0;

//helper methods
bool RectListOptimizationPass::isGSTriStripPrimTopology(Function& F)
{
    auto pPrimTopology = F.getParent()->getGlobalVariable("GsOutputPrimitiveTopology");
    const unsigned int primitiveTopology = static_cast<unsigned int>(
        (llvm::cast<llvm::ConstantInt>(pPrimTopology->getInitializer())->getZExtValue()));
    return (primitiveTopology == USC::GFX3DPRIM_TRISTRIP);
}

bool RectListOptimizationPass::isGSOutputVertValid(Function& F)
{
    auto pMaxOutputVertices = F.getParent()->getGlobalVariable("GsMaxOutputVertices");
    IGC_ASSERT_MESSAGE(pMaxOutputVertices != nullptr, "GsMaxOutputVertices must be defined");
    const unsigned int maxOutputVertices = static_cast<unsigned int>(
        (llvm::cast<llvm::ConstantInt>(pMaxOutputVertices->getInitializer())->getZExtValue()));
    return (maxOutputVertices == m_NUM_OUTPUT_VERT_FOR_RECTLIST);
}


//We are looking for all GenISA.OUTPUTGS instructions. These instructions are clubbed together for each output
//vertex. This function is trying to create that map for further analysis
//Map we are creating below is
// Vertex ID [0-4]--->
//                    Attribute 0 ---> OutputGS Inst 0
//                    Attribute 1 ---> OutputGS Inst 1
//                    .........
bool RectListOptimizationPass::scanAndInserOutputGSList(Function& F) {
    bool foundValidMapping = true;
    typedef std::unordered_map<Value*, vertex_idx > emitCntToVidMap;
    emitCntToVidMap emitCntToVidM;

    int vertexIdx = 0;
    for (auto I = F.begin(), E = F.end(); I != E && foundValidMapping; ++I) {
        llvm::BasicBlock::InstListType& instList = I->getInstList();
        for (auto instIter = instList.begin(), instIterEnd = instList.end();
            instIter != instIterEnd && foundValidMapping;
            ++instIter)
        {
            llvm::Instruction* inst = &(*instIter);
            if (GenIntrinsicInst * CI = dyn_cast<GenIntrinsicInst>(inst)) {
                if (CI->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_OUTPUTGS) {
                    ShaderOutputType outType = static_cast<ShaderOutputType>(
                        llvm::cast<llvm::ConstantInt>(inst->getOperand(m_OUTPUTGS_shaderTypeArgIdx))->getZExtValue());

                    unsigned int attrIdx =
                        static_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(inst->getOperand(m_OUTPUTGS_shaderAttrArgIdx))->getZExtValue());
                    Value* emitCntV = inst->getOperand(m_OUTPUTGS_EmitCountArgIdx);
                    ConstantInt* CI = dyn_cast<ConstantInt>(emitCntV);
                    // We are taking a conservative estimate here to handle a simple case
                    // We expect the emits to be of constant count ranging from 0-3
                    if (!CI ||
                        (unsigned int)CI->getZExtValue() >= m_NUM_OUTPUT_VERT_FOR_RECTLIST) {

                        foundValidMapping = false;
                        break;
                    }
                    emitCntToVidMap::iterator emitToVidit = emitCntToVidM.find(emitCntV);
                    if (emitToVidit == emitCntToVidM.end()) {
                        emitCntToVidM.insert(std::make_pair(emitCntV, vertexIdx));
                        vertexIdx++;
                    }

                    attributeInstMap::iterator attrInstMit = m_gsouputM[emitCntToVidM[emitCntV]].find(attrIdx);
                    if (attrInstMit != m_gsouputM[emitCntToVidM[emitCntV]].end()) {
                        if (outType != SHADER_OUTPUT_TYPE_DEFAULT) {
                            //generally for each SGV we have two OUTPUTGS instructions.
                            //always try to pick the one that has the outType indicating what SGV it is
                            attrInstMit->second = inst;
                        }
                    }
                    else {
                        m_gsouputM[emitCntToVidM[emitCntV]].insert(std::make_pair(attrIdx, inst));
                    }
                }
            }
        }
    }

    //Although max possible out vertices is 4, but may emit less then 4
    //Verify that we have 1:1 mapping with 4 vertices, that means we have same number of attribtues
    //If we have possibility of emitting less then 4, we must return false
    unsigned numattributes = m_gsouputM[0].size();
    for (unsigned int i = 1; i < m_NUM_OUTPUT_VERT_FOR_RECTLIST && foundValidMapping; i++) {
        if ((m_gsouputM[i].size() != numattributes) || m_gsouputM[i].size() == 0) {
            foundValidMapping = false;
            break;
        }
    }
    return foundValidMapping;
}

// Method to detect Rectangle Coordinates
bool RectListOptimizationPass::isRectCoords(
    std::set<Value*>& XChannel,
    std::set<Value*>& YChannel,
    coordToVertMap& rectCoordToVertM
)
{
    bool isRectCoords = true;

    // V1=(X1,Y1)            V2=(X2,Y1)
    // ------------------------
    // |                      |
    // |                      |
    // |                      |
    // |                      |
    // ------------------------
    // V3=(X1,Y2)            V4=(X2,Y2)
    //
    //Condition 1: As you can see there must be 2 unique X's (X1 and X2) for Channel X
    // and (Y1, Y2) for channel Y
    //Condition 2:Similay each X and Y Value must be associated wiht 2 unique vertices
    // In this case X1 -> (V1 and V3), X2 -> (V2, V4)
    // Y1 -> (V1 and V2) and Y2 -> (V3 and V4)
    // These two conditions are sufficient to declare if this is rectangle.
    // Because there are only 4 vertices and each are unique.
    do {
        //Validation of Condition 1 for both X and Y Channels
        if (XChannel.size() != 2 || YChannel.size() != 2) {
            //Doesnt satisfy condition 1
            isRectCoords = false;
            break;
        }

        //Validation of condition 2 for both Y and X Channel
        std::vector<std::set<Value*>*> tempArr = { &XChannel, &YChannel };
        for (unsigned i = 0; i < tempArr.size() && isRectCoords; i++) {
            for (std::set<Value*>::iterator it = tempArr[i]->begin(); it != tempArr[i]->end(); it++) {
                Value* V = *it;
                coordToVertMap::iterator coordToVertIt = rectCoordToVertM.find(V);
                if (coordToVertIt == rectCoordToVertM.end()) {
                    IGC_ASSERT_MESSAGE(0, "Not able to find one of the XChannel Values");
                    isRectCoords = false;
                    break;
                }
                if (coordToVertIt->second.size() != 2) {
                    //Violates condition 2 to for V
                    isRectCoords = false;
                    break;
                }
            }
        }
    } while (false);

    return isRectCoords;
}

//Check if for given Channel all values are same
//Condition 1: Each Channel should have 1 associated value
//Condition 2: That Value should have 4 vertices mapped to it from rectCoordToVertM
//This means for this channel all vertices use the same value.
bool RectListOptimizationPass::isConstantCoords(
    std::set<Value*>& ChannelValues,
    coordToVertMap& rectCoordToVertM)
{
    bool isConstantCoord = false;

    do {
        if (ChannelValues.size() != 1) {
            //fails condition 1
            break;
        }
        Value* uniqueValue = *ChannelValues.begin();
        coordToVertMap::iterator it = rectCoordToVertM.find(uniqueValue);
        if (it == rectCoordToVertM.end()) {
            IGC_ASSERT_MESSAGE(0, "Not able to find the value in the unique value in Vale to vertex List map");
            break;
        }
        if (it->second.size() != m_NUM_OUTPUT_VERT_FOR_RECTLIST) {
            //fails condition 2
            break;
        }
        isConstantCoord = true;
    } while (false);

    return isConstantCoord;
}

void RectListOptimizationPass::analyzeForRectList(Function& F, GeometryShaderContext* pgsctx)
{
    //Now we expand each attribute to Map
    //1. each Value to correspoding list of vertices (0-4)
    //2. Each channel (X,Y,Z,W) to list of Values
    for (vertex_idx vid = 0; vid < m_NUM_OUTPUT_VERT_FOR_RECTLIST; vid++) {
        for (attributeInstMap::iterator it = m_gsouputM[vid].begin(); it != m_gsouputM[vid].end(); it++) {
            //For each attribute we see corresponding instruction
            Instruction* inst = it->second;
            atrribRectListMap::iterator arit = m_rectListPerAttrib.find(it->first);
            if (arit == m_rectListPerAttrib.end()) {
                ATTRIB_SOURCELIST_ST rectList;
                rectList.outType = static_cast<ShaderOutputType>(
                    llvm::cast<llvm::ConstantInt>(inst->getOperand(m_OUTPUTGS_shaderTypeArgIdx))->getZExtValue());
                m_rectListPerAttrib.insert(std::make_pair(it->first, rectList));
                arit = m_rectListPerAttrib.find(it->first);
            }
            for (unsigned int ch_id = 0; ch_id < IGC::NUM_SHADER_CHANNELS; ch_id++) {
                Value* channelV = inst->getOperand(ch_id);
                channelToCoordMap::iterator chtocoordit = arit->second.channelToCoordM.find(ch_id);
                if (chtocoordit != arit->second.channelToCoordM.end()) {
                    chtocoordit->second.insert(channelV);
                }
                else {
                    std::set<Value*> tempChV;
                    tempChV.insert(channelV);
                    arit->second.channelToCoordM.insert(std::make_pair(ch_id, tempChV));
                }
                coordToVertMap::iterator coordToVit = arit->second.rectCoordToVertM.find(channelV);
                if (coordToVit != arit->second.rectCoordToVertM.end()) {
                    coordToVit->second.insert(vid);
                }
                else {
                    std::set<vertex_idx> tempVidList;
                    tempVidList.insert(vid);
                    arit->second.rectCoordToVertM.insert(std::make_pair(channelV, tempVidList));
                }
            }
        }
    }

    //Now there can be only two possibilities for this to be a rectlist
    //a. For SV_POSITION it must be a rectangle for XY Channel and constant for ZY channel
    //b. For any other attribute it must be either be a rectangle or constant for XY Channel
    //   And ZY channel must be constant
    bool bPositionFound = false;
    bool isRectList = false;
    IGC_ASSERT_MESSAGE(m_rectListPerAttrib.size() != 0, "Empty attribute list, should not happen\n");
    for (atrribRectListMap::iterator it = m_rectListPerAttrib.begin(); it != m_rectListPerAttrib.end(); it++) {
        ATTRIB_SOURCELIST_ST* pAttrSrcList = &it->second;
        channelToCoordMap::iterator ctCitXit = pAttrSrcList->channelToCoordM.find(IGC::SHADER_CHANNEL_X);
        channelToCoordMap::iterator ctCitYit = pAttrSrcList->channelToCoordM.find(IGC::SHADER_CHANNEL_Y);
        channelToCoordMap::iterator ctCitZit = pAttrSrcList->channelToCoordM.find(IGC::SHADER_CHANNEL_Z);
        channelToCoordMap::iterator ctCitWit = pAttrSrcList->channelToCoordM.find(IGC::SHADER_CHANNEL_W);
        //reset the rectList to false for each iteration
        isRectList = false;

        if (ctCitXit == pAttrSrcList->channelToCoordM.end() ||
            ctCitYit == pAttrSrcList->channelToCoordM.end() ||
            ctCitZit == pAttrSrcList->channelToCoordM.end() ||
            ctCitWit == pAttrSrcList->channelToCoordM.end())
        {
            IGC_ASSERT_MESSAGE(0, "Not able to find one of the Channel X/Y/Z/W!!!");
            break;
        }

        //Need to handle POSITION specially, because RECTLIST relies on that
        if (pAttrSrcList->outType == SHADER_OUTPUT_TYPE_POSITION) {
            bPositionFound = true;
        }
        // Check for each attribute if it is RECT_LIST
        bool bisRectCoords = isRectCoords(ctCitXit->second, ctCitYit->second, pAttrSrcList->rectCoordToVertM);
        if (!bisRectCoords)
        {
            if (pAttrSrcList->outType == SHADER_OUTPUT_TYPE_POSITION)
            {
                //for POSITION its a must have to be a rect
                break;
            }

            //if constant must be constant for both X and Y
            if (isConstantCoords(ctCitXit->second, pAttrSrcList->rectCoordToVertM) == false)
            {
                break;
            }
            if (isConstantCoords(ctCitYit->second, pAttrSrcList->rectCoordToVertM) == false)
            {
                break;
            }
        }

        //For all attributes both Z and W must always be constant
        if (isConstantCoords(ctCitZit->second, pAttrSrcList->rectCoordToVertM) == false)
        {
            break;
        }
        if (isConstantCoords(ctCitWit->second, pAttrSrcList->rectCoordToVertM) == false)
        {
            //violates condition 2
            break;
        }
        //Set rectList to true if passed all cases
        isRectList = true;
    }

    //Right now just setup the hint for driver to turn this ON whenever needed
    //TBD : Need to check if this needs to be filled instead in CGeometryShader::FillProgram
    pgsctx->programOutput.m_bCanEnableRectList = (bPositionFound == true) ? isRectList : false;
    return;
}

bool RectListOptimizationPass::runOnFunction(Function& F)
{
    bool changed = false;
    do {
        if (IGC_IS_FLAG_ENABLED(DisableRectListOpt))
        {
            break;
        }

        IGCMD::MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        {
            break;
        }
        IGC::CodeGenContext* ctx = getAnalysis<IGC::CodeGenContextWrapper>().getCodeGenContext();
        if (!ctx || ctx->type != ShaderType::GEOMETRY_SHADER) {
            IGC_ASSERT_MESSAGE(0, "Failed to get the code gen context or not a Geometry shader\n");
            break;
        }
        IGC::GeometryShaderContext* pgsctx = static_cast <IGC::GeometryShaderContext*>(ctx);
        if (!pgsctx) {
            IGC_ASSERT_MESSAGE(0, "Failed to get the GS code gen context \n");
            break;
        }

        //First we need to detect if the shader has valid conditions to pursue
      //Check if number of vertices are 4
        if (!isGSOutputVertValid(F))
            break;
        //Create a map of Vertex to (attribute, instruction)
        if (!scanAndInserOutputGSList(F))
            break;

        //analyze for rectangle List pattern
        analyzeForRectList(F, pgsctx);

        //TBD: Add option to eleminate 4th vertex if needed

    } while (false);

    return changed;
}

void RectListOptimizationPass::visitCallInst(llvm::CallInst& I)
{
    return;
}


namespace IGC {
// Identifies rectangle from 4 vertices and replaces TOPOLOGY to RECTLIST
#define PASS_FLAG "igc-rectlist-opt"
#define PASS_DESCRIPTION "This is an optimization pass for inserting RECTLIST topology "
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
    IGC_INITIALIZE_PASS_BEGIN(RectListOptimizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_END(RectListOptimizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        llvm::Pass* createRectListOptimizationPass()
    {
        return new RectListOptimizationPass();
    }
}
