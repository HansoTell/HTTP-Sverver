#pragma once

#include <functional>
#include <future>
#include <memory>
#include <optional>
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
#include "Error/Error.h"
#include "http/HTTPinitialization.h"
#include "http/Request.h"
#include "http/listener.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"


namespace http{

#define HListener u_int64_t
#define HListener_Invalid 0


struct Connections{
    HSteamNetConnection m_connection;
    bool isServed = false;
};

struct SocketInfo{
    std::vector<Connections> m_AllConnections = std::vector<Connections>();
    HSteamNetPollGroup m_PollGroup = k_HSteamNetPollGroup_Invalid;


    SocketInfo(HSteamNetPollGroup pollGroup) : m_PollGroup(pollGroup){ m_AllConnections.reserve(128); }
};

struct ListenerInfo {
    std::unique_ptr<IListener> m_Listener = nullptr;
    char ListenerName[512] = "";
    HSteamListenSocket m_Socket = k_HSteamListenSocket_Invalid;

    ListenerInfo( std::unique_ptr<IListener> listener) 
        : m_Listener(std::move(listener)) {} 
};


class INetworkManagerCore {
public:
    virtual ~INetworkManagerCore() = default;
    virtual HListener createListener( const char* ListenerName ) = 0;
    virtual Result<void> DestroyListener( HListener listener ) = 0;
    virtual Result<void> startListening( HListener listener, u_int16_t port ) = 0;
    virtual Result<void> stopListening( HListener listener ) = 0;
    virtual Result<std::optional<Request>> try_PoPReceivedMessageQueue( HListener listener ) = 0;
    virtual Result<void> push_OutgoingMessageQueue( HListener listener, Request message ) = 0;
    virtual Result<std::optional<Error::ErrorValue<HTTPErrors>>> try_PoPErrorQueue( HListener listener ) = 0;
    virtual void ConnectionServed( HSteamListenSocket socket , HSteamNetConnection connection ) = 0;
    virtual void pollConnectionChanges() = 0;
    virtual void pollFunctionCalls( ThreadSaveQueue<std::function<void()>>* functionQueue ) = 0;
    virtual void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo ) = 0;
    virtual bool isSocketClientsMapEmpty() = 0;
};


class NetworkManagerCore : public INetworkManagerCore {
public:
    HListener createListener( const char* ListenerName ) override;
    Result<void> DestroyListener( HListener listener ) override;

    Result<void> startListening( HListener listener, u_int16_t port ) override;
    Result<void> stopListening( HListener listener ) override;

    Result<std::optional<Request>> try_PoPReceivedMessageQueue( HListener listener ) override;
    Result<void> push_OutgoingMessageQueue( HListener listener, Request message ) override;
    Result<std::optional<Error::ErrorValue<HTTPErrors>>> try_PoPErrorQueue( HListener listener ) override;
    bool isSocketClientsMapEmpty() override { return m_SocketClientsMap.empty(); }

    void ConnectionServed( HSteamListenSocket socket , HSteamNetConnection connection ) override;
    void pollConnectionChanges() override;
    void pollFunctionCalls( ThreadSaveQueue<std::function<void()>>* functionQueue ) override;
    
    void callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo ) override;
public:
    //Methodes only for Testing
    const auto& getSocketClientsMap() const { return m_SocketClientsMap; }
    const auto& getListenerMap() const { return m_Listeners; }
public:
    NetworkManagerCore( std::shared_ptr<ISteamNetworkinSocketsAdapter> interface, std::unique_ptr<IListenerFactory> listenerFactory );
    NetworkManagerCore(const NetworkManagerCore& other) = delete;
    NetworkManagerCore(NetworkManagerCore&& other) = delete;
    ~NetworkManagerCore();
private:
    Result<void> isValidListenerHandler( HListener listenerHandel ) const;

    void Connecting( SteamNetConnectionStatusChangedCallback_t *pInfo );
    void Disconnected( SteamNetConnectionStatusChangedCallback_t *pInfo );
private:
    std::unordered_map<HListener, ListenerInfo> m_Listeners;
    std::unordered_map<HSteamListenSocket, SocketInfo> m_SocketClientsMap;

    u_int64_t m_ListenerHandlerIndex = HListener_Invalid;
    std::shared_ptr<ISteamNetworkinSocketsAdapter> m_pInterface;
    std::unique_ptr<IListenerFactory> m_ListenerFactory;
};

class NetworkManager{
public:
    static NetworkManager& Get(){
        static NetworkManager instance;   
        return instance;
    }

    Result<void> init( std::unique_ptr<INetworkManagerCore> core, std::shared_ptr<ISteamNetworkinSocketsAdapter> pInterface );
    void kill();
    
    HListener createListener( const char* ListenerName );
    Result<void> DestroyListener( HListener listener );
    Result<void> startListening( HListener listener, u_int16_t port);
    Result<void> stopListening( HListener listener );
    Result<std::optional<Request>> try_PoPReceivedMessageQueue( HListener listener );
    Result<void> push_OutgoingMessageQueue( HListener listener, Request message );
    Result<std::optional<Error::ErrorValue<HTTPErrors>>> try_PoPErrorQueue( HListener listener );
    void ConnectionServed( HSteamListenSocket socket, HSteamNetConnection connection );
    void runCallbacks( SteamNetConnectionStatusChangedCallback_t* pInfo ) { m_Core->callbackManager( pInfo ); }
public:
    static void sOnConnectionStatusChangedCallback( SteamNetConnectionStatusChangedCallback_t* pInfo ) { NetworkManager::Get().runCallbacks( pInfo ); } 
    static void sConnectionServedCallback( HSteamListenSocket socket, HSteamNetConnection connection ) { NetworkManager::Get().ConnectionServed( socket, connection ); }
public:
    //Test methoden
    bool isThreadJoinable() const { return m_NetworkThread.joinable(); }
    bool isRunning() const { return m_running; }
    bool isInitialized() const { return m_initialized; }
private:
    void tick();
    void run();

    template<typename Funktion>
    auto executeFunktion(Funktion&& func) ->std::invoke_result_t<Funktion>{

        using returnVal = std::invoke_result_t<Funktion>;

        auto prommisedVal = std::make_shared<std::promise<returnVal>>();

        std::future<returnVal> future = prommisedVal->get_future();

        m_FunctionCalls.push([func = std::forward<Funktion>(func), prommisedVal]() mutable {
            prommisedVal->set_value(func());
        });
        notifyFunktionCall();
        
        return future.get();
    }

    void notifyFunktionCall();
private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_Busy { true };
    std::mutex m_ManagerMutex;
    std::condition_variable m_callbackCV;
    std::thread m_NetworkThread;

    ThreadSaveQueue<std::function<void()>> m_FunctionCalls;
    std::unique_ptr<INetworkManagerCore> m_Core;
    std::shared_ptr<ISteamNetworkinSocketsAdapter> m_pInterface;
    bool m_initialized = false;
};
}
