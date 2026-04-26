/*
    Copyright 2025 XITRIX

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

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace brls {

class ThreadPool {
public:
    explicit ThreadPool(int threads);
    ~ThreadPool();

    void async(std::function<void(void)> func);

    static ThreadPool* global() { return _global; }
private:

    void threadEntry(int i);

    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    std::queue<std::function<void(void)>> jobs_;
    std::vector<std::thread> threads_;

    static ThreadPool* _global;
};

} // namespace brls