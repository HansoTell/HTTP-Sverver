#pragma once

#include <functional>
#include <memory>
#include <steam/steamnetworkingsockets.h>
#include <stdint.h>
#include <sys/types.h>
#include <thread>
#include <cassert>

#include "Error/Error.h"
#include "http/HTTPinitialization.h"
#include "http/Request.h"
#include "Datastrucutres/ThreadSaveQueue.h"
#include "steam/steamnetworkingtypes.h"

namespace http{

struct SocketHandlers {
    HSteamListenSocket m_Socket;
    HSteamNetPollGroup m_PollGroup;
};


class IListener {
public:
    virtual ~IListener() = default;
    virtual Result<SocketHandlers> initSocket( u_int16_t port ) = 0;
    virtual void startListening() = 0;
    virtual void stopListening() = 0;

    virtual ThreadSaveQueue<Request>* getReceivedQueue() = 0;
    virtual ThreadSaveQueue<Request>* getOutgoingQueue() = 0;
    virtual ThreadSaveQueue<Error::ErrorValue<HTTPErrors>>* getErrorQueue() = 0;

};



class Listener : public IListener {
public:
    Result<SocketHandlers> initSocket( u_int16_t port ) override;
    void startListening() override;
    void stopListening() override;

    ThreadSaveQueue<Request>* getReceivedQueue() override { return &m_RecivedMessegas; }
    ThreadSaveQueue<Request>* getOutgoingQueue() override { return &m_OutgoingMessages; }
    ThreadSaveQueue<Error::ErrorValue<HTTPErrors>>* getErrorQueue() override { return &m_ErrorQueue; }
public:
    Listener( std::shared_ptr<ISteamNetworkinSocketsAdapter> interface, std::function<void(HSteamListenSocket, HSteamNetConnection)> ConnectionServedCallback);
    Listener(const Listener& other) = delete;
    Listener(Listener&& other) = delete;
    ~Listener();
private:
    void listen();

    void DestroySocket();

    void pollIncMessages();
    void pollOutMessages();
private:
    std::shared_ptr<ISteamNetworkinSocketsAdapter> m_pInterface;
    HSteamListenSocket m_Socket;
    HSteamNetPollGroup m_pollGroup;

    std::thread m_ListenThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_listening;
    std::mutex m_ListenMutex;
    std::condition_variable m_ListenCV;
    std::function<void(HSteamListenSocket, HSteamNetConnection)>  m_ConnectionServedCallback;

    //Queues
    ThreadSaveQueue<Request> m_RecivedMessegas;
    ThreadSaveQueue<Request> m_OutgoingMessages;
    //kann invalid poll group error on listener enthalten
    ThreadSaveQueue<Error::ErrorValue<HTTPErrors>> m_ErrorQueue;
};

class IListenerFactory {
public:
    virtual ~IListenerFactory() = default;
    virtual std::unique_ptr<IListener> createListener() = 0;
};

class ListenerFactory : public IListenerFactory {
public:
    ListenerFactory( std::shared_ptr<ISteamNetworkinSocketsAdapter> pInterface, std::function<void(HSteamListenSocket, HSteamNetConnection)>  ConnectionServedCallback ) 
        : m_pInterface(pInterface), m_ConnectionServedCallback(ConnectionServedCallback){}

    std::unique_ptr<IListener> createListener() override { return std::make_unique<Listener>(m_pInterface, m_ConnectionServedCallback); }
private:
    std::shared_ptr<ISteamNetworkinSocketsAdapter> m_pInterface;
    std::function<void(HSteamListenSocket, HSteamNetConnection)>  m_ConnectionServedCallback;
};
}

