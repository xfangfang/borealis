#include <borealis/platforms/driver/winrt.hpp>
#include <SDL_main.h>
#include <SDL_syswm.h>
#include <windows.ui.core.h>

IUnknown* SDL_GetCoreWindow(SDL_Window* window) {
    SDL_SysWMinfo info;
    SDL_GetVersion(&info.version);
    SDL_GetWindowWMInfo(window, &info);
    if (info.subsystem == SDL_SYSWM_WINRT) {
        ABI::Windows::UI::Core::ICoreWindow *coreWindow = NULL;
        if (FAILED(info.info.winrt.window->QueryInterface(&coreWindow))) {
            return NULL;
        }
        IUnknown *coreWindowAsIUnknown = NULL;
        coreWindow->QueryInterface(&coreWindowAsIUnknown);
        coreWindow->Release();
        return coreWindowAsIUnknown;
    }
    return NULL;
}
typedef int (*main_func)(int argc, char *argv[]);

int WinRTRunApp(main_func mainFunction) {
    return SDL_WinRTRunApp(mainFunction, NULL);
}
