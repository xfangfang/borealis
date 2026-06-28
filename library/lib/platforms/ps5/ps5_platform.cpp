/*
Copyright 2026 xfangfang

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

#include <SDL2/SDL.h>

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/thread.hpp>
#include <borealis/platforms/ps5/ps5_platform.hpp>

extern "C" int sceSystemServiceLoadExec(const char* path, const char* args[]);

namespace brls
{

Ps5Platform::Ps5Platform()
{
    // Initialize here for loading the system modules
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        Logger::error("sdl: failed to initialize video");
    }
}

Ps5Platform::~Ps5Platform() = default;

std::string Ps5Platform::getName()
{
    return "Ps5";
}

void Ps5Platform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    windowWidth  = 1920;
    windowHeight = 1080;

    this->videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight, NAN, NAN);
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
    this->imeManager   = new SDLImeManager(&this->otherEvent);
}

bool Ps5Platform::canShowWirelessLevel()
{
    return true;
}

bool Ps5Platform::hasWirelessConnection()
{
    return false;
}

int Ps5Platform::getWirelessLevel()
{
    return 0;
}

bool Ps5Platform::hasEthernetConnection()
{
    return false;
}

std::string Ps5Platform::getIpAddress()
{
    return "-";
}

std::string Ps5Platform::getDnsServer()
{
    return "-";
}

void Ps5Platform::openBrowser(std::string url)
{

}

} // namespace brls
