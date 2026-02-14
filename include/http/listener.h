#pragma once

#include <steam/steamnetworkingsockets.h>
#include <stdint.h>
#include <thread>
#include <cassert>

#include "http/HTTPinitialization.h"
#include "http/Request.h"
#include "Datastrucutres/ThreadSaveQueue.h"
#include "steam/isteamnetworkingsockets.h"

namespace http{

class Listener {
public:
    ThreadSaveQueue<Request> m_RecivedMessegas;
    ThreadSaveQueue<Request> m_OutgoingMessages;
    //kann invalid poll group error on listener enthalten
    ThreadSaveQueue<Error::ErrorValue<HTTPErrors>> m_ErrorQueue;
public:
    Result<void> startListening( u_int16_t port, const char* socketName );
    void stopListening();
public:
    Listener();
    Listener(ISteamNetworkingSockets* interface);
    Listener(const Listener& other) = delete;
    Listener(Listener&& other) = delete;
    ~Listener();
private:
    void listen();

    Result<void> initSocket( u_int16_t port, const char* socketName );
    void DestroySocket();

    void pollIncMessages();
    void pollOutMessages();
private:
    ISteamNetworkingSockets* m_pInterface;
    HSteamListenSocket m_Socket;
    char m_SocketName[512];
    HSteamNetPollGroup m_pollGroup;

    std::thread m_ListenThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_listening;
    std::mutex m_ListenMutex;
    std::condition_variable m_ListenCV;
};

}

