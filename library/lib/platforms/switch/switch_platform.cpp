/*
    Copyright 2021 natinusala

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

#include <switch.h>

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/switch/switch_platform.hpp>

extern "C" u32 __nx_applet_exit_mode;

namespace brls
{

SwitchPlatform::SwitchPlatform()
{
    // Cache theme variant before video context init
    // The background color is created once in the "static" command list
    // executed every frame, so we need to know the background color
    // to add the clear command to that list.
    ColorSetId colorSetId;
    setsysGetColorSetId(&colorSetId);

    if (colorSetId == ColorSetId_Dark)
        this->themeVariant = ThemeVariant::DARK;
    else
        this->themeVariant = ThemeVariant::LIGHT;

    Logger::info("switch: system has color set {}, using borealis theme {}", colorSetId, this->themeVariant);

    // Init platform impls
    this->audioPlayer  = new SwitchAudioPlayer();
    this->inputManager = new SwitchInputManager();
    this->fontLoader   = new SwitchFontLoader();

    // Get locale
    uint64_t languageCode = 0;

    Result rc = setGetSystemLanguage(&languageCode);

    if (R_SUCCEEDED(rc))
    {
        char* languageName = (char*)&languageCode;
        this->locale       = std::string(languageName);
    }
    else
    {
        Logger::error("switch: unable to get system language (error {:#x}), using the default one: {1}", rc, LOCALE_DEFAULT);
        this->locale = LOCALE_DEFAULT;
    }
}

void SwitchPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight)
{
    this->videoContext = new GLFWVideoContext(windowTitle, windowWidth, windowHeight);
    
    // Fixme: Dirty fix to reinitialise controllers with settings from borealis, not by GLFW
//    brls::Logger::info("create SwitchInputManager");
//    this->inputManager = new SwitchInputManager();
//    brls::Logger::info("create SwitchInputManager done");
}

bool SwitchPlatform::canShowBatteryLevel()
{
    return true;
}

int SwitchPlatform::getBatteryLevel()
{
    u32 charge;
    psmGetBatteryChargePercentage(&charge);
    return (int)charge;
}

bool SwitchPlatform::isBatteryCharging()
{
    PsmChargerType type;
    psmGetChargerType(&type);
    return type == PsmChargerType_EnoughPower || type == PsmChargerType_LowPower;
}

bool SwitchPlatform::hasWirelessConnection()
{
    bool res = false;
    nifmIsWirelessCommunicationEnabled(&res);
    return res;
}

int SwitchPlatform::getWirelessLevel()
{
    NifmInternetConnectionType type;
    u32 wifiSignal;
    NifmInternetConnectionStatus status;
    nifmGetInternetConnectionStatus(&type, &wifiSignal, &status);
    return wifiSignal;
}

bool SwitchPlatform::isApplicationMode()
{
    AppletType at = appletGetAppletType();
    return at == AppletType_Application || at == AppletType_SystemApplication;
}

void SwitchPlatform::exitToHomeMode(bool value)
{
    __nx_applet_exit_mode = value;
}

void SwitchPlatform::forceEnableGamePlayRecording()
{
    appletInitializeGamePlayRecording();
}

void SwitchPlatform::openBrowser(std::string url)
{
    WebCommonConfig config;

    Result rc = webPageCreate(&config, url.c_str());
    if (R_SUCCEEDED(rc)) {
        rc = webConfigSetWhitelist(&config, "^http*");
        if (R_SUCCEEDED(rc)) {
            rc = webConfigShow(&config, NULL);
        }
    }
}

std::string SwitchPlatform::getName()
{
    return "Switch";
}

bool SwitchPlatform::mainLoopIteration()
{
    return appletMainLoop();
}

VideoContext* SwitchPlatform::getVideoContext()
{
    return this->videoContext;
}

std::string SwitchPlatform::getLocale()
{
    return this->locale;
}

AudioPlayer* SwitchPlatform::getAudioPlayer()
{
    return this->audioPlayer;
}

InputManager* SwitchPlatform::getInputManager()
{
    return this->inputManager;
}

FontLoader* SwitchPlatform::getFontLoader()
{
    return this->fontLoader;
}

ThemeVariant SwitchPlatform::getThemeVariant()
{
    return this->themeVariant;
}

SwitchPlatform::~SwitchPlatform()
{
    delete this->audioPlayer;
    delete this->inputManager;
    delete this->videoContext;
}

} // namespace brls
