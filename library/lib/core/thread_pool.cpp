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

#include <borealis/core/thread_pool.hpp>
#include <borealis/core/logger.hpp>

namespace brls {

// Create global ThreadPool with default amount of threads (probably should be configurable)
ThreadPool* ThreadPool::_global = new ThreadPool(8);

ThreadPool::ThreadPool(int threads) : shutdown_(false) {
    // Create the specified number of threads
    threads_.reserve(threads);
    for (int i = 0; i < threads; ++i)
        threads_.emplace_back([this, i] { threadEntry(i); });
}

ThreadPool::~ThreadPool() {
    {
        // Unblock any threads and tell them to stop
        std::unique_lock<std::mutex> l(lock_);

        shutdown_ = true;
        condVar_.notify_all();
    }

    // Wait for all threads to stop
    brls::Logger::info("Joining threads");
    for (auto &thread: threads_)
        thread.join();
}

void ThreadPool::async(std::function<void(void)> func) {
    // Place a job on the queue and unblock a thread
    std::unique_lock<std::mutex> l(lock_);

    jobs_.emplace(std::move(func));
    condVar_.notify_one();
}

void ThreadPool::threadEntry(int i) {
    std::function<void(void)> job;

    while (true) {
        {
            std::unique_lock<std::mutex> l(lock_);

            while (!shutdown_ && jobs_.empty())
                condVar_.wait(l);

            if (jobs_.empty()) {
                // No jobs to do and we are shutting down
                brls::Logger::info("Thread {} terminates", i);
                return;
            }

//            brls::Logger::info("Thread {} does a job", i);
            job = std::move(jobs_.front());
            jobs_.pop();
        }

        // Do the job without holding any locks
        job();
    }

}

} // namespace brls
