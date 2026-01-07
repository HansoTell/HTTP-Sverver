#pragma once

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>


namespace http{

class NetworkManager{

public:
    static NetworkManager& Get(){
        static NetworkManager instance;   
        return instance;
    }

    void init();

    void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo );

    //müssen wissen wann man srtate (muss schon scoket geben und wenn einmal keine gab dann muss wieder gestartet werden)
    void pollConnectionChanges();
public:
    static void OnConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t *pInfo ){ NetworkManager::Get().callbackManager(pInfo); }
public:
    ISteamNetworkingSockets* m_pInterface = nullptr;
private:
    bool m_Connections_open = true;
    std::mutex m_connection_lock;
//also speichern von connections und sockets und wenn beide leer sind dann wird aufgehört zu pollen
//wass wenn man nur temporär aufhört zu listen? später nachdenken-->iguess wenn socket erstellt wird checken ob läuft und wenn nicht thread starten und laufne lassen und detachen
};

}