#pragma once

#include <functional>
#include <memory>
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

#include "Datastrucutres/ThreadSaveQueue.h"
#include "http/HTTPinitialization.h"
#include "http/listener.h"
#include "steam/steamnetworkingtypes.h"


//Listener methoden auf hier mappen das von hier aufgerufen werden können
//im server so bauen das es nur über ihn hier funktioniert
//vielleicht umbauen dass callbacks und so in listener und destroy socket ev umbauen es könnte einfacher sein so
//alle aufurfe aus listener entfernen und equally ersetzten
//sollten auch nicht zurfrieden sein mit den map, also weil blockiert alles scalet schlecht
//keine ahnung wie man die map thread sicher macht also man kann einach nie entfernen das wäre möglich --> würde fehler
//bringen wie wenn man halt eine alte listener nummer imemr noch funktioniert
namespace http{

#define HListener u_int64_t
#define HListener_Invalid 0

enum QueueType {
    ERROR = 0, RECEIVED = 1, OUTGOING = 2
};

struct SocketInfo {
    HSteamNetPollGroup m_PollGroup;
    std::vector<HSteamNetConnection> m_Clients;   

    SocketInfo(HSteamNetConnection pollGroup) : m_PollGroup(pollGroup), m_Clients(64){}
    SocketInfo(const SocketInfo& other) = default;
    SocketInfo(SocketInfo&& other) : m_PollGroup(other.m_PollGroup), m_Clients(std::move(other.m_PollGroup)) {}
    ~SocketInfo() = default;
};

struct ListenerInfo {
    std::unique_ptr<Listener> m_Listener = nullptr;
    char ListenerName[512] = "";
    HSteamListenSocket m_Socket = k_HSteamListenSocket_Invalid;
    HSteamNetPollGroup m_PollGroup = k_HSteamNetPollGroup_Invalid;
    std::vector<HSteamNetConnection> m_Clients = std::vector<HSteamNetConnection>(64);
    std::mutex test();

    ListenerInfo( std::unique_ptr<Listener> listener) 
        : m_Listener(std::move(listener)) {} 
};


class NetworkManager{
public:
    static NetworkManager& Get(){
        static NetworkManager instance;   
        return instance;
    }

    void init();
    void kill();
    
    HListener createListener( const char* ListenerName );
    Result<void> DestroyListener( HListener listener );

    Result<void> startListening( HListener listener, u_int16_t port);
    Result<void> stopListening( HListener listener );
    template<typename T>
    ThreadSaveQueue<T>* getQueue( HListener listener, QueueType queuetype);


    void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo );

    //auch entfernen
    void notifySocketCreation( HSteamListenSocket createdSocket, HSteamNetPollGroup pollGroup );
    void notifySocketDestruction( HSteamListenSocket destroyedSocket );

    //auch entfernen
    const std::vector<HSteamNetConnection>* getClientList( HSteamListenSocket socket) const; 

public:
    static void OnConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t *pInfo ){ NetworkManager::Get().callbackManager(pInfo); }
public:
    ISteamNetworkingSockets* m_pInterface = nullptr;
private:
    void pollConnectionChanges();
    void pollFunctionCalls();

    void Connecting( SteamNetConnectionStatusChangedCallback_t *pInfo );
    void Disconnected( SteamNetConnectionStatusChangedCallback_t *pInfo );

    Result<void> isValidListenerHandler( HListener listenerHandel ) const;
private:
    bool m_Connections_open = true;
    std::mutex m_connection_lock;
    std::mutex m_callbackMutex;
    std::condition_variable m_callbackCV;
    std::thread m_CallBackThread;
    std::atomic<bool> m_running { true };

    std::unordered_map<HSteamListenSocket, SocketInfo> m_SocketClientsMap;

    //Wie functions wrappen
    ThreadSaveQueue<std::string> m_FunctionCalls;
    std::unordered_map<HListener, ListenerInfo> m_Listeners;
    u_int64_t m_ListenerHandlerIndex = HListener_Invalid;
};

}
