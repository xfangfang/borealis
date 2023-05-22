/*
Copyright 2023 zeromake

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

#include <borealis/platforms/sdl/sdl_ime.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/event.hpp>
#include <borealis/views/edit_text_dialog.hpp>
#include <borealis/core/application.hpp>

namespace brls
{
    SDLImeManager::SDLImeManager(Event<SDL_Event*> *event):
    event(event),
    cursor(0){}

    void SDLImeManager::openInputDialog(
        std::function<void(std::string)> cb,
        std::string headerText,
        std::string subText,
        size_t maxStringLength,
        std::string initialText) {
        EditTextDialog* dialog = new EditTextDialog();
        this->inputBuffer = initialText;
        dialog->setText(initialText);
        dialog->setHeaderText(headerText);
        dialog->setCountText("0/" + std::to_string(maxStringLength));
        float scale = Application::windowScale / Application::getPlatform()->getVideoContext()->getScaleFactor();
        // 更新输入法条位置
        dialog->getLayoutEvent()->subscribe([this, scale](Point p) {
            const SDL_Rect rect = {(p.x) * scale, (p.y) * scale, 10, 10};
            SDL_SetTextInputRect(&rect);
        });
        auto updateText = [this, dialog, maxStringLength]() {
            dialog->setText(this->inputBuffer);
            dialog->setCountText(std::to_string(this->inputBuffer.length())+"/" + std::to_string(maxStringLength));
        };
        auto eventID1 = event->subscribe([this, updateText](SDL_Event *e) {
            switch (e->type) {
            case SDL_TEXTINPUT:
                if (strlen(e->edit.text) == 0) {
                    this->isEditing = false;
                } else {
                    this->isEditing = true;
                }
                this->inputBuffer += e->text.text;
                updateText();
                break;
            case SDL_TEXTEDITING:
                this->isEditing = false;
                break;
            }
        });

        // delete text
        dialog->registerAction("hints/delete"_i18n, BUTTON_B, [this, updateText](...) {
            if(inputBuffer.empty()) return true;
            inputBuffer.erase(inputBuffer.size()-1, 1);
            updateText();
            return true;
        },true, true);

        // cancel
        dialog->getCancelEvent()->subscribe([this, eventID1]() {
            SDL_StopTextInput();
            event->unsubscribe(eventID1);
        });

        // submit
        dialog->getSubmitEvent()->subscribe([this, eventID1, cb]() {
            SDL_StopTextInput();
            event->unsubscribe(eventID1);
            cb(this->inputBuffer);
            return true;
        });

        dialog->open();
        SDL_StartTextInput();
    }

    bool SDLImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
        std::string subText, int maxStringLength, std::string initialText,
        int kbdDisableBitmask)
    {
        this->openInputDialog([f](const std::string& text)
            {if(!text.empty()) f(text); },
            headerText, subText, maxStringLength, initialText);
        return true;
    }

    bool SDLImeManager::openForNumber(std::function<void(long)> f, std::string headerText,
        std::string subText, int maxStringLength, std::string initialText,
        std::string leftButton, std::string rightButton,
        int kbdDisableBitmask)
    {
        this->openInputDialog([f](const std::string& text) {
            if(text.empty()) return ;
            try
            {
                f(stoll(text));
            }
            catch (const std::exception& e)
            {
                Logger::error("Could not parse input, did you enter a valid integer?");
            }
        },headerText, subText, maxStringLength, initialText);
        return true;
    }
}