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

#include <functional>
#include <string>

#include <borealis/core/event.hpp>
#include <borealis/core/ime.hpp>
#include <borealis/platforms/sdl/sdl_ime.hpp>

namespace brls
{

/**
 * The console's own keyboard.
 *
 * SDL's text input is a desktop idea: nothing in the PS5 port of SDL shows a
 * keyboard for it, so the dialog a player types into here is the system's
 * (libSceImeDialog), the one that knows the console's languages and layouts.
 * It is driven from the main loop, so the application goes on running while
 * the keyboard is up.
 *
 * If the console's library cannot be used, the SDL dialog is still there so
 * the text field keeps working the way it did before.
 */
class Ps5ImeManager : public ImeManager
{
  public:
    explicit Ps5ImeManager(Event<SDL_Event*>* event) : fallback(event) { }

    bool openForText(std::function<void(std::string)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 32, std::string initialText = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;

    bool openForNumber(std::function<void(long)> f, std::string headerText = "",
        std::string subText = "", int maxStringLength = 18, std::string initialText = "",
        std::string leftButton = "", std::string rightButton = "",
        int kbdDisableBitmask = KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE) override;

  private:
    // The system dialog is shown here; false means it is not available and the
    // caller should fall back to the SDL one.
    bool openSystemDialog(std::function<void(std::string)> cb, int type,
        const std::string& headerText, const std::string& subText, int maxStringLength,
        const std::string& initialText);

    SDLImeManager fallback;
};

} // namespace brls
