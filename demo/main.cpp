/*
    Copyright 2020-2021 natinusala
    Copyright 2019 p-sam

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

// Switch include only necessary for demo videos recording
#ifdef __SWITCH__
#include <switch.h>
#endif

#ifdef __PSV__
#include <borealis/platforms/psv/vita_debug.h>
extern "C"
{
#include <psp2/kernel/sysmem.h>
#include <EGL/egl1_4.h>
#include <gpu_es4/psp2_pvr_hint.h>
#include <psp2/kernel/modulemgr.h>
}
#endif

#include <cstdlib>
#include <borealis.hpp>
#include <string>

#include "captioned_image.hpp"
#include "components_tab.hpp"
#include "main_activity.hpp"
#include "pokemon_view.hpp"
#include "recycling_list_tab.hpp"
#include "settings_tab.hpp"

using namespace brls::literals; // for _i18n

int main(int argc, char* argv[])
{
    // Enable recording for Twitter memes
#ifdef __SWITCH__
    appletInitializeGamePlayRecording();
#endif

    // Set log level
    // We recommend to use INFO for real apps
    brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);

#ifdef __PSV__
    sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("app0:module/libgpu_es4_ext.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("app0:module/libIMGEGL.suprx", 0, nullptr, 0, nullptr, nullptr);

    startDebug();
    brls::Logger::getLogEvent()->subscribe([](const std::string& log) {
            debug("%s\n", log.c_str());
    });


    PVRSRV_PSP2_APPHINT hint;
    PVRSRVInitializeAppHint(&hint);
    PVRSRVCreateVirtualAppHint(&hint);


    EGLenum a = eglQueryAPI();
    if (a == EGL_OPENGL_ES_API){
        brls::Logger::debug("eglQueryAPI: 1");
    } else if (a == EGL_OPENGL_API){
        brls::Logger::debug("eglQueryAPI: 2");
    } else if (a == EGL_NONE) {
        brls::Logger::debug("eglQueryAPI: 3");
    } else {
        brls::Logger::debug("eglQueryAPI: 4");
    }
#endif

//    // Init the app and i18n
//    if (!brls::Application::init())
//    {
//        brls::Logger::error("Unable to init Borealis application");
//        return EXIT_FAILURE;
//    }
//
//    brls::Application::createWindow("demo/title"_i18n);
//
//    // Have the application register an action on every activity that will quit when you press BUTTON_START
//    brls::Application::setGlobalQuit(false);
//
//    // Register custom views (including tabs, which are views)
//    brls::Application::registerXMLView("CaptionedImage", CaptionedImage::create);
//    brls::Application::registerXMLView("RecyclingListTab", RecyclingListTab::create);
//    brls::Application::registerXMLView("ComponentsTab", ComponentsTab::create);
//    brls::Application::registerXMLView("PokemonView", PokemonView::create);
//    brls::Application::registerXMLView("SettingsTab", SettingsTab::create);
//
//    // Add custom values to the theme
//    brls::Theme::getLightTheme().addColor("captioned_image/caption", nvgRGB(2, 176, 183));
//    brls::Theme::getDarkTheme().addColor("captioned_image/caption", nvgRGB(51, 186, 227));
//
//    // Add custom values to the style
//    brls::getStyle().addMetric("about/padding_top_bottom", 50);
//    brls::getStyle().addMetric("about/padding_sides", 75);
//    brls::getStyle().addMetric("about/description_margin", 50);
//
//    // Create and push the main activity to the stack
//    brls::Application::pushActivity(new MainActivity());
//
//    // Run the app
//    while (brls::Application::mainLoop())
//        ;

#ifdef __PSV__
    cleanupDebug();
#endif
    // Exit
    return EXIT_SUCCESS;
}
