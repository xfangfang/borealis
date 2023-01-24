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

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_platform.hpp>

namespace brls
{

SDLPlatform::SDLPlatform()
{
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
    if (windowWidth > 0 && windowHeight > 0)
        SDL_SetWindowSize(this->videoContext->getSDLWindow(), windowWidth, windowHeight);
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

SDLPlatform::~SDLPlatform()
{
    delete this->audioPlayer;
    delete this->videoContext;
    delete this->inputManager;
}

} // namespace brls
