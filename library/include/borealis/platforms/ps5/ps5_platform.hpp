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

#pragma once

#include <borealis/platforms/sdl/sdl_platform.hpp>

namespace brls
{

    class Ps5Platform : public SDLPlatform
    {
    public:
        Ps5Platform();
        ~Ps5Platform() override;
        std::string getName() override;
        void createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos) override;
        bool canShowWirelessLevel() override;
        bool hasWirelessConnection() override;
        int getWirelessLevel() override;
        bool hasEthernetConnection() override;
        std::string getIpAddress() override;
        std::string getDnsServer() override;
        void openBrowser(std::string url) override;

    private:
    };

} // namespace brls
