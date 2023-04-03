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
    TaskQueue(int numThreads) : stop(false) {
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    this->condition.wait(lock, [this] { return this->stop.load() || !this->tasks.empty(); });
                    if (this->stop.load() && this->tasks.empty()) {
                        return;
                    }
                    auto task = std::move(this->tasks.front());
                    this->tasks.pop();
                    lock.unlock();
                    task.first(task.second);
                }
            });
        }
    }

    ~TaskQueue() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            stop.store(true);
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

    void Enqueue(std::function<void(T)>&& f, const T& args) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            tasks.push({ f, args });
        }
        condition.notify_one();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::pair<std::function<void(T)>, T>> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};

}