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

#define INTER_FONT "font/switch_font.ttf"
#define INTER_FONT_PATH BRLS_ASSET(INTER_FONT)

#define INTER_ICON "font/switch_icons.ttf"
#define INTER_ICON_PATH BRLS_ASSET(INTER_ICON)

namespace brls
{

const static std::vector<std::string> fontExts = {
    ".ttc",
    ".ttf",
    ".otf",
};

bool DesktopFontLoader::loadFontsExist(NVGcontext* vg, std::vector<std::string> fontPaths, std::string fontName, std::string fallbackFont) {
    for (auto &fontPath: fontPaths) {
        for (auto &fontExt: fontExts) {
            std::string fullPath = fontPath + fontExt;
            if (access(fullPath.c_str(), F_OK) != -1) {
                this->loadFontFromFile(fontName, fullPath);
                if (!fallbackFont.empty()) {
                    nvgAddFallbackFontId(vg, Application::getFont(fallbackFont), Application::getFont(fontName));
                }
                brls::Logger::info("Using {} font: {}", fontName, fullPath);
                return true;
            }
        }
    }
    return false;
}

void DesktopFontLoader::loadFonts()
{
    NVGcontext* vg = brls::Application::getNVGContext();

    // Regular
    // Try to use user-provided font first, fallback to Inter
    if (access(USER_FONT_PATH.c_str(), F_OK) != -1 && this->loadFontFromFile(FONT_REGULAR, USER_FONT_PATH))
    {
        brls::Logger::info("Load custom font: {}", USER_FONT_PATH);

        // Add internal font as fallback
#ifdef USE_LIBROMFS
        auto& font = romfs::get(INTER_FONT);
        if (font.valid() && Application::loadFontFromMemory("default", (void*)font.data(), font.size(), false))
#else
        if (this->loadFontFromFile("default", INTER_FONT_PATH))
#endif
        {
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont("default"));
        }
    }
    else
    {
        brls::Logger::warning("Cannot find custom font, (Searched at: {})", USER_FONT_PATH);
        brls::Logger::info("Using internal font: {}", INTER_FONT_PATH);
#ifdef USE_LIBROMFS
        auto& font = romfs::get(INTER_FONT);
        Application::loadFontFromMemory(FONT_REGULAR, (void*)font.data(), font.size(), false);
#else
        this->loadFontFromFile(FONT_REGULAR, INTER_FONT_PATH);
#endif
    }

    // Using system font as fallback
#if defined(__APPLE__) && !defined(IOS)
    std::vector<std::string> koreanFonts = {
        "/System/Library/Fonts/AppleSDGothicNeo",
    };
    std::vector<std::string> simplifiedChineseFonts;
#elif defined(_WIN32)
    std::string prefix = "C:\\Windows\\Fonts\\";
    char* winDir = getenv("systemroot");
    if (winDir) {
        prefix = std::string{winDir} + "\\Fonts\\";
    }
    std::vector<std::string> koreanFonts = {
        prefix+"malgun",
    };
    std::vector<std::string> simplifiedChineseFonts = {
        prefix+"msyh",
    };
#elif defined(ANDROID)
    std::vector<std::string> koreanFonts;
    std::vector<std::string> simplifiedChineseFonts = {
        "/system/fonts/NotoSansCJK-Regular",
        "/system/fonts/DroidSansFallback",
        "/system/fonts/NotoSansSC-Regular",
        "/system/fonts/DroidSansChinese",
    };
#else
    std::vector<std::string> koreanFonts;
    std::vector<std::string> simplifiedChineseFonts;
#endif
    if (!simplifiedChineseFonts.empty()) {
        loadFontsExist(vg, simplifiedChineseFonts, FONT_CHINESE_SIMPLIFIED, FONT_REGULAR);
    }
    if (!koreanFonts.empty()) {
        loadFontsExist(vg, koreanFonts, FONT_KOREAN_REGULAR, FONT_REGULAR);
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
#ifdef USE_LIBROMFS
    bool loaded = false;
    if (USER_ICON_PATH.rfind("@res/", 0) == 0)
    {
        // USER_ICON_PATH is inside the romfs
        try
        {
            auto& icon = romfs::get(USER_ICON_PATH.substr(5));
            if (icon.valid())
            {
                loaded = Application::loadFontFromMemory(FONT_SWITCH_ICONS, (void*)icon.data(), icon.size(), false);
            }
        }
        catch (...)
        {
        }
    } else if (access(USER_ICON_PATH.c_str(), F_OK) != -1) {
        // USER_ICON_PATH is an external path
        loaded = this->loadFontFromFile(FONT_SWITCH_ICONS, USER_ICON_PATH);
    }

    if (loaded)
#else
    if (access(USER_ICON_PATH.c_str(), F_OK) != -1 && this->loadFontFromFile(FONT_SWITCH_ICONS, USER_ICON_PATH))
#endif
    {
        brls::Logger::info("Load keymap icon: {}", USER_ICON_PATH);
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
    }
    else
    {
        brls::Logger::warning("Cannot find custom icon, (Searched at: {})", USER_ICON_PATH);
#ifdef USE_LIBROMFS
        // If you do not want to put a default icons in your own application,
        // you can leave an empty icon file in the resource folder to avoid errors reported by libromfs.
        auto& icon = romfs::get(INTER_ICON);
        // Determine if the file is empty
        if (icon.valid() && Application::loadFontFromMemory(FONT_SWITCH_ICONS, (void*)icon.data(), icon.size(), false))
        {
#else
        if (access(INTER_ICON_PATH, F_OK) != -1 && this->loadFontFromFile(FONT_SWITCH_ICONS, INTER_ICON_PATH))
        {
#endif
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
            brls::Logger::info("Using internal icon: {}", INTER_ICON_PATH);
        }
        else
        {
            Logger::warning("Icons may not be displayed, for more information please refer to: https://github.com/xfangfang/wiliwili/discussions/38");
        }
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
