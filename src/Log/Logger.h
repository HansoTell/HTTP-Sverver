#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace Log{

    enum LogLevel{
        DEBUG = 0, INFO, WARNING, ERROR, CRITICAL
    };

    class Logger{
        public:
        Logger(const std::string& file){
            m_logFile.open(file, std::ios::app);
            if(!m_logFile.is_open())
                std::cerr << "Failed to open Log file" << "\n";
            
        }
        ~Logger(){
            if(m_logFile.is_open())
                m_logFile.close();
        }
        public:
        void setLogLevel(LogLevel level){ m_Loglevel = level; }
        void log(LogLevel logLevel, const std::string& message) {
            if(logLevel < m_Loglevel)
                return;

            auto now = std::chrono::system_clock::now();
            std::time_t logTime = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&logTime);            

            std::ostringstream logentry;
            logentry << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " << levelToString(logLevel) << ": " << message << "\n";

            if( m_logFile.is_open() ){
                m_logFile << logentry.str();
                m_logFile.flush();
            }            
        }


        private:
        const char* levelToString(LogLevel logLevel) const {
            switch ( logLevel ){
                case DEBUG: return "DEBUG"; 
                case INFO: return "INFO";
                case ERROR: return "ERROR";
                case WARNING: return "WARNING";
                case CRITICAL: return "CRITICAL";
            }

        }

        private:
        std::ofstream m_logFile;
        LogLevel m_Loglevel = LogLevel::INFO;
    };

}