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
#if !defined(__MBE_ENCODER_H__)
#define __MBE_ENCODER_H__

#include "mbe.h"
#include "imbe/imbe_vocoder.h"

#include <stdint.h>

namespace vocoder
{
    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements MBE audio encoding.
    // ---------------------------------------------------------------------------

    class MBEEncoder {
    public:
        /// <summary></summary>
        MBEEncoder();

        /// <summary></summary>
        void set49bitMode();
        /// <summary></summary>
        void setDmrMode();
        /// <summary></summary>
        void set88bitMode();

        /// <summary></summary>
        void encode(int16_t samples[], uint8_t codeword[]);

        /// <summary></summary>
        void setGainAdjust(const float gain) { m_gainAdjust = gain; }

    private:
        imbe_vocoder m_vocoder;
        mbe_parms m_curMBEParms;
        mbe_parms m_prevMBEParms;

        bool m_49bitMode;
        bool m_dmrMode;
        bool m_88bitMode;

        float m_gainAdjust;

        /// <summary></summary>
        void encodeDmr(const unsigned char* in, unsigned char* out);
        /// <summary></summary>
        void encodeVcw(uint8_t vf[], const int* b);
        /// <summary></summary>
        void interleaveVcw(uint8_t _vf[], int _c0, int _c1, int _c2, int _c3);
    };
} // namespace vocoder

#endif // __MBE_ENCODER_H__
