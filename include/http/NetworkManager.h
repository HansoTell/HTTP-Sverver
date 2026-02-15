#pragma once

#include <memory>
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <sys/types.h>
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


//Eine sache die ich dneke also soweit ich weiß laufen callbacks auch für einzelne sockets.
//Sollte man für jeden socket möglichkeit haben optionen zu geben? Dann individueller Callback oder so wäre idee. Dann würde es satic method im listener geben
//die called dann networkmanager calback methode mit optionen struct oder int oder so
//Auf jeden fall überlegen individuell die sich selbst verwalten zu alssen
//
//Was fehlt neues start callback mechanismus
//handler mechanismus
//Listener methoden auf hier mappen das von hier aufgerufen werden können
//im server so bauen das es nur über ihn hier funktioniert
//vielleicht umbauen dass callbacks und so in listener und destroy socket ev umbauen es könnte einfacher sein so
//Zudem haben keine start lsiening methode nur eine die auch socket erstellt...
//alle aufurfe aus listener entfernen und equally ersetzten
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

class NetworkManager{
public:
    static NetworkManager& Get(){
        static NetworkManager instance;   
        return instance;
    }

    void init();
    void kill();
    
    HListener createListener();
    Result<void> DestroyListener( HListener listener );

    Result<void> startListening( HListener listener, u_int16_t port);
    Result<void> stopListening( HListener listener );
    template<typename T>
    ThreadSaveQueue<T>* getQueue( HListener listener, QueueType queuetype);


    void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo );

    //Entfernen
    void startCallbacksIfNeeded();

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

    std::unordered_map<HListener, std::unique_ptr<Listener>> m_Listeners;
    u_int64_t m_ListenerHandlerIndex = HListener_Invalid;
};

}
