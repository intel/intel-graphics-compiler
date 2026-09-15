// REQUIRES: regkeys, pvc-supported, llvm-16-plus
// RUN: ocloc compile -file %s -options " -cl-std=CL2.0 -igc_opts 'EnableOpaquePointersBackend=1 ManageableBarriersMode=1 PrintToConsole=1 PrintAfter=ManageableBarriersResolution'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK

///////////////////////////////////////////
//// ManageableBarriersInitINTEL resolution
//// Allocation for the ManageableBarriers data in SLM
// CHECK: [[MB_DATA_SLM:@[0-9]+]] = internal addrspace(3) global [500 x i8]
// CHECK: [[MB_DATA_PTR:%[0-9]+]] = bitcast ptr addrspace(3) [[MB_DATA_SLM]] to ptr addrspace(3)

//// Get pointer to the ManageableBarriers ID Pool
// CHECK: [[BarrierIDPool_GetIntPtr_MB_DATA:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_PTR]] to i32
// CHECK: [[BarrierIDPool_GetIntPtr_Offset:%[0-9]+]] = add i32 [[BarrierIDPool_GetIntPtr_MB_DATA]], 496
// CHECK: [[BarrierIDPool_Ptr:%[0-9]+]] = inttoptr i32 [[BarrierIDPool_GetIntPtr_Offset]] to ptr addrspace(3)

//// Fill the ManageableBarriers ID Pool with init-value
// CHECK: store i32 -2, ptr addrspace(3) [[BarrierIDPool_Ptr]], align 4

//// Jump to the basic block with initialization of the ManageableBarriers struct in SLM
// CHECK: [[THREAD_ID:%[0-9]+]] = call i32 @__builtin_IB_get_local_thread_id()
// CHECK: icmp eq i32 [[THREAD_ID]], 0

//// Get first free ID for the ManageableBarriers
// CHECK: [[BarrierIDPool_Load:%[0-9]+]] = load i32, ptr addrspace(3) [[BarrierIDPool_Ptr]], align 4
// CHECK: [[BarrierIDPool_FirstFreeID:%[0-9]+]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[BarrierIDPool_Load]])

//// Calculate the offset in the ManageableBarriers data in SLM, base on the FreeID
// CHECK: [[MB_DATA_GetIntPTR:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_PTR]] to i32
// CHECK: [[BarrierIDPool_FirstFreeID_Offset:%[0-9]+]] = mul i32 [[BarrierIDPool_FirstFreeID]], 16
// CHECK: [[MB_DATA_OFFSET_IntPTR:%[0-9]+]] = add i32 [[BarrierIDPool_FirstFreeID_Offset]], [[MB_DATA_GetIntPTR]]
// CHECK: [[MB_DATA_OFFSET_PTR:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_IntPTR]] to ptr addrspace(3)

//// Fill with data for the ManageBarrier struct in SLM (for the particular single ManageableBarrier)
//// Fill the FreeID in the ManageableBarriers Data ID offset
// CHECK: [[MB_DATA_OFFSET_ID_GetIntPTR:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_ID_GetIntOffsetPTR:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_ID_GetIntPTR]], 0
// CHECK: [[MB_DATA_OFFSET_ID_PTR8:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_ID_GetIntOffsetPTR]] to ptr addrspace(3)
// CHECK: store i32 [[BarrierIDPool_FirstFreeID]], ptr addrspace(3) [[MB_DATA_OFFSET_ID_PTR8]], align 4

//// Fill the producer count in the ManageableBarriers Data ProducerCount offset
// CHECK: [[MB_DATA_OFFSET_PrdCnt_GetIntPTR:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_PrdCnt_GetIntOffsetPTR:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_PrdCnt_GetIntPTR]], 4
// CHECK: [[MB_DATA_OFFSET_PrdCnt_PTR8:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_PrdCnt_GetIntOffsetPTR]] to ptr addrspace(3)
// CHECK: store i32 8, ptr addrspace(3) [[MB_DATA_OFFSET_PrdCnt_PTR8]], align 4

//// Fill the consumer count in the ManageableBarriers Data ConsumerCount offset
// CHECK: [[MB_DATA_OFFSET_CnsCnt_GetIntPTR:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_CnsCnt_GetIntOffsetPTR:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_CnsCnt_GetIntPTR]], 8
// CHECK: [[MB_DATA_OFFSET_CnsCnt_PTR8:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_CnsCnt_GetIntOffsetPTR]] to ptr addrspace(3)
// CHECK: store i32 8, ptr addrspace(3) [[MB_DATA_OFFSET_CnsCnt_PTR8]], align 4

//// Fill the expected arrvial count in the ManageableBarriers Data ExpectedArrvial offset
// CHECK: [[MB_DATA_OFFSET_ExpArv_GetIntPTR:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_ExpArv_GetIntOffsetPTR:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_ExpArv_GetIntPTR]], 12
// CHECK: [[MB_DATA_OFFSET_ExpArv_PTR8:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_ExpArv_GetIntOffsetPTR]] to ptr addrspace(3)
// CHECK: store i32 8, ptr addrspace(3) [[MB_DATA_OFFSET_ExpArv_PTR8]], align 4

//// Setup workgroup barrier
// CHECK: call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i32 0)
// CHECK: call void @llvm.genx.GenISA.threadgroupbarrier()

//// Mark the FreeID in the ManageableBarriers ID Pool (that this ID is busy)
// CHECK: [[FreeID_InBit:%[0-9]+]] = shl i32 1, [[BarrierIDPool_FirstFreeID]]
// CHECK: [[FreeID_InBitNeg:%[0-9]+]] = xor i32 [[FreeID_InBit]], -1
// CHECK: [[BarrierIDPool_UpdateBits:%[0-9]+]] = and i32 [[FreeID_InBitNeg]], [[BarrierIDPool_Load]]
// CHECK: store i32 [[BarrierIDPool_UpdateBits]], ptr addrspace(3) [[BarrierIDPool_Ptr]], align 4

/////////////////////////////////////////////
//// ManageableBarriersArriveINTEL resolution
/// Get the data for this barrier
// CHECK: [[MB_DATA_OFFSET_GetIntPTR_Arrive:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_GetIntOffsetPTR_Arrive:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_GetIntPTR_Arrive]], 0
// CHECK: [[MB_DATA_OFFSET_PTR8_Arrive:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_GetIntOffsetPTR_Arrive]] to ptr addrspace(3)
// CHECK: [[MB_DATA_OFFSET_Arrive_Int32v4:%[0-9]+]] = load [4 x i32], ptr addrspace(3) [[MB_DATA_OFFSET_PTR8_Arrive]], align 4

//// Get the FreeID in the ManageableBarriers Data ID offset
// CHECK: [[MB_DATA_OFFSET_ID_Arrive_Int32:%[0-9]+]] = extractvalue [4 x i32] [[MB_DATA_OFFSET_Arrive_Int32v4]], 0

//// Get the producer count in the ManageableBarriers Data ProducerCount offset
// CHECK: [[MB_DATA_OFFSET_PrdCnt_Arrive_Int32:%[0-9]+]] = extractvalue [4 x i32] [[MB_DATA_OFFSET_Arrive_Int32v4]], 1

//// Get the consumer count in the ManageableBarriers Data ConsumerCount offset
// CHECK: [[MB_DATA_OFFSET_CnsCnt_Arrive_Int32:%[0-9]+]] = extractvalue [4 x i32] [[MB_DATA_OFFSET_Arrive_Int32v4]], 2

//// Truncate the data to int8
// CHECK: [[MB_DATA_OFFSET_ID_Arrive_Int8:%[0-9]+]] = trunc i32 [[MB_DATA_OFFSET_ID_Arrive_Int32]] to i8
// CHECK: [[MB_DATA_OFFSET_PrdCnt_Arrive_Int8:%[0-9]+]] = trunc i32 [[MB_DATA_OFFSET_PrdCnt_Arrive_Int32]] to i8
// CHECK: [[MB_DATA_OFFSET_CnsCnt_Arrive_Int8:%[0-9]+]] = trunc i32 [[MB_DATA_OFFSET_CnsCnt_Arrive_Int32]] to i8

//// Call named barrier singal with producer mark
// CHECK: call void @llvm.genx.GenISA.threadgroupnamedbarriers.signal(i8 [[MB_DATA_OFFSET_ID_Arrive_Int8]], i16 1, i8 [[MB_DATA_OFFSET_PrdCnt_Arrive_Int8]], i8 [[MB_DATA_OFFSET_CnsCnt_Arrive_Int8]])

///////////////////////////////////////////
//// ManageableBarriersWaitINTEL resolution
/// Get the data for this barrier
// CHECK: [[MB_DATA_OFFSET_GetIntPTR_Wait:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_GetIntOffsetPTR_Wait:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_GetIntPTR_Wait]], 0
// CHECK: [[MB_DATA_OFFSET_PTR8_Wait:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_GetIntOffsetPTR_Wait]] to ptr addrspace(3)
// CHECK: [[MB_DATA_OFFSET_Wait_Int32v4:%[0-9]+]] = load [4 x i32], ptr addrspace(3) [[MB_DATA_OFFSET_PTR8_Wait]], align 4

//// Get the FreeID in the ManageableBarriers Data ID offset
// CHECK: [[MB_DATA_OFFSET_ID_Wait_Int32:%[0-9]+]] = extractvalue [4 x i32] [[MB_DATA_OFFSET_Wait_Int32v4]], 0

//// Get the producer count in the ManageableBarriers Data ProducerCount offset
// CHECK: [[MB_DATA_OFFSET_PrdCnt_Wait_Int32:%[0-9]+]] = extractvalue [4 x i32] [[MB_DATA_OFFSET_Wait_Int32v4]], 1

//// Get the consumer count in the ManageableBarriers Data ConsumerCount offset
// CHECK: [[MB_DATA_OFFSET_CnsCnt_Wait_Int32:%[0-9]+]] = extractvalue [4 x i32] [[MB_DATA_OFFSET_Wait_Int32v4]], 2

//// Truncate the data to int8
// CHECK: [[MB_DATA_OFFSET_ID_Wait_Int8:%[0-9]+]] = trunc i32 [[MB_DATA_OFFSET_ID_Wait_Int32]] to i8
// CHECK: [[MB_DATA_OFFSET_PrdCnt_Wait_Int8:%[0-9]+]] = trunc i32 [[MB_DATA_OFFSET_PrdCnt_Wait_Int32]] to i8
// CHECK: [[MB_DATA_OFFSET_CnsCnt_Wait_Int8:%[0-9]+]] = trunc i32 [[MB_DATA_OFFSET_CnsCnt_Wait_Int32]] to i8

//// Call named barrier singal with consumer mark
// CHECK: call void @llvm.genx.GenISA.threadgroupnamedbarriers.signal(i8 [[MB_DATA_OFFSET_ID_Wait_Int8]], i16 2, i8 [[MB_DATA_OFFSET_PrdCnt_Wait_Int8]], i8 [[MB_DATA_OFFSET_CnsCnt_Wait_Int8]])

//// Truncate the data to int8
// CHECK: [[MB_DATA_OFFSET_ID_Wait2_Int8:%[0-9]+]] = trunc i32 [[MB_DATA_OFFSET_ID_Wait_Int32]] to i8

//// Call named barrier wait
// CHECK: call void @llvm.genx.GenISA.threadgroupnamedbarriers.wait(i8 [[MB_DATA_OFFSET_ID_Wait2_Int8]])

//// Update ProducerCount via rewriting it with ExpectedArrvial value:

//// Get the expected arrvial count in the ManageableBarriers Data ExpectedArrvial offset
// CHECK: [[MB_DATA_OFFSET_ExpArv_Wait_Int32:%[0-9]+]] = extractvalue [4 x i32] [[MB_DATA_OFFSET_Wait_Int32v4]], 3

//// Get the producer count in the ManageableBarriers Data ProducerCount offset
// CHECK: [[MB_DATA_OFFSET_PrdCnt_GetIntPTR_Wait2:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_PrdCnt_GetIntOffsetPTR_Wait2:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_PrdCnt_GetIntPTR_Wait2]], 4
// CHECK: [[MB_DATA_OFFSET_PrdCnt_PTR8_Wait2:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_PrdCnt_GetIntOffsetPTR_Wait2]] to ptr addrspace(3)

//// Store the ExpectedArrvial value under ProducerCount pointer
// CHECK: store i32 [[MB_DATA_OFFSET_ExpArv_Wait_Int32]], ptr addrspace(3) [[MB_DATA_OFFSET_PrdCnt_PTR8_Wait2]], align 4

//////////////////////////////////////////////
//// ManageableBarriersReleaseINTEL resolution
//// Get the FreeID in the ManageableBarriers Data ID offset
// CHECK: [[MB_DATA_OFFSET_ID_GetIntPTR_Release:%[0-9]+]] = ptrtoint ptr addrspace(3) [[MB_DATA_OFFSET_PTR]] to i32
// CHECK: [[MB_DATA_OFFSET_ID_GetIntOffsetPTR_Release:%[0-9]+]] = add i32 [[MB_DATA_OFFSET_ID_GetIntPTR_Release]], 0
// CHECK: [[MB_DATA_OFFSET_ID_PTR8_Release:%[0-9]+]] = inttoptr i32 [[MB_DATA_OFFSET_ID_GetIntOffsetPTR_Release]] to ptr addrspace(3)
// CHECK: [[MB_DATA_OFFSET_ID_Release_Int32:%[0-9]+]] = load i32, ptr addrspace(3) [[MB_DATA_OFFSET_ID_PTR8_Release]], align 4

// CHECK: [[FreeID_InBit2:%[0-9]+]] = shl i32 1, [[MB_DATA_OFFSET_ID_Release_Int32]]

//// Xor the FreeID on bits with the BarrierIDPool
// CHECK: call i32 @llvm.genx.GenISA.intatomicrawsinglelane.i32.p3.i32(ptr addrspace(3) [[BarrierIDPool_Ptr]], i32 [[BarrierIDPool_GetIntPtr_Offset]], i32 [[FreeID_InBit2]], i32 9)

#pragma OPENCL EXTENSION cl_khr_subgroups : enable

__kernel void ManageableBarriers_test1(__global int* input, __global int* output)
{
    const uint ProdCnt = 8;
    const uint ConsCnt = 8;

    __local int tempData[ProdCnt];

    manageable_barrier_t* b1 = intel_manageable_barrier_init(ProdCnt, ConsCnt);

    uint subgroupID = get_sub_group_id();
    uint workItemID_subgroup = get_sub_group_local_id();

    if(subgroupID < ProdCnt)
    {
        if(workItemID_subgroup == 0)
        {
            tempData[subgroupID] = input[subgroupID] * input[subgroupID];
        }
        intel_manageable_barrier_arrive(b1);
    }
    else if(subgroupID < ProdCnt + ConsCnt)
    {
        intel_manageable_barrier_wait(b1);

        intel_manageable_barrier_release(b1);

        if(workItemID_subgroup == 0)
        {
            uint checkIndex = subgroupID - ProdCnt;

            output[checkIndex] = tempData[checkIndex];
        }
    }
}
