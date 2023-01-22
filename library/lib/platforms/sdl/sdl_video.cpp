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

#include <glad/glad.h>

#include <borealis/core/application.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/sdl/sdl_video.hpp>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg-gl/nanovg_gl.h>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace brls
{

static double scaleFactor = 1.0;

static void sdlWindowFramebufferSizeCallback(SDL_Window* window, int width, int height)
{
    if (!width || !height)
        return;

    int fWidth, fHeight;

    SDL_GL_GetDrawableSize(window, &fWidth, &fHeight);
    scaleFactor = fWidth * 1.0 / width;

    brls::Logger::info("windows size changed: {} height: {}", width, height);
    brls::Logger::info("framebuffer size changed: fwidth: {} fheight: {}", fWidth, fHeight);
    brls::Logger::info("scale factor: {}", scaleFactor);

    Application::onWindowResized(fWidth, fHeight);
}

static void sdlWindowPositionCallback(SDL_Window* window, int windowXPos, int windowYPos)
{
    Application::onWindowReposition(windowXPos, windowYPos);
}

static int sdlEventWatcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_WINDOWEVENT) {
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        switch (event->window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                if (win == (SDL_Window*)data)
                {
                    sdlWindowFramebufferSizeCallback(win,
                        event->window.data1,
                        event->window.data2
                    );
                }
                break;
            case SDL_WINDOWEVENT_MOVED:
                if (win == (SDL_Window*)data)
                {
                    sdlWindowPositionCallback(win,
                        event->window.data1,
                        event->window.data2
                    );
                }
                break;
        }
    }
    return 0;
}

SDLVideoContext::SDLVideoContext(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight, int windowXPos, int windowYPos)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        Logger::error("sdl: failed to initialize");
        return;
    }

    // Create window
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

    this->window = SDL_CreateWindow(windowTitle.c_str(),
#if defined(__APPLE__) || defined(__linux__) || defined(_WIN32)
        windowXPos > 0 ? windowXPos : SDL_WINDOWPOS_UNDEFINED,
        windowYPos > 0 ? windowYPos : SDL_WINDOWPOS_UNDEFINED,
#else
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
#endif
        windowWidth,
        windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!this->window)
    {
        fatal("sdl: failed to create window");
        return;
    }

    // Configure window
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    SDL_AddEventWatch(sdlEventWatcher, window);

    // Load OpenGL routines using glad
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
    SDL_GL_SetSwapInterval(1);

    Logger::info("sdl: GL Vendor: {}", glGetString(GL_VENDOR));
    Logger::info("sdl: GL Renderer: {}", glGetString(GL_RENDERER));
    Logger::info("sdl: GL Version: {}", glGetString(GL_VERSION));

    // Initialize nanovg
    this->nvgContext = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_ANTIALIAS);
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

void SDLVideoContext::beginFrame()
{
}

void SDLVideoContext::endFrame()
{
    SDL_GL_SwapWindow(this->window);
}

void SDLVideoContext::clear(NVGcolor color)
{
    glClearColor(
        color.r,
        color.g,
        color.b,
        1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void SDLVideoContext::resetState()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
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
        if (this->nvgContext)
            nvgDeleteGL3(this->nvgContext);
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

} // namespace brls
