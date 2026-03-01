#pragma once

#include <steam/steamnetworkingsockets.h>
#include <memory>

#include "Error/Errorcodes.h"
#include "Error/Error.h"
#include "Logger/Logger.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/steamclientpublic.h"
#include "steam/steamnetworkingtypes.h"

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

class ISteamNetworkinSocketsAdapter {
public:
    virtual EResult AcceptConnection( HSteamNetConnection ) = 0;
    virtual bool SetConnectionPollGroup( HSteamNetConnection, HSteamNetPollGroup ) = 0;
    virtual bool CloseConnection( HSteamNetConnection, int, const char*, bool ) = 0;
    virtual void RunCallbacks() = 0;

    virtual HSteamNetPollGroup CreatePollGroup() = 0;
    virtual HSteamListenSocket CreateListenSocketIP( const SteamNetworkingIPAddr &localAddress, int nOptions, const SteamNetworkingConfigValue_t *pOptions ) = 0;
    virtual int ReceiveMessagesOnPollGroup( HSteamNetPollGroup hPollGroup, SteamNetworkingMessage_t **ppOutMessages, int nMaxMessages ) = 0; 
    virtual EResult SendMessageToConnection( HSteamNetConnection hConn, const void *pData, uint32 cbData, int nSendFlags, int64 *pOutMessageNumber ) = 0;   
	virtual bool CloseListenSocket( HSteamListenSocket hSocket ) = 0;
	virtual bool DestroyPollGroup( HSteamNetPollGroup hPollGroup ) = 0;
};

class SteamNetworkingSocketsAdapter : public ISteamNetworkinSocketsAdapter {
public:
    SteamNetworkingSocketsAdapter( ISteamNetworkingSockets* real ) : m_Real(real) {}

    EResult AcceptConnection( HSteamNetConnection hConn ) override { return m_Real->AcceptConnection( hConn ); } 
    bool SetConnectionPollGroup( HSteamNetConnection hConn, HSteamNetPollGroup hPollGroup ) override { return m_Real->SetConnectionPollGroup(hConn, hPollGroup); } 
    bool CloseConnection( HSteamNetConnection hPeer, int nReason, const char* pszDebug, bool bEnableLinger ) override { return m_Real->CloseConnection( hPeer, nReason, pszDebug, bEnableLinger ); }
    void RunCallbacks() override { m_Real->RunCallbacks(); }

    HSteamNetPollGroup CreatePollGroup() override { return m_Real->CreatePollGroup(); }
    HSteamListenSocket CreateListenSocketIP( const SteamNetworkingIPAddr &localAddress, int nOptions, const SteamNetworkingConfigValue_t *pOptions ) override { return m_Real->CreateListenSocketIP( localAddress, nOptions, pOptions ); }
    int ReceiveMessagesOnPollGroup( HSteamNetPollGroup hPollGroup, SteamNetworkingMessage_t **ppOutMessages, int nMaxMessages ) override { return m_Real->ReceiveMessagesOnPollGroup(hPollGroup, ppOutMessages, nMaxMessages); } 
    EResult SendMessageToConnection( HSteamNetConnection hConn, const void *pData, uint32 cbData, int nSendFlags, int64 *pOutMessageNumber ) override { return m_Real->SendMessageToConnection(hConn, pData, cbData, nSendFlags, pOutMessageNumber); }   
	bool CloseListenSocket( HSteamListenSocket hSocket ) override { return m_Real->CloseListenSocket(hSocket); }
	bool DestroyPollGroup( HSteamNetPollGroup hPollGroup ) override { return m_Real->DestroyPollGroup(hPollGroup); }
private:
    ISteamNetworkingSockets* m_Real;
};
}
