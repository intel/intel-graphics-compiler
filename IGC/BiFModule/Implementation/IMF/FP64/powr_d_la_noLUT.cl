/*===================== begin_copyright_notice ==================================

Copyright (c) 2022 Intel Corporation

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

#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF

static __constant _iml_dp_union_t __dpowr_la_nolut_CoutTab[860] = {
    0x00000000, 0x3FF00000,
    0x00000000, 0x3FEF07C0,
    0x00000000, 0x3FEE1E00,
    0x00000000, 0x3FED41C0,
    0x00000000, 0x3FEC71C0,
    0x00000000, 0x3FEBAD00,
    0x00000000, 0x3FEAF280,
    0x00000000, 0x3FEA41C0,
    0x00000000, 0x3FE99980,
    0x00000000, 0x3FE8F9C0,
    0x00000000, 0x3FE86180,
    0x00000000, 0x3FE7D040,
    0x00000000, 0x3FE745C0,
    0x00000000, 0x3FE6C180,
    0x00000000, 0x3FE642C0,
    0x00000000, 0x3FE5C980,
    0x00000000, 0x3FE55540,
    0x00000000, 0x3FE4E600,
    0x00000000, 0x3FE47B00,
    0x00000000, 0x3FE41400,
    0x00000000, 0x3FE3B140,
    0x00000000, 0x3FE35200,
    0x00000000, 0x3FE2F680,
    0x00000000, 0x3FE29E40,
    0x00000000, 0x3FE24940,
    0x00000000, 0x3FE1F700,
    0x00000000, 0x3FE1A7C0,
    0x00000000, 0x3FE15B00,
    0x00000000, 0x3FE11100,
    0x00000000, 0x3FE0C980,
    0x00000000, 0x3FE08440,
    0x00000000, 0x3FE04100,
    0x00000000, 0x3FE00000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0xA01F0000, 0x3FA6BB01,
    0x7439DB71, 0x3D3C995E,
    0x4FF80000, 0x3FB66568,
    0x8DA93FB0, 0x3D3084F2,
    0x820EC000, 0x3FC08CD7,
    0x11B40207, 0x3D3543C5,
    0x64906000, 0x3FC5C048,
    0x7E5F3668, 0x3D28C5D4,
    0x032BE000, 0x3FCACF30,
    0xDEBF9166, 0x3D2E3733,
    0xE396E000, 0x3FCFBC44,
    0x28665438, 0x3D47B3F9,
    0x90D2B000, 0x3FD243A5,
    0xE8E9D45D, 0x3D3C9B75,
    0xA118D000, 0x3FD49AD4,
    0x7302CCA6, 0x3D45CD37,
    0x92EF1000, 0x3FD6E227,
    0x0E7E9039, 0x3D314F24,
    0x9E695000, 0x3FD91BD1,
    0xE4F6C667, 0x3D4DBB3E,
    0x273ED000, 0x3FDB4865,
    0x099E1F61, 0x3D4AB54A,
    0x20231000, 0x3FDD6799,
    0x96E87504, 0x3D18ED50,
    0x9E747000, 0x3FDF7A34,
    0x81D99120, 0x3D4A6E70,
    0x50CF0000, 0x3FE0C116,
    0xEB1152A5, 0x3D461752,
    0x6E8E9800, 0x3FE1BF42,
    0x6C055F56, 0x3D376AFF,
    0x1C354000, 0xBFDA8F9D,
    0x4F4F9854, 0xBD4604F5,
    0x8A043000, 0xBFD8A922,
    0xCF8DD884, 0xBD49BC20,
    0xE7AA4000, 0xBFD6CB99,
    0xD5A7002B, 0xBD412B5A,
    0x5D830000, 0xBFD4F69F,
    0xD24BAE46, 0xBD38F36D,
    0xF8B57000, 0xBFD32C15,
    0xE01D9232, 0xBD0D6EE2,
    0xD34FB000, 0xBFD16935,
    0x348D84A5, 0xBD2151C6,
    0x7E71A000, 0xBFCF5FAA,
    0x20C552C2, 0xBD3D1576,
    0x1D5BE000, 0xBFCBFC5C,
    0x0E42B538, 0xBD278490,
    0x0948A000, 0xBFC8A9AD,
    0x64F25A56, 0xBD4C89BA,
    0xFF6B0000, 0xBFC563AD,
    0x079422C3, 0xBD4D0837,
    0x02746000, 0xBFC22DF3,
    0xC2505D3D, 0xBD3048E3,
    0x92EE4000, 0xBFBE0894,
    0xFCD57F87, 0xBD405589,
    0x41A08000, 0xBFB7D493,
    0xBCF7AA55, 0xBD4EEEF8,
    0xCF5A4000, 0xBFB1BC75,
    0x139E8397, 0xBD4D7DB2,
    0xC1828000, 0xBFA778FD,
    0xF2AF5333, 0xBD34378A,
    0x317A0000, 0xBF97427D,
    0x4B03B094, 0xBD4700F1,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
    0x00000000, 0x3FEFF000,
    0x00000000, 0x3FEFE000,
    0x00000000, 0x3FEFD040,
    0x00000000, 0x3FEFC080,
    0x00000000, 0x3FEFB0C0,
    0x00000000, 0x3FEFA100,
    0x00000000, 0x3FEF9180,
    0x00000000, 0x3FEF8200,
    0x00000000, 0x3FEF7280,
    0x00000000, 0x3FEF6300,
    0x00000000, 0x3FEF53C0,
    0x00000000, 0x3FEF4480,
    0x00000000, 0x3FEF3540,
    0x00000000, 0x3FEF2600,
    0x00000000, 0x3FF04540,
    0x00000000, 0x3FF04100,
    0x00000000, 0x3FF03D00,
    0x00000000, 0x3FF038C0,
    0x00000000, 0x3FF034C0,
    0x00000000, 0x3FF03080,
    0x00000000, 0x3FF02C80,
    0x00000000, 0x3FF02880,
    0x00000000, 0x3FF02440,
    0x00000000, 0x3FF02040,
    0x00000000, 0x3FF01C40,
    0x00000000, 0x3FF01840,
    0x00000000, 0x3FF01400,
    0x00000000, 0x3FF01000,
    0x00000000, 0x3FF00C00,
    0x00000000, 0x3FF00800,
    0x00000000, 0x3FF00400,
    0x00000000, 0x3FF00000,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0xA4280000, 0x3F671B0E,
    0x9876EF47, 0x3D497F68,
    0xC0680000, 0x3F7720D9,
    0x3778C7CC, 0x3D441AF5,
    0x8A7A0000, 0x3F8145C6,
    0x9AC06488, 0x3D44BDD1,
    0x61D20000, 0x3F86FDF4,
    0x46D9F6F7, 0x3D3C9EFC,
    0x355C0000, 0x3F8CB8F9,
    0xADFBB459, 0x3D3275C9,
    0xEC350000, 0x3F913B6B,
    0x4FC32ADB, 0x3D0F25B4,
    0x7B760000, 0x3F941016,
    0xA8ED5317, 0x3D3F880C,
    0x317A0000, 0x3F96E625,
    0x645614DB, 0x3D453F0E,
    0x6D010000, 0x3F99BD99,
    0x341A2DAB, 0x3D2CD686,
    0x8ED00000, 0x3F9C9674,
    0x66D10B04, 0x3D4EF88D,
    0xD4200000, 0x3F9F64ED,
    0x828828DA, 0x3D4511C3,
    0x97920000, 0x3FA11A62,
    0xD7D436D6, 0x3D4D925C,
    0xFAD70000, 0x3FA282FD,
    0xA58B8D6E, 0x3D49EEE0,
    0xBFC20000, 0x3FA3EC49,
    0x2E0E0086, 0x3D4DEAC3,
    0x31230000, 0xBF98C493,
    0x5EFCABFA, 0xBD49AD07,
    0x317A0000, 0xBF97427D,
    0x4B03B094, 0xBD4700F1,
    0xEE910000, 0xBF95D6C0,
    0x535202A3, 0xBD4A5115,
    0xADB60000, 0xBF9453E6,
    0xE102F731, 0xBD415A44,
    0x57080000, 0xBF92E771,
    0x5EE9AD86, 0xBD4C7ED8,
    0x0D100000, 0xBF9163D2,
    0x664FE33F, 0xBD46E8B9,
    0xCBCC0000, 0xBF8FED45,
    0x43464056, 0xBD37F339,
    0xC5EA0000, 0xBF8D1232,
    0xB0BDC8DF, 0xBD17CF34,
    0x28680000, 0xBF8A08A8,
    0xF02B9CCF, 0xBD35A529,
    0x4CF00000, 0xBF872C1F,
    0x580FE573, 0xBD2B4934,
    0xA6F20000, 0xBF844EE0,
    0xFF314317, 0xBD24C8CB,
    0xDC1C0000, 0xBF8170EB,
    0x2CC5232F, 0xBD447DB0,
    0x97D40000, 0xBF7CC89F,
    0x90330E7B, 0xBD43AC9C,
    0x6D780000, 0xBF7709C4,
    0x56CDE925, 0xBD4563BA,
    0xCCF40000, 0xBF71497A,
    0xDDD3E770, 0xBD4F08E7,
    0xFF080000, 0xBF670F83,
    0x31D4676D, 0xBD33AB26,
    0x37400000, 0xBF571265,
    0xFD4FCA1D, 0xBD2FA2A0,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x40000000, 0x3FF71547,
    0x00000000, 0x3FF71519,
    0x00000000, 0x3FF714EB,
    0xC0000000, 0x3FF714BC,
    0x80000000, 0x3FF7148E,
    0x80000000, 0x3FF71460,
    0x40000000, 0x3FF71432,
    0x40000000, 0x3FF71404,
    0x00000000, 0x3FF713D6,
    0xC0000000, 0x3FF713A7,
    0xC0000000, 0x3FF71379,
    0x80000000, 0x3FF7134B,
    0x80000000, 0x3FF7131D,
    0x40000000, 0x3FF712EF,
    0x40000000, 0x3FF712C1,
    0x00000000, 0x3FF71293,
    0x00000000, 0x3FF71265,
    0xC0000000, 0x3FF71236,
    0xC0000000, 0x3FF71208,
    0x80000000, 0x3FF711DA,
    0x80000000, 0x3FF711AC,
    0x80000000, 0x3FF7117E,
    0x40000000, 0x3FF71150,
    0x40000000, 0x3FF71122,
    0x00000000, 0x3FF710F4,
    0x00000000, 0x3FF710C6,
    0x00000000, 0x3FF71098,
    0xC0000000, 0x3FF71069,
    0xC0000000, 0x3FF7103B,
    0x80000000, 0x3FF7100D,
    0x80000000, 0x3FF70FDF,
    0x80000000, 0x3FF70FB1,
    0x40000000, 0x3FF70F83,
    0x40000000, 0x3FF70F55,
    0x40000000, 0x3FF70F27,
    0x40000000, 0x3FF70EF9,
    0x00000000, 0x3FF70ECB,
    0x00000000, 0x3FF70E9D,
    0x00000000, 0x3FF70E6F,
    0x00000000, 0x3FF70E41,
    0xC0000000, 0x3FF70E12,
    0xC0000000, 0x3FF70DE4,
    0xC0000000, 0x3FF70DB6,
    0xC0000000, 0x3FF70D88,
    0xC0000000, 0x3FF70D5A,
    0x80000000, 0x3FF70D2C,
    0x80000000, 0x3FF70CFE,
    0x80000000, 0x3FF70CD0,
    0x80000000, 0x3FF70CA2,
    0x80000000, 0x3FF70C74,
    0x80000000, 0x3FF70C46,
    0x80000000, 0x3FF70C18,
    0x80000000, 0x3FF70BEA,
    0x80000000, 0x3FF70BBC,
    0x40000000, 0x3FF70B8E,
    0x40000000, 0x3FF70B60,
    0x40000000, 0x3FF70B32,
    0x40000000, 0x3FF70B04,
    0x40000000, 0x3FF70AD6,
    0x40000000, 0x3FF70AA8,
    0x40000000, 0x3FF70A7A,
    0x80000000, 0x3FF71B53,
    0x40000000, 0x3FF71B3C,
    0x40000000, 0x3FF71B25,
    0x00000000, 0x3FF71B0E,
    0x00000000, 0x3FF71AF7,
    0xC0000000, 0x3FF71ADF,
    0xC0000000, 0x3FF71AC8,
    0x80000000, 0x3FF71AB1,
    0x80000000, 0x3FF71A9A,
    0x40000000, 0x3FF71A83,
    0x40000000, 0x3FF71A6C,
    0x00000000, 0x3FF71A55,
    0x00000000, 0x3FF71A3E,
    0xC0000000, 0x3FF71A26,
    0xC0000000, 0x3FF71A0F,
    0x80000000, 0x3FF719F8,
    0x80000000, 0x3FF719E1,
    0x40000000, 0x3FF719CA,
    0x40000000, 0x3FF719B3,
    0x00000000, 0x3FF7199C,
    0x00000000, 0x3FF71985,
    0xC0000000, 0x3FF7196D,
    0xC0000000, 0x3FF71956,
    0x80000000, 0x3FF7193F,
    0x80000000, 0x3FF71928,
    0x40000000, 0x3FF71911,
    0x40000000, 0x3FF718FA,
    0x40000000, 0x3FF718E3,
    0x00000000, 0x3FF718CC,
    0x00000000, 0x3FF718B5,
    0xC0000000, 0x3FF7189D,
    0xC0000000, 0x3FF71886,
    0x80000000, 0x3FF7186F,
    0x80000000, 0x3FF71858,
    0x80000000, 0x3FF71841,
    0x40000000, 0x3FF7182A,
    0x40000000, 0x3FF71813,
    0x00000000, 0x3FF717FC,
    0x00000000, 0x3FF717E5,
    0xC0000000, 0x3FF717CD,
    0xC0000000, 0x3FF717B6,
    0xC0000000, 0x3FF7179F,
    0x80000000, 0x3FF71788,
    0x80000000, 0x3FF71771,
    0x40000000, 0x3FF7175A,
    0x40000000, 0x3FF71743,
    0x40000000, 0x3FF7172C,
    0x00000000, 0x3FF71715,
    0x00000000, 0x3FF716FE,
    0xC0000000, 0x3FF716E6,
    0xC0000000, 0x3FF716CF,
    0xC0000000, 0x3FF716B8,
    0x80000000, 0x3FF716A1,
    0x80000000, 0x3FF7168A,
    0x80000000, 0x3FF71673,
    0x40000000, 0x3FF7165C,
    0x40000000, 0x3FF71645,
    0x00000000, 0x3FF7162E,
    0x00000000, 0x3FF71617,
    0x00000000, 0x3FF71600,
    0xC0000000, 0x3FF715E8,
    0xC0000000, 0x3FF715D1,
    0xC0000000, 0x3FF715BA,
    0x80000000, 0x3FF715A3,
    0x80000000, 0x3FF7158C,
    0x80000000, 0x3FF71575,
    0x40000000, 0x3FF7155E,
    0x40000000, 0x3FF71547,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x50000000, 0x3F072017,
    0x641F4F36, 0x3D099925,
    0x3B000000, 0x3F17102E,
    0xC162D124, 0x3D120082,
    0x0C800000, 0x3F215034,
    0x09125FF4, 0x3D441EE7,
    0x92000000, 0x3F27185C,
    0x8BABF46F, 0x3CEECD1C,
    0x5D000000, 0x3F2CD890,
    0xF0BBF0CF, 0x3D3BEE60,
    0xFF400000, 0x3F315067,
    0x8A9648DD, 0x3D3FDBDF,
    0x63400000, 0x3F34308D,
    0xAEE1B670, 0x3D3681CB,
    0xC2800000, 0x3F3714B8,
    0xBF3D153E, 0x3D4102E6,
    0xED400000, 0x3F39F8E9,
    0x33794ABC, 0x3D365EF2,
    0x93400000, 0x3F3CD920,
    0x3D384508, 0x3D4FDF0B,
    0x4D400000, 0x3F3FBD5D,
    0x782C348A, 0x3D19A166,
    0xB9400000, 0x3F414ECF,
    0x39DDE07F, 0x3D48ABA0,
    0xDDE00000, 0x3F42C0F3,
    0x9D5FD823, 0x3D435F8A,
    0xB0600000, 0x3F44311A,
    0x12458BEC, 0x3D47963A,
    0x9CE00000, 0x3F45A344,
    0x2C8C13FD, 0x3D47E151,
    0x2F600000, 0x3F471371,
    0xCB0BD4FA, 0x3D48D0BE,
    0xE4000000, 0x3F4885A0,
    0xDCE1A474, 0x3D39DB21,
    0x36A00000, 0x3F49F5D3,
    0x55660916, 0x3D489166,
    0xB3800000, 0x3F4B6808,
    0xC862A7D0, 0x3D3D7958,
    0xC6800000, 0x3F4CD840,
    0xD43B70F4, 0x3D4312BA,
    0xB7C00000, 0x3F4E487B,
    0x975E2C41, 0x3D46363F,
    0xDF600000, 0x3F4FBAB9,
    0x850FC6C3, 0x3D224E63,
    0x48A00000, 0x3F50957D,
    0x17A21AA6, 0x3D256896,
    0x40C00000, 0x3F514E9F,
    0xA4582824, 0x3D32AE5F,
    0x7A100000, 0x3F5206C2,
    0x84B0FD57, 0x3D45C3F8,
    0x22A00000, 0x3F52BEE7,
    0x4AAD1649, 0x3D4A525C,
    0x70800000, 0x3F53780D,
    0x275071DE, 0x3D1DEB18,
    0xF9900000, 0x3F543034,
    0xD5D75FB6, 0x3D48EAE4,
    0x2C000000, 0x3F54E95E,
    0xC1291B85, 0x3D3728C5,
    0x95B00000, 0x3F55A188,
    0xABECF0D7, 0x3D4F699A,
    0x6ED00000, 0x3F5659B4,
    0x8F2D1FA9, 0x3D22ED25,
    0xF7400000, 0x3F5712E1,
    0x8B30E580, 0x3D3445EF,
    0xB1100000, 0x3F57CB10,
    0xDC75FAC6, 0x3D431D52,
    0xDA500000, 0x3F588340,
    0xA135BD69, 0x3D3FFBCB,
    0x73000000, 0x3F593B72,
    0x2D63E5DB, 0x3D387100,
    0xC3200000, 0x3F59F4A5,
    0x1C0BB062, 0x3D45AFDD,
    0x3CC00000, 0x3F5AACDA,
    0xDCD040AE, 0x3D41914F,
    0x25E00000, 0x3F5B6510,
    0xDB245B1F, 0x3D3E7387,
    0x7E800000, 0x3F5C1D47,
    0x593D6B3F, 0x3D4A1E1B,
    0x96C00000, 0x3F5CD680,
    0xCC31FC26, 0x3CEA7D5C,
    0xD0800000, 0x3F5D8EBA,
    0x762A3069, 0x3D2BEFBC,
    0x79D00000, 0x3F5E46F6,
    0x952BE02C, 0x3D4D39FF,
    0x92D00000, 0x3F5EFF33,
    0x3B1A1CB4, 0x3D3D3437,
    0x1B700000, 0x3F5FB772,
    0x57218470, 0x3D2A49F2,
    0x36D80000, 0x3F603859,
    0x602BA3B5, 0x3D43ADB6,
    0xEBD00000, 0x3F609479,
    0x0591EE13, 0x3D4A6056,
    0x58A80000, 0x3F60F09B,
    0x4FAFF44B, 0x3D343EEC,
    0x7D580000, 0x3F614CBD,
    0x130DF139, 0x3D3CC4C6,
    0x59E80000, 0x3F61A8E0,
    0x9380107F, 0x3D42A0AD,
    0xEE600000, 0x3F620503,
    0xB9035A2A, 0x3D12915A,
    0x3AB80000, 0x3F626128,
    0x20CAACA3, 0x3D31DDED,
    0x3EF80000, 0x3F62BD4D,
    0x281079C7, 0x3D41EF6E,
    0xFB280000, 0x3F631972,
    0x6B98497F, 0x3D31986B,
    0xA5400000, 0x3F637619,
    0x91F2B430, 0x3D4C3697,
    0xD2500000, 0x3F63D240,
    0xCE1C0762, 0x3D40ECA7,
    0xB7500000, 0x3F642E68,
    0x073B1E2E, 0x3D45FA86,
    0x54480000, 0x3F648A91,
    0x05A622FD, 0x3D46D9E9,
    0xA9380000, 0x3F64E6BA,
    0x4B55A072, 0x3D4F04CC,
    0xB6280000, 0x3F6542E4,
    0x16094E0E, 0x3D49F570,
    0x7B180000, 0x3F659F0F,
    0x616CF239, 0x3D432659,
    0x69800000, 0xBF582DD5,
    0x3ED708F7, 0xBD3AB541,
    0x91900000, 0xBF57D0ED,
    0x7B358E46, 0xBD4135C6,
    0x1B300000, 0xBF577505,
    0xF6ED6FDE, 0xBCFEF3C0,
    0x89400000, 0xBF57181C,
    0x5AF7807F, 0xBD46A455,
    0x5AE00000, 0xBF56BC33,
    0x919C892D, 0xBD452637,
    0x0F000000, 0xBF565F4A,
    0x806E8ED0, 0xBD46D380,
    0x28A00000, 0xBF560360,
    0xFEDC7D6B, 0xBD4E545E,
    0x22D00000, 0xBF55A676,
    0xFDAB27BC, 0xBD28C45A,
    0x84700000, 0xBF554A8B,
    0x94F1D540, 0xBD40EFC0,
    0xC4900000, 0xBF54EDA0,
    0x6E5D7E55, 0xBD492AA1,
    0x6E400000, 0xBF5491B5,
    0xAE3A4995, 0xBD065C31,
    0xF4600000, 0xBF5434C9,
    0x90B30C51, 0xBD20B591,
    0xE6000000, 0xBF53D8DD,
    0x5C64292C, 0xBD208E1F,
    0xB2100000, 0xBF537BF1,
    0xF606F2E6, 0xBD4BA65D,
    0xEBB00000, 0xBF532004,
    0x1A593CA0, 0xBD3B2BD5,
    0xFDC00000, 0xBF52C317,
    0xF5360F7D, 0xBD440246,
    0x7F500000, 0xBF52672A,
    0x7BBFCA47, 0xBD422981,
    0xD7600000, 0xBF520A3C,
    0xCF30F123, 0xBD0AD92A,
    0xA0E00000, 0xBF51AE4E,
    0x693830AD, 0xBD292A75,
    0x3ED00000, 0xBF515160,
    0xE0391426, 0xBD49146E,
    0x50400000, 0xBF50F571,
    0x57002344, 0xBD4E6549,
    0x34300000, 0xBF509882,
    0x8F311F09, 0xBD3D4587,
    0x8D900000, 0xBF503C92,
    0x28FE4EA6, 0xBD3DCA96,
    0x6EC00000, 0xBF4FBF45,
    0x0F0A4C7F, 0xBD46C434,
    0xB1600000, 0xBF4F0764,
    0x4F8F13F5, 0xBD386C45,
    0x90E00000, 0xBF4E4D83,
    0x4F099D6A, 0xBD279073,
    0x63400000, 0xBF4D95A1,
    0x2BF804AC, 0xBD358615,
    0x7E800000, 0xBF4CDDBE,
    0xEDA75E42, 0xBD43F2DC,
    0x30A00000, 0xBF4C23DB,
    0xD23862D0, 0xBD4EF6F7,
    0xDBC00000, 0xBF4B6BF6,
    0x2300F78C, 0xBD3DE7D1,
    0x19A00000, 0xBF4AB212,
    0xBB645928, 0xBD4D3C99,
    0x54800000, 0xBF49FA2C,
    0xADF18185, 0xBD3C87D4,
    0x1E200000, 0xBF494046,
    0x1A0618B0, 0xBD49FE5A,
    0xE8C00000, 0xBF48885E,
    0xAAEEF6A6, 0xBD2934A5,
    0xFC200000, 0xBF47D076,
    0xECBB5462, 0xBD39CFC0,
    0x98600000, 0xBF47168E,
    0xBFA1C16B, 0xBD36F34E,
    0x3B800000, 0xBF465EA5,
    0x9088CA01, 0xBD22ED1C,
    0x63600000, 0xBF45A4BB,
    0x2D6DE9B5, 0xBD423320,
    0x96200000, 0xBF44ECD0,
    0x10D84808, 0xBD485871,
    0x49C00000, 0xBF4432E5,
    0x5992900A, 0xBD3DC614,
    0x0C200000, 0xBF437AF9,
    0xF611F4F2, 0xBD4C28B1,
    0x17600000, 0xBF42C30C,
    0x12496DA4, 0xBD49B472,
    0x9D800000, 0xBF42091E,
    0xB07B63E5, 0xBD22545F,
    0x38600000, 0xBF415130,
    0x6B4DAA19, 0xBCF3FC74,
    0x4A000000, 0xBF409741,
    0x9F7943AC, 0xBD2819AD,
    0xE8C00000, 0xBF3FBEA2,
    0x05018F01, 0xBD4ACC92,
    0xCF400000, 0xBF3E4EC1,
    0x5DECAD9C, 0xBD48B028,
    0x97400000, 0xBF3CDADF,
    0x63115207, 0xBD4CA2DD,
    0x9D000000, 0xBF3B6AFB,
    0xA7FC9363, 0xBD354003,
    0x7C000000, 0xBF39F716,
    0x407A7831, 0xBD4A8AE0,
    0xA0C00000, 0xBF38872F,
    0x91A8939D, 0xBD42DD8B,
    0x57000000, 0xBF371747,
    0x2F243D55, 0xBD3A4925,
    0xDAC00000, 0xBF35A35D,
    0x22A2581B, 0xBD2741DF,
    0xB0000000, 0xBF343372,
    0x3C22E0D2, 0xBD0CAB75,
    0x16800000, 0xBF32C386,
    0x5FC1D4DD, 0xBD4B6E11,
    0x3EC00000, 0xBF314F98,
    0x97121E28, 0xBD3F929A,
    0x89000000, 0xBF2FBF51,
    0x806FDBDA, 0xBD13EA1E,
    0x07000000, 0xBF2CD770,
    0x51E95ECB, 0xBD370EB1,
    0x50000000, 0xBF29F78B,
    0xFF247507, 0xBD418759,
    0xBC000000, 0xBF2717A3,
    0xC6C6F140, 0xBD3C0D6B,
    0x82800000, 0xBF242FB9,
    0xB8D3C162, 0xBD4CF2EF,
    0x2C800000, 0xBF214FCC,
    0x12CFE97E, 0xBD206CE3,
    0xF1000000, 0xBF1CDFB7,
    0x1056AF68, 0xBD4D7734,
    0x10000000, 0xBF170FD2,
    0x29329AEF, 0xBD24CAA4,
    0x24000000, 0xBF114FE6,
    0x06B71311, 0xBCF284C3,
    0xFA000000, 0xBF071FE8,
    0xDD98569A, 0xBD33BB0C,
    0xAC000000, 0xBEF6FFF4,
    0x0B7407C7, 0xBD4D54D7,
    0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
    0x00000000, 0x00000000,
    0xA9FB3335, 0x3FF0163D,
    0x9AB8CDB7, 0x3C9B6129,
    0x3E778061, 0x3FF02C9A,
    0x535B085D, 0xBC719083,
    0xE86E7F85, 0x3FF04315,
    0x1977C96E, 0xBC90A31C,
    0xD3158574, 0x3FF059B0,
    0xA475B465, 0x3C8D73E2,
    0x29DDF6DE, 0x3FF0706B,
    0xE2B13C26, 0xBC8C91DF,
    0x18759BC8, 0x3FF08745,
    0x4BB284FF, 0x3C6186BE,
    0xCAC6F383, 0x3FF09E3E,
    0x18316135, 0x3C914878,
    0x6CF9890F, 0x3FF0B558,
    0x4ADC610A, 0x3C98A62E,
    0x2B7247F7, 0x3FF0CC92,
    0x16E24F71, 0x3C901EDC,
    0x32D3D1A2, 0x3FF0E3EC,
    0x27C57B52, 0x3C403A17,
    0xAFFED31B, 0x3FF0FB66,
    0xC44EBD7B, 0xBC6B9BED,
    0xD0125B51, 0x3FF11301,
    0x39449B39, 0xBC96C510,
    0xC06C31CC, 0x3FF12ABD,
    0xB36CA5C7, 0xBC51B514,
    0xAEA92DE0, 0x3FF1429A,
    0x9AF1369E, 0xBC932FBF,
    0xC8A58E51, 0x3FF15A98,
    0xB9EEAB09, 0x3C82406A,
    0x3C7D517B, 0x3FF172B8,
    0xB9D78A75, 0xBC819041,
    0x388C8DEA, 0x3FF18AF9,
    0xD1970F6B, 0xBC911023,
    0xEB6FCB75, 0x3FF1A35B,
    0x7B4968E4, 0x3C8E5B4C,
    0x84045CD4, 0x3FF1BBE0,
    0x352EF607, 0xBC995386,
    0x3168B9AA, 0x3FF1D487,
    0x00A2643C, 0x3C9E016E,
    0x22FCD91D, 0x3FF1ED50,
    0x027BB78B, 0xBC91DF98,
    0x88628CD6, 0x3FF2063B,
    0x814A8494, 0x3C8DC775,
    0x917DDC96, 0x3FF21F49,
    0x9494A5ED, 0x3C82A97E,
    0x6E756238, 0x3FF2387A,
    0xB6C70572, 0x3C99B07E,
    0x4FB2A63F, 0x3FF251CE,
    0xBEF4F4A4, 0x3C8AC155,
    0x65E27CDD, 0x3FF26B45,
    0x9940E9D9, 0x3C82BD33,
    0xE1F56381, 0x3FF284DF,
    0x8C3F0D7D, 0xBC9A4C3A,
    0xF51FDEE1, 0x3FF29E9D,
    0xAFAD1255, 0x3C8612E8,
    0xD0DAD990, 0x3FF2B87F,
    0xD6381AA3, 0xBC410ADC,
    0xA6E4030B, 0x3FF2D285,
    0x54DB41D4, 0x3C900247,
    0xA93E2F56, 0x3FF2ECAF,
    0x45D52383, 0x3C71CA0F,
    0x0A31B715, 0x3FF306FE,
    0xD23182E4, 0x3C86F46A,
    0xFC4CD831, 0x3FF32170,
    0x8E18047C, 0x3C8A9CE7,
    0xB26416FF, 0x3FF33C08,
    0x843659A5, 0x3C932721,
    0x5F929FF1, 0x3FF356C5,
    0x5C4E4628, 0xBC8B5CEE,
    0x373AA9CB, 0x3FF371A7,
    0xBF42EAE1, 0xBC963AEA,
    0x6D05D866, 0x3FF38CAE,
    0x3C9904BC, 0xBC9E958D,
    0x34E59FF7, 0x3FF3A7DB,
    0xD661F5E2, 0xBC75E436,
    0xC313A8E5, 0x3FF3C32D,
    0x375D29C3, 0xBC9EFFF8,
    0x4C123422, 0x3FF3DEA6,
    0x11F09EBB, 0x3C8ADA09,
    0x04AC801C, 0x3FF3FA45,
    0xF956F9F3, 0xBC97D023,
    0x21F72E2A, 0x3FF4160A,
    0x1C309278, 0xBC5EF369,
    0xD950A897, 0x3FF431F5,
    0xE35F7998, 0xBC81C7DD,
    0x6061892D, 0x3FF44E08,
    0x04EF80CF, 0x3C489B7A,
    0xED1D0057, 0x3FF46A41,
    0xD1648A76, 0x3C9C944B,
    0xB5C13CD0, 0x3FF486A2,
    0xB69062F0, 0x3C73C1A3,
    0xF0D7D3DE, 0x3FF4A32A,
    0xF3D1BE56, 0x3C99CB62,
    0xD5362A27, 0x3FF4BFDA,
    0xAFEC42E2, 0x3C7D4397,
    0x99FDDD0D, 0x3FF4DCB2,
    0xBC6A7833, 0x3C98ECDB,
    0x769D2CA7, 0x3FF4F9B2,
    0xD25957E3, 0xBC94B309,
    0xA2CF6642, 0x3FF516DA,
    0x69BD93EE, 0xBC8F7685,
    0x569D4F82, 0x3FF5342B,
    0x1DB13CAC, 0xBC807ABE,
    0xCA5D920F, 0x3FF551A4,
    0xEFEDE59A, 0xBC8D689C,
    0x36B527DA, 0x3FF56F47,
    0x011D93AC, 0x3C99BB2C,
    0xD497C7FD, 0x3FF58D12,
    0x5B9A1DE7, 0x3C8295E1,
    0xDD485429, 0x3FF5AB07,
    0x054647AC, 0x3C96324C,
    0x8A5946B7, 0x3FF5C926,
    0x816986A2, 0x3C3C4B1B,
    0x15AD2148, 0x3FF5E76F,
    0x3080E65D, 0x3C9BA6F9,
    0xB976DC09, 0x3FF605E1,
    0x9B56DE47, 0xBC93E242,
    0xB03A5585, 0x3FF6247E,
    0x7E40B496, 0xBC9383C1,
    0x34CCC320, 0x3FF64346,
    0x759D8932, 0xBC8C483C,
    0x82552225, 0x3FF66238,
    0x87591C33, 0xBC9BB609,
    0xD44CA973, 0x3FF68155,
    0x44F73E64, 0x3C6038AE,
    0x667F3BCD, 0x3FF6A09E,
    0x13B26455, 0xBC9BDD34,
    0x750BDABF, 0x3FF6C012,
    0x67FF0B0C, 0xBC728956,
    0x3C651A2F, 0x3FF6DFB2,
    0x683C88AA, 0xBC6BBE3A,
    0xF9519484, 0x3FF6FF7D,
    0x25860EF6, 0xBC883C0F,
    0xE8EC5F74, 0x3FF71F75,
    0x86887A99, 0xBC816E47,
    0x48A58174, 0x3FF73F9A,
    0x6C65D53B, 0xBC90A8D9,
    0x564267C9, 0x3FF75FEB,
    0x57316DD3, 0xBC902459,
    0x4FDE5D3F, 0x3FF78069,
    0x0A02162C, 0x3C9866B8,
    0x73EB0187, 0x3FF7A114,
    0xEE04992F, 0xBC841577,
    0x0130C132, 0x3FF7C1ED,
    0xD1164DD5, 0x3C9F124C,
    0x36CF4E62, 0x3FF7E2F3,
    0xBA15797E, 0x3C705D02,
    0x543E1A12, 0x3FF80427,
    0x626D972A, 0xBC927C86,
    0x994CCE13, 0x3FF82589,
    0xD41532D7, 0xBC9D4C1D,
    0x4623C7AD, 0x3FF8471A,
    0xA341CDFB, 0xBC88D684,
    0x9B4492ED, 0x3FF868D9,
    0x9BD4F6BA, 0xBC9FC6F8,
    0xD98A6699, 0x3FF88AC7,
    0xF37CB53A, 0x3C9994C2,
    0x422AA0DB, 0x3FF8ACE5,
    0x56864B26, 0x3C96E9F1,
    0x16B5448C, 0x3FF8CF32,
    0x32E9E3AA, 0xBC70D55E,
    0x99157736, 0x3FF8F1AE,
    0xA2E3976C, 0x3C85CC13,
    0x0B91FFC6, 0x3FF9145B,
    0x2E582523, 0xBC9DD679,
    0xB0CDC5E5, 0x3FF93737,
    0x81B57EBB, 0xBC675FC7,
    0xCBC8520F, 0x3FF95A44,
    0x96A5F039, 0xBC764B7C,
    0x9FDE4E50, 0x3FF97D82,
    0x7C1B85D0, 0xBC9D185B,
    0x70CA07BA, 0x3FF9A0F1,
    0x91CEE632, 0xBC9173BD,
    0x82A3F090, 0x3FF9C491,
    0xB071F2BE, 0x3C7C7C46,
    0x19E32323, 0x3FF9E863,
    0x78E64C6E, 0x3C7824CA,
    0x7B5DE565, 0x3FFA0C66,
    0x5D1CD532, 0xBC935949,
    0xEC4A2D33, 0x3FFA309B,
    0x7DDC36AB, 0x3C96305C,
    0xB23E255D, 0x3FFA5503,
    0xDB8D41E1, 0xBC9D2F6E,
    0x1330B358, 0x3FFA799E,
    0xCAC563C6, 0x3C9BCB7E,
    0x5579FDBF, 0x3FFA9E6B,
    0x0EF7FD31, 0x3C90FAC9,
    0xBFD3F37A, 0x3FFAC36B,
    0xCAE76CD0, 0xBC8F9234,
    0x995AD3AD, 0x3FFAE89F,
    0x345DCC81, 0x3C97A1CD,
    0x298DB666, 0x3FFB0E07,
    0x4C80E424, 0xBC9BDEF5,
    0xB84F15FB, 0x3FFB33A2,
    0x3084D707, 0xBC62805E,
    0x8DE5593A, 0x3FFB5972,
    0xBBBA6DE3, 0xBC9C71DF,
    0xF2FB5E47, 0x3FFB7F76,
    0x7E54AC3A, 0xBC75584F,
    0x30A1064A, 0x3FFBA5B0,
    0x0E54292E, 0xBC9EFCD3,
    0x904BC1D2, 0x3FFBCC1E,
    0x7A2D9E84, 0x3C823DD0,
    0x5BD71E09, 0x3FFBF2C2,
    0x3F6B9C72, 0xBC9EFDCA,
    0xDD85529C, 0x3FFC199B,
    0x895048DD, 0x3C811065,
    0x5FFFD07A, 0x3FFC40AB,
    0xE083C60A, 0x3C9B4537,
    0x2E57D14B, 0x3FFC67F1,
    0xFF483CAC, 0x3C92884D,
    0x9406E7B5, 0x3FFC8F6D,
    0x48805C44, 0x3C71ACBC,
    0xDCEF9069, 0x3FFCB720,
    0xD1E949DB, 0x3C7503CB,
    0x555DC3FA, 0x3FFCDF0B,
    0x53829D72, 0xBC8DD83B,
    0x4A07897C, 0x3FFD072D,
    0x43797A9C, 0xBC9CBC37,
    0x080D89F2, 0x3FFD2F87,
    0x719D8577, 0xBC9D487B,
    0xDCFBA487, 0x3FFD5818,
    0xD75B3706, 0x3C82ED02,
    0x16C98398, 0x3FFD80E3,
    0x8BEDDFE8, 0xBC911EC1,
    0x03DB3285, 0x3FFDA9E6,
    0x696DB532, 0x3C9C2300,
    0xF301B460, 0x3FFDD321,
    0x78F018C2, 0x3C92DA57,
    0x337B9B5F, 0x3FFDFC97,
    0x4F184B5B, 0xBC91A5CD,
    0x14F5A129, 0x3FFE2646,
    0x817A1496, 0xBC97B627,
    0xE78B3FF6, 0x3FFE502E,
    0x80A9CC8F, 0x3C839E89,
    0xFBC74C83, 0x3FFE7A51,
    0xCA0C8DE1, 0x3C92D522,
    0xA2A490DA, 0x3FFEA4AF,
    0x179C2893, 0xBC9E9C23,
    0x2D8E67F1, 0x3FFECF48,
    0xB411AD8C, 0xBC9C93F3,
    0xEE615A27, 0x3FFEFA1B,
    0x86A4B6B0, 0x3C9DC7F4,
    0x376BBA97, 0x3FFF252B,
    0xBF0D8E43, 0x3C93A1A5,
    0x5B6E4540, 0x3FFF5076,
    0x2DD8A18A, 0x3C99D3E1,
    0xAD9CBE14, 0x3FFF7BFD,
    0xD0063509, 0xBC9DBB12,
    0x819E90D8, 0x3FFFA7C1,
    0xF3A5931E, 0x3C874853,
    0x2B8F71F1, 0x3FFFD3C2,
    0x966579E7, 0x3C62EB74,
    0x966457E8, 0x3E79C3A6,
    0x46694107, 0xBFD62E43,
    0x62B6DEE1, 0x3FC47FD4,
    0x2A9012D8, 0xBFB55047,
    0xFEFA39EF, 0x3FE62E42,
    0xFF82C58F, 0x3FCEBFBD,
    0xD704A0C0, 0x3FAC6B08,
    0x6FBA4E77, 0x3F83B2AB,
    0xE78A6731, 0x3F55D87F,
    0x00000000, 0x7FE00000,
    0x00000000, 0x00100000,
    0x00000000, 0x00000000,
    0x00000000, 0x3FF00000,
    0x00000000, 0xBFF00000,
    0x00000000, 0x42C80000,
    0x40000000, 0x3FF71547,
    0x02000000, 0x41A00000,
    0x00000000, 0x4C700000,
    0x00000000, 0x33700000,
};

__attribute__((always_inline))
inline int __internal_dpowr_lut_cout (double *a, double *b, double *r)
{
    int nRet = 0;
    double dbVTmp1, dbVTmp2, dbVPHH, dbVPHL;
    double dX, dY, dR, dbAX, dbSignRes, dbX1, dbRcp1, dbL1Hi, dbL1Lo, dbX2, dbRcp2, dbL2Hi, dbL2Lo,
        dbX3, dbRcp3C, dbL3Hi, dbL3Lo, dbK, dbT, dbD, dbR1, dbCQ, dbRcpC, dbX1Hi, dbX1Lo,
        dbRcpCHi, dbRcpCLo, dbTmp1, dbE, dbT_CQHi, dbCQLo, dbR, dbLogPart3, dbLog2Poly,
        dbHH, dbHL, dbHLL, dbYHi, dbYLo, dbTmp2, dbTmp3, dbPH, dbPL, dbPLL, dbZ, dbExp2Poly, dbExp2PolyT, dbResLo, dbResHi, dbRes, dbTwoPowN, dbAY;
    int i, iEXB, iEYB, iSignX, iSignY, iYHi, iYLo, iYIsFinite, iEY, iYIsInt, iXIsFinite,
        iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes, iSign, iIsSigZeroX, iIsSigZeroY;
    dX = *a;
    dY = *b;
    iEXB = ((((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF);
    iEYB = ((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF);
    iSignX = (((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 31);
    iSignY = (((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 31);
    iIsSigZeroX = (((((_iml_dp_union_t *) & dX)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_dp_union_t *) & dX)->dwords.lo_dword) == 0));
    iIsSigZeroY = (((((_iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF) == 0) && ((((_iml_dp_union_t *) & dY)->dwords.lo_dword) == 0));
    iYHi = (iEYB << 20) | (((_iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF);
    iYLo = (((_iml_dp_union_t *) & dY)->dwords.lo_dword);
    iYIsFinite = (((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);
    {
        int iXisZero = ((iEXB == 0) && (iIsSigZeroX));
        int iYisZero = ((iEYB == 0) && (iIsSigZeroY));
        int iXisNAN = (!((((((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))) && (!(iIsSigZeroX));
        int iYisNAN = (!((((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))) && (!(iIsSigZeroY));
        int iXisINF = (!((((((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))) && ((iIsSigZeroX));
        int iYisINF = (!((((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF))) && ((iIsSigZeroY));
        if (iXisNAN)
        {
            dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
            dbVTmp1 = dbVTmp1 / dbVTmp1;
            *r = dbVTmp1;
            return nRet;
        }
        if ((iXisINF) && (!iSignX) && (iYisZero))
        {
            dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
            dbVTmp1 = dbVTmp1 / dbVTmp1;
            *r = dbVTmp1;
            return nRet;
        }
        if (iXisZero)
        {
            if (iYisZero)
            {
                dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                dbVTmp1 = dbVTmp1 / dbVTmp1;
                *r = dbVTmp1;
                return nRet;
            }
            if (iYisINF && (!iSignY))
            {
                dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                *r = dbVTmp1;
                return nRet;
            }
            if (iYisINF && iSignY)
            {
                dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                dbVTmp1 = 1.0 / dbVTmp1;
                *r = dbVTmp1;
                return nRet;
            }
            if (((((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)) && iSignY)
            {
                dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                dbVTmp1 = 1.0 / dbVTmp1;
                *r = dbVTmp1;
                return nRet;
            }
            if (((((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)) && (!iSignY))
            {
                *r = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                return nRet;
            }
        }
        if (dX == ((__constant double *) __dpowr_la_nolut_CoutTab)[853])
        {
            if (((((((_iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF)))
            {
                *r = ((__constant double *) __dpowr_la_nolut_CoutTab)[853];
                return nRet;
            }
            if (iYisNAN || iYisINF)
            {
                dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                dbVTmp1 = dbVTmp1 / dbVTmp1;
                *r = dbVTmp1;
                return nRet;
            }
        }
        if (iSignX)
        {
            dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
            dbVTmp1 = dbVTmp1 / dbVTmp1;
            *r = dbVTmp1;
            nRet = 1;
            return nRet;
        }
    }
    if (iYHi | iYLo)
    {
        iEY = iEYB - 0x3FF;
        if ((0x3FF <= iEYB) && iYIsFinite)
        {
            if (iEY <= 20)
            {
                if (((iYHi << iEY) << 12) | iYLo)
                {
                    iYIsInt = 0;
                }
                else
                {
                    if ((iYHi << (iEY + 11)) & 0x80000000)
                    {
                        iYIsInt = 1;
                    }
                    else
                    {
                        iYIsInt = 2;
                    }
                }
            }
            else
            {
                if (iEY < 53)
                {
                    if ((iYLo << (iEY + 12 - 32 - 1)) << 1)
                    {
                        iYIsInt = 0;
                    }
                    else
                    {
                        if ((iYLo << (iEY + 12 - 32 - 1)) & 0x80000000)
                        {
                            iYIsInt = 1;
                        }
                        else
                        {
                            iYIsInt = 2;
                        }
                    }
                }
                else
                {
                    iYIsInt = 2;
                }
            }
        }
        else
        {
            iYIsInt = 0;
        }
    }
    else
    {
        iYIsInt = 2;
    }
    if (!((iSignX == 0) && (iEXB == 0x3FF) && iIsSigZeroX) && !((iEYB == 0) && iIsSigZeroY))
    {
        iXIsFinite = (((((_iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);
        if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
        {
            if (dX != ((__constant double *) __dpowr_la_nolut_CoutTab)[852])
            {
                if (!((dX == ((__constant double *) __dpowr_la_nolut_CoutTab)[854]) && (iYIsInt || !iYIsFinite)))
                {
                    if (iXIsFinite && iYIsFinite)
                    {
                        if ((dX > ((__constant double *) __dpowr_la_nolut_CoutTab)[852]) || iYIsInt)
                        {
                            dbSignRes = ((__constant double *) __dpowr_la_nolut_CoutTab)[853 + (iSignX & iYIsInt)];
                            iDenoExpAdd = 0;
                            dbAX = dX;
                            (((_iml_dp_union_t *) & dbAX)->dwords.hi_dword =
                             (((_iml_dp_union_t *) & dbAX)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) << 31));
                            if (iEXB == 0)
                            {
                                dbAX = dbAX * ((__constant double *) __dpowr_la_nolut_CoutTab)[858];
                                iDenoExpAdd = iDenoExpAdd - 200;
                            }
                            dbX1 = dbAX;
                            (((_iml_dp_union_t *) & dbX1)->dwords.hi_dword =
                             (((_iml_dp_union_t *) & dbX1)->dwords.hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF) & 0x7FF) << 20));
                            iXHi = ((((_iml_dp_union_t *) & dbAX)->dwords.hi_dword >> 20) & 0x7FF);
                            iXHi = iXHi << 20;
                            iXHi = iXHi | (((_iml_dp_union_t *) & dbAX)->dwords.hi_dword & 0x000FFFFF);
                            k = iXHi - 0x3FE7C000;
                            k = k >> 20;
                            k = k + iDenoExpAdd;
                            i1 = (((_iml_dp_union_t *) & dbX1)->dwords.hi_dword & 0x000FFFFF);
                            i1 = i1 & 0xFC000;
                            i1 = i1 + 0x4000;
                            i1 = i1 >> 15;
                            dbRcp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[0 + i1];
                            dbL1Hi = ((__constant double *) __dpowr_la_nolut_CoutTab)[33 + 2 * (i1) + 0];
                            dbL1Lo = ((__constant double *) __dpowr_la_nolut_CoutTab)[33 + 2 * (i1) + 1];
                            dbX2 = dbX1 * dbRcp1;
                            i2 = (((_iml_dp_union_t *) & dbX2)->dwords.hi_dword & 0x000FFFFF);
                            i2 = i2 & 0xFC00;
                            i2 = i2 + 0x400;
                            i2 = i2 >> 11;
                            dbRcp2 = ((__constant double *) __dpowr_la_nolut_CoutTab)[99 + i2];
                            dbL2Hi = ((__constant double *) __dpowr_la_nolut_CoutTab)[132 + 2 * (i2) + 0];
                            dbL2Lo = ((__constant double *) __dpowr_la_nolut_CoutTab)[132 + 2 * (i2) + 1];
                            dbX3 = dbX2 * dbRcp2;
                            i3 = (((_iml_dp_union_t *) & dbX3)->dwords.hi_dword & 0x000FFFFF);
                            i3 = i3 & 0xFF0;
                            i3 = i3 + 0x10;
                            i3 = i3 >> 5;
                            dbRcp3C = ((__constant double *) __dpowr_la_nolut_CoutTab)[198 + i3];
                            dbL3Hi = ((__constant double *) __dpowr_la_nolut_CoutTab)[327 + 2 * (i3) + 0];
                            dbL3Lo = ((__constant double *) __dpowr_la_nolut_CoutTab)[327 + 2 * (i3) + 1];
                            dbK = (double) k;
                            dbT = (dbK + dbL1Hi);
                            dbT = (dbT + dbL2Hi);
                            dbT = (dbT + dbL3Hi);
                            dbD = (dbL2Lo + dbL3Lo);
                            dbD = (dbD + dbL1Lo);
                            dbR1 = (dbX3 * dbRcp3C);
                            dbCQ = (dbR1 - ((__constant double *) __dpowr_la_nolut_CoutTab)[856]);
                            dbRcpC = (dbRcp1 * dbRcp2);
                            dbRcpC = (dbRcpC * dbRcp3C);
                            dbVTmp1 = ((dbX1) * (((__constant double *) __dpowr_la_nolut_CoutTab)[857]));
                            dbVTmp2 = (dbVTmp1 - (dbX1));
                            dbVTmp1 = (dbVTmp1 - dbVTmp2);
                            dbVTmp2 = ((dbX1) - dbVTmp1);
                            dbX1Hi = dbVTmp1;
                            dbX1Lo = dbVTmp2;
                            dbVTmp1 = ((dbRcpC) * (((__constant double *) __dpowr_la_nolut_CoutTab)[857]));
                            dbVTmp2 = (dbVTmp1 - (dbRcpC));
                            dbVTmp1 = (dbVTmp1 - dbVTmp2);
                            dbVTmp2 = ((dbRcpC) - dbVTmp1);
                            dbRcpCHi = dbVTmp1;
                            dbRcpCLo = dbVTmp2;
                            dbTmp1 = (dbX1Hi * dbRcpCHi);
                            dbE = (dbTmp1 - dbR1);
                            dbTmp1 = (dbX1Lo * dbRcpCHi);
                            dbE = (dbE + dbTmp1);
                            dbTmp1 = (dbX1Hi * dbRcpCLo);
                            dbE = (dbE + dbTmp1);
                            dbTmp1 = (dbX1Lo * dbRcpCLo);
                            dbE = (dbE + dbTmp1);
                            dbVTmp1 = ((dbT) + (dbCQ));
                            dbTmp1 = ((dbT) - dbVTmp1);
                            dbVTmp2 = (dbTmp1 + (dbCQ));
                            dbT_CQHi = dbVTmp1;
                            dbCQLo = dbVTmp2;
                            iELogAX = ((((_iml_dp_union_t *) & dbT_CQHi)->dwords.hi_dword >> 20) & 0x7FF);
                            if (iELogAX + iEYB < 11 + 2 * 0x3FF)
                            {
                                if (iELogAX + iEYB > -62 + 2 * 0x3FF)
                                {
                                    dbR = (dbCQ + dbE);
                                    dbLog2Poly =
                                        ((((((__constant double *) __dpowr_la_nolut_CoutTab)[844]) * dbR +
                                           ((__constant double *) __dpowr_la_nolut_CoutTab)[843]) * dbR +
                                          ((__constant double *) __dpowr_la_nolut_CoutTab)[842]) * dbR +
                                         ((__constant double *) __dpowr_la_nolut_CoutTab)[841]) * dbR;
                                    dbLogPart3 = (dbCQLo + dbE);
                                    dbLogPart3 = (dbD + dbLogPart3);
                                    dbVTmp1 = ((dbT_CQHi) + (dbLog2Poly));
                                    dbTmp1 = ((dbT_CQHi) - dbVTmp1);
                                    dbVTmp2 = (dbTmp1 + (dbLog2Poly));
                                    dbHH = dbVTmp1;
                                    dbHL = dbVTmp2;
                                    dbVTmp1 = ((dbHH) + (dbLogPart3));
                                    dbTmp1 = ((dbHH) - dbVTmp1);
                                    dbVTmp2 = (dbTmp1 + (dbLogPart3));
                                    dbHH = dbVTmp1;
                                    dbHLL = dbVTmp2;
                                    dbHLL = (dbHLL + dbHL);
                                    dbVTmp1 = ((dbHH) * (((__constant double *) __dpowr_la_nolut_CoutTab)[857]));
                                    dbVTmp2 = (dbVTmp1 - (dbHH));
                                    dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                    dbVTmp2 = ((dbHH) - dbVTmp1);
                                    dbHH = dbVTmp1;
                                    dbHL = dbVTmp2;
                                    dbVTmp1 = ((dY) * (((__constant double *) __dpowr_la_nolut_CoutTab)[857]));
                                    dbVTmp2 = (dbVTmp1 - (dY));
                                    dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                    dbVTmp2 = ((dY) - dbVTmp1);
                                    dbYHi = dbVTmp1;
                                    dbYLo = dbVTmp2;
                                    dbTmp1 = ((dbYHi) * (dbHH));
                                    dbTmp2 = ((dbYLo) * (dbHL));
                                    dbTmp2 = (dbTmp2 + (dbYHi) * (dbHL));
                                    dbTmp3 = (dbTmp2 + (dbYLo) * (dbHH));
                                    dbPH = dbTmp1;
                                    dbPL = dbTmp3;
                                    dbPLL = (dY * dbHLL);
                                    dbVTmp1 = (dbPH + ((__constant double *) __dpowr_la_nolut_CoutTab)[855]);
                                    iN = (((_iml_dp_union_t *) & dbVTmp1)->dwords.lo_dword);
                                    j = iN & 0x7F;
                                    iN = iN >> 7;
                                    dbVPHH = (dbVTmp1 - ((__constant double *) __dpowr_la_nolut_CoutTab)[855]);
                                    dbVPHL = (dbPH - dbVPHH);
                                    dbZ = (dbPLL + dbPL);
                                    dbZ = (dbZ + dbVPHL);
                                    dbExp2Poly =
                                        (((((((__constant double *) __dpowr_la_nolut_CoutTab)[849]) * dbZ +
                                            ((__constant double *) __dpowr_la_nolut_CoutTab)[848]) * dbZ +
                                           ((__constant double *) __dpowr_la_nolut_CoutTab)[847]) * dbZ +
                                          ((__constant double *) __dpowr_la_nolut_CoutTab)[846]) * dbZ +
                                         ((__constant double *) __dpowr_la_nolut_CoutTab)[845]) * dbZ;
                                    dbExp2PolyT = (dbExp2Poly * ((__constant double *) __dpowr_la_nolut_CoutTab)[585 + 2 * (j) + 0]);
                                    dbResLo = (dbExp2PolyT + ((__constant double *) __dpowr_la_nolut_CoutTab)[585 + 2 * (j) + 1]);
                                    dbResHi = ((__constant double *) __dpowr_la_nolut_CoutTab)[585 + 2 * (j) + 0];
                                    dbRes = (dbResHi + dbResLo);
                                    iERes = ((((_iml_dp_union_t *) & dbRes)->dwords.hi_dword >> 20) & 0x7FF) - 0x3FF;
                                    iERes = (iERes + iN);
                                    if (iERes < 1024)
                                    {
                                        if (iERes >= -1022)
                                        {
                                            (((_iml_dp_union_t *) & dbRes)->dwords.hi_dword =
                                             (((_iml_dp_union_t *) & dbRes)->dwords.
                                              hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iERes + 0x3FF) & 0x7FF) << 20));
                                            dbRes = dbRes * dbSignRes;
                                            dR = dbRes;
                                        }
                                        else
                                        {
                                            if (iERes >= -1022 - 10)
                                            {
                                                dbVTmp1 = ((dbResHi) + (dbResLo));
                                                dbTmp1 = ((dbResHi) - dbVTmp1);
                                                dbVTmp2 = (dbTmp1 + (dbResLo));
                                                dbResHi = dbVTmp1;
                                                dbResLo = dbVTmp2;
                                                dbVTmp1 = ((dbResHi) * (((__constant double *) __dpowr_la_nolut_CoutTab)[857]));
                                                dbVTmp2 = (dbVTmp1 - (dbResHi));
                                                dbVTmp1 = (dbVTmp1 - dbVTmp2);
                                                dbVTmp2 = ((dbResHi) - dbVTmp1);
                                                dbResHi = dbVTmp1;
                                                dbTmp2 = dbVTmp2;
                                                dbResLo = (dbResLo + dbTmp2);
                                                dbSignRes *= ((__constant double *) __dpowr_la_nolut_CoutTab)[859];
                                                iN = (iN + 200);
                                                dbTwoPowN = ((__constant double *) __dpowr_la_nolut_CoutTab)[853];
                                                (((_iml_dp_union_t *) & dbTwoPowN)->dwords.hi_dword =
                                                 (((_iml_dp_union_t *) & dbTwoPowN)->dwords.
                                                  hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iN + 0x3FF) & 0x7FF) << 20));
                                                dbResHi = (dbResHi * dbTwoPowN);
                                                dbResHi = (dbResHi * dbSignRes);
                                                dbResLo = (dbResLo * dbTwoPowN);
                                                dbVTmp1 = (dbResLo * dbSignRes);
                                                dbRes = (dbResHi + dbVTmp1);
                                                dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[851];
                                                dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                                dbRes = (dbRes + dbVTmp1);
                                                dR = dbRes;
                                            }
                                            else
                                            {
                                                if (iERes >= -1074 - 10)
                                                {
                                                    dbSignRes *= ((__constant double *) __dpowr_la_nolut_CoutTab)[859];
                                                    iN = iN + 200;
                                                    dbTwoPowN = ((__constant double *) __dpowr_la_nolut_CoutTab)[853];
                                                    (((_iml_dp_union_t *) & dbTwoPowN)->dwords.hi_dword =
                                                     (((_iml_dp_union_t *) & dbTwoPowN)->dwords.
                                                      hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (iN + 0x3FF) & 0x7FF) << 20));
                                                    dbRes = (dbRes * dbTwoPowN);
                                                    dbRes = (dbRes * dbSignRes);
                                                    dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[851];
                                                    dbVTmp1 *= dbVTmp1;
                                                    dbRes = (dbRes + dbVTmp1);
                                                    dR = dbRes;
                                                    nRet = 4;
                                                }
                                                else
                                                {
                                                    dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[851];
                                                    dbVTmp1 *= dbVTmp1;
                                                    dbRes = (dbVTmp1 * dbSignRes);
                                                    dR = dbRes;
                                                    nRet = 4;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[850];
                                        dbVTmp1 = (dbVTmp1 * dbVTmp1);
                                        dbRes = (dbVTmp1 * dbSignRes);
                                        dR = dbRes;
                                        nRet = 3;
                                    }
                                }
                                else
                                {
                                    dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[853];
                                    dbVTmp1 = (dbVTmp1 + ((__constant double *) __dpowr_la_nolut_CoutTab)[851]);
                                    dR = (dbVTmp1 * dbSignRes);
                                }
                            }
                            else
                            {
                                iSign = iSignY ^ (((_iml_dp_union_t *) & dbT_CQHi)->dwords.hi_dword >> 31);
                                dbTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[850 + (iSign)];
                                dbTmp1 = (dbTmp1 * dbTmp1);
                                dbTmp1 = (dbTmp1 * dbSignRes);
                                dR = dbTmp1;
                                nRet = (iSign ? 4 : 3);
                            }
                        }
                        else
                        {
                            dbVTmp1 = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                            dbVTmp1 = dbVTmp1 / dbVTmp1;
                            dR = dbVTmp1;
                            nRet = 1;
                        }
                    }
                    else
                    {
                        if (iEXB < 0x3FF)
                        {
                            if (iSignY)
                            {
                                dR = dY * dY;
                            }
                            else
                            {
                                dR = ((__constant double *) __dpowr_la_nolut_CoutTab)[852];
                            }
                        }
                        else
                        {
                            if (iSignY)
                            {
                                dR = ((__constant double *) __dpowr_la_nolut_CoutTab)[852] * ((__constant double *) __dpowr_la_nolut_CoutTab)[853 +
                                                                                                                                  (iYIsInt & iSignX)];
                            }
                            else
                            {
                                dbTmp1 = dX * dX;
                                dbTmp1 = dbTmp1 * dY;
                                dR = dbTmp1 * ((__constant double *) __dpowr_la_nolut_CoutTab)[853 + (iYIsInt & iSignX)];
                            }
                        }
                    }
                }
                else
                {
                    dR = ((__constant double *) __dpowr_la_nolut_CoutTab)[853 + (iYIsInt & 1)];
                }
            }
            else
            {
                dbTmp1 = dX * dX;
                if (iSignY)
                {
                    dR = ((__constant double *) __dpowr_la_nolut_CoutTab)[853 + (iYIsInt & iSignX)] / dbTmp1;
                    nRet = 1;
                }
                else
                {
                    dR = ((__constant double *) __dpowr_la_nolut_CoutTab)[853 + (iYIsInt & iSignX)] * dbTmp1;
                }
            }
        }
        else
        {
            dR = dX + dY;
        }
    }
    else
    {
        dbVTmp1 = dX + dY;
        iSign = (((_iml_dp_union_t *) & dbVTmp1)->dwords.hi_dword >> 31);
        dbVTmp2 = ((__constant double *) __dpowr_la_nolut_CoutTab)[853];
        (((_iml_dp_union_t *) & dbVTmp2)->dwords.hi_dword =
         (((_iml_dp_union_t *) & dbVTmp2)->dwords.hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));
        dR = dbVTmp2 * dbVTmp2;
    }
    *r = dR;
    return nRet;
}

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc16 = { 0xbfc0eb775ed0d53fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc15 = { 0x3fc1ea5c772d0f69UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc14 = { 0xbfc243278b687c88UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc13 = { 0x3fc3ac83f2e91adfUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc12 = { 0xbfc55569367812bfUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc11 = { 0x3fc745de6106c97eUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc10 = { 0xbfc99999760c1f82UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc9 = { 0x3fcc71c70a4bb945UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc8 = { 0xbfd00000001076daUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc7 = { 0x3fd24924924f345dUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc6 = { 0xbfd5555555554e88UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc5 = { 0x3fd9999999999815UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc4l = { 0xbc8A6AF5D88E6C6DUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc4 = { 0xbfe0000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc3l = { 0x3c8751507e77d245UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_lc3 = { 0x3fe5555555555555UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_LN2H = { 0x3FE62E42FEFA3800UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_LN2L = { 0x3D2EF35793C76800UL };

static __constant unsigned long __dpowr_la_lTh0 = 0xc086232bdd7abae4UL;
static __constant unsigned long __dpowr_la_lTl0 = 0xbdcee3dde7fd844cUL;
static __constant unsigned long __dpowr_la_lTh1 = 0xc08624f4dcf7348dUL;
static __constant unsigned long __dpowr_la_lTl1 = 0xbdceedff94235c6bUL;
static __constant unsigned long __dpowr_la_lTh2 = 0xc086266a41f852d7UL;
static __constant unsigned long __dpowr_la_lTl2 = 0xbdcee475cd6a9f39UL;
static __constant unsigned long __dpowr_la_lTh3 = 0xc08627a5f55256f4UL;
static __constant unsigned long __dpowr_la_lTl3 = 0xbdcee71edfaeeae4UL;
static __constant unsigned long __dpowr_la_lTh4 = 0xc08628b76e3a7972UL;
static __constant unsigned long __dpowr_la_lTl4 = 0xbdceeb9abde27626UL;
static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_L2E = { 0x3ff71547652B82FEUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_Shifter = { 0x43280000000007feUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_NL2H = { 0xbfe62e42fefa39efUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_NL2L = { 0xbc7abc9e3b39803fUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c0 = { 0x3fdffffffffffe76UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c1 = { 0x3fc5555555555462UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c2 = { 0x3fa55555556228ceUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c3 = { 0x3f811111111ac486UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c4 = { 0x3f56c16b8144bd5bUL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c5 = { 0x3f2a019f7560fba3UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c6 = { 0x3efa072e44b58159UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_c7 = { 0x3ec722bccc270959UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_p_one = { 0x3ff0000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_two_52 = { 0x4330000000000000UL };

static __constant union
{
    unsigned long w;
    unsigned int w32[2];
    int s32[2];
    double f;
} __dpowr_la_two_64 = { 0x43f0000000000000UL };

__attribute__((always_inline))
inline int __internal_dpowr_nolut_cout (double *pxin, double *pyin, double *pres)
{
    int nRet = 0;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } x, y, mant_x, rcp, Th, Tl, lTh01, lTl01, lTh34, lTl34;
    union
    {
        unsigned int w;
        float f;
    } mantf, rcpf;
    double lpoly, R, R2h, R2l, RS, RS2, c3Rh, c3Rl, c3R3h, c3R3l, RS2_h, RS2_l;
    double H, L, R_half, expon_x;
    int iexpon, y_is_odd, y_is_even, ires_scale;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } xi, zero, res_special, ylx_h, ylx_l, scale, res;
    union
    {
        unsigned long w;
        unsigned int w32[2];
        int s32[2];
        double f;
    } idx, T, Tlr, dI, dyi, res_scale, ylx_h0, ylx_lh;
    double N, R0, poly, ya;
    int expon32, mask32, mask_h, index;
    unsigned int xa32, sgn_x, expon_corr, iexpon_x, res_sgn = 0;
    double xin = (*pxin);
    double yin = (*pyin);
    x.f = xin;
    y.f = yin;
  LOG_MAIN:
    iexpon_x = x.w32[1] >> 20;
    expon_x = (double) (iexpon_x);
  LOG_MAIN_CONT:
    mant_x.w32[1] = (x.w32[1] & 0x000fffffu) | 0x3fc00000u;
    mant_x.w32[0] = x.w32[0];
    mantf.f = (float) mant_x.f;
    rcpf.f = 1.0f / (mantf.f);
    rcpf.f = SPIRV_OCL_BUILTIN(rint, _f32, ) (rcpf.f);
    rcp.f = (double) rcpf.f;
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (rcp.f, mant_x.f, (-1.0));
    index = (rcpf.w >> (23 - 2)) & 0x7;
    index ^= 4;
    lTh01.w = (index & 1) ? __dpowr_la_lTh1 : __dpowr_la_lTh0;
    lTl01.w = (index & 1) ? __dpowr_la_lTl1 : __dpowr_la_lTl0;
    lTh34.w = (index & 1) ? __dpowr_la_lTh3 : __dpowr_la_lTh4;
    lTl34.w = (index & 1) ? __dpowr_la_lTl3 : __dpowr_la_lTl4;
    Th.w = (index < 2) ? lTh01.w : __dpowr_la_lTh2;
    Tl.w = (index < 2) ? lTl01.w : __dpowr_la_lTl2;
    Th.w = (index > 2) ? lTh34.w : Th.w;
    Tl.w = (index > 2) ? lTl34.w : Tl.w;
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_lc16.f, R, __dpowr_la_lc15.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc14.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc13.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc12.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc11.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc10.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc9.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc8.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc7.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc6.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc5.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc4.f);
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, __dpowr_la_lc3l.f);
    R_half = 0.5 * R;
    RS = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-R), R_half, R);
    R2h = R - RS;
    R2l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-R), R_half, R2h);
    c3Rh = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_lc3.f, R, 0.0);
    c3Rl = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_lc3.f, R, (-c3Rh));
    lpoly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (lpoly, R, c3Rl);
    Tl.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-R2l), lpoly, Tl.f);
    Tl.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (expon_x, __dpowr_la_LN2L.f, Tl.f);
    RS2 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c3Rh, R2h, RS);
    c3R3h = RS2 - RS;
    c3R3l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (c3Rh, R2h, (-c3R3h));
    c3R3l = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) ((-c3Rh), R2l, c3R3l);
    R2l = R2l + c3R3l;
    Th.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (expon_x, __dpowr_la_LN2H.f, Th.f);
    H = Th.f + RS2;
    RS2_h = H - Th.f;
    RS2_l = RS2 - RS2_h;
    R2l = R2l + RS2_l;
    Tl.f = Tl.f + R2l;
    L = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (R2h, lpoly, Tl.f);
    ylx_h0.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (y.f, H, 0.0);
    ylx_l.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (y.f, H, (-ylx_h0.f));
    ylx_l.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (y.f, L, ylx_l.f);
    ylx_h.f = ylx_h0.f + ylx_l.f;
    ylx_lh.f = ylx_h.f - ylx_h0.f;
    ylx_l.f = ylx_l.f - ylx_lh.f;
    iexpon_x--;
    if ((iexpon_x >= 0x7fe) || ((ylx_h.w32[1] & 0x7fffffff) >= 0x4086232B))
        return __internal_dpowr_lut_cout (pxin, pyin, pres);
    idx.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (ylx_h.f, __dpowr_la_p_L2E.f, __dpowr_la_p_Shifter.f);
    N = idx.f - __dpowr_la_p_Shifter.f;
    mask32 = idx.w32[0] << 31;
    expon32 = idx.w32[0] << (20 + 31 - 32);
    R0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_p_NL2H.f, N, ylx_h.f);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_p_NL2L.f, N, R0);
    R = R + ylx_l.f;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_p_c7.f, R, __dpowr_la_p_c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c5.f);
    mask32 = mask32 >> 31;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c3.f);
    mask_h = mask32 & 0x000EA09E;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c1.f);
    T.w32[1] = expon32 ^ mask_h;
    T.w32[0] = mask32 & 0x667F3BCD;
    Tlr.w32[1] = 0x3C6E51C5 ^ (mask32 & (0xBC8FD36E ^ 0x3C6E51C5));
    Tlr.w32[0] = 0;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c0.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_one.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, Tlr.f);
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, T.f);
    res.w32[1] ^= res_sgn;
    *pres = res.f;
    return nRet;
  POW_SPECIAL:
    if (y.f == 0)
    {
        *pres = 1.0;
        return nRet;
    }
    if (x.f == 1.0)
    {
        res.w32[1] = res_sgn | 0x3ff00000;
        res.w32[0] = 0;
        *pres = res.f;
        return nRet;
    }
    if (((x.w << 1) > 0xffe0000000000000UL) || ((y.w << 1) > 0xffe0000000000000UL))
    {
        *pres = x.f + y.f;
        return nRet;
    }
    if (y.w == 0xfff0000000000000UL)
    {
        if (x.f == -1.0)
        {
            *pres = 1.0;
            return nRet;
        }
        res.w = (SPIRV_OCL_BUILTIN(fabs, _f64, ) (x.f) < 1.0) ? 0x7ff0000000000000UL : 0;
        nRet = 1;
        *pres = res.f;
        return nRet;
    }
    if (y.w == 0x7ff0000000000000UL)
    {
        if (x.f == -1.0)
        {
            *pres = 1.0;
            return nRet;
        }
        res.w = (SPIRV_OCL_BUILTIN(fabs, _f64, ) (x.f) < 1.0) ? 0 : 0x7ff0000000000000UL;
        *pres = res.f;
        return nRet;
    }
    if (x.w == 0x7ff0000000000000UL)
    {
        res.w = (y.f < 0) ? 0 : 0x7ff0000000000000UL;
        *pres = res.f;
        return nRet;
    }
    ya = SPIRV_OCL_BUILTIN(fabs, _f64, ) (y.f);
    if (ya >= __dpowr_la_two_52.f)
    {
        if (ya * 0.5 >= __dpowr_la_two_52.f)
        {
            y_is_odd = 0;
            y_is_even = 1;
        }
        else
        {
            y_is_odd = y.w & 1;
            y_is_even = y_is_odd ^ 1;
        }
    }
    else
    {
        y_is_odd = y_is_even = 0;
        dI.f = __dpowr_la_two_52.f + ya;
        dyi.f = dI.f - __dpowr_la_two_52.f;
        if (dyi.f == ya)
        {
            y_is_odd = dI.w32[0] & 1;
            y_is_even = y_is_odd ^ 1;
        }
    }
    if (x.w == 0xfff0000000000000UL)
    {
        if (y.f < 0)
            res.w = (y_is_odd) ? 0x8000000000000000UL : 0;
        else
            res.w = (y_is_odd) ? 0xfff0000000000000UL : 0x7ff0000000000000UL;
        *pres = res.f;
        return nRet;
    }
    if (x.f == 0)
    {
        if (y.f < 0)
        {
            res.w = 0x7ff0000000000000UL;
            if (y_is_odd)
                res.w |= (x.w & 0x8000000000000000UL);
            nRet = 1;
            *pres = res.f;
            return nRet;
        }
        res.w = (y_is_odd) ? (x.w & 0x8000000000000000UL) : 0;
        *pres = res.f;
        return nRet;
    }
    if (x.f < 0)
    {
        if (y_is_odd | y_is_even)
        {
            if (y_is_odd)
                res_sgn = 0x80000000;
            x.w ^= 0x8000000000000000UL;
            goto LOG_MAIN;
        }
        res.w = 0xfff8000000000000UL;
        nRet = 1;
        *pres = res.f;
        return nRet;
    }
    iexpon_x++;
    if (iexpon_x == 0)
    {
        x.f *= __dpowr_la_two_64.f;
        iexpon_x = x.w32[1] >> 20;
        expon_x = (double) (iexpon_x) - 64.0;
        goto LOG_MAIN_CONT;
    }
    res_scale.f = 1.0;
    if (ylx_h0.f < 0)
    {
        if (ylx_h0.f < -746.0)
        {
            res.w32[1] = res_sgn;
            res.w32[0] = 0;
            nRet = 4;
            *pres = res.f;
            return nRet;
        }
        ires_scale = -512 * 2;
        res_scale.w = 0x1ff0000000000000UL;
    }
    else
    {
        if (ylx_h0.f >= 710.0)
        {
            res.w32[1] = res_sgn | 0x7ff00000;
            res.w32[0] = 0;
            nRet = 3;
            *pres = res.f;
            return nRet;
        }
        ires_scale = 512 * 2;
        res_scale.w = 0x5ff0000000000000UL;
    }
    idx.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (ylx_h.f, __dpowr_la_p_L2E.f, __dpowr_la_p_Shifter.f);
    N = idx.f - __dpowr_la_p_Shifter.f;
    mask32 = idx.w32[0] << 31;
    idx.w32[0] -= ires_scale;
    expon32 = idx.w32[0] << (20 + 31 - 32);
    R0 = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_p_NL2H.f, N, ylx_h.f);
    R = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_p_NL2L.f, N, R0);
    R = R + ylx_l.f;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (__dpowr_la_p_c7.f, R, __dpowr_la_p_c6.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c5.f);
    mask32 = mask32 >> 31;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c4.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c3.f);
    mask_h = mask32 & 0x000EA09E;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c2.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c1.f);
    T.w32[1] = expon32 ^ mask_h;
    T.w32[0] = mask32 & 0x667F3BCD;
    Tlr.w32[1] = 0x3C6E51C5 ^ (mask32 & (0xBC8FD36E ^ 0x3C6E51C5));
    Tlr.w32[0] = 0;
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_c0.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, __dpowr_la_p_one.f);
    poly = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (poly, R, Tlr.f);
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (T.f, poly, T.f);
    res_scale.w32[1] ^= res_sgn;
    res.f = SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, ) (res.f, res_scale.f, 0.0);
    *pres = res.f;
    return nRet;
}

double __ocl_svml_powr_noLUT (double a, double b)
{
    double va1;
    double va2;
    double vr1;
    double r;
    va1 = a;
    va2 = b;
    __internal_dpowr_nolut_cout (&va1, &va2, &vr1);
    r = vr1;
    return r;
}
