/*
Copyright 2026 xfangfang

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

#include <borealis/platforms/ps5/ps5_ime.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <borealis/core/logger.hpp>
#include <borealis/core/thread.hpp>

namespace ps5ime
{

// The dialog's description of what it should ask for. The layout is the one
// the console's system libraries take: the same struct serves the PS4 build of
// this library, and the console runs PS4 titles through the same dialog. The
// trailing room is zeroed and only there so that a console that knows more
// fields reads zeros rather than whatever follows this struct.
struct ImeDialogSetting
{
    uint32_t userId;
    int32_t type;
    uint64_t supportedLanguages;
    int32_t enterLabel;
    int32_t inputMethod;
    void* filter;
    uint32_t option;
    uint32_t maxTextLength;
    wchar_t* inputTextBuffer;
    float posx;
    float posy;
    int32_t horizontalAlignment;
    int32_t verticalAlignment;
    const wchar_t* placeholder;
    const wchar_t* title;
    int8_t reserved[64];
};

struct ImeDialogResult
{
    int32_t endstatus;
    int8_t reserved[12];
};

} // namespace ps5ime

// The keyboard is a system library, imported like the pad and the audio
// output: the console loads it when the title starts and writes its addresses
// into this image.
extern "C"
{
    int sceImeDialogInit(const ps5ime::ImeDialogSetting* setting, void* reserved);
    int sceImeDialogGetStatus(void);
    int sceImeDialogGetResult(ps5ime::ImeDialogResult* result);
    int sceImeDialogTerm(void);
    int sceUserServiceGetInitialUser(int32_t* userId);
    int sceSysmoduleLoadModule(uint16_t id);
}

// The console's keyboard dialog is its own system module, and unlike the pad it
// is not one a title starts with: the app asks for it the first time a field is
// used. Loading it is what makes the console write the addresses above into
// this image, so the imports are read back from the slots the build recorded
// before they are called - an import the loader did not fill holds zero, and a
// call through it would jump to zero.
// The linked image's text segment, which is where the import slots are counted
// from. Only the title has one; the platform's own binary leaves it undefined.
extern const char __wiliwili_text_start[] __attribute__((weak));

namespace ps5ime
{
// The title build replaces these with the slots it read from the linked image
// (see scripts/ps5/title/build_title.sh). What is here only satisfies the
// platform's own link and answers "unknown" to the checks below.
__attribute__((weak)) extern const uint64_t kImportSlots[5] = { 0, 0, 0, 0, 0 };
__attribute__((weak)) extern const char* const kImportNames[5] = {
    "sceImeDialogInit", "sceImeDialogGetStatus", "sceImeDialogGetResult",
    "sceImeDialogTerm", "sceUserServiceGetInitialUser"
};
__attribute__((weak)) extern const uint64_t kTextOffset = UINT64_MAX;
} // namespace ps5ime

namespace brls
{

namespace
{

// The keyboard layouts the console offers.
constexpr int kTypeDefault = 0;
constexpr int kTypeNumber  = 4;

constexpr int kAlignHorizontalCenter = 1;
constexpr int kAlignVerticalBottom   = 2;

constexpr int kStatusNone    = 0;
constexpr int kStatusRunning = 1;
constexpr int kStatusStopped = 2;

constexpr int kEndStatusOk = 0;

// The console's module for the keyboard dialog, the same one its own settings
// screens use.
constexpr uint16_t kKeyboardModule = 0x0096;

// The system writes UTF-16; the console's wchar_t is two bytes wide, so the
// two are the same thing here.
constexpr size_t kMaxTextLength  = 512;
constexpr size_t kMaxTitleLength = 128;

uint16_t inputText[kMaxTextLength + 1];
uint16_t titleText[kMaxTitleLength];
uint16_t placeholderText[kMaxTitleLength];

// An import that the console did not bind holds zero.
bool importResolved(int index)
{
    if (__wiliwili_text_start == nullptr || ps5ime::kImportSlots[index] == 0 ||
        ps5ime::kTextOffset == UINT64_MAX)
        return false;
    const uintptr_t base =
        reinterpret_cast<uintptr_t>(__wiliwili_text_start) - (uintptr_t)ps5ime::kTextOffset;
    return *(volatile uint64_t*)(base + ps5ime::kImportSlots[index]) != 0;
}

/**
 * Make sure the console's dialog can be called at all. Asking for the module is
 * what makes the console bind the imports above; a console that answers that
 * the module is already there has done the same, so it is the slots, not the
 * answer, that decide.
 */
bool keyboardReady()
{
    static int state = -1;
    if (state >= 0)
        return state == 1;

    const int rc = sceSysmoduleLoadModule(kKeyboardModule);
    Logger::info("the console's keyboard module: rc=0x{:08x}", (uint32_t)rc);

    bool ready = true;
    for (int i = 0; i < 4; i++) {
        if (!importResolved(i)) {
            Logger::error("the console's keyboard is unresolved: {}", ps5ime::kImportNames[i]);
            ready = false;
        }
    }
    state = ready ? 1 : 0;
    return ready;
}

// The user the keyboard is opened for: it decides which languages and which
// layouts the dialog offers. Reading it is best effort.
uint32_t currentUser()
{
    static int32_t user = -1;
    if (user < 0) {
        user = 0;
        if (importResolved(4) && sceUserServiceGetInitialUser(&user) != 0)
            user = 0;
    }
    return (uint32_t)user;
}

void utf8ToUtf16(const std::string& source, uint16_t* destination, size_t capacity)
{
    size_t used = 0;
    for (size_t i = 0; i < source.size() && used + 1 < capacity;) {
        const unsigned char lead = (unsigned char)source[i];
        uint32_t code;
        size_t extra;
        if (lead < 0x80) {
            code  = lead;
            extra = 0;
        } else if ((lead & 0xE0) == 0xC0) {
            code  = lead & 0x1F;
            extra = 1;
        } else if ((lead & 0xF0) == 0xE0) {
            code  = lead & 0x0F;
            extra = 2;
        } else if ((lead & 0xF8) == 0xF0) {
            code  = lead & 0x07;
            extra = 3;
        } else {
            ++i;
            continue;
        }
        if (i + extra >= source.size())
            break;

        bool complete = true;
        for (size_t k = 1; k <= extra; ++k) {
            const unsigned char next = (unsigned char)source[i + k];
            if ((next & 0xC0) != 0x80) {
                complete = false;
                break;
            }
            code = (code << 6) | (next & 0x3F);
        }
        i += extra + 1;
        if (!complete)
            continue;

        if (code >= 0x10000) {
            if (used + 2 >= capacity)
                break;
            code -= 0x10000;
            destination[used++] = (uint16_t)(0xD800 + (code >> 10));
            destination[used++] = (uint16_t)(0xDC00 + (code & 0x3FF));
        } else {
            destination[used++] = (uint16_t)code;
        }
    }
    destination[used] = 0;
}

std::string utf16ToUtf8(const uint16_t* source)
{
    std::string text;
    for (size_t i = 0; source[i]; ++i) {
        uint32_t code = source[i];
        if (code >= 0xD800 && code <= 0xDBFF) {
            const uint16_t low = source[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                ++i;
            }
        }

        if (code < 0x80) {
            text += (char)code;
        } else if (code < 0x800) {
            text += (char)(0xC0 | (code >> 6));
            text += (char)(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            text += (char)(0xE0 | (code >> 12));
            text += (char)(0x80 | ((code >> 6) & 0x3F));
            text += (char)(0x80 | (code & 0x3F));
        } else {
            text += (char)(0xF0 | (code >> 18));
            text += (char)(0x80 | ((code >> 12) & 0x3F));
            text += (char)(0x80 | ((code >> 6) & 0x3F));
            text += (char)(0x80 | (code & 0x3F));
        }
    }
    return text;
}

/**
 * Follow the dialog from the main loop. The console draws the keyboard itself,
 * so there is nothing to render here: the loop only asks whether it is still
 * open. Polling instead of waiting inside the call keeps the application's own
 * frames going while the player types, and keeps the field's callback on the
 * thread that owns the interface.
 */
void waitForInput(std::function<void(std::string)> callback)
{
    struct Poll
    {
        int frames = 0;
        bool shown = false;
    };
    auto poll = std::make_shared<Poll>();

    brls::sync([callback, poll]() {
        const int status = sceImeDialogGetStatus();

        if (status == kStatusRunning) {
            poll->shown = true;
            waitForInput(callback);
            return;
        }

        if (status != kStatusStopped && !poll->shown) {
            // Opening the dialog is not instantaneous; give the console a few
            // seconds to put it on screen before calling the attempt a failure.
            if (++poll->frames < 300) {
                waitForInput(callback);
                return;
            }
            Logger::error("the console's keyboard never opened");
            sceImeDialogTerm();
            return;
        }

        ps5ime::ImeDialogResult result {};
        const int rc = sceImeDialogGetResult(&result);
        sceImeDialogTerm();

        Logger::info("keyboard closed: rc=0x{:08x} end={}", (uint32_t)rc, (int)result.endstatus);
        if (rc == 0 && result.endstatus == kEndStatusOk)
            callback(utf16ToUtf8(inputText));
    });
}

} // namespace

bool Ps5ImeManager::openSystemDialog(std::function<void(std::string)> callback, int type,
    const std::string& headerText, const std::string& subText, int maxStringLength,
    const std::string& initialText)
{
    if (!keyboardReady())
        return false;

    // Nothing should be on screen: a dialog left behind by an earlier field
    // would make the console refuse this one.
    const int status = sceImeDialogGetStatus();
    if (status == kStatusRunning || status == kStatusStopped) {
        Logger::warning("the console's keyboard is still open: status={}", status);
        sceImeDialogTerm();
    }

    const size_t limit = (size_t)std::clamp(maxStringLength, 1, (int)kMaxTextLength);

    utf8ToUtf16(initialText, inputText, limit + 1);
    utf8ToUtf16(headerText, titleText, kMaxTitleLength);
    utf8ToUtf16(subText, placeholderText, kMaxTitleLength);

    ps5ime::ImeDialogSetting setting {};
    setting.userId              = currentUser();
    setting.type                = type;
    setting.maxTextLength       = (uint32_t)limit;
    setting.inputTextBuffer     = reinterpret_cast<wchar_t*>(inputText);
    setting.posx                = 1000;
    setting.posy                = 1000;
    setting.horizontalAlignment = kAlignHorizontalCenter;
    setting.verticalAlignment   = kAlignVerticalBottom;
    setting.placeholder         = reinterpret_cast<const wchar_t*>(placeholderText);
    setting.title               = reinterpret_cast<const wchar_t*>(titleText);

    const int rc = sceImeDialogInit(&setting, nullptr);
    if (rc != 0) {
        Logger::error("the console's keyboard did not open: 0x{:08x} (user={} status={})",
            (uint32_t)rc, setting.userId, status);
        return false;
    }

    Logger::info("the console's keyboard is open: user={}", setting.userId);
    waitForInput(std::move(callback));
    return true;
}

bool Ps5ImeManager::openForText(std::function<void(std::string)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText, int kbdDisableBitmask)
{
    if (openSystemDialog(f, kTypeDefault, headerText, subText, maxStringLength, initialText))
        return true;
    return this->fallback.openForText(f, headerText, subText, maxStringLength, initialText, kbdDisableBitmask);
}

bool Ps5ImeManager::openForNumber(std::function<void(long)> f, std::string headerText,
    std::string subText, int maxStringLength, std::string initialText,
    std::string leftButton, std::string rightButton, int kbdDisableBitmask)
{
    auto accept = [f](const std::string& text) {
        if (text.empty())
            return;
        try {
            f(std::stoll(text));
        } catch (const std::exception& e) {
            Logger::error("Could not parse input, did you enter a valid integer? {}", e.what());
        }
    };

    if (openSystemDialog(accept, kTypeNumber, headerText, subText, maxStringLength, initialText))
        return true;
    return this->fallback.openForNumber(f, headerText, subText, maxStringLength, initialText,
        leftButton, rightButton, kbdDisableBitmask);
}

} // namespace brls
