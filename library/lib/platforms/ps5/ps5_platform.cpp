/*
Copyright 2026 xfangfang

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

#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/thread.hpp>
#include <borealis/platforms/ps5/ps5_ime.hpp>
#include <borealis/platforms/ps5/ps5_platform.hpp>

extern "C" int sceSystemServiceLoadExec(const char* path, const char* args[]);

namespace brls
{

Ps5Platform::Ps5Platform()
{
    // Initialize here for loading the system modules
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        Logger::error("sdl: failed to initialize video");
    }
}

Ps5Platform::~Ps5Platform() = default;

std::string Ps5Platform::getName()
{
    return "Ps5";
}

void Ps5Platform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
#if defined(BOREALIS_USE_VULKAN)
    windowWidth  = 3840;
    windowHeight = 2160;
#else
    windowWidth  = 1920;
    windowHeight = 1080;
#endif

    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight, NAN, NAN);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
    this->imeManager   = new Ps5ImeManager(&this->otherEvent);
}

bool Ps5Platform::canShowWirelessLevel()
{
    return true;
}

bool Ps5Platform::hasWirelessConnection()
{
    bool has_wlan = false;
    struct ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) == 0)
    {
        for (auto addr = interfaces; addr != nullptr; addr = addr->ifa_next)
        {
            if (!addr->ifa_addr) continue;
            if (addr->ifa_flags & IFF_LOOPBACK) continue;
            if (!(addr->ifa_flags & IFF_UP)) continue;
            if (addr->ifa_addr->sa_family == AF_INET && addr->ifa_name)
            {
                auto* sin = reinterpret_cast<struct sockaddr_in*>(addr->ifa_addr);
                if (sin->sin_addr.s_addr == INADDR_ANY || sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
                    continue;
                std::string name = addr->ifa_name;
                if (name.find("wlan") != std::string::npos || name.find("mlan") != std::string::npos || name.find("wifi") != std::string::npos)
                {
                    has_wlan = true;
                    break;
                }
            }
        }
        freeifaddrs(interfaces);
    }
    return has_wlan;
}

int Ps5Platform::getWirelessLevel()
{
    return hasWirelessConnection() ? 3 : 0;
}

bool Ps5Platform::hasEthernetConnection()
{
    bool has_eth = false;
    struct ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) == 0)
    {
        for (auto addr = interfaces; addr != nullptr; addr = addr->ifa_next)
        {
            if (!addr->ifa_addr) continue;
            if (addr->ifa_flags & IFF_LOOPBACK) continue;
            if (!(addr->ifa_flags & IFF_UP)) continue;
            if (addr->ifa_addr->sa_family == AF_INET && addr->ifa_name)
            {
                auto* sin = reinterpret_cast<struct sockaddr_in*>(addr->ifa_addr);
                if (sin->sin_addr.s_addr == INADDR_ANY || sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
                    continue;
                std::string name = addr->ifa_name;
                if (name.find("wlan") == std::string::npos && name.find("mlan") == std::string::npos && name.find("wifi") == std::string::npos)
                {
                    has_eth = true;
                    break;
                }
            }
        }
        freeifaddrs(interfaces);
    }
    return has_eth;
}

std::string Ps5Platform::getIpAddress()
{
    std::string ipaddr = "-";
    struct ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) == 0)
    {
        for (auto addr = interfaces; addr != nullptr; addr = addr->ifa_next)
        {
            if (!addr->ifa_addr) continue;
            if (addr->ifa_flags & IFF_LOOPBACK) continue;
            if (!(addr->ifa_flags & IFF_UP)) continue;
            if (addr->ifa_addr->sa_family == AF_INET)
            {
                auto* sin = reinterpret_cast<struct sockaddr_in*>(addr->ifa_addr);
                if (sin->sin_addr.s_addr == INADDR_ANY || sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
                    continue;
                char buf[INET_ADDRSTRLEN] = {0};
                if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf)))
                {
                    ipaddr = buf;
                    break;
                }
            }
        }
        freeifaddrs(interfaces);
    }

    if (ipaddr == "-" || ipaddr.empty())
    {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock >= 0)
        {
            struct sockaddr_in serv {};
            serv.sin_family = AF_INET;
            serv.sin_addr.s_addr = inet_addr("119.29.29.29");
            serv.sin_port = htons(53);
            if (connect(sock, reinterpret_cast<const struct sockaddr*>(&serv), sizeof(serv)) == 0)
            {
                struct sockaddr_in name {};
                socklen_t namelen = sizeof(name);
                if (getsockname(sock, reinterpret_cast<struct sockaddr*>(&name), &namelen) == 0)
                {
                    char buf[INET_ADDRSTRLEN] = {0};
                    if (inet_ntop(AF_INET, &name.sin_addr, buf, sizeof(buf)))
                    {
                        ipaddr = buf;
                    }
                }
            }
            close(sock);
        }
    }
    Logger::info("Ps5Platform::getIpAddress: {}", ipaddr);
    return ipaddr;
}

std::string Ps5Platform::getDnsServer()
{
    return "-";
}

void Ps5Platform::openBrowser(std::string url)
{

}

} // namespace brls
