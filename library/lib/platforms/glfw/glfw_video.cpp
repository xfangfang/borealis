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
#include <borealis/core/logger.hpp>
#include <borealis/platforms/glfw/glfw_video.hpp>

// nanovg implementation
#ifdef __PSV__
#define NANOVG_GLES2_IMPLEMENTATION
#include <nanovg-gl/nanovg_gl.h>
#else
#include <glad/glad.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg-gl/nanovg_gl.h>
#endif

#ifdef __SWITCH__
#include <switch.h>
#endif

#ifdef _WIN32
#include "nanovg-gl/stb_image.h"
#endif

namespace brls
{

static double scaleFactor = 1.0;

static void glfwWindowFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (!width || !height)
        return;

    glViewport(0, 0, width, height);

    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);
    scaleFactor = width * 1.0 / wWidth;

    brls::Logger::info("windows size changed: {} height: {}", wWidth, wHeight);
    brls::Logger::info("framebuffer size changed: fwidth: {} fheight: {}", width, height);
    brls::Logger::info("scale factor: {}", scaleFactor);

    Application::onWindowResized(width, height);
}

GLFWVideoContext::GLFWVideoContext(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight)
{
    if (!glfwInit())
    {
        fatal("glfw: failed to initialize");
    }

    // Create window
#if defined(__PSV__)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(__SWITCH__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#elif defined(__linux__) || defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GL_TRUE);
#endif

// create window
#if defined(__linux__) || defined(_WIN32)
    if (VideoContext::FULLSCREEN)
    {
        GLFWmonitor* monitor    = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        this->window            = glfwCreateWindow(mode->width, mode->height, windowTitle.c_str(), monitor, nullptr);
    }
    else
    {
        this->window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
    }
#else
    this->window     = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
#endif

#ifdef _WIN32
    // Set window icon
    GLFWimage images[1];
    images[0].pixels = stbi_load("resources/icon/bilibili.png", &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(this->window, 1, images);
#endif

    if (!this->window)
    {
        glfwTerminate();
        fatal("glfw: Failed to create window");
        return;
    }

    // Configure window
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
#ifdef __APPLE__
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
#endif
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, glfwWindowFramebufferSizeCallback);

#ifndef __PSV__
    // Load OpenGL routines using glad
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#endif
    glfwSwapInterval(1);

    Logger::info("glfw: GL Vendor: {}", glGetString(GL_VENDOR));
    Logger::info("glfw: GL Renderer: {}", glGetString(GL_RENDERER));
    Logger::info("glfw: GL Version: {}", glGetString(GL_VERSION));
    Logger::info("glfw: GLFW Version: {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    // Initialize nanovg
#ifdef __PSV__
    this->nvgContext = nvgCreateGLES2(0);
#else
    this->nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#endif
    if (!this->nvgContext)
    {
        Logger::error("glfw: unable to init nanovg");
        glfwTerminate();
        return;
    }

    // Setup scaling
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glfwWindowFramebufferSizeCallback(window, width, height);

#ifdef __SWITCH__
    monitor          = glfwGetPrimaryMonitor();
    const char* name = glfwGetMonitorName(monitor);
    brls::Logger::info("glfw: Monitor: {}", name);
#endif
}

void GLFWVideoContext::beginFrame()
{
#ifdef __SWITCH__
    const GLFWvidmode* r = glfwGetVideoMode(monitor);

    if (oldWidth != r->width || oldHeight != r->height)
    {
        oldWidth  = r->width;
        oldHeight = r->height;

        glfwSetWindowSize(window, r->width, r->height);
    }
#endif
}

void GLFWVideoContext::endFrame()
{
    glfwSwapBuffers(this->window);
}

void GLFWVideoContext::clear(NVGcolor color)
{
    glClearColor(
        color.r,
        color.g,
        color.b,
        1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLFWVideoContext::resetState()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
}

void GLFWVideoContext::disableScreenDimming(bool disable)
{
#ifdef __SWITCH__
    appletSetMediaPlaybackState(disable);
#endif
}

double GLFWVideoContext::getScaleFactor()
{
    return scaleFactor;
}

GLFWVideoContext::~GLFWVideoContext()
{
    try
    {
        if (this->nvgContext)
#ifdef __PSV__
            nvgDeleteGLES2(this->nvgContext);
#else
            nvgDeleteGL3(this->nvgContext);
#endif
    }
    catch (...)
    {
        Logger::error("Cannot delete nvg Context");
    }
    glfwDestroyWindow(this->window);
    glfwTerminate();
}

NVGcontext* GLFWVideoContext::getNVGContext()
{
    return this->nvgContext;
}

void GLFWVideoContext::fullScreen(bool fs)
{
    VideoContext::FULLSCREEN = fs;

    static int posX = -1, posY = -1, sizeW = -1, sizeH = -1;

    GLFWmonitor* monitor    = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    brls::Logger::info("Set fullscreen: {} refreshRate: {}", fs, mode->refreshRate);

    if (fs)
    {
        glfwGetWindowPos(this->window, &posX, &posY);
        glfwGetWindowSize(this->window, &sizeW, &sizeH);
        glfwSetWindowMonitor(this->window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        if (sizeW < 0 || sizeH < 0)
        {
            glfwSetWindowMonitor(this->window, nullptr, abs(mode->width - 1280) / 2,
                abs(mode->height - 720) / 2, 1280, 720, GLFW_DONT_CARE);
        }
        else
        {
            glfwSetWindowMonitor(this->window, nullptr, posX, posY, sizeW, sizeH, mode->refreshRate);
        }
    }
    glfwSwapInterval(1);
}

GLFWwindow* GLFWVideoContext::getGLFWWindow()
{
    return this->window;
}

} // namespace brls
