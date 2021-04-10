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
*   Copyright (C) 2016,2017,2018 by Jonathan Naylor G4KLX
*   Copyright (C) 2017-2021 by Bryan Biedenkapp N2PLL
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
#include "Defines.h"
#include "dmr/data/Data.h"
#include "dmr/data/EmbeddedData.h"
#include "dmr/data/EMB.h"
#include "dmr/lc/FullLC.h"
#include "dmr/SlotType.h"
#include "dmr/Sync.h"
#include "p25/P25Defines.h"
#include "p25/Transcode.h"
#include "p25/P25Utils.h"
#include "edac/CRC.h"
#include "HostMain.h"
#include "Log.h"
#include "Utils.h"

using namespace p25;

#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the Transcode class.
/// </summary>
/// <param name="network">Instance of the BaseNetwork class representing the source network.</param>
/// <param name="network">Instance of the BaseNetwork class representing the destination network.</param>
/// <param name="timeout">Transmit timeout.</param>
/// <param name="gainAdjust"></param>
/// <param name="debug">Flag indicating whether P25 debug is enabled.</param>
/// <param name="verbose">Flag indicating whether P25 verbose logging is enabled.</param>
Transcode::Transcode(network::BaseNetwork* srcNetwork, network::BaseNetwork* dstNetwork, uint32_t timeout, float gainAdjust, bool debug, bool verbose) :
    m_srcNetwork(srcNetwork),
    m_dstNetwork(dstNetwork),
    m_netState(RS_NET_IDLE),
    m_netTimeout(1000U, timeout),
    m_networkWatchdog(1000U, 0U, 1500U),
    m_netFrames(0U),
    m_netLost(0U),
    m_netLC(),
    m_netLDU1(NULL),
    m_netLDU2(NULL),
    m_lastIMBE(NULL),
    m_ambeBuffer(NULL),
    m_ambeCount(0U),
    m_dmrSeqNo(0U),
    m_dmrN(0U),
    m_embeddedData(),
    m_mbeDecode(NULL),
    m_mbeEncode(NULL),
    m_verbose(verbose),
    m_debug(debug)
{
    assert(srcNetwork != NULL);
    assert(dstNetwork != NULL);

    m_netLDU1 = new uint8_t[9U * 25U];
    m_netLDU2 = new uint8_t[9U * 25U];

    ::memset(m_netLDU1, 0x00U, 9U * 25U);
    ::memset(m_netLDU2, 0x00U, 9U * 25U);

    m_lastIMBE = new uint8_t[11U];
    ::memcpy(m_lastIMBE, P25_NULL_IMBE, 11U);

    m_ambeBuffer = new uint8_t[dmr::DMR_AMBE_LENGTH_BYTES];
    ::memset(m_ambeBuffer, 0x00U, dmr::DMR_AMBE_LENGTH_BYTES);

    m_mbeDecode = new vocoder::MBEDecoder(vocoder::DECODE_88BIT_IMBE);
    m_mbeEncode = new vocoder::MBEEncoder(vocoder::ENCODE_DMR_AMBE);
    m_mbeEncode->setGainAdjust(gainAdjust);
}

/// <summary>
/// Finalizes a instance of the Transcode class.
/// </summary>
Transcode::~Transcode()
{
    delete[] m_netLDU1;
    delete[] m_netLDU2;
    delete[] m_lastIMBE;
    delete m_ambeBuffer;
    delete m_mbeDecode;
    delete m_mbeEncode;
}

/// <summary>
/// Updates the processor by the passed number of milliseconds.
/// </summary>
/// <param name="ms"></param>
void Transcode::clock(uint32_t ms)
{
    if (m_srcNetwork != NULL) {
        processNetwork();
    }

    m_netTimeout.clock(ms);

    if (m_netState == RS_NET_AUDIO) {
        m_networkWatchdog.clock(ms);

        if (m_networkWatchdog.hasExpired()) {
            if (m_netState == RS_NET_AUDIO) {
                ::LogInfoEx(LOG_P25, "network watchdog has expired, %.1f seconds, %u%% packet loss",
                    float(m_netFrames) / 50.0F, (m_netLost * 100U) / m_netFrames);

                writeNet_DMR_Terminator();
            }
            else {
                ::LogInfoEx(LOG_P25, "network watchdog has expired");
            }

            m_netState = RS_NET_IDLE;

            m_networkWatchdog.stop();
            m_netTimeout.stop();
        }
    }
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Process a data frames from the network.
/// </summary>
void Transcode::processNetwork()
{
    lc::LC control;
    data::LowSpeedData lsd;
    uint8_t duid;

    uint32_t length = 100U;
    bool ret = false;
    uint8_t* data = m_srcNetwork->readP25(ret, control, lsd, duid, length);
    if (!ret)
        return;
    if (length == 0U)
        return;
    if (data == NULL) {
        m_srcNetwork->resetP25();
        return;
    }

    m_networkWatchdog.start();

    if (m_debug) {
        Utils::dump(2U, "!!! *RX P25 Network Frame - Data Bytes", data, length);
    }

    uint32_t count = 0U;

    switch (duid) {
    case P25_DUID_LDU1:
        // The '62', '63', '64', '65', '66', '67', '68', '69', '6A' records are LDU1
        if ((data[0U] == 0x62U) && (data[22U] == 0x63U) &&
            (data[36U] == 0x64U) && (data[53U] == 0x65U) &&
            (data[70U] == 0x66U) && (data[87U] == 0x67U) &&
            (data[104U] == 0x68U) && (data[121U] == 0x69U) &&
            (data[138U] == 0x6AU)) {
            // The '62' record - IMBE Voice 1
            ::memcpy(m_netLDU1 + 0U, data + count, 22U);
            count += 22U;

            // The '63' record - IMBE Voice 2
            ::memcpy(m_netLDU1 + 25U, data + count, 14U);
            count += 14U;

            // The '64' record - IMBE Voice 3 + Link Control
            ::memcpy(m_netLDU1 + 50U, data + count, 17U);
            count += 17U;

            // The '65' record - IMBE Voice 4 + Link Control
            ::memcpy(m_netLDU1 + 75U, data + count, 17U);
            count += 17U;

            // The '66' record - IMBE Voice 5 + Link Control
            ::memcpy(m_netLDU1 + 100U, data + count, 17U);
            count += 17U;

            // The '67' record - IMBE Voice 6 + Link Control
            ::memcpy(m_netLDU1 + 125U, data + count, 17U);
            count += 17U;

            // The '68' record - IMBE Voice 7 + Link Control
            ::memcpy(m_netLDU1 + 150U, data + count, 17U);
            count += 17U;

            // The '69' record - IMBE Voice 8 + Link Control
            ::memcpy(m_netLDU1 + 175U, data + count, 17U);
            count += 17U;

            // The '6A' record - IMBE Voice 9 + Low Speed Data
            ::memcpy(m_netLDU1 + 200U, data + count, 16U);
            count += 16U;

            checkNet_LDU2(control, lsd);
            if (m_netState != RS_NET_IDLE) {
                writeNet_LDU1(control, lsd);
            }
        }
        break;
    case P25_DUID_LDU2:
        // The '6B', '6C', '6D', '6E', '6F', '70', '71', '72', '73' records are LDU2
        if ((data[0U] == 0x6BU) && (data[22U] == 0x6CU) &&
            (data[36U] == 0x6DU) && (data[53U] == 0x6EU) &&
            (data[70U] == 0x6FU) && (data[87U] == 0x70U) &&
            (data[104U] == 0x71U) && (data[121U] == 0x72U) &&
            (data[138U] == 0x73U)) {
            // The '6B' record - IMBE Voice 10
            ::memcpy(m_netLDU2 + 0U, data + count, 22U);
            count += 22U;

            // The '6C' record - IMBE Voice 11
            ::memcpy(m_netLDU2 + 25U, data + count, 14U);
            count += 14U;

            // The '6D' record - IMBE Voice 12 + Encryption Sync
            ::memcpy(m_netLDU2 + 50U, data + count, 17U);
            count += 17U;

            // The '6E' record - IMBE Voice 13 + Encryption Sync
            ::memcpy(m_netLDU2 + 75U, data + count, 17U);
            count += 17U;

            // The '6F' record - IMBE Voice 14 + Encryption Sync
            ::memcpy(m_netLDU2 + 100U, data + count, 17U);
            count += 17U;

            // The '70' record - IMBE Voice 15 + Encryption Sync
            ::memcpy(m_netLDU2 + 125U, data + count, 17U);
            count += 17U;

            // The '71' record - IMBE Voice 16 + Encryption Sync
            ::memcpy(m_netLDU2 + 150U, data + count, 17U);
            count += 17U;

            // The '72' record - IMBE Voice 17 + Encryption Sync
            ::memcpy(m_netLDU2 + 175U, data + count, 17U);
            count += 17U;

            // The '73' record - IMBE Voice 18 + Low Speed Data
            ::memcpy(m_netLDU2 + 200U, data + count, 16U);
            count += 16U;

            if (m_netState == RS_NET_IDLE) {
                writeNet_LDU1(control, lsd);
            }
            else {
                checkNet_LDU1(control, lsd);
            }

            if (m_netState != RS_NET_IDLE) {
                writeNet_LDU2(control, lsd);
            }
        }
        break;
    case P25_DUID_TDU:
    case P25_DUID_TDULC:
        if (m_netState != RS_NET_IDLE) {
            writeNet_DMR_Terminator();

            m_netState = RS_NET_IDLE;

            m_netTimeout.stop();
            m_networkWatchdog.stop();
            m_netLC.reset();
        }
        break;

    case P25_DUID_PDU:
    case P25_DUID_TSDU:
        break;
    }

    delete data;
}

/// <summary>
///
/// </summary>
void Transcode::writeNet_DMR_Terminator()
{
    // send DMR voice header
    dmr::data::Data dmrData;
    dmrData.setSrcId(m_netLC.getSrcId());
    dmrData.setDstId(m_netLC.getDstId());

    if (m_netLC.getGroup()) {
        dmrData.setFLCO(dmr::FLCO_GROUP);
    }
    else {
        dmrData.setFLCO(dmr::FLCO_PRIVATE);
    }

    uint32_t n = (m_dmrSeqNo - 3U) % 6U;
    uint32_t fill = (6U - n);

    if (n > 0U) {
        for (uint32_t i = 0U; i < fill; i++) {
            dmrData.setSeqNo(m_dmrSeqNo);
            dmrData.setN(n);

            dmrData.setDataType(dmr::DT_VOICE);

            // generate DMR AMBE data
            uint8_t data[dmr::DMR_FRAME_LENGTH_BYTES];
            ::memset(data, 0x00U, dmr::DMR_FRAME_LENGTH_BYTES);
            ::memcpy(data, dmr::DMR_SILENCE_DATA, dmr::DMR_FRAME_LENGTH_BYTES);

            uint8_t lcss = m_embeddedData.getData(data, m_dmrN);

            // generated embedded signalling
            dmr::data::EMB emb;
            emb.setColorCode(0U);
            emb.setLCSS(lcss);
            emb.encode(data);

            if (m_verbose) {
                LogMessage(LOG_P25, "DMR, DT_VOICE audio, sequence no = %u", m_dmrN);
            }

            dmrData.setData(data);

            m_dstNetwork->writeDMR(dmrData);

            n++;
            m_dmrSeqNo++;
        }
    }

    dmrData.setDataType(dmr::DT_TERMINATOR_WITH_LC);

    uint8_t data[dmr::DMR_FRAME_LENGTH_BYTES];
    ::memset(data, 0x00U, dmr::DMR_FRAME_LENGTH_BYTES);

    // generate LC
    dmr::lc::LC dmrLC = dmr::lc::LC(dmrData.getFLCO(), dmrData.getSrcId(), dmrData.getDstId());

    // generate the Slot Type
    dmr::SlotType slotType;
    slotType.setDataType(dmr::DT_TERMINATOR_WITH_LC);
    slotType.encode(data);

    dmr::lc::FullLC fullLC;
    fullLC.encode(dmrLC, data, dmr::DT_TERMINATOR_WITH_LC);

    // Convert the Data Sync to be from the BS or MS as needed
    dmr::Sync::addDMRDataSync(data, true); // hardcoded to duplex?

    if (m_verbose) {
        LogMessage(LOG_P25, "DMR, end of voice transmission");
    }

    dmrData.setData(data);

    m_dstNetwork->writeDMR(dmrData);

    m_ambeCount = 0U;
    m_dmrSeqNo = 0U;
    m_dmrN = 0U;
}

/// <summary>
///
/// </summary>
/// <param name="ldu"></param>
void Transcode::decodeAndProcessIMBE(uint8_t* ldu)
{
    if (m_netState == RS_NET_IDLE) {
        m_netState = RS_NET_AUDIO;

        m_ambeCount = 0U;
        m_dmrSeqNo = 0U;
    }

    for (uint8_t n = 0; n < 9; n++) {
        m_dmrN = m_dmrSeqNo % 6U;

        if (m_ambeCount == dmr::AMBE_PER_SLOT) {
            if (m_dmrSeqNo == 0U) {
                // send DMR voice header
                dmr::data::Data dmrData;
                dmrData.setSrcId(m_netLC.getSrcId());
                dmrData.setDstId(m_netLC.getDstId());

                if (m_netLC.getGroup()) {
                    dmrData.setFLCO(dmr::FLCO_GROUP);
                }
                else {
                    dmrData.setFLCO(dmr::FLCO_PRIVATE);
                }

                dmrData.setDataType(dmr::DT_VOICE_LC_HEADER);

                uint8_t data[dmr::DMR_FRAME_LENGTH_BYTES];
                ::memset(data, 0x00U, dmr::DMR_FRAME_LENGTH_BYTES);

                // generate LC
                dmr::lc::LC dmrLC = dmr::lc::LC(dmrData.getFLCO(), dmrData.getSrcId(), dmrData.getDstId());
                m_embeddedData.setLC(dmrLC);

                // generate the Slot Type
                dmr::SlotType slotType;
                slotType.setDataType(dmr::DT_VOICE_LC_HEADER);
                slotType.encode(data);

                dmr::lc::FullLC fullLC;
                fullLC.encode(dmrLC, data, dmr::DT_VOICE_LC_HEADER);

                // Convert the Data Sync to be from the BS or MS as needed
                dmr::Sync::addDMRDataSync(data, true); // hardcoded to duplex?

                if (m_debug) {
                    Utils::dump(1U, "DMR Data Frame", data, dmr::DMR_FRAME_LENGTH_BYTES);
                }

                if (m_verbose) {
                    LogMessage(LOG_P25, "DMR, DT_VOICE_LC_HEADER, srcId = %u, dstId = %u, FLCO = $%02X, FID = $%02X, PF = %u", dmrLC.getSrcId(), dmrLC.getDstId(), dmrLC.getFLCO(), dmrLC.getFID(), dmrLC.getPF());
                }

                dmrData.setData(data);

                m_dstNetwork->writeDMR(dmrData);
            }

            // send DMR voice
            dmr::data::Data dmrData;
            dmrData.setSrcId(m_netLC.getSrcId());
            dmrData.setDstId(m_netLC.getDstId());
            dmrData.setSeqNo(m_dmrSeqNo);
            dmrData.setN(m_dmrN);

            if (m_netLC.getGroup()) {
                dmrData.setFLCO(dmr::FLCO_GROUP);
            }
            else {
                dmrData.setFLCO(dmr::FLCO_PRIVATE);
            }

            // generate DMR AMBE data
            uint8_t data[dmr::DMR_FRAME_LENGTH_BYTES];
            ::memset(data, 0x00U, dmr::DMR_FRAME_LENGTH_BYTES);

            if (m_debug) {
                Utils::dump(1U, "DMR AMBE Buffer", m_ambeBuffer, dmr::DMR_FRAME_LENGTH_BYTES);
            }

            ::memcpy(data, m_ambeBuffer, 13U);
            data[13U] = m_ambeBuffer[13U] & 0xF0U;
            data[19U] = m_ambeBuffer[13U] & 0x0FU;
            ::memcpy(data + 20U, m_ambeBuffer + 14U, 13U);

            if (m_debug) {
                Utils::dump(1U, "DMR Data Frame", data, dmr::DMR_FRAME_LENGTH_BYTES);
            }

            if (m_dmrN == 0U) {
                dmrData.setDataType(dmr::DT_VOICE_SYNC);

                // Convert the Voice Sync to be from the BS or MS as needed
                dmr::Sync::addDMRAudioSync(data, true); // hardcoded to duplex?

                if (m_verbose) {
                    LogMessage(LOG_P25, "DMR, DT_VOICE_SYNC audio, sequence no = %u", m_dmrN);
                }
            }
            else {
                dmrData.setDataType(dmr::DT_VOICE);

                uint8_t lcss = m_embeddedData.getData(data, m_dmrN);

                // generated embedded signalling
                dmr::data::EMB emb;
                emb.setColorCode(0U);
                emb.setLCSS(lcss);
                emb.encode(data);

                if (m_verbose) {
                    LogMessage(LOG_P25, "DMR, DT_VOICE audio, sequence no = %u", m_dmrN);
                }
            }

            dmrData.setData(data);

            m_dstNetwork->writeDMR(dmrData);
            m_dmrSeqNo++;
        
            // clear AMBE buffer
            ::memset(m_ambeBuffer, 0x00U, dmr::DMR_AMBE_LENGTH_BYTES);
            m_ambeCount = 0U;
        }

        // get P25 IMBE codeword
        uint8_t imbe[11U];
        ::memset(imbe, 0x00U, 11U);

        switch (n) {
        case 0:
            ::memcpy(imbe, ldu + 10U, 11U);
            break;
        case 1:
            ::memcpy(imbe, ldu + 26U, 11U);
            break;
        case 2:
            ::memcpy(imbe, ldu + 55U, 11U);
            break;
        case 3:
            ::memcpy(imbe, ldu + 80U, 11U);
            break;
        case 4:
            ::memcpy(imbe, ldu + 105U, 11U);
            break;
        case 5:
            ::memcpy(imbe, ldu + 130U, 11U);
            break;
        case 6:
            ::memcpy(imbe, ldu + 155U, 11U);
            break;
        case 7:
            ::memcpy(imbe, ldu + 180U, 11U);
            break;
        case 8:
            ::memcpy(imbe, ldu + 204U, 11U);
            break;
        }

        // decode IMBE into PCM
        int16_t pcmSamples[160U];
        ::memset(pcmSamples, 0x00U, 160U);

        int32_t errs = m_mbeDecode->decode(imbe, pcmSamples);
        if (m_debug) {
            LogDebug(LOG_P25, "decoded IMBE VC%u, errs = %u", n, errs);
        }

        // encode PCM into AMBE
        uint8_t ambe[9U];
        ::memset(ambe, 0x00U, 9U);

        m_mbeEncode->encode(pcmSamples, ambe);
        if (m_debug) {
            Utils::dump(1U, "DMR AMBE", ambe, 9U);
        }

        ::memcpy(m_ambeBuffer + (m_ambeCount * 9U), ambe, 9U);
        m_ambeCount++;
    }
}

/// <summary>
/// Helper to check for an unflushed LDU1 packet.
/// </summary>
/// <param name="control"></param>
/// <param name="lsd"></param>
void Transcode::checkNet_LDU1(const lc::LC& control, const data::LowSpeedData& lsd)
{
    if (m_netState == RS_NET_IDLE)
        return;

    // Check for an unflushed LDU1
    if (m_netLDU1[0U] != 0x00U || m_netLDU1[25U] != 0x00U || m_netLDU1[50U] != 0x00U ||
        m_netLDU1[75U] != 0x00U || m_netLDU1[100U] != 0x00U || m_netLDU1[125U] != 0x00U ||
        m_netLDU1[150U] != 0x00U || m_netLDU1[175U] != 0x00U || m_netLDU1[200U] != 0x00U)
        writeNet_LDU1(control, lsd);
}

/// <summary>
/// Helper to write a network P25 LDU1 packet.
/// </summary>
/// <param name="control"></param>
/// <param name="lsd"></param>
void Transcode::writeNet_LDU1(const lc::LC& control, const data::LowSpeedData& lsd)
{
    uint8_t lco = control.getLCO();
    uint8_t mfId = control.getMFId();
    uint32_t dstId = control.getDstId();
    uint32_t srcId = control.getSrcId();
    bool group = control.getLCO() == LC_GROUP;

    uint8_t serviceOptions = (uint8_t)(m_netLDU1[53U]);

    m_netLC.reset();
    m_netLC.setLCO(lco);
    m_netLC.setMFId(mfId);
    m_netLC.setSrcId(srcId);
    m_netLC.setDstId(dstId);
    m_netLC.setGroup(group);
    m_netLC.setEmergency((serviceOptions & 0x80U) == 0x80U);
    m_netLC.setEncrypted((serviceOptions & 0x40U) == 0x40U);
    m_netLC.setPriority((serviceOptions & 0x07U));

    m_netTimeout.start();
    m_netFrames = 0U;
    m_netLost = 0U;

    insertMissingAudio(m_netLDU1);

    if (m_verbose) {
        uint32_t loss = 0;
        if (m_netFrames != 0) {
            loss = (m_netLost * 100U) / m_netFrames;
        }
        else {
            loss = (m_netLost * 100U) / 1U;
            if (loss > 100) {
                loss = 100;
            }
        }

        LogMessage(LOG_NET, P25_LDU1_STR " audio, srcId = %u, dstId = %u, group = %u, emerg = %u, encrypt = %u, prio = %u, %u%% packet loss",
            m_netLC.getSrcId(), m_netLC.getDstId(), m_netLC.getGroup(), m_netLC.getEmergency(), m_netLC.getEncrypted(), m_netLC.getPriority(), loss);
    }

    // Process the audio
    decodeAndProcessIMBE(m_netLDU1);

    ::memset(m_netLDU1, 0x00U, 9U * 25U);

    m_netFrames += 9U;
}

/// <summary>
/// Helper to check for an unflushed LDU2 packet.
/// </summary>
/// <param name="control"></param>
/// <param name="lsd"></param>
void Transcode::checkNet_LDU2(const lc::LC& control, const data::LowSpeedData& lsd)
{
    if (m_netState == RS_NET_IDLE)
        return;

    // Check for an unflushed LDU2
    if (m_netLDU2[0U] != 0x00U || m_netLDU2[25U] != 0x00U || m_netLDU2[50U] != 0x00U ||
        m_netLDU2[75U] != 0x00U || m_netLDU2[100U] != 0x00U || m_netLDU2[125U] != 0x00U ||
        m_netLDU2[150U] != 0x00U || m_netLDU2[175U] != 0x00U || m_netLDU2[200U] != 0x00U)
        writeNet_LDU2(control, lsd);
}

/// <summary>
/// Helper to write a network P25 LDU2 packet.
/// </summary>
/// <param name="control"></param>
/// <param name="lsd"></param>
void Transcode::writeNet_LDU2(const lc::LC& control, const data::LowSpeedData& lsd)
{
    uint8_t algId = m_netLDU2[126U];
    uint32_t kId = (m_netLDU2[127U] << 8) + m_netLDU2[128U];

    uint8_t mi[P25_MI_LENGTH_BYTES];
    ::memcpy(mi + 0U, m_netLDU2 + 51U, 3U);
    ::memcpy(mi + 3U, m_netLDU2 + 76U, 3U);
    ::memcpy(mi + 6U, m_netLDU2 + 101U, 3U);

    // Utils::dump(1U, "LDU2 Network MI", mi, P25_MI_LENGTH_BYTES);

    m_netLC.setMI(mi);
    m_netLC.setAlgId(algId);
    m_netLC.setKId(kId);

    insertMissingAudio(m_netLDU2);

    if (m_verbose) {
        uint32_t loss = 0;
        if (m_netFrames != 0) {
            loss = (m_netLost * 100U) / m_netFrames;
        }
        else {
            loss = (m_netLost * 100U) / 1U;
            if (loss > 100) {
                loss = 100;
            }
        }

        LogMessage(LOG_NET, P25_LDU2_STR " audio, algo = $%02X, kid = $%04X, %u%% packet loss", algId, kId, loss);
    }

    // Process the audio
    decodeAndProcessIMBE(m_netLDU2);

    ::memset(m_netLDU2, 0x00U, 9U * 25U);

    m_netFrames += 9U;
}

/// <summary>
/// Helper to insert IMBE silence frames for missing audio.
/// </summary>
/// <param name="data"></param>
void Transcode::insertMissingAudio(uint8_t* data)
{
    if (data[0U] == 0x00U) {
        ::memcpy(data + 10U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 10U, 11U);
    }

    if (data[25U] == 0x00U) {
        ::memcpy(data + 26U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 26U, 11U);
    }

    if (data[50U] == 0x00U) {
        ::memcpy(data + 55U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 55U, 11U);
    }

    if (data[75U] == 0x00U) {
        ::memcpy(data + 80U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 80U, 11U);
    }

    if (data[100U] == 0x00U) {
        ::memcpy(data + 105U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 105U, 11U);
    }

    if (data[125U] == 0x00U) {
        ::memcpy(data + 130U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 130U, 11U);
    }

    if (data[150U] == 0x00U) {
        ::memcpy(data + 155U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 155U, 11U);
    }

    if (data[175U] == 0x00U) {
        ::memcpy(data + 180U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 180U, 11U);
    }

    if (data[200U] == 0x00U) {
        ::memcpy(data + 204U, m_lastIMBE, 11U);
        m_netLost++;
    }
    else {
        ::memcpy(m_lastIMBE, data + 204U, 11U);
    }
}
