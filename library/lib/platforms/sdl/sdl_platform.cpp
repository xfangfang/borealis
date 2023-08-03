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

#include <strings.h>

#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_platform.hpp>

#ifdef __PSV__
extern "C"
{
    unsigned int _newlib_heap_size_user = 128 * 1024 * 1024;
    unsigned int sceLibcHeapSize        = 64 * 1024 * 1024;
}
#endif

#ifdef IOS
#include <sys/utsname.h>

static bool isIPad()
{
    struct utsname systemInfo;
    uname(&systemInfo);
    return strncmp(systemInfo.machine, "iPad", 4) == 0;
}
#endif

namespace brls
{

SDLPlatform::SDLPlatform()
{
#ifdef ANDROID
    // Enable Fullscreen on Android
    VideoContext::FULLSCREEN = true;
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#elif defined(IOS)
    // Enable Fullscreen on iOS
    VideoContext::FULLSCREEN = true;
    if (!isIPad()){
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    }
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#endif

    // Init sdl
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0)
    {
        Logger::error("sdl: failed to initialize");
        return;
    }

    // Platform impls
    this->audioPlayer = new NullAudioPlayer();
}

void SDLPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight, windowXPos, windowYPos);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
}

void SDLPlatform::restoreWindow()
{
    SDL_RestoreWindow(this->videoContext->getSDLWindow());
}

void SDLPlatform::setWindowSize(uint32_t windowWidth, uint32_t windowHeight)
{
    if (windowWidth > 0 && windowHeight > 0) {
        SDL_SetWindowSize(this->videoContext->getSDLWindow(), windowWidth, windowHeight);
    }
}

void SDLPlatform::setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight)
{
    if (windowMinWidth > 0 && windowMinHeight > 0)
        SDL_SetWindowMinimumSize(this->videoContext->getSDLWindow(), windowMinWidth, windowMinHeight);
    if ((windowMaxWidth > 0 && windowMaxHeight > 0) && (windowMaxWidth > windowMinWidth && windowMaxHeight > windowMinHeight))
        SDL_SetWindowMaximumSize(this->videoContext->getSDLWindow(), windowMaxWidth, windowMaxHeight);
}

void SDLPlatform::setWindowPosition(int windowXPos, int windowYPos)
{
    SDL_SetWindowPosition(this->videoContext->getSDLWindow(), windowXPos, windowYPos);
}

void SDLPlatform::setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        SDL_Window* win = this->videoContext->getSDLWindow();
        SDL_RestoreWindow(win);
        SDL_SetWindowSize(win, windowWidth, windowHeight);
        SDL_SetWindowPosition(win, windowXPos, windowYPos);
    }
}

void SDLPlatform::disableScreenDimming(bool disable, const std::string& reason, const std::string& app)
{
    if (disable)
    {
        SDL_DisableScreenSaver();
    }
    else
    {
        SDL_EnableScreenSaver();
    }
}

bool SDLPlatform::isScreenDimmingDisabled()
{
    return !SDL_IsScreenSaverEnabled();
}

std::string SDLPlatform::getName()
{
    return "SDL";
}

static bool processEvent(SDLPlatform* platform, SDL_Event* event)
{
    if (event->type == SDL_QUIT)
    {
        return false;
    }
    else if (event->type == SDL_MOUSEWHEEL)
    {
        auto* manager = (SDLInputManager*)platform->getInputManager();
        if (manager)
            manager->updateMouseWheel(event->wheel);
    }
#ifdef IOS
    else if (event->type == SDL_APP_WILLENTERBACKGROUND)
    {
        brls::Application::getWindowFocusChangedEvent()->fire(false);
    }
    else if (event->type == SDL_APP_WILLENTERFOREGROUND)
    {
        brls::Application::getWindowFocusChangedEvent()->fire(true);
    }
#endif
    brls::Application::setActiveEvent(true);
    return true;
}

bool SDLPlatform::mainLoopIteration()
{
    SDL_Event event;
    bool hasEvent = false;
    while (SDL_PollEvent(&event))
    {
        if (!processEvent(this, &event))
        {
            return false;
        }
        hasEvent = true;
    }
    if (!hasEvent && !Application::hasActiveEvent())
    {
        if (SDL_WaitEventTimeout(&event, (int)(brls::Application::getDeactivatedFrameTime() * 1000))
            && !processEvent(this, &event))
        {
            return false;
        }
    }

    return true;
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

SDLPlatform::~SDLPlatform()
{
    delete this->audioPlayer;
    delete this->videoContext;
    delete this->inputManager;
}

} // namespace brls
