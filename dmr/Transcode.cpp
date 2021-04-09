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
*   Copyright (C) 2015,2016,2017 Jonathan Naylor, G4KLX
*   Copyright (C) 2017-2021 by Bryan Biedenkapp N2PLL
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; version 2 of the License.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*/
#include "Defines.h"
#include "dmr/Transcode.h"
#include "Log.h"

using namespace dmr;

#include <cstdio>
#include <cassert>
#include <algorithm>

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the Transcode class.
/// </summary>
/// <param name="network">Instance of the BaseNetwork class representing the source network.</param>
/// <param name="network">Instance of the BaseNetwork class representing the destination network.</param>
/// <param name="timeout">Transmit timeout.</param>
/// <param name="jitter"></param>
/// <param name="gainAdjust"></param>
/// <param name="debug">Flag indicating whether DMR debug is enabled.</param>
/// <param name="verbose">Flag indicating whether DMR verbose logging is enabled.</param>
Transcode::Transcode(network::BaseNetwork* srcNetwork, network::BaseNetwork* dstNetwork, uint32_t timeout, uint32_t jitter, float gainAdjust, bool debug, bool verbose) :
    m_srcNetwork(srcNetwork),
    m_dstNetwork(dstNetwork),
    m_slot1(NULL),
    m_slot2(NULL),
    m_verbose(verbose),
    m_debug(debug)
{
    assert(srcNetwork != NULL);
    assert(dstNetwork != NULL);

    Slot::init(jitter, gainAdjust);
    
    m_slot1 = new Slot(1U, srcNetwork, dstNetwork, timeout, debug, verbose);
    m_slot2 = new Slot(2U, srcNetwork, dstNetwork, timeout, debug, verbose);
}

/// <summary>
/// Finalizes a instance of the Transcode class.
/// </summary>
Transcode::~Transcode()
{
    delete m_slot2;
    delete m_slot1;
}

/// <summary>
/// Updates the processor.
/// </summary>
void Transcode::clock()
{
    if (m_srcNetwork != NULL) {
        data::Data data;
        bool ret = m_srcNetwork->readDMR(data);
        if (ret) {
            uint32_t slotNo = data.getSlotNo();
            switch (slotNo) {
                case 1U:
                    m_slot1->processNetwork(data);
                    break;
                case 2U:
                    m_slot2->processNetwork(data);
                    break;
                default:
                    LogError(LOG_NET, "DMR, invalid slot, slotNo = %u", slotNo);
                    break;
            }
        }
    }

    m_slot1->clock();
    m_slot2->clock();
}
