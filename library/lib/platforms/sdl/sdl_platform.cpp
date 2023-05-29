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
#include <map>

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

void SDLPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    auto videoContext = new SDLVideoContext(windowTitle, windowWidth, windowHeight, windowXPos, windowYPos);
    this->videoContext = videoContext;
    this->inputManager = new SDLInputManager(this->videoContext->getSDLWindow());
    this->imeManager   = new SDLImeManager(videoContext->getOtherEvent());
}

void SDLPlatform::restoreWindow()
{
    SDL_RestoreWindow(this->videoContext->getSDLWindow());
}

void SDLPlatform::setWindowSize(uint32_t windowWidth, uint32_t windowHeight)
{
    if (windowWidth > 0 && windowHeight > 0) {
        SDL_SetWindowSize(this->videoContext->getSDLWindow(), windowWidth, windowHeight);
    }
}

void SDLPlatform::setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight)
{
    if (windowMinWidth > 0 && windowMinHeight > 0)
        SDL_SetWindowMinimumSize(this->videoContext->getSDLWindow(), windowMinWidth, windowMinHeight);
    if ((windowMaxWidth > 0 && windowMaxHeight > 0) && (windowMaxWidth > windowMinWidth && windowMaxHeight > windowMinHeight))
        SDL_SetWindowMaximumSize(this->videoContext->getSDLWindow(), windowMaxWidth, windowMaxHeight);
}

void SDLPlatform::setWindowPosition(int windowXPos, int windowYPos)
{
    SDL_SetWindowPosition(this->videoContext->getSDLWindow(), windowXPos, windowYPos);
}

void SDLPlatform::setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        SDL_Window* win = this->videoContext->getSDLWindow();
        SDL_RestoreWindow(win);
        SDL_SetWindowSize(win, windowWidth, windowHeight);
        SDL_SetWindowPosition(win, windowXPos, windowYPos);
    }
}

std::string SDLPlatform::getName()
{
    return "SDL";
}


const static SDL_Keymod keymods[] = {
    KMOD_SHIFT,
    KMOD_CTRL,
    KMOD_ALT,
    KMOD_GUI,
};

const static std::map<SDL_Scancode, BrlsKeyboardScancode> brlsKeyboardMap = {
    {SDL_SCANCODE_SPACE, BRLS_KBD_KEY_SPACE},
    {SDL_SCANCODE_APOSTROPHE, BRLS_KBD_KEY_APOSTROPHE},
    {SDL_SCANCODE_COMMA, BRLS_KBD_KEY_COMMA},
    {SDL_SCANCODE_MINUS, BRLS_KBD_KEY_MINUS},
    {SDL_SCANCODE_PERIOD, BRLS_KBD_KEY_PERIOD},
    {SDL_SCANCODE_SLASH, BRLS_KBD_KEY_SLASH},
    {SDL_SCANCODE_0, BRLS_KBD_KEY_0},
    {SDL_SCANCODE_1, BRLS_KBD_KEY_1},
    {SDL_SCANCODE_2, BRLS_KBD_KEY_2},
    {SDL_SCANCODE_3, BRLS_KBD_KEY_3},
    {SDL_SCANCODE_4, BRLS_KBD_KEY_4},
    {SDL_SCANCODE_5, BRLS_KBD_KEY_5},
    {SDL_SCANCODE_6, BRLS_KBD_KEY_6},
    {SDL_SCANCODE_7, BRLS_KBD_KEY_7},
    {SDL_SCANCODE_8, BRLS_KBD_KEY_8},
    {SDL_SCANCODE_9, BRLS_KBD_KEY_9},
    {SDL_SCANCODE_SEMICOLON, BRLS_KBD_KEY_SEMICOLON},
    {SDL_SCANCODE_EQUALS, BRLS_KBD_KEY_EQUAL},
    {SDL_SCANCODE_A, BRLS_KBD_KEY_A},
    {SDL_SCANCODE_B, BRLS_KBD_KEY_B},
    {SDL_SCANCODE_C, BRLS_KBD_KEY_C},
    {SDL_SCANCODE_D, BRLS_KBD_KEY_D},
    {SDL_SCANCODE_E, BRLS_KBD_KEY_E},
    {SDL_SCANCODE_F, BRLS_KBD_KEY_F},
    {SDL_SCANCODE_G, BRLS_KBD_KEY_G},
    {SDL_SCANCODE_H, BRLS_KBD_KEY_H},
    {SDL_SCANCODE_I, BRLS_KBD_KEY_I},
    {SDL_SCANCODE_J, BRLS_KBD_KEY_J},
    {SDL_SCANCODE_K, BRLS_KBD_KEY_K},
    {SDL_SCANCODE_L, BRLS_KBD_KEY_L},
    {SDL_SCANCODE_M, BRLS_KBD_KEY_M},
    {SDL_SCANCODE_N, BRLS_KBD_KEY_N},
    {SDL_SCANCODE_O, BRLS_KBD_KEY_O},
    {SDL_SCANCODE_P, BRLS_KBD_KEY_P},
    {SDL_SCANCODE_Q, BRLS_KBD_KEY_Q},
    {SDL_SCANCODE_R, BRLS_KBD_KEY_R},
    {SDL_SCANCODE_S, BRLS_KBD_KEY_S},
    {SDL_SCANCODE_T, BRLS_KBD_KEY_T},
    {SDL_SCANCODE_U, BRLS_KBD_KEY_U},
    {SDL_SCANCODE_V, BRLS_KBD_KEY_V},
    {SDL_SCANCODE_W, BRLS_KBD_KEY_W},
    {SDL_SCANCODE_X, BRLS_KBD_KEY_X},
    {SDL_SCANCODE_Y, BRLS_KBD_KEY_Y},
    {SDL_SCANCODE_Z, BRLS_KBD_KEY_Z},
    {SDL_SCANCODE_LEFTBRACKET, BRLS_KBD_KEY_LEFT_BRACKET},
    {SDL_SCANCODE_BACKSLASH, BRLS_KBD_KEY_BACKSLASH},
    {SDL_SCANCODE_RIGHTBRACKET, BRLS_KBD_KEY_RIGHT_BRACKET},
    {SDL_SCANCODE_GRAVE, BRLS_KBD_KEY_GRAVE_ACCENT},
    {SDL_SCANCODE_APP1, BRLS_KBD_KEY_WORLD_1},
    {SDL_SCANCODE_APP2, BRLS_KBD_KEY_WORLD_2},
    {SDL_SCANCODE_ESCAPE, BRLS_KBD_KEY_ESCAPE},
    {SDL_SCANCODE_RETURN, BRLS_KBD_KEY_ENTER},
    {SDL_SCANCODE_TAB, BRLS_KBD_KEY_TAB},
    {SDL_SCANCODE_BACKSPACE, BRLS_KBD_KEY_BACKSPACE},
    {SDL_SCANCODE_INSERT, BRLS_KBD_KEY_INSERT},
    {SDL_SCANCODE_DELETE, BRLS_KBD_KEY_DELETE},
    {SDL_SCANCODE_RIGHT, BRLS_KBD_KEY_RIGHT},
    {SDL_SCANCODE_LEFT, BRLS_KBD_KEY_LEFT},
    {SDL_SCANCODE_DOWN, BRLS_KBD_KEY_DOWN},
    {SDL_SCANCODE_UP, BRLS_KBD_KEY_UP},
    {SDL_SCANCODE_PAGEUP, BRLS_KBD_KEY_PAGE_UP},
    {SDL_SCANCODE_PAGEDOWN, BRLS_KBD_KEY_PAGE_DOWN},
    {SDL_SCANCODE_HOME, BRLS_KBD_KEY_HOME},
    {SDL_SCANCODE_END, BRLS_KBD_KEY_END},
    {SDL_SCANCODE_CAPSLOCK, BRLS_KBD_KEY_CAPS_LOCK},
    {SDL_SCANCODE_SCROLLLOCK, BRLS_KBD_KEY_SCROLL_LOCK},
    {SDL_SCANCODE_NUMLOCKCLEAR, BRLS_KBD_KEY_NUM_LOCK},
    {SDL_SCANCODE_PRINTSCREEN, BRLS_KBD_KEY_PRINT_SCREEN},
    {SDL_SCANCODE_PAUSE, BRLS_KBD_KEY_PAUSE},
    {SDL_SCANCODE_F1, BRLS_KBD_KEY_F1},
    {SDL_SCANCODE_F2, BRLS_KBD_KEY_F2},
    {SDL_SCANCODE_F3, BRLS_KBD_KEY_F3},
    {SDL_SCANCODE_F4, BRLS_KBD_KEY_F4},
    {SDL_SCANCODE_F5, BRLS_KBD_KEY_F5},
    {SDL_SCANCODE_F6, BRLS_KBD_KEY_F6},
    {SDL_SCANCODE_F7, BRLS_KBD_KEY_F7},
    {SDL_SCANCODE_F8, BRLS_KBD_KEY_F8},
    {SDL_SCANCODE_F9, BRLS_KBD_KEY_F9},
    {SDL_SCANCODE_F10, BRLS_KBD_KEY_F10},
    {SDL_SCANCODE_F11, BRLS_KBD_KEY_F11},
    {SDL_SCANCODE_F12, BRLS_KBD_KEY_F12},
    {SDL_SCANCODE_F13, BRLS_KBD_KEY_F13},
    {SDL_SCANCODE_F14, BRLS_KBD_KEY_F14},
    {SDL_SCANCODE_F15, BRLS_KBD_KEY_F15},
    {SDL_SCANCODE_F16, BRLS_KBD_KEY_F16},
    {SDL_SCANCODE_F17, BRLS_KBD_KEY_F17},
    {SDL_SCANCODE_F18, BRLS_KBD_KEY_F18},
    {SDL_SCANCODE_F19, BRLS_KBD_KEY_F19},
    {SDL_SCANCODE_F20, BRLS_KBD_KEY_F20},
    {SDL_SCANCODE_F21, BRLS_KBD_KEY_F21},
    {SDL_SCANCODE_F22, BRLS_KBD_KEY_F22},
    {SDL_SCANCODE_F23, BRLS_KBD_KEY_F23},
    {SDL_SCANCODE_F24, BRLS_KBD_KEY_F24},
    {SDL_SCANCODE_KP_0, BRLS_KBD_KEY_KP_0},
    {SDL_SCANCODE_KP_1, BRLS_KBD_KEY_KP_1},
    {SDL_SCANCODE_KP_2, BRLS_KBD_KEY_KP_2},
    {SDL_SCANCODE_KP_3, BRLS_KBD_KEY_KP_3},
    {SDL_SCANCODE_KP_4, BRLS_KBD_KEY_KP_4},
    {SDL_SCANCODE_KP_5, BRLS_KBD_KEY_KP_5},
    {SDL_SCANCODE_KP_6, BRLS_KBD_KEY_KP_6},
    {SDL_SCANCODE_KP_7, BRLS_KBD_KEY_KP_7},
    {SDL_SCANCODE_KP_8, BRLS_KBD_KEY_KP_8},
    {SDL_SCANCODE_KP_9, BRLS_KBD_KEY_KP_9},
    {SDL_SCANCODE_KP_DECIMAL, BRLS_KBD_KEY_KP_DECIMAL},
    {SDL_SCANCODE_KP_DIVIDE, BRLS_KBD_KEY_KP_DIVIDE},
    {SDL_SCANCODE_KP_MULTIPLY, BRLS_KBD_KEY_KP_MULTIPLY},
    {SDL_SCANCODE_KP_MEMSUBTRACT, BRLS_KBD_KEY_KP_SUBTRACT},
    {SDL_SCANCODE_KP_MEMADD, BRLS_KBD_KEY_KP_ADD},
    {SDL_SCANCODE_KP_ENTER, BRLS_KBD_KEY_KP_ENTER},
    {SDL_SCANCODE_KP_EQUALS, BRLS_KBD_KEY_KP_EQUAL},
    {SDL_SCANCODE_LSHIFT, BRLS_KBD_KEY_LEFT_SHIFT},
    {SDL_SCANCODE_LCTRL, BRLS_KBD_KEY_LEFT_CONTROL},
    {SDL_SCANCODE_LALT, BRLS_KBD_KEY_LEFT_ALT},
    {SDL_SCANCODE_LGUI, BRLS_KBD_KEY_LEFT_SUPER},
    {SDL_SCANCODE_RSHIFT, BRLS_KBD_KEY_RIGHT_SHIFT},
    {SDL_SCANCODE_RCTRL, BRLS_KBD_KEY_RIGHT_CONTROL},
    {SDL_SCANCODE_RALT, BRLS_KBD_KEY_RIGHT_ALT},
    {SDL_SCANCODE_RGUI, BRLS_KBD_KEY_RIGHT_SUPER},
    {SDL_SCANCODE_MENU, BRLS_KBD_KEY_MENU}
};

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
        else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            if (brlsKeyboardMap.find(event.key.keysym.scancode) != brlsKeyboardMap.end()) {
                int action = event.key.state == SDL_PRESSED ? BRLS_KBD_ACTION_PRESS : BRLS_KBD_ACTION_RELSAE;
                if (event.key.repeat > 0) {
                    action = BRLS_KBD_ACTION_REPEAT;
                }
                int index = 0;
                int keyModifiers = 0;
                for (auto& mod: keymods) {
                    if (mod & event.key.keysym.mod) {
                        keyModifiers |= brls_keymods[index];
                    }
                    index++;
                }

                this->inputManager->keyboardCallback(
                    this->videoContext->getSDLWindow(),
                    brlsKeyboardMap.at(event.key.keysym.scancode),
                    action,
                    keyModifiers);
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
            this->inputManager->controllerCallback(
                event.cbutton.button,
                event.cbutton.state);
        } else if (event.type == SDL_CONTROLLERAXISMOTION) {
            this->inputManager->controllerAxisCallback(
                event.caxis.axis,
                event.caxis.value);
        } else if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED) {
            this->inputManager->updateControllers();
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

ImeManager* SDLPlatform::getImeManager() {
    return this->imeManager;
}

SDLPlatform::~SDLPlatform()
{
    delete this->audioPlayer;
    delete this->videoContext;
    delete this->inputManager;
}

} // namespace brls
