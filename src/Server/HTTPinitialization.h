#pragma once


#include <steam/steamnetworkingsockets.h>

#include "../Error/Error.h"
#include "../Error/Errorcodes.h"
#include "../Listener/NetworkManager.h"
#include "../Log/Logger.h"


#define LOG_DEBUG(msg) http::g_Logger.log(Log::LogLevel::INFO, msg, CURRENT_LOCATION_LOG)
#define LOG_INFO(msg) http::g_Logger.log(Log::LogLevel::INFO, msg, CURRENT_LOCATION_LOG)
#define LOG_WARNING(msg) http::g_Logger.log(Log::LogLevel::WARNING, msg, CURRENT_LOCATION_LOG)
#define LOG_ERROR(msg) http::g_Logger.log(Log::LogLevel::ERROR, msg, CURRENT_LOCATION_LOG)
#define LOG_CRITICAL(msg) http::g_Logger.log(Log::LogLevel::CRITICAL, msg, CURRENT_LOCATION_LOG)

namespace http{

template<typename T>
using Result = Error::Result<T, HTTPErrors>;

extern Log::Logger g_Logger;

extern bool isHTTPInitialized;

//steam library initen und so
extern bool initHTTP();

//was erschaffen wird muss auch getötet werden
extern bool HTTP_Kill();

//blockende methode die nach nutzereingaben guckt und die dann an den server als commands weitergibt einfach in die message queue gepackt iwie so
extern void listenForCommands();


}