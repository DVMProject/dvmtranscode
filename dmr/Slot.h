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
*   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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
#if !defined(__DMR_SLOT_H__)
#define __DMR_SLOT_H__

#include "Defines.h"
#include "dmr/Transcode.h"
#include "dmr/lc/LC.h"
#include "edac/AMBEFEC.h"
#include "p25/lc/LC.h"
#include "network/BaseNetwork.h"
#include "vocoder/MBEDecoder.h"
#include "vocoder/MBEEncoder.h"
#include "RingBuffer.h"
#include "StopWatch.h"
#include "Timer.h"

#include <vector>

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Class Prototypes
    // ---------------------------------------------------------------------------
    class HOST_SW_API Transcode;

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      This class implements core logic for handling DMR slots.
    // ---------------------------------------------------------------------------

    class HOST_SW_API Slot {
    public:
        /// <summary>Initializes a new instance of the Slot class.</summary>
        Slot(uint32_t slotNo, network::BaseNetwork* srcNetwork, network::BaseNetwork* dstNetwork, uint32_t timeout, bool debug, bool verbose);
        /// <summary>Finalizes a instance of the Slot class.</summary>
        ~Slot();

        /// <summary>Process a data frames from the network.</summary>
        void processNetwork(const data::Data& data);

        /// <summary>Updates the slot processor.</summary>
        void clock();

        /// <summary>Helper to initialize the slot processor.</summary>
        static void init(uint32_t jitter, float gainAdjust);

    private:
        friend class Transcode;

        uint32_t m_slotNo;

        RPT_NET_STATE m_netState;

        Timer m_networkWatchdog;
        Timer m_netTimeoutTimer;
        Timer m_packetTimer;

        StopWatch m_interval;
        StopWatch m_elapsed;

        lc::LC* m_netLC;
        uint8_t m_netN;

        uint32_t m_netFrames;
        uint32_t m_netLost;
        uint32_t m_netMissed;

        uint32_t m_netBits;
        uint32_t m_netErrs;

        bool m_netTimeout;

        p25::lc::LC m_p25LC;
        p25::data::LowSpeedData m_p25LSD;
        uint8_t m_p25N;
        uint8_t* m_netLDU1;
        uint8_t* m_netLDU2;

        edac::AMBEFEC m_fec;

        network::BaseNetwork* m_srcNetwork;
        network::BaseNetwork* m_dstNetwork;

        bool m_verbose;
        bool m_debug;

        static vocoder::MBEDecoder* m_mbeDecode;
        static vocoder::MBEEncoder* m_mbeEncode;

        static uint32_t m_jitterTime;
        static uint32_t m_jitterSlots;

        static uint8_t* m_idle;

        static uint8_t m_flco1;
        static uint8_t m_id1;
        static bool m_voice1;

        static uint8_t m_flco2;
        static uint8_t m_id2;
        static bool m_voice2;

        /// <summary></summary>
        void decodeAndProcessAMBE(uint8_t* ambe);
    };
} // namespace dmr

#endif // __DMR_SLOT_H__
