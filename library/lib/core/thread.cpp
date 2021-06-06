/*
    Copyright 2021 XITRIX

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

#include <borealis/core/thread.hpp>
#include <libretro-common/retro_timers.h>

namespace brls {

static pthread_t task_loop_thread;

Threading::Threading()
{
    start_task_loop();
}

void sync(const std::function<void()> &func)
{
    Threading::sync(func);
}

void async(const std::function<void()> &task)
{
    Threading::async(task);
}

void Threading::sync(const std::function<void()> &func) {
    std::lock_guard<std::mutex> guard(m_sync_mutex);
    m_sync_functions.push_back(func);
}

void Threading::async(const std::function<void()> &task) {
    std::lock_guard<std::mutex> guard(m_async_mutex);
    m_async_tasks.push_back(task);
}

void Threading::performSyncTasks()
{
    m_sync_mutex.lock();
    auto local = m_sync_functions;
    m_sync_functions.clear();
    m_sync_mutex.unlock();
    
    for (auto &f : local)
        f();
}

void Threading::start() {
    start_task_loop();
}

void Threading::stop() {
    task_loop_active = false;
    pthread_join(task_loop_thread, NULL);
}

void* Threading::task_loop(void* a) {
    while (task_loop_active) {
        std::vector<std::function<void()>> m_tasks_copy; {
            std::lock_guard<std::mutex> guard(m_async_mutex);
            m_tasks_copy = m_async_tasks;
            m_async_tasks.clear();
        }
        
        for (auto task: m_tasks_copy) {
            task();
        }
        
        retro_sleep(500);
    }
    return NULL;
}

void Threading::start_task_loop() {
    pthread_create(&task_loop_thread, NULL, task_loop, NULL);
}

} // namespace brls
