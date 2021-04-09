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
*   Copyright (C) 2017,2020 by Bryan Biedenkapp N2PLL
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
#if !defined(__DMR_TRANSCODE_H__)
#define __DMR_TRANSCODE_H__

#include "Defines.h"
#include "dmr/data/Data.h"
#include "dmr/Slot.h"
#include "network/BaseNetwork.h"

namespace dmr
{
    // ---------------------------------------------------------------------------
    //  Class Prototypes
    // ---------------------------------------------------------------------------
    class HOST_SW_API Slot;

    // ---------------------------------------------------------------------------
    //  Class Declaration
    //      This class implements core logic for handling DMR -> P25 transcoding.
    // ---------------------------------------------------------------------------

    class HOST_SW_API Transcode {
    public:
        /// <summary>Initializes a new instance of the Transcode class.</summary>
        Transcode(network::BaseNetwork* srcNetwork, network::BaseNetwork* dstNetwork, uint32_t timeout, uint32_t jitter, float gainAdjust, bool debug, bool verbose);
        /// <summary>Finalizes a instance of the Transcode class.</summary>
        ~Transcode();

        /// <summary>Updates the processor.</summary>
        void clock();

    private:
        network::BaseNetwork* m_srcNetwork;
        network::BaseNetwork* m_dstNetwork;

        Slot* m_slot1;
        Slot* m_slot2;

        bool m_verbose;
        bool m_debug;
    };
} // namespace dmr

#endif // __DMR_TRANSCODE_H__
