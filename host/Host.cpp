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
#include "network/UDPSocket.h"
#include "host/Host.h"
#include "dmr/Transcode.h"
#include "p25/Transcode.h"
#include "HostMain.h"
#include "Log.h"
#include "StopWatch.h"
#include "Thread.h"
#include "Utils.h"

using namespace network;

#include <cstdio>
#include <cstdarg>
#include <vector>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

// ---------------------------------------------------------------------------
//  Public Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the Host class.
/// </summary>
/// <param name="confFile">Full-path to the configuration file.</param>
Host::Host(const std::string& confFile) :
    m_confFile(confFile),
    m_conf(),
    m_srcNetwork(NULL),
    m_dstNetwork(NULL),
    m_timeout(180U),
    m_identity(),
    m_latitude(0.0F),
    m_longitude(0.0F),
    m_height(0),
    m_power(0U),
    m_location(),
    m_twoWayTranscode(false),
    m_tcVerbose(true),
    m_tcDebug(false)
{
    UDPSocket::startup();
}

/// <summary>
/// Finalizes a instance of the Host class.
/// </summary>
Host::~Host()
{
    UDPSocket::shutdown();
}

/// <summary>
/// Executes the main modem host processing loop.
/// </summary>
/// <returns>Zero if successful, otherwise error occurred.</returns>
int Host::run()
{
    bool ret = false;
    try {
        ret = yaml::Parse(m_conf, m_confFile.c_str());
        if (!ret) {
            ::fatal("cannot read the configuration file, %s\n", m_confFile.c_str());
        }
    }
    catch (yaml::OperationException e) {
        ::fatal("cannot read the configuration file, %s", e.message());
    }

    bool m_daemon = m_conf["daemon"].as<bool>(false);
    if (m_daemon && g_foreground)
        m_daemon = false;

    // initialize system logging
    yaml::Node logConf = m_conf["log"];
    ret = ::LogInitialise(logConf["filePath"].as<std::string>(), logConf["fileRoot"].as<std::string>(),
        logConf["fileLevel"].as<uint32_t>(0U), logConf["displayLevel"].as<uint32_t>(0U));
    if (!ret) {
        ::fatal("unable to open the log file\n");
    }

#if !defined(_WIN32) && !defined(_WIN64)
    // handle POSIX process forking
    if (m_daemon) {
        // create new process
        pid_t pid = ::fork();
        if (pid == -1) {
            ::fprintf(stderr, "%s: Couldn't fork() , exiting\n", g_progExe.c_str());
            ::LogFinalise();
            return EXIT_FAILURE;
        }
        else if (pid != 0) {
            ::LogFinalise();
            exit(EXIT_SUCCESS);
        }

        // create new session and process group
        if (::setsid() == -1) {
            ::fprintf(stderr, "%s: Couldn't setsid(), exiting\n", g_progExe.c_str());
            ::LogFinalise();
            return EXIT_FAILURE;
        }

        // set the working directory to the root directory
        if (::chdir("/") == -1) {
            ::fprintf(stderr, "%s: Couldn't cd /, exiting\n", g_progExe.c_str());
            ::LogFinalise();
            return EXIT_FAILURE;
        }

        ::close(STDIN_FILENO);
        ::close(STDOUT_FILENO);
        ::close(STDERR_FILENO);
    }
#endif

    getHostVersion();
    ::LogInfo(">> Protocol Transcoder");

    // read base parameters from configuration
    ret = readParams();
    if (!ret)
        return EXIT_FAILURE;

    // initialize source networking
    ret = createSrcNetwork();
    if (!ret)
        return EXIT_FAILURE;

    // initialize source networking
    ret = createDstNetwork();
    if (!ret)
        return EXIT_FAILURE;

    uint32_t jitter = m_conf["network"]["jitter"].as<uint32_t>(360U);

    // initialize DMR
    dmr::Transcode* dmrSrcTranscoder = new dmr::Transcode(m_dstNetwork, m_srcNetwork, m_timeout, jitter, m_tcDebug, m_tcVerbose);
    dmr::Transcode* dmrDstTranscoder = NULL;
    if (m_twoWayTranscode) {
         dmrDstTranscoder = new dmr::Transcode(m_srcNetwork, m_dstNetwork, m_timeout, jitter, m_tcDebug, m_tcVerbose);
    }

    // initialize P25
    p25::Transcode* p25SrcTranscoder = new p25::Transcode(m_srcNetwork, m_dstNetwork, m_timeout, m_tcDebug, m_tcVerbose);
    p25::Transcode* p25DstTranscoder = NULL;
    if (m_twoWayTranscode) {
        p25DstTranscoder = new p25::Transcode(m_dstNetwork, m_srcNetwork, m_timeout, m_tcDebug, m_tcVerbose);
    }

    StopWatch stopWatch;
    stopWatch.start();

    if (!g_killed) {
        ::LogInfoEx(LOG_HOST, "Host is performing late initialization and warmup");

        // perform early pumping of the modem clock (this is so the DSP has time to setup its buffers),
        // and clock the network (so it may perform early connect)
        uint32_t elapsedMs = 0U;
        while (!g_killed) {
            uint32_t ms = stopWatch.elapsed();
            stopWatch.start();

            elapsedMs += ms;

            if (m_srcNetwork != NULL)
                m_srcNetwork->clock(ms);
            if (m_dstNetwork != NULL)
                m_dstNetwork->clock(ms);

            if (ms < 2U)
                Thread::sleep(1U);

            if (elapsedMs > 15000U)
                break;
        }

        ::LogInfoEx(LOG_HOST, "Host is up and running");
        stopWatch.start();
    }

    bool killed = false;

    // main execution loop
    while (!killed) {
        uint32_t ms = stopWatch.elapsed();

        // ------------------------------------------------------
        //  -- Modem, DMR, P25 and Network Clocking           --
        // ------------------------------------------------------

        ms = stopWatch.elapsed();
        stopWatch.start();

        if (dmrSrcTranscoder != NULL)
            dmrSrcTranscoder->clock();
        if (dmrDstTranscoder != NULL)
            dmrDstTranscoder->clock();

        if (p25SrcTranscoder != NULL)
            p25SrcTranscoder->clock(ms);
        if (p25DstTranscoder != NULL)
            p25DstTranscoder->clock(ms);

        if (m_srcNetwork != NULL)
            m_srcNetwork->clock(ms);
        if (m_dstNetwork != NULL)
            m_dstNetwork->clock(ms);

        if (g_killed) {
            killed = true;
        }

        if (ms < 2U)
            Thread::sleep(1U);
    }

    if (dmrSrcTranscoder != NULL) {
        delete dmrSrcTranscoder;
    }

    if (dmrDstTranscoder != NULL) {
        delete dmrDstTranscoder;
    }

    if (p25SrcTranscoder != NULL) {
        delete p25SrcTranscoder;
    }

    if (p25DstTranscoder != NULL) {
        delete p25DstTranscoder;
    }

    if (m_srcNetwork != NULL) {
        m_srcNetwork->close();
        delete m_srcNetwork;
    }

    if (m_dstNetwork != NULL) {
        m_dstNetwork->close();
        delete m_dstNetwork;
    }

    return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Private Class Members
// ---------------------------------------------------------------------------
/// <summary>
/// Reads basic configuration parameters from the INI.
/// </summary>
bool Host::readParams()
{
    yaml::Node systemConf = m_conf["system"];
    m_timeout = systemConf["timeout"].as<uint32_t>(120U);
    m_identity = systemConf["identity"].as<std::string>();
    m_twoWayTranscode = systemConf["twoWayTranscode"].as<bool>(false);
    m_tcVerbose = systemConf["verbose"].as<bool>(true);
    m_tcDebug = systemConf["debug"].as<bool>(true);

    removeLockFile();

    LogInfo("General Parameters");
    LogInfo("    Timeout: %us", m_timeout);
    LogInfo("    Identity: %s", m_identity.c_str());
    LogInfo("    Lock Filename: %s", g_lockFile.c_str());
    LogInfo("    Two-way Transcode: %s", m_twoWayTranscode ? "enabled" : "disabled");

    if (m_tcVerbose) {
        LogInfo("    Verbose: yes");
    }
    if (m_tcDebug) {
        LogInfo("    Debug: yes");
    }

    yaml::Node systemInfo = systemConf["info"];
    m_latitude = systemInfo["latitude"].as<float>(0.0F);
    m_longitude = systemInfo["longitude"].as<float>(0.0F);
    m_height = systemInfo["height"].as<int>(0);
    m_power = systemInfo["power"].as<uint32_t>(0U);
    m_location = systemInfo["location"].as<std::string>();

    LogInfo("System Info Parameters");
    LogInfo("    Latitude: %fdeg N", m_latitude);
    LogInfo("    Longitude: %fdeg E", m_longitude);
    LogInfo("    Height: %um", m_height);
    LogInfo("    Power: %uW", m_power);
    LogInfo("    Location: \"%s\"", m_location.c_str());

    return true;
}

/// <summary>
/// Initializes source network connectivity.
/// </summary>
bool Host::createSrcNetwork()
{
    yaml::Node networkConf = m_conf["srcNetwork"];
    std::string address = networkConf["address"].as<std::string>();
    uint32_t port = networkConf["port"].as<uint32_t>(TRAFFIC_DEFAULT_PORT);
    uint32_t local = networkConf["local"].as<uint32_t>(0U);
    uint32_t id = networkConf["id"].as<uint32_t>(0U);
    uint32_t jitter = networkConf["talkgroupHang"].as<uint32_t>(360U);
    std::string password = networkConf["password"].as<std::string>();
    bool slot1 = networkConf["slot1"].as<bool>(true);
    bool slot2 = networkConf["slot2"].as<bool>(true);
    bool debug = networkConf["debug"].as<bool>(false);

    LogInfo("Source Network Parameters");
    LogInfo("    Peer Id: %u", id);
    LogInfo("    Address: %s", address.c_str());
    LogInfo("    Port: %u", port);
    if (local > 0U)
        LogInfo("    Local: %u", local);
    else
        LogInfo("    Local: random");
    LogInfo("    DMR Jitter: %ums", jitter);
    LogInfo("    Slot 1: %s", slot1 ? "enabled" : "disabled");
    LogInfo("    Slot 2: %s", slot2 ? "enabled" : "disabled");

    if (debug) {
        LogInfo("    Debug: yes");
    }

    m_srcNetwork = new Network(address, port, local, id, password, true, debug, slot1, slot2);
    m_srcNetwork->setMetadata(m_identity, 0, 0, 0.0F, 0.0F, 0, 0, 0, m_latitude, m_longitude, m_height, m_location);

    bool ret = m_srcNetwork->open();
    if (!ret) {
        delete m_srcNetwork;
        m_srcNetwork = NULL;
        LogError(LOG_HOST, "failed to initialize source traffic networking!");
        return false;
    }

    m_srcNetwork->enable(true);

    return true;
}

/// <summary>
/// Initializes destination network connectivity.
/// </summary>
bool Host::createDstNetwork()
{
    yaml::Node networkConf = m_conf["dstNetwork"];
    std::string address = networkConf["address"].as<std::string>();
    uint32_t port = networkConf["port"].as<uint32_t>(TRAFFIC_DEFAULT_PORT);
    uint32_t local = networkConf["local"].as<uint32_t>(0U);
    uint32_t id = networkConf["id"].as<uint32_t>(0U);
    uint32_t jitter = networkConf["talkgroupHang"].as<uint32_t>(360U);
    std::string password = networkConf["password"].as<std::string>();
    bool slot1 = networkConf["slot1"].as<bool>(true);
    bool slot2 = networkConf["slot2"].as<bool>(true);
    bool debug = networkConf["debug"].as<bool>(false);

    LogInfo("Destination Network Parameters");
    LogInfo("    Peer Id: %u", id);
    LogInfo("    Address: %s", address.c_str());
    LogInfo("    Port: %u", port);
    if (local > 0U)
        LogInfo("    Local: %u", local);
    else
        LogInfo("    Local: random");
    LogInfo("    DMR Jitter: %ums", jitter);
    LogInfo("    Slot 1: %s", slot1 ? "enabled" : "disabled");
    LogInfo("    Slot 2: %s", slot2 ? "enabled" : "disabled");

    if (debug) {
        LogInfo("    Debug: yes");
    }

    m_dstNetwork = new Network(address, port, local, id, password, true, debug, slot1, slot2);
    m_dstNetwork->setMetadata(m_identity, 0, 0, 0.0F, 0.0F, 0, 0, 0, m_latitude, m_longitude, m_height, m_location);

    bool ret = m_dstNetwork->open();
    if (!ret) {
        delete m_dstNetwork;
        m_dstNetwork = NULL;
        LogError(LOG_HOST, "failed to initialize destination traffic networking!");
        return false;
    }

    m_dstNetwork->enable(true);

    return true;
}

/// <summary>
///
/// </summary>
/// <param name="mode"></param>
void Host::createLockFile(const char* mode) const
{
    FILE* fp = ::fopen(g_lockFile.c_str(), "wt");
    if (fp != NULL) {
        ::fprintf(fp, "%s\n", mode);
        ::fclose(fp);
    }
}

/// <summary>
///
/// </summary>
void Host::removeLockFile() const
{
    ::remove(g_lockFile.c_str());
}
