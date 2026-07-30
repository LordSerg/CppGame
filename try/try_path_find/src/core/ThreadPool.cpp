#include "core/ThreadPool.h"
#include <algorithm>

ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
    }

    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    condition_.wait(lock, [this]() {
                        return stop_ || !tasks_.empty();
                    });
                    if (stop_ && tasks_.empty()) return;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    condition_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
}

void ThreadPool::parallelFor(int count, const std::function<void(int)>& func) {
    if (count <= 0) return;

    int numThreads = (int)workers_.size();
    if (count <= numThreads || numThreads == 0) {
        // Not worth threading for small counts
        for (int i = 0; i < count; ++i) func(i);
        return;
    }

    // Split work into chunks
    int chunkSize = (count + numThreads) / (numThreads + 1);
    std::atomic<int> remaining{0};

    std::vector<std::future<void>> futures;
    futures.reserve(numThreads + 1);

    int start = 0;
    while (start < count) {
        int end = std::min(start + chunkSize, count);
        if (start == 0) {
            // First chunk runs on calling thread later
            start = end;
            continue;
        }
        int s = start, e = end;
        futures.push_back(submit([&func, s, e]() {
            for (int i = s; i < e; ++i) func(i);
        }));
        start = end;
    }

    // Calling thread does first chunk
    int firstEnd = std::min(chunkSize, count);
    for (int i = 0; i < firstEnd; ++i) func(i);

    // Wait for all worker chunks
    for (auto& f : futures) f.get();
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(mutex_);
    allDone_.wait(lock, [this]() {
        return tasks_.empty() && activeTasks_ == 0;
    });
}