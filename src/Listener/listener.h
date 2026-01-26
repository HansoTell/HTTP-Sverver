#pragma once

#include <steam/steamnetworkingsockets.h>
#include <stdint.h>
#include <chrono>
#include <thread>

#include "NetworkManager.h"
#include "../Server/HTTPinitialization.h"
#include "../Error/Errorcodes.h"
#include "../Server/Queues.h"

namespace http{

class Listener {
public:
    Listener();
    Listener(const Listener& other) = delete;
    Listener(Listener&& other) = default;
    ~Listener();
public:
    Result<void> startListening( uint16 port );
    void stopListening();
private:
    void listen();

    Result<void> initSocket( u_int16_t port);
    void DestroySocket();


    void pollIncMessages();
    void pollOutMessages();
private:
    HSteamListenSocket m_Socket;
    HSteamNetPollGroup m_pollGroup;
    MessageQueues* m_Queues; 

    std::thread m_ListenThread;
    std::atomic<bool> m_running;
    bool m_listening;
    std::mutex m_ListenMutex;
    std::condition_variable m_ListenCV;
};

}

