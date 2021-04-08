/**
* Digital Voice Modem - Transcode Software
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / Transcode Software
*
*/
/*
*   Copyright (C) 2019-2021 Doug McLain
*   Copyright (C) 2021 by Bryan Biedenkapp N2PLL
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <string.h>
#include <math.h>

#include "edac/Golay24128.h"
#include "vocoder/MBEDecoder.h"

using namespace edac;
using namespace vocoder;

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

const int MBEDecoder::dW[72] = { 0,0,3,2,1,1,0,0,1,1,0,0,3,2,1,1,3,2,1,1,0,0,3,2,0,0,3,2,1,1,0,0,1,1,0,0,3,2,1,1,3,2,1,1,0,0,3,2,0,0,3,2,1,1,0,0,1,1,0,0,3,2,1,1,3,3,2,1,0,0,3,3, };

const int MBEDecoder::dX[72] = { 10,22,11,9,10,22,11,23,8,20,9,21,10,8,9,21,8,6,7,19,8,20,9,7,6,18,7,5,6,18,7,19,4,16,5,17,6,4,5,17,4,2,3,15,4,16,5,3,2,14,3,1,2,14,3,15,0,12,1,13,2,0,1,13,0,12,10,11,0,12,1,13, };

const int MBEDecoder::rW[36] = {
    0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 2,
    0, 2, 0, 2, 0, 2,
    0, 2, 0, 2, 0, 2
};

const int MBEDecoder::rX[36] = {
    23, 10, 22, 9, 21, 8,
    20, 7, 19, 6, 18, 5,
    17, 4, 16, 3, 15, 2,
    14, 1, 13, 0, 12, 10,
    11, 9, 10, 8, 9, 7,
    8, 6, 7, 5, 6, 4
};

// bit 0
const int MBEDecoder::rY[36] = {
    0, 2, 0, 2, 0, 2,
    0, 2, 0, 3, 0, 3,
    1, 3, 1, 3, 1, 3,
    1, 3, 1, 3, 1, 3,
    1, 3, 1, 3, 1, 3,
    1, 3, 1, 3, 1, 3
};

const int MBEDecoder::rZ[36] = {
    5, 3, 4, 2, 3, 1,
    2, 0, 1, 13, 0, 12,
    22, 11, 21, 10, 20, 9,
    19, 8, 18, 7, 17, 6,
    16, 5, 15, 4, 14, 3,
    13, 2, 12, 1, 11, 0
};

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the MBEDecoder class.
/// </summary>
MBEDecoder::MBEDecoder() :
    m_upsamplerLastValue(0.0f),
    m_mbelibParms(nullptr)
{
    m_mbelibParms = new mbelibParms();
    m_audioOutTempBufPtr = m_audioOutTempBuf;
    memset(m_audioOutFloatBuf, 0, sizeof(float) * 1120);
    m_audioOutFloatBufPtr = m_audioOutFloatBuf;
    memset(m_aoutMaxBuf, 0, sizeof(float) * 200);
    m_aoutMaxBufPtr = m_aoutMaxBuf;
    m_aoutMaxBufIdx = 0;

    memset(m_audioOutBuf, 0, sizeof(short) * 2 * 48000);
    m_audioOutBufPtr = m_audioOutBuf;
    m_audioOutNbSamples = 0;
    m_audioOutBufSize = 48000; // given in number of unique samples
    m_audioOutIdx = 0;
    m_audioOutIdx2 = 0;

    m_aoutGain = 100;
    m_volume = 1.0f;
    m_autoGain = false;
    m_stereo = false;
    m_channels = 3; // both channels by default if stereo is set
    m_upsample = 0;

    initMbeParms();
    memset(ambe_d, 0, 49);
}

/// <summary>
/// Finalizes a instance of the MBEDecoder class.
/// </summary>
MBEDecoder::~MBEDecoder()
{
    delete m_mbelibParms;
}

/// <summary>
/// 
/// </summary>
void MBEDecoder::initMbeParms()
{
    mbe_initMbeParms(m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced);
    m_errs = 0;
    m_errs2 = 0;
    m_err_str[0] = 0;

    if (m_autoGain) {
        m_aoutGain = 100;
    }
}

/// <summary>
/// 
/// </summary>
/// <param name="d"></param>
void MBEDecoder::processDmr(unsigned char* d)
{
    char ambe_fr[4][24];

    memset(ambe_fr, 0, 96);
    w = rW;
    x = rX;
    y = rY;
    z = rZ;

    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 8; j += 2) {
            ambe_fr[*y][*z] = (1 & (d[i] >> (7 - (j + 1))));
            ambe_fr[*w][*x] = (1 & (d[i] >> (7 - j)));
            w++;
            x++;
            y++;
            z++;
        }
    }

    mbe_processAmbe3600x2450FrameF(m_audioOutTempBuf, &m_errs, &m_errs2, m_err_str, ambe_fr, ambe_d, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
    processAudio();
}

/// <summary>
/// 
/// </summary>
/// <param name="d"></param>
void MBEDecoder::processP25(unsigned char* d)
{
    char imbe_data[88];

    for (int i = 0; i < 11; ++i) {
        for (int j = 0; j < 8; j++) {
            imbe_data[j + (8 * i)] = (1 & (d[i] >> (7 - j)));
        }
    }

    processData4400(imbe_data);
}

/// <summary>
/// 
/// </summary>
/// <param name="ambe_data"></param>
/// <param name="data"></param>
void MBEDecoder::ambe49to72(char ambe_data[49], char data[9])
{
    int tmp = 0;
    char ambe_fr[4][24];

    for (int i = 11; i >= 0; --i) {
        tmp = (tmp << 1) | ambe_data[i];
    }

    tmp = Golay24128::encode23127(tmp);
    int p = tmp & 0xff;
    p = p ^ ((tmp >> 8) & 0xff);
    p = p ^ ((tmp >> 16) & 0xff);
    p = p ^ (p >> 4);
    p = p ^ (p >> 2);
    p = p ^ (p >> 1);
    p = p & 1;
    tmp = tmp | (p << 23);

    for (int i = 23; i >= 0; i--) {
        ambe_fr[0][i] = (tmp & 1);
        tmp = tmp >> 1;
    }
    tmp = 0;
    for (int i = 23; i > 11; --i) {
        tmp = (tmp << 1) | ambe_data[i];
    }
    tmp = Golay24128::encode23127(tmp);
    for (int i = 22; i >= 0; --i) {
        ambe_fr[1][i] = (tmp & 1);
        tmp = tmp >> 1;
    }
    for (int i = 10; i >= 0; --i) {
        ambe_fr[2][i] = ambe_d[34 - i];
    }
    for (int i = 13; i >= 0; --i) {
        ambe_fr[3][i] = ambe_d[48 - i];
    }
    int i, j, k;
    unsigned short pr[115];
    unsigned short foo = 0;

    // create pseudo-random modulator
    for (i = 23; i >= 12; i--) {
        foo <<= 1;
        foo |= ambe_fr[0][i];
    }
    pr[0] = (16 * foo);
    for (i = 1; i < 24; i++) {
        pr[i] = (173 * pr[i - 1]) + 13849 - (65536 * (((173 * pr[i - 1]) + 13849) / 65536));
    }
    for (i = 1; i < 24; i++) {
        pr[i] = pr[i] / 32768;
    }

    // demodulate ambe_fr with pr
    k = 1;
    for (j = 22; j >= 0; j--) {
        ambe_fr[1][j] = ((ambe_fr[1][j]) ^ pr[k]);
        k++;
    }
    //char data[9];
    char bit0, bit1;
    int bitIndex = 0;
    int ww = 0;
    int xx = 0;
    int yy = 0;
    int zz = 0;
    for (i = 0; i < 36; ++i) {
        bit1 = ambe_fr[rW[ww]][rX[xx]];
        bit0 = ambe_fr[rY[yy]][rZ[zz]];
        data[bitIndex / 8] = ((data[bitIndex / 8] << 1) & 0xfe) | ((bit1) ? 1 : 0);
        bitIndex += 1;

        data[bitIndex / 8] = ((data[bitIndex / 8] << 1) & 0xfe) | ((bit0) ? 1 : 0);
        bitIndex += 1;

        ww += 1;
        xx += 1;
        yy += 1;
        zz += 1;
    }
}

/// <summary>
/// 
/// </summary>
/// <param name="ambe_fr"></param>
void MBEDecoder::processFrame(char ambe_fr[4][24])
{
    mbe_processAmbe3600x2450FrameF(m_audioOutTempBuf, &m_errs, &m_errs2, m_err_str, ambe_fr, ambe_d, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
    processAudio();
}

/// <summary>
/// 
/// </summary>
/// <param name="ambe_data"></param>
void MBEDecoder::processData(char ambe_data[49])
{
    mbe_processAmbe2450DataF(m_audioOutTempBuf, &m_errs, &m_errs2, m_err_str, ambe_data, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
    processAudio();
}

/// <summary>
/// 
/// </summary>
/// <param name="imbe_data"></param>
void MBEDecoder::processData4400(char imbe_data[88])
{
    mbe_processImbe4400DataF(m_audioOutTempBuf, &m_errs, &m_errs2, m_err_str, imbe_data, m_mbelibParms->m_cur_mp, m_mbelibParms->m_prev_mp, m_mbelibParms->m_prev_mp_enhanced, 3);
    processAudio();
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// 
/// </summary>
void MBEDecoder::processAudio()
{
    int i, n;
    float aout_abs, max, gainfactor, gaindelta, maxbuf;

    if (m_autoGain) {
        // detect max level
        max = 0;
        m_audioOutTempBufPtr = m_audioOutTempBuf;

        for (n = 0; n < 160; n++) {
            aout_abs = fabsf(*m_audioOutTempBufPtr);

            if (aout_abs > max) {
                max = aout_abs;
            }

            m_audioOutTempBufPtr++;
        }

        *m_aoutMaxBufPtr = max;
        m_aoutMaxBufPtr++;
        m_aoutMaxBufIdx++;

        if (m_aoutMaxBufIdx > 24) {
            m_aoutMaxBufIdx = 0;
            m_aoutMaxBufPtr = m_aoutMaxBuf;
        }

        // lookup max history
        for (i = 0; i < 25; i++) {
            maxbuf = m_aoutMaxBuf[i];

            if (maxbuf > max) {
                max = maxbuf;
            }
        }

        // determine optimal gain level
        if (max > static_cast<float>(0)) {
            gainfactor = (static_cast<float>(30000) / max);
        }
        else {
            gainfactor = static_cast<float>(50);
        }

        if (gainfactor < m_aoutGain) {
            m_aoutGain = gainfactor;
            gaindelta = static_cast<float>(0);
        }
        else {
            if (gainfactor > static_cast<float>(50)) {
                gainfactor = static_cast<float>(50);
            }

            gaindelta = gainfactor - m_aoutGain;

            if (gaindelta > (static_cast<float>(0.05) * m_aoutGain)) {
                gaindelta = (static_cast<float>(0.05) * m_aoutGain);
            }
        }

        gaindelta /= static_cast<float>(160);

        // adjust output gain
        m_audioOutTempBufPtr = m_audioOutTempBuf;

        for (n = 0; n < 160; n++) {
            *m_audioOutTempBufPtr = (m_aoutGain
                + (static_cast<float>(n) * gaindelta)) * (*m_audioOutTempBufPtr);
            m_audioOutTempBufPtr++;
        }

        m_aoutGain += (static_cast<float>(160) * gaindelta);
    }
    else {
        gaindelta = static_cast<float>(0);
    }

    // copy audio data to output buffer and upsample if necessary
    m_audioOutTempBufPtr = m_audioOutTempBuf;

    if (m_audioOutNbSamples + 160 >= m_audioOutBufSize) {
        resetAudio();
    }

    m_audioOutFloatBufPtr = m_audioOutFloatBuf;

    for (n = 0; n < 160; n++) {
        *m_audioOutTempBufPtr *= m_volume;
        if (*m_audioOutTempBufPtr > static_cast<float>(32760)) {
            *m_audioOutTempBufPtr = static_cast<float>(32760);
        }
        else if (*m_audioOutTempBufPtr < static_cast<float>(-32760)) {
            *m_audioOutTempBufPtr = static_cast<float>(-32760);
        }

        *m_audioOutBufPtr = static_cast<short>(*m_audioOutTempBufPtr);
        m_audioOutBufPtr++;

        if (m_stereo) {
            *m_audioOutBufPtr = static_cast<short>(*m_audioOutTempBufPtr);
            m_audioOutBufPtr++;
        }

        m_audioOutNbSamples++;
        m_audioOutTempBufPtr++;
        m_audioOutIdx++;
        m_audioOutIdx2++;
    }
}
