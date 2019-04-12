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

#ifndef __IA32E_POW_D_G_COUT_CL__
#define __IA32E_POW_D_G_COUT_CL__

__constant _iml_dp_union_t _vmldPowHATab[860] = {
  {{0x00000000u, 0x3FF00000u}},
  {{0x00000000u, 0x3FEF07C0u}},
  {{0x00000000u, 0x3FEE1E00u}},
  {{0x00000000u, 0x3FED41C0u}},
  {{0x00000000u, 0x3FEC71C0u}},
  {{0x00000000u, 0x3FEBAD00u}},
  {{0x00000000u, 0x3FEAF280u}},
  {{0x00000000u, 0x3FEA41C0u}},
  {{0x00000000u, 0x3FE99980u}},
  {{0x00000000u, 0x3FE8F9C0u}},
  {{0x00000000u, 0x3FE86180u}},
  {{0x00000000u, 0x3FE7D040u}},
  {{0x00000000u, 0x3FE745C0u}},
  {{0x00000000u, 0x3FE6C180u}},
  {{0x00000000u, 0x3FE642C0u}},
  {{0x00000000u, 0x3FE5C980u}},
  {{0x00000000u, 0x3FE55540u}},
  {{0x00000000u, 0x3FE4E600u}},
  {{0x00000000u, 0x3FE47B00u}},
  {{0x00000000u, 0x3FE41400u}},
  {{0x00000000u, 0x3FE3B140u}},
  {{0x00000000u, 0x3FE35200u}},
  {{0x00000000u, 0x3FE2F680u}},
  {{0x00000000u, 0x3FE29E40u}},
  {{0x00000000u, 0x3FE24940u}},
  {{0x00000000u, 0x3FE1F700u}},
  {{0x00000000u, 0x3FE1A7C0u}},
  {{0x00000000u, 0x3FE15B00u}},
  {{0x00000000u, 0x3FE11100u}},
  {{0x00000000u, 0x3FE0C980u}},
  {{0x00000000u, 0x3FE08440u}},
  {{0x00000000u, 0x3FE04100u}},
  {{0x00000000u, 0x3FE00000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}},
  {{0xA01F0000u, 0x3FA6BB01u}},
  {{0x7439DB71u, 0x3D3C995Eu}},
  {{0x4FF80000u, 0x3FB66568u}},
  {{0x8DA93FB0u, 0x3D3084F2u}},
  {{0x820EC000u, 0x3FC08CD7u}},
  {{0x11B40207u, 0x3D3543C5u}},
  {{0x64906000u, 0x3FC5C048u}},
  {{0x7E5F3668u, 0x3D28C5D4u}},
  {{0x032BE000u, 0x3FCACF30u}},
  {{0xDEBF9166u, 0x3D2E3733u}},
  {{0xE396E000u, 0x3FCFBC44u}},
  {{0x28665438u, 0x3D47B3F9u}},
  {{0x90D2B000u, 0x3FD243A5u}},
  {{0xE8E9D45Du, 0x3D3C9B75u}},
  {{0xA118D000u, 0x3FD49AD4u}},
  {{0x7302CCA6u, 0x3D45CD37u}},
  {{0x92EF1000u, 0x3FD6E227u}},
  {{0x0E7E9039u, 0x3D314F24u}},
  {{0x9E695000u, 0x3FD91BD1u}},
  {{0xE4F6C667u, 0x3D4DBB3Eu}},
  {{0x273ED000u, 0x3FDB4865u}},
  {{0x099E1F61u, 0x3D4AB54Au}},
  {{0x20231000u, 0x3FDD6799u}},
  {{0x96E87504u, 0x3D18ED50u}},
  {{0x9E747000u, 0x3FDF7A34u}},
  {{0x81D99120u, 0x3D4A6E70u}},
  {{0x50CF0000u, 0x3FE0C116u}},
  {{0xEB1152A5u, 0x3D461752u}},
  {{0x6E8E9800u, 0x3FE1BF42u}},
  {{0x6C055F56u, 0x3D376AFFu}},
  {{0x1C354000u, 0xBFDA8F9Du}},
  {{0x4F4F9854u, 0xBD4604F5u}},
  {{0x8A043000u, 0xBFD8A922u}},
  {{0xCF8DD884u, 0xBD49BC20u}},
  {{0xE7AA4000u, 0xBFD6CB99u}},
  {{0xD5A7002Bu, 0xBD412B5Au}},
  {{0x5D830000u, 0xBFD4F69Fu}},
  {{0xD24BAE46u, 0xBD38F36Du}},
  {{0xF8B57000u, 0xBFD32C15u}},
  {{0xE01D9232u, 0xBD0D6EE2u}},
  {{0xD34FB000u, 0xBFD16935u}},
  {{0x348D84A5u, 0xBD2151C6u}},
  {{0x7E71A000u, 0xBFCF5FAAu}},
  {{0x20C552C2u, 0xBD3D1576u}},
  {{0x1D5BE000u, 0xBFCBFC5Cu}},
  {{0x0E42B538u, 0xBD278490u}},
  {{0x0948A000u, 0xBFC8A9ADu}},
  {{0x64F25A56u, 0xBD4C89BAu}},
  {{0xFF6B0000u, 0xBFC563ADu}},
  {{0x079422C3u, 0xBD4D0837u}},
  {{0x02746000u, 0xBFC22DF3u}},
  {{0xC2505D3Du, 0xBD3048E3u}},
  {{0x92EE4000u, 0xBFBE0894u}},
  {{0xFCD57F87u, 0xBD405589u}},
  {{0x41A08000u, 0xBFB7D493u}},
  {{0xBCF7AA55u, 0xBD4EEEF8u}},
  {{0xCF5A4000u, 0xBFB1BC75u}},
  {{0x139E8397u, 0xBD4D7DB2u}},
  {{0xC1828000u, 0xBFA778FDu}},
  {{0xF2AF5333u, 0xBD34378Au}},
  {{0x317A0000u, 0xBF97427Du}},
  {{0x4B03B094u, 0xBD4700F1u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x3FF00000u}},
  {{0x00000000u, 0x3FEFF000u}},
  {{0x00000000u, 0x3FEFE000u}},
  {{0x00000000u, 0x3FEFD040u}},
  {{0x00000000u, 0x3FEFC080u}},
  {{0x00000000u, 0x3FEFB0C0u}},
  {{0x00000000u, 0x3FEFA100u}},
  {{0x00000000u, 0x3FEF9180u}},
  {{0x00000000u, 0x3FEF8200u}},
  {{0x00000000u, 0x3FEF7280u}},
  {{0x00000000u, 0x3FEF6300u}},
  {{0x00000000u, 0x3FEF53C0u}},
  {{0x00000000u, 0x3FEF4480u}},
  {{0x00000000u, 0x3FEF3540u}},
  {{0x00000000u, 0x3FEF2600u}},
  {{0x00000000u, 0x3FF04540u}},
  {{0x00000000u, 0x3FF04100u}},
  {{0x00000000u, 0x3FF03D00u}},
  {{0x00000000u, 0x3FF038C0u}},
  {{0x00000000u, 0x3FF034C0u}},
  {{0x00000000u, 0x3FF03080u}},
  {{0x00000000u, 0x3FF02C80u}},
  {{0x00000000u, 0x3FF02880u}},
  {{0x00000000u, 0x3FF02440u}},
  {{0x00000000u, 0x3FF02040u}},
  {{0x00000000u, 0x3FF01C40u}},
  {{0x00000000u, 0x3FF01840u}},
  {{0x00000000u, 0x3FF01400u}},
  {{0x00000000u, 0x3FF01000u}},
  {{0x00000000u, 0x3FF00C00u}},
  {{0x00000000u, 0x3FF00800u}},
  {{0x00000000u, 0x3FF00400u}},
  {{0x00000000u, 0x3FF00000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}},
  {{0xA4280000u, 0x3F671B0Eu}},
  {{0x9876EF47u, 0x3D497F68u}},
  {{0xC0680000u, 0x3F7720D9u}},
  {{0x3778C7CCu, 0x3D441AF5u}},
  {{0x8A7A0000u, 0x3F8145C6u}},
  {{0x9AC06488u, 0x3D44BDD1u}},
  {{0x61D20000u, 0x3F86FDF4u}},
  {{0x46D9F6F7u, 0x3D3C9EFCu}},
  {{0x355C0000u, 0x3F8CB8F9u}},
  {{0xADFBB459u, 0x3D3275C9u}},
  {{0xEC350000u, 0x3F913B6Bu}},
  {{0x4FC32ADBu, 0x3D0F25B4u}},
  {{0x7B760000u, 0x3F941016u}},
  {{0xA8ED5317u, 0x3D3F880Cu}},
  {{0x317A0000u, 0x3F96E625u}},
  {{0x645614DBu, 0x3D453F0Eu}},
  {{0x6D010000u, 0x3F99BD99u}},
  {{0x341A2DABu, 0x3D2CD686u}},
  {{0x8ED00000u, 0x3F9C9674u}},
  {{0x66D10B04u, 0x3D4EF88Du}},
  {{0xD4200000u, 0x3F9F64EDu}},
  {{0x828828DAu, 0x3D4511C3u}},
  {{0x97920000u, 0x3FA11A62u}},
  {{0xD7D436D6u, 0x3D4D925Cu}},
  {{0xFAD70000u, 0x3FA282FDu}},
  {{0xA58B8D6Eu, 0x3D49EEE0u}},
  {{0xBFC20000u, 0x3FA3EC49u}},
  {{0x2E0E0086u, 0x3D4DEAC3u}},
  {{0x31230000u, 0xBF98C493u}},
  {{0x5EFCABFAu, 0xBD49AD07u}},
  {{0x317A0000u, 0xBF97427Du}},
  {{0x4B03B094u, 0xBD4700F1u}},
  {{0xEE910000u, 0xBF95D6C0u}},
  {{0x535202A3u, 0xBD4A5115u}},
  {{0xADB60000u, 0xBF9453E6u}},
  {{0xE102F731u, 0xBD415A44u}},
  {{0x57080000u, 0xBF92E771u}},
  {{0x5EE9AD86u, 0xBD4C7ED8u}},
  {{0x0D100000u, 0xBF9163D2u}},
  {{0x664FE33Fu, 0xBD46E8B9u}},
  {{0xCBCC0000u, 0xBF8FED45u}},
  {{0x43464056u, 0xBD37F339u}},
  {{0xC5EA0000u, 0xBF8D1232u}},
  {{0xB0BDC8DFu, 0xBD17CF34u}},
  {{0x28680000u, 0xBF8A08A8u}},
  {{0xF02B9CCFu, 0xBD35A529u}},
  {{0x4CF00000u, 0xBF872C1Fu}},
  {{0x580FE573u, 0xBD2B4934u}},
  {{0xA6F20000u, 0xBF844EE0u}},
  {{0xFF314317u, 0xBD24C8CBu}},
  {{0xDC1C0000u, 0xBF8170EBu}},
  {{0x2CC5232Fu, 0xBD447DB0u}},
  {{0x97D40000u, 0xBF7CC89Fu}},
  {{0x90330E7Bu, 0xBD43AC9Cu}},
  {{0x6D780000u, 0xBF7709C4u}},
  {{0x56CDE925u, 0xBD4563BAu}},
  {{0xCCF40000u, 0xBF71497Au}},
  {{0xDDD3E770u, 0xBD4F08E7u}},
  {{0xFF080000u, 0xBF670F83u}},
  {{0x31D4676Du, 0xBD33AB26u}},
  {{0x37400000u, 0xBF571265u}},
  {{0xFD4FCA1Du, 0xBD2FA2A0u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x40000000u, 0x3FF71547u}},
  {{0x00000000u, 0x3FF71519u}},
  {{0x00000000u, 0x3FF714EBu}},
  {{0xC0000000u, 0x3FF714BCu}},
  {{0x80000000u, 0x3FF7148Eu}},
  {{0x80000000u, 0x3FF71460u}},
  {{0x40000000u, 0x3FF71432u}},
  {{0x40000000u, 0x3FF71404u}},
  {{0x00000000u, 0x3FF713D6u}},
  {{0xC0000000u, 0x3FF713A7u}},
  {{0xC0000000u, 0x3FF71379u}},
  {{0x80000000u, 0x3FF7134Bu}},
  {{0x80000000u, 0x3FF7131Du}},
  {{0x40000000u, 0x3FF712EFu}},
  {{0x40000000u, 0x3FF712C1u}},
  {{0x00000000u, 0x3FF71293u}},
  {{0x00000000u, 0x3FF71265u}},
  {{0xC0000000u, 0x3FF71236u}},
  {{0xC0000000u, 0x3FF71208u}},
  {{0x80000000u, 0x3FF711DAu}},
  {{0x80000000u, 0x3FF711ACu}},
  {{0x80000000u, 0x3FF7117Eu}},
  {{0x40000000u, 0x3FF71150u}},
  {{0x40000000u, 0x3FF71122u}},
  {{0x00000000u, 0x3FF710F4u}},
  {{0x00000000u, 0x3FF710C6u}},
  {{0x00000000u, 0x3FF71098u}},
  {{0xC0000000u, 0x3FF71069u}},
  {{0xC0000000u, 0x3FF7103Bu}},
  {{0x80000000u, 0x3FF7100Du}},
  {{0x80000000u, 0x3FF70FDFu}},
  {{0x80000000u, 0x3FF70FB1u}},
  {{0x40000000u, 0x3FF70F83u}},
  {{0x40000000u, 0x3FF70F55u}},
  {{0x40000000u, 0x3FF70F27u}},
  {{0x40000000u, 0x3FF70EF9u}},
  {{0x00000000u, 0x3FF70ECBu}},
  {{0x00000000u, 0x3FF70E9Du}},
  {{0x00000000u, 0x3FF70E6Fu}},
  {{0x00000000u, 0x3FF70E41u}},
  {{0xC0000000u, 0x3FF70E12u}},
  {{0xC0000000u, 0x3FF70DE4u}},
  {{0xC0000000u, 0x3FF70DB6u}},
  {{0xC0000000u, 0x3FF70D88u}},
  {{0xC0000000u, 0x3FF70D5Au}},
  {{0x80000000u, 0x3FF70D2Cu}},
  {{0x80000000u, 0x3FF70CFEu}},
  {{0x80000000u, 0x3FF70CD0u}},
  {{0x80000000u, 0x3FF70CA2u}},
  {{0x80000000u, 0x3FF70C74u}},
  {{0x80000000u, 0x3FF70C46u}},
  {{0x80000000u, 0x3FF70C18u}},
  {{0x80000000u, 0x3FF70BEAu}},
  {{0x80000000u, 0x3FF70BBCu}},
  {{0x40000000u, 0x3FF70B8Eu}},
  {{0x40000000u, 0x3FF70B60u}},
  {{0x40000000u, 0x3FF70B32u}},
  {{0x40000000u, 0x3FF70B04u}},
  {{0x40000000u, 0x3FF70AD6u}},
  {{0x40000000u, 0x3FF70AA8u}},
  {{0x40000000u, 0x3FF70A7Au}},
  {{0x80000000u, 0x3FF71B53u}},
  {{0x40000000u, 0x3FF71B3Cu}},
  {{0x40000000u, 0x3FF71B25u}},
  {{0x00000000u, 0x3FF71B0Eu}},
  {{0x00000000u, 0x3FF71AF7u}},
  {{0xC0000000u, 0x3FF71ADFu}},
  {{0xC0000000u, 0x3FF71AC8u}},
  {{0x80000000u, 0x3FF71AB1u}},
  {{0x80000000u, 0x3FF71A9Au}},
  {{0x40000000u, 0x3FF71A83u}},
  {{0x40000000u, 0x3FF71A6Cu}},
  {{0x00000000u, 0x3FF71A55u}},
  {{0x00000000u, 0x3FF71A3Eu}},
  {{0xC0000000u, 0x3FF71A26u}},
  {{0xC0000000u, 0x3FF71A0Fu}},
  {{0x80000000u, 0x3FF719F8u}},
  {{0x80000000u, 0x3FF719E1u}},
  {{0x40000000u, 0x3FF719CAu}},
  {{0x40000000u, 0x3FF719B3u}},
  {{0x00000000u, 0x3FF7199Cu}},
  {{0x00000000u, 0x3FF71985u}},
  {{0xC0000000u, 0x3FF7196Du}},
  {{0xC0000000u, 0x3FF71956u}},
  {{0x80000000u, 0x3FF7193Fu}},
  {{0x80000000u, 0x3FF71928u}},
  {{0x40000000u, 0x3FF71911u}},
  {{0x40000000u, 0x3FF718FAu}},
  {{0x40000000u, 0x3FF718E3u}},
  {{0x00000000u, 0x3FF718CCu}},
  {{0x00000000u, 0x3FF718B5u}},
  {{0xC0000000u, 0x3FF7189Du}},
  {{0xC0000000u, 0x3FF71886u}},
  {{0x80000000u, 0x3FF7186Fu}},
  {{0x80000000u, 0x3FF71858u}},
  {{0x80000000u, 0x3FF71841u}},
  {{0x40000000u, 0x3FF7182Au}},
  {{0x40000000u, 0x3FF71813u}},
  {{0x00000000u, 0x3FF717FCu}},
  {{0x00000000u, 0x3FF717E5u}},
  {{0xC0000000u, 0x3FF717CDu}},
  {{0xC0000000u, 0x3FF717B6u}},
  {{0xC0000000u, 0x3FF7179Fu}},
  {{0x80000000u, 0x3FF71788u}},
  {{0x80000000u, 0x3FF71771u}},
  {{0x40000000u, 0x3FF7175Au}},
  {{0x40000000u, 0x3FF71743u}},
  {{0x40000000u, 0x3FF7172Cu}},
  {{0x00000000u, 0x3FF71715u}},
  {{0x00000000u, 0x3FF716FEu}},
  {{0xC0000000u, 0x3FF716E6u}},
  {{0xC0000000u, 0x3FF716CFu}},
  {{0xC0000000u, 0x3FF716B8u}},
  {{0x80000000u, 0x3FF716A1u}},
  {{0x80000000u, 0x3FF7168Au}},
  {{0x80000000u, 0x3FF71673u}},
  {{0x40000000u, 0x3FF7165Cu}},
  {{0x40000000u, 0x3FF71645u}},
  {{0x00000000u, 0x3FF7162Eu}},
  {{0x00000000u, 0x3FF71617u}},
  {{0x00000000u, 0x3FF71600u}},
  {{0xC0000000u, 0x3FF715E8u}},
  {{0xC0000000u, 0x3FF715D1u}},
  {{0xC0000000u, 0x3FF715BAu}},
  {{0x80000000u, 0x3FF715A3u}},
  {{0x80000000u, 0x3FF7158Cu}},
  {{0x80000000u, 0x3FF71575u}},
  {{0x40000000u, 0x3FF7155Eu}},
  {{0x40000000u, 0x3FF71547u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x50000000u, 0x3F072017u}},
  {{0x641F4F36u, 0x3D099925u}},
  {{0x3B000000u, 0x3F17102Eu}},
  {{0xC162D124u, 0x3D120082u}},
  {{0x0C800000u, 0x3F215034u}},
  {{0x09125FF4u, 0x3D441EE7u}},
  {{0x92000000u, 0x3F27185Cu}},
  {{0x8BABF46Fu, 0x3CEECD1Cu}},
  {{0x5D000000u, 0x3F2CD890u}},
  {{0xF0BBF0CFu, 0x3D3BEE60u}},
  {{0xFF400000u, 0x3F315067u}},
  {{0x8A9648DDu, 0x3D3FDBDFu}},
  {{0x63400000u, 0x3F34308Du}},
  {{0xAEE1B670u, 0x3D3681CBu}},
  {{0xC2800000u, 0x3F3714B8u}},
  {{0xBF3D153Eu, 0x3D4102E6u}},
  {{0xED400000u, 0x3F39F8E9u}},
  {{0x33794ABCu, 0x3D365EF2u}},
  {{0x93400000u, 0x3F3CD920u}},
  {{0x3D384508u, 0x3D4FDF0Bu}},
  {{0x4D400000u, 0x3F3FBD5Du}},
  {{0x782C348Au, 0x3D19A166u}},
  {{0xB9400000u, 0x3F414ECFu}},
  {{0x39DDE07Fu, 0x3D48ABA0u}},
  {{0xDDE00000u, 0x3F42C0F3u}},
  {{0x9D5FD823u, 0x3D435F8Au}},
  {{0xB0600000u, 0x3F44311Au}},
  {{0x12458BECu, 0x3D47963Au}},
  {{0x9CE00000u, 0x3F45A344u}},
  {{0x2C8C13FDu, 0x3D47E151u}},
  {{0x2F600000u, 0x3F471371u}},
  {{0xCB0BD4FAu, 0x3D48D0BEu}},
  {{0xE4000000u, 0x3F4885A0u}},
  {{0xDCE1A474u, 0x3D39DB21u}},
  {{0x36A00000u, 0x3F49F5D3u}},
  {{0x55660916u, 0x3D489166u}},
  {{0xB3800000u, 0x3F4B6808u}},
  {{0xC862A7D0u, 0x3D3D7958u}},
  {{0xC6800000u, 0x3F4CD840u}},
  {{0xD43B70F4u, 0x3D4312BAu}},
  {{0xB7C00000u, 0x3F4E487Bu}},
  {{0x975E2C41u, 0x3D46363Fu}},
  {{0xDF600000u, 0x3F4FBAB9u}},
  {{0x850FC6C3u, 0x3D224E63u}},
  {{0x48A00000u, 0x3F50957Du}},
  {{0x17A21AA6u, 0x3D256896u}},
  {{0x40C00000u, 0x3F514E9Fu}},
  {{0xA4582824u, 0x3D32AE5Fu}},
  {{0x7A100000u, 0x3F5206C2u}},
  {{0x84B0FD57u, 0x3D45C3F8u}},
  {{0x22A00000u, 0x3F52BEE7u}},
  {{0x4AAD1649u, 0x3D4A525Cu}},
  {{0x70800000u, 0x3F53780Du}},
  {{0x275071DEu, 0x3D1DEB18u}},
  {{0xF9900000u, 0x3F543034u}},
  {{0xD5D75FB6u, 0x3D48EAE4u}},
  {{0x2C000000u, 0x3F54E95Eu}},
  {{0xC1291B85u, 0x3D3728C5u}},
  {{0x95B00000u, 0x3F55A188u}},
  {{0xABECF0D7u, 0x3D4F699Au}},
  {{0x6ED00000u, 0x3F5659B4u}},
  {{0x8F2D1FA9u, 0x3D22ED25u}},
  {{0xF7400000u, 0x3F5712E1u}},
  {{0x8B30E580u, 0x3D3445EFu}},
  {{0xB1100000u, 0x3F57CB10u}},
  {{0xDC75FAC6u, 0x3D431D52u}},
  {{0xDA500000u, 0x3F588340u}},
  {{0xA135BD69u, 0x3D3FFBCBu}},
  {{0x73000000u, 0x3F593B72u}},
  {{0x2D63E5DBu, 0x3D387100u}},
  {{0xC3200000u, 0x3F59F4A5u}},
  {{0x1C0BB062u, 0x3D45AFDDu}},
  {{0x3CC00000u, 0x3F5AACDAu}},
  {{0xDCD040AEu, 0x3D41914Fu}},
  {{0x25E00000u, 0x3F5B6510u}},
  {{0xDB245B1Fu, 0x3D3E7387u}},
  {{0x7E800000u, 0x3F5C1D47u}},
  {{0x593D6B3Fu, 0x3D4A1E1Bu}},
  {{0x96C00000u, 0x3F5CD680u}},
  {{0xCC31FC26u, 0x3CEA7D5Cu}},
  {{0xD0800000u, 0x3F5D8EBAu}},
  {{0x762A3069u, 0x3D2BEFBCu}},
  {{0x79D00000u, 0x3F5E46F6u}},
  {{0x952BE02Cu, 0x3D4D39FFu}},
  {{0x92D00000u, 0x3F5EFF33u}},
  {{0x3B1A1CB4u, 0x3D3D3437u}},
  {{0x1B700000u, 0x3F5FB772u}},
  {{0x57218470u, 0x3D2A49F2u}},
  {{0x36D80000u, 0x3F603859u}},
  {{0x602BA3B5u, 0x3D43ADB6u}},
  {{0xEBD00000u, 0x3F609479u}},
  {{0x0591EE13u, 0x3D4A6056u}},
  {{0x58A80000u, 0x3F60F09Bu}},
  {{0x4FAFF44Bu, 0x3D343EECu}},
  {{0x7D580000u, 0x3F614CBDu}},
  {{0x130DF139u, 0x3D3CC4C6u}},
  {{0x59E80000u, 0x3F61A8E0u}},
  {{0x9380107Fu, 0x3D42A0ADu}},
  {{0xEE600000u, 0x3F620503u}},
  {{0xB9035A2Au, 0x3D12915Au}},
  {{0x3AB80000u, 0x3F626128u}},
  {{0x20CAACA3u, 0x3D31DDEDu}},
  {{0x3EF80000u, 0x3F62BD4Du}},
  {{0x281079C7u, 0x3D41EF6Eu}},
  {{0xFB280000u, 0x3F631972u}},
  {{0x6B98497Fu, 0x3D31986Bu}},
  {{0xA5400000u, 0x3F637619u}},
  {{0x91F2B430u, 0x3D4C3697u}},
  {{0xD2500000u, 0x3F63D240u}},
  {{0xCE1C0762u, 0x3D40ECA7u}},
  {{0xB7500000u, 0x3F642E68u}},
  {{0x073B1E2Eu, 0x3D45FA86u}},
  {{0x54480000u, 0x3F648A91u}},
  {{0x05A622FDu, 0x3D46D9E9u}},
  {{0xA9380000u, 0x3F64E6BAu}},
  {{0x4B55A072u, 0x3D4F04CCu}},
  {{0xB6280000u, 0x3F6542E4u}},
  {{0x16094E0Eu, 0x3D49F570u}},
  {{0x7B180000u, 0x3F659F0Fu}},
  {{0x616CF239u, 0x3D432659u}},
  {{0x69800000u, 0xBF582DD5u}},
  {{0x3ED708F7u, 0xBD3AB541u}},
  {{0x91900000u, 0xBF57D0EDu}},
  {{0x7B358E46u, 0xBD4135C6u}},
  {{0x1B300000u, 0xBF577505u}},
  {{0xF6ED6FDEu, 0xBCFEF3C0u}},
  {{0x89400000u, 0xBF57181Cu}},
  {{0x5AF7807Fu, 0xBD46A455u}},
  {{0x5AE00000u, 0xBF56BC33u}},
  {{0x919C892Du, 0xBD452637u}},
  {{0x0F000000u, 0xBF565F4Au}},
  {{0x806E8ED0u, 0xBD46D380u}},
  {{0x28A00000u, 0xBF560360u}},
  {{0xFEDC7D6Bu, 0xBD4E545Eu}},
  {{0x22D00000u, 0xBF55A676u}},
  {{0xFDAB27BCu, 0xBD28C45Au}},
  {{0x84700000u, 0xBF554A8Bu}},
  {{0x94F1D540u, 0xBD40EFC0u}},
  {{0xC4900000u, 0xBF54EDA0u}},
  {{0x6E5D7E55u, 0xBD492AA1u}},
  {{0x6E400000u, 0xBF5491B5u}},
  {{0xAE3A4995u, 0xBD065C31u}},
  {{0xF4600000u, 0xBF5434C9u}},
  {{0x90B30C51u, 0xBD20B591u}},
  {{0xE6000000u, 0xBF53D8DDu}},
  {{0x5C64292Cu, 0xBD208E1Fu}},
  {{0xB2100000u, 0xBF537BF1u}},
  {{0xF606F2E6u, 0xBD4BA65Du}},
  {{0xEBB00000u, 0xBF532004u}},
  {{0x1A593CA0u, 0xBD3B2BD5u}},
  {{0xFDC00000u, 0xBF52C317u}},
  {{0xF5360F7Du, 0xBD440246u}},
  {{0x7F500000u, 0xBF52672Au}},
  {{0x7BBFCA47u, 0xBD422981u}},
  {{0xD7600000u, 0xBF520A3Cu}},
  {{0xCF30F123u, 0xBD0AD92Au}},
  {{0xA0E00000u, 0xBF51AE4Eu}},
  {{0x693830ADu, 0xBD292A75u}},
  {{0x3ED00000u, 0xBF515160u}},
  {{0xE0391426u, 0xBD49146Eu}},
  {{0x50400000u, 0xBF50F571u}},
  {{0x57002344u, 0xBD4E6549u}},
  {{0x34300000u, 0xBF509882u}},
  {{0x8F311F09u, 0xBD3D4587u}},
  {{0x8D900000u, 0xBF503C92u}},
  {{0x28FE4EA6u, 0xBD3DCA96u}},
  {{0x6EC00000u, 0xBF4FBF45u}},
  {{0x0F0A4C7Fu, 0xBD46C434u}},
  {{0xB1600000u, 0xBF4F0764u}},
  {{0x4F8F13F5u, 0xBD386C45u}},
  {{0x90E00000u, 0xBF4E4D83u}},
  {{0x4F099D6Au, 0xBD279073u}},
  {{0x63400000u, 0xBF4D95A1u}},
  {{0x2BF804ACu, 0xBD358615u}},
  {{0x7E800000u, 0xBF4CDDBEu}},
  {{0xEDA75E42u, 0xBD43F2DCu}},
  {{0x30A00000u, 0xBF4C23DBu}},
  {{0xD23862D0u, 0xBD4EF6F7u}},
  {{0xDBC00000u, 0xBF4B6BF6u}},
  {{0x2300F78Cu, 0xBD3DE7D1u}},
  {{0x19A00000u, 0xBF4AB212u}},
  {{0xBB645928u, 0xBD4D3C99u}},
  {{0x54800000u, 0xBF49FA2Cu}},
  {{0xADF18185u, 0xBD3C87D4u}},
  {{0x1E200000u, 0xBF494046u}},
  {{0x1A0618B0u, 0xBD49FE5Au}},
  {{0xE8C00000u, 0xBF48885Eu}},
  {{0xAAEEF6A6u, 0xBD2934A5u}},
  {{0xFC200000u, 0xBF47D076u}},
  {{0xECBB5462u, 0xBD39CFC0u}},
  {{0x98600000u, 0xBF47168Eu}},
  {{0xBFA1C16Bu, 0xBD36F34Eu}},
  {{0x3B800000u, 0xBF465EA5u}},
  {{0x9088CA01u, 0xBD22ED1Cu}},
  {{0x63600000u, 0xBF45A4BBu}},
  {{0x2D6DE9B5u, 0xBD423320u}},
  {{0x96200000u, 0xBF44ECD0u}},
  {{0x10D84808u, 0xBD485871u}},
  {{0x49C00000u, 0xBF4432E5u}},
  {{0x5992900Au, 0xBD3DC614u}},
  {{0x0C200000u, 0xBF437AF9u}},
  {{0xF611F4F2u, 0xBD4C28B1u}},
  {{0x17600000u, 0xBF42C30Cu}},
  {{0x12496DA4u, 0xBD49B472u}},
  {{0x9D800000u, 0xBF42091Eu}},
  {{0xB07B63E5u, 0xBD22545Fu}},
  {{0x38600000u, 0xBF415130u}},
  {{0x6B4DAA19u, 0xBCF3FC74u}},
  {{0x4A000000u, 0xBF409741u}},
  {{0x9F7943ACu, 0xBD2819ADu}},
  {{0xE8C00000u, 0xBF3FBEA2u}},
  {{0x05018F01u, 0xBD4ACC92u}},
  {{0xCF400000u, 0xBF3E4EC1u}},
  {{0x5DECAD9Cu, 0xBD48B028u}},
  {{0x97400000u, 0xBF3CDADFu}},
  {{0x63115207u, 0xBD4CA2DDu}},
  {{0x9D000000u, 0xBF3B6AFBu}},
  {{0xA7FC9363u, 0xBD354003u}},
  {{0x7C000000u, 0xBF39F716u}},
  {{0x407A7831u, 0xBD4A8AE0u}},
  {{0xA0C00000u, 0xBF38872Fu}},
  {{0x91A8939Du, 0xBD42DD8Bu}},
  {{0x57000000u, 0xBF371747u}},
  {{0x2F243D55u, 0xBD3A4925u}},
  {{0xDAC00000u, 0xBF35A35Du}},
  {{0x22A2581Bu, 0xBD2741DFu}},
  {{0xB0000000u, 0xBF343372u}},
  {{0x3C22E0D2u, 0xBD0CAB75u}},
  {{0x16800000u, 0xBF32C386u}},
  {{0x5FC1D4DDu, 0xBD4B6E11u}},
  {{0x3EC00000u, 0xBF314F98u}},
  {{0x97121E28u, 0xBD3F929Au}},
  {{0x89000000u, 0xBF2FBF51u}},
  {{0x806FDBDAu, 0xBD13EA1Eu}},
  {{0x07000000u, 0xBF2CD770u}},
  {{0x51E95ECBu, 0xBD370EB1u}},
  {{0x50000000u, 0xBF29F78Bu}},
  {{0xFF247507u, 0xBD418759u}},
  {{0xBC000000u, 0xBF2717A3u}},
  {{0xC6C6F140u, 0xBD3C0D6Bu}},
  {{0x82800000u, 0xBF242FB9u}},
  {{0xB8D3C162u, 0xBD4CF2EFu}},
  {{0x2C800000u, 0xBF214FCCu}},
  {{0x12CFE97Eu, 0xBD206CE3u}},
  {{0xF1000000u, 0xBF1CDFB7u}},
  {{0x1056AF68u, 0xBD4D7734u}},
  {{0x10000000u, 0xBF170FD2u}},
  {{0x29329AEFu, 0xBD24CAA4u}},
  {{0x24000000u, 0xBF114FE6u}},
  {{0x06B71311u, 0xBCF284C3u}},
  {{0xFA000000u, 0xBF071FE8u}},
  {{0xDD98569Au, 0xBD33BB0Cu}},
  {{0xAC000000u, 0xBEF6FFF4u}},
  {{0x0B7407C7u, 0xBD4D54D7u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x3FF00000u}},
  {{0x00000000u, 0x00000000u}},
  {{0xA9FB3335u, 0x3FF0163Du}},
  {{0x9AB8CDB7u, 0x3C9B6129u}},
  {{0x3E778061u, 0x3FF02C9Au}},
  {{0x535B085Du, 0xBC719083u}},
  {{0xE86E7F85u, 0x3FF04315u}},
  {{0x1977C96Eu, 0xBC90A31Cu}},
  {{0xD3158574u, 0x3FF059B0u}},
  {{0xA475B465u, 0x3C8D73E2u}},
  {{0x29DDF6DEu, 0x3FF0706Bu}},
  {{0xE2B13C26u, 0xBC8C91DFu}},
  {{0x18759BC8u, 0x3FF08745u}},
  {{0x4BB284FFu, 0x3C6186BEu}},
  {{0xCAC6F383u, 0x3FF09E3Eu}},
  {{0x18316135u, 0x3C914878u}},
  {{0x6CF9890Fu, 0x3FF0B558u}},
  {{0x4ADC610Au, 0x3C98A62Eu}},
  {{0x2B7247F7u, 0x3FF0CC92u}},
  {{0x16E24F71u, 0x3C901EDCu}},
  {{0x32D3D1A2u, 0x3FF0E3ECu}},
  {{0x27C57B52u, 0x3C403A17u}},
  {{0xAFFED31Bu, 0x3FF0FB66u}},
  {{0xC44EBD7Bu, 0xBC6B9BEDu}},
  {{0xD0125B51u, 0x3FF11301u}},
  {{0x39449B39u, 0xBC96C510u}},
  {{0xC06C31CCu, 0x3FF12ABDu}},
  {{0xB36CA5C7u, 0xBC51B514u}},
  {{0xAEA92DE0u, 0x3FF1429Au}},
  {{0x9AF1369Eu, 0xBC932FBFu}},
  {{0xC8A58E51u, 0x3FF15A98u}},
  {{0xB9EEAB09u, 0x3C82406Au}},
  {{0x3C7D517Bu, 0x3FF172B8u}},
  {{0xB9D78A75u, 0xBC819041u}},
  {{0x388C8DEAu, 0x3FF18AF9u}},
  {{0xD1970F6Bu, 0xBC911023u}},
  {{0xEB6FCB75u, 0x3FF1A35Bu}},
  {{0x7B4968E4u, 0x3C8E5B4Cu}},
  {{0x84045CD4u, 0x3FF1BBE0u}},
  {{0x352EF607u, 0xBC995386u}},
  {{0x3168B9AAu, 0x3FF1D487u}},
  {{0x00A2643Cu, 0x3C9E016Eu}},
  {{0x22FCD91Du, 0x3FF1ED50u}},
  {{0x027BB78Bu, 0xBC91DF98u}},
  {{0x88628CD6u, 0x3FF2063Bu}},
  {{0x814A8494u, 0x3C8DC775u}},
  {{0x917DDC96u, 0x3FF21F49u}},
  {{0x9494A5EDu, 0x3C82A97Eu}},
  {{0x6E756238u, 0x3FF2387Au}},
  {{0xB6C70572u, 0x3C99B07Eu}},
  {{0x4FB2A63Fu, 0x3FF251CEu}},
  {{0xBEF4F4A4u, 0x3C8AC155u}},
  {{0x65E27CDDu, 0x3FF26B45u}},
  {{0x9940E9D9u, 0x3C82BD33u}},
  {{0xE1F56381u, 0x3FF284DFu}},
  {{0x8C3F0D7Du, 0xBC9A4C3Au}},
  {{0xF51FDEE1u, 0x3FF29E9Du}},
  {{0xAFAD1255u, 0x3C8612E8u}},
  {{0xD0DAD990u, 0x3FF2B87Fu}},
  {{0xD6381AA3u, 0xBC410ADCu}},
  {{0xA6E4030Bu, 0x3FF2D285u}},
  {{0x54DB41D4u, 0x3C900247u}},
  {{0xA93E2F56u, 0x3FF2ECAFu}},
  {{0x45D52383u, 0x3C71CA0Fu}},
  {{0x0A31B715u, 0x3FF306FEu}},
  {{0xD23182E4u, 0x3C86F46Au}},
  {{0xFC4CD831u, 0x3FF32170u}},
  {{0x8E18047Cu, 0x3C8A9CE7u}},
  {{0xB26416FFu, 0x3FF33C08u}},
  {{0x843659A5u, 0x3C932721u}},
  {{0x5F929FF1u, 0x3FF356C5u}},
  {{0x5C4E4628u, 0xBC8B5CEEu}},
  {{0x373AA9CBu, 0x3FF371A7u}},
  {{0xBF42EAE1u, 0xBC963AEAu}},
  {{0x6D05D866u, 0x3FF38CAEu}},
  {{0x3C9904BCu, 0xBC9E958Du}},
  {{0x34E59FF7u, 0x3FF3A7DBu}},
  {{0xD661F5E2u, 0xBC75E436u}},
  {{0xC313A8E5u, 0x3FF3C32Du}},
  {{0x375D29C3u, 0xBC9EFFF8u}},
  {{0x4C123422u, 0x3FF3DEA6u}},
  {{0x11F09EBBu, 0x3C8ADA09u}},
  {{0x04AC801Cu, 0x3FF3FA45u}},
  {{0xF956F9F3u, 0xBC97D023u}},
  {{0x21F72E2Au, 0x3FF4160Au}},
  {{0x1C309278u, 0xBC5EF369u}},
  {{0xD950A897u, 0x3FF431F5u}},
  {{0xE35F7998u, 0xBC81C7DDu}},
  {{0x6061892Du, 0x3FF44E08u}},
  {{0x04EF80CFu, 0x3C489B7Au}},
  {{0xED1D0057u, 0x3FF46A41u}},
  {{0xD1648A76u, 0x3C9C944Bu}},
  {{0xB5C13CD0u, 0x3FF486A2u}},
  {{0xB69062F0u, 0x3C73C1A3u}},
  {{0xF0D7D3DEu, 0x3FF4A32Au}},
  {{0xF3D1BE56u, 0x3C99CB62u}},
  {{0xD5362A27u, 0x3FF4BFDAu}},
  {{0xAFEC42E2u, 0x3C7D4397u}},
  {{0x99FDDD0Du, 0x3FF4DCB2u}},
  {{0xBC6A7833u, 0x3C98ECDBu}},
  {{0x769D2CA7u, 0x3FF4F9B2u}},
  {{0xD25957E3u, 0xBC94B309u}},
  {{0xA2CF6642u, 0x3FF516DAu}},
  {{0x69BD93EEu, 0xBC8F7685u}},
  {{0x569D4F82u, 0x3FF5342Bu}},
  {{0x1DB13CACu, 0xBC807ABEu}},
  {{0xCA5D920Fu, 0x3FF551A4u}},
  {{0xEFEDE59Au, 0xBC8D689Cu}},
  {{0x36B527DAu, 0x3FF56F47u}},
  {{0x011D93ACu, 0x3C99BB2Cu}},
  {{0xD497C7FDu, 0x3FF58D12u}},
  {{0x5B9A1DE7u, 0x3C8295E1u}},
  {{0xDD485429u, 0x3FF5AB07u}},
  {{0x054647ACu, 0x3C96324Cu}},
  {{0x8A5946B7u, 0x3FF5C926u}},
  {{0x816986A2u, 0x3C3C4B1Bu}},
  {{0x15AD2148u, 0x3FF5E76Fu}},
  {{0x3080E65Du, 0x3C9BA6F9u}},
  {{0xB976DC09u, 0x3FF605E1u}},
  {{0x9B56DE47u, 0xBC93E242u}},
  {{0xB03A5585u, 0x3FF6247Eu}},
  {{0x7E40B496u, 0xBC9383C1u}},
  {{0x34CCC320u, 0x3FF64346u}},
  {{0x759D8932u, 0xBC8C483Cu}},
  {{0x82552225u, 0x3FF66238u}},
  {{0x87591C33u, 0xBC9BB609u}},
  {{0xD44CA973u, 0x3FF68155u}},
  {{0x44F73E64u, 0x3C6038AEu}},
  {{0x667F3BCDu, 0x3FF6A09Eu}},
  {{0x13B26455u, 0xBC9BDD34u}},
  {{0x750BDABFu, 0x3FF6C012u}},
  {{0x67FF0B0Cu, 0xBC728956u}},
  {{0x3C651A2Fu, 0x3FF6DFB2u}},
  {{0x683C88AAu, 0xBC6BBE3Au}},
  {{0xF9519484u, 0x3FF6FF7Du}},
  {{0x25860EF6u, 0xBC883C0Fu}},
  {{0xE8EC5F74u, 0x3FF71F75u}},
  {{0x86887A99u, 0xBC816E47u}},
  {{0x48A58174u, 0x3FF73F9Au}},
  {{0x6C65D53Bu, 0xBC90A8D9u}},
  {{0x564267C9u, 0x3FF75FEBu}},
  {{0x57316DD3u, 0xBC902459u}},
  {{0x4FDE5D3Fu, 0x3FF78069u}},
  {{0x0A02162Cu, 0x3C9866B8u}},
  {{0x73EB0187u, 0x3FF7A114u}},
  {{0xEE04992Fu, 0xBC841577u}},
  {{0x0130C132u, 0x3FF7C1EDu}},
  {{0xD1164DD5u, 0x3C9F124Cu}},
  {{0x36CF4E62u, 0x3FF7E2F3u}},
  {{0xBA15797Eu, 0x3C705D02u}},
  {{0x543E1A12u, 0x3FF80427u}},
  {{0x626D972Au, 0xBC927C86u}},
  {{0x994CCE13u, 0x3FF82589u}},
  {{0xD41532D7u, 0xBC9D4C1Du}},
  {{0x4623C7ADu, 0x3FF8471Au}},
  {{0xA341CDFBu, 0xBC88D684u}},
  {{0x9B4492EDu, 0x3FF868D9u}},
  {{0x9BD4F6BAu, 0xBC9FC6F8u}},
  {{0xD98A6699u, 0x3FF88AC7u}},
  {{0xF37CB53Au, 0x3C9994C2u}},
  {{0x422AA0DBu, 0x3FF8ACE5u}},
  {{0x56864B26u, 0x3C96E9F1u}},
  {{0x16B5448Cu, 0x3FF8CF32u}},
  {{0x32E9E3AAu, 0xBC70D55Eu}},
  {{0x99157736u, 0x3FF8F1AEu}},
  {{0xA2E3976Cu, 0x3C85CC13u}},
  {{0x0B91FFC6u, 0x3FF9145Bu}},
  {{0x2E582523u, 0xBC9DD679u}},
  {{0xB0CDC5E5u, 0x3FF93737u}},
  {{0x81B57EBBu, 0xBC675FC7u}},
  {{0xCBC8520Fu, 0x3FF95A44u}},
  {{0x96A5F039u, 0xBC764B7Cu}},
  {{0x9FDE4E50u, 0x3FF97D82u}},
  {{0x7C1B85D0u, 0xBC9D185Bu}},
  {{0x70CA07BAu, 0x3FF9A0F1u}},
  {{0x91CEE632u, 0xBC9173BDu}},
  {{0x82A3F090u, 0x3FF9C491u}},
  {{0xB071F2BEu, 0x3C7C7C46u}},
  {{0x19E32323u, 0x3FF9E863u}},
  {{0x78E64C6Eu, 0x3C7824CAu}},
  {{0x7B5DE565u, 0x3FFA0C66u}},
  {{0x5D1CD532u, 0xBC935949u}},
  {{0xEC4A2D33u, 0x3FFA309Bu}},
  {{0x7DDC36ABu, 0x3C96305Cu}},
  {{0xB23E255Du, 0x3FFA5503u}},
  {{0xDB8D41E1u, 0xBC9D2F6Eu}},
  {{0x1330B358u, 0x3FFA799Eu}},
  {{0xCAC563C6u, 0x3C9BCB7Eu}},
  {{0x5579FDBFu, 0x3FFA9E6Bu}},
  {{0x0EF7FD31u, 0x3C90FAC9u}},
  {{0xBFD3F37Au, 0x3FFAC36Bu}},
  {{0xCAE76CD0u, 0xBC8F9234u}},
  {{0x995AD3ADu, 0x3FFAE89Fu}},
  {{0x345DCC81u, 0x3C97A1CDu}},
  {{0x298DB666u, 0x3FFB0E07u}},
  {{0x4C80E424u, 0xBC9BDEF5u}},
  {{0xB84F15FBu, 0x3FFB33A2u}},
  {{0x3084D707u, 0xBC62805Eu}},
  {{0x8DE5593Au, 0x3FFB5972u}},
  {{0xBBBA6DE3u, 0xBC9C71DFu}},
  {{0xF2FB5E47u, 0x3FFB7F76u}},
  {{0x7E54AC3Au, 0xBC75584Fu}},
  {{0x30A1064Au, 0x3FFBA5B0u}},
  {{0x0E54292Eu, 0xBC9EFCD3u}},
  {{0x904BC1D2u, 0x3FFBCC1Eu}},
  {{0x7A2D9E84u, 0x3C823DD0u}},
  {{0x5BD71E09u, 0x3FFBF2C2u}},
  {{0x3F6B9C72u, 0xBC9EFDCAu}},
  {{0xDD85529Cu, 0x3FFC199Bu}},
  {{0x895048DDu, 0x3C811065u}},
  {{0x5FFFD07Au, 0x3FFC40ABu}},
  {{0xE083C60Au, 0x3C9B4537u}},
  {{0x2E57D14Bu, 0x3FFC67F1u}},
  {{0xFF483CACu, 0x3C92884Du}},
  {{0x9406E7B5u, 0x3FFC8F6Du}},
  {{0x48805C44u, 0x3C71ACBCu}},
  {{0xDCEF9069u, 0x3FFCB720u}},
  {{0xD1E949DBu, 0x3C7503CBu}},
  {{0x555DC3FAu, 0x3FFCDF0Bu}},
  {{0x53829D72u, 0xBC8DD83Bu}},
  {{0x4A07897Cu, 0x3FFD072Du}},
  {{0x43797A9Cu, 0xBC9CBC37u}},
  {{0x080D89F2u, 0x3FFD2F87u}},
  {{0x719D8577u, 0xBC9D487Bu}},
  {{0xDCFBA487u, 0x3FFD5818u}},
  {{0xD75B3706u, 0x3C82ED02u}},
  {{0x16C98398u, 0x3FFD80E3u}},
  {{0x8BEDDFE8u, 0xBC911EC1u}},
  {{0x03DB3285u, 0x3FFDA9E6u}},
  {{0x696DB532u, 0x3C9C2300u}},
  {{0xF301B460u, 0x3FFDD321u}},
  {{0x78F018C2u, 0x3C92DA57u}},
  {{0x337B9B5Fu, 0x3FFDFC97u}},
  {{0x4F184B5Bu, 0xBC91A5CDu}},
  {{0x14F5A129u, 0x3FFE2646u}},
  {{0x817A1496u, 0xBC97B627u}},
  {{0xE78B3FF6u, 0x3FFE502Eu}},
  {{0x80A9CC8Fu, 0x3C839E89u}},
  {{0xFBC74C83u, 0x3FFE7A51u}},
  {{0xCA0C8DE1u, 0x3C92D522u}},
  {{0xA2A490DAu, 0x3FFEA4AFu}},
  {{0x179C2893u, 0xBC9E9C23u}},
  {{0x2D8E67F1u, 0x3FFECF48u}},
  {{0xB411AD8Cu, 0xBC9C93F3u}},
  {{0xEE615A27u, 0x3FFEFA1Bu}},
  {{0x86A4B6B0u, 0x3C9DC7F4u}},
  {{0x376BBA97u, 0x3FFF252Bu}},
  {{0xBF0D8E43u, 0x3C93A1A5u}},
  {{0x5B6E4540u, 0x3FFF5076u}},
  {{0x2DD8A18Au, 0x3C99D3E1u}},
  {{0xAD9CBE14u, 0x3FFF7BFDu}},
  {{0xD0063509u, 0xBC9DBB12u}},
  {{0x819E90D8u, 0x3FFFA7C1u}},
  {{0xF3A5931Eu, 0x3C874853u}},
  {{0x2B8F71F1u, 0x3FFFD3C2u}},
  {{0x966579E7u, 0x3C62EB74u}},
  {{0x966457E8u, 0x3E79C3A6u}},
  {{0x46694107u, 0xBFD62E43u}},
  {{0x62B6DEE1u, 0x3FC47FD4u}},
  {{0x2A9012D8u, 0xBFB55047u}},
  {{0xFEFA39EFu, 0x3FE62E42u}},
  {{0xFF82C58Fu, 0x3FCEBFBDu}},
  {{0xD704A0C0u, 0x3FAC6B08u}},
  {{0x6FBA4E77u, 0x3F83B2ABu}},
  {{0xE78A6731u, 0x3F55D87Fu}},
  {{0x00000000u, 0x7FE00000u}},
  {{0x00000000u, 0x00100000u}},
  {{0x00000000u, 0x00000000u}},
  {{0x00000000u, 0x3FF00000u}},
  {{0x00000000u, 0xBFF00000u}},
  {{0x00000000u, 0x42C80000u}},
  {{0x40000000u, 0x3FF71547u}},
  {{0x02000000u, 0x41A00000u}},
  {{0x00000000u, 0x4C700000u}},
  {{0x00000000u, 0x33700000u}},
};

__attribute__((always_inline))
int
__ocl_svml_dpow_cout_rare (__private const double *a, __private double *b, __private double *r)
{
  int nRet = 0;
  double dbVTmp1, dbVTmp2, dbVPHH, dbVPHL;
  double dX, dY, dR, dbAX, dbSignRes, dbX1, dbRcp1, dbL1Hi, dbL1Lo, dbX2,
    dbRcp2, dbL2Hi, dbL2Lo, dbX3, dbRcp3C, dbL3Hi, dbL3Lo, dbK, dbT, dbD,
    dbR1, dbCQ, dbRcpC, dbX1Hi, dbX1Lo, dbRcpCHi, dbRcpCLo, dbTmp1, dbE,
    dbT_CQHi, dbCQLo, dbR, dbLogPart3, dbLog2Poly, dbHH, dbHL, dbHLL, dbYHi,
    dbYLo, dbTmp2, dbTmp3, dbPH, dbPL, dbPLL, dbZ, dbExp2Poly, dbExp2PolyT,
    dbResLo, dbResHi, dbRes, dbTwoPowN;
  int iEXB, iEYB, iSignX, iSignY, iYHi, iYLo, iYIsFinite, iEY, iYIsInt,
    iXIsFinite, iDenoExpAdd, iXHi, k, i1, i2, i3, iELogAX, iN, j, iERes,
    iSign, iIsSigZeroX, iIsSigZeroY;

  dX = ((*a) * ((__constant double *) _vmldPowHATab)[853 + (0)]);
  dY = ((*b) * ((__constant double *) _vmldPowHATab)[853 + (0)]);

  iEXB = ((((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF);

  iEYB = ((((__private _iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF);

  iSignX = (((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 31);

  iSignY = (((__private _iml_dp_union_t *) & dY)->dwords.hi_dword >> 31);

  iIsSigZeroX =
    (((((__private _iml_dp_union_t *) & dX)->dwords.hi_dword & 0x000FFFFF) == 0)
     && ((((__private _iml_dp_union_t *) & dX)->dwords.lo_dword) == 0));
  iIsSigZeroY =
    (((((__private _iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF) == 0)
     && ((((__private _iml_dp_union_t *) & dY)->dwords.lo_dword) == 0));

  iYHi =
    (iEYB << 20) | (((__private _iml_dp_union_t *) & dY)->dwords.hi_dword & 0x000FFFFF);

  iYLo = (((__private _iml_dp_union_t *) & dY)->dwords.lo_dword);

  iYIsFinite =
    (((((__private _iml_dp_union_t *) & dY)->dwords.hi_dword >> 20) & 0x7FF) != 0x7FF);

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

  if (!((iSignX == 0) && (iEXB == 0x3FF) && iIsSigZeroX) &&
      !((iEYB == 0) && iIsSigZeroY))
    {

      iXIsFinite =
    (((((__private _iml_dp_union_t *) & dX)->dwords.hi_dword >> 20) & 0x7FF) !=
     0x7FF);

      if ((iXIsFinite || iIsSigZeroX) && (iYIsFinite || iIsSigZeroY))
    {

      if (dX != ((__constant double *) _vmldPowHATab)[852])
        {

          if (!
          ((dX == ((__constant double *) _vmldPowHATab)[854])
           && (iYIsInt || !iYIsFinite)))
        {

          if (iXIsFinite && iYIsFinite)
            {

              if ((dX > ((__constant double *) _vmldPowHATab)[852])
              || iYIsInt)
            {

              dbSignRes =
                ((__constant double *) _vmldPowHATab)[853 +
                                 (iSignX &
                                  iYIsInt)];

              iDenoExpAdd = 0;
              dbAX = dX;
              (((__private _iml_dp_union_t *) & dbAX)->dwords.hi_dword =
               (((__private _iml_dp_union_t *) & dbAX)->dwords.
                hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (0) <<
                              31));

              if (iEXB == 0)
                {

                  dbAX =
                dbAX * ((__constant double *) _vmldPowHATab)[858];
                  iDenoExpAdd = iDenoExpAdd - 200;
                }

              dbX1 = dbAX;
              (((__private _iml_dp_union_t *) & dbX1)->dwords.hi_dword =
               (((__private _iml_dp_union_t *) & dbX1)->dwords.
                hi_dword & 0x800FFFFF) | (((_iml_uint32_t) (0x3FF)
                               & 0x7FF) << 20));

              iXHi =
                ((((__private _iml_dp_union_t *) & dbAX)->dwords.
                  hi_dword >> 20) & 0x7FF);
              iXHi = iXHi << 20;
              iXHi =
                iXHi | (((__private _iml_dp_union_t *) & dbAX)->dwords.
                    hi_dword & 0x000FFFFF);

              k = iXHi - 0x3FE7C000;
              k = k >> 20;
              k = k + iDenoExpAdd;

              i1 =
                (((__private _iml_dp_union_t *) & dbX1)->dwords.
                 hi_dword & 0x000FFFFF);
              i1 = i1 & 0xFC000;
              i1 = i1 + 0x4000;
              i1 = i1 >> 15;

              dbRcp1 = ((__constant double *) _vmldPowHATab)[0 + i1];

              dbL1Hi =
                ((__constant double *) _vmldPowHATab)[33 + 2 * (i1) +
                                 0];
              dbL1Lo =
                ((__constant double *) _vmldPowHATab)[33 + 2 * (i1) +
                                 1];

              dbX2 = dbX1 * dbRcp1;

              i2 =
                (((__private _iml_dp_union_t *) & dbX2)->dwords.
                 hi_dword & 0x000FFFFF);
              i2 = i2 & 0xFC00;
              i2 = i2 + 0x400;
              i2 = i2 >> 11;

              dbRcp2 = ((__constant double *) _vmldPowHATab)[99 + i2];

              dbL2Hi =
                ((__constant double *) _vmldPowHATab)[132 + 2 * (i2) +
                                 0];
              dbL2Lo =
                ((__constant double *) _vmldPowHATab)[132 + 2 * (i2) +
                                 1];

              dbX3 = dbX2 * dbRcp2;

              i3 =
                (((__private _iml_dp_union_t *) & dbX3)->dwords.
                 hi_dword & 0x000FFFFF);
              i3 = i3 & 0xFF0;
              i3 = i3 + 0x10;
              i3 = i3 >> 5;

              dbRcp3C =
                ((__constant double *) _vmldPowHATab)[198 + i3];

              dbL3Hi =
                ((__constant double *) _vmldPowHATab)[327 + 2 * (i3) +
                                 0];
              dbL3Lo =
                ((__constant double *) _vmldPowHATab)[327 + 2 * (i3) +
                                 1];

              dbK = (double) k;
              dbT = (dbK + dbL1Hi);
              dbT = (dbT + dbL2Hi);
              dbT = (dbT + dbL3Hi);

              dbD = (dbL2Lo + dbL3Lo);
              dbD = (dbD + dbL1Lo);

              dbR1 = (dbX3 * dbRcp3C);
              dbCQ =
                (dbR1 - ((__constant double *) _vmldPowHATab)[856]);

              dbRcpC = (dbRcp1 * dbRcp2);
              dbRcpC = (dbRcpC * dbRcp3C);

              dbVTmp1 =
                ((dbX1) *
                 (((__constant double *) _vmldPowHATab)[857]));
              dbVTmp2 = (dbVTmp1 - (dbX1));
              dbVTmp1 = (dbVTmp1 - dbVTmp2);
              dbVTmp2 = ((dbX1) - dbVTmp1);
              dbX1Hi = dbVTmp1;
              dbX1Lo = dbVTmp2;

              dbVTmp1 =
                ((dbRcpC) *
                 (((__constant double *) _vmldPowHATab)[857]));
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

              iELogAX =
                ((((__private _iml_dp_union_t *) & dbT_CQHi)->dwords.
                  hi_dword >> 20) & 0x7FF);

              if (iELogAX + iEYB < 11 + 2 * 0x3FF)
                {

                  if (iELogAX + iEYB > -62 + 2 * 0x3FF)
                {

                  dbR = (dbCQ + dbE);

                  dbLog2Poly =
                    ((((((__constant double *) _vmldPowHATab)[844])
                       * dbR +
                       ((__constant double *) _vmldPowHATab)[843])
                      * dbR +
                      ((__constant double *) _vmldPowHATab)[842]) *
                     dbR +
                     ((__constant double *) _vmldPowHATab)[841]) *
                    dbR;

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

                  dbVTmp1 =
                    ((dbHH) *
                     (((__constant double *) _vmldPowHATab)[857]));
                  dbVTmp2 = (dbVTmp1 - (dbHH));
                  dbVTmp1 = (dbVTmp1 - dbVTmp2);
                  dbVTmp2 = ((dbHH) - dbVTmp1);
                  dbHH = dbVTmp1;
                  dbHL = dbVTmp2;

                  dbVTmp1 =
                    ((dY) *
                     (((__constant double *) _vmldPowHATab)[857]));
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

                  dbVTmp1 =
                    (dbPH +
                     ((__constant double *) _vmldPowHATab)[855]);
                  iN =
                    (((__private _iml_dp_union_t *) & dbVTmp1)->dwords.
                     lo_dword);
                  j = iN & 0x7F;
                  iN = iN >> 7;
                  dbVPHH =
                    (dbVTmp1 -
                     ((__constant double *) _vmldPowHATab)[855]);
                  dbVPHL = (dbPH - dbVPHH);

                  dbZ = (dbPLL + dbPL);
                  dbZ = (dbZ + dbVPHL);

                  dbExp2Poly =
                    (((((((__constant double *)
                      _vmldPowHATab)[849]) * dbZ +
                    ((__constant double *) _vmldPowHATab)[848])
                       * dbZ +
                       ((__constant double *) _vmldPowHATab)[847])
                      * dbZ +
                      ((__constant double *) _vmldPowHATab)[846]) *
                     dbZ +
                     ((__constant double *) _vmldPowHATab)[845]) *
                    dbZ;

                  dbExp2PolyT =
                    (dbExp2Poly *
                     ((__constant double *) _vmldPowHATab)[585 +
                                      2 *
                                      (j) +
                                      0]);
                  dbResLo =
                    (dbExp2PolyT +
                     ((__constant double *) _vmldPowHATab)[585 +
                                      2 *
                                      (j) +
                                      1]);
                  dbResHi =
                    ((__constant double *) _vmldPowHATab)[585 +
                                     2 * (j) +
                                     0];

                  dbRes = (dbResHi + dbResLo);
                  iERes =
                    ((((__private _iml_dp_union_t *) & dbRes)->dwords.
                      hi_dword >> 20) & 0x7FF) - 0x3FF;
                  iERes = (iERes + iN);

                  if (iERes < 1024)
                    {
                      if (iERes >= -1022)
                    {

                      (((__private _iml_dp_union_t *) & dbRes)->
                       dwords.hi_dword =
                       (((__private _iml_dp_union_t *) & dbRes)->
                        dwords.
                        hi_dword & 0x800FFFFF) |
                       (((_iml_uint32_t) (iERes + 0x3FF) &
                         0x7FF) << 20));

                      dbRes = dbRes * dbSignRes;
                      dR = dbRes;
                    }
                      else
                    {

                      if (iERes >= -1022 - 10)
                        {

                          dbVTmp1 =
                        ((dbResHi) + (dbResLo));
                          dbTmp1 = ((dbResHi) - dbVTmp1);
                          dbVTmp2 = (dbTmp1 + (dbResLo));
                          dbResHi = dbVTmp1;
                          dbResLo = dbVTmp2;
                          dbVTmp1 =
                        ((dbResHi) *
                         (((__constant double *)
                           _vmldPowHATab)[857]));
                          dbVTmp2 = (dbVTmp1 - (dbResHi));
                          dbVTmp1 = (dbVTmp1 - dbVTmp2);
                          dbVTmp2 = ((dbResHi) - dbVTmp1);
                          dbResHi = dbVTmp1;
                          dbTmp2 = dbVTmp2;
                          dbResLo = (dbResLo + dbTmp2);

                          dbSignRes *=
                        ((__constant double *)
                         _vmldPowHATab)[859];
                          iN = (iN + 200);

                          dbTwoPowN =
                        ((__constant double *)
                         _vmldPowHATab)[853];
                          (((__private _iml_dp_union_t *) &
                        dbTwoPowN)->dwords.hi_dword =
                           (((__private _iml_dp_union_t *) &
                         dbTwoPowN)->dwords.
                        hi_dword & 0x800FFFFF) |
                           (((_iml_uint32_t) (iN + 0x3FF)
                         & 0x7FF) << 20));

                          dbResHi = (dbResHi * dbTwoPowN);

                          dbResLo = (dbResLo * dbTwoPowN);

                          dbRes = (dbResHi + dbResLo);
                          dbRes = (dbRes * dbSignRes);

                          dbVTmp1 =
                        ((__constant double *)
                         _vmldPowHATab)[851];
                          dbVTmp1 = (dbVTmp1 * dbVTmp1);
                          dbRes = (dbRes + dbVTmp1);

                          dR = dbRes;
                        }
                      else
                        {
                          if (iERes >= -1074 - 10)
                        {

                          dbSignRes *=
                            ((__constant double *)
                             _vmldPowHATab)[859];
                          iN = iN + 200;

                          dbTwoPowN =
                            ((__constant double *)
                             _vmldPowHATab)[853];
                          (((__private _iml_dp_union_t *) &
                            dbTwoPowN)->dwords.
                           hi_dword =
                           (((__private _iml_dp_union_t *) &
                             dbTwoPowN)->dwords.
                            hi_dword & 0x800FFFFF) |
                           (((_iml_uint32_t)
                             (iN +
                              0x3FF) & 0x7FF) << 20));

                          dbRes = (dbRes * dbTwoPowN);
                          dbRes = (dbRes * dbSignRes);

                          dbVTmp1 =
                            ((__constant double *)
                             _vmldPowHATab)[851];
                          dbVTmp1 *= dbVTmp1;
                          dbRes = (dbRes + dbVTmp1);

                          dR = dbRes;
                        }
                          else
                        {

                          dbVTmp1 =
                            ((__constant double *)
                             _vmldPowHATab)[851];
                          dbVTmp1 *= dbVTmp1;
                          dbRes =
                            (dbVTmp1 * dbSignRes);
                          dR = dbRes;
                        }
                        }
                    }
                    }
                  else
                    {

                      dbVTmp1 =
                    ((__constant double *) _vmldPowHATab)[850];
                      dbVTmp1 = (dbVTmp1 * dbVTmp1);
                      dbRes = (dbVTmp1 * dbSignRes);
                      dR = dbRes;
                    }
                }
                  else
                {

                  dbVTmp1 =
                    ((__constant double *) _vmldPowHATab)[853];
                  dbVTmp1 =
                    (dbVTmp1 +
                     ((__constant double *) _vmldPowHATab)[851]);
                  dR = (dbVTmp1 * dbSignRes);
                }
                }
              else
                {

                  iSign =
                iSignY ^ (((__private _iml_dp_union_t *) & dbT_CQHi)->
                      dwords.hi_dword >> 31);

                  dbTmp1 =
                ((__constant double *) _vmldPowHATab)[850 +
                                 (iSign)];

                  dbTmp1 = (dbTmp1 * dbTmp1);

                  dbTmp1 = (dbTmp1 * dbSignRes);
                  dR = dbTmp1;
                }
            }
              else
            {

              dbVTmp1 = ((__constant double *) _vmldPowHATab)[852];
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

                  dR = ((__constant double *) _vmldPowHATab)[852];
                }
            }
              else
            {

              if (iSignY)
                {

                  dR =
                ((__constant double *) _vmldPowHATab)[852] *
                ((__constant double *) _vmldPowHATab)[853 +
                                 (iYIsInt &
                                  iSignX)];
                }
              else
                {

                  dbTmp1 = dX * dX;
                  dbTmp1 = dbTmp1 * dY;
                  dR =
                dbTmp1 *
                ((__constant double *) _vmldPowHATab)[853 +
                                 (iYIsInt &
                                  iSignX)];
                }
            }
            }
        }
          else
        {

          dR = ((__constant double *) _vmldPowHATab)[853 + (iYIsInt & 1)];
        }
        }
      else
        {

          dbTmp1 = dX * dX;

          if (iSignY)
        {

          dR =
            ((__constant double *) _vmldPowHATab)[853 +
                             (iYIsInt & iSignX)] /
            dbTmp1;
          nRet = 1;
        }
          else
        {

          dR =
            ((__constant double *) _vmldPowHATab)[853 +
                             (iYIsInt & iSignX)] *
            dbTmp1;
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
      iSign = (((__private _iml_dp_union_t *) & dbVTmp1)->dwords.hi_dword >> 31);
      dbVTmp2 = ((__constant double *) _vmldPowHATab)[853];
      (((__private _iml_dp_union_t *) & dbVTmp2)->dwords.hi_dword =
       (((__private _iml_dp_union_t *) & dbVTmp2)->dwords.
    hi_dword & 0x7FFFFFFF) | ((_iml_uint32_t) (iSign) << 31));

      dR = dbVTmp2 * dbVTmp2;
    }

  *r = dR;
  return nRet;
}

#endif // __IA32E_POW_D_G_COUT_CL__
