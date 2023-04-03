#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

namespace tiny_csv {

template<typename T>
class TaskQueue {
public:
    TaskQueue(int numThreads) : stop_(false) {
        for (int i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(this->mtx_);
                    this->condition_.wait(lock, [this] { return this->stop_.load() || !this->tasks_.empty(); });
                    if (this->stop_.load() && this->tasks_.empty()) {
                        return;
                    }
                    auto task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                    lock.unlock();
                    task.first(task.second);
                }
            });
        }
    }

    ~TaskQueue() {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            stop_.store(true);
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            worker.join();
        }
    }

    void Enqueue(std::function<void(T)>&& f, const T& args) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            tasks_.push({f, args });
        }
        condition_.notify_one();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::pair<std::function<void(T)>, T>> tasks_;
    std::mutex mtx_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

}