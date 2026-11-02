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
#include <borealis/core/thread.hpp>
#include <borealis/platforms/sdl/sdl_video.hpp>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#ifdef BOREALIS_USE_OPENGL
#ifdef __PSV__
#include <GLES2/gl2.h>
extern "C"
{
#include <gpu_es4/psp2_pvr_hint.h>
#include <psp2/kernel/modulemgr.h>
}
#define NANOVG_GLES2_IMPLEMENTATION
#elif defined(PS4)
#include <orbis/Pigletv2VSH.h>
#define NANOVG_GLES2_IMPLEMENTATION
#else
#include <glad/glad.h>
#ifdef USE_GLES2
#define NANOVG_GLES2_IMPLEMENTATION
#elif USE_GLES3
#define NANOVG_GLES3_IMPLEMENTATION
#elif USE_GL2
#define NANOVG_GL2_IMPLEMENTATION
#else
#define NANOVG_GL3_IMPLEMENTATION
#endif
#endif
#include <nanovg_gl.h>
#elif defined(BOREALIS_USE_D3D11)
#include <nanovg_d3d11.h>

#include <borealis/platforms/driver/d3d11.hpp>
std::unique_ptr<brls::D3D11Context> D3D11_CONTEXT;
#elif defined(BOREALIS_USE_VULKAN)
#define NANOVG_VULKAN_IMPLEMENTATION
#include <nanovg_vk.h>

#include <borealis/platforms/driver/vulkan.hpp>
#endif

namespace brls
{

static double scaleFactor = 1.0;

static void sdlWindowFramebufferSizeCallback(SDL_Window* window, int width, int height)
{
    if (!width || !height)
        return;

    int fWidth, fHeight;
#ifdef BOREALIS_USE_OPENGL
    SDL_GL_GetDrawableSize(window, &fWidth, &fHeight);
    scaleFactor = fWidth * 1.0 / width;
#if defined(ANDROID)
    // On Android, doing this is to ensure that glViewport is called from the main thread
    brls::sync([fWidth, fHeight]()
        { glViewport(0, 0, fWidth, fHeight); });
#else
    glViewport(0, 0, fWidth, fHeight);
#endif
#elif defined(BOREALIS_USE_D3D11)
    fWidth      = width;
    fHeight     = height;
    scaleFactor = D3D11_CONTEXT->getScaleFactor();
    D3D11_CONTEXT->onFramebufferSize(fWidth, fHeight);
#elif defined(BOREALIS_USE_VULKAN)
    fWidth      = width;
    fHeight     = height;
    scaleFactor = VULKAN_CONTEXT ? VULKAN_CONTEXT->getScaleFactor() : 1.0;
    if (VULKAN_CONTEXT)
        VULKAN_CONTEXT->onFramebufferSize(fWidth, fHeight);
#endif

    Application::onWindowResized(fWidth, fHeight);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::sizeW = width;
        VideoContext::sizeH = height;
    }
}

static void sdlWindowPositionCallback(SDL_Window* window, int windowXPos, int windowYPos)
{
    Application::onWindowReposition(windowXPos, windowYPos);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::posX = (float)windowXPos;
        VideoContext::posY = (float)windowYPos;
    }
}

static int sdlWindowEventWatcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_WINDOWEVENT)
    {
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        switch (event->window.event)
        {
            case SDL_WINDOWEVENT_RESIZED:
                if (win == (SDL_Window*)data)
                {
                    sdlWindowFramebufferSizeCallback(win,
                        event->window.data1,
                        event->window.data2);
                }
                break;
            case SDL_WINDOWEVENT_MOVED:
                if (win == (SDL_Window*)data)
                {
                    sdlWindowPositionCallback(win,
                        event->window.data1,
                        event->window.data2);
                }
                break;
        }
    }
    return 0;
}

SDLVideoContext::SDLVideoContext(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, float windowXPos, float windowYPos)
{
#ifdef __PSV__
#define MAX_PATH 256
    /// Huge thanks to SonicMastr for his kindness help and contribution in psv homebrew.

    windowWidth  = 960;
    windowHeight = 544;

    /* Disable Back Touchpad to prevent "misclicks" */
    SDL_setenv("VITA_DISABLE_TOUCH_BACK", "1", 1);

    /* We need to use some custom hints */
    SDL_setenv("VITA_PVR_SKIP_INIT", "yeet", 1);

    PVRSRV_PSP2_APPHINT hint;
    char target_path[MAX_PATH];
    const char* default_path = "app0:module";

    /* Load Modules */
    sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, NULL, 0, NULL, NULL);
    sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, NULL, 0, NULL, NULL);
    snprintf(target_path, MAX_PATH, "%s/%s", default_path, "libgpu_es4_ext.suprx");
    sceKernelLoadStartModule(target_path, 0, NULL, 0, NULL, NULL);
    snprintf(target_path, MAX_PATH, "%s/%s", default_path, "libIMGEGL.suprx");
    sceKernelLoadStartModule(target_path, 0, NULL, 0, NULL, NULL);

    /* Set PVR Hints */
    PVRSRVInitializeAppHint(&hint);
    snprintf(hint.szGLES1, MAX_PATH, "%s/%s", default_path, "libGLESv1_CM.suprx");
    snprintf(hint.szGLES2, MAX_PATH, "%s/%s", default_path, "libGLESv2.suprx");
    snprintf(hint.szWindowSystem, MAX_PATH, "%s/%s", default_path, "libpvrPSP2_WSEGL.suprx");

    hint.ui32SwTexOpCleanupDelay = 32000; // Set to 32 milliseconds to prevent a pool of unfreed memory
    PVRSRVCreateVirtualAppHint(&hint);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        Logger::error("sdl: failed to initialize");
        return;
    }

    // Create window
    Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef BOREALIS_USE_OPENGL
#ifdef __SWITCH__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#elif defined(__PSV__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#elif defined(PS5)
    // The console's G19 EGL driver owns exactly one fixed 1920x1080
    // double-buffered OpenGL window and rejects any other dimensions or a
    // resizable window, so coerce the request to what it accepts.
    windowWidth  = 1920;
    windowHeight = 1080;
    windowFlags &= ~(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    windowFlags |= SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;
#elif defined(USE_GLES2)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#elif defined(USE_GLES3)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#elif defined(USE_GL2)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
#endif
#if defined(USE_EGL)
    SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1");
#endif
    windowFlags |= SDL_WINDOW_OPENGL;
#elif defined(BOREALIS_USE_VULKAN)
    windowFlags |= SDL_WINDOW_VULKAN;
#endif
    if (VideoContext::FULLSCREEN)
    {
#ifdef __WINRT__
        windowFlags |= SDL_WINDOW_FULLSCREEN;
#else
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
    }
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

#if defined(PS5) && defined(BOREALIS_USE_VULKAN)
    // On PS5 with Vulkan, presentation is direct to VideoOut via VK_KHR_display.
    // SDL is used exclusively for gamepad controller input and audio.
    // The PS5 G19 SDL2 video driver has no Vulkan support and rejects SDL_CreateWindow.
    static uint64_t dummyStorage[64] = {0};
    this->window = (SDL_Window*)dummyStorage;
    windowWidth = 1920;
    windowHeight = 1080;
    Logger::info("sdl: using PS5 dummy window for Vulkan direct presentation");
#else
    if (std::isnan(windowXPos) || std::isnan(windowYPos))
    {
        this->window = SDL_CreateWindow(windowTitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            windowWidth,
            windowHeight,
            windowFlags);
    }
    else
    {
        this->window = SDL_CreateWindow(windowTitle.c_str(),
            windowXPos > 0 ? windowXPos : SDL_WINDOWPOS_UNDEFINED,
            windowYPos > 0 ? windowYPos : SDL_WINDOWPOS_UNDEFINED,
            windowWidth,
            windowHeight,
            windowFlags);
    }

    if (!this->window)
    {
        fatal("sdl: failed to create window");
    }
#endif
#ifdef BOREALIS_USE_OPENGL
    // Configure window
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
#endif
#if !(defined(PS5) && defined(BOREALIS_USE_VULKAN))
    SDL_AddEventWatch(sdlWindowEventWatcher, window);
#endif
#ifdef BOREALIS_USE_OPENGL
#if !defined(__PSV__) && !defined(PS4)
    // Load OpenGL routines using glad
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
#endif

    Logger::info("sdl: GL Vendor: {}", (const char*)glGetString(GL_VENDOR));
    Logger::info("sdl: GL Renderer: {}", (const char*)glGetString(GL_RENDERER));
    Logger::info("sdl: GL Version: {}", (const char*)glGetString(GL_VERSION));

    // Initialize nanovg
#ifdef __PSV__
    this->nvgContext = nvgCreateGLES2(0);
#elif PS4
    // Same as GLES2, but with pre-compiled shaders, so the flags must be "NVG_STENCIL_STROKES | NVG_ANTIALIAS" now
    this->nvgContext = nvgCreateGLES2(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#elif USE_GLES2
    this->nvgContext = nvgCreateGLES2(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#elif USE_GLES3
    this->nvgContext = nvgCreateGLES3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#elif USE_GL2
    this->nvgContext = nvgCreateGL2(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#else
    this->nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#endif
#elif defined(BOREALIS_USE_D3D11)
    Logger::info("sdl: USE_D3D11");
    D3D11_CONTEXT    = std::make_unique<D3D11Context>(this->window, windowWidth, windowHeight);
    this->nvgContext = nvgCreateD3D11(D3D11_CONTEXT->getDevice(), NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#elif defined(BOREALIS_USE_VULKAN)
    Logger::info("sdl: USE_VULKAN");
    VULKAN_CONTEXT   = std::make_unique<VulkanContext>(this->window, windowWidth, windowHeight);
    VKNVGCreateInfo createInfo = VULKAN_CONTEXT->getNVGCreateInfo();
    this->nvgContext = nvgCreateVk(createInfo, NVG_ANTIALIAS | NVG_STENCIL_STROKES, VULKAN_CONTEXT->getGraphicsQueue());
#endif
    if (!this->nvgContext)
    {
        brls::fatal("sdl: unable to init nanovg");
    }

    setSwapInterval(VideoContext::swapInterval);

    // Setup window state
    int width = windowWidth, height = windowHeight;
#if !(defined(PS5) && defined(BOREALIS_USE_VULKAN))
    SDL_GetWindowSize(window, &width, &height);
#endif

    int fWidth, fHeight;
#ifdef BOREALIS_USE_OPENGL
    SDL_GL_GetDrawableSize(window, &fWidth, &fHeight);
    scaleFactor = fWidth * 1.0 / width;
    Application::setWindowSize(fWidth, fHeight);
    glViewport(0, 0, fWidth, fHeight);
#elif defined(BOREALIS_USE_D3D11)
    scaleFactor      = D3D11_CONTEXT->getScaleFactor();
    fWidth           = width;
    fHeight          = height;
    Application::setWindowSize(fWidth, fHeight);
    D3D11_CONTEXT->onFramebufferSize(fWidth, fHeight);
#elif defined(BOREALIS_USE_VULKAN)
#if defined(PS5)
    if (VULKAN_CONTEXT)
    {
        VkExtent2D extent = VULKAN_CONTEXT->getSwapchainExtent();
        if (extent.width > 0 && extent.height > 0)
        {
            fWidth = extent.width;
            fHeight = extent.height;
            width = extent.width;
            height = extent.height;
        }
    }
#endif
    scaleFactor      = VULKAN_CONTEXT ? VULKAN_CONTEXT->getScaleFactor() : 1.0;
    fWidth           = width;
    fHeight          = height;
    Application::setWindowSize(fWidth, fHeight);
    if (VULKAN_CONTEXT)
        VULKAN_CONTEXT->onFramebufferSize(fWidth, fHeight, true);
#endif

    int xPos = 0, yPos = 0;
#if !(defined(PS5) && defined(BOREALIS_USE_VULKAN))
    SDL_GetWindowPosition(window, &xPos, &yPos);
#endif
    Application::setWindowPosition(xPos, yPos);

    if (!VideoContext::FULLSCREEN)
    {
        VideoContext::sizeW = width;
        VideoContext::sizeH = height;
        VideoContext::posX  = (float)xPos;
        VideoContext::posY  = (float)yPos;
    }
}

void SDLVideoContext::beginFrame()
{
#if defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->beginFrame();
#elif defined(BOREALIS_USE_VULKAN)
    if (VULKAN_CONTEXT)
        VULKAN_CONTEXT->beginFrame();
#endif
}

void SDLVideoContext::endFrame()
{
#ifdef BOREALIS_USE_OPENGL
    SDL_GL_SwapWindow(this->window);
#elif defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->endFrame();
#elif defined(BOREALIS_USE_VULKAN)
    if (VULKAN_CONTEXT)
        VULKAN_CONTEXT->endFrame();
#endif
}

void SDLVideoContext::setSwapInterval(int interval)
{
    VideoContext::swapInterval = interval;
#ifdef BOREALIS_USE_OPENGL
    SDL_GL_SetSwapInterval(interval);
#elif defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->setSwapInterval(interval);
#elif defined(BOREALIS_USE_VULKAN)
    if (VULKAN_CONTEXT)
        VULKAN_CONTEXT->setSwapInterval(interval);
#endif
}

void SDLVideoContext::clear(NVGcolor color)
{
#ifdef BOREALIS_USE_OPENGL
    glClearColor(
        color.r,
        color.g,
        color.b,
        color.a);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#elif defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->clear(nvgRGBAf(
        color.r,
        color.g,
        color.b,
        color.a));
#elif defined(BOREALIS_USE_VULKAN)
    if (VULKAN_CONTEXT)
        VULKAN_CONTEXT->clear(color);
#endif
}

void SDLVideoContext::resetState()
{
#ifdef BOREALIS_USE_OPENGL
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
#endif
}

double SDLVideoContext::getScaleFactor()
{
    return scaleFactor;
}

SDLVideoContext::~SDLVideoContext()
{
    try
    {
        if (this->nvgContext)
        {
#ifdef BOREALIS_USE_OPENGL
#ifdef USE_GLES2
            nvgDeleteGLES2(this->nvgContext);
#elif USE_GLES3
            nvgDeleteGLES3(this->nvgContext);
#elif USE_GL2
            nvgDeleteGL2(this->nvgContext);
#else
            nvgDeleteGL3(this->nvgContext);
#endif
#elif defined(BOREALIS_USE_D3D11)
            nvgDeleteD3D11(this->nvgContext);
            D3D11_CONTEXT = nullptr;
#elif defined(BOREALIS_USE_VULKAN)
            nvgDeleteVk(this->nvgContext);
            VULKAN_CONTEXT = nullptr;
#endif
        }
    }
    catch (...)
    {
        Logger::error("Cannot delete nvg Context");
    }
#if !(defined(PS5) && defined(BOREALIS_USE_VULKAN))
    SDL_DestroyWindow(this->window);
#endif
    SDL_Quit();
}

NVGcontext* SDLVideoContext::getNVGContext()
{
    return this->nvgContext;
}

SDL_Window* SDLVideoContext::getSDLWindow()
{
    return this->window;
}

void SDLVideoContext::fullScreen(bool fs)
{
#if !(defined(PS5) && defined(BOREALIS_USE_VULKAN))
#ifdef __WINRT__
    // win32 会很模糊，而且点击事件貌似也错位了，只给 winrt 使用。
    static unsigned int flag = SDL_WINDOW_FULLSCREEN;
#else
    static unsigned int flag = SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
    SDL_SetWindowFullscreen(this->window, fs ? flag : 0);
#endif
}

} // namespace brls
