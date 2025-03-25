/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Tester.hpp"
#include "ZEELFObjectBuilder.hpp"
#include "ZEinfoYAML.hpp"

#include <fstream>

using llvm::yaml::Output;
using llvm::yaml::Input;
using namespace zebin;

static void getTestZEInfo(zeInfoContainer& ks)
{
    zeInfoKernel k1;

    k1.name = "kernel_name_1";
    k1.execution_env.actual_kernel_start_offset = 0;
    k1.execution_env.grf_count = 128;
    k1.execution_env.simd_size = 8;
    k1.execution_env.required_work_group_size.push_back(256);
    k1.execution_env.required_work_group_size.push_back(2);
    k1.execution_env.required_work_group_size.push_back(1);

    zeInfoPerThreadPayloadArgument p_arg;
    p_arg.arg_type = "local_id";
    p_arg.offset = 0;
    p_arg.size = 96;

    k1.per_thread_payload_arguments.push_back(p_arg);

    zeInfoPayloadArgument imp_arg1;
    imp_arg1.arg_type = "local_size";
    imp_arg1.offset = 0;
    imp_arg1.size = 12;
    k1.payload_arguments.push_back(imp_arg1);

    zeInfoPayloadArgument imp_arg2;
    imp_arg2.arg_type = "group_size";
    imp_arg2.offset = 12;
    imp_arg2.size = 12;
    k1.payload_arguments.push_back(imp_arg2);

    zeInfoPayloadArgument imp_arg3;
    imp_arg3.arg_type = "global_id_offset";
    imp_arg3.offset = 24;
    imp_arg3.size = 12;
    k1.payload_arguments.push_back(imp_arg3);

    zeInfoPayloadArgument arg1;
    arg1.arg_type = "arg_pointer";
    arg1.offset = 64;
    arg1.size = 8;
    arg1.arg_index = 0;
    arg1.addrmode = "stateless";
    arg1.addrspace = "global";
    arg1.access_type = "readwrite";
    k1.payload_arguments.push_back(arg1);

    zeInfoPayloadArgument arg2;
    arg2.arg_type = "arg_pointer";
    arg2.offset = 0;
    arg2.size = 8;
    arg2.arg_index = 0;
    arg2.addrmode = "stateful";
    arg2.addrspace = "global";
    arg2.access_type = "readwrite";
    k1.payload_arguments.push_back(arg2);

    zeInfoBindingTableIndex bti;
    bti.bti_value = 0;
    bti.arg_index = 0;
    k1.binding_table_indices.push_back(bti);

    zeInfoBindingTableIndex bti2;
    bti2.bti_value = 5;
    bti2.arg_index = 10;
    k1.binding_table_indices.push_back(bti2);

    zeInfoKernel k2;
    k2.name = "kernel_name_2";
    k2.execution_env.actual_kernel_start_offset = 0;
    k2.execution_env.grf_count = 100;
    k2.execution_env.simd_size = 16;

    zeInfoPerThreadPayloadArgument p_arg1;
    p_arg1.arg_type = "local_id";
    p_arg1.offset = 10;
    p_arg1.size = 100;

    k2.per_thread_payload_arguments.push_back(p_arg1);

    zeInfoPerThreadPayloadArgument p_arg2;
    p_arg2.arg_type = "local_size";
    p_arg2.offset = 20;
    p_arg2.size = 30;

    k2.per_thread_payload_arguments.push_back(p_arg2);

    ks.kernels.push_back(k1);
    ks.kernels.push_back(k2);
}

void Tester::testZEInfoOutput()
{
    zeInfoContainer in_ks;
    getTestZEInfo(in_ks);
    std::string in_string;
    llvm::raw_string_ostream OS(in_string);
    Output yout(OS);
    yout << in_ks;

    zeInfoContainer out_ks;
    Input Yin(OS.str());
    Yin >> out_ks;

    std::string out_string;
    llvm::raw_string_ostream out_OS(out_string);
    Output out_yout(out_OS);
    out_yout << out_ks;
}

void Tester::testELFOutput()
{
    TargetFlags flag;
    flag.packed = 10;
    ZEELFObjectBuilder builder(false, ET_ZEBIN_EXE, 0, flag);

    // add fake text
    uint8_t text_buff[100] = { 0x1, 0x2, 0x3, 0x4 };
    uint32_t text =
        builder.addSectionText(".text.kernel", (uint8_t*)text_buff, 10, 0, 0);

    // add fake data 1
    uint8_t data_buff_1[4] = { 0x1, 0x2, 0x3, 0x4 };
    uint32_t data1 = builder.addSectionData(".data.buff_1", data_buff_1, 4);

    // add fake data 2
    uint8_t data_buff_2[2] = { 0x5, 0x6 };
    uint32_t data2 = builder.addSectionData(".data.buff_2", data_buff_2, 2);

    // add fake symbols to text
    builder.addSymbol("text_sym_at_0", 0, 15, llvm::ELF::STB_GLOBAL, llvm::ELF::STT_OBJECT, text);
    builder.addSymbol("text_sym_at_1", 5, 10, llvm::ELF::STB_GLOBAL, llvm::ELF::STT_OBJECT, text);
    // add fake symbols to data
    builder.addSymbol("data1_sym_at_3", 3, 2, llvm::ELF::STB_WEAK, llvm::ELF::STT_OBJECT, data1);
    builder.addSymbol("data1_sym_at_0", 0, 1, llvm::ELF::STB_GLOBAL, llvm::ELF::STT_OBJECT, data1);
    builder.addSymbol("data2_sym_at_1", 1, 1, llvm::ELF::STB_GLOBAL, llvm::ELF::STT_OBJECT, data2);
    builder.addSymbol("undef_sym", 0, 0, llvm::ELF::STB_GLOBAL, llvm::ELF::STT_OBJECT, -1);

    // add fake relocations
    builder.addRelocation(4, "data1_sym_at_3", R_TYPE_ZEBIN::R_ZE_SYM_ADDR, text);
    builder.addRelocation(8, "text_sym_at_1", R_TYPE_ZEBIN::R_ZE_SYM_ADDR_32, text);

    // add fake ze_info
    zeInfoContainer ks;
    getTestZEInfo(ks);
    builder.addSectionZEInfo(ks);

    std::error_code EC;
    llvm::raw_fd_ostream os("testELFOutput", EC);
    builder.finalize(os);
    os.close();
}
