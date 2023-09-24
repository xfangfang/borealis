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
#define SDL_GAMEPAD_AXIS_MAX 6
#define SDL_STICKY 2

    /// HidKeyboardScancode
/// Uses the same key codes as GLFW
static const BrlsKeyboardScancode sdlToBrlsKeyboardScancode(SDL_Scancode scancode)
{
    if (scancode == SDL_SCANCODE_UNKNOWN) return BRLS_KBD_KEY_UNKNOWN;

    // 1 - 9
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) return (BrlsKeyboardScancode) (BRLS_KBD_KEY_1 + scancode - SDL_SCANCODE_1);

    // KP1 - KP9
    if (scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9) return (BrlsKeyboardScancode) (BRLS_KBD_KEY_KP_1 + scancode - SDL_SCANCODE_KP_1);

    // A - Z
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) return (BrlsKeyboardScancode) (BRLS_KBD_KEY_A + scancode - SDL_SCANCODE_A);

    // F1 - F12
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) return (BrlsKeyboardScancode) (BRLS_KBD_KEY_F1 + scancode - SDL_SCANCODE_F1);

    switch (scancode) {
        case SDL_SCANCODE_UNKNOWN: return BRLS_KBD_KEY_UNKNOWN;

    /* Printable keys */
        case SDL_SCANCODE_SPACE: return BRLS_KBD_KEY_SPACE;
        case SDL_SCANCODE_APOSTROPHE: return BRLS_KBD_KEY_APOSTROPHE;
        case SDL_SCANCODE_COMMA: return BRLS_KBD_KEY_COMMA;
        case SDL_SCANCODE_MINUS: return BRLS_KBD_KEY_MINUS;
        case SDL_SCANCODE_PERIOD: return BRLS_KBD_KEY_PERIOD;
        case SDL_SCANCODE_SLASH: return BRLS_KBD_KEY_SLASH;
        case SDL_SCANCODE_0: return BRLS_KBD_KEY_0;
        case SDL_SCANCODE_SEMICOLON: return BRLS_KBD_KEY_SEMICOLON;
        case SDL_SCANCODE_EQUALS: return BRLS_KBD_KEY_EQUAL;
        case SDL_SCANCODE_LEFTBRACKET: return BRLS_KBD_KEY_LEFT_BRACKET;
        case SDL_SCANCODE_BACKSLASH: return BRLS_KBD_KEY_BACKSLASH;
        case SDL_SCANCODE_RIGHTBRACKET: return BRLS_KBD_KEY_RIGHT_BRACKET;
        case SDL_SCANCODE_GRAVE: return BRLS_KBD_KEY_GRAVE_ACCENT;
        case SDL_SCANCODE_INTERNATIONAL1: return BRLS_KBD_KEY_WORLD_1;

    /* Function keys */
        case SDL_SCANCODE_ESCAPE: return BRLS_KBD_KEY_ESCAPE;
        case SDL_SCANCODE_RETURN: return BRLS_KBD_KEY_ENTER;
        case SDL_SCANCODE_TAB: return BRLS_KBD_KEY_TAB;
        case SDL_SCANCODE_BACKSPACE: return BRLS_KBD_KEY_BACKSPACE;
        case SDL_SCANCODE_INSERT: return BRLS_KBD_KEY_INSERT;
        case SDL_SCANCODE_DELETE: return BRLS_KBD_KEY_DELETE;
        case SDL_SCANCODE_RIGHT: return BRLS_KBD_KEY_RIGHT;
        case SDL_SCANCODE_LEFT: return BRLS_KBD_KEY_LEFT;
        case SDL_SCANCODE_DOWN: return BRLS_KBD_KEY_DOWN;
        case SDL_SCANCODE_UP: return BRLS_KBD_KEY_UP;
        case SDL_SCANCODE_PAGEUP: return BRLS_KBD_KEY_PAGE_UP;
        case SDL_SCANCODE_PAGEDOWN: return BRLS_KBD_KEY_PAGE_DOWN;
        case SDL_SCANCODE_HOME: return BRLS_KBD_KEY_HOME;
        case SDL_SCANCODE_END: return BRLS_KBD_KEY_END;
        case SDL_SCANCODE_CAPSLOCK: return BRLS_KBD_KEY_CAPS_LOCK;
        case SDL_SCANCODE_SCROLLLOCK: return BRLS_KBD_KEY_SCROLL_LOCK;
        case SDL_SCANCODE_NUMLOCKCLEAR: return BRLS_KBD_KEY_NUM_LOCK;
        case SDL_SCANCODE_PRINTSCREEN: return BRLS_KBD_KEY_PRINT_SCREEN;
        case SDL_SCANCODE_PAUSE: return BRLS_KBD_KEY_PAUSE;
        case SDL_SCANCODE_KP_0: return BRLS_KBD_KEY_KP_0;
        case SDL_SCANCODE_KP_DECIMAL: return BRLS_KBD_KEY_KP_DECIMAL;
        case SDL_SCANCODE_KP_DIVIDE: return BRLS_KBD_KEY_KP_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY: return BRLS_KBD_KEY_KP_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS: return BRLS_KBD_KEY_KP_SUBTRACT;
        case SDL_SCANCODE_KP_PLUS: return BRLS_KBD_KEY_KP_ADD;
        case SDL_SCANCODE_KP_ENTER: return BRLS_KBD_KEY_KP_ENTER;
        case SDL_SCANCODE_KP_EQUALS: return BRLS_KBD_KEY_KP_EQUAL;
        case SDL_SCANCODE_LSHIFT: return BRLS_KBD_KEY_LEFT_SHIFT;
        case SDL_SCANCODE_LCTRL: return BRLS_KBD_KEY_LEFT_CONTROL;
        case SDL_SCANCODE_LALT: return BRLS_KBD_KEY_LEFT_ALT;
        case SDL_SCANCODE_LGUI: return BRLS_KBD_KEY_LEFT_SUPER;
        case SDL_SCANCODE_RSHIFT: return BRLS_KBD_KEY_RIGHT_SHIFT;
        case SDL_SCANCODE_RCTRL: return BRLS_KBD_KEY_RIGHT_CONTROL;
        case SDL_SCANCODE_RALT: return BRLS_KBD_KEY_RIGHT_ALT;
        case SDL_SCANCODE_RGUI: return BRLS_KBD_KEY_RIGHT_SUPER;
        case SDL_SCANCODE_MENU: return BRLS_KBD_KEY_MENU;
        default: return BRLS_KBD_KEY_UNKNOWN;
    }
}

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
    SDL_SCANCODE_RCTRL, // SDL_CONTROLLER_BUTTON_B
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

static std::unordered_map<SDL_Scancode, int> keyboardKeys {};

static const size_t SDL_AXIS_MAPPING[SDL_GAMEPAD_AXIS_MAX] = {
    LEFT_X,
    LEFT_Y,
    RIGHT_X,
    RIGHT_Y,
    LEFT_Z,
    RIGHT_Z,
};

std::unordered_map<SDL_JoystickID, SDL_GameController*> controllers;

static int mouseButtons[3] = { 0 };

static inline int getMouseButtonState(int buttonIndex)
{
    if (mouseButtons[buttonIndex - 1] == SDL_STICKY)
    {
        mouseButtons[buttonIndex - 1] = SDL_RELEASED;
        return SDL_PRESSED;
    }
    else
    {
        return mouseButtons[buttonIndex - 1];
    }
}

static inline int getKeyboardKeys(SDL_Scancode code)
{
    if (keyboardKeys.find(code) == keyboardKeys.end())
        return SDL_RELEASED;
    if (keyboardKeys[code] == SDL_STICKY)
    {
        keyboardKeys[code] = SDL_RELEASED;
        return SDL_PRESSED;
    }
    else
    {
        return keyboardKeys[code];
    }
}

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
    else if (event->type == SDL_MOUSEBUTTONDOWN)
    {
        if (event->button.button <= 3)
            mouseButtons[event->button.button - 1] = SDL_PRESSED;
    }
    else if (event->type == SDL_MOUSEBUTTONUP)
    {
        if (event->button.button <= 3)
            mouseButtons[event->button.button - 1] = SDL_STICKY;
    }
    else if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
    {
        switch (event->key.keysym.scancode)
        {
            case SDL_SCANCODE_ESCAPE:
            case SDL_SCANCODE_RCTRL:
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_MENU:
            case SDL_SCANCODE_AC_BACK:
            case SDL_SCANCODE_BACKSPACE:
            case SDL_SCANCODE_X:
            case SDL_SCANCODE_Y:
            case SDL_SCANCODE_F1:
            case SDL_SCANCODE_UNKNOWN:
            case SDL_SCANCODE_F2:
            case SDL_SCANCODE_Q:
            case SDL_SCANCODE_P:
            case SDL_SCANCODE_L:
            case SDL_SCANCODE_R:
            case SDL_SCANCODE_UP:
            case SDL_SCANCODE_DOWN:
            case SDL_SCANCODE_LEFT:
            case SDL_SCANCODE_RIGHT:
            case SDL_SCANCODE_SPACE:
            case SDL_SCANCODE_F:
                keyboardKeys[event->key.keysym.scancode] = event->type == SDL_KEYDOWN ? SDL_PRESSED : SDL_STICKY;
                break;
            default:
                break;
        }
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

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

    int controllersCount = SDL_NumJoysticks();
    brls::Logger::info("joystick num: {}", controllersCount);

    for (int i = 0; i < controllersCount; i++)
    {
        SDL_JoystickID jid = SDL_JoystickGetDeviceInstanceID(i);
        Logger::info("sdl: joystick {}: \"{}\"", jid, SDL_JoystickNameForIndex(i));
        controllers.insert({ jid, SDL_GameControllerOpen(i) });
    }

    SDL_AddEventWatch(sdlEventWatcher, this->window);

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
    for (size_t i = 0; i < SDL_GAMEPAD_BUTTON_MAX; i++)
    {
        size_t brlsButton = SDL_BUTTONS_MAPPING[i];
        size_t key        = SDL_GAMEPAD_TO_KEYBOARD[i];
        if (key != SDL_SCANCODE_UNKNOWN)
            state->buttons[brlsButton] |= getKeyboardKeys((SDL_Scancode)key);
    }
    // Android tv remote control
    state->buttons[BUTTON_X] |= getKeyboardKeys(SDL_SCANCODE_MENU);
    state->buttons[BUTTON_B] |= getKeyboardKeys(SDL_SCANCODE_AC_BACK);

    // pc shortcuts
    state->buttons[BUTTON_SPACE] = getKeyboardKeys(SDL_SCANCODE_SPACE);
    state->buttons[BUTTON_BACKSPACE] = getKeyboardKeys(SDL_SCANCODE_BACKSPACE);
    state->buttons[BUTTON_F] = getKeyboardKeys(SDL_SCANCODE_F);

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
    switch (key)
    {
        case BRLS_KBD_KEY_ESCAPE:
            return getKeyboardKeys(SDL_SCANCODE_ESCAPE);
        case BRLS_KBD_KEY_ENTER:
            return getKeyboardKeys(SDL_SCANCODE_RETURN);
        default:
            return false;
    }
    return false;
}

void SDLInputManager::updateTouchStates(std::vector<RawTouchState>* states)
{
    int devices = SDL_GetNumTouchDevices();
    if (devices == 0) return;

    for (int deviceID = 0; deviceID < devices; deviceID++) {
        SDL_TouchID device = SDL_GetTouchDevice(deviceID);
        int touchesCount = SDL_GetNumTouchFingers(device);
        for (int touchID = 0; touchID < touchesCount; touchID++) {
            SDL_Finger* finger = SDL_GetTouchFinger(device, touchID);
            
            RawTouchState state;
            state.pressed    = true;
            state.fingerId   = (int) finger->id;
            state.position.x = Application::contentWidth * finger->x;
            state.position.y = Application::contentHeight * finger->y;
            states->push_back(state);
        }
    }
}

void SDLInputManager::updateMouseStates(RawMouseState* state)
{
    int x, y;
    SDL_GetMouseState(&x, &y);

    state->leftButton   = getMouseButtonState(SDL_BUTTON_LEFT);
    state->middleButton = getMouseButtonState(SDL_BUTTON_MIDDLE);
    state->rightButton  = getMouseButtonState(SDL_BUTTON_RIGHT);

#ifdef BOREALIS_USE_D3D11
    // d3d11 scaleFactor 不计算在点击事件里
    state->position.x = x / Application::windowScale;
    state->position.y = y / Application::windowScale;
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
    if (controllers.find(controller) == controllers.end())
        return;
    SDL_GameController* c = controllers[controller];

    SDL_GameControllerRumble(c, lowFreqMotor, highFreqMotor, 30000);
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

void SDLInputManager::updateControllerSensorsUpdate(SDL_ControllerSensorEvent event)
{
    auto id = event.which;
    SensorEvent state;

    switch (event.sensor) {
        case SDL_SENSOR_ACCEL:
            state = SensorEvent { id, SensorEventType::ACCEL, {event.data[0], event.data[1], event.data[2]}, event.timestamp };
            getControllerSensorStateChanged()->fire(state);
            Application::setActiveEvent(true);
            break;
        case SDL_SENSOR_GYRO:
            state = SensorEvent { id, SensorEventType::GYRO, {event.data[0], event.data[1], event.data[2]}, event.timestamp };
            getControllerSensorStateChanged()->fire(state);
            Application::setActiveEvent(true);
            break;
    }
}

void SDLInputManager::updateKeyboardState(SDL_KeyboardEvent event)
{
    auto* self = (SDLInputManager*)Application::getPlatform()->getInputManager();
    KeyState state {};
    state.key            = sdlToBrlsKeyboardScancode(event.keysym.scancode);
    state.mods           = event.keysym.mod;
    state.pressed        = event.type == SDL_KEYDOWN;

    self->getKeyboardKeyStateChanged()->fire(state);
    Application::setActiveEvent(true);
}

};
