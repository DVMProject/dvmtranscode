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
*   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
*   Copyright (C) 2018-2021 by Bryan Biedenkapp N2PLL
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
#include "Log.h"
#include "network/Network.h"

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <cassert>
#include <cstring>

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#define EOL    "\n"
#else
#define EOL    "\r\n"
#endif

// ---------------------------------------------------------------------------
//  Global Variables
// ---------------------------------------------------------------------------

static uint32_t m_fileLevel = 0U;
static std::string m_filePath;
static std::string m_fileRoot;

static FILE* m_fpLog = NULL;

static uint32_t m_displayLevel = 2U;

static struct tm m_tm;

static char LEVELS[] = " DMIWEF";

// ---------------------------------------------------------------------------
//  Global Functions
// ---------------------------------------------------------------------------

/// <summary>
/// Helper to open the detailed log file, file handle.
/// </summary>
/// <returns>True, if log file is opened, otherwise false.
static bool LogOpen()
{
    if (m_fileLevel == 0U)
        return true;

    time_t now;
    ::time(&now);

    struct tm* tm = ::gmtime(&now);

    if (tm->tm_mday == m_tm.tm_mday && tm->tm_mon == m_tm.tm_mon && tm->tm_year == m_tm.tm_year) {
        if (m_fpLog != NULL)
            return true;
    }
    else {
        if (m_fpLog != NULL)
            ::fclose(m_fpLog);
    }

    char filename[200U];
#if defined(_WIN32) || defined(_WIN64)
    ::sprintf(filename, "%s\\%s-%04d-%02d-%02d.log", m_filePath.c_str(), m_fileRoot.c_str(), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
#else
    ::sprintf(filename, "%s/%s-%04d-%02d-%02d.log", m_filePath.c_str(), m_fileRoot.c_str(), tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
#endif
    m_fpLog = ::fopen(filename, "a+t");
    m_tm = *tm;

    return m_fpLog != NULL;
}

/// <summary>
/// Initializes the diagnostics log.
/// </summary>
/// <param name="filePath">Full-path to the detailed log file.</param>
/// <param name="fileRoot">Prefix of the detailed log file name.</param>
/// <param name="fileLevel">File logging level.</param>
/// <param name="displayLevel">Console logging level.</param>
bool LogInitialise(const std::string& filePath, const std::string& fileRoot, uint32_t fileLevel, uint32_t displayLevel)
{
    m_filePath = filePath;
    m_fileRoot = fileRoot;
    m_fileLevel = fileLevel;
    m_displayLevel = displayLevel;
    return ::LogOpen();
}

/// <summary>
/// Finalizes the diagnostics log.
/// </summary>
void LogFinalise()
{
    if (m_fpLog != NULL)
        ::fclose(m_fpLog);
}

/// <summary>
/// Writes a new entry to the diagnostics log.
/// </summary>
/// <param name="level">Log level.</param>
/// <param name="module">Module name the log entry was genearted from.</param>
/// <param name="msg">Formatted string to write to activity log.</param>
void Log(uint32_t level, const char *module, const char* fmt, ...)
{
    assert(fmt != NULL);

    char buffer[501U];
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME st;
    ::GetSystemTime(&st);

    if (module != NULL) {
        ::sprintf(buffer, "%c: %04u-%02u-%02u %02u:%02u:%02u.%03u (%s) ", LEVELS[level], st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, module);
    }
    else {
        ::sprintf(buffer, "%c: %04u-%02u-%02u %02u:%02u:%02u.%03u ", LEVELS[level], st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }
#else
    struct timeval now;
    ::gettimeofday(&now, NULL);

    struct tm* tm = ::gmtime(&now.tv_sec);

    if (module != NULL) {
        ::sprintf(buffer, "%c: %04d-%02d-%02d %02d:%02d:%02d.%03lu (%s) ", LEVELS[level], tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, now.tv_usec / 1000U, module);
    }
    else {
        ::sprintf(buffer, "%c: %04d-%02d-%02d %02d:%02d:%02d.%03lu ", LEVELS[level], tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, now.tv_usec / 1000U);
    }
#endif

    va_list vl;
    va_start(vl, fmt);

    ::vsnprintf(buffer + ::strlen(buffer), 500, fmt, vl);

    va_end(vl);

    if (level >= m_fileLevel && m_fileLevel != 0U) {
        bool ret = ::LogOpen();
        if (!ret)
            return;

        ::fprintf(m_fpLog, "%s\n", buffer);
        ::fflush(m_fpLog);
    }

    if (level >= m_displayLevel && m_displayLevel != 0U) {
        ::fprintf(stdout, "%s" EOL, buffer);
        ::fflush(stdout);
    }

    if (level >= 6U) {        // Fatal
        ::fclose(m_fpLog);
        exit(1);
    }
}
