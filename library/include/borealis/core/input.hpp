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

#pragma once

#include <borealis/core/geometry.hpp>
#include <borealis/core/event.hpp>
#include <vector>

namespace brls
{

// Abstract buttons enum - names correspond to a generic Xbox controller
// LT and RT should not be buttons but for the sake of simplicity we'll assume they are.
// Similarly, DPAD (also called HAT) is assumed to be buttons here.
enum ControllerButton
{
    BUTTON_LT = 0,
    BUTTON_LB,

    BUTTON_LSB,

    BUTTON_UP,
    BUTTON_RIGHT,
    BUTTON_DOWN,
    BUTTON_LEFT,

    BUTTON_BACK,
    BUTTON_GUIDE,
    BUTTON_START,

    BUTTON_RSB,

    BUTTON_Y,
    BUTTON_B,
    BUTTON_A,
    BUTTON_X,

    BUTTON_RB,
    BUTTON_RT,

    BUTTON_NAV_UP,
    BUTTON_NAV_RIGHT,
    BUTTON_NAV_DOWN,
    BUTTON_NAV_LEFT,

    _BUTTON_MAX,
};

// Abstract axis enum - names correspond to a generic Xbox controller
enum ControllerAxis
{
    LEFT_X,
    LEFT_Y,

    // No Z axis, LT and RT are in the buttons enum for the sake of simplicity

    RIGHT_X, // also called 5th axis
    RIGHT_Y, // also called 4th axis

    _AXES_MAX,
};

struct KeyState
{
    short key;
    short mods;
    bool pressed;
};

// Represents the state of the controller (a gamepad or a keyboard) in the current frame
struct ControllerState
{
    bool buttons[_BUTTON_MAX]; // true: pressed
    float axes[_AXES_MAX]; // from 0.0f to 1.0f
};

// Represents a touch phase in the current frame
enum class TouchPhase
{
    START,
    STAY,
    END,
    NONE,
};

// Contains raw touch data, filled in by platform driver
struct RawTouchState
{
    int fingerId = 0;
    bool pressed = false;
    Point position;
};

// Contains touch data automatically filled with current phase by the library
class View;
struct TouchState
{
    int fingerId     = 0;
    TouchPhase phase = TouchPhase::NONE;
    Point position;
    View* view = nullptr;
};

// Contains raw touch data, filled in by platform driver
struct RawMouseState
{
    Point position;
    Point offset;
    Point scroll;
    bool leftButton   = false;
    bool middleButton = false;
    bool rightButton  = false;
};

struct MouseState
{
    Point position;
    Point scroll;
    TouchPhase leftButton;
    TouchPhase middleButton;
    TouchPhase rightButton;
    View* view = nullptr;
};

// Interface responsible for reporting input state to the application - button presses,
// axis position and touch screen state
class InputManager
{
  public:
    virtual ~InputManager() { }

    /**
     * Called once every frame to fill the given ControllerState struct with the controller state.
     */
    virtual void updateControllerState(ControllerState* state) = 0;

    /**
     * Called once every frame to fill the given RawTouchState struct with the raw touch data.
     */
    virtual void updateTouchStates(std::vector<RawTouchState>* states) = 0;

    /**
     * Called once every frame to fill the given RawTouchState struct with the raw touch data.
     */
    virtual void updateMouseStates(RawMouseState* state) = 0;

    /**
     * Calls to update gamepad's rumble state.
     */
    virtual void sendRumble(unsigned short controller, unsigned short lowFreqMotor, unsigned short highFreqMotor) = 0;

    /**
     * Called once every runloop cycle to perform some cleanup before new one.
     * For internal call only
     */
    virtual void runloopStart() {};
    
    virtual void setPointerLock(bool lock) {};
    
    inline Event<Point> *getMouseCusorOffsetChanged() {
        return &mouseCusorOffsetChanged;
    }
    
    inline Event<Point> *getMouseScrollOffsetChanged() {
        return &mouseScrollOffsetChanged;
    }
    
    inline Event<KeyState> *getKeyboardKeyStateChanged() {
        return &keyboardKeyStateChanged;
    }

    /**
     * Calculate current touch phase based on it's previous state
     */
    static TouchState computeTouchState(RawTouchState currentTouch, TouchState lastFrameState);

    /**
     * Calculate current touch phase based on it's previous state
     */
    static MouseState computeMouseState(RawMouseState currentTouch, MouseState lastFrameState);

    static ControllerButton mapControllerState(ControllerButton button);
    
private:
    Event<Point> mouseCusorOffsetChanged;
    Event<Point> mouseScrollOffsetChanged;
    Event<KeyState> keyboardKeyStateChanged;
};

}; // namespace brls
