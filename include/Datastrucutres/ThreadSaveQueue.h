#pragma once

#include <condition_variable>
#include <cstddef>
#include <optional>
#include <queue>
#include <mutex>
#include <utility>

namespace http {

template<typename T> 
class ThreadSaveQueue {
public:
    bool empty() const {
        std::lock_guard<std::mutex> _lock (m_QueueMutex);
        return m_Queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> _lock (m_QueueMutex);
        return m_Queue.size();
    }

    void push(T item){
        {
            std::lock_guard<std::mutex> _lock (m_QueueMutex);
            m_Queue.push(std::move(item));
        }
        m_QueueCV.notify_one();
    }

    std::optional<T> try_pop(){
        std::lock_guard<std::mutex>_lock (m_QueueMutex);
        if( m_Queue.empty() )
            return std::nullopt;
        T item = std::move(m_Queue.front());
        m_Queue.pop();

        return item;
    }

    T wait_pop(){
        std::lock_guard<std::mutex> _lock(m_QueueMutex);
        m_QueueCV.wait(_lock, [this](){
            return !m_Queue.empty();
        });


        T item = std::move(m_Queue.front());
        m_Queue.pop();
        return  item;
    }
public:
    ThreadSaveQueue() {}
    ThreadSaveQueue(const ThreadSaveQueue& other) = delete;
    ThreadSaveQueue(ThreadSaveQueue&& other) = delete;
    ~ThreadSaveQueue() {}
private:
    std::queue<T> m_Queue;
    mutable std::mutex m_QueueMutex;
    std::condition_variable m_QueueCV;
};
}
