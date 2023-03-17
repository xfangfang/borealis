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
#ifdef BOREALIS_USE_OPENGL
#ifdef __PSV__
#define NANOVG_GLES2_IMPLEMENTATION
#else
#include <glad/glad.h>
#ifdef USE_GL2
#define NANOVG_GL2_IMPLEMENTATION
#else
#define NANOVG_GL3_IMPLEMENTATION
#endif /* USE_GL2 */
#endif /* __PSV__ */
#include <nanovg_gl.h>
#elif defined(BOREALIS_USE_METAL)
static void *METAL_CONTEXT = nullptr;
#include <borealis/platforms/glfw/driver/metal.hpp>
#elif defined(BOREALIS_USE_D3D11)
#include <borealis/platforms/driver/d3d11.hpp>
#include <nanovg_d3d11.h>
static std::shared_ptr<brls::D3D11Context> D3D11_CONTEXT = nullptr;
#endif

#if defined(__linux__) || defined(_WIN32)
#include "stb_image.h"
#endif

namespace brls
{

static double scaleFactor = 1.0;

static int mini(int x, int y)
{
    return x < y ? x : y;
}

static int maxi(int x, int y)
{
    return x > y ? x : y;
}

static GLFWmonitor* getCurrentMonitor(GLFWwindow* window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor* bestmonitor;
    GLFWmonitor** monitors;
    const GLFWvidmode* mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; i++)
    {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap = maxi(0, mini(wx + ww, mx + mw) - maxi(wx, mx)) * maxi(0, mini(wy + wh, my + mh) - maxi(wy, my));

        if (bestoverlap < overlap)
        {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}

static GLFWmonitor* getAvailableMonitor(int index, int x, int y, int w, int h)
{
    int count;
    auto** monitors      = glfwGetMonitors(&count);
    GLFWmonitor* monitor = nullptr;
    if (index < count)
        monitor = monitors[index];
    else
        return nullptr;

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int monitorX, monitorY;
    glfwGetMonitorPos(monitor, &monitorX, &monitorY);

    if (x < monitorX || y < monitorY || x + w > mode->width + monitorX || y + h > mode->height + monitorY)
        return nullptr;
    return monitor;
}

static void glfwWindowFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (!width || !height)
        return;
#ifdef BOREALIS_USE_OPENGL
    glViewport(0, 0, width, height);
    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);
    scaleFactor = width * 1.0 / wWidth;
#elif defined(BOREALIS_USE_METAL)
    if (METAL_CONTEXT == nullptr) {
        return;
    }
    scaleFactor = GetMetalScaleFactor(METAL_CONTEXT);
    ResizeMetalDrawable(METAL_CONTEXT, width, height);
    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);
    // cocoa 画布大小和窗口一致
    width = wWidth;
    height = wHeight;
#elif defined(BOREALIS_USE_D3D11)
    if (D3D11_CONTEXT == nullptr) {
        return;
    }
    D3D11_CONTEXT->ResizeFramebufferSize(width, height);
    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);
    scaleFactor = width * 1.0 / wWidth;
#endif

    Application::onWindowResized(width, height);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::sizeW = wWidth;
        VideoContext::sizeH = wHeight;
    }
}

static void glfwWindowPositionCallback(GLFWwindow* window, int windowXPos, int windowYPos)
{
    Application::onWindowReposition(windowXPos, windowYPos);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::posX = (float)windowXPos;
        VideoContext::posY = (float)windowYPos;
    }
}

GLFWVideoContext::GLFWVideoContext(const std::string& windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowX, float windowY)
{
    if (!glfwInit())
    {
        fatal("glfw: failed to initialize");
    }

    // Create window
#ifdef BOREALIS_USE_OPENGL
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

#ifdef USE_GL2
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#endif
#elif defined(BOREALIS_USE_METAL) || defined(BOREALIS_USE_D3D11)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
    // If the window appears outside the screen, using the default settings
    GLFWmonitor* monitor = nullptr;
    if (!isnan(windowX) && !isnan(windowY))
        monitor = getAvailableMonitor(VideoContext::monitorIndex, (int)windowX, (int)windowY, (int)windowWidth, (int)windowHeight);

    if (monitor == nullptr)
    {
        windowX      = NAN;
        windowY      = NAN;
        windowWidth  = ORIGINAL_WINDOW_WIDTH;
        windowHeight = ORIGINAL_WINDOW_HEIGHT;
        monitor      = glfwGetPrimaryMonitor();
    }
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
#endif

// create window
#if defined(__linux__) || defined(_WIN32)
    if (VideoContext::FULLSCREEN)
    {
        glfwWindowHint(GLFW_SOFT_FULLSCREEN, 1);
        this->window = glfwCreateWindow(mode->width, mode->height, windowTitle.c_str(), monitor, nullptr);
    }
    else
    {
        this->window = glfwCreateWindow((int)windowWidth, (int)windowHeight, windowTitle.c_str(), nullptr, nullptr);
    }
#else
    this->window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
#endif

#if defined(__linux__) || defined(_WIN32)
    // Set window icon
    GLFWimage images[1];
    images[0].pixels = stbi_load(BRLS_ASSET("icon/icon.png"), &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(this->window, 1, images);
#endif

    if (!this->window)
    {
        glfwTerminate();
        fatal("glfw: Failed to create window");
        return;
    }

#if defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
    // Set window position
    if (!VideoContext::FULLSCREEN)
    {
        if (!isnan(windowX) && !isnan(windowY))
        {
            glfwSetWindowPos(this->window, (int)windowX, (int)windowY);
        }
        else
        {
            glfwSetWindowPos(this->window, fabs(mode->width - windowWidth) / 2,
                fabs(mode->height - windowHeight) / 2);
        }
    }
#endif

    // Configure window
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
#ifdef __APPLE__
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
#endif
#ifdef BOREALIS_USE_OPENGL
    glfwMakeContextCurrent(window);
#endif
    glfwSetFramebufferSizeCallback(window, glfwWindowFramebufferSizeCallback);
    glfwSetWindowPosCallback(window, glfwWindowPositionCallback);

#ifdef BOREALIS_USE_OPENGL
#ifndef __PSV__
    // Load OpenGL routines using glad
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#endif
    glfwSwapInterval(1);

    Logger::info("glfw: GL Vendor: {}", (const char*)glGetString(GL_VENDOR));
    Logger::info("glfw: GL Renderer: {}", (const char*)glGetString(GL_RENDERER));
    Logger::info("glfw: GL Version: {}", (const char*)glGetString(GL_VERSION));
#endif
    Logger::info("glfw: GLFW Version: {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    // Initialize nanovg
#ifdef BOREALIS_USE_OPENGL
#ifdef __PSV__
    this->nvgContext = nvgCreateGLES2(0);
#elif defined(USE_GL2)
    this->nvgContext = nvgCreateGL2(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#else
    this->nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#endif
#elif defined(BOREALIS_USE_METAL)
    Logger::info("glfw: use metal");
    void *ctx = CreateMetalContext(window);
    METAL_CONTEXT = ctx;
    this->nvgContext = nvgCreateMTL(GetMetalLayer(ctx), NVG_STENCIL_STROKES | NVG_ANTIALIAS);
    scaleFactor = GetMetalScaleFactor(ctx);
#elif defined(BOREALIS_USE_D3D11)
    Logger::info("glfw: use d3d11");
    D3D11_CONTEXT = std::make_shared<D3D11Context>();
    if (!D3D11_CONTEXT->InitializeDX(window, windowWidth, windowHeight)) {
        Logger::error("glfw: unable to init d3d11");
        glfwTerminate();
        return;
    }
    this->nvgContext = nvgCreateD3D11(D3D11_CONTEXT->GetDevice(), NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
    if (!this->nvgContext)
    {
        Logger::error("glfw: unable to init nanovg");
        glfwTerminate();
        return;
    }

    // Setup window state
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    Application::setWindowSize(width, height);

    int wWidth, wHeight;
    glfwGetWindowSize(window, &wWidth, &wHeight);
    scaleFactor = width * 1.0 / wWidth;

    int xPos, yPos;
    glfwGetWindowPos(window, &xPos, &yPos);
    Application::setWindowPosition(xPos, yPos);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::sizeW = width;
        VideoContext::sizeH = height;
        VideoContext::posX = (float)xPos;
        VideoContext::posY = (float)yPos;
    }

#ifdef __SWITCH__
    this->monitor    = glfwGetPrimaryMonitor();
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
#ifdef BOREALIS_USE_OPENGL
    glfwSwapBuffers(this->window);
#elif defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->Present();
#endif
}

void GLFWVideoContext::clear(NVGcolor color)
{

#ifdef BOREALIS_USE_OPENGL
    glClearColor(
        color.r,
        color.g,
        color.b,
        1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#elif defined(BOREALIS_USE_METAL)
    nvgClearWithColor(nvgContext, nvgRGBAf(
        color.r,
        color.g,
        color.b,
        1.0f));
#elif defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->ClearWithColor(nvgRGBAf(
        color.r,
        color.g,
        color.b,
        1.0f));
#endif
}

void GLFWVideoContext::resetState()
{
#ifdef BOREALIS_USE_OPENGL
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
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
#ifdef BOREALIS_USE_OPENGL
#ifdef __PSV__
            nvgDeleteGLES2(this->nvgContext);
#elif defined(USE_GL2)
            nvgDeleteGL2(this->nvgContext);
#else
            nvgDeleteGL3(this->nvgContext);
#endif
#elif defined(BOREALIS_USE_METAL)
            nvgDeleteMTL(this->nvgContext);
            METAL_CONTEXT = nullptr;
#elif defined(BOREALIS_USE_D3D11)
            nvgDeleteD3D11(this->nvgContext);
            D3D11_CONTEXT = nullptr;
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

int GLFWVideoContext::getCurrentMonitorIndex()
{
    if (!this->window)
        return 0;

    int count;
    auto* monitor   = getCurrentMonitor(this->window);
    auto** monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; i++)
    {
        if (monitor == monitors[i])
            return i;
    }
    return 0;
}

void GLFWVideoContext::fullScreen(bool fs)
{
    VideoContext::FULLSCREEN = fs;

    brls::Logger::info("Set fullscreen: {}", fs);
    if (fs)
    {
        GLFWmonitor* monitor    = getCurrentMonitor(this->window);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        VideoContext::monitorIndex = getCurrentMonitorIndex();
        glfwSetWindowMonitor(this->window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        GLFWmonitor* monitor    = glfwGetWindowMonitor(this->window);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        int monitorX, monitorY;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        glfwRestoreWindow(this->window);

        if (sizeW == 0 || sizeH == 0 || posX < monitorX || posY < monitorY || posX + sizeW > mode->width + monitorX || posY + sizeH > mode->height + monitorY)
        {
            int width = ORIGINAL_WINDOW_WIDTH;
            int height = ORIGINAL_WINDOW_HEIGHT;
            // If the window appears outside the screen, using the default settings
            glfwSetWindowMonitor(this->window, nullptr, fabs(mode->width - width) / 2,
                fabs(mode->height - height) / 2, width, height, GLFW_DONT_CARE);
        }
        else
        {
            // Set the window position and size
            glfwSetWindowMonitor(this->window, nullptr, (int)posX, (int)posY, (int)sizeW, (int)sizeH, mode->refreshRate);
        }
    }
    glfwSwapInterval(1);
}

GLFWwindow* GLFWVideoContext::getGLFWWindow()
{
    return this->window;
}

} // namespace brls
