#pragma once

#include <steam/steamnetworkingsockets.h>
#include <stdint.h>
#include <thread>
#include <cassert>

#include "../Server/HTTPinitialization.h"
#include "../Server/Queues.h"

namespace http{

class Listener {
public:
    Listener();
    Listener(const Listener& other) = delete;
    Listener(Listener&& other) = delete;
    ~Listener();
public:
    Result<void> startListening( u_int16_t port, const char* socketName );
    void stopListening();
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
    MessageQueues* m_Queues; 

    std::thread m_ListenThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_listening;
    std::mutex m_ListenMutex;
    std::condition_variable m_ListenCV;
};

}

