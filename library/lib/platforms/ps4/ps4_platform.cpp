/*
Copyright 2023 xfangfang

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

#include <SDL2/SDL.h>

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/ps4/ps4_platform.hpp>
#include <orbis/Sysmodule.h>
#include <orbis/NetCtl.h>
#include <orbis/UserService.h>

extern "C" int sceSystemServiceLoadExec(const char *path, const char *args[]);

namespace brls
{

int (*sceRtcGetTick)(const OrbisDateTime *inOrbisDateTime, OrbisTick *outTick);
int (*sceRtcSetTick)(OrbisDateTime *outOrbisDateTime, const OrbisTick *inputTick);
int (*sceRtcConvertLocalTimeToUtc)(const OrbisTick *local_time, OrbisTick *utc);
int (*sceRtcConvertUtcToLocalTime)(const OrbisTick *utc, OrbisTick *local_time);
int (*sceRtcGetCurrentClockLocalTime)(OrbisDateTime *time);
int (*sceShellUIUtilLaunchByUri)(const char *uri, SceShellUIUtilLaunchByUriParam *param);
int (*sceShellUIUtilInitialize)();

#define GET_MODULE_SYMBOL(handle, symbol) moduleDlsym(handle, #symbol, reinterpret_cast<void**>(&symbol))

Ps4Platform::Ps4Platform()
{

    // NetCtl
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NETCTL);
    sceNetCtlInit();

    // SceRtc
    int handle = loadStartModuleFromSandbox("libSceRtc.sprx");
    GET_MODULE_SYMBOL(handle, sceRtcGetTick);
    GET_MODULE_SYMBOL(handle, sceRtcSetTick);
    GET_MODULE_SYMBOL(handle, sceRtcConvertLocalTimeToUtc);
    GET_MODULE_SYMBOL(handle, sceRtcConvertUtcToLocalTime);
    GET_MODULE_SYMBOL(handle, sceRtcGetCurrentClockLocalTime);

    // OpenBrowser
    handle = loadStartModuleFromSandbox("libSceShellUIUtil.sprx");
    GET_MODULE_SYMBOL(handle, sceShellUIUtilInitialize);
    GET_MODULE_SYMBOL(handle, sceShellUIUtilLaunchByUri);
    if (sceShellUIUtilInitialize && sceShellUIUtilInitialize() < 0)
        brls::Logger::error("sceShellUIUtilInitialize failed");

    atexit([](){
        sceSystemServiceLoadExec("exit", NULL);
    });
}

Ps4Platform::~Ps4Platform() = default;

bool Ps4Platform::canShowWirelessLevel()
{
    return true;
}

bool Ps4Platform::hasWirelessConnection()
{
    OrbisNetCtlInfo info;
    int ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info);
    if (ret < 0)
        return false;
    ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_SSID, &info);
    return ret >= 0;
}

int Ps4Platform::getWirelessLevel()
{
    OrbisNetCtlInfo info;
    int ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_RSSI_PERCENTAGE, &info);
    if (ret < 0)
        return 0;
    if (info.rssi_percentage >= 75)
        return 3;
    else if (info.rssi_percentage >= 50)
        return 2;
    else if (info.rssi_percentage >= 25)
        return 1;
    return 0;
}

bool Ps4Platform::hasEthernetConnection()
{
    OrbisNetCtlInfo info;
    int ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info);
    if (ret < 0)
        return false;
    ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_SSID, &info);
    return ret < 0;
}

std::string Ps4Platform::getIpAddress()
{
    OrbisNetCtlInfo info {};
    int ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info);
    if (ret < 0)
        return "-";
    return std::string { info.ip_address };
}

std::string Ps4Platform::getDnsServer()
{
    std::string dns = "-";
    OrbisNetCtlInfo info {};
    int ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_PRIMARY_DNS, &info);
    if (ret < 0)
        return dns;
    dns = std::string { info.primary_dns };
    ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_SECONDARY_DNS, &info);
    if (ret < 0)
        return dns;

    return dns + "\n" + std::string { info.secondary_dns };
}

void Ps4Platform::openBrowser(std::string url)
{
    SceShellUIUtilLaunchByUriParam param;
    param.size = sizeof(SceShellUIUtilLaunchByUriParam);
    sceUserServiceGetForegroundUser((int *)&param.userId);

    std::string launch_uri = std::string{"pswebbrowser:search?url="} + url;
    int ret = sceShellUIUtilLaunchByUri(launch_uri.c_str(), &param);
}

int Ps4Platform::loadStartModuleFromSandbox(const std::string& name)
{
    std::string modulePath{sceKernelGetFsSandboxRandomWord()};
    modulePath = "/" + modulePath + "/common/lib/" + name;

    return loadStartModule(modulePath);
}

int Ps4Platform::loadStartModule(const std::string& path)
{
    int handle = sceKernelLoadStartModule( path.c_str(), 0, NULL, 0, NULL, NULL);
    if (handle == 0) {
        Logger::error("Failed to load module: {}", path);
    }
    return handle;
}

int Ps4Platform::moduleDlsym(int handle, const std::string& name, void** func)
{
    int ret = sceKernelDlsym(handle, name.c_str(), func);
    if (func == nullptr) {
        Logger::error("Failed to dlsym: {}", name);
    }
    return ret;
}

} // namespace brls
