//
// Created by Charles on 2018/6/19.
//
// Most definitions copied from Tencent/RapidJSON
//

#ifndef JSONCPP_FLOATNUMUTILS_HPP
#define JSONCPP_FLOATNUMUTILS_HPP

#include <cstdint>

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#include <intrin.h>

#if defined(_M_AMD64)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_umul128)
#else
#pragma intrinsic(_BitScanReverse)
#endif

#endif

constexpr uint64_t const_exp(uint64_t b, int e)
{
    return e == 0 ? 1 : (e % 2 == 0 ? const_exp(b * b, e / 2) : const_exp(b * b, (e - 1) / 2) * b);
}

constexpr uint64_t make_uint64(uint32_t high, uint32_t low)
{
    return (static_cast<uint64_t>(high) << 32u) | static_cast<uint64_t>(low);
}

constexpr uint64_t DpSignMask = make_uint64(0x80000000, 0x00000000);        // NOLINT: compile time value
constexpr uint64_t DpExponentMask = make_uint64(0x7FF00000, 0x00000000);    // NOLINT: compile time value
constexpr uint64_t DpSignificandMask = make_uint64(0x000FFFFF, 0xFFFFFFFF); // NOLINT: compile time value
constexpr uint64_t DpHiddenBit = make_uint64(0x00100000, 0x00000000);       // NOLINT: compile time value

constexpr uint32_t DiySignificandSize = 64;
constexpr uint32_t DpSignificandSize = 52;
constexpr int32_t DpExponentBias = 0x3FF + DpSignificandSize;
// constexpr int32_t DpMaxExponent = 0x7FF - DpExponentBias;
constexpr int32_t DpMinExponent = -DpExponentBias;
constexpr int32_t DpDenormalExponent = -DpExponentBias + 1;

struct diy_fp
{
    constexpr diy_fp() : f(), e() { }

    constexpr diy_fp(uint64_t fp, int exp) : f(fp), e(exp) { }

    explicit diy_fp(double d)
    {
        union
        {
            double d;
            uint64_t u64;
        } u = {d};

        auto biased_e = static_cast<int>((u.u64 & DpExponentMask) >> DpSignificandSize);
        uint64_t significand = (u.u64 & DpSignificandMask);
        if (biased_e != 0) {
            f = significand + DpHiddenBit;
            e = biased_e - DpExponentBias;
        } else {
            f = significand;
            e = DpMinExponent + 1;
        }
    }

    diy_fp operator-(const diy_fp &rhs) const
    {
        return {f - rhs.f, e};
    }

    diy_fp operator*(const diy_fp &rhs) const
    {
#if defined(_MSC_VER) && defined(_M_AMD64)
        uint64_t h;
        uint64_t l = _umul128(f, rhs.f, &h);
        if (l & (uint64_t(1) << 63u)) { // rounding
            ++h;
        }
        return {h, e + rhs.e + 64};
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && defined(__x86_64__)
        __extension__ typedef unsigned __int128 uint128;
        uint128 p = static_cast<uint128>(f) * static_cast<uint128>(rhs.f);
        auto h = static_cast<uint64_t>(p >> 64u);
        auto l = static_cast<uint64_t>(p);
        if (l & (uint64_t(1) << 63u)) { // rounding
            ++h;
        }
        return {h, e + rhs.e + 64};
#else
        constexpr uint64_t M32 = 0xFFFFFFFF;
        const uint64_t a = f >> 32u;
        const uint64_t b = f & M32;
        const uint64_t c = rhs.f >> 32u;
        const uint64_t d = rhs.f & M32;
        const uint64_t ac = a * c;
        const uint64_t bc = b * c;
        const uint64_t ad = a * d;
        const uint64_t bd = b * d;
        uint64_t tmp = (bd >> 32u) + (ad & M32) + (bc & M32);
        tmp += 1u << 31u;  // mult_round
        return {ac + (ad >> 32u) + (bc >> 32u) + (tmp >> 32u), e + rhs.e + 64};
#endif
    }

    diy_fp normalize() const
    {
#if defined(_MSC_VER)
#if defined(_M_AMD64)
        unsigned long index;
        _BitScanReverse64(&index, f);
        return {f << (63 - index), e - (63 - index)};
#else
        unsigned long index;
        if (f >> 32u) {
            _BitScanReverse(&index, static_cast<uint32_t>(f >> 32u));
            return {f << (31 - index), e - static_cast<int>(31 - index)};
        } else {
            _BitScanReverse(&index, static_cast<uint32_t>(f));
            return {f << (63 - index), e - static_cast<int>(63 - index)};
        }
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
        int s = __builtin_clzll(f);
        return {f << s, e - s};
#else
        diy_fp res = *this;
        while (!(res.f & (static_cast<uint64_t>(1) << 63u))) {
            res.f <<= 1;
            --res.e;
        }
        return res;
#endif
    }

    diy_fp normalize_boundary() const
    {
        diy_fp res = *this;
        while (!(res.f & (DpHiddenBit << 1u))) {
            res.f <<= 1;
            --res.e;
        }
        res.f <<= (DiySignificandSize - DpSignificandSize - 2);
        res.e = res.e - (DiySignificandSize - DpSignificandSize - 2);
        return res;
    }

    void normalized_boundaries(diy_fp *minus, diy_fp *plus) const
    {
        diy_fp pl = diy_fp((f << 1u) + 1, e - 1).normalize_boundary();
        diy_fp mi = (f == DpHiddenBit) ? diy_fp((f << 2u) - 1, e - 2) : diy_fp((f << 1u) - 1, e - 1);
        mi.f <<= mi.e - pl.e;
        mi.e = pl.e;
        *plus = pl;
        *minus = mi;
    }

    double to_double() const
    {
        union
        {
            double d;
            uint64_t u64;
        } u;
        const uint64_t be = (e == DpDenormalExponent && (f & DpHiddenBit) == 0) ? 0 :
                            static_cast<uint64_t>(e + DpExponentBias);
        u.u64 = (f & DpSignificandMask) | (be << DpSignificandSize);
        return u.d;
    }

    uint64_t f;
    int e;
};

inline bool is_zero_double(uint64_t i) noexcept
{
    return (i & (DpExponentMask | DpSignificandMask)) == 0;
}

inline bool is_sign_double(uint64_t i) noexcept {
    return (i & DpSignMask) != 0;
}

// 10^-348, 10^-340, ..., 10^340
constexpr uint64_t cached_powers_f[] = {    // NOLINT: compile time value
        make_uint64(0xfa8fd5a0, 0x081c0288), make_uint64(0xbaaee17f, 0xa23ebf76),
        make_uint64(0x8b16fb20, 0x3055ac76), make_uint64(0xcf42894a, 0x5dce35ea),
        make_uint64(0x9a6bb0aa, 0x55653b2d), make_uint64(0xe61acf03, 0x3d1a45df),
        make_uint64(0xab70fe17, 0xc79ac6ca), make_uint64(0xff77b1fc, 0xbebcdc4f),
        make_uint64(0xbe5691ef, 0x416bd60c), make_uint64(0x8dd01fad, 0x907ffc3c),
        make_uint64(0xd3515c28, 0x31559a83), make_uint64(0x9d71ac8f, 0xada6c9b5),
        make_uint64(0xea9c2277, 0x23ee8bcb), make_uint64(0xaecc4991, 0x4078536d),
        make_uint64(0x823c1279, 0x5db6ce57), make_uint64(0xc2109436, 0x4dfb5637),
        make_uint64(0x9096ea6f, 0x3848984f), make_uint64(0xd77485cb, 0x25823ac7),
        make_uint64(0xa086cfcd, 0x97bf97f4), make_uint64(0xef340a98, 0x172aace5),
        make_uint64(0xb23867fb, 0x2a35b28e), make_uint64(0x84c8d4df, 0xd2c63f3b),
        make_uint64(0xc5dd4427, 0x1ad3cdba), make_uint64(0x936b9fce, 0xbb25c996),
        make_uint64(0xdbac6c24, 0x7d62a584), make_uint64(0xa3ab6658, 0x0d5fdaf6),
        make_uint64(0xf3e2f893, 0xdec3f126), make_uint64(0xb5b5ada8, 0xaaff80b8),
        make_uint64(0x87625f05, 0x6c7c4a8b), make_uint64(0xc9bcff60, 0x34c13053),
        make_uint64(0x964e858c, 0x91ba2655), make_uint64(0xdff97724, 0x70297ebd),
        make_uint64(0xa6dfbd9f, 0xb8e5b88f), make_uint64(0xf8a95fcf, 0x88747d94),
        make_uint64(0xb9447093, 0x8fa89bcf), make_uint64(0x8a08f0f8, 0xbf0f156b),
        make_uint64(0xcdb02555, 0x653131b6), make_uint64(0x993fe2c6, 0xd07b7fac),
        make_uint64(0xe45c10c4, 0x2a2b3b06), make_uint64(0xaa242499, 0x697392d3),
        make_uint64(0xfd87b5f2, 0x8300ca0e), make_uint64(0xbce50864, 0x92111aeb),
        make_uint64(0x8cbccc09, 0x6f5088cc), make_uint64(0xd1b71758, 0xe219652c),
        make_uint64(0x9c400000, 0x00000000), make_uint64(0xe8d4a510, 0x00000000),
        make_uint64(0xad78ebc5, 0xac620000), make_uint64(0x813f3978, 0xf8940984),
        make_uint64(0xc097ce7b, 0xc90715b3), make_uint64(0x8f7e32ce, 0x7bea5c70),
        make_uint64(0xd5d238a4, 0xabe98068), make_uint64(0x9f4f2726, 0x179a2245),
        make_uint64(0xed63a231, 0xd4c4fb27), make_uint64(0xb0de6538, 0x8cc8ada8),
        make_uint64(0x83c7088e, 0x1aab65db), make_uint64(0xc45d1df9, 0x42711d9a),
        make_uint64(0x924d692c, 0xa61be758), make_uint64(0xda01ee64, 0x1a708dea),
        make_uint64(0xa26da399, 0x9aef774a), make_uint64(0xf209787b, 0xb47d6b85),
        make_uint64(0xb454e4a1, 0x79dd1877), make_uint64(0x865b8692, 0x5b9bc5c2),
        make_uint64(0xc83553c5, 0xc8965d3d), make_uint64(0x952ab45c, 0xfa97a0b3),
        make_uint64(0xde469fbd, 0x99a05fe3), make_uint64(0xa59bc234, 0xdb398c25),
        make_uint64(0xf6c69a72, 0xa3989f5c), make_uint64(0xb7dcbf53, 0x54e9bece),
        make_uint64(0x88fcf317, 0xf22241e2), make_uint64(0xcc20ce9b, 0xd35c78a5),
        make_uint64(0x98165af3, 0x7b2153df), make_uint64(0xe2a0b5dc, 0x971f303a),
        make_uint64(0xa8d9d153, 0x5ce3b396), make_uint64(0xfb9b7cd9, 0xa4a7443c),
        make_uint64(0xbb764c4c, 0xa7a44410), make_uint64(0x8bab8eef, 0xb6409c1a),
        make_uint64(0xd01fef10, 0xa657842c), make_uint64(0x9b10a4e5, 0xe9913129),
        make_uint64(0xe7109bfb, 0xa19c0c9d), make_uint64(0xac2820d9, 0x623bf429),
        make_uint64(0x80444b5e, 0x7aa7cf85), make_uint64(0xbf21e440, 0x03acdd2d),
        make_uint64(0x8e679c2f, 0x5e44ff8f), make_uint64(0xd433179d, 0x9c8cb841),
        make_uint64(0x9e19db92, 0xb4e31ba9), make_uint64(0xeb96bf6e, 0xbadf77d9),
        make_uint64(0xaf87023b, 0x9bf0ee6b)
};

constexpr int16_t cached_powers_e[] = {
        -1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007, -980,
        -954, -927, -901, -874, -847, -821, -794, -768, -741, -715,
        -688, -661, -635, -608, -582, -555, -529, -502, -475, -449,
        -422, -396, -369, -343, -316, -289, -263, -236, -210, -183,
        -157, -130, -103, -77, -50, -24, 3, 30, 56, 83,
        109, 136, 162, 189, 216, 242, 269, 295, 322, 348,
        375, 402, 428, 455, 481, 508, 534, 561, 588, 614,
        641, 667, 694, 720, 747, 774, 800, 827, 853, 880,
        907, 933, 960, 986, 1013, 1039, 1066
};

constexpr diy_fp get_cached_power_by_index(size_t index)
{
    return {cached_powers_f[index], cached_powers_e[index]};
}

inline diy_fp get_cached_power(int e, int *K)
{
    //int k = static_cast<int>(ceil((-61 - e) * 0.30102999566398114)) + 374;
    double dk = (-61 - e) * 0.30102999566398114 + 347;  // dk must be positive, so can do ceiling in positive
    auto k = static_cast<int>(dk);
    if (dk - k > 0.0) {
        ++k;
    }

    auto index = (static_cast<unsigned>(k) >> 3u) + 1;
    *K = -(-348 + static_cast<int>(index << 3u));    // decimal exponent no need lookup table

    return get_cached_power_by_index(index);
}

inline diy_fp get_cached_power_10(int exp, int *out_exp)
{
    unsigned index = (static_cast<unsigned>(exp) + 348u) / 8u;
    *out_exp = -348 + static_cast<int>(index) * 8;
    return get_cached_power_by_index(index);
}

inline void grisu_round(char *buffer, int len, uint64_t delta, uint64_t rest, uint64_t ten_kappa, uint64_t wp_w)
{
    while (rest < wp_w && delta - rest >= ten_kappa &&
           (rest + ten_kappa < wp_w ||  // closer
            wp_w - rest > rest + ten_kappa - wp_w)) {
        --buffer[len - 1];
        rest += ten_kappa;
    }
}

inline int count_decimal_digit_32(uint32_t n)
{
    // Simple pure C++ implementation is faster than __builtin_clz version in this situation.
    if (n < 10) { return 1; }
    if (n < 100) { return 2; }
    if (n < 1000) { return 3; }
    if (n < 10000) { return 4; }
    if (n < 100000) { return 5; }
    if (n < 1000000) { return 6; }
    if (n < 10000000) { return 7; }
    if (n < 100000000) { return 8; }
    // Will not reach 10 digits in digit_gen()
    return 9;
}

inline void digit_gen(const diy_fp &W, const diy_fp &Mp, uint64_t delta, char *buffer, int *len, int *K)
{
    static constexpr uint32_t pow_10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

    auto minus_exp = static_cast<uint32_t>(-Mp.e);
    const diy_fp one(uint64_t(1) << minus_exp, Mp.e);
    const diy_fp wp_w = Mp - W;

    minus_exp = static_cast<uint32_t>(-one.e);
    auto p1 = static_cast<uint32_t>(Mp.f >> minus_exp);
    uint64_t p2 = Mp.f & (one.f - 1);
    int kappa = count_decimal_digit_32(p1); // kappa in [0, 9]
    *len = 0;

    while (kappa > 0) {
        uint32_t d = 0;
        switch (kappa) {
            case 9:
                d = p1 / 100000000;
                p1 %= 100000000;
                break;
            case 8:
                d = p1 / 10000000;
                p1 %= 10000000;
                break;
            case 7:
                d = p1 / 1000000;
                p1 %= 1000000;
                break;
            case 6:
                d = p1 / 100000;
                p1 %= 100000;
                break;
            case 5:
                d = p1 / 10000;
                p1 %= 10000;
                break;
            case 4:
                d = p1 / 1000;
                p1 %= 1000;
                break;
            case 3:
                d = p1 / 100;
                p1 %= 100;
                break;
            case 2:
                d = p1 / 10;
                p1 %= 10;
                break;
            case 1:
                d = p1;
                p1 = 0;
                break;
            default:
                break;
        }
        if (d || *len) {
            buffer[(*len)++] = '0' + static_cast<char>(d);
        }
        --kappa;

        uint64_t tmp = (static_cast<uint64_t>(p1) << minus_exp) + p2;
        if (tmp <= delta) {
            *K += kappa;
            grisu_round(buffer, *len, delta, tmp, static_cast<uint64_t>(pow_10[kappa]) << minus_exp, wp_w.f);
            return;
        }
    }

    // kappa = 0
    for (;;) {
        p2 *= 10;
        delta *= 10;
        auto d = static_cast<char>(p2 >> minus_exp);
        if (d || *len) {
            buffer[(*len)++] = '0' + d;
        }
        p2 &= one.f - 1;
        --kappa;

        if (p2 < delta) {
            *K += kappa;
            int index = -kappa;
            grisu_round(buffer, *len, delta, p2, one.f, wp_w.f * (index < 9 ? pow_10[index] : 0));
            return;
        }
    }
}

inline void grisu2(double value, char *buffer, int *length, int *K)
{
    const diy_fp v(value);
    diy_fp w_m, w_p;
    v.normalized_boundaries(&w_m, &w_p);

    const diy_fp c_mk = get_cached_power(w_p.e, K);
    const diy_fp W = v.normalize() * c_mk;
    diy_fp Wp = w_p * c_mk;
    diy_fp Wm = w_m * c_mk;
    ++Wm.f;
    --Wp.f;
    digit_gen(W, Wp, Wp.f - Wm.f, buffer, length, K);
}


// string to double

constexpr double e[] = { // 1e-0...1e308: 309 * 8 bytes = 2472 bytes
        1e+0,
        1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20,
        1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e+30, 1e+31, 1e+32, 1e+33, 1e+34, 1e+35, 1e+36, 1e+37, 1e+38, 1e+39, 1e+40,
        1e+41, 1e+42, 1e+43, 1e+44, 1e+45, 1e+46, 1e+47, 1e+48, 1e+49, 1e+50, 1e+51, 1e+52, 1e+53, 1e+54, 1e+55, 1e+56, 1e+57, 1e+58, 1e+59, 1e+60,
        1e+61, 1e+62, 1e+63, 1e+64, 1e+65, 1e+66, 1e+67, 1e+68, 1e+69, 1e+70, 1e+71, 1e+72, 1e+73, 1e+74, 1e+75, 1e+76, 1e+77, 1e+78, 1e+79, 1e+80,
        1e+81, 1e+82, 1e+83, 1e+84, 1e+85, 1e+86, 1e+87, 1e+88, 1e+89, 1e+90, 1e+91, 1e+92, 1e+93, 1e+94, 1e+95, 1e+96, 1e+97, 1e+98, 1e+99, 1e+100,
        1e+101, 1e+102, 1e+103, 1e+104, 1e+105, 1e+106, 1e+107, 1e+108, 1e+109, 1e+110, 1e+111, 1e+112, 1e+113, 1e+114, 1e+115, 1e+116, 1e+117, 1e+118, 1e+119, 1e+120,
        1e+121, 1e+122, 1e+123, 1e+124, 1e+125, 1e+126, 1e+127, 1e+128, 1e+129, 1e+130, 1e+131, 1e+132, 1e+133, 1e+134, 1e+135, 1e+136, 1e+137, 1e+138, 1e+139, 1e+140,
        1e+141, 1e+142, 1e+143, 1e+144, 1e+145, 1e+146, 1e+147, 1e+148, 1e+149, 1e+150, 1e+151, 1e+152, 1e+153, 1e+154, 1e+155, 1e+156, 1e+157, 1e+158, 1e+159, 1e+160,
        1e+161, 1e+162, 1e+163, 1e+164, 1e+165, 1e+166, 1e+167, 1e+168, 1e+169, 1e+170, 1e+171, 1e+172, 1e+173, 1e+174, 1e+175, 1e+176, 1e+177, 1e+178, 1e+179, 1e+180,
        1e+181, 1e+182, 1e+183, 1e+184, 1e+185, 1e+186, 1e+187, 1e+188, 1e+189, 1e+190, 1e+191, 1e+192, 1e+193, 1e+194, 1e+195, 1e+196, 1e+197, 1e+198, 1e+199, 1e+200,
        1e+201, 1e+202, 1e+203, 1e+204, 1e+205, 1e+206, 1e+207, 1e+208, 1e+209, 1e+210, 1e+211, 1e+212, 1e+213, 1e+214, 1e+215, 1e+216, 1e+217, 1e+218, 1e+219, 1e+220,
        1e+221, 1e+222, 1e+223, 1e+224, 1e+225, 1e+226, 1e+227, 1e+228, 1e+229, 1e+230, 1e+231, 1e+232, 1e+233, 1e+234, 1e+235, 1e+236, 1e+237, 1e+238, 1e+239, 1e+240,
        1e+241, 1e+242, 1e+243, 1e+244, 1e+245, 1e+246, 1e+247, 1e+248, 1e+249, 1e+250, 1e+251, 1e+252, 1e+253, 1e+254, 1e+255, 1e+256, 1e+257, 1e+258, 1e+259, 1e+260,
        1e+261, 1e+262, 1e+263, 1e+264, 1e+265, 1e+266, 1e+267, 1e+268, 1e+269, 1e+270, 1e+271, 1e+272, 1e+273, 1e+274, 1e+275, 1e+276, 1e+277, 1e+278, 1e+279, 1e+280,
        1e+281, 1e+282, 1e+283, 1e+284, 1e+285, 1e+286, 1e+287, 1e+288, 1e+289, 1e+290, 1e+291, 1e+292, 1e+293, 1e+294, 1e+295, 1e+296, 1e+297, 1e+298, 1e+299, 1e+300,
        1e+301, 1e+302, 1e+303, 1e+304, 1e+305, 1e+306, 1e+307, 1e+308
};

constexpr double pow10(int32_t n)
{
    // assert(n >= 0 && n <= 308);
    return e[n];
}

static double strtod_normal_precision(double d, int32_t p)
{
    if (p < -308 - 308) {
        return 0.0;
    }
    if (p < -308) {
        d = d / pow10(308);
        return d / pow10(-(p + 308));
    }
    if (p < 0) {
        return d / pow10(-p);
    }
    return d * pow10(p);
}

#endif //JSONCPP_FLOATNUMUTILS_HPP
