/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//
//  1) Range reduction to [-Pi/2; +Pi/2] interval
//     a) Grab sign from source argument and save it.
//     b) Remove sign using AND operation
//     c) Getting octant Y by 1/Pi multiplication
//     d) Add "Right Shifter" value
//     e) Treat obtained value as integer for destination sign setting.
//        Shift first bit of this value to the last (sign) position
//     f) Change destination sign if source sign is negative
//        using XOR operation.
//     g) Subtract "Right Shifter" value
//     h) Subtract Y*PI from X argument, where PI divided to 4 parts:
//        X = X - Y*PI1 - Y*PI2 - Y*PI3 - Y*PI4;
//  2) Polynomial (minimax for sin within [-Pi/2; +Pi/2] interval)
//     a) Calculate X^2 = X * X
//     b) Calculate polynomial:
//        R = X + X * X^2 * (A3 + x^2 * (A5 + ......
//  3) Destination sign setting
//     a) Set shifted destination sign using XOR operation:
//        R = XOR( R, S );
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  unsigned int _sPtable[256][3];
} __ssin_ep_ReductionTab_t;
static __ssin_ep_ReductionTab_t __ocl_svml_internal_ssin_ep_reduction_data = {{
    /*     P_hi                  P_med               P_lo                */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 0 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 1 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 2 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 3 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 4 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 5 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 6 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 7 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 8 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 9 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 10 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 11 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 12 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 13 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 14 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 15 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 16 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 17 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 18 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 19 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 20 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 21 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 22 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 23 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 24 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 25 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 26 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 27 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 28 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 29 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 30 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 31 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 32 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 33 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 34 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 35 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 36 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 37 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 38 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 39 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 40 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 41 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 42 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 43 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 44 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 45 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 46 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 47 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 48 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 49 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 50 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 51 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 52 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 53 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 54 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 55 */
    {0x00000000u, 0x00000000u, 0x00000000u}, /* 56 */
    {0x00000000u, 0x00000000u, 0x00000001u}, /* 57 */
    {0x00000000u, 0x00000000u, 0x00000002u}, /* 58 */
    {0x00000000u, 0x00000000u, 0x00000005u}, /* 59 */
    {0x00000000u, 0x00000000u, 0x0000000Au}, /* 60 */
    {0x00000000u, 0x00000000u, 0x00000014u}, /* 61 */
    {0x00000000u, 0x00000000u, 0x00000028u}, /* 62 */
    {0x00000000u, 0x00000000u, 0x00000051u}, /* 63 */
    {0x00000000u, 0x00000000u, 0x000000A2u}, /* 64 */
    {0x00000000u, 0x00000000u, 0x00000145u}, /* 65 */
    {0x00000000u, 0x00000000u, 0x0000028Bu}, /* 66 */
    {0x00000000u, 0x00000000u, 0x00000517u}, /* 67 */
    {0x00000000u, 0x00000000u, 0x00000A2Fu}, /* 68 */
    {0x00000000u, 0x00000000u, 0x0000145Fu}, /* 69 */
    {0x00000000u, 0x00000000u, 0x000028BEu}, /* 70 */
    {0x00000000u, 0x00000000u, 0x0000517Cu}, /* 71 */
    {0x00000000u, 0x00000000u, 0x0000A2F9u}, /* 72 */
    {0x00000000u, 0x00000000u, 0x000145F3u}, /* 73 */
    {0x00000000u, 0x00000000u, 0x00028BE6u}, /* 74 */
    {0x00000000u, 0x00000000u, 0x000517CCu}, /* 75 */
    {0x00000000u, 0x00000000u, 0x000A2F98u}, /* 76 */
    {0x00000000u, 0x00000000u, 0x00145F30u}, /* 77 */
    {0x00000000u, 0x00000000u, 0x0028BE60u}, /* 78 */
    {0x00000000u, 0x00000000u, 0x00517CC1u}, /* 79 */
    {0x00000000u, 0x00000000u, 0x00A2F983u}, /* 80 */
    {0x00000000u, 0x00000000u, 0x0145F306u}, /* 81 */
    {0x00000000u, 0x00000000u, 0x028BE60Du}, /* 82 */
    {0x00000000u, 0x00000000u, 0x0517CC1Bu}, /* 83 */
    {0x00000000u, 0x00000000u, 0x0A2F9836u}, /* 84 */
    {0x00000000u, 0x00000000u, 0x145F306Du}, /* 85 */
    {0x00000000u, 0x00000000u, 0x28BE60DBu}, /* 86 */
    {0x00000000u, 0x00000000u, 0x517CC1B7u}, /* 87 */
    {0x00000000u, 0x00000000u, 0xA2F9836Eu}, /* 88 */
    {0x00000000u, 0x00000001u, 0x45F306DCu}, /* 89 */
    {0x00000000u, 0x00000002u, 0x8BE60DB9u}, /* 90 */
    {0x00000000u, 0x00000005u, 0x17CC1B72u}, /* 91 */
    {0x00000000u, 0x0000000Au, 0x2F9836E4u}, /* 92 */
    {0x00000000u, 0x00000014u, 0x5F306DC9u}, /* 93 */
    {0x00000000u, 0x00000028u, 0xBE60DB93u}, /* 94 */
    {0x00000000u, 0x00000051u, 0x7CC1B727u}, /* 95 */
    {0x00000000u, 0x000000A2u, 0xF9836E4Eu}, /* 96 */
    {0x00000000u, 0x00000145u, 0xF306DC9Cu}, /* 97 */
    {0x00000000u, 0x0000028Bu, 0xE60DB939u}, /* 98 */
    {0x00000000u, 0x00000517u, 0xCC1B7272u}, /* 99 */
    {0x00000000u, 0x00000A2Fu, 0x9836E4E4u}, /* 100 */
    {0x00000000u, 0x0000145Fu, 0x306DC9C8u}, /* 101 */
    {0x00000000u, 0x000028BEu, 0x60DB9391u}, /* 102 */
    {0x00000000u, 0x0000517Cu, 0xC1B72722u}, /* 103 */
    {0x00000000u, 0x0000A2F9u, 0x836E4E44u}, /* 104 */
    {0x00000000u, 0x000145F3u, 0x06DC9C88u}, /* 105 */
    {0x00000000u, 0x00028BE6u, 0x0DB93910u}, /* 106 */
    {0x00000000u, 0x000517CCu, 0x1B727220u}, /* 107 */
    {0x00000000u, 0x000A2F98u, 0x36E4E441u}, /* 108 */
    {0x00000000u, 0x00145F30u, 0x6DC9C882u}, /* 109 */
    {0x00000000u, 0x0028BE60u, 0xDB939105u}, /* 110 */
    {0x00000000u, 0x00517CC1u, 0xB727220Au}, /* 111 */
    {0x00000000u, 0x00A2F983u, 0x6E4E4415u}, /* 112 */
    {0x00000000u, 0x0145F306u, 0xDC9C882Au}, /* 113 */
    {0x00000000u, 0x028BE60Du, 0xB9391054u}, /* 114 */
    {0x00000000u, 0x0517CC1Bu, 0x727220A9u}, /* 115 */
    {0x00000000u, 0x0A2F9836u, 0xE4E44152u}, /* 116 */
    {0x00000000u, 0x145F306Du, 0xC9C882A5u}, /* 117 */
    {0x00000000u, 0x28BE60DBu, 0x9391054Au}, /* 118 */
    {0x00000000u, 0x517CC1B7u, 0x27220A94u}, /* 119 */
    {0x00000000u, 0xA2F9836Eu, 0x4E441529u}, /* 120 */
    {0x00000001u, 0x45F306DCu, 0x9C882A53u}, /* 121 */
    {0x00000002u, 0x8BE60DB9u, 0x391054A7u}, /* 122 */
    {0x00000005u, 0x17CC1B72u, 0x7220A94Fu}, /* 123 */
    {0x0000000Au, 0x2F9836E4u, 0xE441529Fu}, /* 124 */
    {0x00000014u, 0x5F306DC9u, 0xC882A53Fu}, /* 125 */
    {0x00000028u, 0xBE60DB93u, 0x91054A7Fu}, /* 126 */
    {0x00000051u, 0x7CC1B727u, 0x220A94FEu}, /* 127 */
    {0x000000A2u, 0xF9836E4Eu, 0x441529FCu}, /* 128 */
    {0x00000145u, 0xF306DC9Cu, 0x882A53F8u}, /* 129 */
    {0x0000028Bu, 0xE60DB939u, 0x1054A7F0u}, /* 130 */
    {0x00000517u, 0xCC1B7272u, 0x20A94FE1u}, /* 131 */
    {0x00000A2Fu, 0x9836E4E4u, 0x41529FC2u}, /* 132 */
    {0x0000145Fu, 0x306DC9C8u, 0x82A53F84u}, /* 133 */
    {0x000028BEu, 0x60DB9391u, 0x054A7F09u}, /* 134 */
    {0x0000517Cu, 0xC1B72722u, 0x0A94FE13u}, /* 135 */
    {0x0000A2F9u, 0x836E4E44u, 0x1529FC27u}, /* 136 */
    {0x000145F3u, 0x06DC9C88u, 0x2A53F84Eu}, /* 137 */
    {0x00028BE6u, 0x0DB93910u, 0x54A7F09Du}, /* 138 */
    {0x000517CCu, 0x1B727220u, 0xA94FE13Au}, /* 139 */
    {0x000A2F98u, 0x36E4E441u, 0x529FC275u}, /* 140 */
    {0x00145F30u, 0x6DC9C882u, 0xA53F84EAu}, /* 141 */
    {0x0028BE60u, 0xDB939105u, 0x4A7F09D5u}, /* 142 */
    {0x00517CC1u, 0xB727220Au, 0x94FE13ABu}, /* 143 */
    {0x00A2F983u, 0x6E4E4415u, 0x29FC2757u}, /* 144 */
    {0x0145F306u, 0xDC9C882Au, 0x53F84EAFu}, /* 145 */
    {0x028BE60Du, 0xB9391054u, 0xA7F09D5Fu}, /* 146 */
    {0x0517CC1Bu, 0x727220A9u, 0x4FE13ABEu}, /* 147 */
    {0x0A2F9836u, 0xE4E44152u, 0x9FC2757Du}, /* 148 */
    {0x145F306Du, 0xC9C882A5u, 0x3F84EAFAu}, /* 149 */
    {0x28BE60DBu, 0x9391054Au, 0x7F09D5F4u}, /* 150 */
    {0x517CC1B7u, 0x27220A94u, 0xFE13ABE8u}, /* 151 */
    {0xA2F9836Eu, 0x4E441529u, 0xFC2757D1u}, /* 152 */
    {0x45F306DCu, 0x9C882A53u, 0xF84EAFA3u}, /* 153 */
    {0x8BE60DB9u, 0x391054A7u, 0xF09D5F47u}, /* 154 */
    {0x17CC1B72u, 0x7220A94Fu, 0xE13ABE8Fu}, /* 155 */
    {0x2F9836E4u, 0xE441529Fu, 0xC2757D1Fu}, /* 156 */
    {0x5F306DC9u, 0xC882A53Fu, 0x84EAFA3Eu}, /* 157 */
    {0xBE60DB93u, 0x91054A7Fu, 0x09D5F47Du}, /* 158 */
    {0x7CC1B727u, 0x220A94FEu, 0x13ABE8FAu}, /* 159 */
    {0xF9836E4Eu, 0x441529FCu, 0x2757D1F5u}, /* 160 */
    {0xF306DC9Cu, 0x882A53F8u, 0x4EAFA3EAu}, /* 161 */
    {0xE60DB939u, 0x1054A7F0u, 0x9D5F47D4u}, /* 162 */
    {0xCC1B7272u, 0x20A94FE1u, 0x3ABE8FA9u}, /* 163 */
    {0x9836E4E4u, 0x41529FC2u, 0x757D1F53u}, /* 164 */
    {0x306DC9C8u, 0x82A53F84u, 0xEAFA3EA6u}, /* 165 */
    {0x60DB9391u, 0x054A7F09u, 0xD5F47D4Du}, /* 166 */
    {0xC1B72722u, 0x0A94FE13u, 0xABE8FA9Au}, /* 167 */
    {0x836E4E44u, 0x1529FC27u, 0x57D1F534u}, /* 168 */
    {0x06DC9C88u, 0x2A53F84Eu, 0xAFA3EA69u}, /* 169 */
    {0x0DB93910u, 0x54A7F09Du, 0x5F47D4D3u}, /* 170 */
    {0x1B727220u, 0xA94FE13Au, 0xBE8FA9A6u}, /* 171 */
    {0x36E4E441u, 0x529FC275u, 0x7D1F534Du}, /* 172 */
    {0x6DC9C882u, 0xA53F84EAu, 0xFA3EA69Bu}, /* 173 */
    {0xDB939105u, 0x4A7F09D5u, 0xF47D4D37u}, /* 174 */
    {0xB727220Au, 0x94FE13ABu, 0xE8FA9A6Eu}, /* 175 */
    {0x6E4E4415u, 0x29FC2757u, 0xD1F534DDu}, /* 176 */
    {0xDC9C882Au, 0x53F84EAFu, 0xA3EA69BBu}, /* 177 */
    {0xB9391054u, 0xA7F09D5Fu, 0x47D4D377u}, /* 178 */
    {0x727220A9u, 0x4FE13ABEu, 0x8FA9A6EEu}, /* 179 */
    {0xE4E44152u, 0x9FC2757Du, 0x1F534DDCu}, /* 180 */
    {0xC9C882A5u, 0x3F84EAFAu, 0x3EA69BB8u}, /* 181 */
    {0x9391054Au, 0x7F09D5F4u, 0x7D4D3770u}, /* 182 */
    {0x27220A94u, 0xFE13ABE8u, 0xFA9A6EE0u}, /* 183 */
    {0x4E441529u, 0xFC2757D1u, 0xF534DDC0u}, /* 184 */
    {0x9C882A53u, 0xF84EAFA3u, 0xEA69BB81u}, /* 185 */
    {0x391054A7u, 0xF09D5F47u, 0xD4D37703u}, /* 186 */
    {0x7220A94Fu, 0xE13ABE8Fu, 0xA9A6EE06u}, /* 187 */
    {0xE441529Fu, 0xC2757D1Fu, 0x534DDC0Du}, /* 188 */
    {0xC882A53Fu, 0x84EAFA3Eu, 0xA69BB81Bu}, /* 189 */
    {0x91054A7Fu, 0x09D5F47Du, 0x4D377036u}, /* 190 */
    {0x220A94FEu, 0x13ABE8FAu, 0x9A6EE06Du}, /* 191 */
    {0x441529FCu, 0x2757D1F5u, 0x34DDC0DBu}, /* 192 */
    {0x882A53F8u, 0x4EAFA3EAu, 0x69BB81B6u}, /* 193 */
    {0x1054A7F0u, 0x9D5F47D4u, 0xD377036Du}, /* 194 */
    {0x20A94FE1u, 0x3ABE8FA9u, 0xA6EE06DBu}, /* 195 */
    {0x41529FC2u, 0x757D1F53u, 0x4DDC0DB6u}, /* 196 */
    {0x82A53F84u, 0xEAFA3EA6u, 0x9BB81B6Cu}, /* 197 */
    {0x054A7F09u, 0xD5F47D4Du, 0x377036D8u}, /* 198 */
    {0x0A94FE13u, 0xABE8FA9Au, 0x6EE06DB1u}, /* 199 */
    {0x1529FC27u, 0x57D1F534u, 0xDDC0DB62u}, /* 200 */
    {0x2A53F84Eu, 0xAFA3EA69u, 0xBB81B6C5u}, /* 201 */
    {0x54A7F09Du, 0x5F47D4D3u, 0x77036D8Au}, /* 202 */
    {0xA94FE13Au, 0xBE8FA9A6u, 0xEE06DB14u}, /* 203 */
    {0x529FC275u, 0x7D1F534Du, 0xDC0DB629u}, /* 204 */
    {0xA53F84EAu, 0xFA3EA69Bu, 0xB81B6C52u}, /* 205 */
    {0x4A7F09D5u, 0xF47D4D37u, 0x7036D8A5u}, /* 206 */
    {0x94FE13ABu, 0xE8FA9A6Eu, 0xE06DB14Au}, /* 207 */
    {0x29FC2757u, 0xD1F534DDu, 0xC0DB6295u}, /* 208 */
    {0x53F84EAFu, 0xA3EA69BBu, 0x81B6C52Bu}, /* 209 */
    {0xA7F09D5Fu, 0x47D4D377u, 0x036D8A56u}, /* 210 */
    {0x4FE13ABEu, 0x8FA9A6EEu, 0x06DB14ACu}, /* 211 */
    {0x9FC2757Du, 0x1F534DDCu, 0x0DB62959u}, /* 212 */
    {0x3F84EAFAu, 0x3EA69BB8u, 0x1B6C52B3u}, /* 213 */
    {0x7F09D5F4u, 0x7D4D3770u, 0x36D8A566u}, /* 214 */
    {0xFE13ABE8u, 0xFA9A6EE0u, 0x6DB14ACCu}, /* 215 */
    {0xFC2757D1u, 0xF534DDC0u, 0xDB629599u}, /* 216 */
    {0xF84EAFA3u, 0xEA69BB81u, 0xB6C52B32u}, /* 217 */
    {0xF09D5F47u, 0xD4D37703u, 0x6D8A5664u}, /* 218 */
    {0xE13ABE8Fu, 0xA9A6EE06u, 0xDB14ACC9u}, /* 219 */
    {0xC2757D1Fu, 0x534DDC0Du, 0xB6295993u}, /* 220 */
    {0x84EAFA3Eu, 0xA69BB81Bu, 0x6C52B327u}, /* 221 */
    {0x09D5F47Du, 0x4D377036u, 0xD8A5664Fu}, /* 222 */
    {0x13ABE8FAu, 0x9A6EE06Du, 0xB14ACC9Eu}, /* 223 */
    {0x2757D1F5u, 0x34DDC0DBu, 0x6295993Cu}, /* 224 */
    {0x4EAFA3EAu, 0x69BB81B6u, 0xC52B3278u}, /* 225 */
    {0x9D5F47D4u, 0xD377036Du, 0x8A5664F1u}, /* 226 */
    {0x3ABE8FA9u, 0xA6EE06DBu, 0x14ACC9E2u}, /* 227 */
    {0x757D1F53u, 0x4DDC0DB6u, 0x295993C4u}, /* 228 */
    {0xEAFA3EA6u, 0x9BB81B6Cu, 0x52B32788u}, /* 229 */
    {0xD5F47D4Du, 0x377036D8u, 0xA5664F10u}, /* 230 */
    {0xABE8FA9Au, 0x6EE06DB1u, 0x4ACC9E21u}, /* 231 */
    {0x57D1F534u, 0xDDC0DB62u, 0x95993C43u}, /* 232 */
    {0xAFA3EA69u, 0xBB81B6C5u, 0x2B327887u}, /* 233 */
    {0x5F47D4D3u, 0x77036D8Au, 0x5664F10Eu}, /* 234 */
    {0xBE8FA9A6u, 0xEE06DB14u, 0xACC9E21Cu}, /* 235 */
    {0x7D1F534Du, 0xDC0DB629u, 0x5993C439u}, /* 236 */
    {0xFA3EA69Bu, 0xB81B6C52u, 0xB3278872u}, /* 237 */
    {0xF47D4D37u, 0x7036D8A5u, 0x664F10E4u}, /* 238 */
    {0xE8FA9A6Eu, 0xE06DB14Au, 0xCC9E21C8u}, /* 239 */
    {0xD1F534DDu, 0xC0DB6295u, 0x993C4390u}, /* 240 */
    {0xA3EA69BBu, 0x81B6C52Bu, 0x32788720u}, /* 241 */
    {0x47D4D377u, 0x036D8A56u, 0x64F10E41u}, /* 242 */
    {0x8FA9A6EEu, 0x06DB14ACu, 0xC9E21C82u}, /* 243 */
    {0x1F534DDCu, 0x0DB62959u, 0x93C43904u}, /* 244 */
    {0x3EA69BB8u, 0x1B6C52B3u, 0x27887208u}, /* 245 */
    {0x7D4D3770u, 0x36D8A566u, 0x4F10E410u}, /* 246 */
    {0xFA9A6EE0u, 0x6DB14ACCu, 0x9E21C820u}, /* 247 */
    {0xF534DDC0u, 0xDB629599u, 0x3C439041u}, /* 248 */
    {0xEA69BB81u, 0xB6C52B32u, 0x78872083u}, /* 249 */
    {0xD4D37703u, 0x6D8A5664u, 0xF10E4107u}, /* 250 */
    {0xA9A6EE06u, 0xDB14ACC9u, 0xE21C820Fu}, /* 251 */
    {0x534DDC0Du, 0xB6295993u, 0xC439041Fu}, /* 252 */
    {0xA69BB81Bu, 0x6C52B327u, 0x8872083Fu}, /* 253 */
    {0x4D377036u, 0xD8A5664Fu, 0x10E4107Fu}, /* 254 */
    {0x9A6EE06Du, 0xB14ACC9Eu, 0x21C820FFu}  /* 255 */
}};                                          /*sReduction_Table*/
/* Table parameters */
typedef struct {
  unsigned int _dT[256][4];
  unsigned int _sAbsMask;
  unsigned int _sRangeReductionVal;
  unsigned int _sRangeVal;
  unsigned int _sS1;
  unsigned int _sS2;
  unsigned int _sC1;
  unsigned int _sC2;
  unsigned int _sPI1;
  unsigned int _sPI2;
  unsigned int _sPI3;
  unsigned int _sPI4;
  unsigned int _sPI1_FMA;
  unsigned int _sPI2_FMA;
  unsigned int _sPI3_FMA;
  unsigned int _sA1;
  unsigned int _sA3;
  unsigned int _sA5;
  unsigned int _sInvPI;
  unsigned int _sRShifter;
} __ocl_svml_internal_ssin_ep_data_t;
static __ocl_svml_internal_ssin_ep_data_t __ocl_svml_internal_ssin_ep_data = {
    {
        /* > Lookup table for high accuracy version (CHL,SHi,SLo,Sigma): */
        {0x00000000u, 0x00000000u, 0x00000000u, 0x3F800000u},
        {0xB99DE7DFu, 0x3CC90AB0u, 0xB005C998u, 0x3F800000u},
        {0xBA9DE1C8u, 0x3D48FB30u, 0xB0EF227Fu, 0x3F800000u},
        {0xBB319298u, 0x3D96A905u, 0xB1531E61u, 0x3F800000u},
        {0xBB9DC971u, 0x3DC8BD36u, 0xB07592F5u, 0x3F800000u},
        {0xBBF66E3Cu, 0x3DFAB273u, 0xB11568CFu, 0x3F800000u},
        {0xBC315502u, 0x3E164083u, 0x31E8E614u, 0x3F800000u},
        {0xBC71360Bu, 0x3E2F10A2u, 0x311167F9u, 0x3F800000u},
        {0xBC9D6830u, 0x3E47C5C2u, 0xB0E5967Du, 0x3F800000u},
        {0xBCC70C54u, 0x3E605C13u, 0x31A7E4F6u, 0x3F800000u},
        {0xBCF58104u, 0x3E78CFCCu, 0xB11BD41Du, 0x3F800000u},
        {0xBD145F8Cu, 0x3E888E93u, 0x312C7D9Eu, 0x3F800000u},
        {0xBD305F55u, 0x3E94A031u, 0x326D59F0u, 0x3F800000u},
        {0xBD4EBB8Au, 0x3EA09AE5u, 0xB23E89A0u, 0x3F800000u},
        {0xBD6F6F7Eu, 0x3EAC7CD4u, 0xB2254E02u, 0x3F800000u},
        {0xBD893B12u, 0x3EB8442Au, 0xB2705BA6u, 0x3F800000u},
        {0xBD9BE50Cu, 0x3EC3EF15u, 0x31D5D52Cu, 0x3F800000u},
        {0xBDAFB2CCu, 0x3ECF7BCAu, 0x316A3B63u, 0x3F800000u},
        {0xBDC4A143u, 0x3EDAE880u, 0x321E15CCu, 0x3F800000u},
        {0xBDDAAD38u, 0x3EE63375u, 0xB1D9C774u, 0x3F800000u},
        {0xBDF1D344u, 0x3EF15AEAu, 0xB1FF2139u, 0x3F800000u},
        {0xBE0507EAu, 0x3EFC5D27u, 0xB180ECA9u, 0x3F800000u},
        {0xBE11AF97u, 0x3F039C3Du, 0xB25BA002u, 0x3F800000u},
        {0xBE1EDEB5u, 0x3F08F59Bu, 0xB2BE4B4Eu, 0x3F800000u},
        {0xBE2C933Bu, 0x3F0E39DAu, 0xB24A32E7u, 0x3F800000u},
        {0xBE3ACB0Cu, 0x3F13682Au, 0x32CDD12Eu, 0x3F800000u},
        {0xBE4983F7u, 0x3F187FC0u, 0xB1C7A3F3u, 0x3F800000u},
        {0xBE58BBB7u, 0x3F1D7FD1u, 0x3292050Cu, 0x3F800000u},
        {0xBE686FF3u, 0x3F226799u, 0x322123BBu, 0x3F800000u},
        {0xBE789E3Fu, 0x3F273656u, 0xB2038343u, 0x3F800000u},
        {0xBE84A20Eu, 0x3F2BEB4Au, 0xB2B73136u, 0x3F800000u},
        {0xBE8D2F7Du, 0x3F3085BBu, 0xB2AE2D32u, 0x3F800000u},
        {0xBE95F61Au, 0x3F3504F3u, 0x324FE77Au, 0x3F800000u},
        {0x3E4216EBu, 0x3F396842u, 0xB2810007u, 0x3F000000u},
        {0x3E2FAD27u, 0x3F3DAEF9u, 0x319AABECu, 0x3F000000u},
        {0x3E1CD957u, 0x3F41D870u, 0x32BFF977u, 0x3F000000u},
        {0x3E099E65u, 0x3F45E403u, 0x32B15174u, 0x3F000000u},
        {0x3DEBFE8Au, 0x3F49D112u, 0x32992640u, 0x3F000000u},
        {0x3DC3FDFFu, 0x3F4D9F02u, 0x327E70E8u, 0x3F000000u},
        {0x3D9B4153u, 0x3F514D3Du, 0x300C4F04u, 0x3F000000u},
        {0x3D639D9Du, 0x3F54DB31u, 0x3290EA1Au, 0x3F000000u},
        {0x3D0F59AAu, 0x3F584853u, 0xB27D5FC0u, 0x3F000000u},
        {0x3C670F32u, 0x3F5B941Au, 0x32232DC8u, 0x3F000000u},
        {0xBBE8B648u, 0x3F5EBE05u, 0x32C6F953u, 0x3F000000u},
        {0xBCEA5164u, 0x3F61C598u, 0xB2E7F425u, 0x3F000000u},
        {0xBD4E645Au, 0x3F64AA59u, 0x311A08FAu, 0x3F000000u},
        {0xBD945DFFu, 0x3F676BD8u, 0xB2BC3389u, 0x3F000000u},
        {0xBDC210D8u, 0x3F6A09A7u, 0xB2EB236Cu, 0x3F000000u},
        {0xBDF043ABu, 0x3F6C835Eu, 0x32F328D4u, 0x3F000000u},
        {0xBE0F77ADu, 0x3F6ED89Eu, 0xB29333DCu, 0x3F000000u},
        {0x3DB1F34Fu, 0x3F710908u, 0x321ED0DDu, 0x3E800000u},
        {0x3D826B93u, 0x3F731447u, 0x32C48E11u, 0x3E800000u},
        {0x3D25018Cu, 0x3F74FA0Bu, 0xB2939D22u, 0x3E800000u},
        {0x3C88E931u, 0x3F76BA07u, 0x326D092Cu, 0x3E800000u},
        {0xBBE60685u, 0x3F7853F8u, 0xB20DB9E5u, 0x3E800000u},
        {0xBCFD1F65u, 0x3F79C79Du, 0x32C64E59u, 0x3E800000u},
        {0xBD60E8F8u, 0x3F7B14BEu, 0x32FF75CBu, 0x3E800000u},
        {0x3D3C4289u, 0x3F7C3B28u, 0xB231D68Bu, 0x3E000000u},
        {0x3CB2041Cu, 0x3F7D3AACu, 0xB0F75AE9u, 0x3E000000u},
        {0xBB29B1A9u, 0x3F7E1324u, 0xB2F1E603u, 0x3E000000u},
        {0xBCDD0B28u, 0x3F7EC46Du, 0x31F44949u, 0x3E000000u},
        {0x3C354825u, 0x3F7F4E6Du, 0x32D01884u, 0x3D800000u},
        {0xBC5C1342u, 0x3F7FB10Fu, 0x31DE5B5Fu, 0x3D800000u},
        {0xBBDBD541u, 0x3F7FEC43u, 0x3084CD0Du, 0x3D000000u},
        {0x00000000u, 0x3F800000u, 0x00000000u, 0x00000000u},
        {0x3BDBD541u, 0x3F7FEC43u, 0x3084CD0Du, 0xBD000000u},
        {0x3C5C1342u, 0x3F7FB10Fu, 0x31DE5B5Fu, 0xBD800000u},
        {0xBC354825u, 0x3F7F4E6Du, 0x32D01884u, 0xBD800000u},
        {0x3CDD0B28u, 0x3F7EC46Du, 0x31F44949u, 0xBE000000u},
        {0x3B29B1A9u, 0x3F7E1324u, 0xB2F1E603u, 0xBE000000u},
        {0xBCB2041Cu, 0x3F7D3AACu, 0xB0F75AE9u, 0xBE000000u},
        {0xBD3C4289u, 0x3F7C3B28u, 0xB231D68Bu, 0xBE000000u},
        {0x3D60E8F8u, 0x3F7B14BEu, 0x32FF75CBu, 0xBE800000u},
        {0x3CFD1F65u, 0x3F79C79Du, 0x32C64E59u, 0xBE800000u},
        {0x3BE60685u, 0x3F7853F8u, 0xB20DB9E5u, 0xBE800000u},
        {0xBC88E931u, 0x3F76BA07u, 0x326D092Cu, 0xBE800000u},
        {0xBD25018Cu, 0x3F74FA0Bu, 0xB2939D22u, 0xBE800000u},
        {0xBD826B93u, 0x3F731447u, 0x32C48E11u, 0xBE800000u},
        {0xBDB1F34Fu, 0x3F710908u, 0x321ED0DDu, 0xBE800000u},
        {0x3E0F77ADu, 0x3F6ED89Eu, 0xB29333DCu, 0xBF000000u},
        {0x3DF043ABu, 0x3F6C835Eu, 0x32F328D4u, 0xBF000000u},
        {0x3DC210D8u, 0x3F6A09A7u, 0xB2EB236Cu, 0xBF000000u},
        {0x3D945DFFu, 0x3F676BD8u, 0xB2BC3389u, 0xBF000000u},
        {0x3D4E645Au, 0x3F64AA59u, 0x311A08FAu, 0xBF000000u},
        {0x3CEA5164u, 0x3F61C598u, 0xB2E7F425u, 0xBF000000u},
        {0x3BE8B648u, 0x3F5EBE05u, 0x32C6F953u, 0xBF000000u},
        {0xBC670F32u, 0x3F5B941Au, 0x32232DC8u, 0xBF000000u},
        {0xBD0F59AAu, 0x3F584853u, 0xB27D5FC0u, 0xBF000000u},
        {0xBD639D9Du, 0x3F54DB31u, 0x3290EA1Au, 0xBF000000u},
        {0xBD9B4153u, 0x3F514D3Du, 0x300C4F04u, 0xBF000000u},
        {0xBDC3FDFFu, 0x3F4D9F02u, 0x327E70E8u, 0xBF000000u},
        {0xBDEBFE8Au, 0x3F49D112u, 0x32992640u, 0xBF000000u},
        {0xBE099E65u, 0x3F45E403u, 0x32B15174u, 0xBF000000u},
        {0xBE1CD957u, 0x3F41D870u, 0x32BFF977u, 0xBF000000u},
        {0xBE2FAD27u, 0x3F3DAEF9u, 0x319AABECu, 0xBF000000u},
        {0xBE4216EBu, 0x3F396842u, 0xB2810007u, 0xBF000000u},
        {0x3E95F61Au, 0x3F3504F3u, 0x324FE77Au, 0xBF800000u},
        {0x3E8D2F7Du, 0x3F3085BBu, 0xB2AE2D32u, 0xBF800000u},
        {0x3E84A20Eu, 0x3F2BEB4Au, 0xB2B73136u, 0xBF800000u},
        {0x3E789E3Fu, 0x3F273656u, 0xB2038343u, 0xBF800000u},
        {0x3E686FF3u, 0x3F226799u, 0x322123BBu, 0xBF800000u},
        {0x3E58BBB7u, 0x3F1D7FD1u, 0x3292050Cu, 0xBF800000u},
        {0x3E4983F7u, 0x3F187FC0u, 0xB1C7A3F3u, 0xBF800000u},
        {0x3E3ACB0Cu, 0x3F13682Au, 0x32CDD12Eu, 0xBF800000u},
        {0x3E2C933Bu, 0x3F0E39DAu, 0xB24A32E7u, 0xBF800000u},
        {0x3E1EDEB5u, 0x3F08F59Bu, 0xB2BE4B4Eu, 0xBF800000u},
        {0x3E11AF97u, 0x3F039C3Du, 0xB25BA002u, 0xBF800000u},
        {0x3E0507EAu, 0x3EFC5D27u, 0xB180ECA9u, 0xBF800000u},
        {0x3DF1D344u, 0x3EF15AEAu, 0xB1FF2139u, 0xBF800000u},
        {0x3DDAAD38u, 0x3EE63375u, 0xB1D9C774u, 0xBF800000u},
        {0x3DC4A143u, 0x3EDAE880u, 0x321E15CCu, 0xBF800000u},
        {0x3DAFB2CCu, 0x3ECF7BCAu, 0x316A3B63u, 0xBF800000u},
        {0x3D9BE50Cu, 0x3EC3EF15u, 0x31D5D52Cu, 0xBF800000u},
        {0x3D893B12u, 0x3EB8442Au, 0xB2705BA6u, 0xBF800000u},
        {0x3D6F6F7Eu, 0x3EAC7CD4u, 0xB2254E02u, 0xBF800000u},
        {0x3D4EBB8Au, 0x3EA09AE5u, 0xB23E89A0u, 0xBF800000u},
        {0x3D305F55u, 0x3E94A031u, 0x326D59F0u, 0xBF800000u},
        {0x3D145F8Cu, 0x3E888E93u, 0x312C7D9Eu, 0xBF800000u},
        {0x3CF58104u, 0x3E78CFCCu, 0xB11BD41Du, 0xBF800000u},
        {0x3CC70C54u, 0x3E605C13u, 0x31A7E4F6u, 0xBF800000u},
        {0x3C9D6830u, 0x3E47C5C2u, 0xB0E5967Du, 0xBF800000u},
        {0x3C71360Bu, 0x3E2F10A2u, 0x311167F9u, 0xBF800000u},
        {0x3C315502u, 0x3E164083u, 0x31E8E614u, 0xBF800000u},
        {0x3BF66E3Cu, 0x3DFAB273u, 0xB11568CFu, 0xBF800000u},
        {0x3B9DC971u, 0x3DC8BD36u, 0xB07592F5u, 0xBF800000u},
        {0x3B319298u, 0x3D96A905u, 0xB1531E61u, 0xBF800000u},
        {0x3A9DE1C8u, 0x3D48FB30u, 0xB0EF227Fu, 0xBF800000u},
        {0x399DE7DFu, 0x3CC90AB0u, 0xB005C998u, 0xBF800000u},
        {0x00000000u, 0x00000000u, 0x00000000u, 0xBF800000u},
        {0x399DE7DFu, 0xBCC90AB0u, 0x3005C998u, 0xBF800000u},
        {0x3A9DE1C8u, 0xBD48FB30u, 0x30EF227Fu, 0xBF800000u},
        {0x3B319298u, 0xBD96A905u, 0x31531E61u, 0xBF800000u},
        {0x3B9DC971u, 0xBDC8BD36u, 0x307592F5u, 0xBF800000u},
        {0x3BF66E3Cu, 0xBDFAB273u, 0x311568CFu, 0xBF800000u},
        {0x3C315502u, 0xBE164083u, 0xB1E8E614u, 0xBF800000u},
        {0x3C71360Bu, 0xBE2F10A2u, 0xB11167F9u, 0xBF800000u},
        {0x3C9D6830u, 0xBE47C5C2u, 0x30E5967Du, 0xBF800000u},
        {0x3CC70C54u, 0xBE605C13u, 0xB1A7E4F6u, 0xBF800000u},
        {0x3CF58104u, 0xBE78CFCCu, 0x311BD41Du, 0xBF800000u},
        {0x3D145F8Cu, 0xBE888E93u, 0xB12C7D9Eu, 0xBF800000u},
        {0x3D305F55u, 0xBE94A031u, 0xB26D59F0u, 0xBF800000u},
        {0x3D4EBB8Au, 0xBEA09AE5u, 0x323E89A0u, 0xBF800000u},
        {0x3D6F6F7Eu, 0xBEAC7CD4u, 0x32254E02u, 0xBF800000u},
        {0x3D893B12u, 0xBEB8442Au, 0x32705BA6u, 0xBF800000u},
        {0x3D9BE50Cu, 0xBEC3EF15u, 0xB1D5D52Cu, 0xBF800000u},
        {0x3DAFB2CCu, 0xBECF7BCAu, 0xB16A3B63u, 0xBF800000u},
        {0x3DC4A143u, 0xBEDAE880u, 0xB21E15CCu, 0xBF800000u},
        {0x3DDAAD38u, 0xBEE63375u, 0x31D9C774u, 0xBF800000u},
        {0x3DF1D344u, 0xBEF15AEAu, 0x31FF2139u, 0xBF800000u},
        {0x3E0507EAu, 0xBEFC5D27u, 0x3180ECA9u, 0xBF800000u},
        {0x3E11AF97u, 0xBF039C3Du, 0x325BA002u, 0xBF800000u},
        {0x3E1EDEB5u, 0xBF08F59Bu, 0x32BE4B4Eu, 0xBF800000u},
        {0x3E2C933Bu, 0xBF0E39DAu, 0x324A32E7u, 0xBF800000u},
        {0x3E3ACB0Cu, 0xBF13682Au, 0xB2CDD12Eu, 0xBF800000u},
        {0x3E4983F7u, 0xBF187FC0u, 0x31C7A3F3u, 0xBF800000u},
        {0x3E58BBB7u, 0xBF1D7FD1u, 0xB292050Cu, 0xBF800000u},
        {0x3E686FF3u, 0xBF226799u, 0xB22123BBu, 0xBF800000u},
        {0x3E789E3Fu, 0xBF273656u, 0x32038343u, 0xBF800000u},
        {0x3E84A20Eu, 0xBF2BEB4Au, 0x32B73136u, 0xBF800000u},
        {0x3E8D2F7Du, 0xBF3085BBu, 0x32AE2D32u, 0xBF800000u},
        {0x3E95F61Au, 0xBF3504F3u, 0xB24FE77Au, 0xBF800000u},
        {0xBE4216EBu, 0xBF396842u, 0x32810007u, 0xBF000000u},
        {0xBE2FAD27u, 0xBF3DAEF9u, 0xB19AABECu, 0xBF000000u},
        {0xBE1CD957u, 0xBF41D870u, 0xB2BFF977u, 0xBF000000u},
        {0xBE099E65u, 0xBF45E403u, 0xB2B15174u, 0xBF000000u},
        {0xBDEBFE8Au, 0xBF49D112u, 0xB2992640u, 0xBF000000u},
        {0xBDC3FDFFu, 0xBF4D9F02u, 0xB27E70E8u, 0xBF000000u},
        {0xBD9B4153u, 0xBF514D3Du, 0xB00C4F04u, 0xBF000000u},
        {0xBD639D9Du, 0xBF54DB31u, 0xB290EA1Au, 0xBF000000u},
        {0xBD0F59AAu, 0xBF584853u, 0x327D5FC0u, 0xBF000000u},
        {0xBC670F32u, 0xBF5B941Au, 0xB2232DC8u, 0xBF000000u},
        {0x3BE8B648u, 0xBF5EBE05u, 0xB2C6F953u, 0xBF000000u},
        {0x3CEA5164u, 0xBF61C598u, 0x32E7F425u, 0xBF000000u},
        {0x3D4E645Au, 0xBF64AA59u, 0xB11A08FAu, 0xBF000000u},
        {0x3D945DFFu, 0xBF676BD8u, 0x32BC3389u, 0xBF000000u},
        {0x3DC210D8u, 0xBF6A09A7u, 0x32EB236Cu, 0xBF000000u},
        {0x3DF043ABu, 0xBF6C835Eu, 0xB2F328D4u, 0xBF000000u},
        {0x3E0F77ADu, 0xBF6ED89Eu, 0x329333DCu, 0xBF000000u},
        {0xBDB1F34Fu, 0xBF710908u, 0xB21ED0DDu, 0xBE800000u},
        {0xBD826B93u, 0xBF731447u, 0xB2C48E11u, 0xBE800000u},
        {0xBD25018Cu, 0xBF74FA0Bu, 0x32939D22u, 0xBE800000u},
        {0xBC88E931u, 0xBF76BA07u, 0xB26D092Cu, 0xBE800000u},
        {0x3BE60685u, 0xBF7853F8u, 0x320DB9E5u, 0xBE800000u},
        {0x3CFD1F65u, 0xBF79C79Du, 0xB2C64E59u, 0xBE800000u},
        {0x3D60E8F8u, 0xBF7B14BEu, 0xB2FF75CBu, 0xBE800000u},
        {0xBD3C4289u, 0xBF7C3B28u, 0x3231D68Bu, 0xBE000000u},
        {0xBCB2041Cu, 0xBF7D3AACu, 0x30F75AE9u, 0xBE000000u},
        {0x3B29B1A9u, 0xBF7E1324u, 0x32F1E603u, 0xBE000000u},
        {0x3CDD0B28u, 0xBF7EC46Du, 0xB1F44949u, 0xBE000000u},
        {0xBC354825u, 0xBF7F4E6Du, 0xB2D01884u, 0xBD800000u},
        {0x3C5C1342u, 0xBF7FB10Fu, 0xB1DE5B5Fu, 0xBD800000u},
        {0x3BDBD541u, 0xBF7FEC43u, 0xB084CD0Du, 0xBD000000u},
        {0x00000000u, 0xBF800000u, 0x00000000u, 0x00000000u},
        {0xBBDBD541u, 0xBF7FEC43u, 0xB084CD0Du, 0x3D000000u},
        {0xBC5C1342u, 0xBF7FB10Fu, 0xB1DE5B5Fu, 0x3D800000u},
        {0x3C354825u, 0xBF7F4E6Du, 0xB2D01884u, 0x3D800000u},
        {0xBCDD0B28u, 0xBF7EC46Du, 0xB1F44949u, 0x3E000000u},
        {0xBB29B1A9u, 0xBF7E1324u, 0x32F1E603u, 0x3E000000u},
        {0x3CB2041Cu, 0xBF7D3AACu, 0x30F75AE9u, 0x3E000000u},
        {0x3D3C4289u, 0xBF7C3B28u, 0x3231D68Bu, 0x3E000000u},
        {0xBD60E8F8u, 0xBF7B14BEu, 0xB2FF75CBu, 0x3E800000u},
        {0xBCFD1F65u, 0xBF79C79Du, 0xB2C64E59u, 0x3E800000u},
        {0xBBE60685u, 0xBF7853F8u, 0x320DB9E5u, 0x3E800000u},
        {0x3C88E931u, 0xBF76BA07u, 0xB26D092Cu, 0x3E800000u},
        {0x3D25018Cu, 0xBF74FA0Bu, 0x32939D22u, 0x3E800000u},
        {0x3D826B93u, 0xBF731447u, 0xB2C48E11u, 0x3E800000u},
        {0x3DB1F34Fu, 0xBF710908u, 0xB21ED0DDu, 0x3E800000u},
        {0xBE0F77ADu, 0xBF6ED89Eu, 0x329333DCu, 0x3F000000u},
        {0xBDF043ABu, 0xBF6C835Eu, 0xB2F328D4u, 0x3F000000u},
        {0xBDC210D8u, 0xBF6A09A7u, 0x32EB236Cu, 0x3F000000u},
        {0xBD945DFFu, 0xBF676BD8u, 0x32BC3389u, 0x3F000000u},
        {0xBD4E645Au, 0xBF64AA59u, 0xB11A08FAu, 0x3F000000u},
        {0xBCEA5164u, 0xBF61C598u, 0x32E7F425u, 0x3F000000u},
        {0xBBE8B648u, 0xBF5EBE05u, 0xB2C6F953u, 0x3F000000u},
        {0x3C670F32u, 0xBF5B941Au, 0xB2232DC8u, 0x3F000000u},
        {0x3D0F59AAu, 0xBF584853u, 0x327D5FC0u, 0x3F000000u},
        {0x3D639D9Du, 0xBF54DB31u, 0xB290EA1Au, 0x3F000000u},
        {0x3D9B4153u, 0xBF514D3Du, 0xB00C4F04u, 0x3F000000u},
        {0x3DC3FDFFu, 0xBF4D9F02u, 0xB27E70E8u, 0x3F000000u},
        {0x3DEBFE8Au, 0xBF49D112u, 0xB2992640u, 0x3F000000u},
        {0x3E099E65u, 0xBF45E403u, 0xB2B15174u, 0x3F000000u},
        {0x3E1CD957u, 0xBF41D870u, 0xB2BFF977u, 0x3F000000u},
        {0x3E2FAD27u, 0xBF3DAEF9u, 0xB19AABECu, 0x3F000000u},
        {0x3E4216EBu, 0xBF396842u, 0x32810007u, 0x3F000000u},
        {0xBE95F61Au, 0xBF3504F3u, 0xB24FE77Au, 0x3F800000u},
        {0xBE8D2F7Du, 0xBF3085BBu, 0x32AE2D32u, 0x3F800000u},
        {0xBE84A20Eu, 0xBF2BEB4Au, 0x32B73136u, 0x3F800000u},
        {0xBE789E3Fu, 0xBF273656u, 0x32038343u, 0x3F800000u},
        {0xBE686FF3u, 0xBF226799u, 0xB22123BBu, 0x3F800000u},
        {0xBE58BBB7u, 0xBF1D7FD1u, 0xB292050Cu, 0x3F800000u},
        {0xBE4983F7u, 0xBF187FC0u, 0x31C7A3F3u, 0x3F800000u},
        {0xBE3ACB0Cu, 0xBF13682Au, 0xB2CDD12Eu, 0x3F800000u},
        {0xBE2C933Bu, 0xBF0E39DAu, 0x324A32E7u, 0x3F800000u},
        {0xBE1EDEB5u, 0xBF08F59Bu, 0x32BE4B4Eu, 0x3F800000u},
        {0xBE11AF97u, 0xBF039C3Du, 0x325BA002u, 0x3F800000u},
        {0xBE0507EAu, 0xBEFC5D27u, 0x3180ECA9u, 0x3F800000u},
        {0xBDF1D344u, 0xBEF15AEAu, 0x31FF2139u, 0x3F800000u},
        {0xBDDAAD38u, 0xBEE63375u, 0x31D9C774u, 0x3F800000u},
        {0xBDC4A143u, 0xBEDAE880u, 0xB21E15CCu, 0x3F800000u},
        {0xBDAFB2CCu, 0xBECF7BCAu, 0xB16A3B63u, 0x3F800000u},
        {0xBD9BE50Cu, 0xBEC3EF15u, 0xB1D5D52Cu, 0x3F800000u},
        {0xBD893B12u, 0xBEB8442Au, 0x32705BA6u, 0x3F800000u},
        {0xBD6F6F7Eu, 0xBEAC7CD4u, 0x32254E02u, 0x3F800000u},
        {0xBD4EBB8Au, 0xBEA09AE5u, 0x323E89A0u, 0x3F800000u},
        {0xBD305F55u, 0xBE94A031u, 0xB26D59F0u, 0x3F800000u},
        {0xBD145F8Cu, 0xBE888E93u, 0xB12C7D9Eu, 0x3F800000u},
        {0xBCF58104u, 0xBE78CFCCu, 0x311BD41Du, 0x3F800000u},
        {0xBCC70C54u, 0xBE605C13u, 0xB1A7E4F6u, 0x3F800000u},
        {0xBC9D6830u, 0xBE47C5C2u, 0x30E5967Du, 0x3F800000u},
        {0xBC71360Bu, 0xBE2F10A2u, 0xB11167F9u, 0x3F800000u},
        {0xBC315502u, 0xBE164083u, 0xB1E8E614u, 0x3F800000u},
        {0xBBF66E3Cu, 0xBDFAB273u, 0x311568CFu, 0x3F800000u},
        {0xBB9DC971u, 0xBDC8BD36u, 0x307592F5u, 0x3F800000u},
        {0xBB319298u, 0xBD96A905u, 0x31531E61u, 0x3F800000u},
        {0xBA9DE1C8u, 0xBD48FB30u, 0x30EF227Fu, 0x3F800000u},
        {0xB99DE7DFu, 0xBCC90AB0u, 0x3005C998u, 0x3F800000u},
    },
    /* > General purpose constants: */
    0x7FFFFFFFu, /* absolute value mask  */
    0x461C4000u, /* threshold for out-of-range values  */
    0x7f800000u, /* +INF  */
    /* High Accuracy version polynomial coefficients:  */
    0xBE2AAAABu, /* S1 = -1.66666666664728165763e-01 */
    0x3C08885Cu, /* S2 = 8.33329173045453069014e-03 */
    0xBF000000u, /* C1 = -5.00000000000000000000e-01 */
    0x3D2AAA7Cu, /* C2 = 4.16638942914469202550e-02 */
    /* > Range reduction PI-based constants: */
    0x40490000u, /* _sPI1 */
    0x3A7DA000u, /* _sPI2 */
    0x34222169u, /* _sPI3 */
    0x00000000u, /* _sPI4 */
    /* > Range reduction PI-based constants if FMA available: */
    0x40490FDBu, /* _sPI1_FMA */
    0xB3BBBD2Eu, /* _sPI2_FMA */
    0x00000000u, /* _sPI3_FMA */
    /* > Polynomial coefficients: */
    0x3F7FF87Cu, /* _sA1 */
    0xBE29E72Fu, /* _sA3 */
    0x3BF84524u, /* _sA5 */
    0x3EA2F983u, /* 1/PI */
    0x4B400000u, /* right-shifter constant */
};               /*sSin_Table*/
#pragma float_control(precise, on)
static __constant int __ssin_ep___ip_h = 0x0517CC1B;
static __constant int __ssin_ep___ip_m = 0x727220A9;
static __constant int __ssin_ep___ip_l = 0x28;
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c5 = {0xbbe61a2du};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c5l = {0xaeba0fbau};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c4 = {0x3da807fcu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c4l = {0xaf614e97u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c3 = {0xbf196889u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c3l = {0xaf584810u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c2 = {0x402335e0u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c2l = {0xb2f6c1feu};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c1 = {0xc0a55de6u};
static __constant union {
  unsigned int w;
  float f;
  int i;
} __ssin_ep___c0 = {0x40490fdau};
static __constant unsigned int __ssin_ep_invpi_tbl[] = {
    0,          0x28BE60DB, 0x9391054A, 0x7F09D5F4,
    0x7D4D3770, 0x36D8A566, 0x4F10E410, 0x7F9458EA};
__attribute__((always_inline)) inline int
__ocl_svml_internal_ssin_ep(float *a, float *pres) {
  int nRet = 0;
  float xin = *a;
  unsigned long IP, IP2;
  long IP_s, IP2_s;
  int ip_low_s;
  unsigned int ip_low;
  int_float x, Rh, Rl, res, scale;
  int mx, sgn_x, ex, ip_h, shift, index, j;
  float Low, R2h, R2l, poly_h, poly_l;
  x.f = xin;
  mx = (x.w & 0x007fffff) | 0x00800000;
  sgn_x = x.w & 0x80000000;
  ex = ((x.w ^ sgn_x) >> 23);
  // redirect large or very small inputs
  if (__builtin_expect(((unsigned)(ex - 0x7f + 12)) > (20 + 12), (0 == 1))) {
    // small input
    if (__builtin_expect((ex < 0x7f - 11), (1 == 1))) {
      *pres = xin;
      return nRet;
    }
    // Inf/NaN
    if (ex == 0xff) {
      nRet = ((x.w << 1) == 0xff000000) ? 1 : nRet;
      x.w |= 0x00400000;
      *pres = x.f;
      return nRet;
    }
    ex = ex - 0x7f - 23;
    index = 1 + (ex >> 5);
    // expon % 32
    j = ex & 0x1f;
    // x/Pi, scaled by 2^(63-j)
    ip_low = (((unsigned int)__ssin_ep_invpi_tbl[index]) * ((unsigned int)mx));
    IP = (((unsigned long)((unsigned int)(__ssin_ep_invpi_tbl[index + 1]))) *
          ((unsigned int)(mx))) +
         (((unsigned long)ip_low) << 32);
    // scaled by 2^(95-j)
    IP2 = (((unsigned long)((unsigned int)(__ssin_ep_invpi_tbl[index + 2]))) *
           ((unsigned int)(mx))) +
          ((((unsigned long)((unsigned int)(__ssin_ep_invpi_tbl[index + 3]))) *
            ((unsigned int)(mx))) >>
           32);
    IP = IP + (IP2 >> 32);
    // scale 2^63
    IP <<= j;
    // shift low part by 32-j, j in [0,31]
    ip_low = (unsigned int)IP2;
    ip_low >>= (31 - j);
    ip_low >>= 1;
    IP |= (unsigned long)ip_low;
  } else // main path
  {
    // products are really unsigned; operands are small enough so that signed
    // MuL works as well x*(23-ex)*(1/Pi)*2^28 p[k] products fit in 31 bits each
    IP_s = (((long)((int)(mx))) * ((int)(__ssin_ep___ip_h)));
    IP = (unsigned long)IP_s;
    IP2_s = (((long)((int)(mx))) * ((int)(__ssin_ep___ip_m)));
    IP2 = (unsigned long)IP2_s;
    // scale (23-ex)*2^(28+32+7)
    ip_low_s = (((int)mx) * ((int)__ssin_ep___ip_l));
    ip_low = (unsigned int)ip_low_s;
    IP2 = (IP2 << 7) + ip_low;
    // (x/Pi)*2^63
    IP <<= (ex - 0x7f + 12);
    // IP3 = IP2 << (37 -0x7f + ex);
    IP2 >>= (27 + 0x7f - ex);
    IP += IP2;
  }
  // return to 32-bit, scale 2^31
  ip_h = IP >> 32;
  // fix sign bit
  sgn_x ^= ((ip_h + 0x40000000) & 0x80000000);
  // reduced argument (signed, high-low), scale 2^32
  ip_h <<= 1;
  Rh.f = (float)ip_h;
  // reduced argument will need to be normalized
  shift = 1 + 30 + 0x7f - ((Rh.w >> 23) & 0xff);
  // correction for shift=0
  shift = (shift >= 1) ? shift : 1;
  // normalize
  IP <<= shift; // IP = (IP << shift) | (IP3 >> (64-shift));
  ip_h = IP >> 32;
  Rh.f = (float)ip_h;
  // adjust scale
  scale.w = (0x7f - 31 - shift) << 23;
  Rh.f = __spirv_ocl_fma(Rh.f, scale.f, 0.0f);
  // (Rh)^2
  R2h = __spirv_ocl_fma(Rh.f, Rh.f, 0.0f);
  poly_h = __ssin_ep___c5.f;
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __ssin_ep___c4.f);
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __ssin_ep___c3.f);
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __ssin_ep___c2.f);
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __ssin_ep___c1.f);
  poly_h =
      __spirv_ocl_fma(poly_h, R2h, __ssin_ep___c0.f);
  res.f = __spirv_ocl_fma(poly_h, Rh.f, 0.0f);
  res.w ^= sgn_x;
  *pres = res.f;
  return nRet;
}
float __ocl_svml_sinf_ep(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    float sAbsX;
    float sSignX;
    float sRangeReductionMask;
    unsigned int iRangeReductionMask;
    unsigned int mRangeReductionMask;
    float sRangeReductionVal;
    float sAbsMask;
    sAbsMask = as_float(__ocl_svml_internal_ssin_ep_data._sAbsMask);
    //        b) Remove sign using AND operation
    sAbsX = as_float((as_uint(va1) & as_uint(sAbsMask)));
    vm = 0;
    {
      float sN;
      float sSign;
      float sR;
      float sR2;
      float sP;
      float sPI1;
      float sPI2;
      float sPI3;
      float sPI4;
      float sSRA1;
      float sA1;
      float sA3;
      float sA5;
      float sA7;
      float sA9;
      float sA11;
      float sA13;
      float sInvPI;
      float sRShifter;
      float sRangeVal;
      sInvPI = as_float(__ocl_svml_internal_ssin_ep_data._sInvPI);
      sRShifter = as_float(__ocl_svml_internal_ssin_ep_data._sRShifter);
      //    c) Getting octant Y by 1/Pi multiplication
      //    d) Add "Right Shifter" value
      sN = __spirv_ocl_fma(sAbsX, sInvPI, sRShifter);
      //    e) Treat obtained value as integer for destination sign setting.
      //       Shift first bit of this value to the last (sign) position
      sSign = as_float(((unsigned int)as_uint(sN) << (31)));
      //    g) Subtract "Right Shifter" value
      sN = (sN - sRShifter);
      sPI1 = as_float(__ocl_svml_internal_ssin_ep_data._sPI1_FMA);
      //    h) Subtract Y*PI from X argument, where PI divided to 4 parts:
      //       X = X - Y*PI1 - Y*PI2 - Y*PI3;
      sR = __spirv_ocl_fma(-(sN), sPI1, sAbsX);
      sPI2 = as_float(__ocl_svml_internal_ssin_ep_data._sPI2_FMA);
      sR = __spirv_ocl_fma(-(sN), sPI2, sR);
      // 2) Polynomial (minimax for sin within [-Pi/2; +Pi/2] interval)
      //    a) Calculate X^2 = X * X
      //    b) Calculate polynomial:
      //       R = X + X * X^2 * (A3 + x^2 * (A5 + ......
      sR2 = (sR * sR);
      //    f) Change destination sign if source sign is negative
      //       using XOR operation.
      sSignX = as_float((~(as_uint(sAbsMask)) & as_uint(va1)));
      sR = as_float((as_uint(sR) ^ as_uint(sSign)));
      sA1 = as_float(__ocl_svml_internal_ssin_ep_data._sA1);
      sSRA1 = (sR * sA1);
      sA3 = as_float(__ocl_svml_internal_ssin_ep_data._sA3);
      sA5 = as_float(__ocl_svml_internal_ssin_ep_data._sA5);
      sP = __spirv_ocl_fma(sA5, sR2, sA3);
      sP = (sP * sR2);
      vr1 = __spirv_ocl_fma(sP, sR, sSRA1);
      // 3) Destination sign setting
      //    a) Set shifted destination sign using XOR operation:
      //       R = XOR( R, S );
      vr1 = as_float((as_uint(vr1) ^ as_uint(sSignX)));
    }
    // Check for large and special values
    sRangeReductionVal =
        as_float(__ocl_svml_internal_ssin_ep_data._sRangeReductionVal);
    sRangeReductionMask = as_float(
        ((unsigned int)(-(signed int)(!(sAbsX <= sRangeReductionVal)))));
    iRangeReductionMask = as_uint(sRangeReductionMask);
    vm = 0;
    vm = iRangeReductionMask;
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_ssin_ep(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
