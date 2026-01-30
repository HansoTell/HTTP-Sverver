#pragma once

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include<unordered_set>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

#include "../Error/Error.h"
#include "../Error/Errorcodes.h"
#include "../Server/HTTPinitialization.h"


//Eine sache die ich dneke also soweit ich weiß laufen callbacks auch für einzelne sockets.
//Sollte man für jeden socket möglichkeit haben optionen zu geben? Dann individueller Callback oder so wäre idee. Dann würde es satic method im listener geben
//die called dann networkmanager calback methode mit optionen struct oder int oder so
namespace http{
class NetworkManager{
public:
    static NetworkManager& Get(){
        static NetworkManager instance;   
        return instance;
    }

    void init();

    void kill();

    void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo );

    void startCallbacksIfNeeded();

    void notifySocketCreation( HSteamListenSocket createdSocket );
    void notifySocketDestruction( HSteamListenSocket destroyedSocket );

    Result<const std::vector<HSteamNetConnection>&> getClientList( HSteamListenSocket socket) const { if(m_SocketClientsMap.find(socket) != m_SocketClientsMap.end()){ return m_SocketClientsMap.at(socket); }else { MAKE_ERROR(HTTPErrors::eInvalidSocket, "No Such SOcket found"); } }

public:
    static void OnConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t *pInfo ){ NetworkManager::Get().callbackManager(pInfo); }
public:
    ISteamNetworkingSockets* m_pInterface = nullptr;
private:
    void pollConnectionChanges();
private:
    bool m_Connections_open = true;
    std::mutex m_connection_lock;
    std::mutex m_callbackMutex;
    std::condition_variable m_callbackCV;
    std::thread m_CallBackThread;
    std::atomic<bool> m_running { true };

    std::unordered_map<HSteamListenSocket, std::vector<HSteamNetConnection>> m_SocketClientsMap;
};

}