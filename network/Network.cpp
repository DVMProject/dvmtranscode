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
#include "Defines.h"
#include "edac/SHA256.h"
#include "network/Network.h"
#include "Log.h"
#include "StopWatch.h"
#include "Utils.h"

using namespace network;

#include <cstdio>
#include <cassert>
#include <cstdlib>

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the Network class.
/// </summary>
/// <param name="address">Network Hostname/IP address to connect to.</param>
/// <param name="port">Network port number.</param>
/// <param name="local"></param>
/// <param name="id">Unique ID of this modem on the network.</param>
/// <param name="password">Network authentication password.</param>
/// <param name="duplex">Flag indicating full-duplex operation.</param>
/// <param name="slot1">Flag indicating whether DMR slot 1 is enabled for network traffic.</param>
/// <param name="slot2">Flag indicating whether DMR slot 2 is enabled for network traffic.</param>
Network::Network(const std::string& address, uint32_t port, uint32_t local, uint32_t id, const std::string& password,
    bool duplex, bool debug, bool slot1, bool slot2) :
    BaseNetwork(local, id, duplex, debug, slot1, slot2),
    m_address(address),
    m_port(port),
    m_password(password),
    m_enabled(false),
    m_identity(),
    m_rxFrequency(0U),
    m_txFrequency(0U),
    m_txOffsetMhz(0.0F),
    m_chBandwidthKhz(0.0F),
    m_channelId(0U),
    m_channelNo(0U),
    m_power(0U),
    m_latitude(0.0F),
    m_longitude(0.0F),
    m_height(0),
    m_location()
{
    assert(!address.empty());
    assert(port > 0U);
    assert(!password.empty());
}

/// <summary>
/// Finalizes a instance of the Network class.
/// </summary>
Network::~Network()
{
    /* stub */
}

/// <summary>
/// Sets metadata configuration settings from the modem.
/// </summary>
/// <param name="identity"></param>
/// <param name="rxFrequency"></param>
/// <param name="txFrequency"></param>
/// <param name="txOffsetMhz"></param>
/// <param name="chBandwidthKhz"></param>
/// <param name="channelId"></param>
/// <param name="channelNo"></param>
/// <param name="power"></param>
/// <param name="latitude"></param>
/// <param name="longitude"></param>
/// <param name="height"></param>
/// <param name="location"></param>
void Network::setMetadata(const std::string& identity, uint32_t rxFrequency, uint32_t txFrequency, float txOffsetMhz, float chBandwidthKhz,
    uint8_t channelId, uint32_t channelNo, uint32_t power, float latitude, float longitude, int height, const std::string& location)
{
    m_identity = identity;
    m_rxFrequency = rxFrequency;
    m_txFrequency = txFrequency;

    m_txOffsetMhz = txOffsetMhz;
    m_chBandwidthKhz = chBandwidthKhz;
    m_channelId = channelId;
    m_channelNo = channelNo;

    m_power = power;
    m_latitude = latitude;
    m_longitude = longitude;
    m_height = height;
    m_location = location;
}

/// <summary>
/// Gets the current status of the network.
/// </summary>
/// <returns></returns>
uint8_t Network::getStatus()
{
    return m_status;
}

/// <summary>
/// Updates the timer by the passed number of milliseconds.
/// </summary>
/// <param name="ms"></param>
void Network::clock(uint32_t ms)
{
    if (m_status == NET_STAT_WAITING_CONNECT) {
        m_retryTimer.clock(ms);
        if (m_retryTimer.isRunning() && m_retryTimer.hasExpired()) {
            bool ret = m_socket.open(m_addr.ss_family);
            if (ret) {
                ret = writeLogin();
                if (!ret)
                    return;

                m_status = NET_STAT_WAITING_LOGIN;
                m_timeoutTimer.start();
            }

            m_retryTimer.start();
        }

        return;
    }

    sockaddr_storage address;
    unsigned int addrLen;
    int length = m_socket.read(m_buffer, DATA_PACKET_LENGTH, address, addrLen);
    if (length < 0) {
        LogError(LOG_NET, "Socket has failed, retrying connection to the master");
        close();
        open();
        return;
    }

    if (m_debug && length > 0)
        Utils::dump(1U, "Network Received", m_buffer, length);

    if (length > 0) {
        if (!UDPSocket::match(m_addr, address)) {
            LogError(LOG_NET, "Packet received from an invalid source");
            return;
        }

        if (::memcmp(m_buffer, TAG_DMR_DATA, 4U) == 0) {
            if (m_enabled) {
                if (m_debug)
                    Utils::dump(1U, "Network Received, DMR", m_buffer, length);

                uint8_t len = length;
                m_rxDMRData.addData(&len, 1U);
                m_rxDMRData.addData(m_buffer, len);
            }
        }
        else if (::memcmp(m_buffer, TAG_P25_DATA, 4U) == 0) {
            if (m_enabled) {
                if (m_debug)
                    Utils::dump(1U, "Network Received, P25", m_buffer, length);

                uint8_t len = length;
                m_rxP25Data.addData(&len, 1U);
                m_rxP25Data.addData(m_buffer, len);
            }
        }
        else if (::memcmp(m_buffer, TAG_MASTER_WL_RID, 7U) == 0) {
            // ignore
        }
        else if (::memcmp(m_buffer, TAG_MASTER_BL_RID, 7U) == 0) {
            // ignore
        }
        else if (::memcmp(m_buffer, TAG_MASTER_ACTIVE_TGS, 6U) == 0) {
            // ignore
        }
        else if (::memcmp(m_buffer, TAG_MASTER_DEACTIVE_TGS, 7U) == 0) {
            // ignore
        }
        else if (::memcmp(m_buffer, TAG_MASTER_NAK, 6U) == 0) {
            if (m_status == NET_STAT_RUNNING) {
                LogWarning(LOG_NET, "Master returned a NAK; attemping to relogin ...");
                m_status = NET_STAT_WAITING_LOGIN;
                m_timeoutTimer.start();
                m_retryTimer.start();
            }
            else {
                LogError(LOG_NET, "Master returned a NAK; network reconnect ...");
                close();
                open();
                return;
            }
        }
        else if (::memcmp(m_buffer, TAG_REPEATER_ACK, 6U) == 0) {
            switch (m_status) {
                case NET_STAT_WAITING_LOGIN:
                    LogDebug(LOG_NET, "Sending authorisation");
                    ::memcpy(m_salt, m_buffer + 6U, sizeof(uint32_t));
                    writeAuthorisation();
                    m_status = NET_STAT_WAITING_AUTHORISATION;
                    m_timeoutTimer.start();
                    m_retryTimer.start();
                    break;
                case NET_STAT_WAITING_AUTHORISATION:
                    LogDebug(LOG_NET, "Sending configuration");
                    writeConfig();
                    m_status = NET_STAT_WAITING_CONFIG;
                    m_timeoutTimer.start();
                    m_retryTimer.start();
                    break;
                case NET_STAT_WAITING_CONFIG:
                    LogMessage(LOG_NET, "Logged into the master successfully");
                    m_status = NET_STAT_RUNNING;
                    m_timeoutTimer.start();
                    m_retryTimer.start();
                    break;
                default:
                    break;
            }
        }
        else if (::memcmp(m_buffer, TAG_MASTER_CLOSING, 5U) == 0) {
            LogError(LOG_NET, "Master is closing down");
            close();
            open();
        }
        else if (::memcmp(m_buffer, TAG_MASTER_PONG, 7U) == 0) {
            m_timeoutTimer.start();
        }
        else {
            Utils::dump("Unknown packet from the master", m_buffer, length);
        }
    }

    m_retryTimer.clock(ms);
    if (m_retryTimer.isRunning() && m_retryTimer.hasExpired()) {
        switch (m_status) {
            case NET_STAT_WAITING_LOGIN:
                writeLogin();
                break;
            case NET_STAT_WAITING_AUTHORISATION:
                writeAuthorisation();
                break;
            case NET_STAT_WAITING_CONFIG:
                writeConfig();
                break;
            case NET_STAT_RUNNING:
                writePing();
                break;
            default:
                break;
        }

        m_retryTimer.start();
    }

    m_timeoutTimer.clock(ms);
    if (m_timeoutTimer.isRunning() && m_timeoutTimer.hasExpired()) {
        LogError(LOG_NET, "Connection to the master has timed out, retrying connection");
        close();
        open();
    }
}

/// <summary>
/// Opens connection to the network.
/// </summary>
/// <returns></returns>
bool Network::open()
{
    if (m_debug)
        LogMessage(LOG_NET, "Opening Network");

    if (UDPSocket::lookup(m_address, m_port, m_addr, m_addrLen) != 0) {
        LogMessage(LOG_NET, "Could not lookup the address of the master");
        return false;
    }

    m_status = NET_STAT_WAITING_CONNECT;
    m_timeoutTimer.stop();
    m_retryTimer.start();

    return true;
}

/// <summary>
/// Sets flag enabling network communication.
/// </summary>
/// <param name="enabled"></param>
void Network::enable(bool enabled)
{
    m_enabled = enabled;
}

/// <summary>
/// Closes connection to the network.
/// </summary>
void Network::close()
{
    if (m_debug)
        LogMessage(LOG_NET, "Closing Network");

    if (m_status == NET_STAT_RUNNING) {
        uint8_t buffer[9U];
        ::memcpy(buffer + 0U, TAG_REPEATER_CLOSING, 5U);
        __SET_UINT32(m_id, buffer, 5U);
        write(buffer, 9U);
    }

    m_socket.close();

    m_retryTimer.stop();
    m_timeoutTimer.stop();

    m_status = NET_STAT_WAITING_CONNECT;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Writes login request to the network.
/// </summary>
/// <returns></returns>
bool Network::writeLogin()
{
    uint8_t buffer[8U];

    ::memcpy(buffer + 0U, TAG_REPEATER_LOGIN, 4U);
    __SET_UINT32(m_id, buffer, 4U);

    if (m_debug)
        Utils::dump(1U, "Network Transmitted, Login", buffer, 8U);

    return write(buffer, 8U);
}

/// <summary>
/// Writes network authentication challenge.
/// </summary>
/// <returns></returns>
bool Network::writeAuthorisation()
{
    size_t size = m_password.size();

    uint8_t* in = new uint8_t[size + sizeof(uint32_t)];
    ::memcpy(in, m_salt, sizeof(uint32_t));
    for (size_t i = 0U; i < size; i++)
        in[i + sizeof(uint32_t)] = m_password.at(i);

    uint8_t out[40U];
    ::memcpy(out + 0U, TAG_REPEATER_AUTH, 4U);
    __SET_UINT32(m_id, out, 4U);

    edac::SHA256 sha256;
    sha256.buffer(in, (uint32_t)(size + sizeof(uint32_t)), out + 8U);

    delete[] in;

    if (m_debug)
        Utils::dump(1U, "Network Transmitted, Authorisation", out, 40U);

    return write(out, 40U);
}

/// <summary>
/// Writes modem configuration to the network.
/// </summary>
/// <returns></returns>
bool Network::writeConfig()
{
    const char* software = "TCD_DMR_P25";
    char buffer[168U];

    ::memcpy(buffer + 0U, TAG_REPEATER_CONFIG, 4U);
    __SET_UINT32(m_id, buffer, 4U);

    char latitude[11U];
    ::sprintf(latitude, "%08f", m_latitude);

    char longitude[11U];
    ::sprintf(longitude, "%09f", m_longitude);

    char chBandwidthKhz[6U];
    ::sprintf(chBandwidthKhz, "%02.02f", m_chBandwidthKhz);

    char txOffsetMhz[6U];
    ::sprintf(txOffsetMhz, "%02.02f", m_txOffsetMhz);

    char channelId[4U];
    ::sprintf(channelId, "%d", m_channelId);

    char channelNo[5U];
    ::sprintf(channelNo, "%d", m_channelNo);

    int power = m_power;
    if (m_power > 99U)
        power = 99U;

    int height = m_height;
    if (m_height > 999)
        height = 999;

    //                      IdntRX  TX  RsrvLatLngHghtLoctnRsrvTxOfChBnChIdChNoPowrSftwrRsrvRcnPsRcPt
    ::sprintf(buffer + 8U, "%-8s%09u%09u%10s%8s%9s%03d%-20s%10s%-5s%-5s%-3s%-4s%02d%-16s%10s%-20s%05d", 
            m_identity.c_str(), m_rxFrequency, m_txFrequency,
        "", latitude, longitude, height, m_location.c_str(),
        "", txOffsetMhz, chBandwidthKhz, channelId, channelNo, power, software,
        "", "", 0);

    if (m_debug)
        Utils::dump(1U, "Network Transmitted, Configuration", (uint8_t*)buffer, 168U);

    return write((uint8_t*)buffer, 168U);
}

/// <summary>
/// Writes a network stay-alive ping.
/// </summary>
bool Network::writePing()
{
    uint8_t buffer[11U];

    ::memcpy(buffer + 0U, TAG_REPEATER_PING, 7U);
    __SET_UINT32(m_id, buffer, 7U);

    if (m_debug)
        Utils::dump(1U, "Network Transmitted, Ping", buffer, 11U);

    return write(buffer, 11U);
}
