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
    cursor(-1){}

    static int utf8_len(std::string &s) {
        int result = 0;
        for (auto &it: s) {
            if ((it & 0xc0) != 0x80) {
                result += 1;
            }
        }
        return result;
    }

    static int utf8_find_prev(std::string &s, int size) {
        int result = 0;
        for (int i = s.size() - 1; i >= 0; i--) {
            char p = s.at(i);
            result += 1;
            if ((p & 0xc0) != 0x80) {
                size--;
            }
            if (size <= 0) {
                break;
            }
        }
        return result;
    }

    static int utf8_find_next(std::string &s, int offset, int size) {
        int result = 0;
        if (size == 0) {
            return 0;
        }
        for (int i = offset; i < s.size(); i++) {
            char p = s.at(i);
            if ((p & 0xc0) != 0x80) {
                size--;
            }
            if (size < 0) {
                break;
            }
            result += 1;
        }
        return result;
    }

    void SDLImeManager::openInputDialog(
        std::function<void(std::string)> cb,
        std::string headerText,
        std::string subText,
        size_t maxStringLength,
        std::string initialText) {
        EditTextDialog* dialog = new EditTextDialog();
        this->inputBuffer = initialText;
        auto updateText = [this, dialog, maxStringLength]() {
            dialog->setText(this->inputBuffer);
            dialog->setCountText(fmt::format("{}/{}", utf8_len(this->inputBuffer), maxStringLength));
        };
        auto updateTextCursor = [this, dialog]() {
            dialog->setCursor(this->cursor);
        };
        auto updateTextAndCursor = [this, updateText, updateTextCursor, maxStringLength](std::string text) {
            int prev_n = utf8_len(this->inputBuffer);
            if (prev_n >= maxStringLength) {
                return;
            }
            int n = utf8_len(text);
            if (prev_n + n > maxStringLength) {
                n = maxStringLength - prev_n;
                int end = utf8_find_next(text, 0, n);
                text = text.substr(0, end);
            }
            if (this->cursor >= 0) {
                int start = utf8_find_next(this->inputBuffer, 0, this->cursor);
                this->inputBuffer.insert(start, text);
                this->cursor += n;
            } else {
                this->inputBuffer += text;
            }
            updateTextCursor();
            updateText();
        };
        dialog->setHeaderText(headerText);
        updateText();
        float scale = Application::windowScale / Application::getPlatform()->getVideoContext()->getScaleFactor();
        // 更新输入法条位置
        dialog->getLayoutEvent()->subscribe([this, scale](Point p) {
            const SDL_Rect rect = {(int)(p.x* scale), (int)(p.y * scale), 100, 20};
            SDL_SetTextInputRect(&rect);
        });
        auto eventID1 = event->subscribe([this, updateTextAndCursor](SDL_Event *e) {
            switch (e->type) {
            case SDL_TEXTINPUT:
                if (strlen(e->edit.text) == 0) {
                    this->isEditing = false;
                } else {
                    this->isEditing = true;
                }
                updateTextAndCursor(e->text.text);
                break;
            case SDL_TEXTEDITING:
                this->isEditing = false;
                break;
            }
        });
        
        dialog->registerAction(
            "hints/left"_i18n, BUTTON_LEFT, [this, updateTextCursor](...){
                if (this->cursor == -1) {
                    this->cursor = utf8_len(this->inputBuffer) - 1;
                    updateTextCursor();
                } else if (this->cursor > 0) {
                    this->cursor--;
                    updateTextCursor();
                }
                return true;
            }, true, true
        );
        dialog->registerAction(
            "hints/right"_i18n, BUTTON_RIGHT, [this, updateTextCursor](...){
                if (this->cursor >= 0) {
                    if (this->cursor < utf8_len(this->inputBuffer)) {
                        this->cursor++;
                        updateTextCursor();
                    }
                }
                return true;
            }, true, true
        );
        dialog->registerKeysAction(
            "", "", {BUTTON_RB, BUTTON_LEFT}, 0, {BRLS_KBD_KEY_HOME}, [this, updateTextCursor](...){
                if (this->cursor != 0) {
                    this->cursor = 0;
                    updateTextCursor();
                }
                return true;
            }, true
        );
        dialog->registerKeysAction(
            "", "", {BUTTON_RB, BUTTON_RIGHT}, 0, {BRLS_KBD_KEY_END}, [this, updateTextCursor](...){
                if (this->cursor != -1) {
                    this->cursor = -1;
                    updateTextCursor();
                }
                return true;
            }, true
        );
#if defined(__APPLE__)
        static BrlsKeyboardModifiers mods = BRLS_KBD_MODIFIER_META;
#else
        static BrlsKeyboardModifiers mods = BRLS_KBD_MODIFIER_CTRL;
#endif
        dialog->registerKeysAction(
            "\uE0E3", "hints/copy"_i18n, {BUTTON_RB, BUTTON_Y}, mods, {BRLS_KBD_KEY_C}, [this](...){
                if (!this->inputBuffer.empty())
                    SDL_SetClipboardText(this->inputBuffer.data());
                return true;
            }
        );
        dialog->registerKeysAction(
            "\uE0E2", "hints/paste"_i18n, {BUTTON_RB, BUTTON_X}, mods, {BRLS_KBD_KEY_V}, [this, updateTextAndCursor](...){
                if (SDL_HasClipboardText()) {
                    char* clipboard = SDL_GetClipboardText();
                    updateTextAndCursor(clipboard);
                }
                return true;
            }
        );

        // delete text
        dialog->registerAction("hints/delete"_i18n, BUTTON_B, [this, updateText, updateTextCursor](...) {
            if(inputBuffer.empty()) return true;
            if (this->cursor == 0) {
                return true;
            }
            if (this->cursor > 0) {
                int start = utf8_find_next(inputBuffer, 0, this->cursor-1);
                int n = utf8_find_next(inputBuffer, start, 1);
                inputBuffer.erase(start, n);
                this->cursor -= 1;
                updateTextCursor();
            } else {
                int offset = utf8_find_prev(inputBuffer, 1);
                inputBuffer.erase(inputBuffer.size()-offset, offset);
            }
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

        SDL_StartTextInput();
        dialog->open();
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