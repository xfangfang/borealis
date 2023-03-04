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
#include <borealis/platforms/sdl/sdl_video.hpp>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef BOREALIS_USE_OPENGL
#include <glad/glad.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>
#elif defined(BOREALIS_USE_D3D11)
#include <borealis/platforms/driver/d3d11.hpp>
#include <nanovg_d3d11.h>
static std::shared_ptr<brls::D3D11Context> D3D11_CONTEXT = nullptr;
#endif
#ifdef __SWITCH__
#include <switch.h>
#endif
#ifdef __WINRT__
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.Foundation.h>
#endif

namespace brls
{

static double scaleFactor = 1.0;

#ifdef __WINRT__
static unsigned int GetDpiForSystem() {
    winrt::Windows::Graphics::Display::DisplayInformation displayInformation = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
    return (unsigned int)displayInformation.LogicalDpi();
}

static void InitDpiChanged(SDLVideoContext* ctx) {
    winrt::Windows::Graphics::Display::DisplayInformation displayInformation = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
    displayInformation.DpiChanged([&](auto && sender, auto && args){
        float dpi = (int)sender.ResolutionScale() / 100.0f;
        ctx->dpiChanged(dpi);
    });
}
#endif

static void sdlWindowFramebufferSizeCallback(SDL_Window* window, int width, int height)
{
    if (!width || !height)
        return;

    int fWidth, fHeight;
#ifdef BOREALIS_USE_OPENGL
    SDL_GetWindowSizeInPixels(window, &fWidth, &fHeight);
    scaleFactor = fWidth * 1.0 / width;
    glViewport(0, 0, fWidth, fHeight);
#elif defined(BOREALIS_USE_D3D11)
    fWidth = width;
    fHeight = height;
    D3D11_CONTEXT->ResizeFramebufferSize(fWidth, fHeight);
#endif

    brls::Logger::info("windows size changed: {} height: {}", width, height);
    brls::Logger::info("framebuffer size changed: fwidth: {} fheight: {}", fWidth, fHeight);
    brls::Logger::info("scale factor: {}", scaleFactor);

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

static int sdlEventWatcher(void* data, SDL_Event* event)
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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#else

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    //    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
#endif
    windowFlags |= SDL_WINDOW_OPENGL;
#endif

    if (isnan(windowXPos) || isnan(windowYPos))
    {
        this->window = SDL_CreateWindow(windowTitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            windowWidth,
            windowHeight,
            windowFlags);
    } else {
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
        return;
    }
    // 处理 dpi 窗口缩放
#if defined(_WIN32) || defined(__WINRT__)
    this->dpiChanged(GetDpiForSystem() / 96.0f, true);
#endif

#ifdef BOREALIS_USE_OPENGL
    // Configure window
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
#endif
    SDL_AddEventWatch(sdlEventWatcher, window);
#ifdef BOREALIS_USE_OPENGL
    // Load OpenGL routines using glad
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
    SDL_GL_SetSwapInterval(1);

    Logger::info("sdl: GL Vendor: {}", (const char*)glGetString(GL_VENDOR));
    Logger::info("sdl: GL Renderer: {}", (const char*)glGetString(GL_RENDERER));
    Logger::info("sdl: GL Version: {}", (const char*)glGetString(GL_VERSION));

    // Initialize nanovg
    this->nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
#elif defined(BOREALIS_USE_D3D11)
    Logger::info("sdl: use d3d11");
    D3D11_CONTEXT = std::make_shared<D3D11Context>();
    if (!D3D11_CONTEXT->InitializeDX(window, windowWidth, windowHeight)) {
        fatal("sdl: unable to init d3d11");
        return;
    }
    this->nvgContext = nvgCreateD3D11(D3D11_CONTEXT->GetDevice(), NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
    if (!this->nvgContext)
    {
        brls::fatal("glfw: unable to init nanovg");
        return;
    }
    // Setup scaling
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    sdlWindowFramebufferSizeCallback(window, width, height);
    int xPos, yPos;
    SDL_GetWindowPosition(window, &xPos, &yPos);
    sdlWindowPositionCallback(window, xPos, yPos);
}

void SDLVideoContext::dpiChanged(float nextDpiScale, bool init) {
    if (sizeScale != nextDpiScale && nextDpiScale >= 1.0f) {
        int width, height;
        SDL_GetWindowSize(this->window, &width, &height);
        int scale = (nextDpiScale * 100) / sizeScale;
        width = width / (int)sizeScale * (int)nextDpiScale;
        height = height / (int)sizeScale * (int)nextDpiScale;
        sizeScale = nextDpiScale;
        scaleFactor = nextDpiScale;
        SDL_SetWindowSize(this->window, width, height);
    }
}

void SDLVideoContext::beginFrame()
{
#if defined(_WIN32) && !defined(__WINRT__)
    // 暂时使用每次绘制前扫描 dpi 支持 windows 下的窗口 dpi 变化
    float nextDpiScale = GetDpiForWindow(GetActiveWindow()) / 96.0f;
    this->dpiChanged(nextDpiScale);
#endif
}

void SDLVideoContext::endFrame()
{
#ifdef BOREALIS_USE_OPENGL
    SDL_GL_SwapWindow(this->window);
#elif defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->Present();
#endif
}

void SDLVideoContext::clear(NVGcolor color)
{
#ifdef BOREALIS_USE_OPENGL
    glClearColor(
        color.r,
        color.g,
        color.b,
        1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#elif defined(BOREALIS_USE_D3D11)
    D3D11_CONTEXT->ClearWithColor(nvgRGBAf(
        color.r,
        color.g,
        color.b,
        1.0f));
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

void SDLVideoContext::disableScreenDimming(bool disable)
{
#ifdef __SWITCH__
    appletSetMediaPlaybackState(disable);
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
        if (this->nvgContext) {
#ifdef BOREALIS_USE_OPENGL
            nvgDeleteGL3(this->nvgContext);
#elif defined(BOREALIS_USE_D3D11)
            nvgDeleteD3D11(this->nvgContext);
            D3D11_CONTEXT = nullptr;
#endif
        }
    }
    catch (...)
    {
        Logger::error("Cannot delete nvg Context");
    }
    SDL_DestroyWindow(this->window);
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

void SDLVideoContext::fullScreen(bool fs) {
    SDL_SetWindowFullscreen(this->window, fs? SDL_WINDOW_FULLSCREEN : 0);
}

} // namespace brls
