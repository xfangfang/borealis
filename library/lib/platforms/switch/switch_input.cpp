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
};

static const size_t SWITCH_AXIS_MAPPING[_AXES_MAX] = {
    LEFT_X,
    LEFT_Y,
    RIGHT_X,
    RIGHT_Y,
};

SwitchInputManager::SwitchInputManager()
{
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&this->padState);
    
    hidInitializeVibrationDevices(m_vibration_device_handles[0], 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidInitializeVibrationDevices(m_vibration_device_handles[1], 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);
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

void SwitchInputManager::updateTouchStates(std::array<RawTouchState, TOUCHES_MAX>* states)
{
    // Get touchscreen state
    static HidTouchScreenState hidState = { 0 };

    for (int i = 0; i < TOUCHES_MAX; i++)
        (*states)[i].pressed = false;

    if (hidGetTouchScreenStates(&hidState, 1))
    {
        for (int i = 0; i < hidState.count && i < TOUCHES_MAX; i++)
        {
            (*states)[i].pressed    = true;
            (*states)[i].fingerId   = hidState.touches[i].finger_id;
            (*states)[i].position.x = hidState.touches[i].x / Application::windowScale;
            (*states)[i].position.y = hidState.touches[i].y / Application::windowScale;
        }
    }
}

void SwitchInputManager::sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor)
{
    if (controller == 0) {
        float low = (float)low_freq_motor / 0xFFFF;
        float high = (float)high_freq_motor / 0xFFFF;
        
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
    state->position = Point();
}

} // namespace brls
