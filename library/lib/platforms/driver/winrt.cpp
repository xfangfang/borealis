#include <borealis/platforms/driver/winrt.hpp>
#include <SDL_main.h>
#include <SDL_syswm.h>
#include <windows.ui.core.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.AccessCache.h>

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

extern "C" int SDL_main(int argc, char *argv[]);

typedef struct HINSTANCE__ *HINSTANCE;
/* The VC++ compiler needs main/wmain defined, but not for GDK */
#if defined(_MSC_VER) && !defined(__GDK__)
#if defined(UNICODE) && UNICODE
int wmain(int argc, wchar_t *wargv[], wchar_t *wenvp)
{
    (void) argc;
    (void) wargv;
    (void) wenvp;
    return SDL_WinRTRunApp(SDL_main, NULL);
}
#else  /* ANSI */
int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    return SDL_WinRTRunApp(SDL_main, NULL);
}
#endif /* UNICODE */
#endif /* _MSC_VER && ! __GDK__ */

#if defined(UNICODE) && UNICODE
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR szCmdLine, int sw)
#else /* ANSI */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
#endif
{
    (void) hInst;
    (void) hPrev;
    (void) szCmdLine;
    (void) sw;
    return SDL_WinRTRunApp(SDL_main, NULL);
}
