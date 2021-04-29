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
*   Copyright (C) 2015,2016,2017,2018 Jonathan Naylor, G4KLX
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
#include "dmr/Slot.h"
#include "dmr/SlotType.h"
#include "edac/BPTC19696.h"
#include "edac/CRC.h"
#include "p25/lc/LC.h"
#include "Log.h"
#include "Utils.h"

using namespace dmr;

#include <cassert>
#include <ctime>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
//  Static Class Members
// ---------------------------------------------------------------------------

vocoder::MBEDecoder* Slot::m_mbeDecode = NULL;
vocoder::MBEEncoder* Slot::m_mbeEncode = NULL;

uint32_t Slot::m_jitterTime = 360U;
uint32_t Slot::m_jitterSlots = 6U;

uint8_t* Slot::m_idle = NULL;

uint8_t Slot::m_flco1;
uint8_t Slot::m_id1 = 0U;
bool Slot::m_voice1 = true;
uint8_t Slot::m_flco2;
uint8_t Slot::m_id2 = 0U;
bool Slot::m_voice2 = true;

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the Slot class.
/// </summary>
/// <param name="slotNo">DMR slot number.</param>
/// <param name="network">Instance of the BaseNetwork class representing the source network.</param>
/// <param name="network">Instance of the BaseNetwork class representing the destination network.</param>
/// <param name="timeout">Transmit timeout.</param>
/// <param name="debug">Flag indicating whether DMR debug is enabled.</param>
/// <param name="verbose">Flag indicating whether DMR verbose logging is enabled.</param>
Slot::Slot(uint32_t slotNo, network::BaseNetwork* srcNetwork, network::BaseNetwork* dstNetwork, uint32_t timeout, bool debug, bool verbose) :
    m_slotNo(slotNo),
    m_netState(RS_NET_IDLE),
    m_networkWatchdog(1000U, 0U, 1500U),
    m_netTimeoutTimer(1000U, timeout),
    m_packetTimer(1000U, 0U, 50U),
    m_interval(),
    m_elapsed(),
    m_netLC(NULL),
    m_netN(0U),
    m_netFrames(0U),
    m_netLost(0U),
    m_netMissed(0U),
    m_netBits(1U),
    m_netErrs(0U),
    m_netTimeout(false),
    m_p25LC(),
    m_p25LSD(),
    m_p25N(0U),
    m_netLDU1(NULL),
    m_netLDU2(NULL),
    m_fec(),
    m_srcNetwork(srcNetwork),
    m_dstNetwork(dstNetwork),
    m_verbose(verbose),
    m_debug(debug)
{
    assert(srcNetwork != NULL);
    assert(dstNetwork != NULL);

    m_interval.start();

    m_netLDU1 = new uint8_t[9U * 25U];
    m_netLDU2 = new uint8_t[9U * 25U];

    ::memset(m_netLDU1, 0x00U, 9U * 25U);
    ::memset(m_netLDU2, 0x00U, 9U * 25U);
}

/// <summary>
/// Finalizes a instance of the Slot class.
/// </summary>
Slot::~Slot()
{
    delete[] m_netLDU1;
    delete[] m_netLDU2;
}

/// <summary>
/// Process a data frame from the network.
/// </summary>
/// <param name="dmrData"></param>
void Slot::processNetwork(const data::Data& dmrData)
{
    m_networkWatchdog.start();

    uint8_t dataType = dmrData.getDataType();

    switch (dataType)
    {
    case DT_VOICE_SYNC:
    case DT_VOICE:
    {
        uint8_t dataType = dmrData.getDataType();

        uint8_t data[DMR_FRAME_LENGTH_BYTES];
        dmrData.getData(data);

        if (dataType == DT_VOICE_SYNC) {
            if (m_netState == RS_NET_IDLE) {
                lc::LC* lc = new lc::LC(dmrData.getFLCO(), dmrData.getSrcId(), dmrData.getDstId());

                uint32_t dstId = lc->getDstId();
                uint32_t srcId = lc->getSrcId();

                m_netLC = lc;

                m_netTimeoutTimer.start();
                m_netTimeout = false;

                m_netFrames = 0U;
                m_netLost = 0U;
                m_netBits = 1U;
                m_netErrs = 0U;
            }

            uint8_t fid = m_netLC->getFID();
            if (fid == FID_ETSI || fid == FID_DMRA) {
                m_netErrs += m_fec.regenerateDMR(data);
                if (m_verbose) {
                    LogMessage(LOG_NET, DMR_DT_VOICE_SYNC ", audio, slot = %u, srcId = %u, dstId = %u, seqNo = %u, errs = %u/141 (%.1f%%)", m_slotNo, m_netLC->getSrcId(), m_netLC->getDstId(),
                        m_netN, m_netErrs, float(m_netErrs) / 1.41F);
                }
            }
            m_netBits += 141U;

            uint8_t ambe[DMR_AMBE_LENGTH_BYTES];
            ::memset(ambe, 0x00U, DMR_AMBE_LENGTH_BYTES);

            // extract AMBE super frames
            ::memcpy(ambe, data, 14U);
            ambe[13U] &= 0xF0U;
            ambe[13U] |= (data[19U] & 0x0FU);
            ::memcpy(ambe + 14U, data + 20U, 13U);

            decodeAndProcessAMBE(ambe);

            // Initialise the lost packet data
            if (m_netFrames == 0U) {
                m_netN = 5U;
                m_netLost = 0U;
            }

            m_packetTimer.start();
            m_elapsed.start();

            m_netFrames++;

            // Save details in case we need to infill data
            m_netN = dmrData.getN();
        }
        else if (dataType == DT_VOICE) {
            if (m_netState != RS_NET_AUDIO)
                return;

            uint8_t fid = m_netLC->getFID();
            if (fid == FID_ETSI || fid == FID_DMRA) {
                m_netErrs += m_fec.regenerateDMR(data);
                if (m_verbose) {
                    LogMessage(LOG_NET, DMR_DT_VOICE ", audio, slot = %u, srcId = %u, dstId = %u, seqNo = %u, errs = %u/141 (%.1f%%)", m_slotNo, m_netLC->getSrcId(), m_netLC->getDstId(),
                        m_netN, m_netErrs, float(m_netErrs) / 1.41F);
                }
            }
            m_netBits += 141U;

            uint8_t ambe[DMR_AMBE_LENGTH_BYTES];
            ::memset(ambe, 0x00U, DMR_AMBE_LENGTH_BYTES);

            // extract AMBE frames from network data
            ::memcpy(ambe, data, 14U);
            ambe[13U] &= 0xF0U;
            ambe[13U] |= (data[19U] & 0x0FU);
            ::memcpy(ambe + 14U, data + 20U, 13U);

            decodeAndProcessAMBE(ambe);

            // Initialise the lost packet data
            if (m_netFrames == 0U) {
                m_netN = 5U;
                m_netLost = 0U;
            }

            m_packetTimer.start();
            m_elapsed.start();

            m_netFrames++;

            // Save details in case we need to infill data
            m_netN = dmrData.getN();
        }
        break;
    }

    case DT_TERMINATOR_WITH_LC:
    {
        if (m_verbose) {
            LogMessage(LOG_NET, DMR_DT_TERMINATOR_WITH_LC ", slot = %u, dstId = %u", m_slotNo, m_netLC->getDstId());
        }

        LogMessage(LOG_NET, "DMR Slot %u network end of voice transmission, %.1f seconds, %u%% packet loss, BER: %.1f%%",
            m_slotNo, float(m_netFrames) / 16.667F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));

        m_netState = RS_NET_IDLE;

        m_networkWatchdog.stop();
        m_netTimeoutTimer.stop();
        m_packetTimer.stop();
        m_netTimeout = false;

        m_netFrames = 0U;
        m_netLost = 0U;

        m_netErrs = 0U;
        m_netBits = 1U;

        m_dstNetwork->writeP25TDU(m_p25LC, m_p25LSD);

        m_p25LC.reset();
        m_p25N = 0U;

        ::memset(m_netLDU1, 0x00U, 9U * 25U);
        ::memset(m_netLDU2, 0x00U, 9U * 25U);
        break;
    }

    case DT_VOICE_LC_HEADER:
    case DT_VOICE_PI_HEADER:
    case DT_DATA_HEADER:
    case DT_CSBK:
    case DT_RATE_12_DATA:
    case DT_RATE_34_DATA:
    case DT_RATE_1_DATA:
    default:
        break;
    }
}

/// <summary>
/// Updates the DMR slot processor.
/// </summary>
void Slot::clock()
{
    uint32_t ms = m_interval.elapsed();
    m_interval.start();

    m_netTimeoutTimer.clock(ms);
    if (m_netTimeoutTimer.isRunning() && m_netTimeoutTimer.hasExpired()) {
        if (!m_netTimeout) {
            LogMessage(LOG_NET, "DMR Slot %u, user has timed out", m_slotNo);
            m_netTimeout = true;
        }
    }

    if (m_netState == RS_NET_AUDIO) {
        m_networkWatchdog.clock(ms);

        if (m_networkWatchdog.hasExpired()) {
            if (m_netState == RS_NET_AUDIO) {
                // We've received the voice header haven't we?
                m_netFrames += 1U;
                ::LogInfoEx(LOG_DMR, "Slot %u network watchdog has expired, %.1f seconds, %u%% packet loss, BER: %.1f%%",
                    m_slotNo, float(m_netFrames) / 16.667F, (m_netLost * 100U) / m_netFrames, float(m_netErrs * 100U) / float(m_netBits));
            }
            else {
                ::LogInfoEx(LOG_DMR, "Slot %u network watchdog has expired", m_slotNo);
            }

            m_netState = RS_NET_IDLE;

            m_networkWatchdog.stop();
            m_netTimeoutTimer.stop();
            m_packetTimer.stop();
            m_netTimeout = false;

            m_netFrames = 0U;
            m_netLost = 0U;

            m_netErrs = 0U;
            m_netBits = 1U;

            m_dstNetwork->writeP25TDU(m_p25LC, m_p25LSD);

            m_p25LC.reset();
            m_p25N = 0U;

            ::memset(m_netLDU1, 0x00U, 9U * 25U);
            ::memset(m_netLDU2, 0x00U, 9U * 25U);
        }
    }

    if (m_netState == RS_NET_AUDIO) {
        m_packetTimer.clock(ms);

        if (m_packetTimer.isRunning() && m_packetTimer.hasExpired()) {
            uint32_t elapsed = m_elapsed.elapsed();
            if (elapsed >= m_jitterTime) {
                LogWarning(LOG_NET, "DMR Slot %u, lost audio for %ums filling in", m_slotNo, elapsed);
                //m_voice->insertSilence(m_jitterSlots);
                m_elapsed.start();
            }

            m_packetTimer.start();
        }
    }
}

/// <summary>
/// Helper to initialize the DMR slot processor.
/// </summary>
/// <param name="jitter"></param>
/// <param name="gainAdjust"></param>
void Slot::init(uint32_t jitter, float gainAdjust)
{
    m_mbeDecode = new vocoder::MBEDecoder(vocoder::DECODE_DMR_AMBE);
    m_mbeEncode = new vocoder::MBEEncoder(vocoder::ENCODE_88BIT_IMBE);
    m_mbeEncode->setGainAdjust(gainAdjust);

    m_jitterTime = jitter;

    float jitter_tmp = float(jitter) / 360.0F;
    m_jitterSlots = (uint32_t)(std::ceil(jitter_tmp) * 6.0F);

    m_idle = new uint8_t[DMR_FRAME_LENGTH_BYTES + 2U];
    ::memcpy(m_idle, DMR_IDLE_DATA, DMR_FRAME_LENGTH_BYTES + 2U);

    // Generate the Slot Type for the Idle frame
    SlotType slotType;
    slotType.setColorCode(0U);
    slotType.setDataType(DT_IDLE);
    slotType.encode(m_idle + 2U);
}

/// <summary>
///
/// </summary>
/// <param name="ambe"></param>
void Slot::decodeAndProcessAMBE(uint8_t* ambe)
{
    if (m_netState == RS_NET_IDLE) {
        m_netState = RS_NET_AUDIO;

        // setup P25 LC data
        p25::lc::LC lc;
        lc.setSrcId(m_netLC->getSrcId());
        lc.setDstId(m_netLC->getDstId());

        if (m_netLC->getFLCO() == FLCO_GROUP) {
            lc.setGroup(true);
        }
        else {
            lc.setGroup(false);
        }

        m_p25LC = lc;
        m_p25N = 0U;
    }

    if (m_p25N > 17U) {
        m_p25N = 0U;
    }

    if (m_p25N == 0U) {
        ::memset(m_netLDU1, 0x00U, 9U * 25U);
    }

    if (m_p25N == 9U) {
        ::memset(m_netLDU2, 0x00U, 9U * 25U);
    }

    for (uint8_t n = 0; n < AMBE_PER_SLOT; n++) {
        // decode AMBE into PCM
        int16_t pcmSamples[160U];
        ::memset(pcmSamples, 0x00U, 160U);

        uint8_t ambePartial[9U];
        ::memset(ambePartial, 0x00U, 9U);
        ::memcpy(ambePartial, ambe + (n * 9U), 9U);

        int32_t errs = m_mbeDecode->decode(ambePartial, pcmSamples);
        if (m_debug) {
            LogDebug(LOG_DMR, "decoded AMBE VC%u, errs = %u", n, errs);
        }

        // encode PCM into IMBE
        uint8_t imbe[11U];
        ::memset(imbe, 0x00U, 11U);

        m_mbeEncode->encode(pcmSamples, imbe);

        switch (m_p25N) {
            // LDU1
        case 0U:
            ::memcpy(m_netLDU1 + 10U, imbe, 11U);
            break;
        case 1U:
            ::memcpy(m_netLDU1 + 26U, imbe, 11U);
            break;
        case 2U:
            ::memcpy(m_netLDU1 + 55U, imbe, 11U);
            break;
        case 3U:
            ::memcpy(m_netLDU1 + 80U, imbe, 11U);
            break;
        case 4U:
            ::memcpy(m_netLDU1 + 105U, imbe, 11U);
            break;
        case 5U:
            ::memcpy(m_netLDU1 + 130U, imbe, 11U);
            break;
        case 6U:
            ::memcpy(m_netLDU1 + 155U, imbe, 11U);
            break;
        case 7U:
            ::memcpy(m_netLDU1 + 180U, imbe, 11U);
            break;
        case 8U:
            ::memcpy(m_netLDU1 + 204U, imbe, 11U);
            break;

            // LDU2
        case 9U:
            ::memcpy(m_netLDU2 + 10U, imbe, 11U);
            break;
        case 10U:
            ::memcpy(m_netLDU2 + 26U, imbe, 11U);
            break;
        case 11U:
            ::memcpy(m_netLDU2 + 55U, imbe, 11U);
            break;
        case 12U:
            ::memcpy(m_netLDU2 + 80U, imbe, 11U);
            break;
        case 13U:
            ::memcpy(m_netLDU2 + 105U, imbe, 11U);
            break;
        case 14U:
            ::memcpy(m_netLDU2 + 130U, imbe, 11U);
            break;
        case 15U:
            ::memcpy(m_netLDU2 + 155U, imbe, 11U);
            break;
        case 16U:
            ::memcpy(m_netLDU2 + 180U, imbe, 11U);
            break;
        case 17U:
            ::memcpy(m_netLDU2 + 204U, imbe, 11U);
            break;
        }

        // send P25 LDU1
        if (m_p25N == 8U) {
            if (m_verbose) {
                LogMessage(LOG_DMR, P25_LDU1_STR " audio, srcId = %u, dstId = %u, group = %u, emerg = %u, encrypt = %u, prio = %u",
                    m_p25LC.getSrcId(), m_p25LC.getDstId(), m_p25LC.getGroup(), m_p25LC.getEmergency(), m_p25LC.getEncrypted(), m_p25LC.getPriority());
            }

            m_dstNetwork->writeP25LDU1(m_p25LC, m_p25LSD, m_netLDU1);
        }

        // send P25 LDU2
        if (m_p25N == 17U) {
            if (m_verbose) {
                LogMessage(LOG_DMR, P25_LDU2_STR " audio");
            }

            m_dstNetwork->writeP25LDU2(m_p25LC, m_p25LSD, m_netLDU2);
        }

        m_p25N++;
    }
}
