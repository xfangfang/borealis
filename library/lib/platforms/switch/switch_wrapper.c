/*
    Copyright 2019 natinusala
    Copyright 2019 WerWolv
    Copyright 2019 p-sam

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <switch.h>
#include <unistd.h>

static int nxlink_sock = -1;

uint32_t htonl(uint32_t hostlong)
{
    return __builtin_bswap32(hostlong);
}

uint16_t htons(uint16_t hostshort)
{
    return __builtin_bswap16(hostshort);
}

uint32_t ntohl(uint32_t netlong)
{
    return __builtin_bswap32(netlong);
}

uint16_t ntohs(uint16_t netshort)
{
    return __builtin_bswap16(netshort);
}

void userAppInit()
{
    printf("userAppInit\n");
    appletLockExit();
    SocketInitConfig cfg = *(socketGetDefaultInitConfig());

    cfg.bsd_service_type = BsdServiceType_System;

    AppletType at = appletGetAppletType();
    if (at == AppletType_Application || at == AppletType_SystemApplication)
    {
        cfg.num_bsd_sessions = 12; // default is 3
        cfg.sb_efficiency    = 8; // default is 4
        socketInitialize(&cfg);
    }
    else
    {
        cfg.num_bsd_sessions = 2;
        cfg.sb_efficiency    = 1;
        socketInitialize(&cfg);
    }

#ifdef DEBUG
    nxlink_sock = nxlinkStdio();
#endif

    romfsInit();
    plInitialize(PlServiceType_User);
    setsysInitialize();
    setInitialize();
    psmInitialize();
    nifmInitialize(NifmServiceType_User);

    appletUnlockExit();
}

void userAppExit()
{
    printf("userAppExit\n");

    // 网络状态
    nifmExit();
    // 电源状态
    psmExit();
    // 设置（比如当前语言）
    setExit();
    // 系统设置（比如系统当前语言）
    setsysExit();
    // 系统字体
    plExit();

    romfsExit();

    if (nxlink_sock != -1)
        close(nxlink_sock);

    socketExit();
}
