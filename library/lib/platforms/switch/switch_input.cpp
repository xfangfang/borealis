/*
    Copyright 2021 natinusala

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
#include <borealis/platforms/switch/switch_input.hpp>

namespace brls
{

static const uint64_t HidNpadButton_None = ((uint64_t)1 << (63));

static const uint64_t SWITCH_BUTTONS_MAPPING[_BUTTON_MAX] = {
    HidNpadButton_ZL, // BUTTON_LT
    HidNpadButton_L, // BUTTON_LB

    HidNpadButton_StickL, // BUTTON_LSB

    HidNpadButton_Up, // BUTTON_UP
    HidNpadButton_Right, // BUTTON_RIGHT
    HidNpadButton_Down, // BUTTON_DOWN
    HidNpadButton_Left, // BUTTON_LEFT

    HidNpadButton_Minus, // BUTTON_BACK
    HidNpadButton_None, // BUTTON_GUIDE
    HidNpadButton_Plus, // BUTTON_START

    HidNpadButton_StickR, // BUTTON_RSB

    HidNpadButton_Y, // BUTTON_Y
    HidNpadButton_B, // BUTTON_B
    HidNpadButton_A, // BUTTON_A
    HidNpadButton_X, // BUTTON_X

    HidNpadButton_R, // BUTTON_RB
    HidNpadButton_ZR, // BUTTON_RT

    HidNpadButton_AnyUp, // BUTTON_NAV_UP
    HidNpadButton_AnyRight, // BUTTON_NAV_RIGHT
    HidNpadButton_AnyDown, // BUTTON_NAV_DOWN
    HidNpadButton_AnyLeft, // BUTTON_NAV_LEFT
};

static const size_t SWITCH_AXIS_MAPPING[_AXES_MAX] = {
    LEFT_X,
    LEFT_Y,
    RIGHT_X,
    RIGHT_Y,
};

SwitchInputManager::SwitchInputManager()
{
    padConfigureInput(2, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&this->padState);

    hidInitializeMouse();
    hidInitializeKeyboard();

    hidInitializeVibrationDevices(m_vibration_device_handles[0], 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidInitializeVibrationDevices(m_vibration_device_handles[1], 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);

    m_hid_keyboard_state.assign(256, false);
}

SwitchInputManager::~SwitchInputManager() 
{
    NVGcontext* vg = Application::getNVGContext();

    if (this->cursorTexture != 0)
        nvgDeleteImage(vg, this->cursorTexture);
}

void SwitchInputManager::updateControllerState(ControllerState* state)
{
    padUpdate(&this->padState);
    uint64_t keysDown = padGetButtons(&this->padState);

    for (size_t i = 0; i < _BUTTON_MAX; i++)
    {
        uint64_t switchKey = SWITCH_BUTTONS_MAPPING[i];
        state->buttons[i]  = keysDown & switchKey;
    }

    HidAnalogStickState analog_stick_l = padGetStickPos(&this->padState, 0);
    HidAnalogStickState analog_stick_r = padGetStickPos(&this->padState, 1);

    state->axes[LEFT_X]  = (float)analog_stick_l.x / (float)0x7FFF;
    state->axes[LEFT_Y]  = (float)analog_stick_l.y / (float)0x7FFF * -1.0f;
    state->axes[RIGHT_X] = (float)analog_stick_r.x / (float)0x7FFF;
    state->axes[RIGHT_Y] = (float)analog_stick_r.y / (float)0x7FFF * -1.0f;
}

void SwitchInputManager::updateTouchStates(std::vector<RawTouchState>* states)
{
    // Get touchscreen state
    static HidTouchScreenState hidState;

    if (hidGetTouchScreenStates(&hidState, 1))
    {
        for (int i = 0; i < hidState.count; i++)
        {
            RawTouchState state;
            state.pressed    = true;
            state.fingerId   = hidState.touches[i].finger_id;
            state.position.x = hidState.touches[i].x / Application::windowScale;
            state.position.y = hidState.touches[i].y / Application::windowScale;
            states->push_back(state);
        }
    }
}

void SwitchInputManager::sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor)
{
    if (controller == 0)
    {
        float low  = (float)lowFreqMotor / 0xFFFF;
        float high = (float)highFreqMotor / 0xFFFF;

        memset(m_vibration_values, 0, sizeof(m_vibration_values));

        m_vibration_values[0].amp_low   = low;
        m_vibration_values[0].freq_low  = low * 50;
        m_vibration_values[0].amp_high  = high;
        m_vibration_values[0].freq_high = high * 100;

        m_vibration_values[1].amp_low   = low;
        m_vibration_values[1].freq_low  = low * 50;
        m_vibration_values[1].amp_high  = high;
        m_vibration_values[1].freq_high = high * 100;

        int target_device = padIsHandheld(&this->padState) ? 0 : 1;
        hidSendVibrationValues(m_vibration_device_handles[target_device], m_vibration_values, 2);
    }
}

void SwitchInputManager::updateMouseStates(RawMouseState* state)
{
    if (currentMouseState.attributes & HidMouseAttribute_IsConnected) {
        state->position = Point(currentMouseState.x, currentMouseState.y);
        state->offset = Point(currentMouseState.delta_x, currentMouseState.delta_y);
        state->scroll = Point(0, currentMouseState.wheel_delta_x);
        state->leftButton = currentMouseState.buttons & HidMouseButton_Left;
        state->middleButton = currentMouseState.buttons & HidMouseButton_Middle;
        state->rightButton = currentMouseState.buttons & HidMouseButton_Right;
        lastCoursorPosition = state->position;
    }
}

void SwitchInputManager::runloopStart()
{
    upToDateMouseState();
    handleMouse();
    handleKeyboard();
}

void SwitchInputManager::upToDateMouseState() 
{
    hidGetMouseStates(&currentMouseState, 1);
}

void SwitchInputManager::handleMouse()
{
    if (currentMouseState.attributes & HidMouseAttribute_IsConnected) {
        getMouseCusorOffsetChanged()->fire(Point(currentMouseState.delta_x, currentMouseState.delta_y));
        getMouseScrollOffsetChanged()->fire(Point(currentMouseState.wheel_delta_y, currentMouseState.wheel_delta_x));
    }
}

void SwitchInputManager::handleKeyboard()
{
    HidKeyboardState state;
    
    if (hidGetKeyboardStates(&state, 1)) {
        for (int i = 0; i < 256; ++i) {
            auto is_pressed = (state.keys[i / 64] & (1ul << (i % 64))) != 0;
            if (m_hid_keyboard_state[i] != is_pressed) 
            {
                m_hid_keyboard_state[i] = is_pressed;
                int glfwKey = switchKeyToGlfwKey(i);

                KeyState keyState;
                keyState.key = glfwKey;
                keyState.pressed = is_pressed;
                
                if (state.modifiers & HidKeyboardModifier_LeftAlt)
                    keyState.mods |= BRLS_KBD_MODIFIER_ALT;
                
                if (state.modifiers & HidKeyboardModifier_Control)
                    keyState.mods |= BRLS_KBD_MODIFIER_CTRL;
                
                if (state.modifiers & HidKeyboardModifier_Shift)
                    keyState.mods |= BRLS_KBD_MODIFIER_SHIFT;
                
                if (state.modifiers & HidKeyboardModifier_Gui)
                    keyState.mods |= BRLS_KBD_MODIFIER_META;
                
                getKeyboardKeyStateChanged()->fire(keyState);
            }
        }
    } else {
        Logger::debug("Keyboard failed!");
    }
}

void SwitchInputManager::setPointerLock(bool lock)
{
    pointerLocked = lock;
}

void SwitchInputManager::drawCoursor(NVGcontext* vg)
{
    initCursor(vg);
    if (!pointerLocked) {
        this->paint.xform[4] = lastCoursorPosition.x;
        this->paint.xform[5] = lastCoursorPosition.y;

        nvgBeginPath(vg);
        nvgRect(vg, lastCoursorPosition.x, lastCoursorPosition.y, this->cursorWidth, this->cursorHeight);
        nvgFillPaint(vg, this->paint);
        nvgFill(vg);
    }
}

void SwitchInputManager::initCursor(NVGcontext* vg) 
{
    if (cursorInited) return; 
    if (vg) {
        this->pointerIcon = std::string(BRLS_RESOURCES) + "img/sys/cursor.png";
        this->cursorTexture = nvgCreateImage(vg, pointerIcon.c_str(), NVG_IMAGE_NEAREST);

        int width, height;
        nvgImageSize(vg, cursorTexture, &width, &height);
        float aspect = (float)height / (float)width;
        this->cursorWidth  = 18;
        this->cursorHeight = 18 * aspect;

        this->paint   = nvgImagePattern(vg, 0, 0, this->cursorWidth, this->cursorHeight, 0, this->cursorTexture, 1.0f);
        this->cursorInited = true;
    }
}

int SwitchInputManager::switchKeyToGlfwKey(int key)
{
    if (KBD_A <= key && key <= KBD_Z) {
        return key - KBD_A + BRLS_KBD_KEY_A;
    } else if (KBD_1 <= key && key <= KBD_9) {
        return key - KBD_1 + BRLS_KBD_KEY_1;
    } else if (KBD_F1 <= key && key <= KBD_F12) {
        return key - KBD_F1 + BRLS_KBD_KEY_F1;
    } else if (KBD_KP1 <= key && key <= KBD_KP9) {
        return key - KBD_KP1 + BRLS_KBD_KEY_KP_1;
    }
    
    switch (key) {
        case KBD_0: return BRLS_KBD_KEY_0;
        case KBD_SPACE: return BRLS_KBD_KEY_SPACE;
        case KBD_APOSTROPHE: return BRLS_KBD_KEY_APOSTROPHE;
        case KBD_COMMA: return BRLS_KBD_KEY_COMMA;
        case KBD_MINUS: return BRLS_KBD_KEY_MINUS;
        case KBD_DOT: return BRLS_KBD_KEY_PERIOD;
        case KBD_SLASH: return BRLS_KBD_KEY_SLASH;
        case KBD_SEMICOLON: return BRLS_KBD_KEY_SEMICOLON;
        case KBD_EQUAL: return BRLS_KBD_KEY_EQUAL;
        case KBD_LEFTBRACE: return BRLS_KBD_KEY_LEFT_BRACKET;
        case KBD_RIGHTBRACE: return BRLS_KBD_KEY_RIGHT_BRACKET;
        case KBD_BACKSLASH: return BRLS_KBD_KEY_BACKSLASH;
        case KBD_GRAVE: return BRLS_KBD_KEY_GRAVE_ACCENT;
        case KBD_ESC: return BRLS_KBD_KEY_ESCAPE;
        case KBD_ENTER: return BRLS_KBD_KEY_ENTER;
        case KBD_TAB: return BRLS_KBD_KEY_TAB;
        case KBD_BACKSPACE: return BRLS_KBD_KEY_BACKSPACE;
        case KBD_CAPSLOCK: return BRLS_KBD_KEY_CAPS_LOCK;
        case KBD_LEFTSHIFT: return BRLS_KBD_KEY_LEFT_SHIFT;
        case KBD_LEFTCTRL: return BRLS_KBD_KEY_LEFT_CONTROL;
        case KBD_LEFTALT: return BRLS_KBD_KEY_LEFT_ALT;
        case KBD_LEFTMETA: return BRLS_KBD_KEY_LEFT_SUPER;
        case KBD_RIGHTSHIFT: return BRLS_KBD_KEY_RIGHT_SHIFT;
        case KBD_RIGHTCTRL: return BRLS_KBD_KEY_RIGHT_CONTROL;
        case KBD_RIGHTALT: return BRLS_KBD_KEY_RIGHT_ALT;
        case KBD_RIGHTMETA: return BRLS_KBD_KEY_RIGHT_SUPER;
        case KBD_LEFT: return BRLS_KBD_KEY_LEFT;
        case KBD_RIGHT: return BRLS_KBD_KEY_RIGHT;
        case KBD_UP: return BRLS_KBD_KEY_UP;
        case KBD_DOWN: return BRLS_KBD_KEY_DOWN;
        
        case KBD_SYSRQ: return BRLS_KBD_KEY_PRINT_SCREEN;
        case KBD_SCROLLLOCK: return BRLS_KBD_KEY_SCROLL_LOCK;
        case KBD_PAUSE: return BRLS_KBD_KEY_PAUSE;
        case KBD_INSERT: return BRLS_KBD_KEY_INSERT;
        case KBD_HOME: return BRLS_KBD_KEY_HOME;
        case KBD_PAGEUP: return BRLS_KBD_KEY_PAGE_UP;
        case KBD_DELETE: return BRLS_KBD_KEY_DELETE;
        case KBD_END: return BRLS_KBD_KEY_END;
        case KBD_PAGEDOWN: return BRLS_KBD_KEY_PAGE_DOWN;

        case KBD_NUMLOCK: return BRLS_KBD_KEY_NUM_LOCK;
        case KBD_KPSLASH: return BRLS_KBD_KEY_KP_DIVIDE;
        case KBD_KPASTERISK: return BRLS_KBD_KEY_KP_MULTIPLY;
        case KBD_KPMINUS: return BRLS_KBD_KEY_KP_SUBTRACT;
        case KBD_KPPLUS: return BRLS_KBD_KEY_KP_ADD;
        case KBD_KPENTER: return BRLS_KBD_KEY_KP_ENTER;
        case KBD_KPDOT: return BRLS_KBD_KEY_KP_DECIMAL;
        case KBD_KP0: return BRLS_KBD_KEY_KP_0;

        // case KBD_HASHTILDE: return GLFW_HASHTILDE;
        default: return -1;
    }
}

} // namespace brls
