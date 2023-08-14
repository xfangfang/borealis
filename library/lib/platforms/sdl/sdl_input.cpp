/*
    Copyright 2021 natinusala
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

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_input.hpp>

namespace brls
{

#define SDL_GAMEPAD_BUTTON_NONE SIZE_MAX
#define SDL_GAMEPAD_BUTTON_MAX 15
#define SDL_GAMEPAD_AXIS_MAX 4

// LT and RT do not exist here because they are axes
static const size_t SDL_BUTTONS_MAPPING[SDL_GAMEPAD_BUTTON_MAX] = {
    BUTTON_A, // SDL_CONTROLLER_BUTTON_A
    BUTTON_B, // SDL_CONTROLLER_BUTTON_B
    BUTTON_X, // SDL_CONTROLLER_BUTTON_X
    BUTTON_Y, // SDL_CONTROLLER_BUTTON_Y
    BUTTON_BACK, // SDL_CONTROLLER_BUTTON_BACK
    BUTTON_GUIDE, // SDL_CONTROLLER_BUTTON_GUIDE
    BUTTON_START, // SDL_CONTROLLER_BUTTON_START
    BUTTON_LSB, //    SDL_CONTROLLER_BUTTON_LEFTSTICK
    BUTTON_RSB, //    SDL_CONTROLLER_BUTTON_RIGHTSTICK
    BUTTON_LB, //    SDL_CONTROLLER_BUTTON_LEFTSHOULDER
    BUTTON_RB, //    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
    BUTTON_UP, //    SDL_CONTROLLER_BUTTON_DPAD_UP
    BUTTON_DOWN, //    SDL_CONTROLLER_BUTTON_DPAD_DOWN
    BUTTON_LEFT, //    SDL_CONTROLLER_BUTTON_DPAD_LEFT
    BUTTON_RIGHT, //    SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

static const size_t SDL_GAMEPAD_TO_KEYBOARD[SDL_GAMEPAD_BUTTON_MAX] = {
    SDL_SCANCODE_RETURN, // SDL_CONTROLLER_BUTTON_A
    SDL_SCANCODE_BACKSPACE, // SDL_CONTROLLER_BUTTON_B
    SDL_SCANCODE_X, // SDL_CONTROLLER_BUTTON_X
    SDL_SCANCODE_Y, // SDL_CONTROLLER_BUTTON_Y
    SDL_SCANCODE_F1, // SDL_CONTROLLER_BUTTON_BACK
    SDL_SCANCODE_UNKNOWN, // SDL_CONTROLLER_BUTTON_GUIDE
    SDL_SCANCODE_F2, // SDL_CONTROLLER_BUTTON_START
    SDL_SCANCODE_Q, // SDL_CONTROLLER_BUTTON_LEFTSTICK
    SDL_SCANCODE_P, // SDL_CONTROLLER_BUTTON_RIGHTSTICK
    SDL_SCANCODE_L, // SDL_CONTROLLER_BUTTON_LEFTSHOULDER
    SDL_SCANCODE_R, // SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
    SDL_SCANCODE_UP, // SDL_CONTROLLER_BUTTON_DPAD_UP
    SDL_SCANCODE_DOWN, // SDL_CONTROLLER_BUTTON_DPAD_DOWN
    SDL_SCANCODE_LEFT, // SDL_CONTROLLER_BUTTON_DPAD_LEFT
    SDL_SCANCODE_RIGHT, // SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

static const size_t SDL_AXIS_MAPPING[SDL_GAMEPAD_AXIS_MAX] = {
    LEFT_X,
    LEFT_Y,
    RIGHT_X,
    RIGHT_Y,
};

static const SDL_Keymod keymods[] = {
    KMOD_SHIFT,
    KMOD_CTRL,
    KMOD_ALT,
    KMOD_GUI,
};

static const std::map<SDL_Scancode, BrlsKeyboardScancode> brlsKeyboardMap = {
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

std::unordered_map<SDL_JoystickID, SDL_GameController*> controllers;

static int sdlEventWatcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_CONTROLLERDEVICEADDED)
    {
        SDL_GameController* controller = SDL_GameControllerOpen(event->cdevice.which);
        if (controller)
        {
            SDL_JoystickID jid = SDL_JoystickGetDeviceInstanceID(event->cdevice.which);
            Logger::info("Controller connected: {}/{}", jid, SDL_GameControllerName(controller));
            controllers.insert({ jid, controller });
        }
    }
    else if (event->type == SDL_CONTROLLERDEVICEREMOVED)
    {
        Logger::info("Controller disconnected: {}", event->cdevice.which);
        controllers.erase(event->cdevice.which);
    }
    Application::setActiveEvent(true);
    return 0;
}

SDLInputManager::SDLInputManager(SDL_Window* window)
    : window(window)
{

    int32_t flags = SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;
#ifndef __WINRT__
    flags |= SDL_INIT_HAPTIC;
#endif
    if (SDL_Init(flags) < 0)
    {
        brls::fatal("Couldn't initialize joystick: " + std::string(SDL_GetError()));
    }

    int controllersCount = SDL_NumJoysticks();
    brls::Logger::info("joystick num: {}", controllersCount);

    for (int i = 0; i < controllersCount; i++)
    {
        SDL_JoystickID jid = SDL_JoystickGetDeviceInstanceID(i);
        Logger::info("sdl: joystick {}: \"{}\"", jid, SDL_JoystickNameForIndex(i));
        controllers.insert({ jid, SDL_GameControllerOpen(i) });
    }

    SDL_AddEventWatch(sdlEventWatcher, window);

    Application::getRunLoopEvent()->subscribe([this]()
        {
        if(fabs(scrollOffset.y) < 1) scrollOffset.y = 0;
        else scrollOffset.y *= 0.8;
        if(fabs(scrollOffset.x) < 1) scrollOffset.x = 0;
        else scrollOffset.x *= 0.8;

        pointerOffset.x = 0;
        pointerOffset.y = 0; });
}

SDLInputManager::~SDLInputManager()
{
    for (auto i : controllers)
    {
        SDL_GameControllerClose(i.second);
    }
}

short SDLInputManager::getControllersConnectedCount()
{
    return controllers.size();
}

void SDLInputManager::updateControllers() {
    int controllerNum = SDL_NumJoysticks();
    for (auto i : controllers)
    {
        SDL_GameControllerClose(i);
    }
    controllers.clear();
    controllers.reserve(controllerNum);
    for (int i = 0; i < controllerNum; i++) {
        controllers.emplace_back(SDL_GameControllerOpen(i));
    }
}

void SDLInputManager::updateUnifiedControllerState(ControllerState* state)
{
    for (bool& button : state->buttons)
        button = false;

    for (float& axe : state->axes)
        axe = 0;

    for (auto& c : controllers)
    {
        ControllerState localState {};
        updateControllerState(&localState, c.first);

        for (size_t i = 0; i < _BUTTON_MAX; i++)
            state->buttons[i] |= localState.buttons[i];

        for (size_t i = 0; i < _AXES_MAX; i++)
        {
            state->axes[i] += localState.axes[i];

            if (state->axes[i] < -1)
                state->axes[i] = -1;
            else if (state->axes[i] > 1)
                state->axes[i] = 1;
        }
    }

    // Add keyboard keys on top of gamepad buttons
    const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
    for (size_t i = 0; i < SDL_GAMEPAD_BUTTON_MAX; i++)
    {
        size_t brlsButton = SDL_BUTTONS_MAPPING[i];
        size_t key        = SDL_GAMEPAD_TO_KEYBOARD[i];
        if (key != SDL_SCANCODE_UNKNOWN)
            state->buttons[brlsButton] |= keyboard[key] != 0;
    }
    state->buttons[BUTTON_NAV_UP] |= state->buttons[BUTTON_UP];
    state->buttons[BUTTON_NAV_RIGHT] |= state->buttons[BUTTON_RIGHT];
    state->buttons[BUTTON_NAV_DOWN] |= state->buttons[BUTTON_DOWN];
    state->buttons[BUTTON_NAV_LEFT] |= state->buttons[BUTTON_LEFT];
}

void SDLInputManager::updateControllerState(ControllerState* state, int controller)
{
    if (controllers.find(controller) == controllers.end())
        return;
    SDL_GameController* c = controllers[controller];

    for (size_t i = 0; i < SDL_GAMEPAD_BUTTON_MAX; i++)
    {
        // Translate SDL gamepad to borealis controller
        size_t brlsButton          = SDL_BUTTONS_MAPPING[i];
        state->buttons[brlsButton] = (bool)SDL_GameControllerGetButton(c, (SDL_GameControllerButton)i);
    }

    state->buttons[BUTTON_LT] = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 3276.7f;
    state->buttons[BUTTON_RT] = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 3276.7f;

    state->buttons[BUTTON_NAV_UP]    = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY) < -16383.5f || SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY) < -16383.5f || state->buttons[BUTTON_UP];
    state->buttons[BUTTON_NAV_RIGHT] = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX) > 16383.5f || SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX) > 16383.5f || state->buttons[BUTTON_RIGHT];
    state->buttons[BUTTON_NAV_DOWN]  = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY) > 16383.5f || SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY) > 16383.5f || state->buttons[BUTTON_DOWN];
    state->buttons[BUTTON_NAV_LEFT]  = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX) < -16383.5f || SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX) < -16383.5f || state->buttons[BUTTON_LEFT];

    for (size_t i = 0; i < SDL_GAMEPAD_AXIS_MAX; i++)
    {
        state->axes[SDL_AXIS_MAPPING[i]] = SDL_GameControllerGetAxis(c, (SDL_GameControllerAxis)i) / 32767.0;
    }
}

bool SDLInputManager::getKeyboardKeyState(BrlsKeyboardScancode key)
{
    // todo: 完整映射
    if (key == BRLS_KBD_KEY_ESCAPE)
    {
        return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_ESCAPE];
    }
    if (key == BRLS_KBD_KEY_ENTER)
    {
        return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RETURN];
    }
    return false;
}

void SDLInputManager::keyboardCallback(SDL_Scancode code, uint8_t state, uint8_t repeat, uint16_t mods) {
    if (brlsKeyboardMap.find(code) != brlsKeyboardMap.end()) {
        int action = state == SDL_PRESSED ? BRLS_KBD_ACTION_PRESS : BRLS_KBD_ACTION_RELSAE;
        if (repeat > 0) {
            action = BRLS_KBD_ACTION_REPEAT;
        }
        int index = 0;
        int keyModifiers = 0;
        for (auto& mod: keymods) {
            if (mod & mods) {
                keyModifiers |= brls_keymods[index];
            }
            index++;
        }

        this->keyboardState.mods = keyModifiers;
        auto key = brlsKeyboardMap.at(code);
        if (action == BRLS_KBD_ACTION_RELSAE) {
            this->keyboardState.keys.erase(key);
        } else {
            this->keyboardState.keys[key] = true;
        }
        this->keyboardState.action = action;
        Application::processKeyInput(this->keyboardState);
    }
}

void SDLInputManager::controllerCallback(uint8_t button, uint8_t state) {
    auto k = (ControllerButton)SDL_BUTTONS_MAPPING[button];
    if (state == SDL_RELEASED) {
        this->keyboardState.buttons.erase(k);
    } else {
        this->keyboardState.buttons[k] = true;
    }
    this->keyboardState.action = state;
    Application::processKeyInput(this->keyboardState);
}

const static std::vector<ControllerButton> buttonDiffs = {
    BUTTON_LT,
    BUTTON_RT,
    BUTTON_NAV_UP,
    BUTTON_NAV_RIGHT,
    BUTTON_NAV_DOWN,
    BUTTON_NAV_LEFT,
};

void SDLInputManager::controllerAxisCallback(uint8_t axis, int16_t value) {
    this->keyboardState.axes[(ControllerAxis)axis] = (float)value / 32767.0f;
    auto buttons = this->keyboardState.buttons;
    this->keyboardState.buttons[BUTTON_LT] = this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_TRIGGERLEFT] > 0.1f;
    this->keyboardState.buttons[BUTTON_RT] = this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_TRIGGERRIGHT] > 0.1f;

    this->keyboardState.buttons[BUTTON_NAV_UP]    = this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_LEFTY] < -0.5f || this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_RIGHTY] < -0.5f || this->keyboardState.buttons[BUTTON_UP];
    this->keyboardState.buttons[BUTTON_NAV_RIGHT] = this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_LEFTX] > 0.5f || this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_RIGHTX] > 0.5f || this->keyboardState.buttons[BUTTON_RIGHT];
    this->keyboardState.buttons[BUTTON_NAV_DOWN]  = this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_LEFTY] > 0.5f || this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_RIGHTY] > 0.5f || this->keyboardState.buttons[BUTTON_DOWN];
    this->keyboardState.buttons[BUTTON_NAV_LEFT]  = this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_LEFTX] < -0.5f || this->keyboardState.axes[(ControllerAxis)SDL_CONTROLLER_AXIS_RIGHTX] < -0.5f || this->keyboardState.buttons[BUTTON_LEFT];
    bool isRepeat = true;
    // xbox 的扳机键作为组合键会导致不停的触发
    for (auto& b : buttonDiffs) {
        if (buttons[b] != this->keyboardState.buttons[b]) {
            isRepeat = false;
            break;
        }
    }
    if (!isRepeat) {
        this->keyboardState.action = BRLS_KBD_ACTION_PRESS;
        Application::processKeyInput(this->keyboardState);
    }
}

void SDLInputManager::updateTouchStates(std::vector<RawTouchState>* states)
{
}

void SDLInputManager::updateMouseStates(RawMouseState* state)
{
    int x, y;
    Uint32 buttons = SDL_GetMouseState(&x, &y);

    state->leftButton   = buttons & SDL_BUTTON_LEFT;
    state->middleButton = buttons & SDL_BUTTON_MIDDLE;
    state->rightButton  = buttons & SDL_BUTTON_RIGHT;

#ifdef BOREALIS_USE_D3D11
    // d3d11 scaleFactor 不计算在点击事件里
    state->position.x  = x / Application::windowScale;
    state->position.y  = y / Application::windowScale;
#else
    double scaleFactor = brls::Application::getPlatform()->getVideoContext()->getScaleFactor();
    state->position.x  = x * scaleFactor / Application::windowScale;
    state->position.y  = y * scaleFactor / Application::windowScale;
#endif

    state->offset = pointerOffset;

    state->scroll = scrollOffset;
}

void SDLInputManager::setPointerLock(bool lock)
{
}

void SDLInputManager::runloopStart()
{
    pointerOffset         = pointerOffsetBuffer;
    pointerOffsetBuffer.x = 0;
    pointerOffsetBuffer.y = 0;
}

void SDLInputManager::sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor)
{
}

void SDLInputManager::updateMouseWheel(SDL_MouseWheelEvent event)
{
#if defined(_WIN32) || defined(__linux__)
    this->scrollOffset.x += event.preciseX * 30;
    this->scrollOffset.y += event.preciseY * 30;
#else
    this->scrollOffset.x += event.preciseX * 10;
    this->scrollOffset.y += event.preciseY * 10;
#endif

    this->getMouseScrollOffsetChanged()->fire(Point(event.x, event.y));
}

};
