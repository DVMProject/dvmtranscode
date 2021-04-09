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
#if !defined(__MBE_ENCODER_H__)
#define __MBE_ENCODER_H__

#include "mbe.h"
#include "imbe/imbe_vocoder.h"

#include <stdint.h>

namespace vocoder
{
    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    enum MBE_ENCODER_MODE {
        ENCODE_DMR_AMBE,
        ENCODE_88BIT_IMBE,  // e.g. IMBE used by P25
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements MBE audio encoding.
    // ---------------------------------------------------------------------------

    class MBEEncoder {
    public:
        /// <summary>Initializes a new instance of the MBEEncoder class.</summary>
        MBEEncoder(MBE_ENCODER_MODE mode);

        /// <summary>Encodes the given PCM samples using the encoder mode to MBE codewords.</summary>
        void encode(int16_t samples[], uint8_t codeword[]);

    private:
        imbe_vocoder m_vocoder;
        mbe_parms m_curMBEParms;
        mbe_parms m_prevMBEParms;

        MBE_ENCODER_MODE m_mbeMode;

    public:
        /// <summary></summary>
        __PROPERTY(float, gainAdjust, GainAdjust);
    };
} // namespace vocoder

#endif // __MBE_ENCODER_H__
