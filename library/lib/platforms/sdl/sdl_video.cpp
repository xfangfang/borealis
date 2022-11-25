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

#include <glad/glad.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg-gl/nanovg_gl.h>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace brls
{

static void sdlWindowFramebufferSizeCallback(SDL_Window* window, int width, int height)
{
    if (!width || !height)
        return;

//    glViewport(0, 0, width, height);

    int wWidth, wHeight;
    int fWidth, fHeight;

    SDL_GetWindowSize(window, &wWidth, &wHeight);
    SDL_GL_GetDrawableSize(window, &fWidth, &fHeight);

    Application::onWindowResized(width, height);
}

static int resizingEventWatcher(void* data, SDL_Event* event) {
    if (event->type == SDL_WINDOWEVENT &&
        event->window.event == SDL_WINDOWEVENT_RESIZED) {
        SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
        if (win == (SDL_Window*)data) {
            int width, height;
            SDL_GetWindowSize(win, &width, &height);
            printf("resizing..... %d, %d\n", width, height);
            sdlWindowFramebufferSizeCallback(win, width, height);
        }
    }
    return 0;
}

SDLVideoContext::SDLVideoContext(std::string windowTitle, uint32_t windowWidth, uint32_t windowHeight)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        Logger::error("sdl: failed to initialize");
        return;
    }

    // Create window
#ifdef __SWITCH__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,     SDL_GL_CONTEXT_PROFILE_CORE);
#else

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,         8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,     SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,         1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,         1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,         8);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
#endif

    this->window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!this->window)
    {
        fatal("glfw: failed to create window");
        return;
    }

    // Configure window
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);

    SDL_AddEventWatch(resizingEventWatcher, window);

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
    SDL_DestroyWindow(this->window );
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
