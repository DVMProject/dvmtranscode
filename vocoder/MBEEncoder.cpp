/**
* Digital Voice Modem - Transcode Software
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / Transcode Software
*
*/
/*
 * AMBE halfrate encoder - Copyright 2016 Max H. Parke KA1RBI
 *
 * This file is part of OP25 and part of GNU Radio
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#define _USE_MATH_DEFINES
#include <math.h>

#include "Defines.h"
#include "edac/AMBEFEC.h"
#include "edac/Golay24128.h"
#include "vocoder/MBEEncoder.h"
#include "vocoder/ambe3600x2450_const.h"
#include "vocoder/ambe3600x2400_const.h"

#define _LOG_NO_INCLUDE_INITIALIZERS
#include "Log.h"

using namespace edac;
using namespace vocoder;

#ifdef _MSC_VER
#pragma warning(disable: 4244)
#endif

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

static const short b0_lookup[] = {
    0, 0, 0, 1, 1, 2, 2, 2,
    3, 3, 4, 4, 4, 5, 5, 5,
    6, 6, 7, 7, 7, 8, 8, 8,
    9, 9, 9, 10, 10, 11, 11, 11,
    12, 12, 12, 13, 13, 13, 14, 14,
    14, 15, 15, 15, 16, 16, 16, 17,
    17, 17, 17, 18, 18, 18, 19, 19,
    19, 20, 20, 20, 21, 21, 21, 21,
    22, 22, 22, 23, 23, 23, 24, 24,
    24, 24, 25, 25, 25, 25, 26, 26,
    26, 27, 27, 27, 27, 28, 28, 28,
    29, 29, 29, 29, 30, 30, 30, 30,
    31, 31, 31, 31, 31, 32, 32, 32,
    32, 33, 33, 33, 33, 34, 34, 34,
    34, 35, 35, 35, 35, 36, 36, 36,
    36, 37, 37, 37, 37, 38, 38, 38,
    38, 38, 39, 39, 39, 39, 40, 40,
    40, 40, 40, 41, 41, 41, 41, 42,
    42, 42, 42, 42, 43, 43, 43, 43,
    43, 44, 44, 44, 44, 45, 45, 45,
    45, 45, 46, 46, 46, 46, 46, 47,
    47, 47, 47, 47, 48, 48, 48, 48,
    48, 49, 49, 49, 49, 49, 49, 50,
    50, 50, 50, 50, 51, 51, 51, 51,
    51, 52, 52, 52, 52, 52, 52, 53,
    53, 53, 53, 53, 54, 54, 54, 54,
    54, 54, 55, 55, 55, 55, 55, 56,
    56, 56, 56, 56, 56, 57, 57, 57,
    57, 57, 57, 58, 58, 58, 58, 58,
    58, 59, 59, 59, 59, 59, 59, 60,
    60, 60, 60, 60, 60, 61, 61, 61,
    61, 61, 61, 62, 62, 62, 62, 62,
    62, 63, 63, 63, 63, 63, 63, 63,
    64, 64, 64, 64, 64, 64, 65, 65,
    65, 65, 65, 65, 65, 66, 66, 66,
    66, 66, 66, 67, 67, 67, 67, 67,
    67, 67, 68, 68, 68, 68, 68, 68,
    68, 69, 69, 69, 69, 69, 69, 69,
    70, 70, 70, 70, 70, 70, 70, 71,
    71, 71, 71, 71, 71, 71, 72, 72,
    72, 72, 72, 72, 72, 73, 73, 73,
    73, 73, 73, 73, 73, 74, 74, 74,
    74, 74, 74, 74, 75, 75, 75, 75,
    75, 75, 75, 75, 76, 76, 76, 76,
    76, 76, 76, 76, 77, 77, 77, 77,
    77, 77, 77, 77, 77, 78, 78, 78,
    78, 78, 78, 78, 78, 79, 79, 79,
    79, 79, 79, 79, 79, 80, 80, 80,
    80, 80, 80, 80, 80, 81, 81, 81,
    81, 81, 81, 81, 81, 81, 82, 82,
    82, 82, 82, 82, 82, 82, 83, 83,
    83, 83, 83, 83, 83, 83, 83, 84,
    84, 84, 84, 84, 84, 84, 84, 84,
    85, 85, 85, 85, 85, 85, 85, 85,
    85, 86, 86, 86, 86, 86, 86, 86,
    86, 86, 87, 87, 87, 87, 87, 87,
    87, 87, 87, 88, 88, 88, 88, 88,
    88, 88, 88, 88, 89, 89, 89, 89,
    89, 89, 89, 89, 89, 89, 90, 90,
    90, 90, 90, 90, 90, 90, 90, 90,
    91, 91, 91, 91, 91, 91, 91, 91,
    91, 92, 92, 92, 92, 92, 92, 92,
    92, 92, 92, 93, 93, 93, 93, 93,
    93, 93, 93, 93, 93, 94, 94, 94,
    94, 94, 94, 94, 94, 94, 94, 94,
    95, 95, 95, 95, 95, 95, 95, 95,
    95, 95, 96, 96, 96, 96, 96, 96,
    96, 96, 96, 96, 96, 97, 97, 97,
    97, 97, 97, 97, 97, 97, 97, 98,
    98, 98, 98, 98, 98, 98, 98, 98,
    98, 98, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 101, 101, 101, 101, 101, 101, 101,
    101, 101, 101, 101, 102, 102, 102, 102,
    102, 102, 102, 102, 102, 102, 102, 102,
    103, 103, 103, 103, 103, 103, 103, 103,
    103, 103, 103, 103, 104, 104, 104, 104,
    104, 104, 104, 104, 104, 104, 104, 104,
    105, 105, 105, 105, 105, 105, 105, 105,
    105, 105, 105, 105, 106, 106, 106, 106,
    106, 106, 106, 106, 106, 106, 106, 106,
    107, 107, 107, 107, 107, 107, 107, 107,
    107, 107, 107, 107, 107, 108, 108, 108,
    108, 108, 108, 108, 108, 108, 108, 108,
    108, 109, 109, 109, 109, 109, 109, 109,
    109, 109, 109, 109, 109, 109, 110, 110,
    110, 110, 110, 110, 110, 110, 110, 110,
    110, 110, 110, 111, 111, 111, 111, 111,
    111, 111, 111, 111, 111, 111, 111, 111,
    112, 112, 112, 112, 112, 112, 112, 112,
    112, 112, 112, 112, 112, 112, 113, 113,
    113, 113, 113, 113, 113, 113, 113, 113,
    113, 113, 113, 113, 114, 114, 114, 114,
    114, 114, 114, 114, 114, 114, 114, 114,
    114, 115, 115, 115, 115, 115, 115, 115,
    115, 115, 115, 115, 115, 115, 115, 116,
    116, 116, 116, 116, 116, 116, 116, 116,
    116, 116, 116, 116, 116, 116, 117, 117,
    117, 117, 117, 117, 117, 117, 117, 117,
    117, 117, 117, 117, 118, 118, 118, 118,
    118, 118, 118, 118, 118, 118, 118, 118,
    118, 118, 118, 119, 119, 119, 119, 119,
    119, 119, 119
};

// ---------------------------------------------------------------------------
//  Global Functions
// ---------------------------------------------------------------------------

/// <summary>
/// 
/// </summary>
/// <param name="dest"></param>
/// <param name="src"></param>
/// <param name="count"></param>
static void dump_i(uint8_t dest[], int src, int count)
{
    for (int i = 0; i < count; i++) {
        dest[i] = src & 1;
        src = src >> 1;
    }
}

/// <summary>
/// 
/// </summary>
/// <param name="reg"></param>
/// <param name="val"></param>
/// <param name="len"></param>
static inline void store_reg(int reg, uint8_t val[], int len)
{
    for (int i = 0; i < len; i++) {
        val[i] = (reg >> (len - 1 - i)) & 1;
    }
}

/// <summary>
/// 
/// </summary>
/// <param name="val"></param>
/// <param name="len"></param>
/// <returns></returns>
static inline int load_reg(const uint8_t val[], int len)
{
    int acc = 0;
    for (int i = 0; i < len; i++) {
        acc = (acc << 1) + (val[i] & 1);
    }
    return acc;
}

/// <summary>
/// 
/// </summary>
/// <param name="b0"></param>
/// <returns></returns>
static inline float make_f0(int b0)
{
    return (powf(2, (-4.311767578125 - (2.1336e-2 * ((float)b0 + 0.5)))));
}

/// <summary>
/// 
/// </summary>
/// <param name="imbe_param"></param>
/// <param name="b"></param>
/// <param name="cur_mp"></param>
/// <param name="prev_mp"></param>
/// <param name="gain_adjust"></param>
static void encode_ambe(const IMBE_PARAM* imbe_param, int b[], mbe_parms* cur_mp, mbe_parms* prev_mp, float gain_adjust)
{
    static const float SQRT_2 = sqrtf(2.0);
    static const int b0_lmax = sizeof(b0_lookup) / sizeof(b0_lookup[0]);
    // int b[9];

    // ref_pitch is Q8_8 in range 19.875 - 123.125
    int b0_i = (imbe_param->ref_pitch >> 5) - 159;
    if (b0_i < 0 || b0_i > b0_lmax) {
        LogError(LOG_HOST, "MBE: encode error b0_i %d", b0_i);
        return;
    }

    b[0] = b0_lookup[b0_i];
    int L = (int)AmbeLtable[b[0]];

    // adjust b0 until L agrees
    while (L != imbe_param->num_harms) {
        if (L < imbe_param->num_harms)
            b0_i++;
        else if (L > imbe_param->num_harms)
            b0_i--;
        if (b0_i < 0 || b0_i > b0_lmax) {
            fprintf(stderr, "encode error2 b0_i %d\n", b0_i);
            return;
        }
        b[0] = b0_lookup[b0_i];
        L = (int)AmbeLtable[b[0]];
    }

    float m_float2[NUM_HARMS_MAX];
    for (int l = 1; l <= L; l++) {
        m_float2[l - 1] = (float)imbe_param->sa[l - 1];
        m_float2[l - 1] = m_float2[l - 1] * m_float2[l - 1];
    }

    float en_min = 0;
    b[1] = 0;
    int vuv_max = 17;
    for (int n = 0; n < vuv_max; n++) {
        float En = 0;
        for (int l = 1; l <= L; l++) {
            int jl = (int)((float)l * (float)16.0 * AmbeW0table[b[0]]);
            int kl = 12;
            if (l <= 36)
                kl = (l + 2) / 3;

            if (imbe_param->v_uv_dsn[(kl - 1) * 3] != AmbeVuv[n][jl])
                En += m_float2[l - 1];
        }

        if (n == 0)
            en_min = En;
        else if (En < en_min) {
            b[1] = n;
            en_min = En;
        }
    }

    // log spectral amplitudes
    float num_harms_f = (float)imbe_param->num_harms;
    float log_l_2 = 0.5 * log2f(num_harms_f);	// fixme: table lookup
    float log_l_w0 = 0.5 * log2f(num_harms_f * AmbeW0table[b[0]] * 2.0 * M_PI) + 2.289;
    float lsa[NUM_HARMS_MAX];
    float lsa_sum = 0.0;

    for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
        float sa = (float)imbe_param->sa[i1];
        if (sa < 1) sa = 1.0;
        if (imbe_param->v_uv_dsn[i1])
            lsa[i1] = log_l_2 + log2f(sa);
        else
            lsa[i1] = log_l_w0 + log2f(sa);
        lsa_sum += lsa[i1];
    }

    float gain = lsa_sum / num_harms_f;
    float diff_gain = gain - 0.5 * prev_mp->gamma;

    diff_gain -= gain_adjust;

    float error;
    int error_index;
    int max_dg = 32;
    for (int i1 = 0; i1 < max_dg; i1++) {
        float diff = fabsf(diff_gain - AmbeDg[i1]);
        if ((i1 == 0) || (diff < error)) {
            error = diff;
            error_index = i1;
        }
    }

    b[2] = error_index;

    // prediction residuals
    float l_prev_l = (float)(prev_mp->L) / num_harms_f;
    float tmp_s = 0.0;
    prev_mp->log2Ml[0] = prev_mp->log2Ml[1];
    for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
        float kl = l_prev_l * (float)(i1 + 1);
        int kl_floor = (int)kl;
        float kl_frac = kl - kl_floor;
        tmp_s += (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor + 0] + kl_frac * prev_mp->log2Ml[kl_floor + 1 + 0];
    }

    float T[NUM_HARMS_MAX];
    for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
        float kl = l_prev_l * (float)(i1 + 1);
        int kl_floor = (int)kl;
        float kl_frac = kl - kl_floor;
        T[i1] = lsa[i1] - 0.65 * (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor + 0]	\
            - 0.65 * kl_frac * prev_mp->log2Ml[kl_floor + 1 + 0];
    }

    // DCT
    const int* J = AmbeLmprbl[imbe_param->num_harms];
    float* c[4];
    int acc = 0;
    for (int i = 0; i < 4; i++) {
        c[i] = &T[acc];
        acc += J[i];
    }

    float C[4][17];
    for (int i = 1; i <= 4; i++) {
        for (int k = 1; k <= J[i - 1]; k++) {
            float s = 0.0;
            for (int j = 1; j <= J[i - 1]; j++) {
                //fixme: lut?
                s += (c[i - 1][j - 1] * cosf((M_PI * (((float)k) - 1.0) * (((float)j) - 0.5)) / (float)J[i - 1]));
            }
            C[i - 1][k - 1] = s / (float)J[i - 1];
        }
    }

    float R[8];
    R[0] = C[0][0] + SQRT_2 * C[0][1];
    R[1] = C[0][0] - SQRT_2 * C[0][1];
    R[2] = C[1][0] + SQRT_2 * C[1][1];
    R[3] = C[1][0] - SQRT_2 * C[1][1];
    R[4] = C[2][0] + SQRT_2 * C[2][1];
    R[5] = C[2][0] - SQRT_2 * C[2][1];
    R[6] = C[3][0] + SQRT_2 * C[3][1];
    R[7] = C[3][0] - SQRT_2 * C[3][1];

    // encode PRBA
    float G[8];
    for (int m = 1; m <= 8; m++) {
        G[m - 1] = 0.0;
        for (int i = 1; i <= 8; i++) {
            //fixme: lut?
            G[m - 1] += (R[i - 1] * cosf((M_PI * (((float)m) - 1.0) * (((float)i) - 0.5)) / 8.0));
        }
        G[m - 1] /= 8.0;
    }

    for (int i = 0; i < 512; i++) {
        float err = 0.0;
        float diff;

        diff = G[1] - AmbePRBA24[i][0];
        err += (diff * diff);
        diff = G[2] - AmbePRBA24[i][1];
        err += (diff * diff);
        diff = G[3] - AmbePRBA24[i][2];
        err += (diff * diff);

        if (i == 0 || err < error) {
            error = err;
            error_index = i;
        }
    }

    b[3] = error_index;

    // PRBA58
    for (int i = 0; i < 128; i++) {
        float err = 0.0;
        float diff;

        diff = G[4] - AmbePRBA58[i][0];
        err += (diff * diff);
        diff = G[5] - AmbePRBA58[i][1];
        err += (diff * diff);
        diff = G[6] - AmbePRBA58[i][2];
        err += (diff * diff);
        diff = G[7] - AmbePRBA58[i][3];
        err += (diff * diff);

        if (i == 0 || err < error) {
            error = err;
            error_index = i;
        }
    }

    b[4] = error_index;

    // higher order coeffs b5
    int ii = 1;
    if (J[ii - 1] <= 2) {
        b[4 + ii] = 0.0;
    }
    else {
        int max_5 = 32;
        for (int n = 0; n < max_5; n++) {
            float err = 0.0;
            float diff;
            for (int j = 1; j <= J[ii - 1] - 2 && j <= 4; j++) {
                diff = AmbeHOCb5[n][j - 1] - C[ii - 1][j + 2 - 1];
                err += (diff * diff);
            }
            if (n == 0 || err < error) {
                error = err;
                error_index = n;
            }
        }
        b[4 + ii] = error_index;
    }

    // higher order coeffs b6
    ii = 2;
    if (J[ii - 1] <= 2) {
        b[4 + ii] = 0.0;
    }
    else {
        for (int n = 0; n < 16; n++) {
            float err = 0.0;
            float diff;
            for (int j = 1; j <= J[ii - 1] - 2 && j <= 4; j++) {
                diff = AmbeHOCb6[n][j - 1] - C[ii - 1][j + 2 - 1];
                err += (diff * diff);
            }
            if (n == 0 || err < error) {
                error = err;
                error_index = n;
            }
        }
        b[4 + ii] = error_index;
    }

    // higher order coeffs b7
    ii = 3;
    if (J[ii - 1] <= 2) {
        b[4 + ii] = 0.0;
    }
    else {
        for (int n = 0; n < 16; n++) {
            float err = 0.0;
            float diff;
            for (int j = 1; j <= J[ii - 1] - 2 && j <= 4; j++) {
                diff = AmbeHOCb7[n][j - 1] - C[ii - 1][j + 2 - 1];
                err += (diff * diff);
            }
            if (n == 0 || err < error) {
                error = err;
                error_index = n;
            }
        }
        b[4 + ii] = error_index;
    }

    // higher order coeffs b8
    ii = 4;
    if (J[ii - 1] <= 2) {
        b[4 + ii] = 0.0;
    }
    else {
        int max_8 = 8;
        for (int n = 0; n < max_8; n++) {
            float err = 0.0;
            float diff;
            for (int j = 1; j <= J[ii - 1] - 2 && j <= 4; j++) {
                diff = AmbeHOCb8[n][j - 1] - C[ii - 1][j + 2 - 1];
                err += (diff * diff);
            }
            if (n == 0 || err < error) {
                error = err;
                error_index = n;
            }
        }
        b[4 + ii] = error_index;
    }

    mbe_dequantizeAmbe2250Parms(cur_mp, prev_mp, b);
    mbe_moveMbeParms(cur_mp, prev_mp);
}

/// <summary>
/// 
/// </summary>
/// <param name="outp"></param>
/// <param name="b"></param>
static void encode_49bit(uint8_t outp[49], const int b[9])
{
    outp[0] = (b[0] >> 6) & 1;
    outp[1] = (b[0] >> 5) & 1;
    outp[2] = (b[0] >> 4) & 1;
    outp[3] = (b[0] >> 3) & 1;
    outp[4] = (b[1] >> 4) & 1;
    outp[5] = (b[1] >> 3) & 1;
    outp[6] = (b[1] >> 2) & 1;
    outp[7] = (b[1] >> 1) & 1;
    outp[8] = (b[2] >> 4) & 1;
    outp[9] = (b[2] >> 3) & 1;
    outp[10] = (b[2] >> 2) & 1;
    outp[11] = (b[2] >> 1) & 1;
    outp[12] = (b[3] >> 8) & 1;
    outp[13] = (b[3] >> 7) & 1;
    outp[14] = (b[3] >> 6) & 1;
    outp[15] = (b[3] >> 5) & 1;
    outp[16] = (b[3] >> 4) & 1;
    outp[17] = (b[3] >> 3) & 1;
    outp[18] = (b[3] >> 2) & 1;
    outp[19] = (b[3] >> 1) & 1;
    outp[20] = (b[4] >> 6) & 1;
    outp[21] = (b[4] >> 5) & 1;
    outp[22] = (b[4] >> 4) & 1;
    outp[23] = (b[4] >> 3) & 1;
    outp[24] = (b[5] >> 4) & 1;
    outp[25] = (b[5] >> 3) & 1;
    outp[26] = (b[5] >> 2) & 1;
    outp[27] = (b[5] >> 1) & 1;
    outp[28] = (b[6] >> 3) & 1;
    outp[29] = (b[6] >> 2) & 1;
    outp[30] = (b[6] >> 1) & 1;
    outp[31] = (b[7] >> 3) & 1;
    outp[32] = (b[7] >> 2) & 1;
    outp[33] = (b[7] >> 1) & 1;
    outp[34] = (b[8] >> 2) & 1;
    outp[35] = b[1] & 1;
    outp[36] = b[2] & 1;
    outp[37] = (b[0] >> 2) & 1;
    outp[38] = (b[0] >> 1) & 1;
    outp[39] = b[0] & 1;
    outp[40] = b[3] & 1;
    outp[41] = (b[4] >> 2) & 1;
    outp[42] = (b[4] >> 1) & 1;
    outp[43] = b[4] & 1;
    outp[44] = b[5] & 1;
    outp[45] = b[6] & 1;
    outp[46] = b[7] & 1;
    outp[47] = (b[8] >> 1) & 1;
    outp[48] = b[8] & 1;
}

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the MBEDecoder class.
/// </summary>
MBEEncoder::MBEEncoder() :
    m_49bitMode(false),
    m_dmrMode(false),
    m_88bitMode(false),
    m_gainAdjust(0)
{
    mbe_parms enh_mp;
    mbe_initMbeParms(&m_curMBEParms, &m_prevMBEParms, &enh_mp);
}

/// <summary>
/// 
/// </summary>
void MBEEncoder::set49bitMode()
{
    m_88bitMode = false;
    m_49bitMode = true;
}

/// <summary>
/// 
/// </summary>
void MBEEncoder::setDmrMode()
{
    m_dmrMode = true;
}

/// <summary>
/// 
/// </summary>
void MBEEncoder::set88bitMode()
{
    m_49bitMode = false;
    m_88bitMode = true;
}

// given a buffer of 160 audio samples (S16_LE),
// generate 72-bit ambe codeword (as 36 dibits in codeword[])
// (as 72 bits in codeword[] if in dstar mode)
// or 49-bit output codeword (if set_49bit_mode() has been called)
void MBEEncoder::encode(int16_t samples[], uint8_t codeword[])
{
    int b[9];
    unsigned char dmr[9];
    int16_t frame_vector[8];	// result ignored
    uint8_t ambe_bytes[9];
    memset(ambe_bytes, 0, 9);
    memset(dmr, 0, 9);
    //memset (b, 0, 9);

    // first do speech analysis to generate mbe model parameters
    m_vocoder.imbe_encode(frame_vector, samples);
    if (m_88bitMode) {
        //vocoder.set_gain_adjust(1.0);
        unsigned int offset = 0U;
        int16_t mask = 0x0800;
        for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[0U] & mask) != 0);

        mask = 0x0800;
        for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[1U] & mask) != 0);

        mask = 0x0800;
        for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[2U] & mask) != 0);

        mask = 0x0800;
        for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[3U] & mask) != 0);

        mask = 0x0400;
        for (unsigned int i = 0U; i < 11U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[4U] & mask) != 0);

        mask = 0x0400;
        for (unsigned int i = 0U; i < 11U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[5U] & mask) != 0);

        mask = 0x0400;
        for (unsigned int i = 0U; i < 11U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[6U] & mask) != 0);

        mask = 0x0040;
        for (unsigned int i = 0U; i < 7U; i++, mask >>= 1, offset++)
            WRITE_BIT(codeword, offset, (frame_vector[7U] & mask) != 0);
        return;
    }

    // halfrate audio encoding - output rate is 2450 (49 bits)
    encode_ambe(m_vocoder.param(), b, &m_curMBEParms, &m_prevMBEParms, m_gainAdjust);

    if (m_49bitMode) {
        encode_49bit(codeword, b);
    }
    else if (m_dmrMode) {
        encode_49bit(codeword, b);
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 8; ++j) {
                //ambe_bytes[i] |= (ambe_frame[((8-i)*8)+(7-j)] << (7-j));
                ambe_bytes[i] |= (codeword[(i * 8) + j] << (7 - j));
            }
        }

        encodeDmr(ambe_bytes, dmr);
        memcpy(codeword, dmr, 9);
        // add FEC and interleaving - output rate is 3600 (72 bits)
        //encode_vcw(codeword, b);
    }

    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 8; ++j) {
            //ambe_bytes[i] |= (ambe_frame[((8-i)*8)+(7-j)] << (7-j));
            ambe_bytes[i] |= (codeword[(i * 8) + j] << (7 - j));
        }
    }
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// 
/// </summary>
/// <param name="in"></param>
/// <param name="out"></param>
void MBEEncoder::encodeDmr(const unsigned char* in, unsigned char* out)
{
    unsigned int aOrig = 0U;
    unsigned int bOrig = 0U;
    unsigned int cOrig = 0U;

    unsigned int MASK = 0x000800U;
    for (unsigned int i = 0U; i < 12U; i++, MASK >>= 1) {
        unsigned int n1 = i;
        unsigned int n2 = i + 12U;
        if (READ_BIT(in, n1))
            aOrig |= MASK;
        if (READ_BIT(in, n2))
            bOrig |= MASK;
    }

    MASK = 0x1000000U;
    for (unsigned int i = 0U; i < 25U; i++, MASK >>= 1) {
        unsigned int n = i + 24U;
        if (READ_BIT(in, n))
            cOrig |= MASK;
    }

    unsigned int a = Golay24128::encode24128(aOrig);

    // The PRNG
    unsigned int p = PRNG_TABLE[aOrig] >> 1;

    unsigned int b = Golay24128::encode23127(bOrig) >> 1;
    b ^= p;

    MASK = 0x800000U;
    for (unsigned int i = 0U; i < 24U; i++, MASK >>= 1) {
        unsigned int aPos = DMR_A_TABLE[i];
        WRITE_BIT(out, aPos, a & MASK);
    }

    MASK = 0x400000U;
    for (unsigned int i = 0U; i < 23U; i++, MASK >>= 1) {
        unsigned int bPos = DMR_B_TABLE[i];
        WRITE_BIT(out, bPos, b & MASK);
    }

    MASK = 0x1000000U;
    for (unsigned int i = 0U; i < 25U; i++, MASK >>= 1) {
        unsigned int cPos = DMR_C_TABLE[i];
        WRITE_BIT(out, cPos, cOrig & MASK);
    }
}

/// <summary>
/// 
/// </summary>
/// <param name="vf"></param>
/// <param name="b"></param>
void MBEEncoder::encodeVcw(uint8_t vf[], const int* b)
{
    uint32_t c0, c1, c2, c3;
    int u0, u1, u2, u3;

    u0 = \
        ((b[0] & 0x78) << 5) | \
        ((b[1] & 0x1e) << 3) | \
        ((b[2] & 0x1e) >> 1);
    u1 = \
        ((b[3] & 0x1fe) << 3) | \
        ((b[4] & 0x78) >> 3);
    u2 = \
        ((b[5] & 0x1e) << 6) | \
        ((b[6] & 0xe) << 3) | \
        ((b[7] & 0xe)) | \
        ((b[8] & 0x4) >> 2);
    u3 = \
        ((b[1] & 0x1) << 13) | \
        ((b[2] & 0x1) << 12) | \
        ((b[0] & 0x7) << 9) | \
        ((b[3] & 0x1) << 8) | \
        ((b[4] & 0x7) << 5) | \
        ((b[5] & 0x1) << 4) | \
        ((b[6] & 0x1) << 3) | \
        ((b[7] & 0x1) << 2) | \
        ((b[8] & 0x3));

    int m1 = PRNG_TABLE[u0] >> 1;
    c0 = Golay24128::encode24128(u0);
    c1 = Golay24128::encode23127(u1) ^ m1;
    c2 = u2;
    c3 = u3;

    interleaveVcw(vf, c0, c1, c2, c3);
}

/// <summary>
/// 
/// </summary>
/// <param name="_vf"></param>
/// <param name="_c0"></param>
/// <param name="_c1"></param>
/// <param name="_c2"></param>
/// <param name="_c3"></param>
void MBEEncoder::interleaveVcw(uint8_t _vf[], int _c0, int _c1, int _c2, int _c3)
{
    uint8_t vf[72];
    uint8_t c0[24];
    uint8_t c1[23];
    uint8_t c2[11];
    uint8_t c3[14];

    dump_i(c0, _c0, 24);
    dump_i(c1, _c1, 23);
    dump_i(c2, _c2, 11);
    dump_i(c3, _c3, 14);

    vf[0] = c0[23];
    vf[1] = c0[5];
    vf[2] = c1[10];
    vf[3] = c2[3];
    vf[4] = c0[22];
    vf[5] = c0[4];
    vf[6] = c1[9];
    vf[7] = c2[2];
    vf[8] = c0[21];
    vf[9] = c0[3];
    vf[10] = c1[8];
    vf[11] = c2[1];
    vf[12] = c0[20];
    vf[13] = c0[2];
    vf[14] = c1[7];
    vf[15] = c2[0];
    vf[16] = c0[19];
    vf[17] = c0[1];
    vf[18] = c1[6];
    vf[19] = c3[13];
    vf[20] = c0[18];
    vf[21] = c0[0];
    vf[22] = c1[5];
    vf[23] = c3[12];
    vf[24] = c0[17];
    vf[25] = c1[22];
    vf[26] = c1[4];
    vf[27] = c3[11];
    vf[28] = c0[16];
    vf[29] = c1[21];
    vf[30] = c1[3];
    vf[31] = c3[10];
    vf[32] = c0[15];
    vf[33] = c1[20];
    vf[34] = c1[2];
    vf[35] = c3[9];
    vf[36] = c0[14];
    vf[37] = c1[19];
    vf[38] = c1[1];
    vf[39] = c3[8];
    vf[40] = c0[13];
    vf[41] = c1[18];
    vf[42] = c1[0];
    vf[43] = c3[7];
    vf[44] = c0[12];
    vf[45] = c1[17];
    vf[46] = c2[10];
    vf[47] = c3[6];
    vf[48] = c0[11];
    vf[49] = c1[16];
    vf[50] = c2[9];
    vf[51] = c3[5];
    vf[52] = c0[10];
    vf[53] = c1[15];
    vf[54] = c2[8];
    vf[55] = c3[4];
    vf[56] = c0[9];
    vf[57] = c1[14];
    vf[58] = c2[7];
    vf[59] = c3[3];
    vf[60] = c0[8];
    vf[61] = c1[13];
    vf[62] = c2[6];
    vf[63] = c3[2];
    vf[64] = c0[7];
    vf[65] = c1[12];
    vf[66] = c2[5];
    vf[67] = c3[1];
    vf[68] = c0[6];
    vf[69] = c1[11];
    vf[70] = c2[4];
    vf[71] = c3[0];

    for (unsigned int i = 0; i < sizeof(vf) / 2; i++) {
        _vf[i] = (vf[i * 2] << 1) | vf[i * 2 + 1];
    }
}
