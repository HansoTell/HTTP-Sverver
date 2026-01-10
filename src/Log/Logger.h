#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>

#define CURRENT_LOCATION ::Log::SourceLocation{__FILE__, __func__, __LINE__}
namespace Log{

    enum LogLevel{
        DEBUG = 0, INFO, WARNING, ERROR, CRITICAL
    };


    struct SourceLocation {
        const char* File;
        const char* Function;
        int line;
    };

    inline std::ostream& operator<<(std::ostream& os, const SourceLocation& location){ return os << location.File << ":" << location.line << " " << location.Function; }

    class Logger{
        public:
        Logger(const std::string& file){
            m_logFile.open(file, std::ios::app);
            if(!m_logFile.is_open())
                std::cerr << "Failed to open Log file" << "\n";

            m_LogThread = std::thread ( logThread );
        }
        ~Logger(){

            m_running = false;
            m_cv.notify_all();
            if( m_LogThread.joinable() )
                m_LogThread.join();

            if(m_logFile.is_open())
                m_logFile.close();
        }
        public:
        void setLogLevel(LogLevel level){ m_Loglevel = level; }
        void log(LogLevel logLevel, const std::string& message, SourceLocation location) {
            if(logLevel < m_Loglevel)
                return;

            auto now = std::chrono::system_clock::now();
            std::time_t logTime = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&logTime);            

            std::ostringstream logentry;
            logentry << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " << levelToString(logLevel) << ": " << "["<< location << "] " << message << "\n";

            {
                std::lock_guard<std::mutex> __lock (m_queueMutex);
                m_MessageQueue.push(logentry.str());
            }

            m_cv.notify_one();
        }


        private:
        std::string levelToString(LogLevel logLevel) const {
            switch ( logLevel ){
                case DEBUG: return "DEBUG"; 
                case INFO: return "INFO";
                case ERROR: return "ERROR";
                case WARNING: return "WARNING";
                case CRITICAL: return "CRITICAL";
            }
        }

        void logThread(){
            while( m_running || m_MessageQueue.empty() ){
                std::unique_lock<std::mutex> __lock (m_queueMutex);
                m_cv.wait(__lock, [&]{
                    return !m_MessageQueue.empty() || !m_running;
                });

                while( !m_MessageQueue.empty() ){
                    m_logFile << m_MessageQueue.front();
                    m_MessageQueue.pop();
                }
                m_logFile.flush();
            }
        }

        private:
        std::ofstream m_logFile;
        LogLevel m_Loglevel = LogLevel::INFO;

        std::mutex m_queueMutex;
        std::queue<std::string> m_MessageQueue;
        std::condition_variable m_cv;
        std::thread m_LogThread;
        std::atomic<bool> m_running {true};
    };

}