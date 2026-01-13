#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <string_view>
#include <cstring>
#include <algorithm>
#include <format>
#include <system_error>

#define CURRENT_LOCATION_LOG ::Log::SourceLocation{__FILE__, __func__, __LINE__}

#ifndef NDEBUG
#define DEBUG(msg) Log(Log::LogLevel::DEBUG, msg, CURRENT_LOCATION_LOG)
#define VDEBUG(...) var_log(Log::LogLevel::DEBUG, CURRENT_LOCATION_LOG, __VA_ARGS__)
#else
#define DEBUG(msg) 
#define VDEBUG(...)
#endif

#define INFO(msg) log(Log::LogLevel::INFO, msg, CURRENT_LOCATION_LOG)
#define VINFO(...) var_Log(Log::LogLevel::INFO, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define WARNING(msg) log(Log::LogLevel::WARNING, msg, CURRENT_LOCATION_LOG)
#define VWARNING(...) var_Log(Log::LogLevel::WARNING, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define ERROR(msg) log(Log::LogLevel::ERROR, msg, CURRENT_LOCATION_LOG)
#define VERROR(...) var_Log(Log::LogLevel::ERROR, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define CRITICAL(msg) log(Log::LogLevel::CRITICAL, msg, CURRENT_LOCATION_LOG)
#define VCRITICAL(...) var_Log(Log::LogLevel::CRITICAL, CURRENT_LOCATION_LOG, __VA_ARGS__)

#define MAXLOGSIZE 5*1024*1024


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
            m_logFile.open(file, std::ios::app) : m_logPath(file);
            if(!m_logFile.is_open())
                std::cerr << "Failed to open Log file" << "\n";

            m_LogThread = std::thread ( [this](){ this->logThread(); } );
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

        template<typename ... Args> 
        void var_Log(LogLevel logLevel, SourceLocation location, Args&&... args){
            if( logLevel < m_Loglevel )
                return;

            char buff[32];            
            currentTimetoString(buff);

            std::string logEntry = createDeafultEntry(buff, logLevel, location);

            (addMessageToString(logEntry, std::forward<Args>(args)), ...);

            logEntry.append("\n");
            logEntry.shrink_to_fit();

            addToMessageQueue(std::move(logEntry));

            m_cv.notify_one();
        }

        void log(LogLevel logLevel, const std::string_view message, SourceLocation location) {
            if(logLevel < m_Loglevel)
                return;

            char buff[32];
            currentTimetoString(buff);

            std::string logEntry = createDeafultEntry(buff, logLevel, location);
            logEntry.append(message.data()).append("\n");
            logEntry.shrink_to_fit();


            addToMessageQueue(std::move(logEntry));

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
                default: return "NON TYPE";
            }
        }

        void logThread(){
            while( m_running || !m_MessageQueue.empty() ){
                std::unique_lock<std::mutex> __lock (m_queueMutex);
                m_cv.wait(__lock, [&]{
                    return !m_MessageQueue.empty() || !m_running;
                });

                while( !m_MessageQueue.empty() ){

                    changeLogFileIfNeeded();

                    m_logFile << m_MessageQueue.front();
                    m_MessageQueue.pop();
                }
                m_logFile.flush();
            }
        }

        void currentTimetoString(char* dest){
            std::time_t logTime = std::time(nullptr);
            std::strncpy(dest, std::ctime(&logTime), 24);
            dest[24] = '\0';
        }

        std::string createDeafultEntry(const char* time, LogLevel logLevel, const SourceLocation& location){
            std::string logEntry;
            logEntry.reserve(256);
            logEntry.append("[").append(time).append("]")
                    .append(levelToString(logLevel))
                    .append(": [").append(location.File).append(":").append(std::to_string(location.line)).append(" ").append(location.Function).append("]")
                    .append("] ");
            return logEntry;
        }

        template<typename T>
        void addMessageToString(std::string& string, T&& message){ 
            appendToLog(string, message);
            string.append(" ");
        }


        void addToMessageQueue(std::string&& logEntry){
            std::lock_guard<std::mutex> __lock (m_queueMutex);
            m_MessageQueue.push(std::move(logEntry));
        }

        void changeLogFileIfNeeded(){
            if( !std::filesystem::exists(m_logPath) )
                return;
            
            if( std::filesystem::file_size(m_logPath) < MAXLOGSIZE)
                return;

            if( m_logFile.is_open() )
                m_logFile.close();

            std::time_t currTime = std::time(nullptr);
            char timeBuffer[std::size("yyyy-mm-ddThh:mm:ssZ")];
            std::strftime(timeBuffer, std::size(timeBuffer), "%FT%TZ", std::gmtime(&currTime));
            
            std::filesystem::path newPath = m_logPath;
            (newPath += ".") += timeBuffer;

            std::error_code ec;
            std::filesystem::rename(m_logPath, newPath, ec);

            m_logFile.open(m_logPath, std::ios::app);

            if( !m_logFile.is_open() )
                std::cerr << "Couldt open new Log File after rotating" << "\n";

            if( ec ){
                VERROR("Failed to Rename File", ec.message());
            }
        } 

        private:
        std::ofstream m_logFile;
        std::string m_logPath;
        LogLevel m_Loglevel = LogLevel::INFO;

        std::mutex m_queueMutex;
        std::queue<std::string> m_MessageQueue;
        std::condition_variable m_cv;
        std::thread m_LogThread;
        std::atomic<bool> m_running {true};
    };
}