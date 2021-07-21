/*
    Copyright 2021 XITRIX

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

#include "borealis/views/widgets/battery.hpp"

#define BATTERY_MAX_WIDTH 23.0f

namespace brls
{

BatteryWidget::BatteryWidget()
{
    setSize(Size(44, 44));
    
    back = new Image();
    back->setSize(Size(44, 44));
    back->setScalingType(ImageScalingType::FIT);
    back->detach();
    
    level = new Rectangle();
    level->setDetachedPosition(11, 18);
    level->setSize(Size(BATTERY_MAX_WIDTH, 10));
    level->detach();
    
    platform = Application::getPlatform();
    applyBackTheme(platform->getThemeVariant());
    applyLevelTheme(platform->getThemeVariant());
    
    addView(level);
    addView(back);
}

void BatteryWidget::applyBackTheme(ThemeVariant theme)
{
    switch (theme) {
        case ThemeVariant::LIGHT:
            back->setImageFromRes("img/sys/battery_back_light.png");
            break;
        case ThemeVariant::DARK:
            back->setImageFromRes("img/sys/battery_back_dark.png");
            break;
    }
}

void BatteryWidget::applyLevelTheme(ThemeVariant theme)
{
    switch (theme) {
        case ThemeVariant::LIGHT:
            level->setColor(RGB(0, 0, 0));
            break;
        case ThemeVariant::DARK:
            level->setColor(RGB(255, 255, 255));
            break;
    }
}

void BatteryWidget::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    if (platform->isBatteryCharging())
        level->setColor(RGB(140, 251, 79));
    else
        applyLevelTheme(platform->getThemeVariant());
    
    level->setWidth(BATTERY_MAX_WIDTH * platform->getBatteryLevel() / 100.0f);
    Box::draw(vg, x, y, width, height, style, ctx);
}

View* BatteryWidget::create()
{
    return new BatteryWidget();
}

} // namespace brls
