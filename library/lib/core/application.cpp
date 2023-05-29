/*
    Copyright 2019-2020 natinusala
    Copyright 2019 p-sam
    Copyright 2020 WerWolv
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

#include <stdio.h>
#include <stdlib.h>
#include <yoga/event/event.h>

#include <algorithm>
#include <borealis/core/application.hpp>
#include <borealis/core/font.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/thread.hpp>
#include <borealis/core/time.hpp>
#include <borealis/core/util.hpp>
#include <borealis/views/bottom_bar.hpp>
#include <borealis/views/button.hpp>
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_input.hpp>
#include <borealis/views/cells/cell_radio.hpp>
#include <borealis/views/cells/cell_selector.hpp>
#include <borealis/views/h_scrolling_frame.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/hint.hpp>
#include <borealis/views/image.hpp>
#include <borealis/views/progress_spinner.hpp>
#include <borealis/views/rectangle.hpp>
#include <borealis/views/recycler.hpp>
#include <borealis/views/sidebar.hpp>
#include <borealis/views/slider.hpp>
#include <borealis/views/tab_frame.hpp>
#include <borealis/views/widgets/account.hpp>
#include <borealis/views/widgets/battery.hpp>
#include <borealis/views/widgets/wireless.hpp>
#include <stdexcept>
#include <string>

#ifndef YG_ENABLE_EVENTS
#error Please enable Yoga events with the YG_ENABLE_EVENTS define
#endif

#include <chrono>
#include <set>
#include <thread>

#define BUTTON_REPEAT_DELAY 15
#define BUTTON_REPEAT_CADENCY 5

namespace brls
{

bool Application::init()
{
    // Init platform
    Application::platform = Platform::createPlatform();

    if (!Application::platform)
    {
        fatal("Did not find a valid platform");
        return false;
    }

    Logger::info("Using platform {}", platform->getName());

    // Init i18n
    loadTranslations();

    Threading::start();

    Application::inited = true;

    return true;
}

void Application::createWindow(std::string windowTitle)
{
    if (!Application::inited)
    {
        fatal("Please call brls::Application::init() before calling brls::Application::createWindow().");
        return;
    }

    if (VideoContext::sizeW == 0 || VideoContext::sizeH == 0)
    {
        // Create a window with a default size and position
        Application::getPlatform()->createWindow(windowTitle, ORIGINAL_WINDOW_WIDTH, ORIGINAL_WINDOW_HEIGHT);
    }
    else
    {
        // Creates a window based on the window position and size data from the last non-full-screen mode
        Application::getPlatform()->createWindow(windowTitle, VideoContext::sizeW, VideoContext::sizeH,
            VideoContext::posX, VideoContext::posY);
    }

    // Load most commonly used sounds
    AudioPlayer* audioPlayer = Application::getAudioPlayer();
    for (enum Sound sound : {
             SOUND_FOCUS_CHANGE,
             SOUND_FOCUS_ERROR,
             SOUND_CLICK,
         })
        audioPlayer->load(sound);

    // Init rng
    std::srand(std::time(nullptr));

    // Init static variables
    Application::currentFocus = nullptr;
    Application::title        = windowTitle;

    // Init yoga
    YGConfig* defaultConfig       = YGConfigGetDefault();
    defaultConfig->useWebDefaults = true;
    using namespace facebook;

    facebook::yoga::Event::subscribe([](const YGNode& node, facebook::yoga::Event::Type eventType, facebook::yoga::Event::Data eventData)
        {
        View* view = (View*)node.getContext();

        if (!view)
            return;

        if (eventType == facebook::yoga::Event::NodeLayout)
            view->onLayout(); });

    // Load fonts and setup fallbacks
    Application::platform->getFontLoader()->loadFonts();

    // Register built-in XML views
    Application::registerBuiltInXMLViews();

    Application::getWindowCreationDoneEvent()->fire();
}

bool Application::mainLoop()
{
    static Time frameStartTime = 0;
    if (Application::limitedFrameTime > 0)
    {
        frameStartTime = getCPUTimeUsec();
    }

    // Main loop callback
    if (!Application::platform->mainLoopIteration() || Application::quitRequested)
    {
        Application::getWindowShouldCloseEvent()->fire();
        Application::exit();
        return false;
    }

    // Mouse and touch
    if (Application::blockInputsTokens == 0)
    {
        Application::processInput();
    }
    else
    {
        Logger::debug("input blocked (tokens={})", Application::blockInputsTokens);
        if (!muteSounds)
            Application::getAudioPlayer()->play(Sound::SOUND_CLICK_ERROR);
    }

    // Animations
    updateHighlightAnimation();
    Ticking::updateTickings();

    // Render
    Application::frame();

    // Run sync functions
    Threading::performSyncTasks();

    // Trigger RunLoop subscribers
    runLoopEvent.fire();

    // Free views deletion pool
    std::set<View*> undeletedViews;
    for (auto view : Application::deletionPool)
    {
        if (!view->isPtrLocked())
        {
            delete view;
        }
        else
        {
            undeletedViews.insert(view);
            brls::Logger::verbose("Application: will delete view: {}", view->describe());
        }
    }
    Application::deletionPool = undeletedViews;

    // Calculate FPS
    if (Application::globalFPSToggleEnabled)
        Application::updateFPS();

    if (Application::limitedFrameTime > 0)
    {
        Time deltaTime = getCPUTimeUsec() - frameStartTime;
        Time interval  = Application::limitedFrameTime - deltaTime;
        if (interval > 0)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(interval));
        }
    }

    return true;
}

void Application::updateFPS()
{
    static unsigned int start = getCPUTimeUsec();
    static unsigned int index = 0;

    if (index++ == FPS_INTERNAL)
    {
        unsigned int end       = getCPUTimeUsec();
        Application::globalFPS = FPS_INTERNAL_TIME / (end - start);
        start                  = end;
        index                  = 0;
    }
}

void Application::processInput()
{
    static ControllerState oldControllerState = {};

    // Input
    ControllerState controllerState = {};
    std::vector<RawTouchState> rawTouch;
    RawMouseState rawMouse;

    InputManager* inputManager = Application::platform->getInputManager();
    inputManager->runloopStart();
    inputManager->updateTouchStates(&rawTouch);
    inputManager->updateMouseStates(&rawMouse);
    inputManager->updateUnifiedControllerState(&controllerState);

    if (isSwapInputKeys())
    {
        bool swapKeys[ControllerButton::_BUTTON_MAX];
        for (int i = 0; i < ControllerButton::_BUTTON_MAX; i++)
            swapKeys[i] = controllerState.buttons[InputManager::mapControllerState((ControllerButton)i)];

        for (int i = 0; i < ControllerButton::_BUTTON_MAX; i++)
            controllerState.buttons[i] = swapKeys[i];
    }

    std::vector<TouchState> touchState;
    for (auto& i : rawTouch)
    {
        auto old = std::find_if(std::begin(currentTouchState), std::end(currentTouchState), [rawTouch, &i](TouchState touch)
            { return touch.fingerId == i.fingerId; });

        if (old != std::end(currentTouchState))
        {
            touchState.push_back(InputManager::computeTouchState(i, *old));
        }
        else
        {
            TouchState state;
            state.fingerId = i.fingerId;
            touchState.push_back(InputManager::computeTouchState(i, state));
        }
    }

    for (auto& i : currentTouchState)
    {
        if (i.phase == TouchPhase::NONE)
            continue;

        auto old = std::find_if(std::begin(rawTouch), std::end(rawTouch), [&i](RawTouchState touch)
            { return touch.fingerId == i.fingerId; });

        if (old == std::end(rawTouch))
        {
            touchState.push_back(InputManager::computeTouchState(RawTouchState(), i));
        }
    }

    for (auto& i : touchState)
    {
        if (i.phase == TouchPhase::NONE)
        {
            i.view = nullptr;
            break;
        }
        else if (!i.view || i.phase == TouchPhase::START)
        {
            Point position = i.position;
            Application::setInputType(InputType::TOUCH);
            Application::setDrawCoursor(false);

            // Search for first responder, which will be the root of recognition tree
            if (!Application::activitiesStack.empty())
                i.view = Application::activitiesStack[Application::activitiesStack.size() - 1]
                             ->getContentView()
                             ->hitTest(position);
        }

        if (i.view && i.phase != TouchPhase::NONE)
        {
            Sound sound = i.view->gestureRecognizerRequest(i, MouseState(), i.view);
            float pitch = 1;
            if (sound == SOUND_TOUCH)
            {
                // Play touch sound with random pitch
                pitch = (rand() % 10) / 10.0f + 1.0f;
            }
            Application::getAudioPlayer()->play(sound, pitch);
        }
    }
    currentTouchState = touchState;

    MouseState mouseState = InputManager::computeMouseState(rawMouse, currentMouseState);

    if (mouseState.offset.x != 0 || mouseState.offset.y != 0 || mouseState.scroll.x != 0 || mouseState.scroll.y != 0 || mouseState.leftButton != TouchPhase::NONE || mouseState.middleButton != TouchPhase::NONE || mouseState.rightButton != TouchPhase::NONE)
    {
        Application::setInputType(InputType::TOUCH);
        Application::setDrawCoursor(true);
    }

    if (mouseState.scroll.x == 0 && mouseState.scroll.y == 0 && mouseState.leftButton == TouchPhase::NONE && mouseState.middleButton == TouchPhase::NONE && mouseState.rightButton == TouchPhase::NONE)
        mouseState.view = nullptr;
    else if (mouseState.view == nullptr)
    {
        Point position = mouseState.position;

        // Search for first responder, which will be the root of recognition tree
        if (!Application::activitiesStack.empty())
            mouseState.view = Application::activitiesStack[Application::activitiesStack.size() - 1]
                                  ->getContentView()
                                  ->hitTest(position);
    }
    currentMouseState = mouseState;

    if (mouseState.view)
    {
        Sound sound = mouseState.view->gestureRecognizerRequest(TouchState(), mouseState, mouseState.view);
        float pitch = 1;
        if (sound == SOUND_TOUCH)
        {
            // Play touch sound with random pitch
            pitch = (rand() % 10) / 10.0f + 1.0f;
        }
        Application::getAudioPlayer()->play(sound, pitch);
    }

    // Trigger controller events
    bool anyButtonPressed           = false;
    bool repeating                  = false;
    static Time buttonPressTime     = 0;
    static int repeatingButtonTimer = 0;

    controllerState.buttons[BUTTON_A] |= inputManager->getKeyboardKeyState(BRLS_KBD_KEY_ENTER);
    controllerState.buttons[BUTTON_B] |= inputManager->getKeyboardKeyState(BRLS_KBD_KEY_ESCAPE);

    bool hasControllerState = false;

    for (int i = 0; i < _BUTTON_MAX; i++)
    {
        if (controllerState.buttons[i])
        {
            anyButtonPressed = true;
            repeating        = (repeatingButtonTimer > BUTTON_REPEAT_DELAY && repeatingButtonTimer % BUTTON_REPEAT_CADENCY == 0);
            hasControllerState = true;

            if (!oldControllerState.buttons[i] || repeating)
                Application::onControllerButtonPressed((enum ControllerButton)i, repeating);
        }

        if (controllerState.buttons[i] != oldControllerState.buttons[i])
            buttonPressTime = repeatingButtonTimer = 0;
    }
    Time now = getCPUTimeUsec();
    if (anyButtonPressed && now - buttonPressTime > 1000)
    {
        buttonPressTime = now;
        repeatingButtonTimer++; // Increased once every ~1ms
    }

    oldControllerState = controllerState;
}

void Application::processKeyInput(KeyboardControllerState controllerState) {
    static Time keyPressTime     = 0;
    static int repeatingKeyTimer = 0;
    bool anyKeyPressed = false;
    bool hasControllerState = controllerState.buttons.size() > 0;
    bool isRepeat = controllerState.action == BRLS_KBD_ACTION_REPEAT;
    if (hasControllerState || controllerState.mods > 0 || controllerState.keys.size() > 0) {
        // 处理键盘事件
        View* hintParent = Application::currentFocus;
        if (!hintParent)
            hintParent = Application::activitiesStack[Application::activitiesStack.size() - 1]->getContentView();
        anyKeyPressed = true;
        bool anyAction = false;
        bool repeating = (repeatingKeyTimer > BUTTON_REPEAT_DELAY && repeatingKeyTimer % BUTTON_REPEAT_CADENCY == 0);
        while (hintParent)
        {
            for (auto& action : hintParent->getActions())
            {
                if (!action.availableMulti) {
                    continue;
                }
                bool controlleAvailable = true;
                bool keyAvailable = false;
                for (auto& button : action.buttons) {
                    if (!controllerState.buttons[button]) {
                        controlleAvailable = false;
                        break;
                    }
                }
                if (!controlleAvailable) {
                    keyAvailable = true;
                    if ((controllerState.mods & action.mods) != action.mods) {
                        keyAvailable = false;
                    }
                    if (keyAvailable) {
                        for (auto& key : action.keys) {
                            if (controllerState.keys.find(key) == controllerState.keys.end()) {
                                keyAvailable = false;
                                break;
                            }
                        }
                    }
                }
                bool available = controlleAvailable || keyAvailable;
                if (available)
                {
                    // Logger::info("{} {} {}", repeating, repeatingButtonTimer, resetRepeat);
                    if (!repeating || action.allowRepeating) {
                        anyAction = true;
                        action.actionListener(hintParent);
                    }
                }
            }
            hintParent = hintParent->getParent();
        }
        if (!isRepeat) {
            keyPressTime = repeatingKeyTimer = 0;
        }
    }
    Time now = getCPUTimeUsec();
    if (anyKeyPressed && now - keyPressTime > 1000)
    {
        keyPressTime = now;
        repeatingKeyTimer++; // Increased once every ~1ms
    }
    // oldControllerState = controllerState;
}

Platform* Application::getPlatform()
{
    return Application::platform;
}

AudioPlayer* Application::getAudioPlayer()
{
    return Application::platform->getAudioPlayer();
}

void Application::quit()
{
    Application::quitRequested = true;
}

void Application::navigate(FocusDirection direction, bool repeating)
{
    // Dismiss if repeating the same
    if (repeating && Application::repetitionOldFocus == Application::currentFocus)
        return;

    Application::repetitionOldFocus = Application::currentFocus;

    // Dismiss navigation if input type was changed
    if (Application::setInputType(InputType::GAMEPAD))
        return;

    View* currentFocus = Application::currentFocus;

    // Do nothing if there is no current focus
    if (!currentFocus)
        return;

    View* nextFocus = nullptr;

    // Handle custom navigation routes
    // By View ptr
    if (currentFocus->hasCustomNavigationRouteByPtr(direction))
    {
        nextFocus = currentFocus->getCustomNavigationRoutePtr(direction);

        if (!nextFocus)
            Logger::warning("Tried to follow a navigation route that leads to a nullptr view! (from=\"{}\", direction={})", currentFocus->describe(), std::to_string((int)direction));
    }
    // By ID
    else if (currentFocus->hasCustomNavigationRouteById(direction))
    {
        std::string id = currentFocus->getCustomNavigationRouteId(direction);
        nextFocus      = currentFocus->getNearestView(id);

        if (!nextFocus)
            Logger::warning("Tried to follow a navigation route that leads to an unknown view ID! (from=\"{}\", direction={}, targetId=\"{}\")", currentFocus->describe(), std::to_string((int)direction), id);
    }
    // Do nothing if current focus doesn't have a parent
    // (in which case there is nothing to traverse)
    else if (currentFocus->hasParent())
    {
        // Get next view to focus by traversing the views tree upwards
        nextFocus = currentFocus->getNextFocus(direction, currentFocus);
    }

    // No view to focus at the end of the traversal: wiggle and return
    if (!nextFocus)
    {
        Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
        Application::currentFocus->shakeHighlight(direction);
        return;
    }

    // If new focus not the same as now, play sound and give it focus
    if (nextFocus->getDefaultFocus() && Application::getCurrentFocus() != nextFocus->getDefaultFocus() && nextFocus->getVisibility() == Visibility::VISIBLE)
    {
        enum Sound focusSound = nextFocus->getFocusSound();
        Application::getAudioPlayer()->play(focusSound);
        Application::giveFocus(nextFocus);
    }
    else
    {
        Application::currentFocus->shakeHighlight(direction);
    }
}

void Application::onControllerButtonPressed(enum ControllerButton button, bool repeating)
{

    // Actions
    if (Application::handleAction(button, repeating))
        return;

    // Navigation
    // Only navigate if the button hasn't been consumed by an action
    // (allows overriding DPAD buttons using actions)
    switch (button)
    {
        case BUTTON_NAV_DOWN:
            Application::navigate(FocusDirection::DOWN, repeating);
            return;
        case BUTTON_NAV_UP:
            Application::navigate(FocusDirection::UP, repeating);
            return;
        case BUTTON_NAV_LEFT:
            Application::navigate(FocusDirection::LEFT, repeating);
            return;
        case BUTTON_NAV_RIGHT:
            Application::navigate(FocusDirection::RIGHT, repeating);
            return;
        default:
            break;
    }

    // Only play the error sound if no action applied
    Application::getAudioPlayer()->play(SOUND_CLICK_ERROR);
}

bool Application::setInputType(InputType type)
{
    if (type == Application::inputType)
        return false;

    Application::inputType = type;
    globalInputTypeChangeEvent.fire(type);

    if (type == InputType::GAMEPAD)
    {
        Application::setDrawCoursor(false);
        Application::currentFocus->onFocusGained();
    }

    return true;
}

View* Application::getCurrentFocus()
{
    return Application::currentFocus;
}

bool Application::handleAction(char button, bool repeating)
{
    // Dismiss if input type was changed
    if (button == BUTTON_A && setInputType(InputType::GAMEPAD))
        return false;

    //    if (button == BUTTON_B && setInputType(InputType::GAMEPAD))
    //    {
    //        activitiesStack.back()->getContentView()->dismiss();
    //        return true;
    //    }

    if (Application::activitiesStack.empty())
        return false;

    View* hintParent = Application::currentFocus;
    std::set<enum ControllerButton> consumedButtons;

    if (!hintParent)
        hintParent = Application::activitiesStack[Application::activitiesStack.size() - 1]->getContentView();

    while (hintParent)
    {
        for (auto& action : hintParent->getActions())
        {
            if (action.button != static_cast<enum ControllerButton>(button))
                continue;

            if (consumedButtons.find(action.button) != consumedButtons.end())
                continue;

            if (action.available && (!repeating || action.allowRepeating))
            {
                if (action.actionListener(hintParent))
                {
                    setInputType(InputType::GAMEPAD);
                    if (button == BUTTON_A)
                        hintParent->playClickAnimation();

                    Application::getAudioPlayer()->play(action.sound);

                    consumedButtons.insert(action.button);
                }
            }
        }

        hintParent = hintParent->getParent();
    }

    return !consumedButtons.empty();
}

void Application::frame()
{
    VideoContext* videoContext = Application::platform->getVideoContext();

    // Frame context
    FrameContext frameContext = FrameContext();

    frameContext.pixelRatio = (float)Application::windowWidth / (float)Application::windowHeight;
    frameContext.vg         = Application::getNVGContext();
    frameContext.fontStash  = &Application::fontStash;
    frameContext.theme      = Application::getTheme();

    // Begin frame and clear
    NVGcolor backgroundColor = frameContext.theme["brls/background"];
    videoContext->beginFrame();
    videoContext->clear(backgroundColor);
    float scaleFactor = frameContext.pixelRatio;
#if defined(BOREALIS_USE_METAL) || defined(BOREALIS_USE_D3D11)
    // metal 用 frameContext.pixelRatio 会无法铺满窗口，改用 scaleFactor
    // d3d11 用  文字有明显的锯齿，改用 scaleFactor
    scaleFactor = Application::getPlatform()->getVideoContext()->getScaleFactor();
#endif

    nvgBeginFrame(Application::getNVGContext(), Application::windowWidth, Application::windowHeight, scaleFactor);
    nvgScale(Application::getNVGContext(), Application::windowScale, Application::windowScale);

    std::vector<View*> viewsToDraw;

    // Draw all activities in the stack
    // until we find one that's not translucent
    for (size_t i = 0; i < Application::activitiesStack.size(); i++)
    {
        Activity* activity = Application::activitiesStack[Application::activitiesStack.size() - 1 - i];

        View* view = activity->getContentView();
        if (view)
            viewsToDraw.push_back(view);

        if (!activity->isTranslucent())
            break;
    }

    for (size_t i = 0; i < viewsToDraw.size(); i++)
    {
        View* view = viewsToDraw[viewsToDraw.size() - 1 - i];
        view->frame(&frameContext);
    }

    if (currentFocus && Application::getInputType() != InputType::TOUCH)
    {
        currentFocus->frameHighlight(&frameContext);
    }

    if (isDrawCursor())
    {
        getPlatform()->getInputManager()->drawCursor(frameContext.vg);
    }

    if (debuggingViewEnabled)
    {
        if (!debugLayer)
            debugLayer = new DebugLayer();

        debugLayer->frame(&frameContext);
    }

    // End frame
    nvgResetTransform(Application::getNVGContext()); // scale
    nvgEndFrame(Application::getNVGContext());

    Application::platform->getVideoContext()->endFrame();
}

void Application::exit()
{

    exitEvent.fire();
    Logger::info("Exiting...");

    Application::clear();

    // Free views deletion pool
    for (auto view : Application::deletionPool)
        delete view;

    Application::deletionPool.clear();

    Threading::stop();
    delete Application::platform;
}

void Application::setGlobalQuit(bool enabled)
{
    Application::globalQuitEnabled = enabled;
    for (auto it = Application::activitiesStack.begin(); it != Application::activitiesStack.end(); ++it)
    {
        if (enabled)
            Application::gloablQuitIdentifier = (*it)->registerExitAction();
        else
            (*it)->unregisterAction(Application::gloablQuitIdentifier);
    }
}

void Application::setFPSStatus(bool enabled)
{
    Application::globalFPSToggleEnabled = enabled;
    if (!enabled)
        Application::globalFPS = 60;
}

bool Application::getFPSStatus()
{
    return Application::globalFPSToggleEnabled;
}

size_t Application::getFPS()
{
    return Application::globalFPS;
}

void Application::setLimitedFPS(size_t fps)
{
    Application::limitedFrameTime = 1000000.0f / fps;
}

void Application::notify(std::string text)
{
    // To be implemented
}

void Application::giveFocus(View* view)
{
    View* oldFocus = Application::currentFocus;
    View* newFocus = view ? view->getDefaultFocus() : nullptr;

    if (oldFocus != newFocus && newFocus != nullptr)
    {
        if (oldFocus)
            oldFocus->onFocusLost();

        Application::currentFocus = newFocus;
        Application::globalFocusChangeEvent.fire(newFocus);

        if (newFocus)
        {
            newFocus->onFocusGained();
            Logger::debug("Giving focus to {}", newFocus->describe());
        }

        Application::globalHintsUpdateEvent.fire();
    }
}

bool Application::popActivity(TransitionAnimation animation, std::function<void(void)> cb, bool free)
{
    if (Application::activitiesStack.size() <= 1) // never pop the first activity
        return false;

    Application::blockInputs();

    Activity* last = Application::activitiesStack[Application::activitiesStack.size() - 1];
    last->willDisappear(true);

    last->setInFadeAnimation(true);

    bool fade = animation == TransitionAnimation::FADE;

    Activity* toShow = nullptr;
    // Animate the old activity immediately
    if (Application::activitiesStack.size() > 1)
    {
        toShow = Application::activitiesStack[Application::activitiesStack.size() - 2];
        toShow->hide([]() {}, false, 0);
        toShow->onResume();
        toShow->show([]() {}, false, 0);
    }

    // Focus
    if (!Application::focusStack.empty())
    {
        View* newFocus = Application::focusStack[Application::focusStack.size() - 1];

        if (!toShow || newFocus->getParentActivity() == toShow)
        {
            Logger::debug("Giving focus to {}, and removing it from the focus stack", newFocus->describe());
            Application::giveFocus(newFocus);
        }
        else if (toShow)
        {
            Application::giveFocus(toShow->getContentView());
        }

        Application::focusStack.pop_back();
    }

    // Hide animation (and show previous activity, if any)
    last->hide([last, cb, free]()
        {
        Application::activitiesStack.pop_back();
        cb();
        brls::Logger::debug("Start delete top activity");
        if(free) delete last;
        brls::Logger::debug("Top activity deleted");

        Application::unblockInputs(); },
        fade, last->getShowAnimationDuration(animation));

    return true;
}

std::vector<Activity*> Application::getActivitiesStack()
{
    return activitiesStack;
}

void Application::pushActivity(Activity* activity, TransitionAnimation animation)
{
    Application::blockInputs();

    // Focus
    if (!Application::activitiesStack.empty() && Application::currentFocus != nullptr)
    {
        Logger::debug("Pushing {} to the focus stack", Application::currentFocus->describe());
        Application::focusStack.push_back(Application::currentFocus);
    }

    // Create the activity content view
    activity->setContentView(activity->createContentView());
    activity->onContentAvailable();
    activity->resizeToFitWindow();

    if (!Application::activitiesStack.empty())
    {
        Activity* last = Application::activitiesStack[Application::activitiesStack.size() - 1];
        last->onPause();
    }

    bool fadeIn = animation == TransitionAnimation::FADE || animation == TransitionAnimation::SLIDE_LEFT || animation == TransitionAnimation::SLIDE_RIGHT; // wait for the old activity animation to be done before showing the new one?

    if (Application::globalQuitEnabled)
        Application::gloablQuitIdentifier = activity->registerExitAction();

    // Layout and prepare activity
    activity->willAppear(true);
    Application::giveFocus(activity->getDefaultFocus());

    if (!fadeIn) // No animations
    {
        brls::Logger::debug("push activity to the stack");
        Application::activitiesStack.push_back(activity);
        Application::unblockInputs();
    }
    else
    {
        activity->hide([]() {}, false, 0);

        brls::Logger::debug("push activity to the stack");
        Application::activitiesStack.push_back(activity);
        float duration = activity->getShowAnimationDuration(animation);
        activity->show([]()
            { Application::unblockInputs(); },
            duration > 0, duration);
    }
}

void Application::clear()
{
    for (Activity* activity : Application::activitiesStack)
    {
        activity->willDisappear(true);
        delete activity;
    }

    Application::activitiesStack.clear();
}

Theme Application::getTheme()
{
    if (Application::getThemeVariant() == ThemeVariant::LIGHT)
        return Theme::getLightTheme();
    else
        return Theme::getDarkTheme();
}

ThemeVariant Application::getThemeVariant()
{
    return Application::platform->getThemeVariant();
}

ImeManager* Application::getImeManager()
{
    return Application::platform->getImeManager();
}

std::string Application::getLocale()
{
    return Application::getPlatform()->getLocale();
}

void Application::addToFreeQueue(View* view)
{
    brls::Logger::verbose("Application::addToFreeQueue {}", view->describe());
    deletionPool.insert(view);
}

void Application::tryDeinitFirstResponder(View* view)
{
    if (!view)
        return;

    // Interrupt current gestures if presented
    for (size_t i = 0; i < currentTouchState.size(); i++)
    {
        if (currentTouchState[i].view == view)
        {
            currentTouchState[i].view->interruptGestures(false);
            currentTouchState[i].view = nullptr;
        }
    }

    if (currentMouseState.view == view)
    {
        currentMouseState.view->interruptGestures(false);
        currentMouseState.view = nullptr;
    }
}

bool Application::loadFontFromFile(std::string fontName, std::string filePath)
{
    int handle = nvgCreateFont(Application::getNVGContext(), fontName.c_str(), filePath.c_str());

    if (handle == FONT_INVALID)
    {
        Logger::warning("Could not load the font \"{}\"", fontName);
        return false;
    }

    Application::fontStash[fontName] = handle;
    return true;
}

bool Application::loadFontFromMemory(std::string fontName, void* address, size_t size, bool freeData)
{
    int handle = nvgCreateFontMem(Application::getNVGContext(), fontName.c_str(), (unsigned char*)address, size, freeData);

    if (handle == FONT_INVALID)
    {
        Logger::warning("Could not load the font \"{}\"", fontName);
        return false;
    }

    Application::fontStash[fontName] = handle;
    return true;
}

void Application::crash(std::string text)
{
    // To be implemented
}

void Application::blockInputs(bool muteSounds)
{
    Application::muteSounds |= muteSounds;
    Application::blockInputsTokens += 1;
    getGlobalHintsUpdateEvent()->fire();
    Logger::debug("Adding an inputs block token (tokens={})", Application::blockInputsTokens);
}

void Application::unblockInputs()
{
    Application::blockInputsTokens -= 1;

    if (Application::blockInputsTokens <= 0)
        muteSounds = false;

    getGlobalHintsUpdateEvent()->fire();
    Logger::debug("Removing an inputs block token (tokens={})", Application::blockInputsTokens);
}

bool Application::isInputBlocks()
{
    return Application::blockInputsTokens > 0;
}

void Application::setSwapInputKeys(bool swap)
{
    swapInputKeys = swap;
    getGlobalHintsUpdateEvent()->fire();
}

NVGcontext* Application::getNVGContext()
{
    return Application::platform->getVideoContext()->getNVGContext();
}

void Application::setCommonFooter(std::string footer)
{
    Application::commonFooter = footer;
}

std::string* Application::getCommonFooter()
{
    return &Application::commonFooter;
}

void Application::setWindowSize(int width, int height)
{
    Application::windowWidth  = width;
    Application::windowHeight = height;

    // Rescale UI
    Application::windowScale   = (float)width / (float)ORIGINAL_WINDOW_WIDTH;
    Application::contentWidth  = ORIGINAL_WINDOW_WIDTH;
    Application::contentHeight = (unsigned)roundf((float)height / Application::windowScale);

    for (Activity* activity : Application::activitiesStack)
        activity->onWindowSizeChanged();
}

void Application::onWindowResized(int width, int height)
{
    // Trigger event when Window size is stable
    static size_t iter = 0;
    brls::cancelDelay(iter);
    iter = brls::delay(100, [width, height]()
        {
            Logger::info("Window size changed to {}x{}, content size: {}x{} factor: {}",
                width, height, contentWidth, contentHeight, Application::windowScale);
            brls::Logger::info("scale factor: {}", Application::getPlatform()->getVideoContext()->getScaleFactor());

            Application::setWindowSize(width, height);
            Application::getWindowSizeChangedEvent()->fire(); });
}

void Application::setWindowPosition(int x, int y)
{
    Application::windowXPos = x;
    Application::windowYPos = y;
}

void Application::onWindowReposition(int x, int y)
{
    static size_t iter = 0;
    brls::cancelDelay(iter);
    iter = brls::delay(100, [x, y]()
        {
            Logger::info("Window position changed to {}x{}", x, y);
            Application::setWindowPosition(x, y); });
}

std::string Application::getTitle()
{
    return Application::title;
}

GenericEvent* Application::getGlobalFocusChangeEvent()
{
    return &Application::globalFocusChangeEvent;
}

VoidEvent* Application::getGlobalHintsUpdateEvent()
{
    return &Application::globalHintsUpdateEvent;
}

Event<InputType>* Application::getGlobalInputTypeChangeEvent()
{
    return &Application::globalInputTypeChangeEvent;
}

VoidEvent* Application::getRunLoopEvent()
{
    return &Application::runLoopEvent;
}

VoidEvent* Application::getExitEvent()
{
    return &Application::exitEvent;
}

VoidEvent* Application::getWindowSizeChangedEvent()
{
    return &Application::windowSizeChangedEvent;
}

VoidEvent* Application::getWindowCreationDoneEvent()
{
    return &Application::windowCreationDoneEvent;
}

VoidEvent* Application::getWindowShouldCloseEvent()
{
    return &Application::windowShouldCloseEvent;
}

Event<bool>* Application::getWindowFocusChangedEvent()
{
    return &Application::windowFocusChangedEvent;
}

int Application::getFont(std::string fontName)
{
    if (Application::fontStash.count(fontName) == 0)
        return FONT_INVALID;

    return Application::fontStash[fontName];
}

int Application::getDefaultFont()
{
#ifdef __SWITCH__
    static int regular = Application::getFont(FONT_CHINESE_SIMPLIFIED);
#else
    static int regular = Application::getFont(FONT_REGULAR);
#endif
    return regular;
}

bool Application::XMLViewsRegisterContains(std::string name)
{
    return Application::xmlViewsRegister.count(name) > 0;
}

XMLViewCreator Application::getXMLViewCreator(std::string name)
{
    return Application::xmlViewsRegister[name];
}

void Application::registerBuiltInXMLViews()
{
    Application::registerXMLView("brls:Box", Box::create);
    Application::registerXMLView("brls:Rectangle", Rectangle::create);
    Application::registerXMLView("brls:AppletFrame", AppletFrame::create);
    Application::registerXMLView("brls:Label", Label::create);
    Application::registerXMLView("brls:TabFrame", TabFrame::create);
    Application::registerXMLView("brls:Sidebar", Sidebar::create);
    Application::registerXMLView("brls:Header", Header::create);
    Application::registerXMLView("brls:ScrollingFrame", ScrollingFrame::create);
    Application::registerXMLView("brls:HScrollingFrame", HScrollingFrame::create);
    Application::registerXMLView("brls:RecyclerFrame", RecyclerFrame::create);
    Application::registerXMLView("brls:Image", Image::create);
    Application::registerXMLView("brls:Padding", Padding::create);
    Application::registerXMLView("brls:Button", Button::create);
    Application::registerXMLView("brls:CheckBox", CheckBox::create);
    Application::registerXMLView("brls:Hints", Hints::create);
    Application::registerXMLView("brls:Slider", Slider::create);
    Application::registerXMLView("brls:BottomBar", BottomBar::create);
    Application::registerXMLView("brls:ProgressSpinner", ProgressSpinner::create);

    // Cells
    Application::registerXMLView("brls:DetailCell", DetailCell::create);
    Application::registerXMLView("brls:RadioCell", RadioCell::create);
    Application::registerXMLView("brls:BooleanCell", BooleanCell::create);
    Application::registerXMLView("brls:SelectorCell", SelectorCell::create);
    Application::registerXMLView("brls:InputCell", InputCell::create);
    Application::registerXMLView("brls:InputNumericCell", InputNumericCell::create);

    // Widgets
    Application::registerXMLView("brls:Account", AccountWidget::create);
    Application::registerXMLView("brls:Battery", BatteryWidget::create);
    Application::registerXMLView("brls:Wireless", WirelessWidget::create);
}

void Application::registerXMLView(std::string name, XMLViewCreator creator)
{
    Application::xmlViewsRegister[name] = creator;
}

} // namespace brls
