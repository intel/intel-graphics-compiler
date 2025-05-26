// REQUIRES: regkeys, pvc-supported, llvm-16-plus
// RUN: ocloc compile -file %s -options " -cl-std=CL2.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintAfter=ManageableBarriersResolution'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK

//// Check if we have atomic to decrease the count of producers
// CHECK: call i32 @llvm.genx.GenISA.intatomicrawsinglelane.i32.p3.i32(ptr addrspace(3) [[PTR:%[0-9]+]], i32 [[INT:%[0-9]+]], i32 0, i32 3)

#pragma OPENCL EXTENSION cl_khr_subgroups : enable

__kernel void ManageableBarriers_test1(__global int* input, __global int* output)
{
    const uint ProdCnt = 8;
    const uint ConsCnt = 8;

    __local int tempData[ProdCnt + 8 /*We needd only extra 4 for the next iteration*/];

    manageable_barrier_t* b1 = intel_manageable_barrier_init(ProdCnt, ConsCnt);

    uint subgroupID = get_sub_group_id();
    uint workItemID_subgroup = get_sub_group_local_id();

    if(subgroupID < 4)
    {
        if(workItemID_subgroup == 0)
        {
            tempData[subgroupID] = input[subgroupID] * input[subgroupID];
        }
        intel_manageable_barrier_arrive(b1);
    }
    else if(subgroupID < 8)
    {
        if(workItemID_subgroup == 0)
        {
            tempData[subgroupID] = input[subgroupID] + input[subgroupID];
        }
        intel_manageable_barrier_arrivedrop(b1);
    }
    else if(subgroupID < ProdCnt + ConsCnt)
    {
        intel_manageable_barrier_wait(b1);

        if(workItemID_subgroup == 0)
        {
            uint checkIndex = subgroupID - ProdCnt;

            output[checkIndex] = tempData[checkIndex];
        }
    }

    // As we are re-using the same HW threads in the same barrier
    // we need to switch the roles for each of them

    if(subgroupID >= 8 && subgroupID <= 12)
    {
        if(workItemID_subgroup == 0)
        {
            tempData[subgroupID] = input[subgroupID] + input[subgroupID];
        }
        intel_manageable_barrier_arrivedrop(b1);
    }
    else if(subgroupID < 8)
    {
        intel_manageable_barrier_wait(b1);
        intel_manageable_barrier_release(b1);

        if(workItemID_subgroup == 0)
        {
            uint checkIndex = subgroupID + ProdCnt;

            output[checkIndex] = tempData[checkIndex];
        }
    }
}