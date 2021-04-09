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
#if !defined(__MBE_DECODER_H__)
#define __MBE_DECODER_H__

extern "C" {
#include "mbe.h"
}

#include "Defines.h"

#include <stdlib.h>
#include <queue>

namespace vocoder
{
    // ---------------------------------------------------------------------------
    //  Structure Declaration
    //      
    // ---------------------------------------------------------------------------

    struct mbelibParms
    {
        mbe_parms* m_cur_mp;
        mbe_parms* m_prev_mp;
        mbe_parms* m_prev_mp_enhanced;

        /// <summary></summary>
        mbelibParms()
        {
            m_cur_mp = (mbe_parms*)malloc(sizeof(mbe_parms));
            m_prev_mp = (mbe_parms*)malloc(sizeof(mbe_parms));
            m_prev_mp_enhanced = (mbe_parms*)malloc(sizeof(mbe_parms));
        }

        /// <summary></summary>
        ~mbelibParms()
        {
            free(m_prev_mp_enhanced);
            free(m_prev_mp);
            free(m_cur_mp);
        }
    };

    // ---------------------------------------------------------------------------
    //  Constants
    // ---------------------------------------------------------------------------

    enum MBE_DECODER_MODE {
        DECODE_DMR_AMBE,
        DECODE_88BIT_IMBE   // e.g. IMBE used by P25
    };

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      Implements MBE audio decoding.
    // ---------------------------------------------------------------------------

    class HOST_SW_API MBEDecoder
    {
    public:
        /// <summary>Initializes a new instance of the MBEDecoder class.</summary>
        MBEDecoder(MBE_DECODER_MODE mode);
        /// <summary>Finalizes a instance of the MBEDecoder class.</summary>
        ~MBEDecoder();

        /// <summary>Decodes the given MBE codewords to PCM samples using the decoder mode.</summary>
        int32_t decode(uint8_t* codeword, int16_t samples[]);

    private:
        mbelibParms* m_mbelibParms;

        MBE_DECODER_MODE m_mbeMode;

        static const int dW[72];
        static const int dX[72];
        static const int rW[36];
        static const int rX[36];
        static const int rY[36];
        static const int rZ[36];

    public:
        /// <summary></summary>
        __PROPERTY(float, gainAdjust, GainAdjust);
    };
} // namespace vocoder

#endif // __MBE_DECODER_H__
