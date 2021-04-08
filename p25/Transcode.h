/**
* Digital Voice Modem - Transcode Software
* GPLv2 Open Source. Use is subject to license terms.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
*
* @package DVM / Transcode Software
*
*/
//
// Based on code from the MMDVMHost project. (https://github.com/g4klx/MMDVMHost)
// Licensed under the GPLv2 License (https://opensource.org/licenses/GPL-2.0)
//
/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2017-2020 by Bryan Biedenkapp N2PLL
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#if !defined(__P25_TRANSCODE_H__)
#define __P25_TRANSCODE_H__

#include "Defines.h"
#include "dmr/data/EmbeddedData.h"
#include "network/BaseNetwork.h"
#include "vocoder/MBEDecoder.h"
#include "vocoder/MBEEncoder.h"
#include "RingBuffer.h"
#include "Timer.h"

#include <cstdio>

namespace p25
{
    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      This class implements core logic for handling P25 -> DMR transcoding.
    // ---------------------------------------------------------------------------

    class HOST_SW_API Transcode {
    public:
        /// <summary>Initializes a new instance of the Transcode class.</summary>
        Transcode(network::BaseNetwork* srcNetwork, network::BaseNetwork* dstNetwork, uint32_t timeout, bool debug, bool verbose);
        /// <summary>Finalizes a instance of the Transcode class.</summary>
        ~Transcode();

        /// <summary>Updates the processor by the passed number of milliseconds.</summary>
        void clock(uint32_t ms);

    private:
        network::BaseNetwork* m_srcNetwork;
        network::BaseNetwork* m_dstNetwork;

        RPT_NET_STATE m_netState;

        Timer m_netTimeout;
        Timer m_networkWatchdog;

        uint32_t m_netFrames;
        uint32_t m_netLost;

        lc::LC m_netLC;

        uint8_t* m_netLDU1;
        uint8_t* m_netLDU2;
        uint8_t* m_lastIMBE;

        uint8_t* m_ambeBuffer;
        uint8_t m_ambeCount;
        uint8_t m_dmrSeqNo;
        uint8_t m_dmrN;
        dmr::data::EmbeddedData m_embeddedData;

        vocoder::MBEDecoder* m_mbeDecode;
        vocoder::MBEEncoder* m_mbeEncode;

        bool m_verbose;
        bool m_debug;

        /// <summary>Process a data frames from the network.</summary>
        void processNetwork();
        /// <summary></summary>
        void decodeAndProcessIMBE(uint8_t* imbe);

        /// <summary>Helper to check for an unflushed LDU1 packet.</summary>
        void checkNet_LDU1(const lc::LC& control, const data::LowSpeedData& lsd);
        /// <summary>Helper to write a network P25 LDU1 packet.</summary>
        void writeNet_LDU1(const lc::LC& control, const data::LowSpeedData& lsd);
        /// <summary>Helper to check for an unflushed LDU2 packet.</summary>
        void checkNet_LDU2(const lc::LC& control, const data::LowSpeedData& lsd);
        /// <summary>Helper to write a network P25 LDU1 packet.</summary>
        void writeNet_LDU2(const lc::LC& control, const data::LowSpeedData& lsd);

        /// <summary>Helper to insert IMBE silence frames for missing audio.</summary>
        void insertMissingAudio(uint8_t* data);
    };
} // namespace p25

#endif // __P25_TRANSCODE_H__
