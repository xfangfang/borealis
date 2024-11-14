/*
Copyright 2024 xfangfang

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
#include <borealis/platforms/psv/psv_video.hpp>

#define NANOVG_GXM_IMPLEMENTATION
#define NANOVG_GXM_UTILS_IMPLEMENTATION
#include <nanovg_gxm.h>

namespace brls
{

PsvVideoContext::PsvVideoContext()
{
#ifdef USE_VITA_SHARK
    if (shark_init("app0:module/libshacccg.suprx") < 0) {
        sceClibPrintf("vitashark: failed to initialize");
        return EXIT_FAILURE;
    }
#endif

    // Initialize nanovg
    NVGXMinitOptions initOptions = {
        .msaa = SCE_GXM_MULTISAMPLE_4X,
        .swapInterval = 1,
        .dumpShader = 0,
    };

    gxm = nvgxmCreateFramebuffer(&initOptions);
    if (gxm == NULL) {
        fatal("gxm: failed to initialize");
    }

    this->nvgContext = nvgCreateGXM(gxm, 0);
    if (!this->nvgContext)
    {
        brls::fatal("sdl: unable to init nanovg");
    }

    Application::setWindowSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);


    // TODO: 删除
    brls::Logger::info("PSVita Video Context initialized");
}

PsvVideoContext::~PsvVideoContext()
{
    try
    {
        if (this->nvgContext)
        {
            nvgDeleteGXM(this->nvgContext);
            nvgxmDeleteFramebuffer(gxm);
        }
    }
    catch (...)
    {
        Logger::error("Cannot delete nvg Context");
    }
#ifdef USE_VITA_SHARK
    shark_end();
#endif
}

void PsvVideoContext::clear(NVGcolor color) {
    static NVGcolor clearColor{};
    if (clearColor.r != color.r || clearColor.g != color.g || clearColor.b != color.b || clearColor.a != color.a) {
        clearColor = color;
        gxmClearColor(color.r, color.g, color.b, color.a);
    }
}

void PsvVideoContext::resetState() { }

void PsvVideoContext::beginFrame() {
    gxmBeginFrame();
    gxmClear();
}

void PsvVideoContext::endFrame() {
    gxmEndFrame();
    gxmDialogUpdate();
    gxmSwapBuffer();
}

double PsvVideoContext::getScaleFactor() {
    return 1.0;
}

NVGcontext* PsvVideoContext::getNVGContext() {
    return this->nvgContext;
}

NVGXMframebuffer* PsvVideoContext::getFramebuffer()
{
    return this->gxm;
}

}