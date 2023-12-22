/*
    Copyright 2021 natinusala
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

#include <borealis/core/application.hpp>
#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/glfw/glfw_platform.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <strings.h>

#ifdef __PSV__
extern "C"
{
    unsigned int sceLibcHeapSize = 64 * 1024 * 1024;
};
#endif

// glfw video and input code inspired from the glfw hybrid app by fincs
// https://github.com/fincs/hybrid_app

namespace brls
{

static void glfwErrorCallback(int errorCode, const char* description)
{
    switch (errorCode)
    {
        case GLFW_API_UNAVAILABLE:
            Logger::error("OpenGL is unavailable: {}", description);
            break;
        case GLFW_VERSION_UNAVAILABLE:
            Logger::error("OpenGL 3.2 (the minimum requirement) is not available: {}", description);
            break;
        default:
            Logger::error("GLFW {}: {}", errorCode, description);
    }
}

GLFWPlatform::GLFWPlatform()
{
    // Init glfw
    glfwSetErrorCallback(glfwErrorCallback);
    glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_TRUE);

    if (!glfwInit())
    {
        Logger::error("glfw: failed to initialize");
        return;
    }

    // Misc
    glfwSetTime(0.0);

    // Platform impls
    this->audioPlayer = new NullAudioPlayer();
}

void GLFWPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
    this->videoContext = new GLFWVideoContext(windowTitle, windowWidth, windowHeight, windowXPos, windowYPos);
    GLFWwindow* win    = this->videoContext->getGLFWWindow();
    this->inputManager = new GLFWInputManager(win);
    this->imeManager   = new GLFWImeManager(win);
}

void GLFWPlatform::restoreWindow()
{
    glfwRestoreWindow(this->videoContext->getGLFWWindow());
}

void GLFWPlatform::setWindowSize(uint32_t windowWidth, uint32_t windowHeight)
{
    if (windowWidth > 0 && windowHeight > 0) {
        glfwSetWindowSize(this->videoContext->getGLFWWindow(), windowWidth, windowHeight);
    }
}

void GLFWPlatform::setWindowSizeLimits(uint32_t windowMinWidth, uint32_t windowMinHeight, uint32_t windowMaxWidth, uint32_t windowMaxHeight)
{
    if (windowMinWidth > 0 && windowMinHeight > 0)
        glfwSetWindowSizeLimits(this->videoContext->getGLFWWindow(), windowMinWidth, windowMinHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
    if ((windowMaxWidth > 0 && windowMaxHeight > 0) && (windowMaxHeight > windowMinWidth && windowMaxHeight > windowMinHeight))
        glfwSetWindowSizeLimits(this->videoContext->getGLFWWindow(), GLFW_DONT_CARE, GLFW_DONT_CARE, windowMaxWidth, windowMaxHeight);
}

void GLFWPlatform::setWindowPosition(int windowXPos, int windowYPos)
{
    glfwSetWindowPos(this->videoContext->getGLFWWindow(), windowXPos, windowYPos);
}

void GLFWPlatform::setWindowState(uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos)
{
    if (windowWidth > 0 && windowHeight > 0)
    {
        GLFWwindow* win = this->videoContext->getGLFWWindow();
        glfwRestoreWindow(win);
        glfwSetWindowMonitor(win, nullptr, windowXPos, windowYPos, windowWidth, windowHeight, 0);
    }
}

std::string GLFWPlatform::getName()
{
    return "GLFW";
}

bool GLFWPlatform::mainLoopIteration()
{
    bool isActive;
    do
    {
        isActive = !glfwGetWindowAttrib(this->videoContext->getGLFWWindow(), GLFW_ICONIFIED);

        if (isActive)
        {
            glfwPollEvents();
            if (!Application::hasActiveEvent())
            {
                glfwWaitEventsTimeout(Application::getDeactivatedFrameTime());
            }
        }
        else
        {
            glfwWaitEvents();
        }
    } while (!isActive);

    return !glfwWindowShouldClose(this->videoContext->getGLFWWindow());
}

AudioPlayer* GLFWPlatform::getAudioPlayer()
{
    return this->audioPlayer;
}

VideoContext* GLFWPlatform::getVideoContext()
{
    return this->videoContext;
}

InputManager* GLFWPlatform::getInputManager()
{
    return this->inputManager;
}

ImeManager* GLFWPlatform::getImeManager()
{
    return this->imeManager;
}

GLFWPlatform::~GLFWPlatform()
{
    delete this->audioPlayer;
    delete this->inputManager;
    delete this->imeManager;
    delete this->videoContext;
}

} // namespace brls
