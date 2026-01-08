#pragma once

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include<unordered_set>
#include <condition_variable>

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

    void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo );

    void startCallbacksIfNeeded();

    void notifySocketCreation( HSteamListenSocket createdSocket );
    void notifySocketDestruction( HSteamListenSocket destroyedSocket );
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
    std::unordered_set<HSteamListenSocket> m_all_sockets;
    //erstmal keine date und distinction von conenctions brauchen eig hier nicht, dann map 
    std::unordered_set<HSteamNetConnection> m_all_connections;
};

}