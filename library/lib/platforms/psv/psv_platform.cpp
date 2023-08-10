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
#include <psp2/kernel/processmgr.h>
#include <psp2/net/netctl.h>
#include <psp2/power.h>
#include <psp2/rtc.h>

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/psv/psv_platform.hpp>

extern "C"
{
    unsigned int _newlib_heap_size_user      = 220 * 1024 * 1024;
    unsigned int sceLibcHeapSize             = 24 * 1024 * 1024;
    unsigned int _pthread_stack_default_user = 2 * 1024 * 1024;
}

namespace brls
{

static bool DISABLE_SUSPEND = false;

static int powerCallback(int notifyId, int notifyCount, int powerInfo, void* common)
{
    if ((powerInfo & SCE_POWER_CB_APP_RESUME) || (powerInfo & SCE_POWER_CB_APP_RESUMING))
    {
        brls::Application::getWindowFocusChangedEvent()->fire(true);
    }
    else if ((powerInfo & SCE_POWER_CB_BUTTON_PS_PRESS) || (powerInfo & SCE_POWER_CB_APP_SUSPEND) || (powerInfo & SCE_POWER_CB_SYSTEM_SUSPEND))
    {
        brls::Application::getWindowFocusChangedEvent()->fire(false);
    }
    return 0;
}

int CallbackThread(SceSize args, void* arg)
{
    int cbid = sceKernelCreateCallback("Power Callback", 0, powerCallback, NULL);
    scePowerRegisterCallback(cbid);

    for (;;)
    {
        if (DISABLE_SUSPEND)
        {
            sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
        }
        sceKernelDelayThreadCB(10000000);
    }

    return 0;
}

PsvPlatform::PsvPlatform()
{
    //    The main thread does not handle callbacks, so we need to make one to handle them.
    int thid = sceKernelCreateThread("callbackThread", CallbackThread, 0x10000100, 0x10000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
}

PsvPlatform::~PsvPlatform()
{
}

bool PsvPlatform::canShowBatteryLevel()
{
    return true;
}

int PsvPlatform::getBatteryLevel()
{
    return scePowerGetBatteryLifePercent();
}

bool PsvPlatform::isBatteryCharging()
{
    return scePowerIsPowerOnline() == SCE_TRUE;
}

bool PsvPlatform::canShowWirelessLevel()
{
    return true;
}

bool PsvPlatform::hasWirelessConnection()
{
    SceNetCtlInfo info;
    int ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
    return ret >= 0;
}

int PsvPlatform::getWirelessLevel()
{
    SceNetCtlInfo info;
    int ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_RSSI_PERCENTAGE, &info);
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

bool PsvPlatform::hasEthernetConnection()
{
    return false;
}

std::string PsvPlatform::getIpAddress()
{
    SceNetCtlInfo info {};
    int ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
    if (ret < 0)
        return "-";
    return std::string { info.ip_address };
}

std::string PsvPlatform::getDnsServer()
{
    std::string dns = "-";
    SceNetCtlInfo info {};
    int ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_PRIMARY_DNS, &info);
    if (ret < 0)
        return dns;
    dns = std::string { info.primary_dns };
    ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_SECONDARY_DNS, &info);
    if (ret < 0)
        return dns;
    return dns + "\n" + std::string { info.secondary_dns };
}

void PsvPlatform::openBrowser(std::string url)
{
}

void PsvPlatform::disableScreenDimming(bool disable, const std::string& reason, const std::string& app)
{
    DISABLE_SUSPEND = disable;
}

bool PsvPlatform::isScreenDimmingDisabled()
{
    return DISABLE_SUSPEND;
}

} // namespace brls
