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

#include <unistd.h>

#include <borealis/core/application.hpp>
#include <borealis/core/assets.hpp>
#include <borealis/platforms/desktop/desktop_font.hpp>

#define INTER_FONT_PATH BRLS_ASSET("font/switch_font.ttf")

namespace brls
{

void DesktopFontLoader::loadFonts()
{
    NVGcontext* vg = brls::Application::getNVGContext();

    // Regular
    // Try to use user-provided font first, fallback to Inter
    if (access(USER_FONT_PATH.c_str(), F_OK) != -1)
    {
        brls::Logger::info("Load custom font: {}", USER_FONT_PATH);
        this->loadFontFromFile(FONT_REGULAR, USER_FONT_PATH);

        // Add internal font as fallback
        this->loadFontFromFile("default", INTER_FONT_PATH);
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont("default"));
    }
    else
    {
        brls::Logger::warning("Cannot find custom font, (Searched at: {})", USER_FONT_PATH);
        brls::Logger::info("Using internal font: {}", INTER_FONT_PATH);
        this->loadFontFromFile(FONT_REGULAR, INTER_FONT_PATH);
    }

    // Using system font as fallback
#if defined(__APPLE__)
    std::vector<std::string> systemFonts = {
        "PingFang.ttc",
        "PingFang.ttf",
        "Arial Unicode.ttf",
    };
    std::string prefix = "/Library/Fonts/";
#elif defined(_WIN32)
    std::string prefix = "C:\\Windows\\Fonts\\";
    char* winDir = getenv("systemroot");
    if (winDir) {
        prefix = std::string{winDir} + "\\Fonts\\";
    }
    std::vector<std::string> systemFonts = {
        "msyh.ttc",
        "msyh.ttf",
        "malgun.ttf",
    };
#elif defined(ANDROID)
    std::string prefix = "/system/fonts/";
    std::vector<std::string> systemFonts = {
        "NotoSansSC-Regular.otf",
        "NotoSansCJK-Regular.ttc",
        "DroidSansFallback.ttf",
        "DroidSansChinese.ttf",
    };
#else
    std::vector<std::string> systemFonts;
    std::string prefix;
#endif
    if (!systemFonts.empty()) {
        for (auto &fontName: systemFonts) {
            std::string systemFont = prefix + fontName;
            if (access(systemFont.c_str(), F_OK) != -1) {
                brls::Logger::info("Load system font: {}", systemFont);
                this->loadFontFromFile("system", systemFont);
                nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont("system"));
                break;
            } else {
                brls::Logger::warning("Cannot find system font, (Searched at: {})", systemFont);
            }
        }
    }

    // Load Emoji
    if (!USER_EMOJI_PATH.empty())
    {
        if (access(USER_EMOJI_PATH.c_str(), F_OK) != -1)
        {
            brls::Logger::info("Load emoji font: {}", USER_EMOJI_PATH);
            this->loadFontFromFile("emoji", USER_EMOJI_PATH);
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont("emoji"));
        }
    }

    // Switch icons
    // Only supports user-provided font
    if (access(USER_ICON_PATH.c_str(), F_OK) != -1 && this->loadFontFromFile(FONT_SWITCH_ICONS, USER_ICON_PATH))
    {
        brls::Logger::info("Load keymap icon: {}", USER_ICON_PATH);
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
    }
    else
    {
        Logger::warning("Cannot find keymap icon, (Searched at: {})", USER_ICON_PATH);
        Logger::warning("Icons may not be displayed, for more information please refer to: https://github.com/xfangfang/wiliwili/discussions/38");
    }

    // Material icons
    if (this->loadMaterialFromResources())
    {
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_MATERIAL_ICONS));
    }
    else
    {
        Logger::error("switch: could not load Material icons font from resources");
    }
}

} // namespace brls
