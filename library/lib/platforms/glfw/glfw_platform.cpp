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
    Logger::error("glfw: error {}: {}", errorCode, description);
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

    // Theme
    char* themeEnv = getenv("BOREALIS_THEME");
    if (themeEnv == nullptr)
    {
#ifdef __APPLE__
        char buffer[10];
        memset(buffer, 0, sizeof buffer);
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("defaults read -g AppleInterfaceStyle", "r"), pclose);
        if (pipe)
        {
            fgets(buffer, sizeof buffer, pipe.get());
            if (strncmp(buffer, "Dark", 4) == 0)
            {
                this->themeVariant = ThemeVariant::DARK;
                brls::Logger::info("Set app theme: Dark");
            }
            else
            {
                brls::Logger::info("Set app theme: Light");
            }
        }
        else
        {
            brls::Logger::error("cannot get system theme");
        }
#endif
    }
    else if (!strcasecmp(themeEnv, "DARK"))
    {
        this->themeVariant = ThemeVariant::DARK;
    }

    // Misc
    glfwSetTime(0.0);

    // Platform impls
    this->fontLoader  = new GLFWFontLoader();
    this->audioPlayer = new NullAudioPlayer();
}

void GLFWPlatform::createWindow(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight)
{
    this->videoContext = new GLFWVideoContext(windowTitle, windowWidth, windowHeight);
    this->inputManager = new GLFWInputManager(this->videoContext->getGLFWWindow());
}

bool GLFWPlatform::canShowBatteryLevel()
{
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

bool GLFWPlatform::canShowWirelessLevel()
{
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

int GLFWPlatform::getBatteryLevel()
{
#if defined(__APPLE__)
    std::string b = exec("pmset -g batt | grep -Eo '[0-9]+%'");
    if (!b.empty() && b[b.size() - 1] == '%')
    {
        b = b.substr(0, b.size() - 1);
    }
    return stoi(b);
#else
    return 100;
#endif
}

bool GLFWPlatform::isBatteryCharging()
{
#if defined(__APPLE__)
    std::string res = exec("pmset -g batt | grep -o 'AC Power'");
    return !res.empty();
#else
    return false;
#endif
}

bool GLFWPlatform::hasWirelessConnection()
{
#if defined(__APPLE__)
    std::string res = exec("networksetup -listallhardwareports | awk '/Wi-Fi/{getline; print $2}' | xargs networksetup -getairportpower | grep -o On");
    return !res.empty();
#else
    return true;
#endif
}

int GLFWPlatform::getWirelessLevel()
{
    return 3;
}

std::string GLFWPlatform::getIpAddress()
{
#if defined(__APPLE__) || defined(__linux__)
    return exec("ifconfig | grep \"inet \" | grep -Fv 127.0.0.1 | awk '{print $2}' ");
#else
    return "-";
#endif
}

std::string GLFWPlatform::getDnsServer()
{
#if defined(__APPLE__)
    return exec("scutil --dns | grep nameserver | awk '{print $3}' | sort -u | paste -s -d',' -");
#else
    return "-";
#endif
}

std::string GLFWPlatform::exec(const char* cmd)
{
    std::string result = "";
#if defined(__APPLE__) || defined(__linux__)
    char buffer[128];
    memset(buffer, 0, sizeof buffer);
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr)
    {
        result += buffer;
    }
#endif

    if (!result.empty() && result[result.size() - 1] == '\n')
        result = result.substr(0, result.size() - 1);
    return result;
}

bool GLFWPlatform::isApplicationMode()
{
    return true;
}

void GLFWPlatform::exitToHomeMode(bool value)
{
    return;
}

void GLFWPlatform::forceEnableGamePlayRecording()
{
    return;
}

void GLFWPlatform::openBrowser(std::string url)
{
    brls::Logger::debug("open url: {}", url);
#ifdef __APPLE__
    std::string cmd = "open " + url;
    system(cmd.c_str());
#endif
#ifdef __linux__
    std::string cmd = "xdg-open " + url;
    system(cmd.c_str());
#endif
#ifdef _WIN32
    std::string cmd = "explorer " + url;
    system(cmd.c_str());
#endif
    return;
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
            glfwPollEvents();
        else
            glfwWaitEvents();
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

FontLoader* GLFWPlatform::getFontLoader()
{
    return this->fontLoader;
}

ThemeVariant GLFWPlatform::getThemeVariant()
{
    return this->themeVariant;
}

void GLFWPlatform::setThemeVariant(ThemeVariant theme)
{
    this->themeVariant = theme;
}

std::string GLFWPlatform::getLocale()
{
    char* langEnv = getenv("BOREALIS_LANG");
    if (langEnv == nullptr)
        return LOCALE_DEFAULT;
    return std::string(langEnv);
}

GLFWPlatform::~GLFWPlatform()
{
    delete this->fontLoader;
    delete this->audioPlayer;
    delete this->videoContext;
    delete this->inputManager;
}

} // namespace brls
