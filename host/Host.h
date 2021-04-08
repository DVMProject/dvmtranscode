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
#if !defined(__HOST_H__)
#define __HOST_H__

#include "Defines.h"
#include "network/Network.h"
#include "Timer.h"
#include "yaml/Yaml.h"

#include <string>

// ---------------------------------------------------------------------------
//  Class Prototypes
// ---------------------------------------------------------------------------
class HOST_SW_API RemoteControl;

// ---------------------------------------------------------------------------
//  Class Declaration
//      This class implements the core host service logic.
// ---------------------------------------------------------------------------

class HOST_SW_API Host {
public:
    /// <summary>Initializes a new instance of the Host class.</summary>
    Host(const std::string& confFile);
    /// <summary>Finalizes a instance of the Host class.</summary>
    ~Host();

    /// <summary>Executes the main modem host processing loop.</summary>
    int run();

private:
    const std::string& m_confFile;
    yaml::Node m_conf;

    network::Network* m_srcNetwork;
    network::Network* m_dstNetwork;

    uint32_t m_timeout;

    std::string m_identity;

    float m_latitude;
    float m_longitude;
    int m_height;
    uint32_t m_power;
    std::string m_location;

    bool m_twoWayTranscode;

    bool m_tcVerbose;
    bool m_tcDebug;


    /// <summary>Reads basic configuration parameters from the INI.</summary>
    bool readParams();
    /// <summary>Initializes source network connectivity.</summary>
    bool createSrcNetwork();
    /// <summary>Initializes destination network connectivity.</summary>
    bool createDstNetwork();

    /// <summary></summary>
    void createLockFile(const char* mode) const;
    /// <summary></summary>
    void removeLockFile() const;
};

#endif // __HOST_H__
