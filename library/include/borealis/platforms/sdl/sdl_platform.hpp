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

#pragma once

#include <SDL2/SDL.h>

#include <borealis/platforms/desktop/desktop_platform.hpp>
#include <borealis/platforms/sdl/sdl_input.hpp>
#include <borealis/platforms/sdl/sdl_video.hpp>

namespace brls
{

class SDLPlatform : public DesktopPlatform
{
  public:
    SDLPlatform();
    ~SDLPlatform();

    std::string getName() override;
    void createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos) override;

    virtual void restoreWindow() override;
    virtual void setWindowSize(uint32_t windowWidth, uint32_t windowHeight) override;
    virtual void setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight) override;
    virtual void setWindowPosition(int windowXPos, int windowYPos) override;
    virtual void setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos) override;
    void disableScreenDimming(bool disable, const std::string& reason, const std::string& app) override;
    bool isScreenDimmingDisabled() override;

    bool mainLoopIteration() override;

    AudioPlayer* getAudioPlayer() override;
    VideoContext* getVideoContext() override;
    InputManager* getInputManager() override;

  private:
    NullAudioPlayer* audioPlayer  = nullptr;
    SDLVideoContext* videoContext = nullptr;
    SDLInputManager* inputManager = nullptr;
};

} // namespace brls
