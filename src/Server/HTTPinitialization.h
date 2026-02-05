#pragma once


#include <steam/steamnetworkingsockets.h>
#include <memory>

#include "../Error/Error.h"
#include "../Error/Errorcodes.h"
#include "../Log/Logger.h"

#ifndef NDEBUG
#define LOGLEVEL Log::LogLevel::DEBUG
#define LOG_DEBUG(msg) http::g_Logger->log(Log::LogLevel::DEBUG, msg, CURRENT_LOCATION_LOG)
#define LOG_VDEBUG(...) http::g_Logger->VDEBUG(__VA_ARGS__)
#else
#define LOGLEVEL Log:LogLevel::INFO
#define LOG_DEBUG(msg)  
#define LOG_VDEBUG(...) 
#endif

#define LOG_INFO(msg) http::g_Logger->log(Log::LogLevel::INFO, msg, CURRENT_LOCATION_LOG)
#define LOG_WARNING(msg) http::g_Logger->log(Log::LogLevel::WARNING, msg, CURRENT_LOCATION_LOG)
#define LOG_ERROR(msg) http::g_Logger->log(Log::LogLevel::ERROR, msg, CURRENT_LOCATION_LOG)
#define LOG_CRITICAL(msg) http::g_Logger->log(Log::LogLevel::CRITICAL, msg, CURRENT_LOCATION_LOG)

#define LOG_VINFO(...) http::g_Logger->VINFO(__VA_ARGS__)
#define LOG_VWARNING(...) http::g_Logger->VWARNING(__VA_ARGS__)
#define LOG_VERROR(...) http::g_Logger->VERROR(__VA_ARGS__)
#define LOG_VCRITICAL(...) http::g_Logger->VCRITICAL(__VA_ARGS__)

#define MAKE_ERROR(code, message) ::Error::make_error<http::HTTPErrors>(code, message) 

namespace http{

template<typename T>
using Result = Error::Result<T, HTTPErrors>;

inline std::unique_ptr<Log::Logger> g_Logger;

extern bool isHTTPInitialized;

//steam library initen und so
extern bool initHTTP();

//was erschaffen wird muss auch getötet werden
extern bool HTTP_Kill();

//blockende methode die nach nutzereingaben guckt und die dann an den server als commands weitergibt einfach in die message queue gepackt iwie so
extern void listenForCommands();
}
