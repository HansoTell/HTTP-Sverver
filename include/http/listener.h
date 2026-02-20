#pragma once

#include <functional>
#include <steam/steamnetworkingsockets.h>
#include <stdint.h>
#include <thread>
#include <cassert>

#include "http/HTTPinitialization.h"
#include "http/Request.h"
#include "Datastrucutres/ThreadSaveQueue.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"

namespace http{

struct SocketHandlers {
    HSteamListenSocket m_Socket;
    HSteamNetPollGroup m_PollGroup;
};

class Listener {
public:
    ThreadSaveQueue<Request> m_RecivedMessegas;
    ThreadSaveQueue<Request> m_OutgoingMessages;
    //kann invalid poll group error on listener enthalten
    ThreadSaveQueue<Error::ErrorValue<HTTPErrors>> m_ErrorQueue;
public:
    Result<SocketHandlers> initSocket( u_int16_t port );
    void startListening();
    void stopListening();
public:
    Listener(ISteamNetworkingSockets* interface, std::function<void(HSteamListenSocket, HSteamNetConnection)> ConnectionServedCallback);
    Listener(const Listener& other) = delete;
    Listener(Listener&& other) = delete;
    ~Listener();
private:
    void listen();

    void DestroySocket();

    void pollIncMessages();
    void pollOutMessages();
private:
    ISteamNetworkingSockets* m_pInterface;
    HSteamListenSocket m_Socket;
    HSteamNetPollGroup m_pollGroup;

    std::thread m_ListenThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_listening;
    std::mutex m_ListenMutex;
    std::condition_variable m_ListenCV;
    std::function<void(HSteamListenSocket, HSteamNetConnection)>  m_ConnectionServedCallback;
};

}

