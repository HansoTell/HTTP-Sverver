#pragma once

#include <queue>
#include <thread>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>


namespace http{
    
    class ThreadPool {
    public:
        void assignTask(std::function<void()> task);

    public:
        ThreadPool( u_int32_t numWorkers );
        ThreadPool(const ThreadPool& other) = delete;
        ThreadPool(ThreadPool&& other) = delete;
        ~ThreadPool();
    private:
        void workerLoop();
    private:
        std::atomic<bool> m_running;
        std::queue<std::function<void()>> m_Tasks;
        std::vector<std::thread> m_Workers;
        std::mutex m_tasksMutex;
        std::condition_variable m_tasksCV;
    };
}