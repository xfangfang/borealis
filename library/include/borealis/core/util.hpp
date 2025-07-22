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

#pragma once

#include <borealis/core/logger.hpp>
#include <stdexcept>

namespace brls
{
bool endsWith(const std::string& data, const std::string& suffix);

bool startsWith(const std::string& data, const std::string& prefix);
/**
 * Prints the given error message message and throws a std::logic_error.
 */
[[noreturn]] void fatal(std::string message);

std::string loadFileContents(const std::string& path);

namespace utils {

#define _BR_CAT(x, y) x ## y
#define  BR_CAT(x, y) _BR_CAT(x, y)
#define BR_ANONYMOUS BR_CAT(var, __COUNTER__)
#define DEFER(f) auto BR_ANONYMOUS = ::brls::utils::_Defer(f)

template <typename F>
struct _Defer {
    [[nodiscard]] _Defer(F &&f): f(std::move(f)) { }

    _Defer(const _Defer &) = delete;
    _Defer &operator =(const _Defer &) = delete;

    ~_Defer() {
        if (this->want_run)
            this->f();
    }

    void cancel() {
        this->want_run = false;
    }

    private:
        bool want_run = true;
        F f;
};

} // namespace utils
} // namespace brls
