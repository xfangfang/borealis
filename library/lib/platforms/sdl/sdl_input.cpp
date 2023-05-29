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

    controllerNum = SDL_NumJoysticks();
    brls::Logger::info("joystick num: {}", controllerNum);

    for (int i = 0; i < controllerNum; i++)
    {
        Logger::info("sdl: joystick {} is gamepad: \"{}\"", i, SDL_JoystickNameForIndex(i));
        controllers.emplace_back(SDL_GameControllerOpen(i));
    }

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
        SDL_GameControllerClose(i);
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

    for (int j = 0; j < controllerNum; j++)
    {
        ControllerState localState {};
        updateControllerState(&localState, j);

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
    if (controller >= controllers.size() || controller < 0)
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

void SDLInputManager::keyboardCallback(SDL_Window* window, BrlsKeyboardScancode key, int action, int mods) {
    this->keyboardState.mods = mods;
    if (action == BRLS_KBD_ACTION_RELSAE) {
        this->keyboardState.keys.erase(key);
    } else {
        this->keyboardState.keys[key] = true;
    }
    this->keyboardState.action = action;
    Application::processKeyInput(this->keyboardState);
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
    // Uncomment to enable touch simulation by mouse
    //    double x, y;
    //    glfwGetCursorPos(this->window, &x, &y);
    //
    //    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    //    {
    //        RawTouchState state;
    //        state.fingerId = 0;
    //        state.pressed = true;
    //        state.position.x = x / Application::windowScale;
    //        state.position.y = y / Application::windowScale;
    //        states->push_back(state);
    //    }
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
    //    pointerLocked = lock;
    //
    //    int state = lock ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    //    glfwSetInputMode(window, GLFW_CURSOR, state);
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
