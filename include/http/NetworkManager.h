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
#include "steam/isteamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"


//im server so bauen das es nur über ihn hier funktioniert
namespace http{

#define HListener u_int64_t
#define HListener_Invalid 0

enum QueueType {
    ERROR = 0, RECEIVED = 1, OUTGOING = 2
};

struct Connections{
    HSteamNetConnection m_connection;
    bool isServed = false;
};

//auch die idee ist halt doof weil in listener würden wir ja shcon gerne socket verwalten weil von listner wissen wir nicht ob aktiv oder nicht
//und haben halt niergendwo wo wir wirklich socket haben
//In AllClients nicht mehr den vector sondern SocketInfo speichern
struct SocketInfo{
    std::vector<Connections> m_AllConnections = std::vector<Connections>(128);
    HSteamNetPollGroup m_PollGroup = k_HSteamNetPollGroup_Invalid;


    SocketInfo(HSteamNetPollGroup pollGroup) : m_PollGroup(pollGroup){}
};

struct ListenerInfo {
    std::unique_ptr<IListener> m_Listener = nullptr;
    char ListenerName[512] = "";
    HSteamListenSocket m_Socket = k_HSteamListenSocket_Invalid;
    //das dann halt hier entfernen auch wenns weh tut
    HSteamNetPollGroup m_PollGroup = k_HSteamNetPollGroup_Invalid;

    ListenerInfo( std::unique_ptr<Listener> listener) 
        : m_Listener(std::move(listener)) {} 
};


class NetworkManagerCore {
public:
    //können bestimmt public los werden
    std::unordered_map<HSteamListenSocket, SocketInfo> m_SocketClientsMap;
public:
    HListener createListener( const char* ListenerName );
    Result<void> DestroyListener( HListener listener );

    Result<void> startListening( HListener listener, u_int16_t port );
    Result<void> stopListening( HListener listener );
    template<typename T>
    Result<ThreadSaveQueue<T>*> getQueue( HListener listener, QueueType queuetype);
    void ConnectionServed( HSteamListenSocket socket , HSteamNetConnection connection );

    void pollConnectionChanges();
    void pollFunctionCalls( ThreadSaveQueue<std::function<void()>>* functionQueue );
    
    void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo );
public:
    NetworkManagerCore( ISteamNetworkingSockets* interface );
    NetworkManagerCore(const NetworkManagerCore& other) = delete;
    NetworkManagerCore(NetworkManagerCore&& other) = delete;
    ~NetworkManagerCore();
private:
    Result<void> isValidListenerHandler( HListener listenerHandel ) const;

    void Connecting( SteamNetConnectionStatusChangedCallback_t *pInfo );
    void Disconnected( SteamNetConnectionStatusChangedCallback_t *pInfo );

private:
    std::unordered_map<HListener, ListenerInfo> m_Listeners;

    u_int64_t m_ListenerHandlerIndex = HListener_Invalid;
    ISteamNetworkingSockets* m_pInterface = nullptr;
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
    Result<ThreadSaveQueue<T>*> getQueue( HListener listener, QueueType queuetype);
    void ConnectionServed( HSteamListenSocket socket, HSteamNetConnection connection );

    void runCallbacks( SteamNetConnectionStatusChangedCallback_t* pInfo ) { m_Core->callbackManager( pInfo ); }

public:
    static void sOnConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo ) { NetworkManager::Get().runCallbacks( pInfo ); } 
    static void sConnectionServedCallback( HSteamListenSocket socket, HSteamNetConnection connection ) { NetworkManager::Get().ConnectionServed( socket, connection ); }
public:
    ISteamNetworkingSockets* m_pInterface = nullptr;
private:

    void tick();
    void run();

    template<typename Funktion>
    auto executeFunktion(Funktion&& func) -> decltype(func());

    void notifyFunktionCall();
private:
    std::atomic<bool> m_running { true };
    std::atomic<bool> m_Busy { true };
    std::mutex m_ManagerMutex;
    std::condition_variable m_callbackCV;
    std::thread m_NetworkThread;


    ThreadSaveQueue<std::function<void()>> m_FunctionCalls;
    std::unique_ptr<NetworkManagerCore> m_Core;
};
}
