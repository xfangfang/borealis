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

void SDLPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight)
{
    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
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
