/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
/*
// ALGORITHM DESCRIPTION:
//
//   NOTE: Since the hyperbolic tangent function is odd
//         (tanh(x) = -tanh(-x)), below algorithm deals with the absolute
//         value of the argument |x|: tanh(x) = sign(x) * tanh(|x|)
//
//   We use a table lookup method to compute tanh(|x|).
//   The basic idea is to split the input range into a number of subintervals
//   and to approximate tanh(.) with a polynomial on each of them.
//
//   IEEE SPECIAL CONDITIONS:
//   x = [+,-]0, r = [+,-]0
//   x = +Inf,   r = +1
//   x = -Inf,   r = -1
//   x = QNaN,   r = QNaN
//   x = SNaN,   r = QNaN
//
//
//   ALGORITHM DETAILS
//   We handle special values in a callout function, aside from main path
//   computations. "Special" for this algorithm are:
//   INF, NAN, |x| > HUGE_THRESHOLD
//
//
//   Main path computations are organized as follows:
//   Actually we split the interval [0, SATURATION_THRESHOLD)
//   into a number of subintervals.  On each subinterval we approximate tanh(.)
//   with a minimax polynomial of pre-defined degree. Polynomial coefficients
//   are computed beforehand and stored in table. We also use
//
//       y := |x| + B,
//
//   here B depends on subinterval and is used to make argument
//   closer to zero.
//   We also add large fake interval [SATURATION_THRESHOLD, HUGE_THRESHOLD],
//   where 1.0 + 0.0*y + 0.0*y^2 ... coefficients are stored - just to
//   preserve main path computation logic but return 1.0 for all arguments.
//
//   Hence reconstruction looks as follows:
//   we extract proper polynomial and range reduction coefficients
//        (Pj and B), corresponding to subinterval, to which |x| belongs,
//        and return
//
//       r := sign(x) * (P0 + P1 * y + ... + Pn * y^n)
//
//   NOTE: we use multiprecision technique to multiply and sum the first
//         K terms of the polynomial. So Pj, j = 0..K are stored in
//         table each as a pair of target precision numbers (Pj and PLj) to
//         achieve wider than target precision.
//
// --
//
*/
#include "../imf.h"
#pragma OPENCL FP_CONTRACT OFF
typedef struct {
  // Use small table
  unsigned int _sC[32];
  unsigned int _sP0[32];
  unsigned int _sP1[32];
  unsigned int _sP2[32];
  unsigned int _sP3[32];
  unsigned int _sP4[32];
  unsigned int _sP5[32];
  unsigned int _sP6[32];
  unsigned int _sP7[32];
  unsigned int _iExpMantMask_UISA;
  unsigned int _iMinIdxOfsMask_UISA;
  unsigned int _iMaxIdxMask_UISA;
  // Use large table
  unsigned long _dbP[(27 * 8)];
  unsigned int _sSignMask;
  unsigned int _sAbsMask;
  unsigned int _iExpMantMask;
  unsigned int _iExpMask;
  unsigned int _iMinIdxOfsMask;
  unsigned int _iMaxIdxMask;
} __ocl_svml_internal_stanh_ha_data_t;
static __ocl_svml_internal_stanh_ha_data_t __ocl_svml_internal_stanh_ha_data = {
    {/*== _sC ==*/
     0x00000000u, 0x3d700000u, 0x3d900000u, 0x3db00000u, 0x3dd00000u,
     0x3df00000u, 0x3e100000u, 0x3e300000u, 0x3e500000u, 0x3e700000u,
     0x3e900000u, 0x3eb00000u, 0x3ed00000u, 0x3ef00000u, 0x3f100000u,
     0x3f300000u, 0x3f500000u, 0x3f700000u, 0x3f900000u, 0x3fb00000u,
     0x3fd00000u, 0x3ff00000u, 0x40100000u, 0x40300000u, 0x40500000u,
     0x40700000u, 0x40900000u, 0x40b00000u, 0x40d00000u, 0x40f00000u,
     0x41100000u, 0x00000000u}, /* C (midpoint offset) */
    {                           /*== p0 ==*/
     0x00000000u, 0x3d6fb9c9u, 0x3d8fc35fu, 0x3daf9169u, 0x3dcf49abu,
     0x3deee849u, 0x3e0f0ee8u, 0x3e2e4984u, 0x3e4d2f8eu, 0x3e6bb32eu,
     0x3e8c51cdu, 0x3ea96163u, 0x3ec543f1u, 0x3edfd735u, 0x3f028438u,
     0x3f18abf0u, 0x3f2bc480u, 0x3f3bec1cu, 0x3f4f2e5bu, 0x3f613c53u,
     0x3f6ce37du, 0x3f743c4fu, 0x3f7a5febu, 0x3f7dea85u, 0x3f7f3b3du,
     0x3f7fb78cu, 0x3f7fefd4u, 0x3f7ffdd0u, 0x3f7fffb4u, 0x3f7ffff6u,
     0x3f7fffffu, 0x3f800000u}, /* p0 */
    {                           /*== p1 ==*/
     0x00000000u, 0xb0a1501eu, 0xb11d0b9eu, 0xaf932f10u, 0x30fa467cu,
     0x31249ef0u, 0x312b9ae0u, 0xb1dd8650u, 0xb044cc00u, 0x31f9ba98u,
     0xb0ca4580u, 0xb21e2644u, 0xb234cc14u, 0x32397534u, 0xb2d0938eu,
     0xb1303240u, 0xb2ef278au, 0x32e33e9cu, 0xb20b98c0u, 0xb2d5b8c2u,
     0x32fe70b8u, 0x32cec21au, 0x32c28572u, 0xb2230d78u, 0xb2a2c238u,
     0xb2315220u, 0xb2dddb64u, 0xb2d020d0u, 0x321fe128u, 0xb2875bb0u,
     0x32fa6146u, 0x00000000u}, /* p1 */
    {                           /*== p2 ==*/
     0x3f800000u, 0x3f7f1f84u, 0x3f7ebd11u, 0x3f7e1e5fu, 0x3f7d609fu,
     0x3f7c842du, 0x3f7b00e5u, 0x3f789580u, 0x3f75b8adu, 0x3f726fd9u,
     0x3f6cc59bu, 0x3f63fb92u, 0x3f59ff97u, 0x3f4f11d7u, 0x3f3d7573u,
     0x3f24f360u, 0x3f0cbfe7u, 0x3eec1a69u, 0x3eb0a801u, 0x3e6753a2u,
     0x3e132f1au, 0x3db7e7d3u, 0x3d320845u, 0x3c84d3d4u, 0x3bc477b7u,
     0x3b10d3dau, 0x3a01601eu, 0x388c1a3bu, 0x3717b0dau, 0x35a43bceu,
     0x338306c6u, 0x00000000u}, /* p2 */
    {                           /*== p3 ==*/
     0xb0343c7bu, 0xbd6ee69du, 0xbd8f0da7u, 0xbdae477du, 0xbdcd2a1fu,
     0xbdeba80du, 0xbe0c443bu, 0xbe293cf3u, 0xbe44f282u, 0xbe5f3651u,
     0xbe81c7c0u, 0xbe96d7cau, 0xbea7fb8eu, 0xbeb50e9eu, 0xbec12efeu,
     0xbec4be92u, 0xbebce070u, 0xbead510eu, 0xbe8ef7d6u, 0xbe4b8704u,
     0xbe083237u, 0xbdaf7449u, 0xbd2e1ec4u, 0xbc83bf06u, 0xbbc3e0b5u,
     0xbb10aadcu, 0xba0157dbu, 0xb88c18f2u, 0xb717b096u, 0xb5a43baeu,
     0xb383012cu, 0x00000000u}, /* p3 */
    {                           /*== p4 ==*/
     0xbeaaaaa5u, 0xbeab0612u, 0xbea7f01fu, 0xbea4e120u, 0xbea387b7u,
     0xbea15962u, 0xbe9d57f7u, 0xbe976b5au, 0xbe90230du, 0xbe880dffu,
     0xbe7479b3u, 0xbe4c3d88u, 0xbe212482u, 0xbdeb8cbau, 0xbd5e78adu,
     0x3c6b5e6eu, 0x3d839143u, 0x3dc21ee1u, 0x3de347afu, 0x3dcbec96u,
     0x3d99ef2du, 0x3d542ea1u, 0x3cdde701u, 0x3c2cca67u, 0x3b81cb27u,
     0x3ac073a1u, 0x39ac3032u, 0x383a94d9u, 0x36ca081du, 0x355abd4cu,
     0x332b3cb6u, 0x00000000u}, /* p4 */
    {                           /*== p5 ==*/
     0xb76dd6b9u, 0xbe1c276du, 0x3c1dcf2fu, 0x3dc1a78du, 0x3d96f985u,
     0x3da2b61bu, 0x3dc13397u, 0x3dd2f670u, 0x3df48a0au, 0x3e06c5a8u,
     0x3e1a3abau, 0x3e27c405u, 0x3e2e78d0u, 0x3e2c3e44u, 0x3e1d3097u,
     0x3df4a8f4u, 0x3da38508u, 0x3d31416au, 0x3b562657u, 0xbcaeeac9u,
     0xbcce9419u, 0xbcaaeac4u, 0xbc49e7d0u, 0xbba71dddu, 0xbb003b0eu,
     0xba3f9a05u, 0xb92c08a7u, 0xb7ba9232u, 0xb64a0b0fu, 0xb4dac169u,
     0xb2ab78acu, 0x00000000u}, /* p5 */
    {                           /*== p6 ==*/
     0x3e0910e9u, 0x43761143u, 0x4165ecdcu, 0xc190f756u, 0xc08c097du,
     0xc02ba813u, 0xbf7f6bdau, 0x3f2b1dc0u, 0x3ece105du, 0x3f426a94u,
     0xbadb0dc4u, 0x3da43b17u, 0xbd51ab88u, 0xbcaea23du, 0xbd3b6d8du,
     0xbd6caaadu, 0xbd795bedu, 0xbd5fdddau, 0xbd038f3bu, 0xbc1cad63u,
     0x3abb4766u, 0x3b95f10bu, 0x3b825873u, 0x3afaea66u, 0x3a49f878u,
     0x39996bf3u, 0x388f3e6cu, 0x371bb0e3u, 0x35a8a5e6u, 0x34369b17u,
     0x322487b0u, 0x00000000u}, /* p6 */
    {                           /*== p7 ==*/
     0xbc0e2f66u, 0x460bda12u, 0x43d638efu, 0xc3e11c3eu, 0xc2baa4e9u,
     0xc249da2du, 0xc1859b82u, 0x40dd5b57u, 0x40494640u, 0x40c730a8u,
     0xbf0f160eu, 0x3e30e76fu, 0xbea81387u, 0xbdb26a1cu, 0xbd351e57u,
     0xbb4c01a0u, 0x3c1d7bfbu, 0x3c722cd1u, 0x3c973f1cu, 0x3c33a31bu,
     0x3b862ef4u, 0x3a27b3d0u, 0xba3b5907u, 0xba0efc22u, 0xb97f9f0fu,
     0xb8c8af50u, 0xb7bdddfbu, 0xb64f2950u, 0xb4e085b1u, 0xb3731dfau,
     0xb15a1f04u, 0x00000000u}, /* p7 */
    0x7fe00000u,                /* _iExpMantMask_UISA     */
    0x3d400000u,                /* _iMinIdxOfsMask_UISA   */
    0x03e00000u,                /* _iMaxIdxMask_UISA      */
    {
        /* Pol00:  err=8.97e-11, x in [0.00000; 0.12500]. */
        0x0000000000000000uL, /* A00 = +0.000000000000000000000e-01 */
        0x3FF000000006A31DuL, /* A01 = +1.000000000096583407938e+00 */
        0xBE6DEFDE92C4F79BuL, /* A02 = -5.576198911165821221685e-08 */
        0xBFD5553F7294F0DDuL, /* A03 = -3.333281153643506411477e-01 */
        0xBF276CBDCCCDB701uL, /* A04 = -1.787168206663289620351e-04 */
        0x3FC16BE24561FB19uL, /* A05 = +1.361048544676684290966e-01 */
        0xBF945BC4A4EAA3AFuL, /* A06 = -1.988131767817307923862e-02 */
        0x0000000000000000uL,
        /* Pol01:  e in [0.1rr=1.05e-15, x2500; 0.15625]. */
        0xBE68277761C5AF2EuL, /* A00 = -4.499063896681357642699e-08 */
        0x3FF0000263167A05uL, /* A01 = +1.000002276479450502578e+00 */
        0xBF09EBCEDD85B175uL, /* A02 = -4.944062283382652701761e-05 */
        0xBFD54B886E7A59B5uL, /* A03 = -3.327351645360992482559e-01 */
        0xBF71DFA6BC24A233uL, /* A04 = -4.363681133590579901138e-03 */
        0x3FC388AFBDCFEABFuL, /* A05 = +1.526088406388534657321e-01 */
        0xBFA8AC02C389F5D2uL, /* A06 = -4.818733822774458330773e-02 */
        0x0000000000000000uL,
        /* Pol02:  e in [0.1rr=7.25e-16, x5625; 0.18750]. */
        0xBE85D5DB74F57D93uL, /* A00 = -1.626862577803522216316e-07 */
        0x3FF0000715D460DBuL, /* A01 = +1.000006757041822735843e+00 */
        0xBF1FA2DAC6F48388uL, /* A02 = -1.206823378550170690685e-04 */
        0xBFD5419C4F2D1E3BuL, /* A03 = -3.321295521932118854913e-01 */
        0xBF7DC3B257A22BA7uL, /* A04 = -7.266708993607296461492e-03 */
        0x3FC47C7D37008997uL, /* A05 = +1.600491064610422842218e-01 */
        0xBFACC0082A078926uL, /* A06 = -5.615258706144947431493e-02 */
        0x0000000000000000uL,
        /* Pol03:  e in [0.1rr=4.84e-16, x8750; 0.21875]. */
        0xBE9DE52836AA24BFuL, /* A00 = -4.454723724697224448596e-07 */
        0x3FF0001087517324uL, /* A01 = +1.000015762888502912631e+00 */
        0xBF2F8218014C6D29uL, /* A02 = -2.403883269373821584842e-04 */
        0xBFD533AEE5ADD15CuL, /* A03 = -3.312794917067287681078e-01 */
        0xBF85D926EC078549uL, /* A04 = -1.066809089474198175840e-02 */
        0x3FC56AC1320650BDuL, /* A05 = +1.673203939686852825819e-01 */
        0xBFB00935CF576018uL, /* A06 = -6.264053642314626468846e-02 */
        0x0000000000000000uL,
        /* Pol04:  e in [0.2rr=3.01e-16, x1875; 0.25000]. */
        0xBEB068E713261C1DuL, /* A00 = -9.780988646528018457660e-07 */
        0x3FF0001FD116A35CuL, /* A01 = +1.000030342818909012692e+00 */
        0xBF3AAA8332FF5DA7uL, /* A02 = -4.068918524331118272662e-04 */
        0xBFD5230BFA2F4B5FuL, /* A03 = -3.302640860758198626534e-01 */
        0xBF8CFDAB4B1D041CuL, /* A04 = -1.415571043946966395266e-02 */
        0x3FC63C5E1E5F0C1BuL, /* A05 = +1.737172745034464005354e-01 */
        0xBFB14A00EE874965uL, /* A06 = -6.753545592736072411544e-02 */
        0x0000000000000000uL,
        /* Pol05:  e in [0.2rr=1.25e-14, x5000; 0.31250]. */
        0xBEC1D1CC995B465AuL, /* A00 = -2.124253294912692735269e-06 */
        0x3FF0003BFC9F9FA7uL, /* A01 = +1.000057207880652887511e+00 */
        0xBF45F2405EB78312uL, /* A02 = -6.697477633076118295335e-04 */
        0xBFD50C881348F074uL, /* A03 = -3.288898647622382487299e-01 */
        0xBF92A43C07BD0A14uL, /* A04 = -1.820462987167033308555e-02 */
        0x3FC70D3F9EDD46B3uL, /* A05 = +1.800918126778491001883e-01 */
        0xBFB25C93E2CFC5D2uL, /* A06 = -7.172512328522565039357e-02 */
        0x0000000000000000uL,
        /* Pol06:  e in [0.3rr=9.54e-15, x1250; 0.37500]. */
        0xBEBAFC5951E455ADuL, /* A00 = -1.608475322327977283528e-06 */
        0x3FF000338E8A7C9CuL, /* A01 = +1.000049168396606624754e+00 */
        0xBF44528BD04B1B6EuL, /* A02 = -6.201918300455641543806e-04 */
        0xBFD50EF882B5F9C7uL, /* A03 = -3.290387417290756011390e-01 */
        0xBF926DCE22844AADuL, /* A04 = -1.799699865029917925097e-02 */
        0x3FC70AD1CC96BDF2uL, /* A05 = +1.800176857918667772118e-01 */
        0xBFB260B5E6B89CC5uL, /* A06 = -7.178818590965636847745e-02 */
        0x0000000000000000uL,
        /* Pol07:  e in [0.3rr=2.20e-14, x7500; 0.43750]. */
        0x3EF0299BB6893EA1uL, /* A00 = +1.541379188757400079735e-05 */
        0x3FEFFE36C6C2BC18uL, /* A01 = +9.997819788021642395393e-01 */
        0x3F52821FD07BD4F7uL, /* A02 = +1.129656857242365133528e-03 */
        0xBFD5733FEF4ADD77uL, /* A03 = -3.351592861976863679807e-01 */
        0xBF7851E165D82E62uL, /* A04 = -5.937462291977975772928e-03 */
        0x3FC56AF126A3AC3CuL, /* A05 = +1.673261107128797275934e-01 */
        0xBFB0F36D44EF195AuL, /* A06 = -6.621439869841108660786e-02 */
        0x0000000000000000uL,
        /* Pol08:  e in [0.4rr=2.78e-14, x3750; 0.50000]. */
        0x3F16DD09F9855499uL, /* A00 = +8.721707707100497550343e-05 */
        0x3FEFF639E7791EBDuL, /* A01 = +9.988069077453115562903e-01 */
        0x3F7B407AEE840846uL, /* A02 = +6.653289987527405066614e-03 */
        0xBFD684FF1CE3511CuL, /* A03 = -3.518674642666768104249e-01 */
        0x3F9710F011FE379DuL, /* A04 = +2.252554998226906704528e-02 */
        0x3FC21A8490A5C684uL, /* A05 = +1.414342600142398653773e-01 */
        0xBFACDEF4D1B833AAuL, /* A06 = -5.638852176716764585951e-02 */
        0x0000000000000000uL,
        /* Pol09:  e in [0.5rr=3.56e-12, x0000; 0.62500]. */
        0x3F3DB7344EEF7BC7uL, /* A00 = +4.534247117912695488294e-04 */
        0x3FEFD39A63FEDACAuL, /* A01 = +9.945804551160446482783e-01 */
        0x3F9BA8D02E96079EuL, /* A02 = +2.701115879381853907004e-02 */
        0xBFD9DF55D986F340uL, /* A03 = -4.042563080109182749311e-01 */
        0x3FB9371787006E2DuL, /* A04 = +9.849688573779565026189e-02 */
        0x3FB523268BC2AB46uL, /* A05 = +8.256760513419889035980e-02 */
        0xBFA31F14F14414F5uL, /* A06 = -3.734651036396911122361e-02 */
        0x0000000000000000uL,
        /* Pol10:  e in [0.6rr=2.61e-12, x2500; 0.75000]. */
        0x3F607ABD04390FA6uL, /* A00 = +2.011651203959591120707e-03 */
        0x3FEF5A2B730F703EuL, /* A01 = +9.797570464393243749868e-01 */
        0x3FB5FDFC22F53666uL, /* A02 = +8.590675214255724667645e-02 */
        0xBFE0F0808FF94C0BuL, /* A03 = -5.293581783283810571206e-01 */
        0x3FCFC9847A2EE0FEuL, /* A04 = +2.483373257252949328766e-01 */
        0xBF8B6A646A50F27BuL, /* A05 = -1.338652085734647577409e-02 */
        0xBF87EBDB8ED25E53uL, /* A06 = -1.168033151252171296386e-02 */
        0x0000000000000000uL,
        /* Pol11:  e in [0.7rr=1.43e-12, x5000; 0.87500]. */
        0x3F75C710A4CBF450uL, /* A00 = +5.316796316717178894073e-03 */
        0x3FEE81EC05B6BC39uL, /* A01 = +9.533596145246355790803e-01 */
        0x3FC6427314E0420EuL, /* A02 = +1.739028789456970591765e-01 */
        0xBFE5F44F553BD24AuL, /* A03 = -6.860729851455940053739e-01 */
        0x3FD9F55A79F4AC9EuL, /* A04 = +4.056001845335205358012e-01 */
        0xBFB902BCA7FD83BFuL, /* A05 = -9.769801236463047933167e-02 */
        0x3F7D6EA114F55349uL, /* A06 = +7.185582376526175076015e-03 */
        0x0000000000000000uL,
        /* Pol12:  e in [0.8rr=5.12e-13, x7500; 1.00000]. */
        0x3F83741830C6F615uL, /* A00 = +9.498776424943714338789e-03 */
        0x3FED95E102CFB52CuL, /* A01 = +9.245457701596584421111e-01 */
        0x3FD06E31AEBD35B9uL, /* A02 = +2.567257124363453990590e-01 */
        0xBFEA05BA935A3A1BuL, /* A03 = -8.131993177392503602263e-01 */
        0x3FE07EEF35904669uL, /* A04 = +5.154949232257425295600e-01 */
        0xBFC2FF9CA95468A3uL, /* A05 = -1.484256579213410753848e-01 */
        0x3F915C6048511BBEuL, /* A06 = +1.695394936930560286781e-02 */
        0x0000000000000000uL,
        /* Pol13:  e in [1.0rr=2.37e-11, x0000; 1.25000]. */
        0x3F84BEFAE0BACF16uL, /* A00 = +1.012989042587934471196e-02 */
        0x3FED6EFEEFAD09D3uL, /* A01 = +9.197992974443941482932e-01 */
        0x3FD1566FC1B92A45uL, /* A02 = +2.709006683186639397043e-01 */
        0xBFEAB8E2B6D8965BuL, /* A03 = -8.350690432750512703919e-01 */
        0x3FE116F42A1B00FAuL, /* A04 = +5.340519735691764413588e-01 */
        0xBFC40E3ECE1CAB9EuL, /* A05 = -1.566847330480784505902e-01 */
        0x3F92E8C72958BF4BuL, /* A06 = +1.846610250339545886145e-02 */
        0x0000000000000000uL,
        /* Pol14:  e in [1.2rr=4.05e-11, x5000; 1.50000]. */
        0xBF9540312F58C1CBuL, /* A00 = -2.075268603783956763986e-02 */
        0x3FF10BA9282C8BA6uL, /* A01 = +1.065346867494306959401e+00 */
        0xBF8FB70E4FD40A92uL, /* A02 = -1.548587018124195427426e-02 */
        0xBFE115CD7AFD2C33uL, /* A03 = -5.339114572647133405425e-01 */
        0x3FD6C11AB555B5E0uL, /* A04 = +3.555361529205054438307e-01 */
        0xBFB9A1DE489AE8D7uL, /* A05 = -1.001261641748983771683e-01 */
        0x3F867E656D038F4AuL, /* A06 = +1.098326910909687773032e-02 */
        0x0000000000000000uL,
        /* Pol15:  e in [1.5rr=2.36e-11, x0000; 1.75000]. */
        0xBFB8FEEADD186056uL, /* A00 = -9.763973138649481575690e-02 */
        0x3FF5F55365982F5BuL, /* A01 = +1.372393986562164824861e+00 */
        0xBFE0DF5B13EA5818uL, /* A02 = -5.272651089642321764472e-01 */
        0xBFB4043FD932506BuL, /* A03 = -7.818984081888095960533e-02 */
        0x3FC03DA439FDD281uL, /* A04 = +1.268811495935047439776e-01 */
        0xBFA3E221FE149FFBuL, /* A05 = -3.883463121957216740432e-02 */
        0x3F70E687EB4458ECuL, /* A06 = +4.126101430548905629747e-03 */
        0x0000000000000000uL,
        /* Pol16:  e in [1.7rr=9.38e-12, x5000; 2.00000]. */
        0xBFC8B6A728D01448uL, /* A00 = -1.930741261750503756645e-01 */
        0x3FFB373A7AC80CE3uL, /* A01 = +1.700983504881883545679e+00 */
        0xBFEFF9F50069A3EFuL, /* A02 = -9.992623336848945880817e-01 */
        0x3FD22AC674F9ED2EuL, /* A03 = +2.838607923838080138168e-01 */
        0xBF9E3CA89C9332F7uL, /* A04 = -2.952826934488236507170e-02 */
        0xBF668CA45BE7A635uL, /* A05 = -2.752610219648921833363e-03 */
        0x3F456B344B630B12uL, /* A06 = +6.536488861218234334288e-04 */
        0x0000000000000000uL,
        /* Pol17:  e in [2.0rr=1.11e-10, x0000; 2.50000]. */
        0xBFD2038AF9EEE89AuL, /* A00 = -2.814662400011315179782e-01 */
        0x3FFF6F4F96A3D05AuL, /* A01 = +1.964675510823346460398e+00 */
        0xBFF53DC66A0D78D3uL, /* A02 = -1.327581800716463034107e+00 */
        0x3FE0125D81D29A0EuL, /* A03 = +5.022418532942667379615e-01 */
        0xBFBC82A28D56B6D2uL, /* A04 = -1.113683314385654210543e-01 */
        0x3F8BEAA70CC91126uL, /* A05 = +1.363115794914244657821e-02 */
        0xBF476EFA96A8E6D1uL, /* A06 = -7.151340080959416424869e-04 */
        0x0000000000000000uL,
        /* Pol18:  e in [2.5rr=1.01e-10, x0000; 3.00000]. */
        0xBFCC7A244440D3DEuL, /* A00 = -2.224774678485620937884e-01 */
        0x3FFD4A0D48B2C5BAuL, /* A01 = +1.830579074838495134969e+00 */
        0xBFF33648C9558418uL, /* A02 = -1.200753008328552695616e+00 */
        0x3FDC0E614D2355F7uL, /* A03 = +4.383776906300619891077e-01 */
        0xBFB7E39E1FEF00BDuL, /* A04 = -9.331692008618382161433e-02 */
        0x3F865B82CB84163CuL, /* A05 = +1.091673072415765693988e-02 */
        0xBF41E092E6D27730uL, /* A06 = -5.455701153953326082435e-04 */
        0x0000000000000000uL,
        /* Pol19:  e in [3.0rr=5.97e-11, x0000; 3.50000]. */
        0xBF7A6B44CC927E68uL, /* A00 = -6.449955698513677726513e-03 */
        0x3FF66260CF7F9FF0uL, /* A01 = +1.399018106976651409923e+00 */
        0xBFEAE8D67C72682CuL, /* A02 = -8.409225874522738841677e-01 */
        0x3FD1CC4B5BC4E667uL, /* A03 = +2.780941387351049676191e-01 */
        0xBFAB2E5679E4B2F9uL, /* A04 = -5.308790433841709816010e-02 */
        0x3F769ED855C422D5uL, /* A05 = +5.522580202758494045956e-03 */
        0xBF2FF119B2A5A628uL, /* A06 = -2.436965817865105205647e-04 */
        0x0000000000000000uL,
        /* Pol20:  e in [3.5rr=2.52e-11, x0000; 4.00000]. */
        0x3FD047CB095C02B8uL, /* A00 = +2.543819037819434747405e-01 */
        0x3FEE67AD71046E63uL, /* A01 = +9.501559455270122134252e-01 */
        0xBFE098E15B696662uL, /* A02 = -5.186621461281897271789e-01 */
        0x3FC3C808CF5EB31EuL, /* A03 = +1.545420658634605870496e-01 */
        0xBF9B0B0F1186B840uL, /* A04 = -2.640937370750173762701e-02 */
        0x3F640A72C61BAFA7uL, /* A05 = +2.446388404555538669677e-03 */
        0xBF1917C7478DB907uL, /* A06 = -9.572175794201133230878e-05 */
        0x0000000000000000uL,
        /* Pol21:  e in [4.0rr=7.80e-10, x0000; 5.00000]. */
        0x3FE271672E80132AuL, /* A00 = +5.763431461533390365304e-01 */
        0x3FDE6BE7CCDC0D48uL, /* A01 = +4.753360272008717579695e-01 */
        0xBFCCF84A51DADF7DuL, /* A02 = -2.263272189791186994601e-01 */
        0x3FADE1FA24BE6877uL, /* A03 = +5.836469362766732310588e-02 */
        0xBF81905F3C595840uL, /* A04 = -8.576149014668854597687e-03 */
        0x3F464384C98AFBBAuL, /* A05 = +6.794355750442328690381e-04 */
        0xBEF7BBE3D7BE9F42uL, /* A06 = -2.263445411157463134756e-05 */
        0x0000000000000000uL,
        /* Pol22:  e in [5.0rr=1.07e-10, x0000; 6.00000]. */
        0x3FEACE37D3E55CD8uL, /* A00 = +8.376731051056607313399e-01 */
        0x3FC3E15729F5A9DDuL, /* A01 = +1.553143458252880526604e-01 */
        0xBFB009B0041AF713uL, /* A02 = -6.264782047489018956821e-02 */
        0x3F8BE0F5FF703E1CuL, /* A03 = +1.361267267023352073307e-02 */
        0xBF5B7E9264E296ADuL, /* A04 = -1.678126299830588795750e-03 */
        0x3F1D230586266C27uL, /* A05 = +1.111480828854127534669e-04 */
        0xBEC9E518A45C6684uL, /* A06 = -3.086913497163403010131e-06 */
        0x0000000000000000uL,
        /* Pol23:  e in [6.0rr=1.45e-11, x0000; 7.00000]. */
        0x3FEE48F6F3B20545uL, /* A00 = +9.464068183589594562832e-01 */
        0x3FA6CC28994BC90FuL, /* A01 = +4.452635642327795045814e-02 */
        0xBF8FD284F3D7EB09uL, /* A02 = -1.553825253630726661858e-02 */
        0x3F67DB44FD1B7665uL, /* A03 = +2.912173026614439056564e-03 */
        0xBF343E3A0589A1A1uL, /* A04 = -3.088847784398444777133e-04 */
        0x3EF26B760ED7403DuL, /* A05 = +1.756646172684804403067e-05 */
        0xBE9C10AA8D42975FuL, /* A06 = -4.182026149913673403710e-07 */
        0x0000000000000000uL,
        /* Pol24:  e in [7.0rr=1.97e-12, x0000; 8.00000]. */
        0x3FEF7E62D563A20CuL, /* A00 = +9.841779868848461454434e-01 */
        0x3F87BF0E50ADEE64uL, /* A01 = +1.159487899507567060242e-02 */
        0xBF6D2C2E8FB3321DuL, /* A02 = -3.561106624692790632164e-03 */
        0x3F433656B22F0998uL, /* A03 = +5.863116569957858500994e-04 */
        0xBF0C9908E7AA1CFFuL, /* A04 = -5.454595971792797914511e-05 */
        0x3EC6CB651F92C3CAuL, /* A05 = +2.717317553712206319044e-06 */
        0xBE6E63CDE066FB34uL, /* A06 = -5.660552564482285781102e-08 */
        0x0000000000000000uL,
        /* Pol25:  e in [8.0rr=1.39e-11, x0000; 10.00000]. */
        0x3FEFEC312764AA67uL, /* A00 = +9.975820321463516604510e-01 */
        0x3F58B44A9F798B0EuL, /* A01 = +1.507828592274665629319e-03 */
        0xBF39C22D2E0B62A3uL, /* A02 = -3.930435469050951968577e-04 */
        0x3F0CBA89167D11CAuL, /* A03 = +5.479555949120284475979e-05 */
        0xBED2113609B1CFE0uL, /* A04 = -4.307563496220412997337e-06 */
        0x3E884A862529B1B2uL, /* A05 = +1.809828683198534028568e-07 */
        0xBE2B4417D9A939C3uL, /* A06 = -3.174178875721672842563e-09 */
        0x0000000000000000uL,
        /* Pol26: */
        0x3ff0000000000000uL,
        0x0000000000000000uL,
        0x0000000000000000uL,
        0x0000000000000000uL,
        0x0000000000000000uL,
        0x0000000000000000uL,
        0x0000000000000000uL,
        0x0000000000000000uL,
    },
    0x80000000u, /* _sSignMask        */
    0x7fffffffu, /* _sAbsMask         */
    0x7fe00000u, /* _iExpMantMask     */
    0x7f000000u, /* _iExpMask         */
    0x3de00000u, /* _iMinIdxOfsMask   */
    0x03400000u, /* _iMaxIdxMask      */
};               /*sErf_Table*/
static __constant _iml_v2_sp_union_t __stanh_ha__imlsTanhTab[2] = {
    0x3F800000, /* ONES(0)       =  1.0    */
    0xBF800000, /* ONES(1)       = -1.0    */
};
__attribute__((always_inline)) inline int
__ocl_svml_internal_stanh_ha(float *a, float *r) {
  int nRet = 0;
  float sSign;
  sSign = ((__constant float *)__stanh_ha__imlsTanhTab)[(
      (((_iml_v2_sp_union_t *)&a[0])->hex[0] >> 31))];
  if ((((((_iml_v2_sp_union_t *)&a[0])->hex[0] >> 23) & 0xFF) != 0xFF) ||
      ((((_iml_v2_sp_union_t *)&a[0])->hex[0] & 0x007FFFFF) == 0)) {
    /* Here if a[0] is not NaN */
    r[0] = sSign;
  } else {
    /* Path 1) Here if a[0] = [S,Q]NaN */
    r[0] = a[0] + a[0];
  }
  return nRet;
}
float __ocl_svml_tanhf_ha(float x) {
  float r;
  unsigned int vm;
  float va1;
  float vr1;
  va1 = x;
  {
    float sPoly;
    float sP[8];
    float sC;
    float sAbsX;
    float sSignX;
    unsigned int iMaskedIn;
    unsigned int iSpecIndex;
    unsigned int iX;
    unsigned int iZero;
    unsigned int iMask1;
    unsigned int iMask2;
    unsigned int iIndex;
    float sSignMask;
    float sAbsMask;
    unsigned int iExpMantMask;
    unsigned int iExpMask;
    unsigned int iMinIdxOfsMask;
    unsigned int iMaxIdxMask;
    sSignMask = as_float(__ocl_svml_internal_stanh_ha_data._sSignMask);
    sAbsMask = as_float(__ocl_svml_internal_stanh_ha_data._sAbsMask);
    iExpMask = (__ocl_svml_internal_stanh_ha_data._iExpMask);
    iExpMantMask = (__ocl_svml_internal_stanh_ha_data._iExpMantMask_UISA);
    iMinIdxOfsMask = (__ocl_svml_internal_stanh_ha_data._iMinIdxOfsMask_UISA);
    iMaxIdxMask = (__ocl_svml_internal_stanh_ha_data._iMaxIdxMask_UISA);
    iZero = 0;
    // Absolute argument
    sAbsX = as_float((as_uint(va1) & as_uint(sAbsMask)));
    // Sign of argument
    sSignX = as_float((as_uint(va1) & as_uint(sSignMask)));
    // Argument as integer
    iX = as_uint(va1);
    iMaskedIn = (iX & iExpMantMask);
    iSpecIndex = ((unsigned int)(-(signed int)((signed int)iMaskedIn >
                                               (signed int)iExpMask)));
    vm = 0;
    vm = iSpecIndex;
    iIndex = (iMaskedIn - iMinIdxOfsMask);
    /* Put index between zero and _iMaxIdxMask */
    iIndex = (((int)(iIndex) > (int)(iZero)) ? iIndex : iZero);
    iIndex = (((int)(iIndex) < (int)(iMaxIdxMask)) ? iIndex : iMaxIdxMask);
    // Scale index
    iIndex = ((unsigned int)(iIndex) >> (19));
    // Load sC
    sC = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sC)))[iIndex >> 2]);
    // |x| = |x| - sC
    sAbsX = (sAbsX - sC);
    // Load sP[0] coefficient
    sP[0] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP0)))[iIndex >> 2]);
    // Load sP[2] coefficient
    sP[2] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP2)))[iIndex >> 2]);
    // Load sP[3] coefficient
    sP[3] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP3)))[iIndex >> 2]);
    // Load sP[4] coefficient
    sP[4] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP4)))[iIndex >> 2]);
    // Load sP[5] coefficient
    sP[5] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP5)))[iIndex >> 2]);
    // Load sP[6] coefficient
    sP[6] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP6)))[iIndex >> 2]);
    // Load sP[7] coefficient
    sP[7] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP7)))[iIndex >> 2]);
    // Compute polynomial
    sPoly = __spirv_ocl_fma(sP[7], sAbsX, sP[6]);
    sPoly = __spirv_ocl_fma(sPoly, sAbsX, sP[5]);
    sPoly = __spirv_ocl_fma(sPoly, sAbsX, sP[4]);
    sPoly = __spirv_ocl_fma(sPoly, sAbsX, sP[3]);
    /* sP[1] is the lower part of constant term sP[0] */
    sP[1] = as_float(((unsigned int *)((
        float *)(&__ocl_svml_internal_stanh_ha_data._sP1)))[iIndex >> 2]);
    sPoly = (sPoly * sAbsX);
    sPoly = __spirv_ocl_fma(sPoly, sAbsX, sP[1]);
    sPoly = __spirv_ocl_fma(sAbsX, sP[2], sPoly);
    sPoly = (sPoly + sP[0]);
    // Set result sign
    vr1 = as_float((as_uint(sPoly) | as_uint(sSignX)));
  }
  if (__builtin_expect((vm) != 0, 0)) {
    float __cout_a1;
    float __cout_r1;
    ((float *)&__cout_a1)[0] = va1;
    ((float *)&__cout_r1)[0] = vr1;
    __ocl_svml_internal_stanh_ha(&__cout_a1, &__cout_r1);
    vr1 = ((float *)&__cout_r1)[0];
  }
  r = vr1;
  return r;
}
