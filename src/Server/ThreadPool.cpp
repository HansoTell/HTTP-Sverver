#include "ThreadPool.h"

namespace http {

    ThreadPool::ThreadPool( u_int32_t numWorkers ) : m_Workers(numWorkers), m_running(true) {
        for( std::thread& worker : m_Workers)
            worker = std::thread( workerLoop );
    }

    ThreadPool::~ThreadPool() {
        m_running = false;

        m_tasksCV.notify_all();

        for( std::thread& worker : m_Workers ){
            if( worker.joinable() )
                worker.join();
        } 
    }


    void ThreadPool::assignTask( std::function<void()> task ) {
        {
            std::lock_guard<std::mutex> _lock (m_tasksMutex);
            m_Tasks.push(std::move(task));
        }

        m_tasksCV.notify_one();
    }

    void ThreadPool::workerLoop(){
        std::unique_lock<std::mutex> _lock (m_tasksMutex);
        while ( m_running ) {
            std::function<void()> task;

            m_tasksCV.wait(_lock, [this](){
                return !m_Tasks.empty() || !m_running;
            });

            if( !m_running )
                break;

            task = std::move(m_Tasks.front());

            m_Tasks.pop();
                

            _lock.unlock();
            task();            
        }
    }
}