/*
    Copyright 2021 natinusala
    Copyright 2023 xfangfang

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

#include <strings.h>

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/desktop/desktop_platform.hpp>
#include <memory>

#ifdef __SDL2__
#include <SDL2/SDL_misc.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

namespace brls
{

int shell_open(const char* command)
{
#ifdef __SDL2__
    return SDL_OpenURL(command);
#elif defined(_WIN32) and !defined(__WINRT__)
    WCHAR wcmd[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, command, -1, wcmd, MAX_PATH);
    ShellExecuteW(NULL, L"open", wcmd, NULL, NULL, SW_SHOWNORMAL);
    return 0;
#else
    return 0;
#endif
}

#ifdef __linux__
// Thanks to: https://github.com/videolan/vlc/blob/master/modules/misc/inhibit/dbus.c
enum INHIBIT_TYPE
{
    FDO_SS, /**< KDE >= 4 and GNOME >= 3.10 */
    FDO_PM, /**< KDE and GNOME <= 2.26 and Xfce */
    MATE, /**< >= 1.0 */
    GNOME, /**< GNOME 2.26..3.4 */
};

static const char dbus_service[][40] = {
    "org.freedesktop.ScreenSaver",
    "org.freedesktop.PowerManagement",
    "org.mate.SessionManager",
    "org.gnome.SessionManager",
};

static const char dbus_interface[][40] = {
    "org.freedesktop.ScreenSaver",
    "org.freedesktop.PowerManagement.Inhibit",
    "org.mate.SessionManager",
    "org.gnome.SessionManager",
};

static const char dbus_path[][41] = {
    "/ScreenSaver",
    "/org/freedesktop/PowerManagement/Inhibit",
    "/org/mate/SessionManager",
    "/org/gnome/SessionManager",
};

static const char dbus_method_uninhibit[][10] = {
    "UnInhibit",
    "UnInhibit",
    "Uninhibit",
    "Uninhibit",
};

static const char dbus_method_inhibit[] = "Inhibit";

static inline INHIBIT_TYPE detectLinuxDesktopEnvironment()
{
    const char* currentDesktop = getenv("XDG_CURRENT_DESKTOP");
    if (currentDesktop)
    {
        std::string xdgCurrentDesktop { currentDesktop };
        // to upper
        for (auto& i : xdgCurrentDesktop)
        {
            if ('a' <= i && i <= 'z')
            {
                i -= 32;
            }
        }
        Logger::info("XDG_CURRENT_DESKTOP: {}", xdgCurrentDesktop);
        if (xdgCurrentDesktop == "GNOME")
            return GNOME;
        if (xdgCurrentDesktop == "UBUNTU:GNOME")
            return GNOME;
        if (xdgCurrentDesktop == "MATE")
            return MATE;
    }
    if (getenv("GNOME_DESKTOP_SESSION_ID"))
    {
        Logger::info("CURRENT_DESKTOP: GNOME");
        return GNOME;
    }
    const char* kdeVersion = getenv("KDE_SESSION_VERSION");
    if (kdeVersion && atoi(kdeVersion) >= 4)
    {
        Logger::info("CURRENT_DESKTOP: KDE {}", kdeVersion);
        return FDO_SS;
    }
    Logger::info("CURRENT_DESKTOP: DEFAULT");
    return FDO_PM;
}

static INHIBIT_TYPE systemType = detectLinuxDesktopEnvironment();

static DBusConnection* connectSessionBus()
{
    DBusConnection* bus;
    DBusError err;

    dbus_error_init(&err);

    bus = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

    if (!bus)
    {
        Logger::error("Could not connect to bus: {}", err.message);
        dbus_error_free(&err);
    }

    return bus;
}

void closeSessionBus(DBusConnection* bus)
{
    Logger::info("DBus closed");
    dbus_connection_close(bus);
    dbus_connection_unref(bus);
}

uint32_t dbusInhibit(DBusConnection* connection, const std::string& app, const std::string& reason)
{
    DBusMessage* msg = dbus_message_new_method_call(dbus_service[systemType],
        dbus_path[systemType],
        dbus_interface[systemType],
        dbus_method_inhibit);
    if (!msg)
    {
        Logger::error("DBus cannot create new method call: {};{};{};{}", dbus_service[systemType],
            dbus_path[systemType],
            dbus_interface[systemType],
            dbus_method_inhibit);
        return 0;
    }

    const char* app_ptr    = app.c_str();
    const char* reason_ptr = reason.c_str();
    switch (systemType)
    {
        case MATE:
        case GNOME:
        {
            dbus_uint32_t xid    = 0;
            dbus_uint32_t gflags = 0xC;
            dbus_message_append_args(msg,
                DBUS_TYPE_STRING, &app_ptr,
                DBUS_TYPE_UINT32, &xid,
                DBUS_TYPE_STRING, &reason_ptr,
                DBUS_TYPE_UINT32, &gflags,
                DBUS_TYPE_INVALID);
            break;
        }
        default:
            dbus_message_append_args(msg,
                DBUS_TYPE_STRING, &app_ptr,
                DBUS_TYPE_STRING, &reason_ptr,
                DBUS_TYPE_INVALID);
            break;
    }

    DBusError dbus_error;
    dbus_error_init(&dbus_error);
    DBusMessage* dbus_reply = dbus_connection_send_with_reply_and_block(connection, msg, DBUS_TIMEOUT_USE_DEFAULT,
        &dbus_error);
    dbus_message_unref(msg);
    if (!dbus_reply)
    {
        Logger::error("DBus connection failed: {}/{}", dbus_error.name, dbus_error.message);
        dbus_error_free(&dbus_error);
        return 0;
    }

    uint32_t id               = 0;
    dbus_bool_t dbus_args_res = dbus_message_get_args(dbus_reply, &dbus_error, DBUS_TYPE_UINT32, &id,
        DBUS_TYPE_INVALID);
    dbus_message_unref(dbus_reply);
    if (!dbus_args_res)
    {
        Logger::error("DBus cannot parse replay: {}/{}", dbus_error.name, dbus_error.message);
        dbus_error_free(&dbus_error);
        return 0;
    }
    return id;
}

void dbusUnInhibit(DBusConnection* connection, uint32_t cookie)
{
    DBusMessage* msg = dbus_message_new_method_call(dbus_service[systemType],
        dbus_path[systemType],
        dbus_interface[systemType],
        dbus_method_uninhibit[systemType]);
    if (!msg)
    {
        Logger::error("DBus cannot create new method call: {};{};{};{}", dbus_service[systemType],
            dbus_path[systemType],
            dbus_interface[systemType],
            dbus_method_uninhibit[systemType]);
        return;
    }

    dbus_message_append_args(msg,
        DBUS_TYPE_UINT32, &cookie,
        DBUS_TYPE_INVALID);

    DBusError dbus_error;
    dbus_error_init(&dbus_error);
    DBusMessage* dbus_reply = dbus_connection_send_with_reply_and_block(connection, msg, DBUS_TIMEOUT_USE_DEFAULT,
        &dbus_error);
    dbus_message_unref(msg);
    if (!dbus_reply)
    {
        Logger::error("DBus connection failed: {}/{}", dbus_error.name, dbus_error.message);
        dbus_error_free(&dbus_error);
    }
}

static std::unique_ptr<DBusConnection, std::function<void(DBusConnection*)>> dbus_conn(connectSessionBus(),
    closeSessionBus);
#endif

DesktopPlatform::DesktopPlatform()
{
    // Theme
    char* themeEnv = getenv("BOREALIS_THEME");
    if (themeEnv == nullptr)
    {
#ifdef __APPLE__
        char buffer[10];
        memset(buffer, 0, sizeof buffer);
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("defaults read -g AppleInterfaceStyle", "r"), pclose);
        if (pipe)
        {
            fgets(buffer, sizeof buffer, pipe.get());
            if (strncmp(buffer, "Dark", 4) == 0)
            {
                this->themeVariant = ThemeVariant::DARK;
                brls::Logger::info("Set app theme: Dark");
            }
            else
            {
                brls::Logger::info("Set app theme: Light");
            }
        }
        else
        {
            brls::Logger::error("cannot get system theme");
        }
#endif
    }
    else if (!strcasecmp(themeEnv, "DARK"))
    {
        this->themeVariant = ThemeVariant::DARK;
    }

    // Locale
    if (Platform::APP_LOCALE_DEFAULT == LOCALE_AUTO)
    {
        char* langEnv = getenv("BOREALIS_LANG");
        this->locale  = langEnv ? std::string(langEnv) : LOCALE_DEFAULT;
    }
    else
    {
        this->locale = Platform::APP_LOCALE_DEFAULT;
    }
    brls::Logger::info("Set app locale: {}", this->locale);

    // Platform impls
    this->fontLoader = new DesktopFontLoader();
    this->imeManager = new DesktopImeManager();
}

bool DesktopPlatform::canShowBatteryLevel()
{
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

bool DesktopPlatform::canShowWirelessLevel()
{
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

int DesktopPlatform::getBatteryLevel()
{
#if defined(__APPLE__)
    std::string b = exec("pmset -g batt | grep -Eo '[0-9]+%'");
    if (!b.empty() && b[b.size() - 1] == '%')
    {
        b = b.substr(0, b.size() - 1);
    }
    int res = 100;
    try
    {
        res = stoi(b);
    }
    catch (...)
    {
        return 100;
    }
    return res;
#else
    return 100;
#endif
}

bool DesktopPlatform::isBatteryCharging()
{
#if defined(__APPLE__)
    std::string res = exec("pmset -g batt | grep -o 'AC Power'");
    return !res.empty();
#else
    return false;
#endif
}

bool DesktopPlatform::hasWirelessConnection()
{
#if defined(__APPLE__)
    std::string res = exec("networksetup -listallhardwareports | awk '/Wi-Fi/{getline; print $2}' | xargs networksetup -getairportpower | grep -o On");
    return !res.empty();
#else
    return true;
#endif
}

int DesktopPlatform::getWirelessLevel()
{
    return 3;
}

void DesktopPlatform::disableScreenDimming(bool disable, const std::string& reason, const std::string& app)
{
    if (this->screenDimmingDisabled == disable)
        return;
    this->screenDimmingDisabled = disable;

    if (disable)
    {
#ifdef __linux__
        inhibitCookie = dbusInhibit(dbus_conn.get(), app, reason);
#elif __APPLE__
        std::string sleepReason           = app + " " + reason;
        CFStringRef reasonForActivity     = ::CFStringCreateWithCString(kCFAllocatorDefault, sleepReason.c_str(),
                kCFStringEncodingUTF8);
        [[maybe_unused]] IOReturn success = IOPMAssertionCreateWithName(
            kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
            reasonForActivity, &assertionID);
#elif _WIN32
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
#endif
    }
    else
    {
#ifdef __linux__
        if (inhibitCookie != 0)
            dbusUnInhibit(dbus_conn.get(), inhibitCookie);
#elif __APPLE__
        IOPMAssertionRelease(assertionID);
#elif _WIN32
        SetThreadExecutionState(ES_CONTINUOUS);
#endif
    }
}

bool DesktopPlatform::isScreenDimmingDisabled()
{
    return this->screenDimmingDisabled;
}

std::string DesktopPlatform::getIpAddress()
{
#if defined(__APPLE__) || defined(__linux__)
    return exec("ifconfig | grep \"inet \" | grep -Fv 127.0.0.1 | awk '{print $2}' ");
#else
    return "-";
#endif
}

std::string DesktopPlatform::getDnsServer()
{
#if defined(__APPLE__)
    return exec("scutil --dns | grep nameserver | awk '{print $3}' | sort -u | paste -s -d',' -");
#else
    return "-";
#endif
}

std::string DesktopPlatform::exec(const char* cmd)
{
    std::string result;
#if defined(__APPLE__) || defined(__linux__)
    char buffer[128];
    memset(buffer, 0, sizeof buffer);
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr)
    {
        result += buffer;
    }
#endif

    if (!result.empty() && result[result.size() - 1] == '\n')
        result = result.substr(0, result.size() - 1);
    return result;
}

bool DesktopPlatform::isApplicationMode()
{
    return true;
}

void DesktopPlatform::exitToHomeMode(bool value)
{
    DesktopPlatform::RESTART_APP = !value;
}

void DesktopPlatform::forceEnableGamePlayRecording()
{
}

void DesktopPlatform::openBrowser(std::string url)
{
    brls::Logger::debug("open url: {}", url);
#if __APPLE__
    std::string cmd = "open \"" + url + "\"";
    system(cmd.c_str());
#elif __linux__
    std::string cmd = "xdg-open \"" + url + "\"";
    system(cmd.c_str());
#elif _WIN32
    shell_open(url.c_str());
#endif
}

std::string DesktopPlatform::getName()
{
    return "Desktop";
}

FontLoader* DesktopPlatform::getFontLoader()
{
    return this->fontLoader;
}

ImeManager* DesktopPlatform::getImeManager()
{
    return this->imeManager;
}

ThemeVariant DesktopPlatform::getThemeVariant()
{
    return this->themeVariant;
}

void DesktopPlatform::setThemeVariant(ThemeVariant theme)
{
    this->themeVariant = theme;
}

std::string DesktopPlatform::getLocale()
{
    return this->locale;
}

DesktopPlatform::~DesktopPlatform()
{
    delete this->fontLoader;
    delete this->imeManager;
}

} // namespace brls
