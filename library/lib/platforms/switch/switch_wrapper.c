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
#include <switch.h>
#include <unistd.h>
#include <stdio.h>

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
    SocketInitConfig cfg = *(socketGetDefaultInitConfig());
    cfg.num_bsd_sessions = 16; // default is 3, (16)
    cfg.sb_efficiency = 8; // default is 4 (8)
//    cfg.udp_rx_buf_size = 0;
//    cfg.udp_tx_buf_size = 0;

//    u32 tcp_tx_buf_size;                        ///< Size of the TCP transfer (send) buffer (initial or fixed).
//    u32 tcp_rx_buf_size;                        ///< Size of the TCP receive buffer (initial or fixed).
//    u32 tcp_tx_buf_max_size;                    ///< Maximum size of the TCP transfer (send) buffer. If it is 0, the size of the buffer is fixed to its initial value.
//    u32 tcp_rx_buf_max_size;                    ///< Maximum size of the TCP receive buffer. If it is 0, the size of the buffer is fixed to its initial value.
//
//    u32 udp_tx_buf_size;                        ///< Size of the UDP transfer (send) buffer (typically 0x2400 bytes).
//    u32 udp_rx_buf_size;                        ///< Size of the UDP receive buffer (typically 0xA500 bytes).
//
//    u32 sb_efficiency;                          ///< Number of buffers for each socket (standard values range from 1 to 8).
//
//    u32 num_bsd_sessions;                       ///< Number of BSD service sessions (typically 3).
//    BsdServiceType bsd_service_type;


    socketInitialize(&cfg);

//    socketInitializeDefault();
    nxlink_sock = nxlinkStdio();

    romfsInit();
    plInitialize(PlServiceType_User);
    setsysInitialize();
    setInitialize();
    psmInitialize();
    nifmInitialize(NifmServiceType_User);

    printf("userAppInit\n");
    printf("version: %d\n", cfg.bsdsockets_version);
    printf("sessions: %d\n", cfg.num_bsd_sessions);

    printf("tcp_tx_buf_size: %d\n", cfg.tcp_tx_buf_size);
    printf("tcp_rx_buf_size: %d\n", cfg.tcp_rx_buf_size);
    printf("tcp_tx_buf_max_size: %d\n", cfg.tcp_tx_buf_max_size);
    printf("tcp_rx_buf_max_size: %d\n", cfg.tcp_rx_buf_max_size);
    printf("udp_tx_buf_size: %d\n", cfg.udp_tx_buf_size);
    printf("udp_rx_buf_size: %d\n", cfg.udp_rx_buf_size);

    printf("sb_efficiency: %d\n", cfg.sb_efficiency);

    if(cfg.bsd_service_type == BsdServiceType_User){
        printf("bsd_service_type: User\n");
    }

}

void userAppExit()
{
    printf("userAppExit\n");

    //网络状态
    nifmExit();
    //电源状态
    psmExit();
    //设置（比如当前语言）
    setExit();
    //系统设置（比如系统当前语言）
    setsysExit();
    //系统字体
    plExit();

    romfsExit();

    if (nxlink_sock != -1)
        close(nxlink_sock);

    socketExit();
}
