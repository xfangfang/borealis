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
        auto font = romfs::get(INTER_FONT);
        Application::loadFontFromMemory("default", (void*)font.string().data(), font.string().size(), false);
#else
        this->loadFontFromFile("default", INTER_FONT_PATH);
#endif
        nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont("default"));
    }
    else
    {
        brls::Logger::warning("Cannot find custom font, (Searched at: {})", USER_FONT_PATH);
        brls::Logger::info("Using internal font: {}", INTER_FONT_PATH);
#ifdef USE_LIBROMFS
        auto font = romfs::get(INTER_FONT);
        Application::loadFontFromMemory(FONT_REGULAR, (void*)font.string().data(), font.string().size(), false);
#else
        this->loadFontFromFile(FONT_REGULAR, INTER_FONT_PATH);
#endif
    }

    // Using system font as fallback
    std::string systemFont;
#if defined(__APPLE__) && !defined(IOS)
    systemFont = "/Library/Fonts/Arial Unicode.ttf";
#elif defined(_WIN32)
    char* winDir = getenv("systemroot");
    if (winDir)
        systemFont = std::string{winDir} + "\\Fonts\\malgun.ttf";
    else
        systemFont = "C:\\Windows\\Fonts\\malgun.ttf";
#endif
    if (!systemFont.empty())
    {
        if (access(systemFont.c_str(), F_OK) != -1)
        {
            brls::Logger::info("Load system font: {}", systemFont);
            this->loadFontFromFile("system", systemFont);
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont("system"));
        }
        else
        {
            brls::Logger::warning("Cannot find system font, (Searched at: {})", systemFont);
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
#ifdef USE_LIBROMFS
    bool loaded = false;
    if (USER_ICON_PATH.rfind("@res/", 0) == 0)
    {
        auto icon = romfs::get(USER_ICON_PATH.substr(5));
        if (icon.valid())
        {
            Application::loadFontFromMemory(FONT_SWITCH_ICONS, (void*)icon.string().data(), icon.string().size(), false);
            loaded = true;
        }
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
        auto icon = romfs::get(INTER_ICON);
        // Determine if the file is empty
        if (icon.valid()) {
            Application::loadFontFromMemory(FONT_SWITCH_ICONS, (void*)icon.string().data(), icon.string().size(), false);
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
        }
#else
        if (access(INTER_ICON_PATH, F_OK) != -1 && this->loadFontFromFile(FONT_SWITCH_ICONS, INTER_ICON_PATH))
        {
            brls::Logger::info("Using internal icon: {}", INTER_ICON);
            nvgAddFallbackFontId(vg, Application::getFont(FONT_REGULAR), Application::getFont(FONT_SWITCH_ICONS));
        }
#endif
        else {
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
