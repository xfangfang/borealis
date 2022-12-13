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

#include <strings.h>

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_platform.hpp>

// glfw video and input code inspired from the glfw hybrid app by fincs
// https://github.com/fincs/hybrid_app

namespace brls
{

SDLPlatform::SDLPlatform()
{
    // Init sdl
    //    glfwSetErrorCallback(glfwErrorCallback);
    //    glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_TRUE);
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0)
    {
        Logger::error("sdl: failed to initialize");
        return;
    }

    // Theme
    char* themeEnv = getenv("BOREALIS_THEME");
    if (themeEnv != nullptr && !strcasecmp(themeEnv, "DARK"))
        this->themeVariant = ThemeVariant::DARK;

    // Misc
    //    glfwSetTime(0.0);

    // Platform impls
    this->fontLoader  = new GLFWFontLoader();
    this->audioPlayer = new NullAudioPlayer();
}

void SDLPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight)
{
    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
}

bool SDLPlatform::canShowBatteryLevel()
{
    return true;
}

int battery = 50;
int SDLPlatform::getBatteryLevel()
{
    battery %= 100;
    battery++;
    return battery;
}

bool SDLPlatform::isBatteryCharging()
{
    return true;
}

bool SDLPlatform::hasWirelessConnection()
{
    return true;
}

int SDLPlatform::getWirelessLevel()
{
    return battery / 20;
}

std::string SDLPlatform::getIpAddress()
{
    return "0.0.0.0";
}

std::string SDLPlatform::getDnsServer()
{
    return "0.0.0.0";
}

bool SDLPlatform::isApplicationMode()
{
    return true;
}

void SDLPlatform::exitToHomeMode(bool value)
{
    return;
}

void SDLPlatform::forceEnableGamePlayRecording()
{
    return;
}

void SDLPlatform::openBrowser(std::string url)
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

std::string SDLPlatform::getName()
{
    return "SDL";
}

bool SDLPlatform::mainLoopIteration()
{

    bool platform = true;

#ifdef __SWITCH__
    platform = appletMainLoop();
#endif

    SDL_Event event;
    bool running = true;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            running = false;
        }
        else if (event.type == SDL_MOUSEWHEEL)
        {
            if (this->inputManager)
                this->inputManager->updateMouseWheel(event.wheel);
        }
    }

    return running || !platform;
}

AudioPlayer* SDLPlatform::getAudioPlayer()
{
    return this->audioPlayer;
}

VideoContext* SDLPlatform::getVideoContext()
{
    return this->videoContext;
}

InputManager* SDLPlatform::getInputManager()
{
    return this->inputManager;
}

FontLoader* SDLPlatform::getFontLoader()
{
    return this->fontLoader;
}

ThemeVariant SDLPlatform::getThemeVariant()
{
    return this->themeVariant;
}

void SDLPlatform::setThemeVariant(ThemeVariant theme)
{
    this->themeVariant = theme;
}

std::string SDLPlatform::getLocale()
{
    char* langEnv = getenv("BOREALIS_LANG");
    if (langEnv == nullptr)
        return LOCALE_DEFAULT;
    return std::string(langEnv);
}

SDLPlatform::~SDLPlatform()
{
    delete this->fontLoader;
    delete this->audioPlayer;
    delete this->videoContext;
    delete this->inputManager;
}

} // namespace brls
