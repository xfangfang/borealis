/*
    Copyright 2021 natinusala
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

#pragma once

#include <borealis/core/platform.hpp>
#include <borealis/platforms/desktop/desktop_font.hpp>

namespace brls
{

class DesktopPlatform : public Platform
{
  public:
    DesktopPlatform();
    ~DesktopPlatform();

    std::string getName() override;

    ThemeVariant getThemeVariant() override;
    void setThemeVariant(ThemeVariant theme) override;
    std::string getLocale() override;

    FontLoader* getFontLoader() override;
    bool canShowBatteryLevel() override;
    bool canShowWirelessLevel() override;
    int getBatteryLevel() override;
    bool isBatteryCharging() override;
    bool hasWirelessConnection() override;
    int getWirelessLevel() override;
    std::string getIpAddress() override;
    std::string getDnsServer() override;
    std::string exec(const char* cmd);
    bool isApplicationMode() override;
    void exitToHomeMode(bool value) override;
    void forceEnableGamePlayRecording() override;
    void openBrowser(std::string url) override;

  protected:
    DesktopFontLoader* fontLoader = nullptr;
    ThemeVariant themeVariant     = ThemeVariant::LIGHT;
    std::string locale;
};

} // namespace brls
