/*
    Copyright 2021 CrustySeanPro

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

#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/views/image.hpp>
#include <borealis/core/assets.hpp>

extern "C" {
    #include <switch.h>
}

namespace brls
{

class AccountWidget : public Box
{
public:
    AccountWidget();
    void getIcon(AccountUid id);
    
    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    static View* create();
    
private:
    Image* acc;
    AccountUid userID = {0};
    Result rc = 0;
};


} // namespace brls