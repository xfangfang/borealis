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

#include <borealis/core/thread.hpp>
#include <borealis/views/debug_layer.hpp>
#include <borealis/views/label.hpp>

namespace brls
{

DebugLayer::DebugLayer()
    : Box(Axis::COLUMN)
{
    setWidth(1280);
    setHeight(View::AUTO);

    setJustifyContent(JustifyContent::FLEX_START);
    setAlignItems(AlignItems::FLEX_END);

    Box* contentView = new Box(Axis::COLUMN);
    addView(contentView);
    contentView->setWidth(600);
    contentView->setBackgroundColor(RGBA(0, 0, 0, 80));

    Logger::getLogEvent()->subscribe([this, contentView](std::string log) {
        brls::sync([this, contentView, log] {
            Label* label = new Label();
            label->setText(log);
            label->setFontSize(8);
            label->setTextColor(RGB(255, 0, 0));
            label->setLineBottom(1);
            label->setLineColor(RGB(255, 255, 255));
            contentView->addView(label, 0);

            if (contentView->getChildren().size() > 50)
                contentView->removeView(contentView->getChildren()[contentView->getChildren().size() - 1]);
        });
    });
}

} // namespace brls
