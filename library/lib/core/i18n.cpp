/*
    Copyright 2020-2021 natinusala

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
#include <borealis/core/assets.hpp>
#include <borealis/core/i18n.hpp>
#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

#ifndef BRLS_I18N_PREFIX
#define BRLS_I18N_PREFIX ""
#endif

namespace brls
{

static nlohmann::json defaultLocale = {};
static nlohmann::json currentLocale = {};

static bool endsWith(const std::string& str, const std::string& suffix)
{
    // if I wanted to write my own endsWith I would have made borealis in PHP
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static void loadLocale(std::string locale, nlohmann::json* target)
{
    if (locale.empty())
        return;
#ifdef USE_LIBROMFS
    auto localePath = romfs::list("i18n/" + locale);
    if (localePath.empty())
    {
        Logger::error("Cannot load locale {}: directory i18n/{} doesn't exist", locale, locale);
        return;
    }
    for (auto& entry : localePath)
    {
        std::string path = entry.string();
        std::string name = entry.filename().string();
        if (!endsWith(name, ".json"))
            continue;

        (*target)[name.substr(0, name.length() - 5)] = nlohmann::json::parse(romfs::get(path).string());
    }
#else
    std::string localePath = BRLS_ASSET("i18n/" + locale);

    if (!fs::exists(localePath))
    {
        Logger::error("Cannot load locale {}: directory {} doesn't exist", locale, localePath);
        return;
    }
    else if (!fs::is_directory(localePath))
    {
        Logger::error("Cannot load locale {}: {} isn't a directory", locale, localePath);
        return;
    }

    // Iterate over all JSON files in the directory
    for (const fs::directory_entry& entry : fs::directory_iterator(localePath))
    {
#if USE_BOOST_FILESYSTEM
        if (fs::is_directory(entry))
#else
        if (entry.is_directory())
#endif
            continue;

        std::string name = entry.path().filename().string();

        if (!endsWith(name, ".json"))
            continue;

        std::string path = entry.path().string();

        nlohmann::json strings;

        std::ifstream jsonStream;
        jsonStream.open(path);

        try
        {
            jsonStream >> strings;
        }
        catch (const std::exception& e)
        {
            Logger::error("Error while loading \"{}\": {}", path, e.what());
        }

        jsonStream.close();

        (*target)[name.substr(0, name.length() - 5)] = strings;
    }
#endif /* USE_LIBROMFS */
}

void loadTranslations()
{
    loadLocale(LOCALE_DEFAULT, &defaultLocale);

    std::string currentLocaleName = Application::getLocale();
    if (currentLocaleName != LOCALE_DEFAULT)
        loadLocale(currentLocaleName, &currentLocale);
}

void loadTranslations(const std::string& locale) {
    loadLocale(LOCALE_DEFAULT, &defaultLocale);
    if(locale != LOCALE_DEFAULT)
        loadLocale(locale, &currentLocale);
}

namespace internal
{
    std::string getRawStr(std::string stringName)
    {
        nlohmann::json::json_pointer pointer;

        try
        {
            pointer = nlohmann::json::json_pointer("/" + std::string(BRLS_I18N_PREFIX) + stringName);
        }
        catch (const std::exception& e)
        {
            Logger::error("Error while getting string \"{}\": {}", stringName, e.what());
            return stringName;
        }

        // First look for translated string in current locale
        try
        {
            return currentLocale[pointer].get<std::string>();
        }
        catch (...)
        {
        }

        // Then look for default locale
        try
        {
            return defaultLocale[pointer].get<std::string>();
        }
        catch (...)
        {
        }

        // Fallback to returning the string name
        return stringName;
    }
} // namespace internal

inline namespace literals
{
    std::string operator"" _i18n(const char* str, size_t len)
    {
        return internal::getRawStr(std::string(str, len));
    }

} // namespace literals

} // namespace brls
