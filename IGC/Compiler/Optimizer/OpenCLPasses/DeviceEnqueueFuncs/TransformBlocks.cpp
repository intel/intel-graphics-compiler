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
//                                                                            //
// This file implements OCL execution model for IGIL target. Gets rid of the  //
// function pointers, transform the layout for block invoker kernels          //
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "TransformBlocks"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/IR/Argument.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/Attributes.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/ValueHandle.h"
#include "llvmWrapper/Transforms/Utils.h"

#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DIBuilder.h"
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>
#include <map>
#include <unordered_set>

using namespace llvm;
using namespace IGC;

STATISTIC(TransformBlockEnqueCounter, "Counts number of functions greeted");

// Command line option to dump the output .ll from this pass
static cl::opt<bool>
TransformBlocksOutput("TransformBlocksOutput", cl::desc("Output the .llvm generated from TransformBlocks pass"), cl::init(false));

namespace //Anonymous
{
    typedef uint32_t GEPIndex;
    class CallHandler;
    class Dispatcher;
    class DeviceEnqueueParamValue;
    class DataContext;

    class FuncCompare
    {
    public:
        bool operator() (const llvm::Function* a, const llvm::Function* b) const
        {
            return a->getName().compare(b->getName()) < 0;
        }
    };

    enum BlockElements : GEPIndex
    {
        BLOCK_INDEX_ISA,
        BLOCK_INDEX_FLAGS,
        BLOCK_INDEX_RESERVED,
        BLOCK_INDEX_INVOKE_FUNC,
        BLOCK_INDEX_DESCRIPTOR,
        BLOCK_CAPTURED_INDEX_BEGIN
    };

    enum class CaptureKind
    {
        SCALAR,
        POINTER,
        IMAGE,
        SAMPLER
    };

    enum class DeviceEnqueueFunction {
        ENQUEUE_KERNEL,
        ENQUEUE_KERNEL_BASIC,
        ENQUEUE_KERNEL_VAARGS,
        ENQUEUE_KERNEL_EVENTS_VAARGS,
        WORK_GROUP_SIZE_IMPL,
        PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
        PREFERRED_WORK_GROUP_MULTIPLE_IMPL,
        MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
        SUB_GROUP_COUNT_FOR_NDRANGE,
        SPIRV_ENQUEUE_KERNEL,
        SPIRV_SUB_GROUP_COUNT_FOR_NDRANGE,
        SPIRV_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
        SPIRV_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
        SPIRV_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
        SPIRV_MAX_NUM_SUB_GROUPS,
        NUM_FUNCTIONS_WITH_BLOCK_ARGS
    };
    
    const std::map<DeviceEnqueueFunction, const char*>  DeviceEnqueueFunctionNames = {
        { DeviceEnqueueFunction::ENQUEUE_KERNEL, "_Z14enqueue_kernel" },
        { DeviceEnqueueFunction::ENQUEUE_KERNEL_BASIC, "__enqueue_kernel_basic" },
        { DeviceEnqueueFunction::ENQUEUE_KERNEL_VAARGS, "__enqueue_kernel_vaargs" },
        { DeviceEnqueueFunction::ENQUEUE_KERNEL_EVENTS_VAARGS, "__enqueue_kernel_events_vaargs" },
        { DeviceEnqueueFunction::WORK_GROUP_SIZE_IMPL, "__get_kernel_work_group_size_impl" },
        { DeviceEnqueueFunction::PREFERRED_WORK_GROUP_SIZE_MULTIPLE, "_Z45get_kernel_preferred_work_group_size_multiple" },
        { DeviceEnqueueFunction::PREFERRED_WORK_GROUP_MULTIPLE_IMPL, "__get_kernel_preferred_work_group_multiple_impl" },
        { DeviceEnqueueFunction::MAX_SUB_GROUP_SIZE_FOR_NDRANGE, "_Z41get_kernel_max_sub_group_size_for_ndrange" },
        { DeviceEnqueueFunction::SUB_GROUP_COUNT_FOR_NDRANGE, "_Z38get_kernel_sub_group_count_for_ndrange" },
        { DeviceEnqueueFunction::SPIRV_ENQUEUE_KERNEL, "__builtin_spirv_OpEnqueueKernel" },
        { DeviceEnqueueFunction::SPIRV_SUB_GROUP_COUNT_FOR_NDRANGE, "__builtin_spirv_OpGetKernelNDrangeSubGroupCount" },
        { DeviceEnqueueFunction::SPIRV_MAX_SUB_GROUP_SIZE_FOR_NDRANGE, "__builtin_spirv_OpGetKernelNDrangeMaxSubGroupSize" },
        { DeviceEnqueueFunction::SPIRV_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, "__builtin_spirv_OpGetKernelPreferredWorkGroupSizeMultiple" },
        { DeviceEnqueueFunction::SPIRV_LOCAL_SIZE_FOR_SUB_GROUP_COUNT, "__builtin_spirv_OpGetKernelLocalSizeForSubgroupCount" },
        { DeviceEnqueueFunction::SPIRV_MAX_NUM_SUB_GROUPS, "__builtin_spirv_OpGetKernelMaxNumSubgroups" }
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////
    /// helper class to build and query llvm metadata of kernel/dispatcher
    /////////////////////////////////////////////////////////////////////////////////////////////////
    class MetadataBuilder
    {
        IGC::IGCMD::MetaDataUtils* _pMdUtils;
        IGC::ModuleMetaData* modMD;
        DataContext& _dataContext;

    public:
        MetadataBuilder(IGC::IGCMD::MetaDataUtils* pMdUtils, DataContext& dataContext, IGC::ModuleMetaData* moduleMD)
            : modMD(moduleMD), _pMdUtils(pMdUtils), _dataContext(dataContext) {}

        /// Return function metadata if function is kernel
        IGC::FunctionMetaData* getKernelMetadata(const llvm::Function* func)
        {
            if (isEntryFunc(_pMdUtils, func))
            {
                if (modMD->FuncMD.find(const_cast<llvm::Function*>(func)) != modMD->FuncMD.end())
                    return &modMD->FuncMD[const_cast<llvm::Function*>(func)];
            }
            return nullptr;
        }

        bool eraseKernelMetadata(llvm::Function* func, IGC::ModuleMetaData* modMD) {
            bool changed = false;
            auto it = _pMdUtils->findFunctionsInfoItem(func);
            if (it != _pMdUtils->end_FunctionsInfo()) {
                _pMdUtils->eraseFunctionsInfoItem(it);
                _pMdUtils->save(func->getContext());
                changed = true;
            }
            if (modMD->FuncMD.find(func) != modMD->FuncMD.end()) {
                modMD->FuncMD.erase(func);
                changed = true;
            }
            return changed;
        }

        /// Return kernel argument type name from metadata
        std::string getKernelArgTypeName(const llvm::Function* func, unsigned argNum)
        {
            auto funcInfoMD = getOrEmitKernelMetadata(const_cast<llvm::Function*>(func));
            assert((funcInfoMD != nullptr) && "cannot get or emit for kernel metadata");
            assert(funcInfoMD->m_OpenCLArgBaseTypes.size() > (unsigned) argNum);
            return funcInfoMD->m_OpenCLArgBaseTypes[argNum];
        }

        /// Return function metadata if function is kernel, emit new kernel metadata if not.
        FunctionMetaData* getOrEmitKernelMetadata(const llvm::Function* func)
        {
            auto kernelMD = getKernelMetadata(func);
            if (kernelMD == nullptr)
            {
                EmitKernelMetadata(const_cast<llvm::Function*>(func));
                kernelMD = getKernelMetadata(func);
            }
            return kernelMD;
        }

        void EmitDeviceEnqueueMetadataPair(const llvm::Function* first, const llvm::Function* second)
        {
            auto module = first->getParent();
            auto& context = first->getContext();
            auto val = const_cast<llvm::Module*>(module)->getOrInsertNamedMetadata("igc.device.enqueue");
            Metadata* Elts[] = {
                MDString::get(context, first->getName()),
                MDString::get(context, second->getName())
            };

            MDNode* md = MDNode::get(context, Elts);
            val->addOperand(md);
        }

    private:
        /////////////////////////////////////////////////////////////////////////////////////////////////
        //  Function responsible for emitting special meta data for block dispatcher functions         //
        //  this will be part of stadard meta data layout and will be consumed by the Target           //
        /////////////////////////////////////////////////////////////////////////////////////////////////
        void EmitKernelMetadata(const llvm::Function* kernelFunc);
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////
    /// helper class to check value category
    /////////////////////////////////////////////////////////////////////////////////////////////////
    class KindQuery
    {
        DataContext& _dataContext;
    public:
        KindQuery(DataContext& dataContext) : _dataContext(dataContext) {}

        /// Resolve StructType or return nullptr
        static inline const llvm::StructType* toStructType(const llvm::Type* type)
        {
            if (type->isStructTy()) return cast<llvm::StructType>(type);

            auto ptrType = dyn_cast_or_null<llvm::PointerType>(type);
            return ptrType != nullptr
                ? dyn_cast<llvm::StructType>(ptrType->getElementType())
                : nullptr;
        }

        /// Check if type is OCL "ndrange_t"
        static bool isNDRangeType(const llvm::Type* type)
        {
            // ndrange_t is the struct type
            if (auto sType = toStructType(type))
            {
                return sType->getName().endswith(".ndrange_t");
            }
            return false;
        }

        /// Check if type is OCL image type
        static bool isImageType(const llvm::Type* type, llvm::SmallVectorImpl<llvm::StringRef> *nameFractions = nullptr)
        {
            if (auto *sType = toStructType(type))
            {
                if (sType->isOpaque())
                {
                    auto typeName = sType->getName();
                    llvm::SmallVector<llvm::StringRef, 3> buf;
                    auto& tokens = nameFractions != nullptr ? *nameFractions : buf;
                    typeName.split(tokens, ".");
                    return tokens.size() >= 2 && tokens[0].equals("opencl") && tokens[1].startswith("image") && tokens[1].endswith("_t");
                }
            }
            return false;
        }

        /// Check if type is StructType
        /// if "name" is set, check struct name
        static bool isStructType(llvm::Type* type, llvm::StringRef name = llvm::StringRef())
        {
            if (auto structType = toStructType(type))
            {
                if (!structType->isOpaque())
                {
                    return name.empty() || (!structType->isLiteral() && structType->getName().equals(name));
                }
            }
            return false;
        }

        /// Check if type is OCL event_t
        static bool isEventType(llvm::Type * type)
        {
            if (auto *sType = toStructType(type))
            {
                if (sType->isOpaque())
                {
                    auto typeName = sType->getName();
                    return typeName.equals("opencl.clk_event_t") || typeName.equals("opencl.event_t");
                }
            }
            return false;
        }

        /// Check if type is OCL queue_t
        static bool isQueueType(llvm::Type* type)
        {
            if (auto *sType = toStructType(type))
            {
                if (sType->isOpaque())
                {
                    auto typeName = sType->getName();
                    return typeName.equals("opencl.queue_t");
                }
            }
            return false;
        }

        /// Check if type is matcher Objective-C block type
        static bool isBlockStructType(llvm::Type* type)
        {
            if (auto structType = toStructType(type))
            {
                return (!structType->isOpaque())
                    && (structType->getNumElements() > BLOCK_INDEX_DESCRIPTOR)
                    && (structType->getElementType(BLOCK_INDEX_ISA)->isPointerTy())
                    && (structType->getElementType(BLOCK_INDEX_FLAGS)->isIntegerTy(32))
                    && (structType->getElementType(BLOCK_INDEX_RESERVED)->isIntegerTy(32))
                    && (structType->getElementType(BLOCK_INDEX_INVOKE_FUNC)->isPointerTy())
                    && (isStructType(structType->getElementType(BLOCK_INDEX_DESCRIPTOR), "struct.__block_descriptor"));
            }
            return false;
        }

        /// Lookup in metadata if the function arg is a "sampler_t"
        bool isSamplerArg(const llvm::Argument* arg) const;

        /// Check if dispatcher's capture argument has to be a "sampler_t"
        bool isSampleCaptured(const llvm::Function* dispatchFunc, unsigned captureNum) const;

        // IsKernel(function* func) - checks if the function is a OCL kernel
        bool isKernel(const llvm::Function* func) const;
    };

    //////////////////////////////////////////////////////////////////////////
    /// Abstract class handles a value of type struct
    /// Allows to get a value stored in a struct element
    //////////////////////////////////////////////////////////////////////////
    class StructValue
    {
    protected:
        llvm::Value* _structValue;
        llvm::StructType* _structType;
        explicit StructValue(llvm::Value* value) : _structValue(value)
        {
            assert(value);
            llvm::Type* valType = value->getType();
            while (valType->isPointerTy())
            {
                valType = valType->getPointerElementType();
            }
            _structType = dyn_cast<llvm::StructType>(valType);
        }

    public:
        /// Return value stored at particular struct element
        virtual llvm::Value* getValueStoredAtIndex(GEPIndex index) = 0;
        virtual ~StructValue() {}

        /// Return instance of particular child dapends of value kind: constant or allocaled
        static std::unique_ptr<StructValue> get(llvm::Value* value);

        /// Return number of elements in struct
        unsigned getNumElements() const { return _structType->getNumElements(); }

        llvm::StructType* getType() const { return _structType; }
    };

    /// Handles struct value given as constant struct
    ///     \@__block_literal_global = internal constant { i8**, i32, i32, i8*, %struct.__block_descriptor* } { ...}
    class ConstantStructValue : public StructValue
    {
    private:
        llvm::ConstantStruct* getStructValue(){ return cast<llvm::ConstantStruct>(_structValue); }
    public:
        explicit ConstantStructValue(llvm::ConstantStruct* value) : StructValue(value) {}
        virtual llvm::Value* getValueStoredAtIndex(GEPIndex index) override
        {
            Value* v = (getNumElements() > index)
                ? getStructValue()->getAggregateElement(index)
                : nullptr;

            if (ConstantExpr *CE = dyn_cast_or_null<ConstantExpr>(v)) {
                if (CE->getOpcode() == Instruction::BitCast)
                    v = CE->getOperand(0);
            }

            return v;
        }
        virtual ~ConstantStructValue() override {}
    };

    /// Handles struct value given as pointer to alloca, followed by getelementptr, store 
    ///
    ///     %1 = alloca <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, ... }>
    ///     %17 = getelementptr inbounds <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, ... }>* %1, i32 0, i32 3
    ///     store i8* bitcast(void(i8*)* \@__initObj_block_invoke to i8*), i8** %17
    class AllocaStructValue : public StructValue
    {
    private:
        std::vector<llvm::Value*> _storedValues;
        llvm::AllocaInst* getStructValue(){ return cast<llvm::AllocaInst>(_structValue); }

        llvm::Value* getValueStoredTo(llvm::Value* ptrVal)
        {
            assert(ptrVal);
            assert(ptrVal->getType()->isPointerTy());
            for (auto ptrUser : ptrVal->users())
            {
                if (auto storeInst = dyn_cast<llvm::StoreInst>(ptrUser))
                {
                    if (storeInst->getPointerOperand() == ptrVal)
                    {
                        return storeInst->getValueOperand()->stripPointerCasts();
                    }
                }
                else if (auto memInst = dyn_cast<llvm::MemTransferInst>(ptrUser))
                {
                    if (memInst->getRawDest() == ptrVal)
                    {
                        return memInst->getSource()->stripPointerCasts();
                    }
                }
                else if (auto castInst = dyn_cast<llvm::CastInst>(ptrUser))
                {
                    return getValueStoredTo(castInst);
                }
            }
            return nullptr;
        }

        /// Fill _storedValues list by all values stored in the struct
        void populateStoredValues()
        {
            for (auto user : getStructValue()->users())
            {
                if (auto elem_ptr_inst = dyn_cast<llvm::GetElementPtrInst>(user))
                {
                    if (elem_ptr_inst->getNumIndices() == 2)
                    {
                        if (auto indexValue = dyn_cast<llvm::ConstantInt>(elem_ptr_inst->getOperand(2)))
                        {
                            size_t index = (size_t)indexValue->getSExtValue();
                            _storedValues[index] = getValueStoredTo(elem_ptr_inst);
                            assert(_storedValues[index] != nullptr);
                        }
                    }
                }
            }
        }

    public:
        explicit AllocaStructValue(llvm::AllocaInst* value) : StructValue(value), _storedValues(value->getAllocatedType()->getStructNumElements(), nullptr)
        {
            populateStoredValues();
        }

        virtual llvm::Value* getValueStoredAtIndex(GEPIndex index) override 
        {
            return _storedValues.size() > index ? _storedValues[index] : nullptr;
        }

        virtual ~AllocaStructValue() override {}
    };

    class NullStructValue : public StructValue
    {
    public:
        explicit NullStructValue(llvm::Value* value) : StructValue(value)
        {
            _structType = StructType::get(value->getContext());
        }

        virtual llvm::Value* getValueStoredAtIndex(GEPIndex index)
        {
            assert("Should not be here");
            return nullptr;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Helper class to emit/build instructions to store values to buffers
    //////////////////////////////////////////////////////////////////////////
    class StoreInstBuilder
    {
        IGCLLVM::IRBuilder<>& _builder;
        const llvm::DataLayout* _DL;

        uint64_t CreateStore(llvm::Value* dest, llvm::Value* source, unsigned align) const
        {
            auto destPtr = dest;

            auto sourceType = source->getType();
            if (dest->getType() != sourceType->getPointerTo())
                destPtr = _builder.CreatePointerCast(dest, sourceType->getPointerTo(), "casted_ptr");

            _builder.CreateStore(source, destPtr)->setAlignment(align);
            return _DL->getTypeAllocSize(sourceType);
        }

        uint64_t CreateMemCpy(llvm::Value* dest, llvm::Value* source, unsigned align) const
        {
            assert(source->getType()->isPointerTy());

            auto size = _DL->getTypeAllocSize(source->getType()->getPointerElementType());
            _builder.CreateMemCpy(dest, source, size, align);
            return size;
        }

    public:
        StoreInstBuilder(IGCLLVM::IRBuilder<>& builder) : _builder(builder), _DL(&(builder.GetInsertBlock()->getParent()->getParent()->getDataLayout()))
        {
            assert(_DL != nullptr);
        }

        /// Create instructions to store source value to the ptr at specified index
        /// using llvm store or memcpy depending on source and dest type
        uint64_t Store(llvm::Value* dest, llvm::Value* source, uint64_t destIndex = UINT64_MAX, bool byVal = true);
    };

    //////////////////////////////////////////////////////////////////////////
    /// Represents and handles block_invoke function for Dispatcher
    ///
    /// Note: class can be inherited for other kinds of invoke functions
    //////////////////////////////////////////////////////////////////////////
    class BlockInvoke
    {
        const Function* _invokeFunc;
        StructType* _captureStructType;
        std::vector<llvm::Type*> _localPointerTypes;
        std::vector<GEPIndex> _captureIndicies;

        /// Analyze block_invoke function.
        /// Build ordered list of indexes of block structure elements used by block_invoke
        void analyzeInvokeFunction();

        /// Return preferred struct alignment based on max preferred alignment of its elements
        static unsigned getPrefStructAlignment(llvm::StructType* structType, const llvm::DataLayout* dl);
    public:
        explicit BlockInvoke(const llvm::Function* invokeFunc) : _invokeFunc(invokeFunc), _captureStructType(nullptr)
        {
            analyzeInvokeFunction();
        }

        /// Return associated block_invoke function
        const Function* getFunction() const { return _invokeFunc; }

        /// Return list of __local pointer argument types
        ArrayRef<llvm::Type*> getLocalPointerTypes() const { return _localPointerTypes; }

        /// Return ordered block structure indexes which are used to build _dispatch_X kernels
        /// and buffers for enqueue_IB_kernel()
        ArrayRef<GEPIndex> getCaptureIndicies() const { return _captureIndicies; }

        /// Return type of block structure element by it's number in capture indicies list
        llvm::Type* getCaptureType(size_t captureNum) const { return _captureStructType->getElementType(_captureIndicies[captureNum]); }

        /// Create set of instructions to prepare block structure and call block_invoke from _dispatch_X
        llvm::CallInst* EmitBlockInvokeCall(IGCLLVM::IRBuilder<>& builder, llvm::ArrayRef<llvm::Argument*> captures, llvm::ArrayRef<llvm::Argument*> tailingArgs) const;
    };

    //////////////////////////////////////////////////////////////////////////
    /// class to create and handle _dispatch_X functions
    //////////////////////////////////////////////////////////////////////////
    class Dispatcher
    {
    private:
        BlockInvoke _blockInvoke;
        const unsigned _blockNumber;
        llvm::Function* _dispatchKernel;
        std::vector<llvm::Argument*> _captureArgs;
        std::vector<llvm::Argument*> _pointerArgs;
        llvm::LLVMContext& _context;

        /// Create _dispatch_X function
        Function* EmitDispatchFunction(const llvm::Twine& dispatchKernelName);

    public:

        /// Construct Dispatcher and create '{namePrefix}_dispatch_{blockId}' function
        explicit Dispatcher(const llvm::Function* invokeFunc, llvm::StringRef namePrefix, unsigned blockId)
            : _blockInvoke(invokeFunc)
            , _blockNumber(blockId)
            , _context(invokeFunc->getContext())
        {
            EmitDispatchFunction(namePrefix + Twine("_dispatch_") + Twine(blockId));
        }

        /// Return created _dispatch_X function
        const Function* getDispatchKernel() const { return _dispatchKernel; }

        /// return X from _dispatch_X
        unsigned getBlockId() const { return _blockNumber; }

        /// Return ordered block structure indexes which are used to build _dispatch_X kernels
        /// and buffers for enqueue_IB_kernel()
        ArrayRef<GEPIndex> getCaptureIndicies() const { return _blockInvoke.getCaptureIndicies(); }

        /// Return dispatched block_invoke function
        const llvm::Function* getBlockInvokeFunc() const { return _blockInvoke.getFunction(); }

        /// Return list of _dispatch_X function arguments for captures
        ArrayRef<llvm::Argument*> getCaptureArgs() const { return _captureArgs; }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Represents a captured value of parent kernel
    //////////////////////////////////////////////////////////////////////////
    struct Capture
    {
        enum { ARG_NUM_NONE = UINT_MAX };
        IGCLLVM::WeakVH value;
        CaptureKind  kind;
        unsigned     argNum;

        Capture(llvm::Value* val, DataContext& context);
        Capture(const Capture& rhs) : value(rhs.value), kind(rhs.kind), argNum(rhs.argNum) {}
        Capture& operator=(const Capture& rhs)
        {
            value = rhs.value;
            kind = rhs.kind;
            argNum = rhs.argNum;
            return *this;
        }

    private:
        CaptureKind kindOf(const llvm::Value* value, DataContext& context);
    };

    //////////////////////////////////////////////////////////////////////////
    /// Represents capture structure value passed to OCL "device execution" call
    //////////////////////////////////////////////////////////////////////////
    class DeviceEnqueueParamValue
    {
    protected:
        bool gotCapturedValues;
        std::vector<Capture> _capturedValues;
        std::unique_ptr<StructValue> _paramStruct;
        llvm::Function* _block_invokeFunction;
        DataContext& _dataContext;

        /// Populate captures list ordered by capture indicies
        bool populateCapturedValues(llvm::ArrayRef<GEPIndex> captureIndicies)
        {
            assert(captureIndicies.size() == 0 || *std::max_element(captureIndicies.begin(), captureIndicies.end()) < _paramStruct->getNumElements() );

            for (auto captureIndex : captureIndicies)
            {
                _capturedValues.emplace_back(_paramStruct->getValueStoredAtIndex(captureIndex), _dataContext);
            }
            return true;
        }

    public:
        DeviceEnqueueParamValue(llvm::Value* param, DataContext& dataContext);

        /// Return captures list ordered by capture indicies
        ArrayRef<Capture> getCapturedValues(llvm::ArrayRef<GEPIndex> captureIndicies)
        {
            if (!gotCapturedValues)
            {
                _capturedValues.clear();
                gotCapturedValues = populateCapturedValues(captureIndicies);
            }
            assert(captureIndicies.size() == _capturedValues.size());
            return _capturedValues;
        }

        llvm::Function* getBlockInvokeFunction() const { return _block_invokeFunction; }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Represents "device execution" call arguments
    //////////////////////////////////////////////////////////////////////////
    class DeviceExecCallArgs
    {
    protected:
        llvm::CallInst& _call;
        DataContext& _dataContext;
        llvm::Function* _callerFunc;
        llvm::Function* _calledFunc;
        DeviceEnqueueParamValue* _paramValue;

        llvm::Value* _queue;
        llvm::Value* _flags;
        llvm::Value* _nDRange;
        llvm::Value* _subgroupCount;
        llvm::Value* _numWaitEvents;
        llvm::Value* _waitEventsList;
        llvm::Value* _retEvent;
        std::vector<llvm::Value*> _local_sizes;

        virtual llvm::Value* getParamArg() = 0;
        virtual llvm::Value* getInvokeArg() = 0;

        DeviceExecCallArgs(llvm::CallInst& call, DataContext& dataContext)
            : _call(call)
            , _dataContext(dataContext)
            , _callerFunc(call.getParent()->getParent())
            , _calledFunc(call.getCalledFunction())
            , _paramValue(nullptr)
            , _queue(nullptr)
            , _flags(nullptr)
            , _nDRange(nullptr)
            , _subgroupCount(nullptr)
            , _numWaitEvents(nullptr)
            , _waitEventsList(nullptr)
            , _retEvent(nullptr)
        {
            if (_calledFunc == nullptr) report_fatal_error("indirect calls are not supported");
        }

    public:
        virtual ~DeviceExecCallArgs() {}

        llvm::CallInst* getCall() const { return &_call; }
        llvm::Function* getCalledFunction() const { return _calledFunc; }

        /// Return parent function for a call
        llvm::Function* getCallerFunc() const { return _callerFunc; }
        IGCLLVM::Module* getModule() const { return (IGCLLVM::Module*)getCallerFunc()->getParent(); }
        llvm::LLVMContext& getContext() const { return _call.getContext(); }

        // Device exec arguments
        llvm::Value* getQueue() const { return _queue; }
        llvm::Value* getEnqueueFlags() const { return _flags; }
        llvm::Value* getNDRange() const { return _nDRange; }
        llvm::Value* getSubgroupCount() const { return _subgroupCount; }
        llvm::Value* getNumWaitEvents() const { return _numWaitEvents; }
        llvm::Value* getWaitEventsList() const { return _waitEventsList; }
        llvm::Value* getRetEvent() const { return _retEvent; }

        DeviceEnqueueParamValue* getParamValue();

        virtual llvm::Function* getEnqueuedFunction()
        {
            auto invokeFunc = dyn_cast<llvm::Function>(getInvokeArg()->stripPointerCasts());
            if (invokeFunc == nullptr) report_fatal_error("Device enqueue invoke param is not a function");
            return invokeFunc;
        }

        std::vector<llvm::Value*> getLocalSizes() const { return _local_sizes; }

        bool hasEvents() const { return getNumWaitEvents() != nullptr; }
        bool hasLocals() const { return getLocalSizes().size() > 0; }
    };

    //////////////////////////////////////////////////////////////////////////
    // ObjectiveC device enqueue arguments
    //////////////////////////////////////////////////////////////////////////
    class ObjCBlockCallArgs : public DeviceExecCallArgs
    {
    protected:
        virtual llvm::Value* getParamArg() override { return _call.getArgOperand(0); }
        virtual llvm::Value* getInvokeArg() override { return getParamValue()->getBlockInvokeFunction(); }

    public:
        ObjCBlockCallArgs(llvm::CallInst& call, DataContext& dataContext) : DeviceExecCallArgs(call, dataContext) {}
    };

    class ObjCNDRangeAndBlockCallArgs : public ObjCBlockCallArgs
    {
    protected:
        virtual llvm::Value* getParamArg() override { return _call.getArgOperand(1); }
    public:
        ObjCNDRangeAndBlockCallArgs(llvm::CallInst& call, DataContext& dataContext) : ObjCBlockCallArgs(call, dataContext)
        {
            _nDRange = _call.getArgOperand(0);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Represents enqueue_kernel() arguments using ObjectiveC block
    //////////////////////////////////////////////////////////////////////////
    class ObjCEnqueueKernelArgs : public ObjCBlockCallArgs
    {
        llvm::Value* _block;
    protected:
        virtual llvm::Value* getParamArg() override { return _block; }

    public:
        ObjCEnqueueKernelArgs(llvm::CallInst& call, DataContext& dataContext)
            : ObjCBlockCallArgs(call, dataContext)
            , _block(nullptr)
        {
            auto arg = call.arg_operands().begin();

            _queue = *(arg++);
            _flags = *(arg++);
            _nDRange = *(arg++);
            if ((*arg)->getType()->isIntegerTy(32))
            {
                _numWaitEvents = *(arg++);
                _waitEventsList = *(arg++);
                _retEvent = *(arg++);
            }

            _block = *(arg++);

            while (arg != call.arg_operands().end())
            {
                if ((*arg)->getType()->isIntegerTy(32)) 
                {
                    _local_sizes.push_back(*(arg++));
                }
                else if((*arg)->getType()->isIntegerTy(64))
                {
                    Type* int32Ty = Type::getInt32Ty(call.getParent()->getContext());
                    Value *newArg = CastInst::CreateTruncOrBitCast(*arg, int32Ty, "", &call);
                    _local_sizes.push_back(newArg);
                    arg++;
                }
                else
                {
                    report_fatal_error("enqueue_kernel signature does not match");
                }
            }
        }

        virtual ~ObjCEnqueueKernelArgs() {}
    };

    //////////////////////////////////////////////////////////////////////////
    // SPIR-V device enqueue arguments
    //////////////////////////////////////////////////////////////////////////
    class SPIRVInvokeCallArgs : public DeviceExecCallArgs
    {
    protected:
        virtual llvm::Value* getParamArg() override { return _call.getArgOperand(1); }
        virtual llvm::Value* getInvokeArg() override { return _call.getArgOperand(0); }
    public:
        SPIRVInvokeCallArgs(llvm::CallInst& call, DataContext& dataContext)
            : DeviceExecCallArgs(call, dataContext)
        {}
    };

    class SPIRVNDRangeAndInvokeCallArgs : public DeviceExecCallArgs
    {
    protected:
        virtual llvm::Value* getParamArg() override { return _call.getArgOperand(2); }
        virtual llvm::Value* getInvokeArg() override { return _call.getArgOperand(1); }

    public:
        SPIRVNDRangeAndInvokeCallArgs(llvm::CallInst& call, DataContext& dataContext)
            : DeviceExecCallArgs(call, dataContext)
        {
            _nDRange = _call.getArgOperand(0);
        }
    };

    class SPIRVSubgroupCountAndInvokeCallArgs : public DeviceExecCallArgs
    {
    protected:
        virtual llvm::Value* getParamArg() override { return _call.getArgOperand(2); }
        virtual llvm::Value* getInvokeArg() override { return _call.getArgOperand(1); }

    public:
        SPIRVSubgroupCountAndInvokeCallArgs(llvm::CallInst& call, DataContext& dataContext)
            : DeviceExecCallArgs(call, dataContext)
        {
            _subgroupCount = _call.getArgOperand(0);
        }
    };

    class SPIRVOpEnqueueKernelCallArgs : public DeviceExecCallArgs
    {
        llvm::Value* _param;
    protected:
        virtual llvm::Value* getParamArg() override { return _call.getArgOperand(7); }
        virtual llvm::Value* getInvokeArg() override { return _call.getArgOperand(6); }

    public:
        SPIRVOpEnqueueKernelCallArgs(llvm::CallInst& call, DataContext& dataContext)
            : DeviceExecCallArgs(call, dataContext)
        {
            if (_call.getNumArgOperands() < 8) report_fatal_error("OpEnqueueKernel signature does not match");

            _queue = _call.getArgOperand(0);
            _flags = _call.getArgOperand(1);
            _nDRange = _call.getArgOperand(2);

            _numWaitEvents = _call.getArgOperand(3);
            _waitEventsList = _call.getArgOperand(4);
            _retEvent = _call.getArgOperand(5);
            if (auto constNumWaitEvents = dyn_cast<llvm::ConstantInt>(_numWaitEvents))
            {
                if (constNumWaitEvents->isZero() && isa<llvm::ConstantPointerNull>(_retEvent->stripPointerCasts()))
                {
                    _numWaitEvents = nullptr;
                    _waitEventsList = nullptr;
                    _retEvent = nullptr;
                }
            }

            const unsigned localSizesStartArgNum = 10;
            const unsigned argsNum = _call.getNumArgOperands();
            for (unsigned i = localSizesStartArgNum; i < argsNum; i++)
            {
                auto arg = _call.getArgOperand(i);
                if (arg->getType()->isPointerTy()) {
                    IRBuilder<> builder(&_call);
                    arg = builder.CreateLoad(arg);
                }

                if (!arg->getType()->isIntegerTy(64) && !arg->getType()->isIntegerTy(32)) 
                    report_fatal_error("OpEnqueueKernel signature does not match");

                if (arg->getType()->isIntegerTy(64))
                {
                    Type* int32Ty = Type::getInt32Ty(_call.getParent()->getContext());
                    arg = CastInst::CreateTruncOrBitCast(arg, int32Ty, "", &_call);
                }
                _local_sizes.push_back(arg);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Abstract class handles a "device execution" call
    //////////////////////////////////////////////////////////////////////////
    class CallHandler
    {
    protected:
        std::unique_ptr<DeviceExecCallArgs> _deviceExecCall;

        /// "Device exec" call to be replaced by returned value
        virtual llvm::Value* getNewValue(const Dispatcher* dispatcher) = 0;

        /// Return the 'name' function. if the function does not exist create it with specified signature
        llvm::Function* getOrCreateFunc(llvm::StringRef name, llvm::Type* retType, llvm::ArrayRef<llvm::Type*> argTypes, bool isVarArg = false);

        /// Create new call instruction with specified signature. Create new function if needed.
        llvm::CallInst* CreateNewCall(llvm::StringRef newName, llvm::Type* retType, llvm::ArrayRef<llvm::Value*> args);

        llvm::StructType* getNDRangeType()
        {
            auto ndrangeStructName = "struct.ndrange_t";
            auto module = _deviceExecCall->getModule();
            auto ndrangeTy = module->getTypeByName(ndrangeStructName);
            if (ndrangeTy == nullptr)
            {
                //create struct type
                auto& context = _deviceExecCall->getContext();
                auto uintTy = llvm::IntegerType::get(context, 32);
                auto ulongTy = llvm::IntegerType::get(context, 64);
                auto arr3ulongTy = llvm::ArrayType::get(ulongTy, 3);
                llvm::Type* ndrangeFields[] = { uintTy, arr3ulongTy, arr3ulongTy, arr3ulongTy };
                ndrangeTy = llvm::StructType::create(context, ndrangeFields, ndrangeStructName, true);
            }
            return ndrangeTy;
        }

        llvm::Value* AdjustNDRangeType(llvm::Value* nDRange)
        {
            auto expectedNdrangeTy = getNDRangeType();

            assert(nDRange->getType()->isPointerTy());
            auto actualNDRangeTy = KindQuery::toStructType(nDRange->getType());
            assert(actualNDRangeTy);
            if (actualNDRangeTy == nullptr) report_fatal_error("NDRange type mismatch");

            if (actualNDRangeTy->isLayoutIdentical(expectedNdrangeTy))
            {
                return nDRange;
            }

            auto allocaInst = new AllocaInst(expectedNdrangeTy, 0, "converted_ndrange", &(*_deviceExecCall->getCallerFunc()->getEntryBlock().getFirstInsertionPt()));
            //TODO: fix bug in later IGC passes
            allocaInst->setAlignment(4);

            auto newName = "__builtin_IB_copyNDRangeTondrange";
            llvm::Value* args[2] = { allocaInst, nDRange };
            CreateNewCall(newName, llvm::Type::getVoidTy(_deviceExecCall->getContext()), args);
            return allocaInst;
        }

    public:
        explicit CallHandler(DeviceExecCallArgs* call)
        {
            assert(call != nullptr);
            _deviceExecCall.reset(call);
        }

        virtual ~CallHandler() {}

        /// Replaces "device execution" call by builtins
        llvm::Value* ReplaceCall(const Dispatcher* dispatcher);

        DeviceExecCallArgs* getArgs() const { return _deviceExecCall.get(); }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Handle get_kernel_preferred_work_group_size_multiple() call
    /// and get_kernel_max_sub_group_size_for_ndrange() call
    //////////////////////////////////////////////////////////////////////////
    class KernelSubGroupSizeCall : public CallHandler
    {
    public:
        explicit KernelSubGroupSizeCall(DeviceExecCallArgs* call) : CallHandler(call)
        {}

        virtual llvm::Value* getNewValue(const Dispatcher* dispatcher) override
        {
            auto blockIdValue = llvm::ConstantInt::get(_deviceExecCall->getContext(), APInt(32, (uint64_t)dispatcher->getBlockId(), false));

            const auto newName = "__builtin_IB_get_block_simd_size";
            auto calledFunction = _deviceExecCall->getCalledFunction();
            if (calledFunction == nullptr) report_fatal_error("indirect calls are not supported");
            return CreateNewCall(newName, calledFunction->getReturnType(), blockIdValue);
        }
    };

    class KernelMaxWorkGroupSizeCall : public CallHandler
    {
    public:
        explicit KernelMaxWorkGroupSizeCall(DeviceExecCallArgs* call) : CallHandler(call)
        {}

        virtual llvm::Value* getNewValue(const Dispatcher* dispatcher) override
        {
            const auto newName = "__builtin_IB_get_max_workgroup_size";
            auto calledFunction = _deviceExecCall->getCalledFunction();
            if (calledFunction == nullptr) report_fatal_error("indirect calls are not supported");
            return CreateNewCall(newName, calledFunction->getReturnType(), {});
        }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Handle get_kernel_sub_group_count_for_ndrange() call
    //////////////////////////////////////////////////////////////////////////
    class KernelSubGroupCountForNDRangeCall : public KernelSubGroupSizeCall
    {
    public:
        explicit KernelSubGroupCountForNDRangeCall(DeviceExecCallArgs* call) : KernelSubGroupSizeCall(call)
        {};

        virtual llvm::Value* getNewValue(const Dispatcher* dispatcher) override
        {
            llvm::Value* block_simd_size = KernelSubGroupSizeCall::getNewValue(dispatcher);
            block_simd_size->setName("block_simd_size");
            llvm::Value* range = AdjustNDRangeType(_deviceExecCall->getNDRange());
            assert(range != nullptr);
            assert(KindQuery::isNDRangeType(range->getType()));

            llvm::Value* args[] = { range, block_simd_size };

            const auto newName = "IGIL_calc_sub_group_count_for_ndrange";
            auto calledFunction = _deviceExecCall->getCalledFunction();
            if (calledFunction == nullptr) report_fatal_error("indirect calls are not supported");
            return CreateNewCall(newName, calledFunction->getReturnType(), args);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Handle OpGetKernelLocalSizeForSubgroupCount call
    //////////////////////////////////////////////////////////////////////////
    class KernelLocalSizeForSubgroupCount : public KernelSubGroupSizeCall
    {
    public:
        explicit KernelLocalSizeForSubgroupCount(DeviceExecCallArgs* call) : KernelSubGroupSizeCall(call)
        {};

        virtual llvm::Value* getNewValue(const Dispatcher* dispatcher) override
        {
            llvm::Value* block_simd_size = KernelSubGroupSizeCall::getNewValue(dispatcher);
            block_simd_size->setName("block_simd_size");
            llvm::Value* subgroupCount = _deviceExecCall->getSubgroupCount();
            assert(subgroupCount != nullptr);

            llvm::Value* args[] = { subgroupCount, block_simd_size };

            const auto newName = "__intel_calc_kernel_local_size_for_sub_group_count";
            auto calledFunction = _deviceExecCall->getCalledFunction();
            if (calledFunction == nullptr) report_fatal_error("indirect calls are not supported");
            return CreateNewCall(newName, calledFunction->getReturnType(), args);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Handle OpGetKernelMaxNumSubgroups call
    //////////////////////////////////////////////////////////////////////////
    class KernelMaxNumSubgroups : public KernelSubGroupSizeCall
    {
    public:
        explicit KernelMaxNumSubgroups(DeviceExecCallArgs* call) : KernelSubGroupSizeCall(call)
        {};

        virtual llvm::Value* getNewValue(const Dispatcher* dispatcher) override
        {
            llvm::Value* block_simd_size = KernelSubGroupSizeCall::getNewValue(dispatcher);
            block_simd_size->setName("block_simd_size");

            llvm::Value* args[] = { block_simd_size };

            const auto newName = "__intel_calc_kernel_max_num_subgroups";
            auto calledFunction = _deviceExecCall->getCalledFunction();
            if (calledFunction == nullptr) report_fatal_error("indirect calls are not supported");
            return CreateNewCall(newName, calledFunction->getReturnType(), args);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    /// Handles enqueue_kernel() call
    //////////////////////////////////////////////////////////////////////////
    class EnqueueKernelCall : public CallHandler
    {

        /// structure for buffer arguments for enqueue_IB_kernel
        /// matches signature of enqueue_IB_kernel_local()
        struct BufferArguments
        {
            llvm::Value* scalarParamBuf;
            llvm::Value* sizeofscalarParamBuf;
            llvm::Value* globalArgBuf;
            llvm::Value* numGlobalArgBuf;
            llvm::Value* local_size_buf;
            llvm::Value* sizeof_local_size_buf;
            llvm::Value* globalPtrArgMappingBuf;
            llvm::Value* getobjectidMappingBuf;
            llvm::Value* numArgMappings;
        };

        const llvm::DataLayout* _DL;

        uint64_t sizeInBlocks(uint64_t size, llvm::Type* blockType)
        {
            auto blockSize = _DL->getTypeAllocSize(blockType);
            return size / blockSize + (((size % blockSize) == 0) ? 0 : 1);
        }

        /// Create alloca instruction
        llvm::AllocaInst* AllocateBuffer(llvm::Type* type, uint64_t arrSize, const llvm::Twine& name = "");

        /// Create instruction to fill buffers for enqueue_IB_kernel()
        BufferArguments CreateBufferArguments(IGCLLVM::IRBuilder<>& builder, llvm::ArrayRef<Capture> capturedValues);

    protected:
        /// Create call to enqueue_IB_kernel
        virtual llvm::Value* getNewValue(const Dispatcher* dispatcher) override;

    public:
        explicit EnqueueKernelCall(DeviceExecCallArgs* call)
            : CallHandler(call)
            , _DL(&call->getCall()->getParent()->getParent()->getParent()->getDataLayout())
        {}

        virtual ~EnqueueKernelCall() {}
    };

    //////////////////////////////////////////////////////////////////////////
    // DataContext - "database" to store all objects used in the pass
    // contains queries to get objects by keys
    //////////////////////////////////////////////////////////////////////////
    class DataContext
    {
        struct InvokeRecord
        {
            std::vector<std::unique_ptr<CallHandler>> _enqueues;
            std::unique_ptr<Dispatcher> _dispatcher;
            IGC::IGCMD::FunctionInfoMetaDataHandle _dispatcherMetaData;

            InvokeRecord() = default;
            InvokeRecord(InvokeRecord&& other)
                : _enqueues(std::move(other._enqueues))
                , _dispatcher(std::move(other._dispatcher))
                , _dispatcherMetaData(std::move(other._dispatcherMetaData))
            {
            }

        };

        // Since map _invocations's key (Function*) could be non-deterministic (
        // depending on where a function is allocated), this would cause invokedFunc
        // to be mapped to a different dispatch_x each time the program is compiled.
        // To prevent this from happening, we will use a custom compare, which is based
        // on the function's name.
        std::map<const llvm::Function*, InvokeRecord, FuncCompare> _invocations;
        std::map<llvm::Value*, std::unique_ptr<DeviceEnqueueParamValue>> _deviceEnqueueParamValueMap;
        unsigned _blocksNum;

        KindQuery _kindQuery;
        MetadataBuilder _MDBuilder;

    public:

        DataContext(IGC::ModuleMetaData* modMD, IGC::IGCMD::MetaDataUtils* pMdUtils)
            : _blocksNum(0)
            , _kindQuery(*this)
            , _MDBuilder(pMdUtils, *this, modMD)
        {}

        const KindQuery& getKindQuery() { return _kindQuery; }
        MetadataBuilder& getMetaDataBuilder() { return _MDBuilder; }

        /// Register new "device execution" call handler in the "database"
        CallHandler* registerCallHandler(llvm::CallInst& call);

        /// Check if function is registered as InvokeFunc
        bool isInvokeFunc(const llvm::Function* func)
        {
            return _invocations.count(func) > 0;
        }

        /// Return a list of all block_invoke functions used in registered "device execution" calls
        std::vector<const llvm::Function*> getInvokeFuncs()
        {
            std::vector<const llvm::Function*> invokedFuncs;
            for (auto& pair : _invocations)
            {
                invokedFuncs.push_back(pair.first);
            }
            return invokedFuncs;
        }

        /// Return Dispatcher for the registered block_invoke function.
        /// Create new Dispatcher and store in "DB" if needed
        Dispatcher* getDispatcherForInvokeFunc(const llvm::Function* invokeFunc);

        /// Return Dispatcher for _dispatch_X function
        Dispatcher* getDispatcherForDispatchFunc(const llvm::Function* dispatchFunc);

        /// Return a list of registered cahh handlers for particular block_invoke
        const std::vector<CallHandler*> getCallHandlersFor(const llvm::Function* invokeFunc)
        {
            std::vector<CallHandler*> callHandlers;
            auto invocationPos = _invocations.find(invokeFunc);
            if (invocationPos != _invocations.end())
            {
                for (auto& ptr : invocationPos->second._enqueues)
                {
                    callHandlers.push_back(ptr.get());
                }
            }
            return callHandlers;
        }

        /// Create, register and return BlockDescriptorValue
        /// for the block value passed to a "device execution" call
        DeviceEnqueueParamValue* getDeviceEnqueueParamValue(llvm::Value* value);

        /// Return parent's kernel name for block_invoke function
        /// used for _dispatch_X kernel generation
        llvm::StringRef getParentKernelName(const llvm::Function* invokeFunc, std::set<const llvm::Function*> &processedFuncs);

        /// track load/store instructions to get source value
        static llvm::Value* getSourceValueFor(llvm::Value* value, llvm::Instruction* destValue = nullptr);

        /// Lookup for the kernel/dispatcher argument connected to the value
        llvm::Argument* getArgForValue(llvm::Value* value);
    };

    //////////////////////////////////////////////////////////////////////////
    /// llvm Pass
    //////////////////////////////////////////////////////////////////////////
    class TransformBlocks : public llvm::ModulePass
    {
    public:
        static char ID;
        TransformBlocks() : llvm::ModulePass(ID)
        {
            initializeTransformBlocksPass(*PassRegistry::getPassRegistry());
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "TransformBlocks Pass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        bool runOnModule(Module &M) override
        {
            bool changed = false;
            auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
            auto modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

            // dataContext "owns" all created objects in internal "database"
            DataContext dataContext(modMD, pMdUtils);

            // Inline functions which have device exec calls up to kernel or block_invoke level
            // to properly resolve parent argument number for the following case:
            //
            // void doEnq(image, sampler) { enqueue_kernel(..., ^{ child(image, sampler); }); }
            // kernel void k1(image, sampler) { doEnc(image, sampler); }
            // kernel void k2(sampler, image) { doEnc(image, sampler); }
            changed = InlineEnqueueKernelCalls(M, dataContext) || changed;

            changed = HandleIvokeKernelWrappers(M, dataContext, modMD) || changed;

            // Run mem2reg pass to make the processing simpler
            bool isOptDisabled = modMD->compOpt.OptDisable;
            if (!isOptDisabled || IGC_IS_FLAG_ENABLED(AllowMem2Reg))
            {
                legacy::PassManager PM;
                PM.add(createPromoteMemoryToRegisterPass());
                changed = PM.run(M) || changed;
            }

            // calls to blocks (not enqueues) are implemented as indirect calls
            // these calls should be resolved now because indirect calls are not
            // supported by other passes
            changed = ResolveIndirectBlockCalls(M) || changed;

            // Lookup and register all "device enqueue" calls
            RegisterCallHandlers(M, dataContext);

            auto invokeFuncs = dataContext.getInvokeFuncs();

            for (auto& invokeFunc : invokeFuncs)
            {
                auto dispatcher = dataContext.getDispatcherForInvokeFunc(invokeFunc);
                auto dispatchKernel = dispatcher->getDispatchKernel();
                dataContext.getMetaDataBuilder().getOrEmitKernelMetadata(dispatchKernel);

                dataContext.getMetaDataBuilder().EmitDeviceEnqueueMetadataPair(invokeFunc, dispatchKernel);
                
                changed = true;
            }

            // All dispatchers should be created before replace calls (SAMPLERS!!!)
            for (auto& invokeFunc : invokeFuncs)
            {
                for (auto& enqueue : dataContext.getCallHandlersFor(invokeFunc))
                {
                    enqueue->ReplaceCall(dataContext.getDispatcherForInvokeFunc(invokeFunc));
                    changed = true;
                }
            }

            // Remove values not supported by compiler (extern global BlockStacks and pointer to functions)
            changed = RemoveUnsupportedInstFromModule(M) || changed;
            
            return changed;
        }

    private:

        // Inline function to its callers and remove
        bool InlineToParents(llvm::Function* func, DataContext& dataContext)
        {
            bool inlined = false;
            std::vector<llvm::User*> users(func->user_begin(), func->user_end());
            for (auto user : users)
            {
                if (auto callInst = dyn_cast<llvm::CallInst>(user))
                {
                    llvm::InlineFunctionInfo IFI;
                    inlined = llvm::InlineFunction(callInst, IFI, nullptr, false) || inlined;
                }
            }

            // kernels and block_invokes should be kept
            // block_invoke has at least 1 use which is not a call but pointer
            if (func->use_empty() && !dataContext.getKindQuery().isKernel(func))
            {
                func->eraseFromParent();
            }
            return inlined;
        }

        // Clang might insert a kernel wrapper around invoke function, details and motivation can be found
        // here: https://reviews.llvm.org/D38134
        bool isInvokeFunctionKernelWrapper(const Function* invokeFunc, DataContext& dataContext) {
            return dataContext.getKindQuery().isKernel(invokeFunc);
        }

        Function* getInvokeFunctionFromKernelWrapper(const Function* invokeFunc, DataContext& dataContext) {
            assert(isInvokeFunctionKernelWrapper(invokeFunc, dataContext));
            const CallInst* inst = dyn_cast<CallInst>(&*(invokeFunc->begin()->begin()));
            if (inst) {
                return inst->getCalledFunction();
            } else {
                return nullptr;
            }
        }

        bool isEnqueueKernelFunction(StringRef funcName) {

            return funcName.startswith(DeviceEnqueueFunctionNames.at(DeviceEnqueueFunction::ENQUEUE_KERNEL)) ||
                funcName.startswith(DeviceEnqueueFunctionNames.at(DeviceEnqueueFunction::SPIRV_ENQUEUE_KERNEL)) ||
                funcName.startswith(DeviceEnqueueFunctionNames.at(DeviceEnqueueFunction::ENQUEUE_KERNEL_BASIC)) ||
                funcName.startswith(DeviceEnqueueFunctionNames.at(DeviceEnqueueFunction::ENQUEUE_KERNEL_VAARGS)) ||
                funcName.startswith(DeviceEnqueueFunctionNames.at(DeviceEnqueueFunction::ENQUEUE_KERNEL_EVENTS_VAARGS));
        }

        bool isDeviceEnqueueFunction(StringRef funcName) {
            for (auto el : DeviceEnqueueFunctionNames) {
                if (funcName.startswith(el.second)) {
                    return true;
                }
            }
            return false;
        }


        // All functions which call enqueue_kernel should be inlined up to
        // kernel or block_invoke level to proper argument number detection of
        // captured images and samplers
        bool InlineEnqueueKernelCalls(llvm::Module &M, DataContext& dataContext)
        {
            bool changed = false;
            for (auto& func : M.functions())
            {
                // enqueue_kernel functions are just declared
                if (!func.isDeclaration())
                {
                    continue;
                }

                auto funcName = func.getName();
                if (!isEnqueueKernelFunction(funcName))
                {
                    continue;
                }

                bool inlined = false;
                do
                {
                    inlined = false;
                    std::unordered_set<llvm::Function*> functionsToInline;;

                    for (auto user : func.users()) {
                        if (auto callInst = dyn_cast<llvm::CallInst>(user)) {
                        //for each call to enqueue_kernel
                            functionsToInline.insert(callInst->getParent()->getParent());
                        }
                    }

                    for (auto func : functionsToInline)
                        {
                            //try to inline the caller
                        inlined = InlineToParents(func, dataContext) || inlined;
                        }
                    changed = inlined || changed;

                } while (inlined); // if callers are inlined, restart loop with new users list
            }
            return changed;
        }

        // Clang might insert a kernel wrapper around invoke function, details and motivation can be found
        // here: https://reviews.llvm.org/D38134
        // As we want to generate our own enqueue kernel with possibly different arguments, 
        // we need to treat this kernel as a regular invoke function. 
        // To achieve this, we need to: 
        // 1. Inline the inner invoke function called by this kernel - analyzeInvokeFunction assumes that.
        // 2. Remove the metadata related to this kernel, so that codegen does not treat it as a regular kernel.
        bool HandleIvokeKernelWrappers(llvm::Module &M, DataContext& dataContext, IGC::ModuleMetaData* modMD)
        {
            bool changed = false;
            for (auto &func : M.functions()) {
                if (!isDeviceEnqueueFunction(func.getName())) continue;

                for (auto user : func.users()) {
                    auto callInst = dyn_cast<CallInst>(user);
                    if (!callInst) continue;

                    for (auto& arg : callInst->arg_operands()) {
                        if (Function* invoke = dyn_cast<Function>(arg)) {
                            if (isInvokeFunctionKernelWrapper(invoke, dataContext)) {
                                // Inline the wrapped invoke function.
                                Function* innerInvoke = getInvokeFunctionFromKernelWrapper(invoke, dataContext);
                                if (innerInvoke) {
                                    changed = InlineToParents(innerInvoke, dataContext) || changed;
                                }

                                // Remove the kernel metadata.
                                changed = dataContext.getMetaDataBuilder().eraseKernelMetadata(invoke, modMD) || changed;
                            }
                        }
                    }
                }
            }

            return changed;
        }

        // ResolveIndirectCalls - resolves all idirect calls assuming these are block calls
        // calls are inlined to properly resolve cases when block returns block
        bool ResolveIndirectBlockCalls(llvm::Module &M)
        {
            bool changed = false;
            for (auto& func : M.functions())
            {
                if (func.isDeclaration())
                    continue;

                bool inlined = false;
                do
                {
                    // repeat in-function iteration until complete all inlinings
                    // because inlined blocks can contain indirect calls as well
                    inlined = false;
                    for (auto& BB : func)
                    {
                        for (auto& I : BB)
                        {
                            auto callInst = dyn_cast<llvm::CallInst>(&I);
                            if (callInst != nullptr && callInst->getCalledFunction() == nullptr)
                            {
                                // assuming indirect call is the block call
                                unsigned blockArgIdx = callInst->hasStructRetAttr() ? 1 : 0;
                                if (callInst->getNumArgOperands() > blockArgIdx)
                                {
                                    if (auto blockDescrStruct = StructValue::get(callInst->getArgOperand(blockArgIdx)->stripPointerCasts()))
                                    {
                                        if (auto blockInvokeFunc = dyn_cast<llvm::Function>(blockDescrStruct->getValueStoredAtIndex(BLOCK_INDEX_INVOKE_FUNC)))
                                        {
                                            callInst->setCalledFunction(blockInvokeFunc);
                                            changed = true;
                                            llvm::InlineFunctionInfo IFI;
                                            inlined = llvm::InlineFunction(callInst, IFI, nullptr, false);
                                            assert(inlined && "failed inlining block invoke function");
                                        }
                                    }
                                }
                            }

                            // break in-BB iteration
                            if (inlined) break;
                        }
                        // restart in-function iteration
                        if (inlined) break;
                    }
                } while (inlined);
            }
            return changed;
        }

        /// Enumerate all call instructions in the module and register "device enq" handlers
        void RegisterCallHandlers(llvm::Module& M, DataContext& dataContext)
        {
            for (auto& func : M.functions())
            {
                if (func.isDeclaration())
                    continue;
                
                for (auto& BB : func)
                {
                    for (auto& I : BB)
                    {
                        if (I.getOpcode() == Instruction::Call)
                        {
                            // data context checks if call "device enq"
                            dataContext.registerCallHandler(static_cast<llvm::CallInst&>(I));
                        }
                    }
                }
            }
        }

        /// Remove values which are not supported (extern global blocks and function pointers)
        bool RemoveUnsupportedInstFromModule(llvm::Module& M)
        {
            bool changed = false;
            // remove unsupported externals
            const llvm::StringRef unsupportedExternalPtrNames[] = { "_NSConcreteStackBlock", "_NSConcreteGlobalBlock" };
            for (auto name : unsupportedExternalPtrNames)
            {
                if (auto gv = M.getGlobalVariable(name))
                {
                    auto gvType = cast<llvm::PointerType>(gv->getType());
                    auto initType = cast<llvm::PointerType>(gvType->getElementType());
                    auto nullConst = llvm::ConstantPointerNull::get(initType);
                    gv->setExternallyInitialized(false);
                    gv->setConstant(true);
                    gv->setInitializer(nullConst);
                    changed = true;
                }
            }

            // remove function pointers
            auto nullPtrConst = llvm::ConstantPointerNull::get(Type::getInt8PtrTy(M.getContext()));
            for (auto& func : M.functions())
            {
                for (auto user : func.users())
                {
                    if (!isa<llvm::CallInst>(user))
                    {
                        if (!isa<llvm::Constant>(user)) {
                            user->replaceUsesOfWith(&func, nullPtrConst);
                        } else {
                            user->replaceAllUsesWith(llvm::Constant::getNullValue(user->getType()));
                        }
                        changed = true;
                    }
                }
            }
            return changed;
        }
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // MetadataBuilder implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////
    void MetadataBuilder::EmitKernelMetadata(const llvm::Function* kernelFunc)
    {
        // Helper structure for pretty printing types
        struct TypeNameHelper {
            static llvm::raw_ostream& BaseTypeName(llvm::Type* type, llvm::raw_ostream& os) {
                switch (type->getTypeID()) {
                case Type::VoidTyID:
                    return os << "void";
                case Type::HalfTyID:
                    return os << "half";
                case Type::FloatTyID:
                    return os << "float";
                case Type::DoubleTyID:
                case Type::X86_FP80TyID:
                case Type::FP128TyID:
                case Type::PPC_FP128TyID:
                    return os << "double";
                case Type::IntegerTyID:
                    switch (type->getIntegerBitWidth()) {
                    case 1:
                        return os << "bool";
                    case 8:
                        return os << "uchar";
                    case 16:
                        return os << "short";
                    case 32:
                        return os << "int";
                    case 64:
                        return os << "long";
                    default:
                        return os << "int";
                    }
                case Type::VectorTyID:
                    // this generates <element_type><num_elements> string. Ie for char2 element_type is char and num_elements is 2
                    // that is done by callin BaseTypeName on vector element type, this recursive call has only a depth of one since
                    // there are no compound vectors in OpenCL.
                    return BaseTypeName(type->getVectorElementType(), os) << type->getVectorNumElements();
                default:
                    assert(false && "Unknown basic type found");
                    return os << "unknown_type";
                }
            }

            static llvm::raw_ostream& Print(llvm::Type* type, llvm::raw_ostream& os) {
                if (type->isPointerTy())
                {
                    if (type->getPointerElementType()->isStructTy())
                    {
                        auto val = type->getPointerElementType()->getStructName();
                        size_t offset = val.find('.');
                        return os << val.substr(offset + 1);
                    }
                    return BaseTypeName(type->getPointerElementType(), os) << "*";
                }

                return BaseTypeName(type, os);
            }
        };

        //link dispatchMd to dispatch kernel
        IGC::IGCMD::FunctionInfoMetaDataHandle dispatchMd = _pMdUtils->getOrInsertFunctionsInfoItem(const_cast<llvm::Function*>(kernelFunc));

        auto funcMD = &modMD->FuncMD[const_cast<llvm::Function*>(kernelFunc)];//insert if not present 
                                                                                  
        //set function type for dispatch
        dispatchMd->setType(FunctionTypeMD::KernelFunction);

        funcMD->functionType = IGC::FunctionTypeMD::KernelFunction;

        for (auto& arg : kernelFunc->args())
        {
            auto argType = arg.getType();

            int32_t addrSpace = (argType->isPointerTy()) ? static_cast<int32_t>(argType->getPointerAddressSpace()) : 0;
            std::string accessQual = "none";

            std::string typeName;
            llvm::SmallVector<llvm::StringRef, 3> nameFractions;
            if (argType->isIntegerTy() && _dataContext.getKindQuery().isSampleCaptured(kernelFunc, arg.getArgNo()))
            {
                typeName = "sampler_t";
            }
            else if (KindQuery::isImageType(argType, &nameFractions))
            {
                typeName = nameFractions[1];
                accessQual = nameFractions.size() > 2 ? nameFractions[2] : "read_write";
            }
            else
            {
                llvm::raw_string_ostream os(typeName);
                TypeNameHelper::Print(argType, os);
            }

            funcMD->m_OpenCLArgAddressSpaces.push_back(addrSpace);
            funcMD->m_OpenCLArgAccessQualifiers.push_back(accessQual);
            funcMD->m_OpenCLArgTypes.push_back(typeName);
            funcMD->m_OpenCLArgTypeQualifiers.push_back("");
            funcMD->m_OpenCLArgBaseTypes.push_back(typeName);
            funcMD->m_OpenCLArgNames.push_back(arg.getName());
        }
        _pMdUtils->save(kernelFunc->getContext());

        return;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  KindQuery implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    // check if arg is sampler. 
    bool KindQuery::isSamplerArg(const llvm::Argument* arg) const
    {
        // Currently we support sampler types being represented as contant pointers or integer types
        if (arg == nullptr || !(arg->getType()->isIntegerTy() || arg->getType()->isPointerTy())) 
            return false;

        auto parentFunc = arg->getParent();

        auto argType = _dataContext.getMetaDataBuilder().getKernelArgTypeName(parentFunc, arg->getArgNo());
        if ("sampler_t" == argType)
        {
            return true;
        }
        return false;
    }

    // check if dispatcher's argument is sampler. 
    bool KindQuery::isSampleCaptured(const llvm::Function* dispatchFunc, unsigned captureNum) const
    {
        auto dispatcher = _dataContext.getDispatcherForDispatchFunc(dispatchFunc);
        if (dispatcher == nullptr) return false;

        auto invokeFunc = dispatcher->getBlockInvokeFunc();

        // get list of calls which "enqueue" this dispatcher
        auto parentCalls = _dataContext.getCallHandlersFor(invokeFunc);
        if (parentCalls.size() == 0) report_fatal_error("parent calls for invoke are not set");

        // lookup for a call which located in a "__kernel"
        // if __kernel call will not found, use the one which is not self
        CallHandler* parentCallHandler = nullptr;
        for (auto callHandler : parentCalls)
        {
            const llvm::Function* callerFunc = callHandler->getArgs()->getCallerFunc();
            if (isKernel(callerFunc))
            {
                parentCallHandler = callHandler;
                break;
            }
            if (callerFunc != invokeFunc)
            {
                parentCallHandler = callHandler;
            }
        }

        if (parentCallHandler == nullptr) report_fatal_error("Fail parent call lookup: possible closed self-enqueue");

        auto capturedValues = parentCallHandler->getArgs()->getParamValue()->getCapturedValues(_dataContext.getDispatcherForInvokeFunc(invokeFunc)->getCaptureIndicies());
        auto& capture = capturedValues[captureNum];
        return (capture.kind == CaptureKind::SAMPLER);
    }

    bool KindQuery::isKernel(const llvm::Function* func) const
    {
        return _dataContext.getMetaDataBuilder().getKernelMetadata(func) != nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  DataContext implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    /// Create new CallHandler only for "device enqueue" calls
    CallHandler* DataContext::registerCallHandler(llvm::CallInst& call)
    {
        // device enqueue call handlers factories registry
        const std::pair<DeviceEnqueueFunction, std::function<CallHandler*(llvm::CallInst&, DataContext& dm)>> handlers[] =
        {
            {
                DeviceEnqueueFunction::MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelSubGroupSizeCall(new ObjCNDRangeAndBlockCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelSubGroupSizeCall(new ObjCBlockCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::PREFERRED_WORK_GROUP_MULTIPLE_IMPL,
                [](llvm::CallInst& call, DataContext& dm) { return new KernelSubGroupSizeCall(new ObjCBlockCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::WORK_GROUP_SIZE_IMPL,
                [](llvm::CallInst& call, DataContext& dm) { return new KernelMaxWorkGroupSizeCall(new ObjCBlockCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::SUB_GROUP_COUNT_FOR_NDRANGE,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelSubGroupCountForNDRangeCall(new ObjCNDRangeAndBlockCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::ENQUEUE_KERNEL,
                [](llvm::CallInst& call, DataContext& dm){ return new EnqueueKernelCall(new ObjCEnqueueKernelArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::ENQUEUE_KERNEL_BASIC,
                [](llvm::CallInst& call, DataContext& dm){ return new EnqueueKernelCall(new ObjCEnqueueKernelArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::ENQUEUE_KERNEL_VAARGS,
                [](llvm::CallInst& call, DataContext& dm){ return new EnqueueKernelCall(new ObjCEnqueueKernelArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::ENQUEUE_KERNEL_EVENTS_VAARGS,
                [](llvm::CallInst& call, DataContext& dm){ return new EnqueueKernelCall(new ObjCEnqueueKernelArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::SPIRV_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelSubGroupSizeCall(new SPIRVNDRangeAndInvokeCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::SPIRV_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelSubGroupSizeCall(new SPIRVInvokeCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::SPIRV_LOCAL_SIZE_FOR_SUB_GROUP_COUNT,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelLocalSizeForSubgroupCount(new SPIRVSubgroupCountAndInvokeCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::SPIRV_MAX_NUM_SUB_GROUPS,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelMaxNumSubgroups(new SPIRVInvokeCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::SPIRV_SUB_GROUP_COUNT_FOR_NDRANGE,
                [](llvm::CallInst& call, DataContext& dm){ return new KernelSubGroupCountForNDRangeCall(new SPIRVNDRangeAndInvokeCallArgs(call, dm)); }
            },
            {
                DeviceEnqueueFunction::SPIRV_ENQUEUE_KERNEL,
                [](llvm::CallInst& call, DataContext& dm){ return new EnqueueKernelCall(new SPIRVOpEnqueueKernelCallArgs(call, dm)); }
            },
        };

        static_assert(sizeof(handlers) == sizeof(decltype(handlers[0])) * (size_t)DeviceEnqueueFunction::NUM_FUNCTIONS_WITH_BLOCK_ARGS, "Not all enqueue functions have handlers!");

        auto calledFunction = call.getCalledFunction();
        //fail indirect calls
        if (calledFunction == nullptr)
        {
            return nullptr;
        }
        auto calledFunctionName = calledFunction->getName();

        // Strategy/Factory
        // lookup called function name in handlers registry
        for (auto& handler_pair : handlers)
        {
            // if called function name matches one of known
            if (calledFunctionName.startswith(DeviceEnqueueFunctionNames.at(handler_pair.first)))
            {
                // use appropriate factory to construct CallHandler
                auto callHandler = handler_pair.second(call, *this);
                _invocations[callHandler->getArgs()->getEnqueuedFunction()]._enqueues.emplace_back(callHandler);
                return callHandler;
            }
        }
        return nullptr;
    }

    Dispatcher* DataContext::getDispatcherForInvokeFunc(const llvm::Function* invokeFunc)
    {
        if (!_invocations[invokeFunc]._dispatcher)
        {
            std::set<const llvm::Function*> processedFuncs;
            StringRef parentKernelName = getParentKernelName(invokeFunc, processedFuncs);
            _invocations[invokeFunc]._dispatcher.reset(new Dispatcher(invokeFunc, parentKernelName, _blocksNum++));
        }

        return _invocations.at(invokeFunc)._dispatcher.get();
    }

    Dispatcher* DataContext::getDispatcherForDispatchFunc(const llvm::Function* dispatchFunc)
    {
        for (auto& pair : _invocations)
        {
            if (auto& dispatcher = pair.second._dispatcher)
            {
                if (dispatcher->getDispatchKernel() == dispatchFunc)
                {
                    return dispatcher.get();
                }
            }
        }
        return nullptr;
    }

    DeviceEnqueueParamValue* DataContext::getDeviceEnqueueParamValue(llvm::Value* value)
    {
        if (!_deviceEnqueueParamValueMap[value])
        {
            _deviceEnqueueParamValueMap[value].reset(new DeviceEnqueueParamValue(value, *this));
        }
        return _deviceEnqueueParamValueMap[value].get();
    }

    llvm::StringRef DataContext::getParentKernelName(const llvm::Function* invokeFunc, std::set<const llvm::Function*> &processedFuncs)
    {
        if (invokeFunc == nullptr) report_fatal_error("invoke function should not be null");

        processedFuncs.insert(invokeFunc);

        auto callHandlers = getCallHandlersFor(invokeFunc);
        std::vector<const llvm::Function*> parentCandidates;
        for (auto& callHandler : callHandlers)
        {
            auto caller = callHandler->getArgs()->getCallerFunc();
            if (processedFuncs.count(caller) == 0)
            {
                parentCandidates.push_back(caller);
            }
        }

        //if function is not invoked, then lookup for direct callers
        for (auto user : invokeFunc->users())
        {
            if (auto callInst = dyn_cast<llvm::CallInst>(user))
            {
                if (callInst->getCalledFunction() == invokeFunc)
                {
                    auto caller = callInst->getParent()->getParent();
                    if (processedFuncs.count(caller) == 0)
                    {
                        parentCandidates.push_back(caller);
                    }
                }
            }
        }

        const llvm::Function* parentCandidate = nullptr;
        for (auto myFunc : parentCandidates)
        {
            if (getKindQuery().isKernel(myFunc))
            {
                return myFunc->getName();
            }
            if (myFunc != invokeFunc)
            {
                parentCandidate = myFunc;
            }
        }

        if (parentCandidate == nullptr) report_fatal_error("Fail parent kernel lookup: possible closed self-enqueue");
        return getParentKernelName(parentCandidate, processedFuncs);
        
    }

    llvm::Value* DataContext::getSourceValueFor(llvm::Value* value, llvm::Instruction* destValue /*= nullptr*/)
    {
        if (value == nullptr) return value;

        if (auto loadInst = dyn_cast<llvm::LoadInst>(value))
        {
            return getSourceValueFor(loadInst->getPointerOperand(), loadInst);
        }

        if (auto castInst = dyn_cast<llvm::CastInst>(value))
        {
            return getSourceValueFor(castInst->getOperand(0), castInst);
        }

        if (auto constExpr = dyn_cast<llvm::ConstantExpr>(value))
        {
            if (constExpr->isCast())
            {
                return getSourceValueFor(constExpr->getOperand(0), destValue);
            }
        }

        if (auto allocaInst = dyn_cast<llvm::AllocaInst>(value))
        {
            auto currBB = allocaInst->getParent();
            assert(currBB);
            auto currFunc = currBB->getParent();
            assert(currFunc);

            llvm::StoreInst* foundStore = nullptr;

            for (auto II = llvm::inst_begin(currFunc); !II.atEnd() && (&(*II) != destValue); ++II)
            {
                auto inst = &(*II);
                if (auto storeInst = dyn_cast<llvm::StoreInst>(inst))
                {
                    if (storeInst->getPointerOperand() == allocaInst)
                    {
                        foundStore = storeInst;
                    }
                }
            }

            if (foundStore != nullptr)
            {
                return getSourceValueFor(foundStore->getValueOperand(), nullptr);
            }
        }

        return value;
    }

    llvm::Argument* DataContext::getArgForValue(llvm::Value* value)
    {
        auto sourceValue = getSourceValueFor(value);

        //just an argument of parent kernel
        if (auto arg = dyn_cast<llvm::Argument>(sourceValue))
        {
            return arg;
        }

        //passed as block_descr element to block_invoke
        if (auto getElemPtrInstr = dyn_cast<llvm::GetElementPtrInst>(sourceValue))
        {
            //check if we are in the block_invoke which is already dispatched
            auto myFunction = getElemPtrInstr->getParent()->getParent();
            if (isInvokeFunc(myFunction))
            {
              auto sourcePointer = getSourceValueFor(getElemPtrInstr->getPointerOperand());
              //assuming that block_invoke function always has the first argument which is the pointer to block descriptor
              auto sourceArg = llvm::dyn_cast_or_null<llvm::Argument>(sourcePointer);
              if (sourceArg != nullptr && sourceArg->getArgNo() == 0)
              {
                auto dispatcherCallingMe = getDispatcherForInvokeFunc(myFunction);

                if (auto elemIdxValue = dyn_cast<const llvm::ConstantInt>(getElemPtrInstr->getOperand(getElemPtrInstr->getNumOperands() - 1)))
                {
                    auto elemIndex = (GEPIndex)elemIdxValue->getZExtValue();
                    unsigned argId = 0;
                    for (auto idx : dispatcherCallingMe->getCaptureIndicies())
                    {
                        if (elemIndex == idx) return dispatcherCallingMe->getCaptureArgs()[argId];
                        argId++;
                    }
                }
              }
            }
        }

        return nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  StructValue implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    std::unique_ptr<StructValue> StructValue::get(llvm::Value* value)
    {
        assert(value != nullptr);

        if (value == nullptr)
            return nullptr;

        auto sourceValue = DataContext::getSourceValueFor(value);

        if (auto allocaInst = dyn_cast<llvm::AllocaInst>(sourceValue))
        {
            return std::unique_ptr<StructValue>(new AllocaStructValue(allocaInst));
        }
        else if (auto globalVar = dyn_cast<llvm::GlobalVariable>(sourceValue))
        {
            if (!globalVar->hasInitializer())
            {
                return nullptr;
            }

            while (auto subGV = dyn_cast<llvm::GlobalVariable>(globalVar->getInitializer()->stripPointerCasts()))
            {
                globalVar = subGV;
                if (!globalVar->hasInitializer())
                {
                    return nullptr;
                }
            }

            if (auto constantStruct = dyn_cast<llvm::ConstantStruct>(globalVar->getInitializer()))
            {
                return std::unique_ptr<StructValue>(new ConstantStructValue(constantStruct));
            }
        }
        else if (isa<llvm::ConstantPointerNull>(sourceValue))
        {
            return std::unique_ptr<StructValue>(new NullStructValue(sourceValue));
        }
        assert(false && "should not be here");
        return nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  StoreInstBuilder implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    uint64_t StoreInstBuilder::Store(llvm::Value* dest, llvm::Value* source, uint64_t destIndex /*= UINT64_MAX*/, bool byVal /*= true*/)
    {
        assert(dest != nullptr);
        assert(source != nullptr);

        auto ptrType = dest->getType();
        assert(ptrType->isPointerTy());

        auto destPtr = dest;
        auto typeToSelect = source->getType();
        if (destIndex != UINT64_MAX)
        {
            if (KindQuery::isStructType(ptrType))
            {
                destPtr = _builder.CreateStructGEP(nullptr, dest, static_cast<unsigned>(destIndex));
                // If store to struct then use struct element type
                // to select proper store instruction
                typeToSelect = destPtr->getType();
            }
            else
            {
                destPtr = (_DL->getPointerTypeSize(ptrType) > 4)
                    ? _builder.CreateConstInBoundsGEP2_64(dest, 0, destIndex)
                    : _builder.CreateConstInBoundsGEP2_32(nullptr, dest, 0, (unsigned)destIndex);
            }
        }

        auto align = _DL->getPrefTypeAlignment(destPtr->getType()->getPointerElementType());

        return (byVal && KindQuery::isStructType(typeToSelect))
            ? CreateMemCpy(destPtr, source, align)
            : CreateStore(destPtr, source, align);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  BlockInvoke implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    void BlockInvoke::analyzeInvokeFunction()
    {
        // Block descriptor is passed to block_invoke() as i8* in the first argument
        // then casted to the pointer to proper structure type
        // we should use this structure type in dispatcher

        _localPointerTypes.clear();

        //get the structure value which contains captured values (block_structure)
        for (auto& arg : _invokeFunc->args())
        {
            auto argType = arg.getType();
            if (argType->isPointerTy())
            {
                //catch local pointer
                if (argType->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL)
                {
                    _localPointerTypes.push_back(argType);
                }
                //catch block_descriptor pointer
                else if (argType->getPointerElementType()->isIntegerTy(8))
                {
                    for (auto arg_user : arg.users())
                    {
                        // block_descriptor is casted to struct
                        if (auto casted_block_descr = dyn_cast<llvm::CastInst>(arg_user))
                        {
                            //store it's type to captureStructType
                            auto dest_type = casted_block_descr->getDestTy();
                            while (dest_type->isPointerTy())
                            {
                                dest_type = dest_type->getPointerElementType();
                            }

                            _captureStructType = cast<llvm::StructType>(dest_type);

                            //for every getelementptr user of casted block_structure
                            for (auto casted_block_descr_user : casted_block_descr->users())
                            {
                                if (auto gepInstr = dyn_cast<llvm::GetElementPtrInst>(casted_block_descr_user))
                                {
                                    if (gepInstr->getNumIndices() == 2)
                                    {
                                        //store index 
                                        auto gepIndexValue = cast<llvm::ConstantInt>(gepInstr->getOperand(2));
                                        auto gepIndex = (GEPIndex)gepIndexValue->getZExtValue();
                                        _captureIndicies.push_back(gepIndex);
                                    }
                                }
                            }

                            //sort captureStructIndicies
                            std::sort(_captureIndicies.begin(), _captureIndicies.end());
                            auto newEnd = std::unique(_captureIndicies.begin(), _captureIndicies.end());
                            _captureIndicies.erase(newEnd, _captureIndicies.end());
                        }
                    }
                }
                else
                {
                    assert(0 && "Unacceptable block_invoke() argument");
                }
            }

        }
    }

    unsigned BlockInvoke::getPrefStructAlignment(llvm::StructType* structType, const llvm::DataLayout* dl)
    {
        auto align = dl->getPrefTypeAlignment(structType);
        for (auto elemType : structType->elements())
        {
            auto elemAlign = dl->getPrefTypeAlignment(elemType);
            if (auto subStructType = dyn_cast<llvm::StructType>(elemType))
            {
                elemAlign = getPrefStructAlignment(subStructType, dl);
            }
            align = elemAlign > align ? elemAlign : align;
        }
        return align;
    }

    llvm::CallInst* BlockInvoke::EmitBlockInvokeCall(IGCLLVM::IRBuilder<>& builder, llvm::ArrayRef<llvm::Argument*> captures, llvm::ArrayRef<llvm::Argument*> tailingArgs) const
    {
        //IRBuilder: allocate structure
        // If we didn't track the capturedStructType, it might have been not used in the kernel.
        Value* block_descriptor_val = ConstantPointerNull::get(builder.getInt8PtrTy());
        if (_captureStructType) {
            block_descriptor_val = builder.CreateAlloca(_captureStructType, nullptr, ".block_struct");
        auto dl = getFunction()->getParent()->getDataLayout();
        auto blockStructAlign = getPrefStructAlignment(_captureStructType, &dl);
            cast<AllocaInst>(block_descriptor_val)->setAlignment(blockStructAlign);
        //IRBuilder: store arguments to structure
        StoreInstBuilder storeBuilder(builder);
        for (unsigned argIdx = 0; argIdx < getCaptureIndicies().size(); ++argIdx)
        {
            auto srcArg = captures[argIdx];
            storeBuilder.Store(block_descriptor_val, srcArg, getCaptureIndicies()[argIdx]);
        }
        }

        //IRBuilder: call block_invoke
        std::vector<llvm::Value*> invoke_args{ tailingArgs.size() + 1 };

        unsigned AS = getFunction()->getFunctionType()->getFunctionParamType(0)->getPointerAddressSpace();
        invoke_args[0] = builder.CreatePointerCast(block_descriptor_val, llvm::Type::getInt8PtrTy(builder.getContext(), AS), ".block_descriptor");
        std::copy(tailingArgs.begin(), tailingArgs.end(), invoke_args.begin() + 1);

        return builder.CreateCall(const_cast<llvm::Function*>(getFunction()), invoke_args);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  Dispatcher implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    Function* Dispatcher::EmitDispatchFunction(const llvm::Twine& dispatchKernelName)
    {
        std::vector<llvm::Type*> argTypes;
        std::vector<unsigned int> byValArgs;
        auto captureIndicies = getCaptureIndicies();
        auto capturesNum = captureIndicies.size();
        for (unsigned i = 0; i < capturesNum; i++)
        {
            auto argType = _blockInvoke.getCaptureType(i);
            auto ptrToElemType = argType->getPointerTo();
            if (KindQuery::isStructType(ptrToElemType))
            {
                argType = ptrToElemType;
                byValArgs.push_back(i);
            }
            else if (KindQuery::isEventType(argType))
            {
                llvm::DataLayout DL = getBlockInvokeFunc()->getParent()->getDataLayout();
                argType = DL.getIntPtrType(argType);
            }
            argTypes.push_back(argType);
        }

        for (auto ptrType : _blockInvoke.getLocalPointerTypes())
        {
            argTypes.push_back(ptrType);
        }

        // dispatch_kernel = create signature
        llvm::FunctionType* dispatcherFunctionType = llvm::FunctionType::get(llvm::Type::getVoidTy(_context), argTypes, false);
        auto module = const_cast<llvm::Module*>(getBlockInvokeFunc()->getParent());
        _dispatchKernel = llvm::Function::Create(dispatcherFunctionType, llvm::GlobalValue::ExternalLinkage, dispatchKernelName, module);
        // Copy debug metadata from block invoke function.
        auto dbgMetadata = getBlockInvokeFunc()->getMetadata(LLVMContext::MD_dbg);
        _dispatchKernel->setMetadata(LLVMContext::MD_dbg, dbgMetadata);

        auto byValI = byValArgs.begin(), byValE = byValArgs.end();

        for (auto& arg : _dispatchKernel->args())
        {
            unsigned argNum = arg.getArgNo();
            if (argNum < capturesNum)
            {
                _captureArgs.push_back(&arg);
                arg.setName("capture" + Twine(captureIndicies[argNum]));
                // set byval attribute for struct arguments
                // note: byValArgs is filled in sorted manner
                if ((byValI != byValE) && (argNum == *byValI))
                {
                    IGCLLVM::ArgumentAddAttr(arg, IGCLLVM::AttributeSet::FunctionIndex, llvm::Attribute::ByVal);                    
                    ++byValI;
                }
            }
            else
            {
                _pointerArgs.push_back(&arg);
                arg.setName("local_ptr");
            }
        }


        //Generate body
        llvm::BasicBlock* BB = BasicBlock::Create(_context, "entry", _dispatchKernel);
        IGCLLVM::IRBuilder<> builder(BB);
        _blockInvoke.EmitBlockInvokeCall(builder, _captureArgs, _pointerArgs);
        builder.CreateRetVoid();

        return _dispatchKernel;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  Capture implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    Capture::Capture(llvm::Value* val, DataContext& context)
    {
        if (val == nullptr) report_fatal_error("captured value is null");
        value = val;
        if (auto arg = context.getArgForValue(value))
        {
            argNum = arg->getArgNo();
            kind = kindOf(arg, context);
        }
        else
        {
            argNum = ARG_NUM_NONE;
            kind = kindOf(value, context);
        }
    }

    CaptureKind Capture::kindOf(const llvm::Value* value, DataContext& context)
    {
        const llvm::Argument* arg = dyn_cast<llvm::Argument>(value);
        if (context.getKindQuery().isSamplerArg(arg))
        {
            return CaptureKind::SAMPLER;
        }

        auto valueType = value->getType();
        if (valueType->isPointerTy())
        {
            if (KindQuery::isImageType(valueType))
            {
                return CaptureKind::IMAGE;
            }
            if (KindQuery::isQueueType(valueType))
            {
                // queue_t is actually __global pointer
                return CaptureKind::POINTER;
            }
            IGC::ADDRESS_SPACE addrSpace = (IGC::ADDRESS_SPACE)valueType->getPointerAddressSpace();
            if ((addrSpace == ADDRESS_SPACE_GLOBAL || addrSpace == ADDRESS_SPACE_CONSTANT))
            {
                return CaptureKind::POINTER;
            }
        }
       
        return CaptureKind::SCALAR;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  DeviceEnqueueParamValue implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    DeviceEnqueueParamValue::DeviceEnqueueParamValue(llvm::Value* param, DataContext& dataContext) : gotCapturedValues(false)
        , _paramStruct(StructValue::get(param->stripPointerCasts()))
        , _block_invokeFunction(nullptr)
        , _dataContext(dataContext)
    {
        if (!_paramStruct) report_fatal_error("Enqueue param is not a struct");

        if (_dataContext.getKindQuery().isBlockStructType(_paramStruct->getType())) {
            /// ObjectiveC block value passed to OCL "device execution" call. 
            // On SPIR-V path it can be null - invoke function is taken from call arguments.
            _block_invokeFunction = dyn_cast_or_null<llvm::Function>(_paramStruct->getValueStoredAtIndex(BLOCK_INDEX_INVOKE_FUNC));

            assert(_block_invokeFunction == nullptr || _block_invokeFunction->getName().find("block_invoke") != llvm::StringRef::npos);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  DeviceExecCallArgs implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    DeviceEnqueueParamValue* DeviceExecCallArgs::getParamValue()
    {
        if (_paramValue == nullptr)
        {
            if (auto param = getParamArg()) {
                _paramValue = _dataContext.getDeviceEnqueueParamValue(param->stripPointerCasts());
            }
            else {
                report_fatal_error("Enqueue param is not set");
            }
        }
        return _paramValue;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //  CallHandler implementation
    /////////////////////////////////////////////////////////////////////////////////////////////////

    llvm::Function* CallHandler::getOrCreateFunc(llvm::StringRef name, llvm::Type* retType, llvm::ArrayRef<llvm::Type*> argTypes, bool isVarArg /*= false*/)
    {
        auto funcType = llvm::FunctionType::get(
            /*Result=*/retType,
            /*Params=*/argTypes,
            /*isVarArg=*/isVarArg);
        auto func = cast<llvm::Function>(_deviceExecCall->getModule()->getOrInsertFunction(name, funcType));
        assert(func != nullptr);
        func->setCallingConv(llvm::CallingConv::C);

        return func;
    }

    llvm::CallInst* CallHandler::CreateNewCall(llvm::StringRef newName, llvm::Type* retType, llvm::ArrayRef<llvm::Value*> args)
    {
        auto call = _deviceExecCall->getCall();
        llvm::Function* oldFunc = call->getCalledFunction();
        assert(oldFunc != NULL);

        std::vector<llvm::Type*> argTypes;
        for (auto arg : args)
        {
            argTypes.push_back(arg->getType());
        }

        auto newFunc = getOrCreateFunc(newName, retType, argTypes);
        newFunc->copyAttributesFrom(oldFunc);

        llvm::CallInst* newCall = llvm::CallInst::Create(newFunc, args, "", call);
        assert(newCall != NULL);
        newCall->setCallingConv(call->getCallingConv());
        newCall->setAttributes(call->getAttributes());
        if (call->isTailCall())
            newCall->setTailCall();

        newCall->setDebugLoc(call->getDebugLoc());

        return newCall;
    }

    llvm::Value* CallHandler::ReplaceCall(const Dispatcher* dispatcher)
    {
        auto call = _deviceExecCall->getCall();
        llvm::Value* newValue = getNewValue(dispatcher);
        if (!call->use_empty())
            call->replaceAllUsesWith(newValue);

        newValue->takeName(call);
        call->eraseFromParent();
        //TODO _call = nullptr;
        return newValue;
    }

    llvm::AllocaInst* EnqueueKernelCall::AllocateBuffer(llvm::Type* type, uint64_t arrSize, const llvm::Twine& name /*= ""*/)
    {
        auto arrayType = llvm::ArrayType::get(type, arrSize);
        auto allocaInst = new AllocaInst(arrayType, 0, name, &(*_deviceExecCall->getCallerFunc()->getEntryBlock().getFirstInsertionPt()));
        //TODO: fix bug in later IGC passes
        allocaInst->setAlignment(8);
        return allocaInst;
    }

    EnqueueKernelCall::BufferArguments EnqueueKernelCall::CreateBufferArguments(IGCLLVM::IRBuilder<>& builder, llvm::ArrayRef<Capture> capturedValues)
    {
        auto& context = _deviceExecCall->getContext();
        auto int32ty = Type::getInt32Ty(context);
        auto int64ty = Type::getInt64Ty(context);
        auto int8ptrty = Type::getInt8PtrTy(context);
        uint64_t scalarsBufSize = 0;
        uint64_t pointersNum = 0;
        uint64_t objectsNum = 0;
        for (auto& capturedValue : capturedValues)
        {
            auto valueType = capturedValue.value->getType();
            switch (capturedValue.kind)
            {
            case CaptureKind::SCALAR:
                if (KindQuery::isStructType(valueType))
                {
                    valueType = valueType->getPointerElementType();
                }
                scalarsBufSize += sizeInBlocks(_DL->getTypeAllocSize(valueType), int32ty);
                break;

            case CaptureKind::POINTER:
                // temporary ignore queue_t for backward compatibility
                if (!KindQuery::isQueueType(valueType))
                {
                    pointersNum++;
                }
                break;

            case CaptureKind::IMAGE:
            case CaptureKind::SAMPLER:
                objectsNum++;
                break;
                // omit "default" to prevent compilation
                // if not all cases are covered
            }
        }
        auto scalarsBuf = AllocateBuffer(int32ty, scalarsBufSize, "scalar_buf");
        auto pointersBuf = AllocateBuffer(int64ty, pointersNum, "pointer_buf");
        auto ptrMapBuf = AllocateBuffer(int32ty, pointersNum, "pointer_arg_map_buf");
        auto objectMapBuf = AllocateBuffer(int32ty, objectsNum * 2, "object_map_buf");

        uint64_t scalarsBufOffset = 0;
        uint64_t pointersBufOffset = 0;
        uint64_t ptrMapBufOffset = 0;
        uint64_t objectMapBufOffset = 0;

        // assuming _capturedValues is ordered in regard to Dispatcher arguments
        uint64_t dispatcherArgIdx = 0;
        StoreInstBuilder storeBuilder(builder);

        for (auto& capturedValue : capturedValues)
        {
            switch (capturedValue.kind)
            {
            case CaptureKind::SCALAR:
            {
                auto storedSize = storeBuilder.Store(scalarsBuf, capturedValue.value, scalarsBufOffset);
                scalarsBufOffset += sizeInBlocks(storedSize, int32ty);
            }
            break;

            case CaptureKind::POINTER:
                // temporary ignore queue_t for backward compatibility
                if (!KindQuery::isQueueType(capturedValue.value->getType()))
                {
                    auto storedSize = storeBuilder.Store(pointersBuf, capturedValue.value, pointersBufOffset, false);
                    pointersBufOffset += sizeInBlocks(storedSize, int64ty);

                    auto dispatcherArgIdxValue = llvm::ConstantInt::get(int32ty, dispatcherArgIdx);
                    storedSize = storeBuilder.Store(ptrMapBuf, dispatcherArgIdxValue, ptrMapBufOffset);
                    ptrMapBufOffset += sizeInBlocks(storedSize, int32ty);
                }
                break;

            case CaptureKind::IMAGE:
            case CaptureKind::SAMPLER:
            {
                if (Capture::ARG_NUM_NONE == capturedValue.argNum) report_fatal_error("unknown argument number for an object");
                auto objArgNumValue = llvm::ConstantInt::get(int32ty, capturedValue.argNum);
                auto getObjIDFunc = getOrCreateFunc("__builtin_IB_get_object_id", int32ty, int32ty);
                auto objIdValue = builder.CreateCall(getObjIDFunc, objArgNumValue, "obj_id");
                storeBuilder.Store(objectMapBuf, objIdValue, objectMapBufOffset + objectsNum);

                auto dispatcherArgIdxValue = llvm::ConstantInt::get(int32ty, dispatcherArgIdx);
                auto storedSize = storeBuilder.Store(objectMapBuf, dispatcherArgIdxValue, objectMapBufOffset);
                objectMapBufOffset += sizeInBlocks(storedSize, int32ty);
            }
            break;
            // omit "default" to prevent compilation if not all cases are covered
            }
            dispatcherArgIdx++;
        }

        assert(scalarsBufSize == scalarsBufOffset);
        assert(pointersNum == pointersBufOffset);
        assert(pointersNum == ptrMapBufOffset);
        assert(objectsNum == objectMapBufOffset);

        llvm::Value* localSizesBuf = nullptr;
        llvm::Value* localSizesNumValue = nullptr;
        if (_deviceExecCall->hasLocals())
        {
            auto int32ptrty = Type::getInt32PtrTy(context);
            auto localsBuf = AllocateBuffer(int32ty, _deviceExecCall->getLocalSizes().size(), "local_size_buf");
            uint64_t localSizeOffset = 0;
            for (auto localSizeValue : _deviceExecCall->getLocalSizes())
            {
                auto storedSize = storeBuilder.Store(localsBuf, localSizeValue, localSizeOffset);
                localSizeOffset += sizeInBlocks(storedSize, int32ty);
            }
            assert(_deviceExecCall->getLocalSizes().size() == localSizeOffset);

            localSizesBuf = builder.CreatePointerCast(localsBuf, int32ptrty);
            localSizesNumValue = llvm::ConstantInt::get(int32ty, localSizeOffset);
        }

        return BufferArguments{
            builder.CreatePointerCast(scalarsBuf, int8ptrty, "scalarParamBuf"),          // __private void* scalarParamBuf,
            llvm::ConstantInt::get(int32ty, scalarsBufSize * 4),                         // unsigned sizeofscalarParamBuf,
            builder.CreatePointerCast(pointersBuf, int8ptrty, "globalArgBuf"),           // __private void* globalArgBuf,
            llvm::ConstantInt::get(int32ty, pointersNum),                                // unsigned  numGlobalArgBuf,
            localSizesBuf,                                                               // __private int* local_size_buf,
            localSizesNumValue,                                                               // uint sizeof_local_size_buf,
            builder.CreatePointerCast(ptrMapBuf, int8ptrty, "globalPtrArgMappingBuf"),   // __private void* globalPtrArgMappingBuf,
            builder.CreatePointerCast(objectMapBuf, int8ptrty, "getobjectidMappingBuf"), // __private void* getobjectidMappingBuf,
            llvm::ConstantInt::get(int32ty, objectsNum),                                 // unsigned  numArgMappings
        };
    }

    llvm::Value* EnqueueKernelCall::getNewValue(const Dispatcher* dispatcher)
    {
        //New call signature:
        //
        //INLINE int enqueue_IB_kernel(...
        //or
        //INLINE int enqueue_IB_kernel_local(...
        //or
        //INLINE int enqueue_IB_kernel_events(...
        //or
        //INLINE int enqueue_IB_kernel_local_events(...
        SmallString<64> newFuncName{ "enqueue_IB_kernel" };
        if (_deviceExecCall->hasLocals())
        {
            newFuncName.append("_local");
        }
        if (_deviceExecCall->hasEvents())
        {
            newFuncName.append("_events");
        }

        IGCLLVM::IRBuilder<> builder(_deviceExecCall->getCall());
        auto capturedValues = _deviceExecCall->getParamValue()->getCapturedValues(dispatcher->getCaptureIndicies());
        auto buffers = CreateBufferArguments(builder, capturedValues);

        //...( queue_t q, kernel_enqueue_flags_t flags, const ndrange_t range,...
        std::vector<llvm::Value*> args{ _deviceExecCall->getQueue(), _deviceExecCall->getEnqueueFlags(), AdjustNDRangeType(_deviceExecCall->getNDRange()) };

        //..._events(..., uint numEventsInWaitList, const clk_event_t* waitList, clk_event_t* returnEvent, ...
        if (_deviceExecCall->hasEvents())
        {
            auto CastValue = [&](Value* pEvt)
            {
                auto *pNewType = PointerType::get(pEvt->getType()->getPointerElementType(), ADDRESS_SPACE_PRIVATE);
                auto *pCasted  = builder.CreatePointerBitCastOrAddrSpaceCast(pEvt, pNewType);
                return pCasted;
            };

            args.push_back(_deviceExecCall->getNumWaitEvents());
            args.push_back(CastValue(_deviceExecCall->getWaitEventsList()));
            args.push_back(CastValue(_deviceExecCall->getRetEvent()));
        }

        //...(..., unsigned block_id, ...
        auto int32ty = Type::getInt32Ty(_deviceExecCall->getContext());
        auto blockIdValue = llvm::ConstantInt::get(int32ty, dispatcher->getBlockId());
        args.push_back(blockIdValue);

        //...(..., void* scalarParamBuf, unsigned sizeofscalarParamBuf, void* globalArgBuf, unsigned  numGlobalArgBuf,...
        args.push_back(buffers.scalarParamBuf);
        args.push_back(buffers.sizeofscalarParamBuf);
        args.push_back(buffers.globalArgBuf);
        args.push_back(buffers.numGlobalArgBuf);

        //..._locals...(..., int* local_size_buf, uint sizeof_local_size_buf,...
        if (_deviceExecCall->hasLocals())
        {
            assert(buffers.local_size_buf != nullptr);
            assert(buffers.sizeof_local_size_buf != nullptr);
            args.push_back(buffers.local_size_buf);
            args.push_back(buffers.sizeof_local_size_buf);
        }

        //...(..., void* globalPtrArgMappingBuf, __private void* getobjectidMappingBuf, unsigned  numArgMappings )
        args.push_back(buffers.globalPtrArgMappingBuf);
        args.push_back(buffers.getobjectidMappingBuf);
        args.push_back(buffers.numArgMappings);

        return CreateNewCall(newFuncName, int32ty, args);
    }

} //namespace

// Register pass to igc-opt
#define PASS_FLAG "igc-block-transform"
#define PASS_DESCRIPTION "Analyzes device enqueue functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(TransformBlocks, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(TransformBlocks, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char TransformBlocks::ID = 0;

extern "C" llvm::ModulePass* createTransformBlocksPass()
{
    return new TransformBlocks;
}

