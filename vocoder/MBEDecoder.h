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
    //  Class Declaration
    //      Implements MBE audio decoding.
    // ---------------------------------------------------------------------------

    class HOST_SW_API MBEDecoder
    {
    public:
        /// <summary>Initializes a new instance of the MBEDecoder class.</summary>
        explicit MBEDecoder();
        /// <summary>Finalizes a instance of the MBEDecoder class.</summary>
        ~MBEDecoder();

        /// <summary></summary>
        void initMbeParms();

        /// <summary></summary>
        void processDmr(unsigned char* d);
        /// <summary></summary>
        void processP25(unsigned char* d);

        /// <summary></summary>
        void ambe49to72(char ambe_data[49], char data[9]);

        /// <summary></summary>
        void processFrame(char ambe_fr[4][24]);

        /// <summary></summary>
        void processData(char ambe_data[49]);
        /// <summary></summary>
        void processData4400(char imbe_data[88]);

        /// <summary></summary>
        short* getAudio(int& nbSamples)
        {
            nbSamples = m_audioOutNbSamples;
            return m_audioOutBuf;
        }

        /// <summary></summary>
        void resetAudio()
        {
            m_audioOutNbSamples = 0;
            m_audioOutBufPtr = m_audioOutBuf;
        }

        /// <summary></summary>
        void setAudioGain(float gain) { m_aoutGain = gain; }
        /// <summary></summary>
        void setAutoGain(bool enabled) { m_autoGain = enabled; }
        /// <summary></summary>
        void setVolume(float volume) { m_volume = volume; }
        /// <summary></summary>
        void setStereo(bool stereo) { m_stereo = stereo; }
        /// <summary></summary>
        void setChannels(unsigned char channels) { m_channels = channels % 4; }
        /// <summary></summary>
        void setUpsamplingFactor(int upsample) { m_upsample = upsample; }
        /// <summary></summary>
        int getUpsamplingFactor() const { return m_upsample; }

        std::queue<char> ambe72;

    private:
        float m_upsamplerLastValue;

        mbelibParms* m_mbelibParms;
        int m_errs;
        int m_errs2;
        char m_err_str[64];

        float m_audioOutTempBuf[160];   //!< output of decoder
        float* m_audioOutTempBufPtr;

        float m_audioOutFloatBuf[1120]; //!< output of upsampler - 1 frame of 160 samples upampled up to 7 times
        float* m_audioOutFloatBufPtr;

        float m_aoutMaxBuf[200];
        float* m_aoutMaxBufPtr;
        int m_aoutMaxBufIdx;

        short m_audioOutBuf[2 * 48000];    //!< final result - 1s of L+R S16LE samples
        short* m_audioOutBufPtr;
        int m_audioOutNbSamples;
        int m_audioOutBufSize;
        int m_audioOutIdx;
        int m_audioOutIdx2;

        float m_aoutGain;
        float m_volume;
        bool m_autoGain;
        int m_upsample;            //!< upsampling factor
        bool m_stereo;             //!< double each audio sample to produce L+R channels
        unsigned char m_channels;  //!< when in stereo output to none (0) or only left (1), right (2) or both (3) channels

        const int* w, *x, *y, *z;
        static const int dW[72];
        static const int dX[72];
        static const int rW[36];
        static const int rX[36];
        static const int rY[36];
        static const int rZ[36];

        char ambe_d[49];

        /// <summary></summary>
        void processAudio();
    };
} // namespace vocoder

#endif // __MBE_DECODER_H__
