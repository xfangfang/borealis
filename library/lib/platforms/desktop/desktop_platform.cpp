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

#include <strings.h>

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/desktop/desktop_platform.hpp>

namespace brls
{

DesktopPlatform::DesktopPlatform()
{
    // Theme
    char* themeEnv = getenv("BOREALIS_THEME");
    if (themeEnv == nullptr)
    {
#ifdef __APPLE__
        char buffer[10];
        memset(buffer, 0, sizeof buffer);
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("defaults read -g AppleInterfaceStyle", "r"), pclose);
        if (pipe)
        {
            fgets(buffer, sizeof buffer, pipe.get());
            if (strncmp(buffer, "Dark", 4) == 0)
            {
                this->themeVariant = ThemeVariant::DARK;
                brls::Logger::info("Set app theme: Dark");
            }
            else
            {
                brls::Logger::info("Set app theme: Light");
            }
        }
        else
        {
            brls::Logger::error("cannot get system theme");
        }
#endif
    }
    else if (!strcasecmp(themeEnv, "DARK"))
    {
        this->themeVariant = ThemeVariant::DARK;
    }

    // Locale
    if (Platform::APP_LOCALE_DEFAULT == LOCALE_AUTO)
    {
        char* langEnv = getenv("BOREALIS_LANG");
        this->locale  = langEnv ? std::string(langEnv) : LOCALE_DEFAULT;
    }
    else
    {
        this->locale = Platform::APP_LOCALE_DEFAULT;
    }
    brls::Logger::info("Set app locale: {}", this->locale);

    // Platform impls
    this->fontLoader = new DesktopFontLoader();
}

bool DesktopPlatform::canShowBatteryLevel()
{
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

bool DesktopPlatform::canShowWirelessLevel()
{
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

int DesktopPlatform::getBatteryLevel()
{
#if defined(__APPLE__)
    std::string b = exec("pmset -g batt | grep -Eo '[0-9]+%'");
    if (!b.empty() && b[b.size() - 1] == '%')
    {
        b = b.substr(0, b.size() - 1);
    }
    return stoi(b);
#else
    return 100;
#endif
}

bool DesktopPlatform::isBatteryCharging()
{
#if defined(__APPLE__)
    std::string res = exec("pmset -g batt | grep -o 'AC Power'");
    return !res.empty();
#else
    return false;
#endif
}

bool DesktopPlatform::hasWirelessConnection()
{
#if defined(__APPLE__)
    std::string res = exec("networksetup -listallhardwareports | awk '/Wi-Fi/{getline; print $2}' | xargs networksetup -getairportpower | grep -o On");
    return !res.empty();
#else
    return true;
#endif
}

int DesktopPlatform::getWirelessLevel()
{
    return 3;
}

std::string DesktopPlatform::getIpAddress()
{
#if defined(__APPLE__) || defined(__linux__)
    return exec("ifconfig | grep \"inet \" | grep -Fv 127.0.0.1 | awk '{print $2}' ");
#else
    return "-";
#endif
}

std::string DesktopPlatform::getDnsServer()
{
#if defined(__APPLE__)
    return exec("scutil --dns | grep nameserver | awk '{print $3}' | sort -u | paste -s -d',' -");
#else
    return "-";
#endif
}

std::string DesktopPlatform::exec(const char* cmd)
{
    std::string result = "";
#if defined(__APPLE__) || defined(__linux__)
    char buffer[128];
    memset(buffer, 0, sizeof buffer);
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr)
    {
        result += buffer;
    }
#endif

    if (!result.empty() && result[result.size() - 1] == '\n')
        result = result.substr(0, result.size() - 1);
    return result;
}

bool DesktopPlatform::isApplicationMode()
{
    return true;
}

void DesktopPlatform::exitToHomeMode(bool value)
{
    DesktopPlatform::RESTART_APP = !value;
}

void DesktopPlatform::forceEnableGamePlayRecording()
{
    return;
}

void DesktopPlatform::openBrowser(std::string url)
{
    brls::Logger::debug("open url: {}", url);
#ifdef __APPLE__
    std::string cmd = "open " + url;
    system(cmd.c_str());
#endif
#ifdef __linux__
    std::string cmd = "xdg-open " + url;
    system(cmd.c_str());
#endif
#ifdef _WIN32
    std::string cmd = "explorer " + url;
    system(cmd.c_str());
#endif
    return;
}

std::string DesktopPlatform::getName()
{
    return "Desktop";
}

FontLoader* DesktopPlatform::getFontLoader()
{
    return this->fontLoader;
}

ThemeVariant DesktopPlatform::getThemeVariant()
{
    return this->themeVariant;
}

void DesktopPlatform::setThemeVariant(ThemeVariant theme)
{
    this->themeVariant = theme;
}

std::string DesktopPlatform::getLocale()
{
    return this->locale;
}

DesktopPlatform::~DesktopPlatform()
{
    delete this->fontLoader;
}

} // namespace brls
