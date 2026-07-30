#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = 0);
    ~ThreadPool();

    // Submit a task and get a future
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    // Submit a batch of work: calls func(i) for i in [0, count) across threads
    void parallelFor(int count, const std::function<void(int)>& func);

    // Wait for all submitted tasks to complete
    void waitAll();

    size_t threadCount() const { return workers_.size(); }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    std::atomic<int> activeTasks_{0};
    std::condition_variable allDone_;
};

template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    using ReturnType = decltype(f(args...));
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<ReturnType> result = task->get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.emplace([task, this]() {
            activeTasks_++;
            (*task)();
            activeTasks_--;
            allDone_.notify_all();
        });
    }
    condition_.notify_one();
    return result;
}