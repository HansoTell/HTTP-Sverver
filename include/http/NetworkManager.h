#pragma once

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


//Listener methoden auf hier mappen das von hier aufgerufen werden können
//im server so bauen das es nur über ihn hier funktioniert
//vielleicht umbauen dass callbacks und so in listener und destroy socket ev umbauen es könnte einfacher sein so
//alle aufurfe aus listener entfernen und equally ersetzten
//sollten auch nicht zurfrieden sein mit den map, also weil blockiert alles scalet schlecht
//keine ahnung wie man die map thread sicher macht also man kann einach nie entfernen das wäre möglich --> würde fehler
//bringen wie wenn man halt eine alte listener nummer imemr noch funktioniert
//
//
//Desing : Brauchen einen teil der public alle methoden aufruft kann, die aber nur die entsprechenden funktion pointer ablegt
//Bstandteil der die funktion pointer besitzt und damit auch die interne logik sowie die callback funktionen
//Brauchen Thread bestandteil mit tick funktion die die poll Methoden aufruft
//
//also brauchen public Singelkton als Thread teil -> braucht die public aufrufbaren methoden
//core teil -> braucht die logik und interna
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

struct ListenerInfo {
    std::unique_ptr<Listener> m_Listener = nullptr;
    char ListenerName[512] = "";
    HSteamListenSocket m_Socket = k_HSteamListenSocket_Invalid;
    HSteamNetPollGroup m_PollGroup = k_HSteamNetPollGroup_Invalid;
    std::mutex test();

    ListenerInfo( std::unique_ptr<Listener> listener) 
        : m_Listener(std::move(listener)) {} 
};


class NetworkManagerCore {
public:
    //sollten schon so machen dass da reserved wird an platz geht ja so nicht
    //wie setzten wird eigentlich served haben ja gar keine idee davon
    //können bestimmt public los werden
    std::unordered_map<HSteamListenSocket, std::vector<Connections>> m_SocketClientsMap;
public:
    HListener createListener( const char* ListenerName );
    Result<void> DestroyListener( HListener listener );

    Result<void> startListening( HListener listener, u_int16_t port);
    Result<void> stopListening( HListener listener );
    template<typename T>
    Result<ThreadSaveQueue<T>*> getQueue( HListener listener, QueueType queuetype);

    void pollConnectionChanges();
    void pollFunctionCalls();
    
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
    ThreadSaveQueue<T>* getQueue( HListener listener, QueueType queuetype);

    //auch entfernen
    void notifySocketCreation( HSteamListenSocket createdSocket, HSteamNetPollGroup pollGroup );
    void notifySocketDestruction( HSteamListenSocket destroyedSocket );
    const std::vector<HSteamNetConnection>* getClientList( HSteamListenSocket socket) const; 

    void runCallbacks( SteamNetConnectionStatusChangedCallback_t* pInfo ) { m_Core->callbackManager( pInfo ); }

public:
    static void sOnConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo ) { NetworkManager::Get().runCallbacks( pInfo ); } 
public:
    ISteamNetworkingSockets* m_pInterface = nullptr;
private:

    void tick();
    void run();

private:
    std::atomic<bool> m_running { true };
    std::atomic<bool> m_Busy { true };
    std::mutex m_ManagerMutex;
    std::condition_variable m_callbackCV;
    std::thread m_NetworkThread;


    ThreadSaveQueue<std::string> m_FunctionCalls;
    std::unique_ptr<NetworkManagerCore> m_Core;
};

}
